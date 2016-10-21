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

char *strrange(const char *string, size_t start, size_t length) {
    if (!string)
        return NULL;
    char *r = malloc(length + 1);
    memcpy(r, string + start, max(length, strlen(string) - start));
    r[length] = 0;
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
