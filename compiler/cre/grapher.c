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

static struct cg_block generate_char_token_block(struct cg_block_builder *parent, struct cg_block failure, char filter[]) {
    char label[18];
    struct cg_block_builder builder;
    block_builder_create_child(&builder, parent);
    sprintf(label, "_%lx", unique_id++);
    size_t character = block_builder_create_variable(&builder, CG_BYTE), comparison = block_builder_create_variable(&builder, CG_BYTE);
    block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ASSIGN, .data.assign = {.var = character, .expr = {.type = CG_UNARY, .data.unary = {.type = CG_DEREF, .var = 0}}}});
    block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_JUMPIF, .data.jumpif = {.cond_var = character, .label = strdup(label)}});
    block_builder_merge_block(&builder, generate_return_value_block(&builder, 0, CG_BYTE));
    block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_LABEL, .data.label.label = strdup(label)});
    sprintf(label, "_%lx", unique_id++);
    for (size_t i = 0; i < 256; ++i) {
        if (getFlag(filter, i)) {
            block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ASSIGN, .data.assign = {.var = comparison, .expr = {.type = CG_VALUE, .data.value.value = i}}});
            block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ASSIGN, .data.assign = {.var = comparison, .expr = {.type = CG_BINARY, .data.binary = {.type = CG_EQ, .left_var = comparison, .right_var = character}}}});
            block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_JUMPIF, .data.jumpif = {.cond_var = comparison, .label = strdup(label)}});
        }
    }
    block_builder_destroy_variable(&builder, character);
    block_builder_destroy_variable(&builder, comparison);
    block_builder_merge_block(&builder, failure);
    block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_LABEL, .data.label.label = strdup(label)});
    struct cg_block block;
    block_builder_end(&builder, &block);
    return block;
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

static struct cg_block *generate_match_function(struct cre_token *tokens, size_t token_count, struct delegate_t add_func) {
    struct cg_block *result = NULL;
    struct cg_block_builder builder;
    block_builder_create_root(&builder, 1);
    for (size_t i = 0; i < token_count; ++i) {
        char skipLabel[18];
        bool skip = false;
        bool inc = i != token_count - 1;
        switch (tokens[i].type) {
            case CRE_CHAR: {
                switch (tokens[i].mode) {
                    case CRE_ONE:
                        block_builder_merge_block(&builder, generate_char_token_block(&builder, generate_return_value_block(&builder, 0, CG_BYTE), tokens[i].filter));
                        break;
                    case CRE_MAYBE: {
                        skip = true;
                        sprintf(skipLabel, "_%lx", unique_id++);
                        struct cg_block failure = {.statement_count = 1, .statements = malloc(sizeof(struct cg_statement))};
                        *failure.statements = (struct cg_statement) {.type = CG_JUMP, .data.jump.label = strdup(skipLabel)};
                        block_builder_merge_block(&builder, generate_char_token_block(&builder, failure, tokens[i].filter));
                        break;
                    }
                    case CRE_MULTIPLE:
                        block_builder_merge_block(&builder, generate_char_token_block(&builder, generate_return_value_block(&builder, 0, CG_BYTE), tokens[i].filter));
                        block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ASSIGN, .data.assign = {.var = 0, .expr = {.type = CG_UNARY, .data.unary = {.type = CG_PREINC, .var = 0}}}});
                        // Intentionally without break, as a multiple token is equivalent to a many token prefixed by a one token
                    case CRE_MANY:
                        if (inc) {
                            inc = false;
                            struct cg_function function;
                            char name[23];
                            sprintf(name, "__cre_%lx", unique_id++);
                            map_to_match_function(&function, generate_match_function(&tokens[i + 1], token_count - i - 1, add_func), strdup(name), 0);
                            delegate_invoke(add_func, &function);
                            char loop[18];
                            sprintf(loop, "_%lx", unique_id++);
                            if (tokens[i].lazy) {
                                size_t next_stage_result = block_builder_create_variable(&builder, CG_BYTE);
                                block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_LABEL, .data.label.label = strdup(loop)});
                                size_t *args = malloc(sizeof(size_t));
                                args[0] = 0;
                                block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_CALL, .data.call = {.name = strdup(name), .arg_count = 1, .args = args, .out = next_stage_result, .out_size = CG_BYTE}});
                                struct cg_block_builder cont_builder;
                                block_builder_create_child(&cont_builder, &builder);
                                block_builder_merge_block(&cont_builder, generate_char_token_block(&builder, generate_return_value_block(&builder, 0, CG_BYTE), tokens[i].filter));
                                block_builder_add_statement(&cont_builder, (struct cg_statement) {.type = CG_ASSIGN, .data.assign = {.var = 0, .expr = {.type = CG_UNARY, .data.unary = {.type = CG_PREINC, .var = 0}}}});
                                block_builder_add_statement(&cont_builder, (struct cg_statement) {.type = CG_JUMP, .data.jump.label = strdup(loop)});
                                block_builder_merge_block(&builder, generate_return_value_block(&builder, 0, CG_BYTE));
                                struct cg_block cont_block;
                                block_builder_end(&cont_builder, &cont_block);
                                block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_IFELSE, .data.ifelse = {.cond_var = next_stage_result, .if_true = generate_return_value_block(&builder, 1, CG_BYTE), .if_false = cont_block}});
                                block_builder_destroy_variable(&builder, next_stage_result);
                                goto end_token;
                            } else {
                                size_t starting_ptr = block_builder_create_variable(&builder, CG_LONG);
                                block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ASSIGN, .data.assign = {.var = starting_ptr, .expr = {.type = CG_VAR, .data.var = 0}}});
                                block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_LABEL, .data.label.label = strdup(loop)});
                                char loop2[18];
                                sprintf(loop2, "_%lx", unique_id++);
                                struct cg_block_builder loop2_builder;
                                block_builder_create_child(&loop2_builder, &builder);
                                block_builder_add_statement(&loop2_builder, (struct cg_statement) {.type = CG_JUMP, .data.jump.label = strdup(loop2)});
                                struct cg_block loop2_block;
                                block_builder_end(&loop2_builder, &loop2_block);
                                block_builder_merge_block(&builder, generate_char_token_block(&builder, loop2_block, tokens[i].filter));
                                block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ASSIGN, .data.assign = {.var = 0, .expr = {.type = CG_UNARY, .data.unary = {.type = CG_PREINC, .var = 0}}}});
                                block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_JUMP, .data.jump.label = strdup(loop)});
                                size_t next_stage_result = block_builder_create_variable(&builder, CG_BYTE);
                                block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_LABEL, .data.label.label = strdup(loop2)});
                                size_t *args = malloc(sizeof(size_t));
                                args[0] = 0;
                                block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_CALL, .data.call = {.name = strdup(name), .arg_count = 1, .args = args, .out = next_stage_result, .out_size = CG_BYTE}});
                                struct cg_block_builder cont_builder;
                                block_builder_create_child(&cont_builder, &builder);
                                block_builder_add_statement(&cont_builder, (struct cg_statement) {.type = CG_ASSIGN, .data.assign = {.var = 0, .expr = {.type = CG_UNARY, .data.unary = {.type = CG_PREDEC, .var = 0}}}});
                                size_t comparison = block_builder_create_variable(&cont_builder, CG_BYTE);
                                block_builder_add_statement(&cont_builder, (struct cg_statement) {.type = CG_ASSIGN, .data.assign = {.var = comparison, .expr = {.type = CG_BINARY, .data.binary = {.type = CG_EQ, .left_var = 0, .right_var = starting_ptr}}}});
                                block_builder_add_statement(&cont_builder, (struct cg_statement) {.type = CG_IFELSE, .data.ifelse = {.cond_var = comparison, .if_true = generate_return_value_block(&cont_builder, 0, CG_BYTE), .if_false = loop2_block}});
                                struct cg_block cont_block;
                                block_builder_end(&cont_builder, &cont_block);
                                block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_IFELSE, .data.ifelse = {.cond_var = next_stage_result, .if_true = generate_return_value_block(&builder, 1, CG_BYTE), .if_false = cont_block}});
                                block_builder_destroy_variable(&builder, next_stage_result);
                                goto end_token;
                            }
                        } else if (!tokens[i].lazy) {
                            //TODO: eventually make the return value the length of the matching string; nothing to do here otherwise
                        }
                        break;
                }
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
        if (inc)
            block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ASSIGN, .data.assign = {.var = 0, .expr = {.type = CG_UNARY, .data.unary = {.type = CG_PREINC, .var = 0}}}});
        if (skip)
            block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_LABEL, .data.label.label = strdup(skipLabel)});
    }
    block_builder_merge_block(&builder, generate_return_value_block(&builder, 1, CG_BYTE));
    end_token:
    result = malloc(sizeof(struct cg_block));
    block_builder_end(&builder, result);
    return result;
}

static void add_function_internal(struct cg_function *function, linked_list_t *functions) {
    linked_list_insert(functions, 0, function, sizeof(struct cg_function));
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
                    match = generate_match_function(expr->tokens, expr->token_count, (struct delegate_t) {.func = (void (*)(void *, void *)) &add_function_internal, .state = functions});
                map_to_match_function(&function, match, attr->data, 1);
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
