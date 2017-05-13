#ifndef ISU_CODE_GRAPH_H
#define ISU_CODE_GRAPH_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../util/linked_list.h"

struct cg_statement;

enum cg_var_size {
    CG_VOID,
    CG_BYTE,
    CG_WORD,
    CG_LONG,
    CG_QWORD,
};

struct cg_block {
    struct cg_statement *statements;
    size_t statement_count;
};

enum cg_expression_type {
    CG_VAR,
    CG_VALUE,
    CG_UNARY,
    CG_BINARY,
};

struct cg_expression_value {
    size_t value;
};

enum cg_expression_unary_type {
    CG_POSTINC,
    CG_POSTDEC,
    CG_PREINC,
    CG_PREDEC,
    CG_NEGATE,
    CG_LOGIC_NOT,
    CG_BITWISE_NOT,
    CG_DEREF,
    CG_ADDR,
};

struct cg_expression_unary {
    enum cg_expression_unary_type type : 8;
    size_t var;
};

enum cg_expression_binary_type {
    CG_ADD,
    CG_SUB,
    CG_MUL,
    CG_DIV,
    CG_MOD,
    CG_LOGIC_AND,
    CG_LOGIC_OR,
    CG_LOGIC_XOR,
    CG_BITWISE_AND,
    CG_BITWISE_OR,
    CG_BITWISE_XOR,
    CG_LT,
    CG_GT,
    CG_LTE,
    CG_GTE,
    CG_EQ,
    CG_NEQ,
};

struct cg_expression_binary {
    enum cg_expression_binary_type type : 8;
    size_t left_var;
    size_t right_var;
};

union cg_expression_data {
    size_t var;
    struct cg_expression_value value;
    struct cg_expression_unary unary;
    struct cg_expression_binary binary;
};

struct cg_expression {
    enum cg_expression_type type : 8;
    union cg_expression_data data;
};

enum cg_statement_type {
    CG_IFELSE,
    CG_JUMPIF,
    CG_JUMP,
    CG_LABEL,
    CG_ASSIGN,
    CG_CALL,
    CG_ENDFUNC,
    CG_CREATEVAR,
    CG_DESTROYVAR,
};

struct cg_statement_ifelse {
    size_t cond_var;
    struct cg_block if_true;
    struct cg_block if_false;
};

struct cg_statement_jumpif {
    size_t cond_var;
    const char *label;
};

struct cg_statement_jump {
    const char *label;
};

struct cg_statement_label {
    const char *label;
};

struct cg_statement_assign {
    size_t var;
    struct cg_expression expr;
};

struct cg_statement_call {
    const char *name;
    size_t *args;
    size_t arg_count;
    size_t out;
    enum cg_var_size out_size;
};

struct cg_statement_endfunc {
    size_t var;
};

struct cg_statement_createvar {
    size_t var;
    enum cg_var_size size;
};

struct cg_statement_destroyvar {
    size_t var;
};

union cg_statement_data {
    struct cg_statement_ifelse ifelse;
    struct cg_statement_jumpif jumpif;
    struct cg_statement_jump jump;
    struct cg_statement_label label;
    struct cg_statement_assign assign;
    struct cg_statement_call call;
    struct cg_statement_endfunc endfunc;
    struct cg_statement_createvar createvar;
    struct cg_statement_destroyvar destroyvar;
};

struct cg_statement {
    enum cg_statement_type type : 8;
    union cg_statement_data data;
};

struct cg_function {
    const char *name;
    uint8_t access_level;
    enum cg_var_size *args;
    size_t arg_count;
    enum cg_var_size return_type;
    struct cg_block body;
};

struct cg_file_graph {
    struct cg_function *functions;
    size_t  function_count;
};

struct cg_block_builder {
    size_t start_index;
    size_t *next_var;
    bool root;
    linked_list_t *statements;
};

void block_builder_create_root(struct cg_block_builder *builder, size_t parameter_count);

void block_builder_create_child(struct cg_block_builder *builder, struct cg_block_builder *parent);

void block_builder_add_statement(struct cg_block_builder *builder, struct cg_statement statement);

void block_builder_merge_block(struct cg_block_builder *builder, struct cg_block block);

size_t block_builder_create_variable(struct cg_block_builder *builder, enum cg_var_size size);

void block_builder_destroy_variable(struct cg_block_builder *builder, size_t variable);

void block_builder_end(struct cg_block_builder *builder, struct cg_block *out);

#endif //ISU_CODE_GRAPH_H
