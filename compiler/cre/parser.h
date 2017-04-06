#ifndef ISU_PARSER_H
#define ISU_PARSER_H

#include <stdbool.h>
#include <memory.h>

enum cre_token_type {
    CRE_CHAR,
    CRE_START,
    CRE_END,
};

enum cre_token_mode {
    CRE_ONE,
    CRE_RANGE,
    CRE_MULTIPLE,
    CRE_MANY,
};

struct cre_attribute {
    const char *label;
    const char *data;
};

struct cre_token {
    enum cre_token_type type : 4;
    enum cre_token_mode mode : 3;
    bool lazy : 1;
    char filter[32];
};

struct cre_expression {
    const char *name;
    struct cre_attribute *attributes;
    size_t attribute_count;
    struct cre_token *tokens;
    size_t token_count;
};

struct cre_parsed_file {
    struct cre_expression *expressions;
    size_t expression_count;
};

struct cre_parsed_file *parse(const char *source);

#endif //ISU_PARSER_H
