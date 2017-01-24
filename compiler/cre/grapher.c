#include "grapher.h"
#include <malloc.h>
#include "../../util/linked_list.h"
#include "../../util/misc.h"

static size_t unique_id = 0;

static struct size_t flag_count(void *data, size_t length) {

}

static struct cg_block generate_jump_block(char *label) {
    struct cg_statement *statement = malloc(sizeof(struct cg_statement));
    *statement = {.type = CG_JUMP, .data.jump.label = label};
    return (struct cg_block) {.new_namespace = false, .statemets = statement};
}

static struct cg_block generate_return_block(bool result) {
    struct cg_statement *statements = malloc(sizeof(struct cg_statement) * 2);
    statements[0] = {.type = CG_ASSIGN, .data.assign = {.var = 0, .expr = {.type = CG_VALUE, .data.value.value =  result ? 1 : 0}}};
    statements[1] = {.type = CG_ENDFUNC, .data.endfunc = {.var = 0}};
    return (struct cg_block) {.new_namespace = false, .statemets = statements, .statement_count = 2};
}

static struct cg_block *generate_match_function(struct cre_token *tokens, size_t token_count) {
    struct cg_block *result = NULL;
    struct cg_statement stmt;
    linked_list_t *statements = linked_list_create();
    for (size_t i = 0; i < token_count; ++i) {
        switch (tokens[i].type) {
            case CRE_CHAR:
                for (size_t j = 0; j < 256; ++j) {
                    char label[18];
                    sprintf(label, "_%lx", unique_id++);
                    if (getFlag(tokens[i].filter, j)) {
                        stmt = {.type = CG_ASSIGN, .data.assign = {.var = 2, .expr = {.type = CG_BINARY, .data.binary = {.type = CG_ADD, .left_var = 0, .right_var = 1}}}};
                        linked_list_insert(statements, 0, &stmt, sizeof(struct cg_statement));
                        stmt.data.assign = {.var = 3, .expr = {.type = CG_UNARY, .data.unary = {.var = 2, .type = CG_DEREF}}};
                        linked_list_insert(statements, 0, &stmt, sizeof(struct cg_statement));
                        stmt.data.assign = {.var = 4, .expr = {.type = CG_VALUE, .data.value.value = 0}};
                        linked_list_insert(statements, 0, &stmt, sizeof(struct cg_statement));
                        stmt.data.assign = {.var = 3, .expr = {.type = CG_BINARY, .data.binary = {.type = CG_EQ, .left_var = 3, .right_var = 4}}};
                        linked_list_insert(statements, 0, &stmt, sizeof(struct cg_statement));
                        stmt = {.type = CG_IFELSE, .data.ifelse = {.cond_var = 3, .if_true = generate_jump_block(strdup(label)), .if_false = generate_return_block(false)}};
                        linked_list_insert(statements, 0, &stmt, sizeof(struct cg_statement));
                    }
                    stmt = {.type = CG_LABEL, .data.label.label = strdup(label)};
                }
                break;
        }
    }
    result = malloc(sizeof(struct cg_block));
    *result = {.new_namespace = false, .statement_count = linked_list_size(statements)};
    result->statemets = malloc(sizeof(struct cg_statement) * result->statement_count);
    void *state[2];
    state[0] = &state[1];
    state[1] = result->statemets;
    linked_list_foreach(statements, (struct delegate_t) {.func = &copy_to_array, .state = state});
}

static void map_to_match_function(struct cg_function *out, struct cg_block *block, const char *name, uint8_t access) {
    out->arg_count = 1;
    out->access_level = access;
    out->body = *block;
    out->name = name;
}

struct cg_file_graph *graph(struct cre_parsed_file *file) {
    struct cg_file_graph *result = NULL;
    linked_list_t *functions = linked_list_create();
    for (size_t i = 0; i < file->expression_count; ++i) {
        struct cre_expression *expr = &file->expressions[i];
        struct cg_block *match = NULL, *find = NULL, *replace = NULL;
        for (size_t j = 0; j < expr->attribute_count; ++j) {
            struct cre_attribute *attr = &expr->attributes[j];
            if (strcmp(attr->label, "match")) {
                struct cg_function function;
                if (!match)
                    match = generate_match_function(expr->tokens, expr->token_count);
                map_to_match_function(&function, match, attr->data, 0);
                linked_list_insert(functions, 0, &function, sizeof(struct cg_function));
            }
        }
    }
}
