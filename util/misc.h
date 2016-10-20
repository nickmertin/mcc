#ifndef ISU_MISC_H
#define ISU_MISC_H

#include <stdbool.h>

#define isalnum_(c) (isalnum(c) || c == '_')

#define swap(x, y) { x ^= y; y ^= x; x ^= y;}

char *strdup(const char *string);

void setFlag(void *data, size_t flag, bool value);

bool getFlag(void *data, size_t flag);

#endif //ISU_MISC_H
