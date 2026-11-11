#include "../include/son_ir.h"
#include "../include/son_sema.h"
#include <stdarg.h>

static SonIRValue* make_val(SonArena *arena, SonIRValueKind kind, SonType *type) {
    SonIRValue *val = (SonIRValue*)son_arena_alloc(arena, sizeof(SonIRValue), sizeof(void*));
    memset(val, 0, sizeof(SonIRValue));
    val->kind = kind;
    val->type = type;
    return val;
}

static SonIRValue* ir_const_int(SonArena *arena, int64_t v, SonType *type) {
    SonIRValue *val = make_val(arena, IR_VAL_CONST_INT, type);
    val->int_val = v;
    return val;
}

static SonIRValue* ir_const_float(SonArena *arena, double v, SonType *type) {
    SonIRValue *val = make_val(arena, IR_VAL_CONST_FLOAT, type);
    val->float_val = v;
    return val;
}

static SonIRValue* ir_const_bool(SonArena *arena, bool b, SonType *type) {
    SonIRValue *val = make_val(arena, IR_VAL_CONST_BOOL, type);
    val->bool_val = b;
    return val;
}

static SonIRValue* ir_const_string(SonArena *arena, SonStringSlice s, SonType *type) {
    SonIRValue *val = make_val(arena, IR_VAL_CONST_STRING, type);
    val->string_val = s;
    return val;
}

static SonIRValue* ir_new_reg(SonArena *arena, SonIRFunc *func, SonType *type) {
    SonIRValue *val = make_val(arena, IR_VAL_REG, type);
    val->reg_id = func->next_reg_id++;
    return val;
}

static SonIRBlock* ir_new_block(SonArena *arena, SonIRFunc *func, const char *prefix) {
    SonIRBlock *block = (SonIRBlock*)son_arena_alloc(arena, sizeof(SonIRBlock), sizeof(void*));
    memset(block, 0, sizeof(SonIRBlock));
    block->id = func->next_block_id++;

    char name_buf[64];
    snprintf(name_buf, sizeof(name_buf), "%s%zu", prefix ? prefix : "bb", block->id);
    size_t len = strlen(name_buf);
    char *name_copy = (char*)son_arena_alloc(arena, len + 1, 1);
    memcpy(name_copy, name_buf, len + 1);

    block->name.data = name_copy;
    block->name.length = len;

    if (!func->first_block) {
        func->first_block = block;
        func->last_block = block;
    } else {
        func->last_block->next = block;
        func->last_block = block;
    }
    return block;
}

static void ir_append_inst(SonIRBlock *block, SonIRInst *inst) {
    if (!block->first_inst) {
        block->first_inst = inst;
        block->last_inst = inst;
    } else {
        block->last_inst->next = inst;
        block->last_inst = inst;
    }
}

static SonIRInst* ir_emit_inst(SonArena *arena, SonIRBlock *block, SonIROpKind op, SonIRValue *dest, size_t operand_count, ...) {
    SonIRInst *inst = (SonIRInst*)son_arena_alloc(arena, sizeof(SonIRInst), sizeof(void*));
    memset(inst, 0, sizeof(SonIRInst));
    inst->op = op;
    inst->dest = dest;
    inst->operand_count = operand_count;

    if (operand_count > 0) {
        inst->operands = (SonIRValue**)son_arena_alloc(arena, sizeof(SonIRValue*) * operand_count, sizeof(void*));
        va_list args;
        va_start(args, operand_count);
        for (size_t i = 0; i < operand_count; i++) {
            inst->operands[i] = va_arg(args, SonIRValue*);
        }
        va_end(args);
    }

    ir_append_inst(block, inst);
    return inst;
}

// Forward declarations
static SonIRValue* lower_expr(SonArena *arena, SonIRFunc *func, SonIRBlock **curr_block, SonExpr *expr);
static void lower_stmt(SonArena *arena, SonIRFunc *func, SonIRBlock **curr_block, SonStmt *stmt);

static SonIRValue* lower_expr(SonArena *arena, SonIRFunc *func, SonIRBlock **curr_block, SonExpr *expr) {
    if (!expr) return NULL;
    SonIRBlock *b = *curr_block;

    switch (expr->kind) {
        case EXPR_INT_LITERAL:
            return ir_const_int(arena, expr->int_val, expr->inferred_type);
        case EXPR_FLOAT_LITERAL:
            return ir_const_float(arena, expr->float_val, expr->inferred_type);
        case EXPR_BOOL_LITERAL:
            return ir_const_bool(arena, expr->bool_val, expr->inferred_type);
        case EXPR_STRING_LITERAL:
            return ir_const_string(arena, expr->string_val, expr->inferred_type);
        case EXPR_IDENTIFIER: {
            // Check if parameter
            for (size_t i = 0; i < func->param_count; i++) {
                if (son_slice_eq(func->params[i]->name, expr->ident_name)) {
                    return func->params[i];
                }
            }
            // Local variable: load value from its alloca pointer
            SonIRValue *var_loc = make_val(arena, IR_VAL_GLOBAL, expr->inferred_type);
            var_loc->name = expr->ident_name;

            SonIRValue *loaded = ir_new_reg(arena, func, expr->inferred_type);
            ir_emit_inst(arena, *curr_block, IR_OP_LOAD, loaded, 1, var_loc);
            return loaded;
        }
        case EXPR_UNARY: {
            SonIRValue *operand = lower_expr(arena, func, curr_block, expr->unary.operand);
            if (expr->unary.op == TOKEN_BANG) {
                SonIRValue *dest = ir_new_reg(arena, func, expr->inferred_type);
                SonIRValue *zero = ir_const_bool(arena, false, expr->inferred_type);
                ir_emit_inst(arena, *curr_block, IR_OP_EQ, dest, 2, operand, zero);
                return dest;
            } else if (expr->unary.op == TOKEN_MINUS) {
                SonIRValue *dest = ir_new_reg(arena, func, expr->inferred_type);
                SonIRValue *zero = ir_const_int(arena, 0, expr->inferred_type);
                ir_emit_inst(arena, *curr_block, IR_OP_SUB, dest, 2, zero, operand);
                return dest;
            } else if (expr->unary.op == TOKEN_KW_REF || expr->unary.op == TOKEN_KW_MUT) {
                return operand;
            }
            return operand;
        }
        case EXPR_BINARY: {
            SonIRValue *left = lower_expr(arena, func, curr_block, expr->binary.left);
            SonIRValue *right = lower_expr(arena, func, curr_block, expr->binary.right);
            b = *curr_block;

            SonIROpKind op = IR_OP_ADD;
            switch (expr->binary.op) {
                case TOKEN_PLUS: op = IR_OP_ADD; break;
                case TOKEN_MINUS: op = IR_OP_SUB; break;
                case TOKEN_STAR: op = IR_OP_MUL; break;
                case TOKEN_SLASH: op = IR_OP_DIV; break;
                case TOKEN_PERCENT: op = IR_OP_REM; break;
                case TOKEN_EQ_EQ: op = IR_OP_EQ; break;
                case TOKEN_BANG_EQ: op = IR_OP_NE; break;
                case TOKEN_LESS: op = IR_OP_LT; break;
                case TOKEN_LESS_EQ: op = IR_OP_LE; break;
                case TOKEN_GREATER: op = IR_OP_GT; break;
                case TOKEN_GREATER_EQ: op = IR_OP_GE; break;
                case TOKEN_AMP_AMP: op = IR_OP_AND; break;
                case TOKEN_PIPE_PIPE: op = IR_OP_OR; break;
                default: break;
            }

            SonIRValue *dest = ir_new_reg(arena, func, expr->inferred_type);
            ir_emit_inst(arena, b, op, dest, 2, left, right);
            return dest;
        }
        case EXPR_CALL: {
            size_t argc = expr->call.arg_count;
            SonIRValue **args = (SonIRValue**)son_arena_alloc(arena, sizeof(SonIRValue*) * (argc + 1), sizeof(void*));
            for (size_t i = 0; i < argc; i++) {
                args[i] = lower_expr(arena, func, curr_block, expr->call.args[i]);
            }
            b = *curr_block;

            if (expr->call.callee->kind != EXPR_IDENTIFIER) {
                lower_expr(arena, func, curr_block, expr->call.callee);
                b = *curr_block;
            }

            SonIRValue *dest = ir_new_reg(arena, func, expr->inferred_type);
            SonIRInst *inst = (SonIRInst*)son_arena_alloc(arena, sizeof(SonIRInst), sizeof(void*));
            memset(inst, 0, sizeof(SonIRInst));
            inst->op = IR_OP_CALL;
            inst->dest = dest;
            inst->operand_count = argc;
            inst->operands = args;
            if (expr->call.callee->kind == EXPR_IDENTIFIER) {
                inst->extra_name = expr->call.callee->ident_name;
            } else if (expr->call.callee->kind == EXPR_MEMBER) {
                inst->extra_name = expr->call.callee->member.member;
            }
            ir_append_inst(b, inst);
            return dest;
        }
        case EXPR_MEMBER: {
            SonIRValue *target = lower_expr(arena, func, curr_block, expr->member.target);
            b = *curr_block;
            SonIRValue *dest = ir_new_reg(arena, func, expr->inferred_type);
            SonIRInst *inst = ir_emit_inst(arena, b, IR_OP_GET_FIELD, dest, 1, target);
            inst->extra_name = expr->member.member;
            return dest;
        }
        case EXPR_INDEX: {
            SonIRValue *target = lower_expr(arena, func, curr_block, expr->index.target);
            SonIRValue *idx = lower_expr(arena, func, curr_block, expr->index.index);
            b = *curr_block;
            SonIRValue *dest = ir_new_reg(arena, func, expr->inferred_type);
            ir_emit_inst(arena, b, IR_OP_INDEX, dest, 2, target, idx);
            return dest;
        }
        case EXPR_CAST: {
            SonIRValue *val = lower_expr(arena, func, curr_block, expr->cast.expr);
            b = *curr_block;
            SonIRValue *dest = ir_new_reg(arena, func, expr->cast.target_type);
            ir_emit_inst(arena, b, IR_OP_CAST, dest, 1, val);
            return dest;
        }
        case EXPR_MATCH: {
            SonIRValue *target = lower_expr(arena, func, curr_block, expr->match_expr.target);
            SonIRBlock *merge_bb = ir_new_block(arena, func, "match_merge");
            SonIRValue *dest = expr->inferred_type ? ir_new_reg(arena, func, expr->inferred_type) : NULL;

            for (size_t i = 0; i < expr->match_expr.arm_count; i++) {
                SonMatchArm *arm = &expr->match_expr.arms[i];
                SonIRBlock *arm_bb = ir_new_block(arena, func, "match_arm");

                SonIRInst *br = ir_emit_inst(arena, *curr_block, IR_OP_MATCH_BRANCH, NULL, 1, target);
                br->target_true = arm_bb;
                br->extra_name = arm->variant_name;

                *curr_block = arm_bb;
                if (arm->is_expr) {
                    lower_expr(arena, func, curr_block, arm->expr);
                } else {
                    for (size_t s = 0; s < arm->body.stmt_count; s++) {
                        lower_stmt(arena, func, curr_block, arm->body.stmts[s]);
                    }
                }
                SonIRInst *jmp = ir_emit_inst(arena, *curr_block, IR_OP_JUMP, NULL, 0);
                jmp->target_true = merge_bb;
            }

            *curr_block = merge_bb;
            return dest;
        }
        default:
            return NULL;
    }
}

static void lower_stmt(SonArena *arena, SonIRFunc *func, SonIRBlock **curr_block, SonStmt *stmt) {
    if (!stmt) return;
    SonIRBlock *b = *curr_block;

    switch (stmt->kind) {
        case STMT_VAR_DECL: {
            SonIRValue *init_val = NULL;
            if (stmt->var_decl.init_expr) {
                init_val = lower_expr(arena, func, curr_block, stmt->var_decl.init_expr);
            }
            b = *curr_block;

            SonIRValue *var_loc = make_val(arena, IR_VAL_GLOBAL, stmt->var_decl.type_annot);
            var_loc->name = stmt->var_decl.name;

            SonIRInst *inst = (SonIRInst*)son_arena_alloc(arena, sizeof(SonIRInst), sizeof(void*));
            memset(inst, 0, sizeof(SonIRInst));
            inst->op = IR_OP_ALLOCA;
            inst->dest = var_loc;
            inst->extra_name = stmt->var_decl.name;
            ir_append_inst(b, inst);

            if (init_val) {
                ir_emit_inst(arena, b, IR_OP_STORE, NULL, 2, init_val, var_loc);
            }
            break;
        }
        case STMT_ASSIGN: {
            SonIRValue *rval = lower_expr(arena, func, curr_block, stmt->assign.rvalue);
            SonIRValue *lval = NULL;
            if (stmt->assign.lvalue && stmt->assign.lvalue->kind == EXPR_IDENTIFIER) {
                lval = make_val(arena, IR_VAL_GLOBAL, stmt->assign.lvalue->inferred_type);
                lval->name = stmt->assign.lvalue->ident_name;
            } else {
                lval = lower_expr(arena, func, curr_block, stmt->assign.lvalue);
            }
            b = *curr_block;
            ir_emit_inst(arena, b, IR_OP_STORE, NULL, 2, rval, lval);
            break;
        }
        case STMT_EXPR:
            lower_expr(arena, func, curr_block, stmt->expr);
            break;
        case STMT_RETURN: {
            SonIRValue *ret_val = NULL;
            if (stmt->ret.expr) {
                ret_val = lower_expr(arena, func, curr_block, stmt->ret.expr);
            }
            b = *curr_block;
            if (ret_val) {
                ir_emit_inst(arena, b, IR_OP_RETURN, NULL, 1, ret_val);
            } else {
                ir_emit_inst(arena, b, IR_OP_RETURN, NULL, 0);
            }
            break;
        }
        case STMT_IF: {
            SonIRValue *cond = lower_expr(arena, func, curr_block, stmt->if_stmt.cond);
            b = *curr_block;

            SonIRBlock *then_bb = ir_new_block(arena, func, "then_bb");
            SonIRBlock *else_bb = stmt->if_stmt.has_else ? ir_new_block(arena, func, "else_bb") : NULL;
            SonIRBlock *merge_bb = ir_new_block(arena, func, "merge_bb");

            SonIRInst *br_inst = ir_emit_inst(arena, b, IR_OP_BRANCH, NULL, 1, cond);
            br_inst->target_true = then_bb;
            br_inst->target_false = else_bb ? else_bb : merge_bb;

            // Lower then block
            *curr_block = then_bb;
            for (size_t i = 0; i < stmt->if_stmt.then_block.stmt_count; i++) {
                lower_stmt(arena, func, curr_block, stmt->if_stmt.then_block.stmts[i]);
            }
            SonIRInst *then_jump = ir_emit_inst(arena, *curr_block, IR_OP_JUMP, NULL, 0);
            then_jump->target_true = merge_bb;

            // Lower else block
            if (else_bb) {
                *curr_block = else_bb;
                for (size_t i = 0; i < stmt->if_stmt.else_block.stmt_count; i++) {
                    lower_stmt(arena, func, curr_block, stmt->if_stmt.else_block.stmts[i]);
                }
                SonIRInst *else_jump = ir_emit_inst(arena, *curr_block, IR_OP_JUMP, NULL, 0);
                else_jump->target_true = merge_bb;
            }

            *curr_block = merge_bb;
            break;
        }
        case STMT_WHILE: {
            SonIRBlock *cond_bb = ir_new_block(arena, func, "loop_cond");
            SonIRBlock *body_bb = ir_new_block(arena, func, "loop_body");
            SonIRBlock *exit_bb = ir_new_block(arena, func, "loop_exit");

            SonIRInst *init_jump = ir_emit_inst(arena, b, IR_OP_JUMP, NULL, 0);
            init_jump->target_true = cond_bb;

            *curr_block = cond_bb;
            SonIRValue *cond = lower_expr(arena, func, curr_block, stmt->while_stmt.cond);
            SonIRInst *br = ir_emit_inst(arena, *curr_block, IR_OP_BRANCH, NULL, 1, cond);
            br->target_true = body_bb;
            br->target_false = exit_bb;

            *curr_block = body_bb;
            for (size_t i = 0; i < stmt->while_stmt.body.stmt_count; i++) {
                lower_stmt(arena, func, curr_block, stmt->while_stmt.body.stmts[i]);
            }
            SonIRInst *back_jump = ir_emit_inst(arena, *curr_block, IR_OP_JUMP, NULL, 0);
            back_jump->target_true = cond_bb;

            *curr_block = exit_bb;
            break;
        }
        case STMT_BLOCK: {
            for (size_t i = 0; i < stmt->block.stmt_count; i++) {
                lower_stmt(arena, func, curr_block, stmt->block.stmts[i]);
            }
            break;
        }
        default:
            break;
    }
}

SonIRModule* son_ir_build_module(SonArena *arena, SonModule *ast_module) {
    SonIRModule *ir = (SonIRModule*)son_arena_alloc(arena, sizeof(SonIRModule), sizeof(void*));
    memset(ir, 0, sizeof(SonIRModule));
    ir->arena = arena;
    ir->ast_module = ast_module;

    for (size_t i = 0; i < ast_module->decl_count; i++) {
        SonDecl *decl = ast_module->decls[i];
        if (decl->kind == DECL_FUNCTION) {
            SonIRFunc *func = (SonIRFunc*)son_arena_alloc(arena, sizeof(SonIRFunc), sizeof(void*));
            memset(func, 0, sizeof(SonIRFunc));
            func->name = decl->name;
            func->is_extern = decl->is_extern;
            func->return_type = decl->func.return_type;
            func->param_count = decl->func.param_count;

            if (func->param_count > 0) {
                func->params = (SonIRValue**)son_arena_alloc(arena, sizeof(SonIRValue*) * func->param_count, sizeof(void*));
                for (size_t p = 0; p < func->param_count; p++) {
                    func->params[p] = make_val(arena, IR_VAL_PARAM, decl->func.params[p].type);
                    func->params[p]->name = decl->func.params[p].name;
                }
            }

            if (!func->is_extern) {
                SonIRBlock *entry_bb = ir_new_block(arena, func, "bb");
                func->entry_block = entry_bb;

                SonIRBlock *curr_bb = entry_bb;
                for (size_t s = 0; s < decl->func.body.stmt_count; s++) {
                    lower_stmt(arena, func, &curr_bb, decl->func.body.stmts[s]);
                }
            }

            if (!ir->first_func) {
                ir->first_func = func;
                ir->last_func = func;
            } else {
                ir->last_func->next = func;
                ir->last_func = func;
            }
        }
    }

    return ir;
}

static void dump_val(FILE *out, SonIRValue *val) {
    if (!val) {
        fprintf(out, "none");
        return;
    }
    switch (val->kind) {
        case IR_VAL_CONST_INT:
            fprintf(out, "%lld:%s", (long long)val->int_val, val->type ? son_type_to_string(val->type) : "i32");
            break;
        case IR_VAL_CONST_FLOAT:
            fprintf(out, "%f:f64", val->float_val);
            break;
        case IR_VAL_CONST_BOOL:
            fprintf(out, "%s:bool", val->bool_val ? "true" : "false");
            break;
        case IR_VAL_CONST_CHAR:
            fprintf(out, "'%c':char", val->char_val);
            break;
        case IR_VAL_CONST_STRING:
            fprintf(out, "\"%.*s\":str", (int)val->string_val.length, val->string_val.data);
            break;
        case IR_VAL_REG:
            fprintf(out, "%%%zu", val->reg_id);
            break;
        case IR_VAL_PARAM:
        case IR_VAL_GLOBAL:
            fprintf(out, "%%%.*s", (int)val->name.length, val->name.data);
            break;
    }
}

void son_ir_dump(FILE *out, SonIRModule *ir_module) {
    fprintf(out, "; SonIR (Intermediate Representation)\n");
    fprintf(out, "; Target-independent SSA Control-Flow Graph\n\n");

    SonIRFunc *func = ir_module->first_func;
    while (func) {
        fprintf(out, "func @%.*s(", (int)func->name.length, func->name.data);
        for (size_t p = 0; p < func->param_count; p++) {
            if (p > 0) fprintf(out, ", ");
            fprintf(out, "%%%.*s: %s",
                    (int)func->params[p]->name.length, func->params[p]->name.data,
                    son_type_to_string(func->params[p]->type));
        }
        fprintf(out, ") -> %s {\n", son_type_to_string(func->return_type));

        SonIRBlock *block = func->first_block;
        while (block) {
            fprintf(out, "%.*s:\n", (int)block->name.length, block->name.data);

            SonIRInst *inst = block->first_inst;
            while (inst) {
                fprintf(out, "    ");
                if (inst->dest) {
                    dump_val(out, inst->dest);
                    fprintf(out, " = ");
                }

                switch (inst->op) {
                    case IR_OP_ALLOCA:
                        fprintf(out, "alloca %s", inst->dest && inst->dest->type ? son_type_to_string(inst->dest->type) : "val");
                        break;
                    case IR_OP_LOAD:
                        fprintf(out, "load ");
                        dump_val(out, inst->operands[0]);
                        break;
                    case IR_OP_STORE:
                        fprintf(out, "store ");
                        dump_val(out, inst->operands[0]);
                        fprintf(out, ", ");
                        dump_val(out, inst->operands[1]);
                        break;
                    case IR_OP_ADD: fprintf(out, "add "); dump_val(out, inst->operands[0]); fprintf(out, ", "); dump_val(out, inst->operands[1]); break;
                    case IR_OP_SUB: fprintf(out, "sub "); dump_val(out, inst->operands[0]); fprintf(out, ", "); dump_val(out, inst->operands[1]); break;
                    case IR_OP_MUL: fprintf(out, "mul "); dump_val(out, inst->operands[0]); fprintf(out, ", "); dump_val(out, inst->operands[1]); break;
                    case IR_OP_DIV: fprintf(out, "div "); dump_val(out, inst->operands[0]); fprintf(out, ", "); dump_val(out, inst->operands[1]); break;
                    case IR_OP_REM: fprintf(out, "rem "); dump_val(out, inst->operands[0]); fprintf(out, ", "); dump_val(out, inst->operands[1]); break;
                    case IR_OP_EQ:  fprintf(out, "cmp eq "); dump_val(out, inst->operands[0]); fprintf(out, ", "); dump_val(out, inst->operands[1]); break;
                    case IR_OP_NE:  fprintf(out, "cmp ne "); dump_val(out, inst->operands[0]); fprintf(out, ", "); dump_val(out, inst->operands[1]); break;
                    case IR_OP_LT:  fprintf(out, "cmp lt "); dump_val(out, inst->operands[0]); fprintf(out, ", "); dump_val(out, inst->operands[1]); break;
                    case IR_OP_LE:  fprintf(out, "cmp le "); dump_val(out, inst->operands[0]); fprintf(out, ", "); dump_val(out, inst->operands[1]); break;
                    case IR_OP_GT:  fprintf(out, "cmp gt "); dump_val(out, inst->operands[0]); fprintf(out, ", "); dump_val(out, inst->operands[1]); break;
                    case IR_OP_GE:  fprintf(out, "cmp ge "); dump_val(out, inst->operands[0]); fprintf(out, ", "); dump_val(out, inst->operands[1]); break;
                    case IR_OP_AND: fprintf(out, "and "); dump_val(out, inst->operands[0]); fprintf(out, ", "); dump_val(out, inst->operands[1]); break;
                    case IR_OP_OR:  fprintf(out, "or "); dump_val(out, inst->operands[0]); fprintf(out, ", "); dump_val(out, inst->operands[1]); break;
                    case IR_OP_CALL:
                        fprintf(out, "call @%.*s(", (int)inst->extra_name.length, inst->extra_name.data);
                        for (size_t a = 0; a < inst->operand_count; a++) {
                            if (a > 0) fprintf(out, ", ");
                            dump_val(out, inst->operands[a]);
                        }
                        fprintf(out, ")");
                        break;
                    case IR_OP_GET_FIELD:
                        fprintf(out, "get_field ");
                        dump_val(out, inst->operands[0]);
                        fprintf(out, ", .%.*s", (int)inst->extra_name.length, inst->extra_name.data);
                        break;
                    case IR_OP_INDEX:
                        fprintf(out, "index ");
                        dump_val(out, inst->operands[0]);
                        fprintf(out, "[");
                        dump_val(out, inst->operands[1]);
                        fprintf(out, "]");
                        break;
                    case IR_OP_CAST:
                        fprintf(out, "cast ");
                        dump_val(out, inst->operands[0]);
                        fprintf(out, " as %s", inst->dest && inst->dest->type ? son_type_to_string(inst->dest->type) : "type");
                        break;
                    case IR_OP_JUMP:
                        fprintf(out, "jump %.*s", (int)inst->target_true->name.length, inst->target_true->name.data);
                        break;
                    case IR_OP_BRANCH:
                        fprintf(out, "branch ");
                        dump_val(out, inst->operands[0]);
                        fprintf(out, ", %.*s, %.*s",
                                (int)inst->target_true->name.length, inst->target_true->name.data,
                                (int)inst->target_false->name.length, inst->target_false->name.data);
                        break;
                    case IR_OP_RETURN:
                        fprintf(out, "return");
                        if (inst->operand_count > 0) {
                            fprintf(out, " ");
                            dump_val(out, inst->operands[0]);
                        }
                        break;
                    default:
                        break;
                }
                fprintf(out, "\n");
                inst = inst->next;
            }

            block = block->next;
        }

        fprintf(out, "}\n\n");
        func = func->next;
    }
}
