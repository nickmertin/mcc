#include <stdlib.h>
#include <memory.h>
#include "misc.h"

char *strdup(const char *string) {
    if (!string)
        return NULL;
    char *r = malloc(strlen(string) + 1);
    strcpy(r, string);
    return r;
}

void setFlag(void *data, size_t flag, bool value) {
    char *f = (char *) data + flag / 8;
    *f &= ~(1 << (flag % 8));
    if (value)
        *f |= 1 << (flag % 8);
}

bool getFlag(void *data, size_t flag) {
    return (bool) (((char *) data)[flag / 8] & (1 << (flag % 8)));
}
