#include "generator.h"
#include "../../util/linked_list.h"
#include "../../util/misc.h"

struct var_ref {
    size_t id;
    enum cg_var_size size : 8;
    bool replace;
};

struct var_ref_filter_state {
    size_t cur;
    size_t max;
    linked_list_t *past;
};

struct filter_past_state {
    size_t size;
    size_t id;
    bool cont;
    bool create;
};

struct var_map {
    struct var {
        size_t pos;
        enum cg_var_size size : 8;
    } *vars;
    size_t var_count;
};

static char var_size_map[4] = {'b', 'w', 'l', 'q'};

static void add_var_ref(linked_list_t *refs, struct cg_var var, size_t off, bool replace, size_t *max) {
    struct var_ref ref = {.id = var.id + off, .size = var.size, .replace = replace};
    if (var.id > *max)
        *max = var.id;
    linked_list_insert(refs, 0, &ref, sizeof(struct var_ref));
}

static void add_var_refs_expr(linked_list_t *refs, struct cg_expression *expr, size_t base, size_t *max) {
    switch (expr->type) {
        case CG_VAR: {
            add_var_ref(refs, expr->data.var, base, false, max);
            break;
        }
        case CG_UNARY: {
            add_var_ref(refs, expr->data.unary.var, base, false, max);
            break;
        }
        case CG_BINARY: {
            struct cg_expression_binary *data = &expr->data.binary;
            add_var_ref(refs, data->left_var, base, false, max);
            add_var_ref(refs, data->right_var, base, false, max);
            break;
        }
        case CG_TERNARY: {
            struct cg_expression_ternary *data = &expr->data.ternary;
            add_var_ref(refs, data->cond_var, base, false, max);
            add_var_ref(refs, data->true_var, base, false, max);
            add_var_ref(refs, data->false_var, base, false, max);
            break;
        }
    }
}

static bool add_var_refs(linked_list_t *refs, struct cg_block *block, size_t base, size_t *_max, bool conditional) {
    size_t max = base;
    for (size_t i = 0; i < block->statement_count; ++i) {
        switch (block->statemets[i].type) {
            case CG_IFELSE: {
                struct cg_statement_ifelse *data = &block->statemets[i].data.ifelse;
                add_var_ref(refs, data->cond_var, base, false, &max);
                conditional |= add_var_refs(refs, &data->if_true, data->if_true.new_namespace ? max + 1 : base, &max, true);
                conditional |= add_var_refs(refs, &data->if_false, data->if_false.new_namespace ? max + 1 : base, &max, true);
                break;
            }
            case CG_JUMP:
                conditional = true;
                break;
            case CG_ASSIGN: {
                struct cg_statement_assign *data = &block->statemets[i].data.assign;
                add_var_ref(refs, data->var, base, !conditional, &max);
                add_var_refs_expr(refs, &data->expr, base, &max);
                break;
            }
            case CG_CALL: {
                struct cg_statement_call *data = &block->statemets[i].data.call;
                for (size_t j = 0; j < data->arg_count; ++j)
                    add_var_ref(refs, data->args[j], 0, false, &max);
                add_var_ref(refs, data->out, 0, !conditional, &max);
                break;
            }
            case CG_ENDFUNC: {
                struct cg_statement_endfunc *data = &block->statemets[i].data.endfunc;
                add_var_ref(refs, data->var, 0, false, &max);
                break;
            }
        }
    }
    if (max > *_max)
        *_max = max;
    return conditional;
}

static void past_filter(void *data, void *state) {
    struct filter_past_state *_state = state;
    struct var_ref *_data = data;
    if (_state->cont && _data->id == _state->id) {
        _state->size = _data->size;
        _state->create = _data->replace;
    }
}

static void ref_filter(void *data, void *state) {
    struct var_ref_filter_state *_state = state;
    struct var_ref *_data = data;
    struct filter_past_state past_state = {.size = 0, .id = _data->id, .cont = true};
    linked_list_foreach(_state->past, (struct delegate_t) {.func = &past_filter, .state = &past_state});
    if (_data->replace) {
        if (!past_state.create)
            _state->cur -= 1 << past_state.size;
    } else if (past_state.create) {
        _state->cur += 1 << past_state.size;
        if (_state->cur > _state->max)
            _state->max = _state->cur;
    }
}

static size_t get_block_stack_size(struct cg_block *block) {
    linked_list_t *references = linked_list_create();
    size_t max_var;
    add_var_refs(references, block, 0, &max_var, false);/*
    struct var_ref_filter_state state = {.cur = 0, .max = 0, .past = linked_list_create()};
    linked_list_foreach(references, (struct delegate_t) {.func = &ref_filter, .state = &state});
    linked_list_destroy(state.past);
    linked_list_destroy(references);
    return state.max;*/
    return max_var * 8;
}

static size_t get_var_off(linked_list_t *map, struct cg_var var) {
    size_t off = var.id * 8;
    switch (var.size) {
        case CG_BYTE:
            off += 7;
            break;
        case CG_WORD:
            off += 6;
        case CG_LONG:
            off += 4;
            break;
    }
    return off;
}

static void out_zero(enum cg_var_size size, size_t off, FILE *out) {
    fprintf(out, "\txor%c %lu(%%esp), %lu(%%esp)\n", var_size_map[size], off, off);
}

static struct offsets {size_t _in, _out;} out_copy(struct cg_var src, struct cg_var dest, linked_list_t *map, FILE *out) {
    size_t in_off = get_var_off(map, src);
    size_t out_off = get_var_off(map, dest);
    bool zero = false;
    switch (dest.size) {
        case CG_BYTE:
            switch (src.size) {
                case CG_WORD:
                    in_off += 1;
                    break;
                case CG_LONG:
                    in_off += 3;
                    break;
                case CG_QWORD:
                    in_off += 7;
                    break;
            }
            break;
        case CG_WORD:
            switch (src.size) {
                case CG_BYTE:
                    zero = true;
                    out_off += 1;
                    break;
                case CG_LONG:
                    in_off += 2;
                    break;
                case CG_QWORD:
                    in_off += 6;
                    break;
            }
            break;
        case CG_LONG:
            switch (src.size) {
                case CG_BYTE:
                    zero = true;
                    out_off += 3;
                    break;
                case CG_WORD:
                    zero = true;
                    out_off += 2;
                    break;
                case CG_QWORD:
                    in_off += 4;
                    break;
            }
            break;
        case CG_QWORD:
            switch (src.size) {
                case CG_BYTE:
                    zero = true;
                    out_off += 7;
                    break;
                case CG_WORD:
                    zero = true;
                    out_off += 6;
                    break;
                case CG_LONG:
                    zero = true;
                    out_off += 4;
                    break;
            }
            break;
    }
    if (zero)
        out_zero(dest.size, out_off, out);
    fprintf(out, "\tmov%c %lu(%%esp) %lu(%%esp)\n", var_size_map[min(dest.size, src.size)], in_off, out_off);
    return (struct offsets) {._in = in_off, ._out = out_off};
}

static void generate_block(struct cg_block *block, linked_list_t *map, size_t *unique_id, const char *name, FILE *out) {
    for (size_t i = 0; i < block->statement_count; ++i) {
        switch (block->statemets[i].type) {
            case CG_IFELSE: {
                struct cg_statement_ifelse *data = &block->statemets[i].data.ifelse;
                fprintf(out, "\tcmp%c $0, %lu(%%esp)\n", var_size_map[data->cond_var.size], get_var_off(map, data->cond_var));
                size_t _else = (*unique_id)++;
                size_t _end = (*unique_id)++;
                fprintf(out, "\tje _%lu\n", _else);
                generate_block(&data->if_true, map, unique_id, name, out);
                fprintf(out, "\tjmp _%lu\n", _end);
                fprintf(out, "_%lu:\n", _else);
                generate_block(&data->if_false, map, unique_id, name, out);
                fprintf(out, "_%lu:\n", _end);
                break;
            }
            case CG_JUMP: {
                struct cg_statement_jump *data = &block->statemets[i].data.jump;
                fprintf(out, "\tjmp _%s$$%s\n", name, data->label);
                break;
            }
            case CG_LABEL: {
                struct cg_statement_label *data = &block->statemets[i].data.label;
                fprintf(out, "%s$$%s:\n", name, data->label);
                break;
            }
            case CG_ASSIGN: {
                struct cg_statement_assign *data = &block->statemets[i].data.assign;
                switch (data->expr.type) {
                    case CG_VAR:
                        out_copy(data->expr.data.var, data->var, map, out);
                        break;
                    case CG_VALUE:
                        fprintf(out, "\tmov%c $%lu, %lu(%%esp)\n", var_size_map[data->var.size], data->expr.data.value.value, get_var_off(map, data->var));
                        break;
                    case CG_UNARY: {
                        struct cg_expression_unary *edata = &data->expr.data.unary;
                        switch (edata->type) {
                            case CG_POSTINC:
                                fprintf(out, "\tinc%c %lu(%%esp)\n", var_size_map[edata->var.size], out_copy(edata->var, data->var, map, out)._in);
                                break;
                            case CG_POSTDEC:
                                fprintf(out, "\tdec%c %lu(%%esp)\n", var_size_map[edata->var.size], out_copy(edata->var, data->var, map, out)._in);
                                break;
                            case CG_PREINC:
                                fprintf(out, "\tinc%c %lu(%%esp)\n", var_size_map[edata->var.size], get_var_off(map, edata->var));
                                out_copy(edata->var, data->var, map, out);
                                break;
                            case CG_PREDEC:
                                fprintf(out, "\tdec%c %lu(%%esp)\n", var_size_map[edata->var.size], get_var_off(map, edata->var));
                                out_copy(edata->var, data->var, map, out);
                                break;
                            case CG_NEGATE:
                                fprintf(out, "\tneg%c %lu(%%esp)\n", var_size_map[edata->var.size], out_copy(edata->var, data->var, map, out)._out);
                                break;
                            case CG_LOGIC_NOT: {
                                fprintf(out, "\tcmp%c $0, %lu(%%esp)\n", var_size_map[edata->var.size], get_var_off(map, edata->var));
                                size_t skip = (*unique_id)++;
                                fprintf(out, "\tje _%lu\n", skip);
                                size_t off = get_var_off(map, data->var);
                                fprintf(out, "\txor%c %lu(%%esp), %lu(%%esp)\n", var_size_map[data->var.size], off, off);
                                size_t end = (*unique_id)++;
                                fprintf(out, "\tjmp _%lu\n", end);
                                fprintf(out, "_%lu\n", skip);
                                fprintf(out, "\tmov%c $1, %lu(%%esp)\n", var_size_map[data->var.size], off);
                                fprintf(out, "_%lu\n", end);
                                break;
                            }
                            case CG_BITWISE_NOT:
                                fprintf(out, "\tnot%c %lu(%%esp)\n", var_size_map[edata->var.size], out_copy(edata->var, data->var, map, out)._out);
                                break;
                            case CG_DEREF: {
                                size_t off = get_var_off(map, edata->var);
                                bool zero = false;
                                const char *reg = "eax";
                                switch (edata->var.size) {
                                    case CG_BYTE:
                                        zero = true;
                                        reg = "al";
                                        break;
                                    case CG_WORD:
                                        zero = true;
                                        reg = "ax";
                                        break;
                                    case CG_QWORD:
                                        off += 4;
                                        break;
                                }
                                if (zero)
                                    fprintf(out, "\txorl %%eax, %%eax\n");
                                fprintf(out, "\tmov%c %lu(%%esp), %%%s\n", var_size_map[min(edata->var.size, CG_LONG)], off, reg);
                                fprintf(out, "\tmov%c (%%eax), %lu(%%esp)\n", var_size_map[data->var.size], get_var_off(map, data->var));
                                break;
                            }
                            case CG_ADDR: {
                                size_t off = get_var_off(map, data->var);
                                const char *reg = "eax";
                                switch (edata->var.size) {
                                    case CG_BYTE:
                                        reg = "al";
                                        break;
                                    case CG_WORD:
                                        reg = "ax";
                                        break;
                                    case CG_QWORD:
                                        fprintf(out, "\txorl %lu(%%esp), %lu(%%esp)\n", off, off);
                                        off += 4;
                                        break;
                                }
                                fprintf(out, "\tmovl %%esp, %%eax\n");
                                fprintf(out, "\taddl $%lu, %%eax", off);
                                fprintf(out, "\tmov%c %%%s, %lu(%%esp)\n", var_size_map[min(edata->var.size, CG_LONG)], reg, off);
                                break;
                            }
                        }
                        break;
                    }
                    case CG_BINARY:break;
                    case CG_TERNARY:break;
                }
            }
        }
    }
}

void generate_code(struct cg_file_graph *graph, FILE *out) {
    size_t unique_id = 0;
    fprintf(out, ".text\n");
    for (size_t i = 0; i < graph->function_count; ++i) {
        struct cg_function *f = &graph->functions[i];
        if (f->access_level)
            fprintf(out, ".globl %s\n", f->name);
        fprintf(out, "%s:\n", f->name);
        size_t stack_size = get_block_stack_size(&f->body);
        fprintf(out, "\tpushl %%ebp\n");
        fprintf(out, "\tmovl %%esp, %%ebp\n");
        fprintf(out, "\tsubl $%lu, %%esp\n", stack_size);
        linked_list_t *map = linked_list_create();
        generate_block(&f->body, map, &unique_id, f->name, out);
        linked_list_destroy(map);
        fprintf(out, "%s$end:\n", f->name);
        fprintf(out, "\tmovl %%ebp,%%esp\n");
        fprintf(out, "\tpopl %%ebp\n");
        fprintf(out, "\tret\n");
    }
}
