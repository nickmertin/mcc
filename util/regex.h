#ifndef ISU_REGEX_H
#define ISU_REGEX_H

#ifndef __MCC__

struct __regex_result_t {
    size_t length, se_count;
    const char *se[0];
};

char *__regex_replace(const char *expr, const char *text, const char *value);

const char *__regex_find(const char *expr, const char *text);

struct __regex_result_t *__regex_match(const char *expr, const char *text);

#endif

#endif //ISU_REGEX_H
