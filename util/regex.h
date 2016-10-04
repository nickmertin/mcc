#ifndef ISU_REGEX_H
#define ISU_REGEX_H

#ifndef __MCC__

#include <stdbool.h>

char *__regex_replace(const char *expr, const char *text, const char *value);

const char *__regex_find(const char *expr, const char *text);

bool __regex_match(const char *expr, const char *text);

#endif

#endif //ISU_REGEX_H
