#include <stdio.h>
#include <ctype.h>
#include "preprocessor.h"
#include "../../util/buffer.h"
#include "../../util/misc.h"

struct macro_t {
    char *params;
    char *content;
};

char *resolve_macro(char *macro, char **args) {

}

char *read_file(const char *path, add_pragma_t adder, struct config_t *config) {
    FILE *f = fopen(path, "r");
    buffer_t *b = buffer_create();
    int l = 0, mode = 0, pmode = 0;
    buffer_t *cur = buffer_create();
    while (!feof(f)) {
        int c = fgetc(f);
        switch (mode) {
            case 0:
                if (c == '#' && l == '\n')
                    mode = pmode = 1;
                else if (c == '/' && l == '/')
                    mode = 2;
                else if (c == '*' && l == '/')
                    mode = 3;
                else if (c == '"')
                    mode = 4;
                else if (c == '\'')
                    mode = 5;
                if (isalnum_(c))
                    buffer_append_one(cur, (char) c);
                break;
            case 1:
                if (c == '\\')
                    continue;
                if (c == '\n') {
                    if (l != '\\')
                        mode = pmode = 0;
                    continue;
                }
                if (isalnum_(c))
                    buffer_append_one(cur, (char) c);
                else if (!buffer_empty(cur)) {
                    buffer_defrag(cur);
                    buffer_append(b, (char *) ((*cur) + 1), (*cur)->length);
                    buffer_destroy(cur);
                    cur = buffer_create();
                }
                break;
        }
        if (mode == 0 || mode == 3 || mode == 4)
            buffer_append_one(b, (char) c);
        l = c;
    }
}
