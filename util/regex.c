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
    RE_CHOICE,
    RE_CHAR,
};

struct regex_element_t {
    enum regex_element_type_t type;
    char data[0];
};

static inline size_t element_size(struct regex_element_t *element) {

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
                                goto subexpr_error;
                            char begin = last, end = expr[++i];
                            if (begin > end) {
                                swap(begin, end)
                                --begin;
                                --end;
                            }
                            size_t len = (size_t) (end - begin);
                            char *chars = malloc(len);
                            for (size_t j = 0; j < len; ++j)
                                chars[j] = (char) (begin + j + 1);
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
                    goto subexpr_error;
                buffer_defrag(buffer);
                current = malloc(sizeof(struct regex_element_t) + 64);
                current->type = RE_CHARSET;
                memset(current->data, negated, 64);
                struct buffer_iterator_t iterator = buffer_iterate(buffer);
                if (last)
                    do
                        setFlag(current->data, (size_t) *iterator.current, !negated);
                    while (buffer_iterator_next(&iterator));
                break;
                subexpr_error:
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
                }
                if (n >= 0)
                    goto error;
                char *inner = malloc(len);
                memcpy(inner, expr - len, len - 1);
                inner[len - 1] = 0;
                current = malloc(sizeof(struct regex_element_t) + sizeof(linked_list_t *));
                current->type = RE_SUBEXPR;
                *(linked_list_t **) current->data = parse(inner);
                free(inner);
                if (!*(uintptr_t *) current->data)
                    goto error;
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
                unsigned x = 0, y = 0, *c = &x;
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
                current = malloc(sizeof(struct regex_element_t) + sizeof(unsigned) * 2);
                current->type = RE_NUMSET;
                c = (unsigned *) current->data;
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
        linked_list_insert(list, 0, current, element_size(current));
        free(current);
        current = NULL;
    }
    linked_list_reverse(list);
    linked_list_reverse(master);
    return master;
    error:
    linked_list_destroy(list);
    if (current)
        free(current);
    linked_list_foreach(master, (void (*)(void *)) &linked_list_destroy);
    return NULL;
}

char *__regex_replace(const char *expr, const char *text, const char *value) {

}

const char *__regex_find(const char *expr, const char *text) {

}

bool __regex_match(const char *expr, const char *text) {
    linked_list_t *components = parse(expr);
}
