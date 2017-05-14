#ifndef ISU_MISC_H
#define ISU_MISC_H

#include <stdbool.h>
#include "linked_list.h"

#define isalnum_(c) (isalnum(c) || (c) == '_')

#define swap(x, y) ((x) ^= (y), (y) ^= (x), (x) ^= (y))

#define max(x, y) ((x) > (y) ? (x) : (y))

#define min(x, y) ((x) > (y) ? (y) : (x))

#define sgn(x) (((x) > 0) - (0 > (x)))

char *strdup(const char *string);

char *strrange(const char *string, size_t start, size_t length);

void setFlag(void *data, size_t flag, bool value);

bool getFlag(void *data, size_t flag);

void copy_to_array(linked_list_t *list, void *array, size_t step);

bool contains_size_t(linked_list_t *list, size_t value);

#endif //ISU_MISC_H
