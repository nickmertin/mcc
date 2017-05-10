#include <stdio.h>
#include "../generator/code_graph.h"
#include "../generator/linux-x86/generator.h"

int main() {
    enum cg_var_size args[] = {CG_LONG, CG_LONG};
    struct cg_function func = {.arg_count = 2, .args = args, .access_level = 1, .name = "foo", .return_type = CG_LONG};
    struct cg_block_builder builder;
    block_builder_create_root(&builder, 2);
    size_t out_var = block_builder_create_variable(&builder, CG_LONG);
    block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ASSIGN, .data.assign = {.var = out_var, .expr = {.type = CG_BINARY, .data.binary = {.type = CG_ADD, .left_var = 0, .right_var = 1}}}});
    block_builder_add_statement(&builder, (struct cg_statement) {.type = CG_ENDFUNC, .data.endfunc.var = out_var});
    block_builder_end(&builder, &func.body);
    struct cg_file_graph graph = {.function_count = 1, .functions = &func};
    generate_code(&graph, NULL, stdout);
}
