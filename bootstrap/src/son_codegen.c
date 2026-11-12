#include "../include/son_codegen.h"

void son_codegen_init(SonCodeGen *gen, FILE *output) {
    gen->out = output;
    gen->indent_level = 0;
    gen->has_error = false;
    gen->current_fn_ret_type = NULL;
    gen->defer_top = 0;
    gen->fn_defer_base = 0;
    gen->loop_defer_base = 0;
}

static void print_indent(SonCodeGen *gen) {
    for (size_t i = 0; i < gen->indent_level; i++) {
        fprintf(gen->out, "    ");
    }
}

static void emit_type_ident(SonCodeGen *gen, SonType *type) {
    if (!type) {
        fprintf(gen->out, "void");
        return;
    }
    switch (type->kind) {
        case TYPE_U8: fprintf(gen->out, "u8"); break;
        case TYPE_I32: fprintf(gen->out, "i32"); break;
        case TYPE_I64: fprintf(gen->out, "i64"); break;
        case TYPE_CHAR: fprintf(gen->out, "char"); break;
        case TYPE_STRUCT:
        case TYPE_UNION:
            fprintf(gen->out, "%.*s", (int)type->name.length, type->name.data);
            break;
        default:
            fprintf(gen->out, "val");
            break;
    }
}

static void emit_type(SonCodeGen *gen, SonType *type) {
    if (!type) {
        fprintf(gen->out, "void");
        return;
    }

    switch (type->kind) {
        case TYPE_VOID: fprintf(gen->out, "void"); break;
        case TYPE_BOOL: fprintf(gen->out, "bool"); break;
        case TYPE_CHAR: fprintf(gen->out, "char"); break;
        case TYPE_I8:   fprintf(gen->out, "int8_t"); break;
        case TYPE_I16:  fprintf(gen->out, "int16_t"); break;
        case TYPE_I32:  fprintf(gen->out, "int32_t"); break;
        case TYPE_I64:  fprintf(gen->out, "int64_t"); break;
        case TYPE_ISIZE: fprintf(gen->out, "intptr_t"); break;
        case TYPE_U8:   fprintf(gen->out, "uint8_t"); break;
        case TYPE_U16:  fprintf(gen->out, "uint16_t"); break;
        case TYPE_U32:  fprintf(gen->out, "uint32_t"); break;
        case TYPE_U64:  fprintf(gen->out, "uint64_t"); break;
        case TYPE_USIZE: fprintf(gen->out, "size_t"); break;
        case TYPE_F32:  fprintf(gen->out, "float"); break;
        case TYPE_F64:  fprintf(gen->out, "double"); break;
        case TYPE_SLICE:
            fprintf(gen->out, "SonSlice_");
            emit_type_ident(gen, type->sub_type);
            break;
        case TYPE_ARRAY:
            emit_type(gen, type->sub_type);
            break;
        case TYPE_POINTER:
            if (!type->is_mut) fprintf(gen->out, "const ");
            emit_type(gen, type->sub_type);
            fprintf(gen->out, "*");
            break;
        case TYPE_REFERENCE:
            if (!type->is_mut) fprintf(gen->out, "const ");
            emit_type(gen, type->sub_type);
            if (type->is_mut) {
                fprintf(gen->out, "* __restrict");
            } else {
                fprintf(gen->out, "*");
            }
            break;
        case TYPE_STRUCT:
        case TYPE_UNION:
            fprintf(gen->out, "%.*s", (int)type->name.length, type->name.data);
            break;
        default:
            fprintf(gen->out, "void*");
            break;
    }
}

static void emit_stmt(SonCodeGen *gen, SonStmt *stmt);

static void emit_expr(SonCodeGen *gen, SonExpr *expr) {
    if (!expr) return;

    switch (expr->kind) {
        case EXPR_INT_LITERAL:
            fprintf(gen->out, "%lld", (long long)expr->int_val);
            break;
        case EXPR_FLOAT_LITERAL:
            fprintf(gen->out, "%f", expr->float_val);
            break;
        case EXPR_CHAR_LITERAL:
            if (expr->char_val == '\n') fprintf(gen->out, "'\\n'");
            else if (expr->char_val == '\t') fprintf(gen->out, "'\\t'");
            else if (expr->char_val == '\r') fprintf(gen->out, "'\\r'");
            else if (expr->char_val == '\0') fprintf(gen->out, "'\\0'");
            else if (expr->char_val == '\\') fprintf(gen->out, "'\\\\'");
            else if (expr->char_val == '\'') fprintf(gen->out, "'\\''");
            else fprintf(gen->out, "'%c'", expr->char_val);
            break;
        case EXPR_STRING_LITERAL:
            fprintf(gen->out, "((SonSlice_u8){ .data = (uint8_t*)\"%.*s\", .len = (sizeof(\"%.*s\") - 1) })",
                    (int)expr->string_val.length, expr->string_val.data,
                    (int)expr->string_val.length, expr->string_val.data);
            break;
        case EXPR_BOOL_LITERAL:
            fprintf(gen->out, "%s", expr->bool_val ? "true" : "false");
            break;
        case EXPR_IDENTIFIER:
            fprintf(gen->out, "%.*s", (int)expr->ident_name.length, expr->ident_name.data);
            break;
        case EXPR_BINARY:
            fprintf(gen->out, "(");
            emit_expr(gen, expr->binary.left);
            switch (expr->binary.op) {
                case TOKEN_PLUS: fprintf(gen->out, " + "); break;
                case TOKEN_MINUS: fprintf(gen->out, " - "); break;
                case TOKEN_STAR: fprintf(gen->out, " * "); break;
                case TOKEN_SLASH: fprintf(gen->out, " / "); break;
                case TOKEN_PERCENT: fprintf(gen->out, " %% "); break;
                case TOKEN_CARET: fprintf(gen->out, " ^ "); break;
                case TOKEN_SHL: fprintf(gen->out, " << "); break;
                case TOKEN_SHR: fprintf(gen->out, " >> "); break;
                case TOKEN_AMP: fprintf(gen->out, " & "); break;
                case TOKEN_PIPE: fprintf(gen->out, " | "); break;
                case TOKEN_EQ_EQ: fprintf(gen->out, " == "); break;
                case TOKEN_BANG_EQ: fprintf(gen->out, " != "); break;
                case TOKEN_LESS: fprintf(gen->out, " < "); break;
                case TOKEN_LESS_EQ: fprintf(gen->out, " <= "); break;
                case TOKEN_GREATER: fprintf(gen->out, " > "); break;
                case TOKEN_GREATER_EQ: fprintf(gen->out, " >= "); break;
                case TOKEN_AMP_AMP: fprintf(gen->out, " && "); break;
                case TOKEN_PIPE_PIPE: fprintf(gen->out, " || "); break;
                default: fprintf(gen->out, " ? "); break;
            }
            emit_expr(gen, expr->binary.right);
            fprintf(gen->out, ")");
            break;
        case EXPR_UNARY:
            if (expr->unary.op == TOKEN_MINUS) fprintf(gen->out, "-");
            else if (expr->unary.op == TOKEN_BANG) fprintf(gen->out, "!");
            else if (expr->unary.op == TOKEN_TILDE) fprintf(gen->out, "~");
            else if (expr->unary.op == TOKEN_KW_MUT || expr->unary.op == TOKEN_KW_REF) {
                fprintf(gen->out, "&(");
                emit_expr(gen, expr->unary.operand);
                fprintf(gen->out, ")");
                break;
            }
            emit_expr(gen, expr->unary.operand);
            break;
        case EXPR_CALL:
            // Check if call is a union variant constructor: Union.Variant(arg)
            if (expr->call.callee->kind == EXPR_MEMBER) {
                SonExpr *target = expr->call.callee->member.target;
                if (target->inferred_type && target->inferred_type->kind == TYPE_UNION) {
                    fprintf(gen->out, "%.*s_%.*s(",
                            (int)target->inferred_type->name.length, target->inferred_type->name.data,
                            (int)expr->call.callee->member.member.length, expr->call.callee->member.member.data);
                    for (size_t i = 0; i < expr->call.arg_count; i++) {
                        if (i > 0) fprintf(gen->out, ", ");
                        emit_expr(gen, expr->call.args[i]);
                    }
                    fprintf(gen->out, ")");
                    break;
                }
            }

            emit_expr(gen, expr->call.callee);
            fprintf(gen->out, "(");
            for (size_t i = 0; i < expr->call.arg_count; i++) {
                if (i > 0) fprintf(gen->out, ", ");
                SonExpr *arg = expr->call.args[i];
                if (arg->inferred_type && arg->inferred_type->kind == TYPE_ARRAY) {
                    fprintf(gen->out, "(SonSlice_");
                    emit_type_ident(gen, arg->inferred_type->sub_type);
                    fprintf(gen->out, "){ .data = ");
                    emit_expr(gen, arg);
                    fprintf(gen->out, ", .len = %zu }", arg->inferred_type->array_len);
                } else {
                    emit_expr(gen, arg);
                }
            }
            fprintf(gen->out, ")");
            break;
        case EXPR_MEMBER:
            // Check if member is a variant without payload: Union.Variant
            if (expr->member.target->inferred_type && expr->member.target->inferred_type->kind == TYPE_UNION) {
                fprintf(gen->out, "%.*s_%.*s()",
                        (int)expr->member.target->inferred_type->name.length, expr->member.target->inferred_type->name.data,
                        (int)expr->member.member.length, expr->member.member.data);
                break;
            }
            emit_expr(gen, expr->member.target);
            if (expr->member.target->inferred_type &&
                (expr->member.target->inferred_type->kind == TYPE_REFERENCE ||
                 expr->member.target->inferred_type->kind == TYPE_POINTER)) {
                fprintf(gen->out, "->%.*s", (int)expr->member.member.length, expr->member.member.data);
            } else {
                fprintf(gen->out, ".%.*s", (int)expr->member.member.length, expr->member.member.data);
            }
            break;
        case EXPR_INDEX:
            emit_expr(gen, expr->index.target);
            if (expr->index.target->inferred_type && (expr->index.target->inferred_type->kind == TYPE_ARRAY || expr->index.target->inferred_type->kind == TYPE_POINTER)) {
                fprintf(gen->out, "[");
            } else {
                fprintf(gen->out, ".data[");
            }
            emit_expr(gen, expr->index.index);
            fprintf(gen->out, "]");
            break;
        case EXPR_CAST:
            fprintf(gen->out, "((");
            emit_type(gen, expr->cast.target_type);
            fprintf(gen->out, ")");
            emit_expr(gen, expr->cast.expr);
            fprintf(gen->out, ")");
            break;
        case EXPR_STRUCT_INIT:
            fprintf(gen->out, "(%.*s){", (int)expr->struct_init.struct_name.length, expr->struct_init.struct_name.data);
            for (size_t i = 0; i < expr->struct_init.field_count; i++) {
                if (i > 0) fprintf(gen->out, ", ");
                fprintf(gen->out, ".%.*s = ", (int)expr->struct_init.fields[i].name.length, expr->struct_init.fields[i].name.data);
                emit_expr(gen, expr->struct_init.fields[i].value);
            }
            fprintf(gen->out, "}");
            break;
        case EXPR_MATCH: {
            fprintf(gen->out, "({\n");
            gen->indent_level++;

            print_indent(gen);
            fprintf(gen->out, "const SON_AUTO _target = ");
            emit_expr(gen, expr->match_expr.target);
            fprintf(gen->out, ";\n");

            if (expr->inferred_type && expr->inferred_type->kind != TYPE_VOID) {
                print_indent(gen);
                emit_type(gen, expr->inferred_type);
                fprintf(gen->out, " _res;\n");
            }

            print_indent(gen);
            fprintf(gen->out, "switch (_target.tag) {\n");
            gen->indent_level++;

            SonType *tgt_type = expr->match_expr.target->inferred_type;
            for (size_t i = 0; i < expr->match_expr.arm_count; i++) {
                SonMatchArm *arm = &expr->match_expr.arms[i];
                print_indent(gen);
                if (arm->is_default) {
                    fprintf(gen->out, "default: {\n");
                } else if (tgt_type) {
                    fprintf(gen->out, "case TAG_%.*s_%.*s: {\n",
                            (int)tgt_type->name.length, tgt_type->name.data,
                            (int)arm->variant_name.length, arm->variant_name.data);
                } else {
                    fprintf(gen->out, "case TAG_%.*s: {\n", (int)arm->variant_name.length, arm->variant_name.data);
                }
                gen->indent_level++;

                if (arm->has_payload) {
                    print_indent(gen);
                    fprintf(gen->out, "const SON_AUTO %.*s = _target.data.%.*s;\n",
                            (int)arm->payload_var.length, arm->payload_var.data,
                            (int)arm->variant_name.length, arm->variant_name.data);
                }

                if (arm->is_expr) {
                    print_indent(gen);
                    if (expr->inferred_type && expr->inferred_type->kind != TYPE_VOID) {
                        fprintf(gen->out, "_res = ");
                        emit_expr(gen, arm->expr);
                        fprintf(gen->out, ";\n");
                    } else {
                        emit_expr(gen, arm->expr);
                        fprintf(gen->out, ";\n");
                    }
                    print_indent(gen);
                    fprintf(gen->out, "break;\n");
                } else {
                    for (size_t s = 0; s < arm->body.stmt_count; s++) {
                        emit_stmt(gen, arm->body.stmts[s]);
                    }
                    print_indent(gen);
                    fprintf(gen->out, "break;\n");
                }

                gen->indent_level--;
                print_indent(gen);
                fprintf(gen->out, "}\n");
            }

            gen->indent_level--;
            print_indent(gen);
            fprintf(gen->out, "}\n");

            if (expr->inferred_type && expr->inferred_type->kind != TYPE_VOID) {
                print_indent(gen);
                fprintf(gen->out, "_res;\n");
            }

            gen->indent_level--;
            print_indent(gen);
            fprintf(gen->out, "})");
            break;
        }
        case EXPR_ARRAY_LITERAL: {
            fprintf(gen->out, "{ ");
            for (size_t i = 0; i < expr->array_literal.element_count; i++) {
                if (i > 0) fprintf(gen->out, ", ");
                emit_expr(gen, expr->array_literal.elements[i]);
            }
            fprintf(gen->out, " }");
            break;
        }
    }
}

static void emit_stmt(SonCodeGen *gen, SonStmt *stmt);

static void emit_defers_down_to(SonCodeGen *gen, int base) {
    for (int i = gen->defer_top - 1; i >= base; i--) {
        emit_stmt(gen, gen->defer_stack[i]);
    }
}

static void emit_block(SonCodeGen *gen, SonBlock block) {
    int saved_defer_top = gen->defer_top;
    fprintf(gen->out, "{\n");
    gen->indent_level++;
    bool terminated = false;
    for (size_t i = 0; i < block.stmt_count; i++) {
        emit_stmt(gen, block.stmts[i]);
        SonStmtKind k = block.stmts[i] ? block.stmts[i]->kind : STMT_EXPR;
        if (k == STMT_RETURN || k == STMT_BREAK || k == STMT_CONTINUE) {
            terminated = true;
            break;
        }
    }
    if (!terminated && gen->defer_top > saved_defer_top) {
        emit_defers_down_to(gen, saved_defer_top);
    }
    gen->defer_top = saved_defer_top;
    gen->indent_level--;
    print_indent(gen);
    fprintf(gen->out, "}\n");
}

static void emit_stmt(SonCodeGen *gen, SonStmt *stmt) {
    if (!stmt) return;
    if (stmt->kind == STMT_DEFER) {
        if (gen->defer_top < SON_MAX_DEFER_STACK) {
            gen->defer_stack[gen->defer_top++] = stmt->defer_stmt.stmt;
        }
        return;
    }
    print_indent(gen);

    switch (stmt->kind) {
        case STMT_VAR_DECL: {
            SonType *vt = stmt->var_decl.type_annot ? stmt->var_decl.type_annot : (stmt->var_decl.init_expr ? stmt->var_decl.init_expr->inferred_type : NULL);
            if (!stmt->var_decl.is_var) {
                if (!(vt && (vt->kind == TYPE_POINTER || vt->kind == TYPE_REFERENCE) && vt->is_mut)) {
                    fprintf(gen->out, "const ");
                }
            }
            emit_type(gen, vt);
            fprintf(gen->out, " %.*s", (int)stmt->var_decl.name.length, stmt->var_decl.name.data);
            if (vt && vt->kind == TYPE_ARRAY) {
                fprintf(gen->out, "[%zu]", vt->array_len);
            }
            if (stmt->var_decl.init_expr) {
                fprintf(gen->out, " = ");
                emit_expr(gen, stmt->var_decl.init_expr);
            }
            fprintf(gen->out, ";\n");
            break;
        }
        case STMT_ASSIGN:
            emit_expr(gen, stmt->assign.lvalue);
            fprintf(gen->out, " = ");
            emit_expr(gen, stmt->assign.rvalue);
            fprintf(gen->out, ";\n");
            break;
        case STMT_EXPR:
            emit_expr(gen, stmt->expr);
            fprintf(gen->out, ";\n");
            break;
        case STMT_RETURN:
            if (gen->defer_top > gen->fn_defer_base) {
                if (stmt->ret.expr && gen->current_fn_ret_type && gen->current_fn_ret_type->kind != TYPE_VOID) {
                    fprintf(gen->out, "{ ");
                    emit_type(gen, gen->current_fn_ret_type);
                    fprintf(gen->out, " _son_ret = ");
                    emit_expr(gen, stmt->ret.expr);
                    fprintf(gen->out, ";\n");
                    gen->indent_level++;
                    emit_defers_down_to(gen, gen->fn_defer_base);
                    print_indent(gen);
                    fprintf(gen->out, "return _son_ret;\n");
                    gen->indent_level--;
                    print_indent(gen);
                    fprintf(gen->out, "}\n");
                } else {
                    fprintf(gen->out, "{\n");
                    gen->indent_level++;
                    emit_defers_down_to(gen, gen->fn_defer_base);
                    print_indent(gen);
                    fprintf(gen->out, "return;\n");
                    gen->indent_level--;
                    print_indent(gen);
                    fprintf(gen->out, "}\n");
                }
                break;
            }
            fprintf(gen->out, "return");
            if (stmt->ret.expr) {
                fprintf(gen->out, " ");
                emit_expr(gen, stmt->ret.expr);
            }
            fprintf(gen->out, ";\n");
            break;
        case STMT_IF:
            fprintf(gen->out, "if ");
            if (stmt->if_stmt.cond->kind == EXPR_BINARY) {
                emit_expr(gen, stmt->if_stmt.cond);
                fprintf(gen->out, " ");
            } else {
                fprintf(gen->out, "(");
                emit_expr(gen, stmt->if_stmt.cond);
                fprintf(gen->out, ") ");
            }
            emit_block(gen, stmt->if_stmt.then_block);
            if (stmt->if_stmt.has_else) {
                print_indent(gen);
                fprintf(gen->out, "else ");
                emit_block(gen, stmt->if_stmt.else_block);
            }
            break;
        case STMT_WHILE: {
            int prev_loop_base = gen->loop_defer_base;
            gen->loop_defer_base = gen->defer_top;
            fprintf(gen->out, "while ");
            if (stmt->while_stmt.cond->kind == EXPR_BINARY) {
                emit_expr(gen, stmt->while_stmt.cond);
                fprintf(gen->out, " ");
            } else {
                fprintf(gen->out, "(");
                emit_expr(gen, stmt->while_stmt.cond);
                fprintf(gen->out, ") ");
            }
            emit_block(gen, stmt->while_stmt.body);
            gen->loop_defer_base = prev_loop_base;
            break;
        }
        case STMT_FOR: {
            int prev_loop_base = gen->loop_defer_base;
            gen->loop_defer_base = gen->defer_top;
            SonType *lt = stmt->for_stmt.end && stmt->for_stmt.end->inferred_type
                ? stmt->for_stmt.end->inferred_type
                : (stmt->for_stmt.start ? stmt->for_stmt.start->inferred_type : NULL);
            if (stmt->for_stmt.end && stmt->for_stmt.end->kind == EXPR_INT_LITERAL && stmt->for_stmt.start && stmt->for_stmt.start->inferred_type) {
                lt = stmt->for_stmt.start->inferred_type;
            }
            fprintf(gen->out, "{ ");
            emit_type(gen, lt);
            fprintf(gen->out, " _son_for_end = ");
            emit_expr(gen, stmt->for_stmt.end);
            fprintf(gen->out, "; for (");
            emit_type(gen, lt);
            fprintf(gen->out, " %.*s = ", (int)stmt->for_stmt.var_name.length, stmt->for_stmt.var_name.data);
            emit_expr(gen, stmt->for_stmt.start);
            fprintf(gen->out, "; %.*s < _son_for_end; %.*s++) ",
                    (int)stmt->for_stmt.var_name.length, stmt->for_stmt.var_name.data,
                    (int)stmt->for_stmt.var_name.length, stmt->for_stmt.var_name.data);
            emit_block(gen, stmt->for_stmt.body);
            print_indent(gen);
            fprintf(gen->out, "}\n");
            gen->loop_defer_base = prev_loop_base;
            break;
        }
        case STMT_BREAK:
            if (gen->defer_top > gen->loop_defer_base) {
                fprintf(gen->out, "{\n");
                gen->indent_level++;
                emit_defers_down_to(gen, gen->loop_defer_base);
                print_indent(gen);
                fprintf(gen->out, "break;\n");
                gen->indent_level--;
                print_indent(gen);
                fprintf(gen->out, "}\n");
            } else {
                fprintf(gen->out, "break;\n");
            }
            break;
        case STMT_CONTINUE:
            if (gen->defer_top > gen->loop_defer_base) {
                fprintf(gen->out, "{\n");
                gen->indent_level++;
                emit_defers_down_to(gen, gen->loop_defer_base);
                print_indent(gen);
                fprintf(gen->out, "continue;\n");
                gen->indent_level--;
                print_indent(gen);
                fprintf(gen->out, "}\n");
            } else {
                fprintf(gen->out, "continue;\n");
            }
            break;
        case STMT_MATCH:
            fprintf(gen->out, "switch ((");
            emit_expr(gen, stmt->match_stmt.expr);
            fprintf(gen->out, ").tag) {\n");
            gen->indent_level++;
            SonType *tgt = stmt->match_stmt.expr->inferred_type;
            for (size_t i = 0; i < stmt->match_stmt.arm_count; i++) {
                SonMatchArm *arm = &stmt->match_stmt.arms[i];
                print_indent(gen);
                if (arm->is_default) {
                    fprintf(gen->out, "default: ");
                } else if (tgt) {
                    fprintf(gen->out, "case TAG_%.*s_%.*s: ", (int)tgt->name.length, tgt->name.data, (int)arm->variant_name.length, arm->variant_name.data);
                } else {
                    fprintf(gen->out, "case TAG_%.*s: ", (int)arm->variant_name.length, arm->variant_name.data);
                }
                fprintf(gen->out, "{\n");
                gen->indent_level++;
                if (arm->has_payload) {
                    print_indent(gen);
                    fprintf(gen->out, "const SON_AUTO %.*s = (", (int)arm->payload_var.length, arm->payload_var.data);
                    emit_expr(gen, stmt->match_stmt.expr);
                    fprintf(gen->out, ").data.%.*s;\n", (int)arm->variant_name.length, arm->variant_name.data);
                }
                for (size_t s = 0; s < arm->body.stmt_count; s++) {
                    emit_stmt(gen, arm->body.stmts[s]);
                }
                print_indent(gen);
                fprintf(gen->out, "break;\n");
                gen->indent_level--;
                print_indent(gen);
                fprintf(gen->out, "}\n");
            }
            gen->indent_level--;
            print_indent(gen);
            fprintf(gen->out, "}\n");
            break;
        case STMT_BLOCK:
            emit_block(gen, stmt->block);
            break;
        default:
            break;
    }
}

bool son_codegen_emit_module(SonCodeGen *gen, SonModule *module) {
    // Standard Stage 0 C11 Runtime Prelude
    fprintf(gen->out, "// Auto-generated by SonLang Stage 0 Bootstrap Compiler\n");
    fprintf(gen->out, "#define _CRT_SECURE_NO_WARNINGS\n");
    fprintf(gen->out, "#include <stdio.h>\n");
    fprintf(gen->out, "#include <stdint.h>\n");
    fprintf(gen->out, "#include <stdbool.h>\n");
    fprintf(gen->out, "#include <stddef.h>\n");
    fprintf(gen->out, "#include <stdlib.h>\n");
    fprintf(gen->out, "#include <string.h>\n");
    fprintf(gen->out, "#include <math.h>\n\n");

    fprintf(gen->out, "#if defined(__GNUC__) || defined(__clang__)\n");
    fprintf(gen->out, "#define SON_AUTO __auto_type\n");
    fprintf(gen->out, "#else\n");
    fprintf(gen->out, "#define SON_AUTO auto\n");
    fprintf(gen->out, "#endif\n\n");

    // Standard Fat Pointer Slice Typedefs
    fprintf(gen->out, "// Native Fat Pointer Slices with strict No-Alias Restrict Pointer\n");
    fprintf(gen->out, "typedef struct { uint8_t* __restrict data; size_t len; } SonSlice_u8;\n");
    fprintf(gen->out, "typedef struct { int32_t* __restrict data; size_t len; } SonSlice_i32;\n");
    fprintf(gen->out, "typedef struct { int64_t* __restrict data; size_t len; } SonSlice_i64;\n\n");

    // Native OS & Freestanding Runtime Interop
    fprintf(gen->out, "static inline bool son_fs_write_file(SonSlice_u8 path, SonSlice_u8 content) {\n");
    fprintf(gen->out, "    char p[512];\n");
    fprintf(gen->out, "    if (path.len >= sizeof(p)) return false;\n");
    fprintf(gen->out, "    memcpy(p, path.data, path.len);\n");
    fprintf(gen->out, "    p[path.len] = '\\0';\n");
    fprintf(gen->out, "    FILE *f = fopen(p, \"wb\");\n");
    fprintf(gen->out, "    if (!f) return false;\n");
    fprintf(gen->out, "    size_t written = fwrite(content.data, 1, content.len, f);\n");
    fprintf(gen->out, "    fclose(f);\n");
    fprintf(gen->out, "    return written == content.len;\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline bool son_fs_write_file_len(SonSlice_u8 path, SonSlice_u8 content, size_t len) {\n");
    fprintf(gen->out, "    char p[512];\n");
    fprintf(gen->out, "    if (path.len >= sizeof(p)) return false;\n");
    fprintf(gen->out, "    memcpy(p, path.data, path.len);\n");
    fprintf(gen->out, "    p[path.len] = '\\0';\n");
    fprintf(gen->out, "    FILE *f = fopen(p, \"wb\");\n");
    fprintf(gen->out, "    if (!f) return false;\n");
    fprintf(gen->out, "    size_t to_write = len < content.len ? len : content.len;\n");
    fprintf(gen->out, "    size_t written = fwrite(content.data, 1, to_write, f);\n");
    fprintf(gen->out, "    fclose(f);\n");
    fprintf(gen->out, "    return written == to_write;\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline int64_t son_fs_file_size(SonSlice_u8 path) {\n");
    fprintf(gen->out, "    char p[512];\n");
    fprintf(gen->out, "    if (path.len >= sizeof(p)) return -1;\n");
    fprintf(gen->out, "    memcpy(p, path.data, path.len);\n");
    fprintf(gen->out, "    p[path.len] = '\\0';\n");
    fprintf(gen->out, "    FILE *f = fopen(p, \"rb\");\n");
    fprintf(gen->out, "    if (!f) return -1;\n");
    fprintf(gen->out, "    fseek(f, 0, SEEK_END);\n");
    fprintf(gen->out, "    long sz = ftell(f);\n");
    fprintf(gen->out, "    fclose(f);\n");
    fprintf(gen->out, "    return (int64_t)sz;\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline int64_t son_fs_read_file(SonSlice_u8 path, SonSlice_u8 out_buf) {\n");
    fprintf(gen->out, "    char p[512];\n");
    fprintf(gen->out, "    if (path.len >= sizeof(p)) return -1;\n");
    fprintf(gen->out, "    memcpy(p, path.data, path.len);\n");
    fprintf(gen->out, "    p[path.len] = '\\0';\n");
    fprintf(gen->out, "    FILE *f = fopen(p, \"rb\");\n");
    fprintf(gen->out, "    if (!f) return -1;\n");
    fprintf(gen->out, "    size_t rd = fread(out_buf.data, 1, out_buf.len, f);\n");
    fprintf(gen->out, "    fclose(f);\n");
    fprintf(gen->out, "    return (int64_t)rd;\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline int32_t son_os_system(SonSlice_u8 cmd) {\n");
    fprintf(gen->out, "    char c[1024];\n");
    fprintf(gen->out, "    if (cmd.len >= sizeof(c)) return -1;\n");
    fprintf(gen->out, "    memcpy(c, cmd.data, cmd.len);\n");
    fprintf(gen->out, "    c[cmd.len] = '\\0';\n");
    fprintf(gen->out, "    return (int32_t)system(c);\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "#if defined(_WIN32)\n");
    fprintf(gen->out, "#include <winsock2.h>\n");
    fprintf(gen->out, "#include <ws2tcpip.h>\n");
    fprintf(gen->out, "#include <windows.h>\n");
    fprintf(gen->out, "#pragma comment(lib, \"ws2_32.lib\")\n");
    fprintf(gen->out, "#else\n");
    fprintf(gen->out, "#include <sys/socket.h>\n");
    fprintf(gen->out, "#include <netinet/in.h>\n");
    fprintf(gen->out, "#include <arpa/inet.h>\n");
    fprintf(gen->out, "#include <unistd.h>\n");
    fprintf(gen->out, "#include <fcntl.h>\n");
    fprintf(gen->out, "#define SOCKET int\n");
    fprintf(gen->out, "#define INVALID_SOCKET -1\n");
    fprintf(gen->out, "#define SOCKET_ERROR -1\n");
    fprintf(gen->out, "#define closesocket close\n");
    fprintf(gen->out, "#endif\n\n");
    fprintf(gen->out, "static inline int32_t son_net_init(void) {\n");
    fprintf(gen->out, "#if defined(_WIN32)\n");
    fprintf(gen->out, "    WSADATA wsa; return WSAStartup(MAKEWORD(2, 2), &wsa) == 0 ? 0 : -1;\n");
    fprintf(gen->out, "#else\n");
    fprintf(gen->out, "    return 0;\n");
    fprintf(gen->out, "#endif\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline int64_t son_net_listen(uint16_t port) {\n");
    fprintf(gen->out, "    son_net_init();\n");
    fprintf(gen->out, "    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);\n");
    fprintf(gen->out, "    if (s == INVALID_SOCKET) return -1;\n");
    fprintf(gen->out, "    int opt = 1; setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));\n");
    fprintf(gen->out, "    struct sockaddr_in addr; memset(&addr, 0, sizeof(addr));\n");
    fprintf(gen->out, "    addr.sin_family = AF_INET; addr.sin_addr.s_addr = INADDR_ANY; addr.sin_port = htons(port);\n");
    fprintf(gen->out, "    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) { closesocket(s); return -2; }\n");
    fprintf(gen->out, "    if (listen(s, 128) == SOCKET_ERROR) { closesocket(s); return -3; }\n");
    fprintf(gen->out, "    return (int64_t)s;\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline int64_t son_net_accept(int64_t listener_handle) {\n");
    fprintf(gen->out, "    SOCKET s = accept((SOCKET)listener_handle, NULL, NULL);\n");
    fprintf(gen->out, "    if (s == INVALID_SOCKET) return -1;\n");
    fprintf(gen->out, "    return (int64_t)s;\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline int32_t son_net_send(int64_t sock, SonSlice_u8 data) {\n");
    fprintf(gen->out, "    return send((SOCKET)sock, (const char*)data.data, (int)data.len, 0);\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline int32_t son_net_recv(int64_t sock, SonSlice_u8 buf) {\n");
    fprintf(gen->out, "    return recv((SOCKET)sock, (char*)buf.data, (int)buf.len, 0);\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline void son_net_close(int64_t sock) {\n");
    fprintf(gen->out, "    closesocket((SOCKET)sock);\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline bool son_net_set_nonblocking(int64_t sock, bool nonblocking) {\n");
    fprintf(gen->out, "#if defined(_WIN32)\n");
    fprintf(gen->out, "    u_long mode = nonblocking ? 1 : 0;\n");
    fprintf(gen->out, "    return ioctlsocket((SOCKET)sock, FIONBIO, &mode) == 0;\n");
    fprintf(gen->out, "#else\n");
    fprintf(gen->out, "    int flags = fcntl((int)sock, F_GETFL, 0);\n");
    fprintf(gen->out, "    if (flags == -1) return false;\n");
    fprintf(gen->out, "    flags = nonblocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);\n");
    fprintf(gen->out, "    return fcntl((int)sock, F_SETFL, flags) == 0;\n");
    fprintf(gen->out, "#endif\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline bool son_net_set_nodelay(int64_t sock, bool nodelay) {\n");
    fprintf(gen->out, "    int opt = nodelay ? 1 : 0;\n");
    fprintf(gen->out, "    return setsockopt((SOCKET)sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(opt)) == 0;\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline int32_t son_net_poll(int64_t sock, int32_t events, int32_t timeout_ms) {\n");
    fprintf(gen->out, "#if defined(_WIN32)\n");
    fprintf(gen->out, "    short win_events = 0;\n");
    fprintf(gen->out, "    if (events & 1) win_events |= (POLLRDNORM | POLLRDBAND);\n");
    fprintf(gen->out, "    if (events & 2) win_events |= POLLWRNORM;\n");
    fprintf(gen->out, "    if (events & 4) win_events |= POLLERR;\n");
    fprintf(gen->out, "    WSAPOLLFD pfd;\n");
    fprintf(gen->out, "    pfd.fd = (SOCKET)sock; pfd.events = win_events; pfd.revents = 0;\n");
    fprintf(gen->out, "    int ret = WSAPoll(&pfd, 1, timeout_ms);\n");
    fprintf(gen->out, "    if (ret <= 0) return ret;\n");
    fprintf(gen->out, "    int32_t rev = 0;\n");
    fprintf(gen->out, "    if (pfd.revents & (POLLRDNORM | POLLRDBAND)) rev |= 1;\n");
    fprintf(gen->out, "    if (pfd.revents & POLLWRNORM) rev |= 2;\n");
    fprintf(gen->out, "    if (pfd.revents & (POLLERR | POLLHUP)) rev |= 4;\n");
    fprintf(gen->out, "    return rev;\n");
    fprintf(gen->out, "#else\n");
    fprintf(gen->out, "    struct pollfd pfd;\n");
    fprintf(gen->out, "    pfd.fd = (int)sock; pfd.events = (short)events; pfd.revents = 0;\n");
    fprintf(gen->out, "    int ret = poll(&pfd, 1, timeout_ms);\n");
    fprintf(gen->out, "    if (ret <= 0) return ret;\n");
    fprintf(gen->out, "    return (int32_t)pfd.revents;\n");
    fprintf(gen->out, "#endif\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "// Threading and Synchronization PAL\n");
    fprintf(gen->out, "typedef void* (*SonThreadFn)(void*);\n");
    fprintf(gen->out, "static inline int64_t son_thread_spawn(void* fn_ptr, void* arg) {\n");
    fprintf(gen->out, "#if defined(_WIN32)\n");
    fprintf(gen->out, "    HANDLE h = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)fn_ptr, arg, 0, NULL);\n");
    fprintf(gen->out, "    return (int64_t)(intptr_t)h;\n");
    fprintf(gen->out, "#else\n");
    fprintf(gen->out, "    pthread_t th;\n");
    fprintf(gen->out, "    if (pthread_create(&th, NULL, (SonThreadFn)fn_ptr, arg) != 0) return -1;\n");
    fprintf(gen->out, "    return (int64_t)th;\n");
    fprintf(gen->out, "#endif\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline bool son_thread_join(int64_t handle) {\n");
    fprintf(gen->out, "#if defined(_WIN32)\n");
    fprintf(gen->out, "    if (!handle) return false;\n");
    fprintf(gen->out, "    WaitForSingleObject((HANDLE)(intptr_t)handle, INFINITE);\n");
    fprintf(gen->out, "    CloseHandle((HANDLE)(intptr_t)handle);\n");
    fprintf(gen->out, "    return true;\n");
    fprintf(gen->out, "#else\n");
    fprintf(gen->out, "    return pthread_join((pthread_t)handle, NULL) == 0;\n");
    fprintf(gen->out, "#endif\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline int64_t son_mutex_init(void) {\n");
    fprintf(gen->out, "#if defined(_WIN32)\n");
    fprintf(gen->out, "    CRITICAL_SECTION* cs = (CRITICAL_SECTION*)malloc(sizeof(CRITICAL_SECTION));\n");
    fprintf(gen->out, "    InitializeCriticalSection(cs);\n");
    fprintf(gen->out, "    return (int64_t)(intptr_t)cs;\n");
    fprintf(gen->out, "#else\n");
    fprintf(gen->out, "    pthread_mutex_t* m = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));\n");
    fprintf(gen->out, "    pthread_mutex_init(m, NULL);\n");
    fprintf(gen->out, "    return (int64_t)(intptr_t)m;\n");
    fprintf(gen->out, "#endif\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline void son_mutex_lock(int64_t handle) {\n");
    fprintf(gen->out, "#if defined(_WIN32)\n");
    fprintf(gen->out, "    EnterCriticalSection((CRITICAL_SECTION*)(intptr_t)handle);\n");
    fprintf(gen->out, "#else\n");
    fprintf(gen->out, "    pthread_mutex_lock((pthread_mutex_t*)(intptr_t)handle);\n");
    fprintf(gen->out, "#endif\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline void son_mutex_unlock(int64_t handle) {\n");
    fprintf(gen->out, "#if defined(_WIN32)\n");
    fprintf(gen->out, "    LeaveCriticalSection((CRITICAL_SECTION*)(intptr_t)handle);\n");
    fprintf(gen->out, "#else\n");
    fprintf(gen->out, "    pthread_mutex_unlock((pthread_mutex_t*)(intptr_t)handle);\n");
    fprintf(gen->out, "#endif\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline void son_mutex_destroy(int64_t handle) {\n");
    fprintf(gen->out, "#if defined(_WIN32)\n");
    fprintf(gen->out, "    DeleteCriticalSection((CRITICAL_SECTION*)(intptr_t)handle);\n");
    fprintf(gen->out, "    free((void*)(intptr_t)handle);\n");
    fprintf(gen->out, "#else\n");
    fprintf(gen->out, "    pthread_mutex_destroy((pthread_mutex_t*)(intptr_t)handle);\n");
    fprintf(gen->out, "    free((void*)(intptr_t)handle);\n");
    fprintf(gen->out, "#endif\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "// Random-Access File I/O\n");
    fprintf(gen->out, "static inline int64_t son_file_open(SonSlice_u8 path, SonSlice_u8 mode) {\n");
    fprintf(gen->out, "    char p[512], m[16];\n");
    fprintf(gen->out, "    if (path.len >= sizeof(p) || mode.len >= sizeof(m)) return -1;\n");
    fprintf(gen->out, "    memcpy(p, path.data, path.len); p[path.len] = '\\0';\n");
    fprintf(gen->out, "    memcpy(m, mode.data, mode.len); m[mode.len] = '\\0';\n");
    fprintf(gen->out, "    FILE* f = fopen(p, m);\n");
    fprintf(gen->out, "    return (int64_t)(intptr_t)f;\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline bool son_file_seek(int64_t handle, int64_t offset, int32_t whence) {\n");
    fprintf(gen->out, "    if (!handle) return false;\n");
    fprintf(gen->out, "#if defined(_WIN32)\n");
    fprintf(gen->out, "    return _fseeki64((FILE*)(intptr_t)handle, offset, whence) == 0;\n");
    fprintf(gen->out, "#else\n");
    fprintf(gen->out, "    return fseeko((FILE*)(intptr_t)handle, (off_t)offset, whence) == 0;\n");
    fprintf(gen->out, "#endif\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline int64_t son_file_tell(int64_t handle) {\n");
    fprintf(gen->out, "    if (!handle) return -1;\n");
    fprintf(gen->out, "#if defined(_WIN32)\n");
    fprintf(gen->out, "    return _ftelli64((FILE*)(intptr_t)handle);\n");
    fprintf(gen->out, "#else\n");
    fprintf(gen->out, "    return (int64_t)ftello((FILE*)(intptr_t)handle);\n");
    fprintf(gen->out, "#endif\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline int64_t son_file_read_exact(int64_t handle, SonSlice_u8 buf, size_t len) {\n");
    fprintf(gen->out, "    if (!handle) return -1;\n");
    fprintf(gen->out, "    size_t to_read = len < buf.len ? len : buf.len;\n");
    fprintf(gen->out, "    return (int64_t)fread(buf.data, 1, to_read, (FILE*)(intptr_t)handle);\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline int64_t son_file_write_exact(int64_t handle, SonSlice_u8 buf, size_t len) {\n");
    fprintf(gen->out, "    if (!handle) return -1;\n");
    fprintf(gen->out, "    size_t to_write = len < buf.len ? len : buf.len;\n");
    fprintf(gen->out, "    return (int64_t)fwrite(buf.data, 1, to_write, (FILE*)(intptr_t)handle);\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline void son_file_close_handle(int64_t handle) {\n");
    fprintf(gen->out, "    if (handle) fclose((FILE*)(intptr_t)handle);\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "// Console I/O Runtime Helpers\n");
    fprintf(gen->out, "static inline void son_io_print(SonSlice_u8 s) {\n");
    fprintf(gen->out, "    if (s.data && s.len > 0) fwrite(s.data, 1, s.len, stdout);\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline void son_io_println(SonSlice_u8 s) {\n");
    fprintf(gen->out, "    if (s.data && s.len > 0) fwrite(s.data, 1, s.len, stdout);\n");
    fprintf(gen->out, "    fputc('\\n', stdout);\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline void son_io_eprintln(SonSlice_u8 s) {\n");
    fprintf(gen->out, "    if (s.data && s.len > 0) fwrite(s.data, 1, s.len, stderr);\n");
    fprintf(gen->out, "    fputc('\\n', stderr);\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline void son_io_print_i32(int32_t v) {\n");
    fprintf(gen->out, "    printf(\"%%d\", v);\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline void son_io_print_i64(int64_t v) {\n");
    fprintf(gen->out, "    printf(\"%%lld\", (long long)v);\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline void son_io_print_f64(double v) {\n");
    fprintf(gen->out, "    printf(\"%%g\", v);\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "#if defined(_WIN32)\n");
    fprintf(gen->out, "static inline int64_t son_time_now_ticks(void) {\n");
    fprintf(gen->out, "    LARGE_INTEGER val; QueryPerformanceCounter(&val);\n");
    fprintf(gen->out, "    return (int64_t)val.QuadPart;\n");
    fprintf(gen->out, "}\n");
    fprintf(gen->out, "static inline int64_t son_time_frequency(void) {\n");
    fprintf(gen->out, "    LARGE_INTEGER val; QueryPerformanceFrequency(&val);\n");
    fprintf(gen->out, "    return (int64_t)val.QuadPart;\n");
    fprintf(gen->out, "}\n");
    fprintf(gen->out, "static inline void son_sleep_ms(uint32_t ms) { Sleep(ms); }\n");
    fprintf(gen->out, "#else\n");
    fprintf(gen->out, "#include <time.h>\n#include <unistd.h>\n");
    fprintf(gen->out, "static inline int64_t son_time_now_ticks(void) {\n");
    fprintf(gen->out, "    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);\n");
    fprintf(gen->out, "    return (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;\n");
    fprintf(gen->out, "}\n");
    fprintf(gen->out, "static inline int64_t son_time_frequency(void) { return 1000000000LL; }\n");
    fprintf(gen->out, "static inline void son_sleep_ms(uint32_t ms) { usleep(ms * 1000); }\n");
    fprintf(gen->out, "#endif\n\n");
    fprintf(gen->out, "static inline int64_t son_time_nanos(void) {\n");
    fprintf(gen->out, "    int64_t ticks = son_time_now_ticks();\n");
    fprintf(gen->out, "    int64_t freq = son_time_frequency();\n");
    fprintf(gen->out, "    if (freq <= 0) return 0;\n");
    fprintf(gen->out, "    return (ticks * 1000000000LL) / freq;\n");
    fprintf(gen->out, "}\n");
    fprintf(gen->out, "static inline void son_sleep_us(uint64_t us) {\n");
    fprintf(gen->out, "#if defined(_WIN32)\n");
    fprintf(gen->out, "    if (us >= 1000) Sleep((DWORD)(us / 1000));\n");
    fprintf(gen->out, "    else {\n");
    fprintf(gen->out, "        int64_t target = son_time_nanos() + (int64_t)(us * 1000);\n");
    fprintf(gen->out, "        while (son_time_nanos() < target) { /* spin-wait for high precision */ }\n");
    fprintf(gen->out, "    }\n");
    fprintf(gen->out, "#else\n");
    fprintf(gen->out, "    usleep((useconds_t)us);\n");
    fprintf(gen->out, "#endif\n");
    fprintf(gen->out, "}\n\n");
    fprintf(gen->out, "static inline size_t son_heap_alloc(size_t sz) {\n");
    fprintf(gen->out, "    return (size_t)(uintptr_t)malloc(sz);\n");
    fprintf(gen->out, "}\n");
    fprintf(gen->out, "static inline void son_heap_free(size_t ptr) {\n");
    fprintf(gen->out, "    free((void*)(uintptr_t)ptr);\n");
    fprintf(gen->out, "}\n\n");

    // Forward declare structs and unions
    for (size_t i = 0; i < module->decl_count; i++) {
        SonDecl *decl = module->decls[i];
        if (decl->type_param_count > 0) continue;
        if (decl->kind == DECL_STRUCT || decl->kind == DECL_UNION) {
            fprintf(gen->out, "typedef struct %.*s %.*s;\n",
                    (int)decl->name.length, decl->name.data,
                    (int)decl->name.length, decl->name.data);
        }
    }
    fprintf(gen->out, "\n");

    // Define Unions
    for (size_t i = 0; i < module->decl_count; i++) {
        SonDecl *decl = module->decls[i];
        if (decl->type_param_count > 0) continue;
        if (decl->kind == DECL_UNION) {
            fprintf(gen->out, "// Union: %.*s\n", (int)decl->name.length, decl->name.data);
            fprintf(gen->out, "typedef enum {\n");
            for (size_t v = 0; v < decl->union_decl.variant_count; v++) {
                fprintf(gen->out, "    TAG_%.*s_%.*s = %d,\n",
                        (int)decl->name.length, decl->name.data,
                        (int)decl->union_decl.variants[v].name.length,
                        decl->union_decl.variants[v].name.data,
                        decl->union_decl.variants[v].tag_value);
            }
            fprintf(gen->out, "} Tag_%.*s;\n\n", (int)decl->name.length, decl->name.data);

            fprintf(gen->out, "struct %.*s {\n", (int)decl->name.length, decl->name.data);
            fprintf(gen->out, "    Tag_%.*s tag;\n", (int)decl->name.length, decl->name.data);
            fprintf(gen->out, "    union {\n");
            for (size_t v = 0; v < decl->union_decl.variant_count; v++) {
                if (decl->union_decl.variants[v].payload_type) {
                    fprintf(gen->out, "        ");
                    emit_type(gen, decl->union_decl.variants[v].payload_type);
                    fprintf(gen->out, " %.*s;\n",
                            (int)decl->union_decl.variants[v].name.length,
                            decl->union_decl.variants[v].name.data);
                }
            }
            fprintf(gen->out, "    } data;\n");
            fprintf(gen->out, "};\n\n");

            // Variant constructor helpers
            for (size_t v = 0; v < decl->union_decl.variant_count; v++) {
                SonVariantDecl *var = &decl->union_decl.variants[v];
                fprintf(gen->out, "static inline %.*s %.*s_%.*s(",
                        (int)decl->name.length, decl->name.data,
                        (int)decl->name.length, decl->name.data,
                        (int)var->name.length, var->name.data);
                if (var->payload_type) {
                    emit_type(gen, var->payload_type);
                    fprintf(gen->out, " payload");
                }
                fprintf(gen->out, ") {\n");
                fprintf(gen->out, "    %.*s res = {0};\n", (int)decl->name.length, decl->name.data);
                fprintf(gen->out, "    res.tag = TAG_%.*s_%.*s;\n",
                        (int)decl->name.length, decl->name.data,
                        (int)var->name.length, var->name.data);
                if (var->payload_type) {
                    fprintf(gen->out, "    res.data.%.*s = payload;\n", (int)var->name.length, var->name.data);
                }
                fprintf(gen->out, "    return res;\n");
                fprintf(gen->out, "}\n\n");
            }
        }
    }

    // Define structs
    for (size_t i = 0; i < module->decl_count; i++) {
        SonDecl *decl = module->decls[i];
        if (decl->type_param_count > 0) continue;
        if (decl->kind == DECL_STRUCT) {
            fprintf(gen->out, "struct %.*s {\n", (int)decl->name.length, decl->name.data);
            for (size_t f = 0; f < decl->struct_decl.field_count; f++) {
                fprintf(gen->out, "    ");
                emit_type(gen, decl->struct_decl.fields[f].type);
                fprintf(gen->out, " %.*s;\n",
                        (int)decl->struct_decl.fields[f].name.length,
                        decl->struct_decl.fields[f].name.data);
            }
            fprintf(gen->out, "};\n\n");
        }
    }

    // Forward declare functions
    for (size_t i = 0; i < module->decl_count; i++) {
        SonDecl *decl = module->decls[i];
        if (decl->kind == DECL_FUNCTION) {
            bool is_main = (decl->name.length == 4 && memcmp(decl->name.data, "main", 4) == 0);
            if (decl->is_extern) {
                if (son_slice_eq_cstr(decl->name, "malloc") ||
                    son_slice_eq_cstr(decl->name, "free") ||
                    son_slice_eq_cstr(decl->name, "realloc") ||
                    son_slice_eq_cstr(decl->name, "memset") ||
                    son_slice_eq_cstr(decl->name, "memcpy") ||
                    son_slice_eq_cstr(decl->name, "memmove") ||
                    son_slice_eq_cstr(decl->name, "printf") ||
                    son_slice_eq_cstr(decl->name, "puts") ||
                    son_slice_eq_cstr(decl->name, "exit") ||
                    son_slice_eq_cstr(decl->name, "abs") ||
                    son_slice_eq_cstr(decl->name, "sqrt")) {
                    continue;
                }
                fprintf(gen->out, "extern ");
            } else if (!is_main) {
                fprintf(gen->out, "static inline ");
            }
            emit_type(gen, decl->func.return_type);
            fprintf(gen->out, " %.*s(", (int)decl->name.length, decl->name.data);
            for (size_t p = 0; p < decl->func.param_count; p++) {
                if (p > 0) fprintf(gen->out, ", ");
                emit_type(gen, decl->func.params[p].type);
                fprintf(gen->out, " %.*s",
                        (int)decl->func.params[p].name.length,
                        decl->func.params[p].name.data);
            }
            fprintf(gen->out, ");\n");
        }
    }
    fprintf(gen->out, "\n");

    // Define functions
    for (size_t i = 0; i < module->decl_count; i++) {
        SonDecl *decl = module->decls[i];
        if (decl->kind == DECL_FUNCTION) {
            if (decl->is_extern) continue;
            bool is_main = (decl->name.length == 4 && memcmp(decl->name.data, "main", 4) == 0);
            if (!is_main) {
                fprintf(gen->out, "static inline ");
            }
            emit_type(gen, decl->func.return_type);
            fprintf(gen->out, " %.*s(", (int)decl->name.length, decl->name.data);
            for (size_t p = 0; p < decl->func.param_count; p++) {
                if (p > 0) fprintf(gen->out, ", ");
                emit_type(gen, decl->func.params[p].type);
                fprintf(gen->out, " %.*s",
                        (int)decl->func.params[p].name.length,
                        decl->func.params[p].name.data);
            }
            fprintf(gen->out, ") ");
            gen->current_fn_ret_type = decl->func.return_type;
            gen->fn_defer_base = gen->defer_top;
            gen->loop_defer_base = gen->defer_top;
            emit_block(gen, decl->func.body);
            gen->current_fn_ret_type = NULL;
            fprintf(gen->out, "\n");
        }
    }

    return !gen->has_error;
}
