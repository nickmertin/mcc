#ifndef ISU_CODE_GRAPH_H
#define ISU_CODE_GRAPH_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct cg_statement;

enum cg_var_size {
    CG_BYTE,
    CG_WORD,
    CG_LONG,
    CG_QWORD,
};

struct cg_var {
    size_t id;
    enum cg_var_size size : 8;
};

struct cg_block {
    bool new_namespace;
    struct cg_statement *statemets;
    size_t statement_count;
};

enum cg_expression_type {
    CG_VAR,
    CG_VALUE,
    CG_UNARY,
    CG_BINARY,
    CG_TERNARY,
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
    struct cg_var var;
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
    struct cg_var left_var;
    struct cg_var right_var;
};

struct cg_expression_ternary {
    struct cg_var cond_var;
    struct cg_var true_var;
    struct cg_var false_var;
};

union cg_expression_data {
    struct cg_var var;
    struct cg_expression_value value;
    struct cg_expression_unary unary;
    struct cg_expression_binary binary;
    struct cg_expression_ternary ternary;
};

struct cg_expression {
    enum cg_expression_type type : 8;
    union cg_expression_data data;
};

enum cg_statement_type {
    CG_IFELSE,
    CG_JUMP,
    CG_LABEL,
    CG_ASSIGN,
    CG_CALL,
    CG_ENDFUNC,
};

struct cg_statement_ifelse {
    struct cg_var cond_var;
    struct cg_block if_true;
    struct cg_block if_false;
};

struct cg_statement_jump {
    const char *label;
};

struct cg_statement_label {
    const char *label;
};

struct cg_statement_assign {
    struct cg_var var;
    struct cg_expression expr;
};

struct cg_statement_call {
    const char *name;
    struct cg_var *args;
    size_t arg_count;
    struct cg_var out;
};

struct cg_statement_endfunc {
    struct cg_var var;
};

union cg_statement_data {
    struct cg_statement_ifelse ifelse;
    struct cg_statement_jump jump;
    struct cg_statement_label label;
    struct cg_statement_assign assign;
    struct cg_statement_call call;
    struct cg_statement_endfunc endfunc;
};

struct cg_statement {
    enum cg_statement_type type : 8;
    union cg_statement_data data;
};

struct cg_function {
    const char *name;
    uint8_t access_level;
    size_t arg_count;
    struct cg_block body;
};

struct cg_file_graph {
    struct cg_function *functions;
    size_t  function_count;
};

#endif //ISU_CODE_GRAPH_H
