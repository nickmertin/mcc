#include "grapher.h"
#include <malloc.h>
#include "../../util/misc.h"

static size_t unique_id = 0;

static struct cg_block generate_jump_block(struct cg_block_builder *parent, char *label) {
    struct cg_block_builder builder;
    block_builder_create_child(&builder, parent);
    block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_JUMP, .data.jump.label = label});
    struct cg_block block;
    block_builder_end(&builder, &block);
    return block;
}

static struct cg_block generate_return_value_block(struct cg_block_builder *parent, size_t result, enum cg_var_size size) {
    struct cg_block_builder builder;
    block_builder_create_child(&builder, parent);
    size_t result_var = block_builder_create_variable(&builder, size);
    block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ASSIGN, .data.assign = {.var = result_var, .expr = {.type = CG_VALUE, .data.value.value =  result}}});
    block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ENDFUNC, .data.endfunc.var = result_var});
    block_builder_destroy_variable(&builder, result_var);
    struct cg_block block;
    block_builder_end(&builder, &block);
    return block;
}

static struct cg_block *generate_match_function(struct cre_token *tokens, size_t token_count) {
    struct cg_block *result = NULL;
    struct cg_block_builder builder;
    block_builder_create_root(&builder, 1);
    char label[18];
    for (size_t i = 0; i < token_count; ++i) {
        switch (tokens[i].type) {
            case CRE_CHAR: {
                sprintf(label, "_%lx", unique_id++);
                size_t character = block_builder_create_variable(&builder, CG_BYTE), comparison = block_builder_create_variable(&builder, CG_BYTE);
                block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ASSIGN, .data.assign = {.var = character, .expr = {.type = CG_UNARY, .data.unary = {.type = CG_DEREF, .var = 0}}}});
                block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_JUMPIF, .data.jumpif = {.cond_var = character, .label = strdup(label)}});
                block_builder_merge_block(&builder, generate_return_value_block(&builder, 0, CG_BYTE));
                block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_LABEL, .data.label.label = strdup(label)});
                sprintf(label, "_%lx", unique_id++);
                for (size_t j = 0; j < 256; ++j) {
                    if (getFlag(tokens[i].filter, j)) {
                        block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ASSIGN, .data.assign = {.var = comparison, .expr = {.type = CG_VALUE, .data.value.value = j}}});
                        block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ASSIGN, .data.assign = {.var = comparison, .expr = {.type = CG_BINARY, .data.binary = {.type = CG_EQ, .left_var = comparison, .right_var = character}}}});
                        block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_JUMPIF, .data.jumpif = {.cond_var = comparison, .label = strdup(label)}});
                    }
                }
                block_builder_destroy_variable(&builder, character);
                block_builder_destroy_variable(&builder, comparison);
                block_builder_merge_block(&builder, generate_return_value_block(&builder, 0, CG_BYTE));
                block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_LABEL, .data.label.label = strdup(label)});
                break;
            }
            case CRE_START:
                break;
            case CRE_END: {
                size_t out = block_builder_create_variable(&builder, CG_BYTE);
                block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ASSIGN, .data.assign = {.var = out, .expr = {.type = CG_UNARY, .data.unary = {.type = CG_DEREF, .var = 0}}}});
                block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ASSIGN, .data.assign = {.var = out, .expr = {.type = CG_UNARY, .data.unary = {.type = CG_LOGIC_NOT, .var = out}}}});
                block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ENDFUNC, .data.endfunc.var = out});
                goto end_token;
            }
        }
        block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ASSIGN, .data.assign = {.var = 0, .expr = {.type = CG_UNARY, .data.unary = {.type = CG_PREINC, .var = 0}}}});
    }
    block_builder_merge_block(&builder, generate_return_value_block(&builder, 1, CG_BYTE));
    end_token:
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
                function.access_level = 1;
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
