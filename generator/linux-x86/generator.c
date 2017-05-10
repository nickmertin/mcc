#include <malloc.h>
#include <strings.h>
#include <string.h>
#include "generator.h"
#include "../../util/misc.h"

#define SIZE_CONVERT(size) ((size) ? (size_t) (1 << ((size) - 1)) : 0)

bool enable_comments = false;

struct var_ref {
    size_t offset;
    enum cg_var_size size : 8;
    bool exists;
};

struct defragment_state {
    size_t offset;
    struct var_ref *map;
    FILE *out;
};

static const char var_size_map[] = {'\0', 'b', 'w', 'l', 'l'};

static const char *primary_registers[] = {"", "al", "ax", "eax", "edx"};

static const char *secondary_registers[] = {"", "bl", "bx", "ebx", "ecx"};

static const char *binary_operators[] = {"+", "-", "*", "/", "%", "&&", "||", "^^", "&", "|", "^", "<", ">", "<=", ">=", "==", "!="};

static const char *var_size_names[] = {"void", "byte", "word", "long", "qword"};

static size_t get_block_stack_size(struct cg_block *block, size_t initial_count, enum cg_var_size *initial_sizes, size_t *var_count_out) {
    if (!block->statement_count)
        return 0;
    size_t *counts = malloc(sizeof(size_t) * block->statement_count);
    bzero(counts, sizeof(size_t) * block->statement_count);
    enum cg_var_size *map = malloc(sizeof(enum cg_var_size) * block->statement_count);
    bzero(map, sizeof(enum cg_var_size) * block->statement_count);
    memcpy(map, initial_sizes, sizeof(enum cg_var_size) * initial_count);
    size_t max_var = initial_count;
    for (size_t i = 0; i < block->statement_count; ++i) {
        struct cg_statement *stmt = &block->statements[i];
        switch (stmt->type) {
            case CG_CREATEVAR:
                map[stmt->data.createvar.var] = stmt->data.createvar.size;
                max_var = max(max_var, stmt->data.createvar.var);
                break;
            case CG_DESTROYVAR:
                map[stmt->data.createvar.var] = CG_VOID;
                break;
            case CG_IFELSE: {
                size_t max_var_1, max_var_2;
                size_t size_1 = get_block_stack_size(&stmt->data.ifelse.if_true, 0, NULL, &max_var_1);
                size_t size_2 = get_block_stack_size(&stmt->data.ifelse.if_false, 0, NULL, &max_var_2);
                counts[i] += max(size_1, size_2);
                max_var = max(max_var, max(max_var_1, max_var_2));
                break;
            }
        }
        for (size_t j = 0; j < block->statement_count; ++j)
            counts[i] += SIZE_CONVERT(map[j]);
    }
    free(map);
    size_t size = 0;
    for (size_t i = 0; i < block->statement_count; ++i)
        if (counts[i] > size)
            size = counts[i];
    free(counts);
    if (var_count_out)
        *var_count_out = max_var + 1;
    return size;
}

static void out_zero(enum cg_var_size size, size_t off, FILE *out) {
    fprintf(out, "\txor%c %lu(%%esp), %lu(%%esp)\n", var_size_map[size], off, off);
}

static void write_move(size_t dest, size_t src, enum cg_var_size size, FILE *out) {
    if (size == CG_QWORD) {
        fprintf(out, "\tmovl %lu(%%esp), %%edx\n", src);
        fprintf(out, "\tmovl %lu(%%esp), %%eax\n", src + 4);
        fprintf(out, "\tmovl %%edx, %lu(%%esp)\n", dest);
        fprintf(out, "\tmovl %%eax, %lu(%%esp)\n", dest + 4);
    } else {
        fprintf(out, "\tmov%c %lu(%%esp), %%%s\n", var_size_map[size], src, primary_registers[size]);
        fprintf(out, "\tmov%c %%%s, %lu(%%esp)\n", var_size_map[size], primary_registers[size], dest);
    }
}

static void defragment(size_t *data, struct defragment_state *state) {
    struct var_ref *var = &state->map[*data];
    if (var->offset != state->offset) {
        write_move(state->offset, var->offset, var->size, state->out);
        var->offset = state->offset;
    }
    state->offset += SIZE_CONVERT(var->size);
}

static void generate_block(struct cg_function *function, struct cg_block *block, struct var_ref *map, linked_list_t *reverse, size_t max_stack_size, size_t *unique_id, const char *name, FILE *out) {
    for (size_t i = 0; i < block->statement_count; ++i) {
        struct cg_statement *stmt = &block->statements[i];
        switch (stmt->type) {
            case CG_IFELSE: {
                struct cg_statement_ifelse *data = &stmt->data.ifelse;
                if (enable_comments)
                    fprintf(out, "\t# if $%lu then\n", data->cond_var);
                struct var_ref var = map[data->cond_var];
                if (var.size == CG_QWORD) {
                    fprintf(out, "\tmovl %lu(%%esp), %%eax\n", var.offset + 4);
                    fprintf(out, "\torl %lu(%%esp), %%eax\n", var.offset);
                    fprintf(out, "\tcmpl $0, %%eax\n");
                } else
                    fprintf(out, "\tcmp%c $0, %lu(%%esp)\n", var_size_map[var.size], var.offset);
                size_t _else = (*unique_id)++;
                size_t _end = (*unique_id)++;
                fprintf(out, "\tje _%s$%lu\n", name, _else);
                generate_block(function, &data->if_true, map, reverse, max_stack_size, unique_id, name, out);
                fprintf(out, "\tjmp _%s$%lu\n", name, _end);
                if (enable_comments)
                    fprintf(out, "\t# else\n");
                fprintf(out, "_%s$%lu:\n", name, _else);
                generate_block(function, &data->if_false, map, reverse, max_stack_size, unique_id, name, out);
                fprintf(out, "_%s$%lu:\n", name, _end);
                if (enable_comments)
                    fprintf(out, "\t# end if\n");
                break;
            }
            case CG_JUMPIF: {
                struct cg_statement_jumpif *data = &stmt->data.jumpif;
                if (enable_comments)
                    fprintf(out, "\t# if $%lu goto %s\n", data->cond_var, data->label);
                struct var_ref var = map[data->cond_var];
                if (var.size == CG_QWORD) {
                    fprintf(out, "\tmovl %lu(%%esp), %%eax\n", var.offset + 4);
                    fprintf(out, "\torl %lu(%%esp), %%eax\n", var.offset);
                    fprintf(out, "\tcmpl $0, %%eax\n");
                } else
                    fprintf(out, "\tcmp%c $0, %lu(%%esp)\n", var_size_map[var.size], var.offset);
                fprintf(out, "\tjne _%s$$%s\n", name, data->label);
                break;
            }
            case CG_JUMP: {
                struct cg_statement_jump *data = &stmt->data.jump;
                if (enable_comments)
                    fprintf(out, "\t# goto %s\n", data->label);
                fprintf(out, "\tjmp _%s$$%s\n", name, data->label);
                break;
            }
            case CG_LABEL: {
                struct cg_statement_label *data = &stmt->data.label;
                if (enable_comments)
                    fprintf(out, "\t# %s:\n", data->label);
                fprintf(out, "_%s$$%s:\n", name, data->label);
                break;
            }
            case CG_ASSIGN: {
                struct cg_statement_assign *data = &stmt->data.assign;
                switch (data->expr.type) {
                    case CG_VAR: {
                        if (enable_comments)
                            fprintf(out, "\t# $%lu = $%lu\n", data->var, data->expr.data.var);
                        struct var_ref src = map[data->expr.data.var];
                        struct var_ref dest = map[data->var];
                        size_t src_off = src.offset;
                        size_t dest_off = dest.offset;
                        if (src.size > dest.size)
                            src_off += SIZE_CONVERT(src.size) - SIZE_CONVERT(dest.size);
                        else if (src.size < dest.size)
                            dest_off += SIZE_CONVERT(dest.size) - SIZE_CONVERT(src.size);
                        write_move(dest_off, src_off, min(src.size, dest.size), out);
                        break;
                    }
                    case CG_VALUE:
                        if (enable_comments)
                            fprintf(out, "\t# $%lu = %lu\n", data->var, data->expr.data.value);
                        fprintf(out, "\tmov%c $%lu, %lu(%%esp)\n", var_size_map[map[data->var].size], data->expr.data.value.value, map[data->var]);
                        break;
                    case CG_UNARY: {
                        struct cg_expression_unary *edata = &data->expr.data.unary;
                        bool copy = true;
                        bool qword = false;
                        struct var_ref var = map[edata->var];
                        struct var_ref dest = map[data->var];
                        switch (edata->type) {
                            case CG_POSTINC:
                                if (enable_comments)
                                    fprintf(out, "\t# $%lu = $%lu++\n", data->var, edata->var);
                                if (var.size == CG_QWORD) {
                                    fprintf(out, "\tmovl %lu(%%esp), %%eax\n", var.offset + 4);
                                    fprintf(out, "\tmovl %lu(%%esp), %%edx\n", var.offset);
                                    fprintf(out, "\taddl $1, %lu(%%esp)\n", var.offset + 4);
                                    fprintf(out, "\tadcl $0, %lu(%%esp)\n", var.offset);
                                    qword = true;
                                } else {
                                    if (var.size != CG_LONG)
                                        fprintf(out, "\txorl %%eax, %%eax\n");
                                    fprintf(out, "\tmov%c %lu(%%esp), %%%s\n", var_size_map[var.size], var.offset, primary_registers[var.size]);
                                    fprintf(out, "\tinc%c %lu(%%esp)\n", var_size_map[map[edata->var].size], map[edata->var].offset);
                                }
                                break;
                            case CG_POSTDEC:
                                if (enable_comments)
                                    fprintf(out, "\t# $%lu = $%lu--\n", data->var, edata->var);
                                if (var.size == CG_QWORD) {
                                    fprintf(out, "\tmovl %lu(%%esp), %%eax\n", var.offset + 4);
                                    fprintf(out, "\tmovl %lu(%%esp), %%edx\n", var.offset);
                                    fprintf(out, "\tsubl $1, %lu(%%esp)\n", var.offset + 4);
                                    fprintf(out, "\tsbbl $0, %lu(%%esp)\n", var.offset);
                                    qword = true;
                                } else {
                                    if (var.size != CG_LONG)
                                        fprintf(out, "\txorl %%eax, %%eax\n");
                                    fprintf(out, "\tmov%c %lu(%%esp), %%%s\n", var_size_map[var.size], var.offset, primary_registers[var.size]);
                                    fprintf(out, "\tdec%c %lu(%%esp)\n", var_size_map[map[edata->var].size], map[edata->var].offset);
                                }
                                break;
                            case CG_PREINC:
                                if (enable_comments)
                                    fprintf(out, "\t# $%lu = ++$%lu\n", data->var, edata->var);
                                if (var.size == CG_QWORD) {
                                    fprintf(out, "\taddl $1, %lu(%%esp)\n", var.offset + 4);
                                    fprintf(out, "\tadcl $0, %lu(%%esp)\n", var.offset);
                                    fprintf(out, "\tmovl %lu(%%esp), %%eax\n", var.offset + 4);
                                    fprintf(out, "\tmovl %lu(%%esp), %%edx\n", var.offset);
                                    qword = true;
                                } else {
                                    fprintf(out, "\tinc%c %lu(%%esp)\n", var_size_map[map[edata->var].size], map[edata->var].offset);
                                    if (var.size != CG_LONG)
                                        fprintf(out, "\txorl %%eax, %%eax\n");
                                    fprintf(out, "\tmov%c %lu(%%esp), %%%s\n", var_size_map[var.size], var.offset, primary_registers[var.size]);
                                }
                                break;
                            case CG_PREDEC:
                                if (enable_comments)
                                    fprintf(out, "\t# $%lu = --$%lu\n", data->var, edata->var);
                                if (var.size == CG_QWORD) {
                                    fprintf(out, "\tsubl $1, %lu(%%esp)\n", var.offset + 4);
                                    fprintf(out, "\tsbbl $0, %lu(%%esp)\n", var.offset);
                                    fprintf(out, "\tmovl %lu(%%esp), %%eax\n", var.offset + 4);
                                    fprintf(out, "\tmovl %lu(%%esp), %%edx\n", var.offset);
                                } else {
                                    fprintf(out, "\tdec%c %lu(%%esp)\n", var_size_map[map[edata->var].size], map[edata->var].offset);
                                    if (var.size != CG_LONG)
                                        fprintf(out, "\txorl %%eax, %%eax\n");
                                    fprintf(out, "\tmov%c %lu(%%esp), %%%s\n", var_size_map[var.size], var.offset, primary_registers[var.size]);
                                }
                                break;
                            case CG_NEGATE:
                                if (enable_comments)
                                    fprintf(out, "\t# $%lu = -$%lu\n", data->var, edata->var);
                                if (var.size == CG_QWORD) {
                                    fprintf(out, "\tmovl %lu(%%esp), %%eax\n", var.offset + 4);
                                    fprintf(out, "\tmovl %lu(%%esp), %%edx\n", var.offset);
                                    qword = true;
                                } else {
                                    if (var.size != CG_LONG)
                                        fprintf(out, "\txorl %%eax, %%eax\n");
                                    fprintf(out, "\tmov%c %lu(%%esp), %%%s\n", var_size_map[var.size], var.offset, primary_registers[var.size]);
                                }
                                fprintf(out, "\tneg%c %%%s\n", var_size_map[var.size], primary_registers[var.size]);
                                break;
                            case CG_LOGIC_NOT: {
                                if (enable_comments)
                                    fprintf(out, "\t# $%lu = !$%lu\n", data->var, edata->var);
                                if (var.size == CG_QWORD) {
                                    fprintf(out, "\tmovl %lu(%%esp), %%eax\n", var.offset + 4);
                                    fprintf(out, "\torl %lu(%%esp), %%edx\n", var.offset);
                                    fprintf(out, "\tcmpl $0, %%eax\n");
                                } else
                                    fprintf(out, "\tcmp%c $0, %lu(%%esp)\n", var_size_map[var.size], var.offset);
                                size_t skip = (*unique_id)++;
                                fprintf(out, "\tje _%s$%lu\n", name, skip);
                                fprintf(out, "\tmov%c $0, %lu(%%esp)\n", var_size_map[var.size], dest.offset);
                                if (var.size == CG_QWORD)
                                    fprintf(out, "\tmovl $0, %lu(%%esp)\n", dest.offset + 4);
                                size_t end = (*unique_id)++;
                                fprintf(out, "\tjmp _%s$%lu\n", name, end);
                                fprintf(out, "_%s$%lu\n", name, skip);
                                if (var.size == CG_QWORD) {
                                    fprintf(out, "\tmovl $0, %lu(%%esp)\n", dest.offset + 4);
                                    fprintf(out, "\tmovl $0, %lu(%%esp)\n", dest.offset);
                                } else
                                    fprintf(out, "\tmov%c $1, %lu(%%esp)\n", var_size_map[var.size], var.offset);
                                fprintf(out, "_%s$%lu\n", name, end);
                                copy = false;
                                break;
                            }
                            case CG_BITWISE_NOT:
                                if (enable_comments)
                                    fprintf(out, "\t# $%lu = ~$%lu\n", data->var, edata->var);
                                if (var.size == CG_QWORD) {
                                    fprintf(out, "\tmovl %lu(%%esp), %%eax\n", var.offset + 4);
                                    fprintf(out, "\tmovl %lu(%%esp), %%edx\n", var.offset);
                                    qword = true;
                                } else {
                                    if (var.size != CG_LONG)
                                        fprintf(out, "\txorl %%eax, %%eax\n");
                                    fprintf(out, "\tmov%c %lu(%%esp), %%%s\n", var_size_map[var.size], var.offset, primary_registers[var.size]);
                                }
                                fprintf(out, "\tnot%c %%%s\n", var_size_map[var.size], primary_registers[var.size]);
                                break;
                            case CG_DEREF: {
                                if (enable_comments)
                                    fprintf(out, "\t# $%lu = *$%lu\n", data->var, edata->var);
                                switch (var.size) {
                                    case CG_QWORD:
                                        fprintf(out, "\tmovl %lu(%%esp), %%eax\n", var.offset + 4);
                                        qword = true;
                                        break;
                                    case CG_LONG:
                                        fprintf(out, "\tmovl %lu(%%esp), %%eax\n", var.offset);
                                        break;
                                    default:
                                        fprintf(out, "\txorl %%eax, %%eax\n");
                                        fprintf(out, "\tmov%c %lu(%%esp), %%%s\n", var_size_map[var.size], var.offset, primary_registers[var.size]);
                                }
                                fprintf(out, "\tmov%c 0(%%eax), %%%s\n", var_size_map[dest.size], primary_registers[dest.size]);
                                if (dest.size == CG_QWORD) {
                                    fprintf(out, "\tmovl 4(%%eax), %%eax\n");
                                    qword = true;
                                }
                                break;
                            }
                            case CG_ADDR: {
                                if (enable_comments)
                                    fprintf(out, "\t# $%lu = &$%lu\n", data->var, edata->var);
                                fprintf(out, "\tleal %lu(%%esp), %%eax\n", dest.offset);
                                break;
                            }
                        }
                        if (copy) {
                            if (dest.size == CG_QWORD) {
                                fprintf(out, "\tmovl %%eax, %lu(%%esp)\n", dest.offset + 4);
                                fprintf(out, "\tmovl %s, %lu(%%esp)\n", qword ? "%eax" : "$0", dest.offset);
                            } else
                                fprintf(out, "\tmov%c %%%s, %lu(%%esp)\n", var_size_map[dest.size], primary_registers[dest.size], dest.offset);
                        }
                        break;
                    }
                    case CG_BINARY: {
                        struct cg_expression_binary *edata = &data->expr.data.binary;
                        if (enable_comments)
                            fprintf(out, "\t# $%lu = $%lu %s $%lu\n", data->var, edata->left_var, binary_operators[edata->type], edata->right_var);
                        struct var_ref dest = map[data->var];
                        struct var_ref left = map[edata->left_var];
                        struct var_ref right = map[edata->right_var];
                        if (left.size == CG_QWORD) {
                            fprintf(out, "\tmovl %lu(%%esp), %%eax\n", left.offset + 4);
                            fprintf(out, "\tmovl %lu(%%esp), %%edx\n", left.offset);
                        } else {
                            if (left.size != CG_LONG)
                                fprintf(out, "\txorl %%eax, %%eax\n");
                            fprintf(out, "\tmov%c %lu(%%esp), %%%s\n", var_size_map[left.size], left.offset, primary_registers[left.size]);
                        }
                        if (right.size == CG_QWORD) {
                            fprintf(out, "\tmovl %lu(%%esp), %%ebx\n", right.offset + 4);
                            fprintf(out, "\tmovl %lu(%%esp), %%ecx\n", right.offset);
                        } else {
                            if (right.size != CG_LONG)
                                fprintf(out, "\txorl %%ebx, %%ebx\n");
                            fprintf(out, "\tmov%c %lu(%%esp), %%%s\n", var_size_map[right.size], right.offset, secondary_registers[right.size]);
                        }
                        switch (edata->type) {
                            case CG_ADD:
                                fprintf(out, "\taddl %%ebx, %%eax\n");
                                if (dest.size == CG_QWORD)
                                    fprintf(out, "\tadcl %%ecx, %%edx\n");
                                break;
                            case CG_SUB:
                                fprintf(out, "\tsub %%ebx, %%eax\n");
                                if (dest.size == CG_QWORD)
                                    fprintf(out, "\tsbb %%ecx, %%edx\n");
                                break;
                            case CG_MUL:
                                fprintf(out, "\timull %%ebx, %%eax\n");
                                //NOT IMPLEMENTED FOR 64-BIT
                                if (dest.size == CG_QWORD)
                                    fprintf(out, "\txorl %%edx, %%edx\n");
                                break;
                            case CG_DIV:
                                fprintf(out, "\tidivl %%ebx\n");
                                //NOT IMPLEMENTED FOR 64-BIT
                                if (dest.size == CG_QWORD)
                                    fprintf(out, "\txorl %%edx, %%edx\n");
                                break;
                            case CG_MOD:
                                fprintf(out, "\tidivl %%ebx\n");
                                fprintf(out, "\tmovl %%edx, %%eax\n");
                                //NOT IMPLEMENTED FOR 64-BIT
                                if (dest.size == CG_QWORD)
                                    fprintf(out, "\txorl %%edx, %%edx\n");
                                break;
                            case CG_LOGIC_AND: {
                                if (left.size == CG_QWORD)
                                    fprintf(out, "\torl %%edx, %%eax\n");
                                if (right.size == CG_QWORD)
                                    fprintf(out, "\torl %%ecx, %%ebx\n");
                                size_t end = (*unique_id)++;
                                fprintf(out, "\tcmpl $0, %%eax\n");
                                fprintf(out, "\tje _%s$%lu\n", name, end);
                                fprintf(out, "\tcmpl $0, %%ebx\n");
                                fprintf(out, "\tje _%s$%lu\n", name, end);
                                fprintf(out, "\tmovl $1, %%eax\n");
                                fprintf(out, "_%s$%lu:\n", name, end);
                                if (dest.size == CG_QWORD)
                                    fprintf(out, "\txorl %%edx, %%edx\n");
                                break;
                            }
                            case CG_LOGIC_OR: {
                                if (left.size == CG_QWORD)
                                    fprintf(out, "\torl %%edx, %%eax\n");
                                if (right.size == CG_QWORD)
                                    fprintf(out, "\torl %%ecx, %%ebx\n");
                                fprintf(out, "\torl %%ebx, %%eax\n");
                                size_t end = (*unique_id)++;
                                fprintf(out, "\tcmpl $0, %%eax\n");
                                fprintf(out, "\tje _%s$%lu\n", name, end);
                                fprintf(out, "\tmovl $1, %%eax\n");
                                fprintf(out, "_%s$%lu:\n", name, end);
                                if (dest.size == CG_QWORD)
                                    fprintf(out, "\txorl %%edx, %%edx\n");
                                break;
                            }
                            case CG_LOGIC_XOR: {
                                if (left.size == CG_QWORD)
                                    fprintf(out, "\torl %%edx, %%eax\n");
                                if (right.size == CG_QWORD)
                                    fprintf(out, "\torl %%ecx, %%ebx\n");
                                size_t skip1 = (*unique_id)++;
                                size_t skip2 = (*unique_id)++;
                                size_t end = (*unique_id)++;
                                fprintf(out, "\tcmpl $0, %%eax\n");
                                fprintf(out, "\tje _%s$%lu\n", name, skip1);
                                fprintf(out, "\tcmpl $0, %%ebx\n");
                                fprintf(out, "\tje _%s$%lu\n", name, skip2);
                                fprintf(out, "\txorl %%eax, %%eax\n");
                                fprintf(out, "\tjmp _%s$%lu\n", name, end);
                                fprintf(out, "_%s$%lu:\n", name, skip1);
                                fprintf(out, "\tcmpl $0, %%ebx\n");
                                fprintf(out, "\tjne _%s$%lu\n", name, skip2);
                                fprintf(out, "\txorl %%eax, %%eax\n");
                                fprintf(out, "\tjmp _%s$%lu\n", name, end);
                                fprintf(out, "_%s$%lu:\n", name, skip2);
                                fprintf(out, "\tmovl $1, %%eax\n");
                                fprintf(out, "_%s$%lu:\n", name, end);
                                if (dest.size == CG_QWORD)
                                    fprintf(out, "\txorl %%edx, %%edx\n");
                                break;
                            }
                            case CG_BITWISE_AND:
                                fprintf(out, "\tandl %%ebx, %%eax\n");
                                if (dest.size == CG_QWORD) {
                                    if (right.size == CG_QWORD && left.size == CG_QWORD)
                                        fprintf(out, "\tandl %%ecx, %%edx\n");
                                    else
                                        fprintf(out, "\txorl %%edx, %%edx\n");
                                }
                                break;
                            case CG_BITWISE_OR:
                                fprintf(out, "\torl %%ebx, %%eax\n");
                                if (dest.size == CG_QWORD) {
                                    if (right.size == CG_QWORD) {
                                        if (left.size == CG_QWORD)
                                            fprintf(out, "\torl %%ecx, %%edx\n");
                                        else
                                            fprintf(out, "\tmovl %%ecx, %%edx\n");
                                    } else
                                        fprintf(out, "\txorl %%edx, %%edx\n");
                                }
                                break;
                            case CG_BITWISE_XOR:
                                fprintf(out, "\txorl %%ebx, %%eax\n");
                                if (dest.size == CG_QWORD) {
                                    if (right.size == CG_QWORD) {
                                        if (left.size == CG_QWORD)
                                            fprintf(out, "\txorl %%ecx, %%edx\n");
                                        else
                                            fprintf(out, "\tmovl %%ecx, %%edx\n");
                                    } else
                                        fprintf(out, "\txorl %%edx, %%edx\n");
                                }
                                break;
                            case CG_LT: {
                                bool _64 = true;
                                if (left.size == CG_QWORD) {
                                    if (right.size != CG_QWORD)
                                        fprintf(out, "\txorl %%ecx, %%ecx\n");
                                } else if (right.size == CG_QWORD)
                                    fprintf(out, "\txorl %%edx, %%edx\n");
                                else
                                    _64 = false;
                                size_t skip1 = (*unique_id)++;
                                size_t skip2 = (*unique_id)++;
                                size_t end = (*unique_id)++;
                                if (_64) {
                                    fprintf(out, "\tcmpl %%edx, %%ecx\n");
                                    fprintf(out, "\tjlt _%s$%lu\n", name, skip1);
                                    fprintf(out, "\tjgt _%s$%lu\n", name, skip2);
                                }
                                fprintf(out, "\tcmpl %%eax, %%ebx\n");
                                fprintf(out, "\tjlt _%s$%lu\n", name, skip1);
                                fprintf(out, "_%s$%lu:\n", name, skip2);
                                fprintf(out, "\txorl %%eax, %%eax\n");
                                fprintf(out, "\tjmp _%s$%lu\n", name, end);
                                fprintf(out, "_%s$%lu:\n", name, skip1);
                                fprintf(out, "\tmov $1, %%eax\n");
                                fprintf(out, "_%s$%lu:\n", name, end);
                                if (dest.size == CG_QWORD && (right.size != CG_QWORD || left.size == CG_QWORD))
                                    fprintf(out, "\txorl %%edx, %%edx\n");
                                break;
                            }
                            case CG_GT: {
                                bool _64 = true;
                                if (left.size == CG_QWORD) {
                                    if (right.size != CG_QWORD)
                                        fprintf(out, "\txorl %%ecx, %%ecx\n");
                                } else if (right.size == CG_QWORD)
                                    fprintf(out, "\txorl %%edx, %%edx\n");
                                else
                                    _64 = false;
                                size_t skip1 = (*unique_id)++;
                                size_t skip2 = (*unique_id)++;
                                size_t end = (*unique_id)++;
                                if (_64) {
                                    fprintf(out, "\tcmpl %%edx, %%ecx\n");
                                    fprintf(out, "\tjgt _%s$%lu\n", name, skip1);
                                    fprintf(out, "\tjlt _%s$%lu\n", name, skip2);
                                }
                                fprintf(out, "\tcmpl %%eax, %%ebx\n");
                                fprintf(out, "\tjgt _%s$%lu\n", name, skip1);
                                fprintf(out, "_%s$%lu:\n", name, skip2);
                                fprintf(out, "\txorl %%eax, %%eax\n");
                                fprintf(out, "\tjmp _%s$%lu\n", name, end);
                                fprintf(out, "_%s$%lu:\n", name, skip1);
                                fprintf(out, "\tmov $1, %%eax\n");
                                fprintf(out, "_%s$%lu:\n", name, end);
                                if (dest.size == CG_QWORD && (right.size != CG_QWORD || left.size == CG_QWORD))
                                    fprintf(out, "\txorl %%edx, %%edx\n");
                                break;
                            }
                            case CG_LTE: {
                                bool _64 = true;
                                if (left.size == CG_QWORD) {
                                    if (right.size != CG_QWORD)
                                        fprintf(out, "\txorl %%ecx, %%ecx\n");
                                } else if (right.size == CG_QWORD)
                                    fprintf(out, "\txorl %%edx, %%edx\n");
                                else
                                    _64 = false;
                                size_t skip1 = (*unique_id)++;
                                size_t skip2 = (*unique_id)++;
                                size_t end = (*unique_id)++;
                                if (_64) {
                                    fprintf(out, "\tcmpl %%edx, %%ecx\n");
                                    fprintf(out, "\tjgt _%s$%lu\n", name, skip1);
                                    fprintf(out, "\tjlt _%s$%lu\n", name, skip2);
                                }
                                fprintf(out, "\tcmpl %%eax, %%ebx\n");
                                fprintf(out, "\tjgt _%s$%lu\n", name, skip1);
                                fprintf(out, "_%s$%lu:\n", name, skip2);
                                fprintf(out, "\tmov $1, %%eax\n");
                                fprintf(out, "\tjmp _%s$%lu\n", name, end);
                                fprintf(out, "_%s$%lu:\n", name, skip1);
                                fprintf(out, "\txorl %%eax, %%eax\n");
                                fprintf(out, "_%s$%lu:\n", name, end);
                                if (dest.size == CG_QWORD && (right.size != CG_QWORD || left.size == CG_QWORD))
                                    fprintf(out, "\txorl %%edx, %%edx\n");
                                break;
                            }
                            case CG_GTE: {
                                bool _64 = true;
                                if (left.size == CG_QWORD) {
                                    if (right.size != CG_QWORD)
                                        fprintf(out, "\txorl %%ecx, %%ecx\n");
                                } else if (right.size == CG_QWORD)
                                    fprintf(out, "\txorl %%edx, %%edx\n");
                                else
                                    _64 = false;
                                size_t skip1 = (*unique_id)++;
                                size_t skip2 = (*unique_id)++;
                                size_t end = (*unique_id)++;
                                if (_64) {
                                    fprintf(out, "\tcmpl %%edx, %%ecx\n");
                                    fprintf(out, "\tjlt _%s$%lu\n", name, skip1);
                                    fprintf(out, "\tjgt _%s$%lu\n", name, skip2);
                                }
                                fprintf(out, "\tcmpl %%eax, %%ebx\n");
                                fprintf(out, "\tjlt _%s$%lu\n", name, skip1);
                                fprintf(out, "_%s$%lu:\n", name, skip2);
                                fprintf(out, "\tmov $1, %%eax\n");
                                fprintf(out, "\tjmp _%s$%lu\n", name, end);
                                fprintf(out, "_%s$%lu:\n", name, skip1);
                                fprintf(out, "\txorl %%eax, %%eax\n");
                                fprintf(out, "_%s$%lu:\n", name, end);
                                if (dest.size == CG_QWORD && (right.size != CG_QWORD || left.size == CG_QWORD))
                                    fprintf(out, "\txorl %%edx, %%edx\n");
                                break;
                            }
                            case CG_EQ: {
                                bool _64 = true;
                                if (left.size == CG_QWORD) {
                                    if (right.size != CG_QWORD)
                                        fprintf(out, "\txorl %%ecx, %%ecx\n");
                                } else if (right.size == CG_QWORD)
                                    fprintf(out, "\txorl %%edx, %%edx\n");
                                else
                                    _64 = false;
                                size_t skip = (*unique_id)++;
                                size_t end = (*unique_id)++;
                                if (_64) {
                                    fprintf(out, "\tcmpl %%edx, %%ecx\n");
                                    fprintf(out, "\tjne _%s$%lu\n", name, skip);
                                }
                                fprintf(out, "\tcmpl %%eax, %%ebx\n");
                                fprintf(out, "\tjne _%s$%lu\n", name, skip);
                                fprintf(out, "\tmovl $1, %%eax\n");
                                fprintf(out, "\tjmp _%s$%lu\n", name, end);
                                fprintf(out, "_%s$%lu:\n", name, skip);
                                fprintf(out, "\txorl %%eax, %%eax\n");
                                fprintf(out, "_%s$%lu:\n", name, end);
                                if (dest.size == CG_QWORD && (right.size != CG_QWORD || left.size == CG_QWORD))
                                    fprintf(out, "\txorl %%edx, %%edx\n");
                                break;
                            }
                            case CG_NEQ: {
                                bool _64 = true;
                                if (left.size == CG_QWORD) {
                                    if (right.size != CG_QWORD)
                                        fprintf(out, "\txorl %%ecx, %%ecx\n");
                                } else if (right.size == CG_QWORD)
                                    fprintf(out, "\txorl %%edx, %%edx\n");
                                else
                                    _64 = false;
                                size_t skip = (*unique_id)++;
                                size_t end = (*unique_id)++;
                                if (_64) {
                                    fprintf(out, "\tcmpl %%edx, %%ecx\n");
                                    fprintf(out, "\tjne _%s$%lu\n", name, skip);
                                }
                                fprintf(out, "\tcmpl %%eax, %%ebx\n");
                                fprintf(out, "\tjne _%s$%lu\n", name, skip);
                                fprintf(out, "\txorl %%eax, %%eax\n");
                                fprintf(out, "\tjmp _%s$%lu\n", name, end);
                                fprintf(out, "_%s$%lu:\n", name, skip);
                                fprintf(out, "\tmovl $1, %%eax\n");
                                fprintf(out, "_%s$%lu:\n", name, end);
                                if (dest.size == CG_QWORD && (right.size != CG_QWORD || left.size == CG_QWORD))
                                    fprintf(out, "\txorl %%edx, %%edx\n");
                                break;
                            }
                        }
                        if (dest.size == CG_QWORD) {
                            fprintf(out, "\tmovl %%eax, %lu(%%esp)\n", dest.offset + 4);
                            fprintf(out, "\tmovl %%edx, %lu(%%esp)\n", dest.offset);
                        } else
                            fprintf(out, "\tmov%c %%%s, %lu(%%esp)\n", var_size_map[dest.size], primary_registers[dest.size], dest.offset);
                        break;
                    }
                }
                break;
            }
            case CG_CALL: {
                struct cg_statement_call *data = &stmt->data.call;
                if (enable_comments) {
                    fprintf(out, "\t# $%lu = %s(", data->out, data->name);
                    for (size_t j = 0; j < data->arg_count; ++j)
                        fprintf(out, j ? ", $%lu" : "$%lu", data->args[j]);
                    fprintf(out, ")\n");
                }
                fprintf(stderr, "Warning: statement #%lu is a call, which is not implemented yet!\n", i);
                break;
            }
            case CG_ENDFUNC: {
                struct cg_statement_endfunc *data = &stmt->data.endfunc;
                if (enable_comments) {
                    if (function->return_type)
                        fprintf(out, "\t# return $%lu\n", data->var);
                    else
                        fprintf(out, "\t# return\n");
                }
                struct var_ref var = map[data->var];
                if (var.size == CG_QWORD) {
                    fprintf(out, "\tmovl %lu(%%esp), %%eax\n", var.offset + 4);
                    fprintf(out, "\tmovl %lu(%%esp), %%edx\n", var.offset);
                } else {
                    if (var.size != CG_LONG)
                        fprintf(out, "\txorl %%eax, %%eax\n");
                    fprintf(out, "\tmov%c %lu(%%esp), %%%s\n", var_size_map[var.size], var.offset, primary_registers[var.size]);
                }
                fprintf(out, "\tjmp _%s$end\n", name);
                break;
            }
            case CG_CREATEVAR: {
                struct cg_statement_createvar *data = &stmt->data.createvar;
                if (enable_comments)
                    fprintf(out, "\t# %s $%lu\n", var_size_names[data->size], data->var);
                size_t size = SIZE_CONVERT(data->size);
                size_t off = 0;
                struct linked_list_node_t *node = *reverse;
                size_t index = 0;
                while (node) {
                    struct var_ref var = map[*(size_t *) node->data];
                    if (var.offset - off >= size)
                        break;
                    off = var.offset + SIZE_CONVERT(var.size);
                    node = node->ptr;
                    ++index;
                }
                if (off + size > max_stack_size) {
                    struct defragment_state state = {.offset = 0, .map = map, .out = out};
                    linked_list_foreach(reverse, (struct delegate_t) {.func = (void (*)(void *, void *)) &defragment, .state = &state});
                    off = state.offset;
                }
                linked_list_insert(reverse, index, &data->var, sizeof(size_t));
                map[data->var] = (struct var_ref) {.offset = off, .size = data->size, .exists = true};
                break;
            }
            case CG_DESTROYVAR: {
                struct cg_statement_destroyvar *data = &stmt->data.destroyvar;
                if (enable_comments)
                    fprintf(out, "\t# delete $%lu\n", data->var);
                map[data->var].exists = false;
                struct linked_list_node_t *node = *reverse;
                size_t index = 0;
                while (node) {
                    if (*(size_t *) node->data == data->var)
                        break;
                    node = node->ptr;
                    ++index;
                }
                linked_list_remove(reverse, index);
                break;
            }
        }
    }
}

void generate_code(struct cg_file_graph *graph, FILE *out) {
    size_t unique_id = 0;
    fprintf(out, "\t.text\n");
    for (size_t i = 0; i < graph->function_count; ++i) {
        struct cg_function *f = &graph->functions[i];
        if (f->access_level)
            fprintf(out, "\t.globl %s\n", f->name);
        fprintf(out, "%s:\n", f->name);
        size_t var_count;
        size_t stack_size = get_block_stack_size(&f->body, f->arg_count, f->args, &var_count);
        struct var_ref *map = malloc(sizeof(struct var_ref) * var_count);
        linked_list_t *reverse = linked_list_create();
        size_t off = 0;
        fprintf(out, "\tpushl %%ebp\n");
        fprintf(out, "\tmovl %%esp, %%ebp\n");
        fprintf(out, "\tsubl $%lu, %%esp\n", stack_size);
        for (size_t j = 0; j < f->arg_count; ++j) {
            map[j] = (struct var_ref) {.offset = off, .size = f->args[j], .exists = true};
            linked_list_insert(reverse, j, &j, sizeof(size_t));
            write_move(off, stack_size + j * 4 + 8, f->args[j], out);
            off += SIZE_CONVERT(f->args[j]);
        }
        bzero(map + f->arg_count, sizeof(struct var_ref) * (var_count - f->arg_count));
        generate_block(f, &f->body, map, reverse, stack_size, &unique_id, f->name, out);
        free(map);
        fprintf(out, "_%s$end:\n", f->name);
        fprintf(out, "\tleave\n");
        fprintf(out, "\tret\n");
    }
}
