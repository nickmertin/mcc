#include <stddef.h>
#include "regex.h"

struct regex_element_t {
    enum { RE_ANY, RE_CHARSET, RE_START, RE_END, RE_SUBEXPR, RE_SUBEXPRREF, RE_NUMSET, RE_ONETWO, RE_MANY, RE_MANYPLUS, RE_CHOICE } type;
    void *data;
};

char *__regex_replace(const char *expr, const char *text, const char *value) {

}

const char *__regex_find(const char *expr, const char *text);

bool __regex_match(const char *expr, const char *text) {
    struct regex_element_t *current = NULL;
    for (size_t i = 0; expr[i]; ++i) {
        switch(expr[i]) {
            case '.':

                break;
        }
    }
}
