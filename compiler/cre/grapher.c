#include "grapher.h"
#include <malloc.h>
#include "../../util/linked_list.h"
#include "../../util/misc.h"

static size_t unique_id = 0;

static size_t flag_count(void *data, size_t length) {

}

static struct cg_block generate_jump_block(struct cg_block_builder *parent, char *label) {
    struct cg_block_builder builder;
    block_builder_create_child(&builder, parent);
    block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_JUMP, .data.jump.label = label});
    struct cg_block block;
    block_builder_end(&builder, &block);
    return block;
}

static struct cg_block generate_return_block(struct cg_block_builder *parent) {
    struct cg_block_builder builder;
    block_builder_create_child(&builder, parent);
    block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ENDFUNC});
    struct cg_block block;
    block_builder_end(&builder, &block);
    return block;
}

static struct cg_block generate_return_var_block(struct cg_block_builder *parent, size_t result_var) {
    struct cg_block_builder builder;
    block_builder_create_child(&builder, parent);
    block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ENDFUNC, .data.endfunc.var = result_var});
    struct cg_block block;
    block_builder_end(&builder, &block);
    return block;
}

static struct cg_block generate_return_value_block(struct cg_block_builder *parent, size_t result, enum cg_var_size size) {
    struct cg_block_builder builder;
    block_builder_create_child(&builder, parent);
    size_t result_var = block_builder_create_variable(&builder, size);
    block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ASSIGN, .data.assign = {.var = result_var, .expr = {.type = CG_VAR, .data.value.value =  result}}});
    block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ENDFUNC, .data.endfunc.var = result_var});
    struct cg_block block;
    block_builder_end(&builder, &block);
    return block;
}

static struct cg_block *generate_match_function(struct cre_token *tokens, size_t token_count) {
    struct cg_block *result = NULL;
    struct cg_block_builder builder;
    block_builder_create_root(&builder, 1);
    size_t var1 = block_builder_create_variable(&builder, CG_LONG);
    block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ASSIGN, .data.assign = {.var = var1, .expr = {.type = CG_VALUE, .data.value.value = 1}}});
    char label[18];
    for (size_t i = 0; i < token_count; ++i) {
        switch (tokens[i].type) {
            case CRE_CHAR:
                sprintf(label, "_%lx", unique_id++);
                for (size_t j = 0; j < 256; ++j) {
                    if (getFlag(tokens[i].filter, j)) {
                        size_t character = block_builder_create_variable(&builder, CG_BYTE), comparison = block_builder_create_variable(&builder, CG_BYTE);
                        block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ASSIGN, .data.assign = {.var = character, .expr = {.type = CG_UNARY, .data.unary = {.var = 2, .type = CG_DEREF}}}});
                        block_builder_destroy_variable(&builder, character);
                        block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ASSIGN, .data.assign = {.var = comparison, .expr = {.type = CG_VALUE, .data.value.value = j}}});
                        block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ASSIGN, .data.assign = {.var = comparison, .expr = {.type = CG_BINARY, .data.binary = {.type = CG_EQ, .left_var = 3, .right_var = 4}}}});
                        block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_IFELSE, .data.ifelse = {.cond_var = comparison, .if_true = generate_jump_block(&builder, strdup(label)), .if_false = { .statement_count = 0 }}});
                        block_builder_destroy_variable(&builder, comparison);
                    }
                }
                block_builder_merge_block(&builder, generate_return_value_block(&builder, 0, CG_BYTE));
                block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_LABEL, .data.label.label = strdup(label)});
                break;
        }
        block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ASSIGN, .data.assign = {.var = 0, .expr = {.type = CG_BINARY, .data.binary = {.type = CG_ADD, .left_var = 0, .right_var = var1}}}});
    }
    result = malloc(sizeof(struct cg_block));
    block_builder_end(&builder, result);
    return result;
}

static void map_to_match_function(struct cg_function *out, struct cg_block *block, const char *name, uint8_t access) {
    out->name = name;
    out->access_level = access;
    out->args = malloc(sizeof(enum cg_var_size));
    out->args[0] = CG_LONG;
    out->arg_count = 1;
    out->return_type = CG_LONG;
    out->body = *block;
}

struct cg_file_graph *graph(struct cre_parsed_file *file) {
    struct cg_file_graph *result = NULL;
    linked_list_t *functions = linked_list_create();
    for (size_t i = 0; i < file->expression_count; ++i) {
        struct cre_expression *expr = &file->expressions[i];
        struct cg_block *match = NULL, *find = NULL, *replace = NULL;
        for (size_t j = 0; j < expr->attribute_count; ++j) {
            struct cre_attribute *attr = &expr->attributes[j];
            if (!strcmp(attr->label, "match")) {
                struct cg_function function;
                if (!match)
                    match = generate_match_function(expr->tokens, expr->token_count);
                map_to_match_function(&function, match, attr->data, 0);
                linked_list_insert(functions, 0, &function, sizeof(struct cg_function));
            }
        }
        if (match)
            free(match);
    }
    result = malloc(sizeof(struct cg_file_graph));
    result->function_count = linked_list_size(functions);
    result->functions = malloc(sizeof(struct cg_function) * result->function_count);
    copy_to_array(functions, result->functions, sizeof(struct cg_function));
    linked_list_destroy(functions);
    return result;
}
