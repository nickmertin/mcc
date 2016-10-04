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
