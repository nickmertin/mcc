#include "code_graph.h"
#include <malloc.h>
#include "../util/misc.h"

void block_builder_create_root(struct cg_block_builder *builder, size_t parameter_count) {
    builder->start_index = 0;
    builder->next_var = malloc(sizeof(size_t));
    *builder->next_var = parameter_count;
    builder->root = true;
    builder->statements = linked_list_create();
    builder->variables = linked_list_create();
}

void block_builder_create_child(struct cg_block_builder *builder, struct cg_block_builder *parent) {
    builder->start_index = linked_list_size(parent->statements);
    builder->next_var = parent->next_var;
    builder->root = false;
    builder->statements = linked_list_create();
    builder->variables = parent->variables;
}

void block_builder_add_statement(struct cg_block_builder *builder, struct cg_statement statement) {
    linked_list_insert(builder->statements, 0, &statement, sizeof(struct cg_statement));
}

void block_builder_merge_block(struct cg_block_builder *builder, struct cg_block block) {
    for (size_t i = 0; i < block.statement_count; ++i)
        block_builder_add_statement(builder, block.statements[i]);
}

size_t block_builder_create_variable(struct cg_block_builder *builder, enum cg_var_size size) {
    struct cg_var var = { .index = linked_list_size(builder->statements), .size = size, .create = true, .id = (*builder->next_var)++ };
    linked_list_insert(builder->variables, 0, &var, sizeof(struct cg_var));
    return var.id;
}

void block_builder_destroy_variable(struct cg_block_builder *builder, size_t variable) {
    struct cg_var var = { .index = linked_list_size(builder->statements), .create = false, .id = variable };
    linked_list_insert(builder->variables, 0, &var, sizeof(struct cg_var));
}

void block_builder_end(struct cg_block_builder *builder, struct cg_block *out) {
    out->statement_count = linked_list_size(builder->statements);
    out->statements = malloc(sizeof(struct cg_statement) * out->statement_count);
    copy_to_array(builder->statements, out->statements, sizeof(struct cg_statement));
    linked_list_destroy(builder->statements);
    if (builder->root) {
        out->variable_count = linked_list_size(builder->variables);
        out->variables = malloc(sizeof(struct cg_var) * out->variable_count);
        copy_to_array(builder->variables, out->variables, sizeof(struct cg_var));
        linked_list_destroy(builder->variables);
    } else {
        struct linked_list_node_t *node = *builder->variables;
        linked_list_t *processed = linked_list_create();
        for (size_t i = linked_list_size(builder->variables); i > builder->start_index; --i) {
            struct cg_var *var = (struct cg_var *) node->data;
            if (var->create) {
                if (!contains_size_t(processed, var->id))
                    block_builder_destroy_variable(builder, var->id);
            } else
                linked_list_insert(processed, 0, &var->id, sizeof(size_t));
            node = node->ptr;
        }
        out->variable_count = 0;
    }
}
