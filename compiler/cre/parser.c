#include "parser.h"
#include <malloc.h>
#include <memory.h>
#include "../../util/linked_list.h"
#include "../../util/regex.h"
#include "../../util/misc.h"

struct cre_parsed_file *parse(char *source) {
    struct cre_parsed_file *result = NULL;
    void *state[2];
    state[0] = &state[1];
    linked_list_t *expressions = linked_list_create();
    while (*source) {
        linked_list_t *attributes = linked_list_create();
        char *line = source;
        struct __regex_find_result_t *find = __regex_find("\r?\n", line);
        if (find) {
            line[find->offset] = 0;
            source += find->offset + find->info.length;
            free(find);
        }
        while ((find = __regex_find("([a-zA-Z_]+)[ \t]*\\(([^()]*)\\)", line))) {
            struct cre_attribute a = {.label = find->info.se[0], .data = find->info.se[1]};
            free(find);
            linked_list_insert(attributes, 0, &a, sizeof(struct cre_attribute));
            line += find->offset + find->info.length;
        }
        find = __regex_find("([a-zA-Z_]+)[ \t]*@(.*)$", line);
        if (!find) {
            linked_list_destroy(attributes);
            continue;
        }
        const unsigned char *s = (const unsigned char *) find->info.se[1];
        linked_list_t *tokens = linked_list_create();
        for (size_t i = 0; i < find->info.length; ++i) {
            struct cre_token token = {.mode = CRE_ONE, .lazy = false};
            switch (s[i]) {
                case '^':
                    token.type = CRE_START;
                    break;
                case '$':
                    token.type = CRE_END;
                    break;
                case '.':
                    token.type = CRE_CHAR;
                    memset(token.filter, 0xFF, sizeof(token.filter));
                    break;
                case '[':
                    token.type = CRE_CHAR;
                    bool flag = s[++i] == '^';
                    if (flag)
                        ++i;
                    memset(token.filter, flag ? 0xFF : 0, sizeof(token.filter));
                    char last = 0;
                    while (s[i] && s[i] != ']') {
                        if (s[i] == '\\')
                            setFlag(token.filter, s[++i], flag);
                        else
                            setFlag(token.filter, s[i], flag);
                        ++i;
                    }
                    break;
                case '\\':
                    token.type = CRE_CHAR;
                    memset(token.filter, 0, sizeof(token.filter));
                    switch (s[++i]) {
                        case 'n':
                            setFlag(token.filter, '\n', true);
                            break;
                        case 'r':
                            setFlag(token.filter, '\r', true);
                            break;
                        case 't':
                            setFlag(token.filter, '\t', true);
                            break;
                        case '0':
                            setFlag(token.filter, '\0', true);
                            break;
                        default:
                            setFlag(token.filter, s[i], true);
                    }
                    break;
                default:
                    token.type = CRE_CHAR;
                    memset(token.filter, 0, sizeof(token.filter));
                    setFlag(token.filter, s[i], true);
                    break;
            }
            linked_list_insert(tokens, 0, &token, sizeof(struct cre_token));
        }
        struct cre_expression expr = {.token_count = linked_list_size(tokens), .attribute_count = linked_list_size(attributes)};
        linked_list_reverse(tokens);
        expr.tokens = malloc(sizeof(struct cre_token) * expr.token_count);
        state[1] = expr.tokens;
        linked_list_foreach(tokens, (struct delegate_t) {.func = &copy_to_array, .state = state});
        linked_list_destroy(tokens);
        linked_list_reverse(attributes);
        expr.attributes = malloc(sizeof(struct cre_attribute) * expr.attribute_count);
        state[1] = expr.attributes;
        linked_list_foreach(attributes, (struct delegate_t) {.func = &copy_to_array, .state = state});
        linked_list_destroy(attributes);
        expr.name = find->info.se[0];
        linked_list_insert(expressions, 0, &expr, sizeof(struct cre_expression));
    }
    linked_list_reverse(expressions);
    result = malloc(sizeof(struct cre_parsed_file));
    result->expression_count = linked_list_size(expressions);
    result->expressions = malloc(sizeof(struct cre_expression) * result->expression_count);
    state[1] = result->expressions;
    linked_list_foreach(expressions, (struct delegate_t) {.func = &copy_to_array, .state = state});
    end:
    free(source);
    linked_list_destroy(expressions);
    return result;
}
