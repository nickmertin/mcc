#include "code_graph.h"
#include <malloc.h>
#include "../util/misc.h"

struct block_builder_end_internal_state {
    struct cg_block_builder *builder;
    linked_list_t *processed;
};

void block_builder_end_internal(struct cg_statement *data, struct block_builder_end_internal_state *state) {
    if (data->type == CG_CREATEVAR) {
        if (!contains_size_t(state->processed, data->data.createvar.var))
            block_builder_destroy_variable(state->builder, data->data.createvar.var);
    } else if (data->type == CG_DESTROYVAR)
        linked_list_insert(state->processed, 0, &data->data.destroyvar.var, sizeof(size_t));
}

void block_builder_create_root(struct cg_block_builder *builder, size_t parameter_count) {
    builder->start_index = 0;
    builder->next_var = malloc(sizeof(size_t));
    *builder->next_var = parameter_count;
    builder->root = true;
    builder->statements = linked_list_create();
}

void block_builder_create_child(struct cg_block_builder *builder, struct cg_block_builder *parent) {
    builder->start_index = linked_list_size(parent->statements);
    builder->next_var = parent->next_var;
    builder->root = false;
    builder->statements = linked_list_create();
}

void block_builder_add_statement(struct cg_block_builder *builder, struct cg_statement statement) {
    linked_list_insert(builder->statements, 0, &statement, sizeof(struct cg_statement));
}

void block_builder_merge_block(struct cg_block_builder *builder, struct cg_block block) {
    for (size_t i = 0; i < block.statement_count; ++i)
        block_builder_add_statement(builder, block.statements[i]);
}

size_t block_builder_create_variable(struct cg_block_builder *builder, enum cg_var_size size) {
    size_t var = (*builder->next_var)++;
    block_builder_add_statement(builder, (struct cg_statement) {.type = CG_CREATEVAR, .data.createvar = {.size = size, .var = var}});
    return var;
}

void block_builder_destroy_variable(struct cg_block_builder *builder, size_t variable) {
    block_builder_add_statement(builder, (struct cg_statement) {.type = CG_DESTROYVAR, .data.destroyvar.var = variable});
}

void block_builder_end(struct cg_block_builder *builder, struct cg_block *out) {
    if (!builder->root) {
        struct block_builder_end_internal_state state = {.builder = builder, .processed = linked_list_create()};
        linked_list_foreach(builder->statements, (struct delegate_t) {.func = (void (*)(void *, void *)) &block_builder_end_internal, .state = &state});
        linked_list_destroy(state.processed);
    }
    linked_list_reverse(builder->statements);
    out->statement_count = linked_list_size(builder->statements);
    out->statements = malloc(sizeof(struct cg_statement) * out->statement_count);
    copy_to_array(builder->statements, out->statements, sizeof(struct cg_statement));
    linked_list_destroy(builder->statements);
    out->max_var = 0;
    for (size_t i = 0; i < out->statement_count; ++i)
        if (out->statements[i].type == CG_CREATEVAR && out->statements[i].data.createvar.var > out->max_var)
            out->max_var = out->statements[i].data.createvar.var;
}
