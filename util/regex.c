#include <stddef.h>
#include <malloc.h>
#include <memory.h>
#include "regex.h"
#include "linked_list.h"
#include "buffer.h"
#include "misc.h"

enum regex_element_type_t {
    RE_ANY,
    RE_CHARSET,
    RE_START,
    RE_END,
    RE_SUBEXPR,
    RE_SUBEXPRREF,
    RE_NUMSET,
    RE_MAYBE,
    RE_MULTIPLE,
    RE_MANY,
    RE_CHAR,
};

enum regex_mode_t {
    MODE_NORMAL,
    MODE_MAYBE,
    MODE_MULTIPLE,
    MODE_MANY,
    MODE_LAZY,
};

struct regex_element_t {
    enum regex_element_type_t type;
    char data[0];
};

static inline size_t element_size(struct regex_element_t *element) {
    switch (element->type) {
        case RE_ANY:
        case RE_START:
        case RE_END:
        case RE_MAYBE:
        case RE_MULTIPLE:
        case RE_MANY:
            return sizeof(struct regex_element_t);
        case RE_CHARSET:
            return sizeof(struct regex_element_t) + 64;
        case RE_SUBEXPR:
            return sizeof(struct regex_element_t) + sizeof(linked_list_t);
        case RE_SUBEXPRREF:
        case RE_CHAR:
            return sizeof(struct regex_element_t) + 1;
        case RE_NUMSET:
            return sizeof(struct regex_element_t) + sizeof(size_t) * 2;
        default:
            return 0;
    }
}

static linked_list_t *parse(const char *expr) {
    struct regex_element_t *current = NULL;
    linked_list_t *master = linked_list_create(), *list = linked_list_create();
    for (size_t i = 0; expr[i]; ++i) {
        switch (expr[i]) {
            case '.':
                current = malloc(sizeof(struct regex_element_t));
                current->type = RE_ANY;
                break;
            case '[': {
                buffer_t *buffer = buffer_create();
                bool negated;
                negated = expr[i + 1] == '^';
                if (negated)
                    ++i;
                char last = 0;
                while (expr[++i] && expr[i] != ']') {
                    switch (expr[i]) {
                        case '\\':
                            buffer_append_one(buffer, last = expr[++i]);
                            break;
                        case '-': {
                            if (!last)
                                goto charset_error;
                            char begin = last, end = expr[++i];
                            if (begin > end) {
                                swap(begin, end);
                                --begin;
                                --end;
                            }
                            size_t len = (size_t) (end - begin);
                            char *chars = malloc(len);
                            for (size_t j = 1; j <= len; ++j)
                                chars[j - 1] = (char) (begin + j);
                            buffer_append(buffer, chars, len);
                            free(chars);
                            break;
                        }
                        default:
                            buffer_append_one(buffer, last = expr[i]);
                            break;
                    }
                }
                if (!expr[i])
                    goto charset_error;
                //buffer_defrag(buffer);
                current = malloc(sizeof(struct regex_element_t) + 64);
                current->type = RE_CHARSET;
                memset(current->data, negated * 255, 64);
                struct buffer_iterator_t iterator = buffer_iterate(buffer);
                if (last)
                    do
                        setFlag(current->data, (size_t) *iterator.current, !negated);
                    while (buffer_iterator_next(&iterator));
                break;
                charset_error:
                buffer_destroy(buffer);
                goto error;
            }
            case '^':
            case '$':
                current = malloc(sizeof(struct regex_element_t));
                current->type = expr[i] == '^' ? RE_START : RE_END;
                break;
            case '(': {
                int n = 0;
                size_t len = 0;
                while (expr[++i] && n >= 0) {
                    if (expr[i] == '(')
                        ++n;
                    else if (expr[i] == ')')
                        --n;
                    ++len;
                    if (expr[i] == '\\' && expr[i + 1]) {
                        ++i;
                        ++len;
                    }
                }
                if (n >= 0)
                    goto error;
                char *inner = malloc(len);
                memcpy(inner, expr + i - len, len - 1);
                inner[len - 1] = 0;
                current = malloc(sizeof(struct regex_element_t) + sizeof(linked_list_t));
                current->type = RE_SUBEXPR;
                linked_list_t  *inner_list = parse(inner);
                if (!inner_list)
                    goto error;
                memcpy(current->data, inner_list, sizeof(linked_list_t));
                free(inner);
                free(inner_list);
                --i;
                break;
            }
            case '\\':
                current = malloc(sizeof(struct regex_element_t) + 1);
                if (expr[++i] >= '1' && expr[i] <= '9') {
                    current->type = RE_SUBEXPRREF;
                    current->data[0] = (char) (expr[i] - '1');
                } else if (expr[i]) {
                    current->type = RE_CHAR;
                    switch (expr[i]) {
                        case 'n':
                            current->data[0] = '\n';
                            break;
                        case 't':
                            current->data[0] = '\t';
                            break;
                        default:
                            current->data[0] = expr[i];
                            break;
                    }
                    current->data[0] = expr[i];
                } else
                    goto error;
                break;
            case '{': {
                size_t x = 0, y = 0, *c = &x;
                while (expr[++i] && expr[i] != '}')
                    if (expr[i] >= '0' && expr[i] <= '9') {
                        *c *= 10;
                        *c += expr[i] - '0';
                    } else if (expr[i] == ',')
                        c = &y;
                    else
                        goto error;
                if (x > y)
                    goto error;
                current = malloc(sizeof(struct regex_element_t) + sizeof(size_t) * 2);
                current->type = RE_NUMSET;
                c = (size_t *) current->data;
                c[0] = x;
                c[1] = y;
                break;
            }
            case '?':
            case '+':
            case '*':
                current = malloc(sizeof(struct regex_element_t));
                current->type = expr[i] == '?' ? RE_MAYBE : expr[i] == '+' ? RE_MULTIPLE : RE_MANY;
                break;
            case '|':
                linked_list_reverse(list);
                linked_list_insert(master, 0, list, sizeof(linked_list_t));
                free(list);
                list = linked_list_create();
                break;
            default:
                current = malloc(sizeof(struct regex_element_t) + 1);
                current->type = RE_CHAR;
                current->data[0] = expr[i];
                break;
        }
        if (current) {
            linked_list_insert(list, 0, current, element_size(current));
            free(current);
            current = NULL;
        }
    }
    linked_list_reverse(list);
    linked_list_insert(master, 0, list, sizeof(linked_list_t));
    linked_list_reverse(master);
    return master;
    error:
    linked_list_destroy(list);
    if (current)
        free(current);
    linked_list_foreach(master, delegate_wrap((void (*)(void *)) &linked_list_destroy));
    linked_list_destroy(master);
    return NULL;
}

static void regex_match_internal(linked_list_t *data, void **state);

static struct __regex_result_t *regex_match_element(const char *text, struct linked_list_node_t *node, size_t  offset, const char **subexpressions) {
    if (!node) {
        size_t se_count = 0;
        const char **se = subexpressions;
        while (*(se++))
            ++se_count;
        struct __regex_result_t *result = malloc(sizeof(struct __regex_result_t) + sizeof(const char *) * se_count);
        result->length = offset;
        result->se_count = se_count;
        memcpy(result->se, subexpressions, sizeof(const char *) * se_count);
        return result;
    }
    size_t len = strlen(text);
    struct linked_list_node_t *next = node->ptr;
    struct regex_element_t *element = (struct regex_element_t *) node->data, *next_e = next ? (struct regex_element_t *) next->data : NULL;
    switch (element->type) {
        case RE_START:
            return offset ? NULL : regex_match_element(text, next, offset, subexpressions);
        case RE_NUMSET:
        case RE_MAYBE:
        case RE_MANY:
        case RE_MULTIPLE:
            return NULL;
        default: {
            enum regex_mode_t mode = MODE_NORMAL;
            if (next) {
                if (next_e->type == RE_MAYBE)
                    mode = MODE_MAYBE;
                else if (next_e->type == RE_MULTIPLE)
                    mode = MODE_MULTIPLE;
                else if (next_e->type == RE_MANY)
                    mode = MODE_MANY;
                else
                    goto cont;
                if ((next = next->ptr)) {
                    next_e = (struct regex_element_t *) next->data;
                    if (next_e->type == RE_MAYBE) {
                        mode |= MODE_LAZY;
                        next = next->ptr;
                    }
                }
            }
            cont:
            switch (element->type) {
                case RE_ANY:
                    if ((mode == MODE_NORMAL || mode & ~MODE_LAZY == MODE_MULTIPLE) && !*text)
                        return NULL;
                    if (mode & ~MODE_LAZY == MODE_MULTIPLE) {
                        ++offset;
                        ++text;
                        --len;
                    }
                    if (mode & ~MODE_LAZY == MODE_MULTIPLE || mode & ~MODE_LAZY == MODE_MANY) {
                        if (mode & MODE_LAZY)
                            for (size_t i = 0; i < len + 1; ++i) {
                                struct __regex_result_t *result = regex_match_element(text + i, next, offset + i, subexpressions);
                                if (result)
                                    return result;
                            }
                        else
                            for (size_t i = len + 1; i > 0; --i) {
                                struct __regex_result_t *result = regex_match_element(text + i - 1, next, offset + i - 1, subexpressions);
                                if (result)
                                    return result;
                            }
                        return NULL;
                    }
                    if (*text) {
                        if (mode & MODE_MAYBE == MODE_MAYBE && mode & MODE_LAZY) {
                            struct __regex_result_t *result = regex_match_element(text, next, offset, subexpressions);
                            if (result)
                                return result;
                        }
                        return regex_match_element(text + 1, next, offset + 1, subexpressions);
                    }
                    mode &= ~MODE_LAZY;
                    return mode == MODE_MAYBE ? regex_match_element(text, next, offset, subexpressions) : NULL;
                case RE_CHARSET:
                    if ((mode == MODE_NORMAL || mode & ~MODE_LAZY == MODE_MULTIPLE) && !(*text && getFlag(element->data, (size_t) *text)))
                        return NULL;
                    if (mode & ~MODE_LAZY == MODE_MULTIPLE) {
                        ++offset;
                        ++text;
                        --len;
                    }
                    if (mode & ~MODE_LAZY == MODE_MULTIPLE || mode & ~MODE_LAZY == MODE_MANY) {
                        for (size_t i = 0; i < len; ++i)
                            if (!getFlag(element->data, (size_t) text[i])) {
                                len = i;
                                break;
                            }
                        if (mode & MODE_LAZY)
                            for (size_t i = 0; i < len + 1; ++i) {
                                struct __regex_result_t *result = regex_match_element(text + i, next, offset + i, subexpressions);
                                if (result)
                                    return result;
                            }
                        else
                            for (size_t i = len + 1; i > 0; --i) {
                                struct __regex_result_t *result = regex_match_element(text + i - 1, next, offset + i - 1, subexpressions);
                                if (result)
                                    return result;
                            }
                    }
                    if (*text && getFlag(element->data, (size_t) *text)) {
                        if (mode & MODE_MAYBE == MODE_MAYBE && mode & MODE_LAZY) {
                            struct __regex_result_t *result = regex_match_element(text, next, offset, subexpressions);
                            if (result)
                                return result;
                        }
                        return regex_match_element(text + 1, next, offset + 1, subexpressions);
                    }
                    mode &= ~MODE_LAZY;
                    return mode & MODE_MAYBE ? regex_match_element(text, next, offset, subexpressions) : NULL;
                case RE_END:
                    return len || node->ptr ? NULL : regex_match_element(text, NULL, offset, subexpressions);
                case RE_SUBEXPR: {
                    struct __regex_result_t *result = NULL;
                    void *data[2] = {(void *) text, &result};
                    linked_list_foreach((linked_list_t *) element->data, (struct delegate_t) {.func = (void (*)(void *, void *)) &regex_match_internal, .state = data});
                    if (!result)
                        return NULL;
                    size_t se_count = 0;
                    const char **se = subexpressions;
                    while (*(se++))
                        ++se_count;
                    se = malloc((se_count + 2) * sizeof(const char *));
                    memcpy(se, subexpressions, se_count * sizeof(const char *));
                    se[se_count] = strrange(text, 0, result->length);
                    se[se_count + 1] = NULL;
                    free(subexpressions);
                    return regex_match_element(text + result->length, next, offset + result->length, se);
                }
                case RE_SUBEXPRREF: {
                    const char *se = subexpressions[element->data[0]];
                    size_t se_len = strlen(se);
                    if ((mode == MODE_NORMAL || mode & ~MODE_LAZY == MODE_MULTIPLE) && !(se_len > len || memcmp(se, text, se_len)))
                        return NULL;
                    if (mode & ~MODE_LAZY == MODE_MULTIPLE) {
                        ++offset;
                        ++text;
                        --len;
                    }
                    if (mode & ~MODE_LAZY == MODE_MULTIPLE || mode & ~MODE_LAZY == MODE_MANY) {
                        for (size_t i = 0; i < len; i += se_len)
                            if (se_len > len - i || memcmp(se, text, se_len)) {
                                len = i;
                                break;
                            }
                        if (mode & MODE_LAZY)
                            for (size_t i = 0; i < len + 1; i += se_len) {
                                struct __regex_result_t *result = regex_match_element(text + i + se_len - 1, next, offset + i + se_len - 1, subexpressions);
                                if (result)
                                    return result;
                            }
                        else
                            for (size_t i = len + 1; i >= se_len; i -= se_len) {
                                struct __regex_result_t *result = regex_match_element(text + i - se_len, next, offset + i - se_len, subexpressions);
                                if (result)
                                    return result;
                            }
                        return NULL;
                    }
                    if (se_len <= len && !memcmp(se, text, se_len)) {
                        if (mode & MODE_MAYBE == MODE_MAYBE && mode & MODE_LAZY) {
                            struct __regex_result_t *result = regex_match_element(text, next, offset, subexpressions);
                            if (result)
                                return result;
                        }
                        return regex_match_element(text + se_len, next, offset + se_len, subexpressions);
                    }
                    mode &= ~MODE_LAZY;
                    return mode & MODE_MAYBE ? regex_match_element(text, next, offset, subexpressions) : NULL;
                }
                case RE_CHAR:
                    if ((mode == MODE_NORMAL || mode & ~MODE_LAZY == MODE_MULTIPLE) && *text != element->data[0])
                        return NULL;
                    if (mode & ~MODE_LAZY == MODE_MULTIPLE) {
                        ++offset;
                        ++text;
                        --len;
                    }
                    if (mode & ~MODE_LAZY == MODE_MULTIPLE || mode & ~MODE_LAZY == MODE_MANY) {
                        for (size_t i = 0; i < len; ++i)
                            if (text[i] != element->data[0]) {
                                len = i;
                                break;
                            }
                        if (mode & MODE_LAZY)
                            for (size_t i = 0; i < len + 1; ++i) {
                                struct __regex_result_t *result = regex_match_element(text + i, next, offset + i, subexpressions);
                                if (result)
                                    return result;
                            }
                        else
                            for (size_t i = len + 1; i > 0; --i) {
                                struct __regex_result_t *result = regex_match_element(text + i - 1, next, offset + i - 1, subexpressions);
                                if (result)
                                    return result;
                            }
                        return NULL;
                    }
                    if (*text == element->data[0]) {
                        if (mode & MODE_MAYBE == MODE_MAYBE && mode & MODE_LAZY) {
                            struct __regex_result_t *result = regex_match_element(text, next, offset, subexpressions);
                            if (result)
                                return result;
                        }
                        return regex_match_element(text + 1, next, offset + 1, subexpressions);
                    }
                    mode &= ~MODE_LAZY;
                    return mode & MODE_MAYBE ? regex_match_element(text, next, offset, subexpressions) : NULL;
            }
        }
    }
    return NULL;
}

static void regex_match_internal(linked_list_t *data, void **state) {
    const char *text = state[0];
    struct __regex_result_t **result = state[1];
    if (*result)
        return;
    const char **se = malloc(sizeof(const char *));
    *se = NULL;
    *result = regex_match_element(text, *data, 0, se);
    linked_list_destroy(data);
}

static void add(char **data, size_t *state) {
    *state += strlen(*data);
}

static void concat(char **data, char *state) {
    strcat(state, *data);
}

static void free_target(void **data) {
    free(*data);
}

char *__regex_replace(const char *expr, const char *text, const char *value) {
    linked_list_t *list = linked_list_create();
    struct __regex_find_result_t *result;
    char *segment;
    while (*text && (result = __regex_find(expr, text))) {
        if (result->string != text) {
            segment = strrange(text, 0, result->offset);
            linked_list_insert(list, 0, &segment, sizeof(char *));
        }
        segment = strdup(value);
        linked_list_insert(list, 0, &segment, sizeof(char *));
        text += result->offset + result->info.length;
    }
    if (*text)
        linked_list_insert(list, 0, &text, sizeof(char *));
    linked_list_reverse(list);
    size_t len = 0;
    linked_list_foreach(list, (struct delegate_t) {.func = (void (*)(void *, void *)) &add, .state = &len});
    char *output = malloc(len + 1);
    *output = 0;
    linked_list_foreach(list, (struct delegate_t) {.func = (void (*)(void *, void *)) &concat, .state = output});
    linked_list_foreach(list, delegate_wrap((void (*)(void *)) &free_target));
    linked_list_destroy(list);
    return output;
}

struct __regex_find_result_t *__regex_find(const char *expr, const char *text) {
    size_t off = 0;
    struct __regex_result_t *result;
    while (!(result = __regex_match(expr, text)))
        if (!(off++, text++))
            return NULL;
    struct __regex_find_result_t *fr = malloc(sizeof(struct __regex_find_result_t) + sizeof(const char *) * result->se_count);
    fr->string = text;
    memcpy(&fr->info, result, sizeof(struct __regex_result_t) + sizeof(const char *) * result->se_count);
    free(result);
    fr->offset = off;
    return fr;
}

struct __regex_result_t *__regex_match(const char *expr, const char *text) {
    linked_list_t *components = parse(expr);
    if (!components)
        return NULL;
    struct __regex_result_t *result = NULL;
    void *data[2] = {(void *) text, &result};
    linked_list_foreach(components, (struct delegate_t) {.func = (void (*)(void *, void *)) &regex_match_internal, .state = data});
    linked_list_destroy(components);
    return result;
}
