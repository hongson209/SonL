#include "../include/son_llvm.h"

static const char* llvm_type_str(SonType *t) {
    if (!t) return "void";
    switch (t->kind) {
        case TYPE_VOID: return "void";
        case TYPE_BOOL: return "i1";
        case TYPE_I8:
        case TYPE_U8:
        case TYPE_CHAR: return "i8";
        case TYPE_I16:
        case TYPE_U16: return "i16";
        case TYPE_I32:
        case TYPE_U32: return "i32";
        case TYPE_I64:
        case TYPE_U64:
        case TYPE_ISIZE:
        case TYPE_USIZE: return "i64";
        case TYPE_F32: return "float";
        case TYPE_F64: return "double";
        case TYPE_REFERENCE:
        case TYPE_POINTER: return "ptr";
        case TYPE_STRUCT:
        case TYPE_UNION: return "ptr";
        default: return "i64";
    }
}

static void dump_llvm_val(FILE *out, SonIRValue *val) {
    if (!val) {
        fprintf(out, "0");
        return;
    }
    switch (val->kind) {
        case IR_VAL_CONST_INT:
            fprintf(out, "%lld", (long long)val->int_val);
            break;
        case IR_VAL_CONST_FLOAT:
            fprintf(out, "%f", val->float_val);
            break;
        case IR_VAL_CONST_BOOL:
            fprintf(out, "%s", val->bool_val ? "true" : "false");
            break;
        case IR_VAL_CONST_CHAR:
            fprintf(out, "%d", (int)val->char_val);
            break;
        case IR_VAL_CONST_STRING:
            fprintf(out, "c\"%.*s\\00\"", (int)val->string_val.length, val->string_val.data);
            break;
        case IR_VAL_REG:
            fprintf(out, "%%r%zu", val->reg_id);
            break;
        case IR_VAL_PARAM:
        case IR_VAL_GLOBAL:
            fprintf(out, "%%%.*s", (int)val->name.length, val->name.data);
            break;
    }
}

bool son_llvm_emit(FILE *out, SonIRModule *ir_module) {
    if (!out || !ir_module) return false;

    // Module header with target specifications
    fprintf(out, "; ModuleID = 'SonLang Native Backend'\n");
    fprintf(out, "source_filename = \"SonLang\"\n\n");

    // Declare runtime support functions if needed
    fprintf(out, "declare i32 @printf(ptr, ...)\n\n");

    SonIRFunc *func = ir_module->first_func;
    while (func) {
        const char *ret_t = llvm_type_str(func->return_type);
        if (func->is_extern) {
            fprintf(out, "declare %s @%.*s(", ret_t, (int)func->name.length, func->name.data);
            for (size_t p = 0; p < func->param_count; p++) {
                if (p > 0) fprintf(out, ", ");
                fprintf(out, "%s", llvm_type_str(func->params[p]->type));
            }
            fprintf(out, ")\n\n");
            func = func->next;
            continue;
        }
        fprintf(out, "define %s @%.*s(", ret_t, (int)func->name.length, func->name.data);

        for (size_t p = 0; p < func->param_count; p++) {
            if (p > 0) fprintf(out, ", ");
            fprintf(out, "%s %%%.*s",
                    llvm_type_str(func->params[p]->type),
                    (int)func->params[p]->name.length, func->params[p]->name.data);
        }
        fprintf(out, ") {\n");

        SonIRBlock *block = func->first_block;
        while (block) {
            fprintf(out, "%.*s:\n", (int)block->name.length, block->name.data);

            SonIRInst *inst = block->first_inst;
            while (inst) {
                fprintf(out, "    ");
                switch (inst->op) {
                    case IR_OP_ALLOCA:
                        if (inst->dest && inst->dest->kind == IR_VAL_GLOBAL) {
                            fprintf(out, "%%%.*s = alloca %s\n",
                                    (int)inst->dest->name.length, inst->dest->name.data,
                                    llvm_type_str(inst->dest->type));
                        } else {
                            fprintf(out, "%%r%zu = alloca %s\n",
                                    inst->dest ? inst->dest->reg_id : 0,
                                    llvm_type_str(inst->dest ? inst->dest->type : NULL));
                        }
                        break;
                    case IR_OP_LOAD:
                        fprintf(out, "%%r%zu = load %s, ptr ",
                                inst->dest->reg_id,
                                llvm_type_str(inst->dest->type));
                        dump_llvm_val(out, inst->operands[0]);
                        fprintf(out, "\n");
                        break;
                    case IR_OP_STORE:
                        if (inst->operands[0] && inst->operands[0]->kind == IR_VAL_CONST_STRING) {
                            fprintf(out, "store ptr ");
                        } else {
                            fprintf(out, "store %s ", llvm_type_str(inst->operands[0] ? inst->operands[0]->type : NULL));
                        }
                        dump_llvm_val(out, inst->operands[0]);
                        fprintf(out, ", ptr ");
                        dump_llvm_val(out, inst->operands[1]);
                        fprintf(out, "\n");
                        break;
                    case IR_OP_ADD:
                        fprintf(out, "%%r%zu = add %s ",
                                inst->dest->reg_id,
                                llvm_type_str(inst->dest->type));
                        dump_llvm_val(out, inst->operands[0]);
                        fprintf(out, ", ");
                        dump_llvm_val(out, inst->operands[1]);
                        fprintf(out, "\n");
                        break;
                    case IR_OP_SUB:
                        fprintf(out, "%%r%zu = sub %s ",
                                inst->dest->reg_id,
                                llvm_type_str(inst->dest->type));
                        dump_llvm_val(out, inst->operands[0]);
                        fprintf(out, ", ");
                        dump_llvm_val(out, inst->operands[1]);
                        fprintf(out, "\n");
                        break;
                    case IR_OP_MUL:
                        fprintf(out, "%%r%zu = mul %s ",
                                inst->dest->reg_id,
                                llvm_type_str(inst->dest->type));
                        dump_llvm_val(out, inst->operands[0]);
                        fprintf(out, ", ");
                        dump_llvm_val(out, inst->operands[1]);
                        fprintf(out, "\n");
                        break;
                    case IR_OP_DIV:
                        fprintf(out, "%%r%zu = sdiv %s ",
                                inst->dest->reg_id,
                                llvm_type_str(inst->dest->type));
                        dump_llvm_val(out, inst->operands[0]);
                        fprintf(out, ", ");
                        dump_llvm_val(out, inst->operands[1]);
                        fprintf(out, "\n");
                        break;
                    case IR_OP_REM:
                        fprintf(out, "%%r%zu = srem %s ",
                                inst->dest->reg_id,
                                llvm_type_str(inst->dest->type));
                        dump_llvm_val(out, inst->operands[0]);
                        fprintf(out, ", ");
                        dump_llvm_val(out, inst->operands[1]);
                        fprintf(out, "\n");
                        break;
                    case IR_OP_AND:
                        fprintf(out, "%%r%zu = and %s ",
                                inst->dest->reg_id,
                                llvm_type_str(inst->dest->type));
                        dump_llvm_val(out, inst->operands[0]);
                        fprintf(out, ", ");
                        dump_llvm_val(out, inst->operands[1]);
                        fprintf(out, "\n");
                        break;
                    case IR_OP_OR:
                        fprintf(out, "%%r%zu = or %s ",
                                inst->dest->reg_id,
                                llvm_type_str(inst->dest->type));
                        dump_llvm_val(out, inst->operands[0]);
                        fprintf(out, ", ");
                        dump_llvm_val(out, inst->operands[1]);
                        fprintf(out, "\n");
                        break;
                    case IR_OP_NOT:
                        fprintf(out, "%%r%zu = xor %s ",
                                inst->dest->reg_id,
                                llvm_type_str(inst->dest->type));
                        dump_llvm_val(out, inst->operands[0]);
                        fprintf(out, ", true\n");
                        break;
                    case IR_OP_EQ: {
                        bool is_flt = inst->operands[0]->type && (inst->operands[0]->type->kind == TYPE_F32 || inst->operands[0]->type->kind == TYPE_F64);
                        fprintf(out, "%%r%zu = %s %s ",
                                inst->dest->reg_id,
                                is_flt ? "fcmp oeq" : "icmp eq",
                                llvm_type_str(inst->operands[0]->type));
                        dump_llvm_val(out, inst->operands[0]);
                        fprintf(out, ", ");
                        dump_llvm_val(out, inst->operands[1]);
                        fprintf(out, "\n");
                        break;
                    }
                    case IR_OP_NE: {
                        bool is_flt = inst->operands[0]->type && (inst->operands[0]->type->kind == TYPE_F32 || inst->operands[0]->type->kind == TYPE_F64);
                        fprintf(out, "%%r%zu = %s %s ",
                                inst->dest->reg_id,
                                is_flt ? "fcmp one" : "icmp ne",
                                llvm_type_str(inst->operands[0]->type));
                        dump_llvm_val(out, inst->operands[0]);
                        fprintf(out, ", ");
                        dump_llvm_val(out, inst->operands[1]);
                        fprintf(out, "\n");
                        break;
                    }
                    case IR_OP_LT: {
                        bool is_flt = inst->operands[0]->type && (inst->operands[0]->type->kind == TYPE_F32 || inst->operands[0]->type->kind == TYPE_F64);
                        fprintf(out, "%%r%zu = %s %s ",
                                inst->dest->reg_id,
                                is_flt ? "fcmp olt" : "icmp slt",
                                llvm_type_str(inst->operands[0]->type));
                        dump_llvm_val(out, inst->operands[0]);
                        fprintf(out, ", ");
                        dump_llvm_val(out, inst->operands[1]);
                        fprintf(out, "\n");
                        break;
                    }
                    case IR_OP_LE: {
                        bool is_flt = inst->operands[0]->type && (inst->operands[0]->type->kind == TYPE_F32 || inst->operands[0]->type->kind == TYPE_F64);
                        fprintf(out, "%%r%zu = %s %s ",
                                inst->dest->reg_id,
                                is_flt ? "fcmp ole" : "icmp sle",
                                llvm_type_str(inst->operands[0]->type));
                        dump_llvm_val(out, inst->operands[0]);
                        fprintf(out, ", ");
                        dump_llvm_val(out, inst->operands[1]);
                        fprintf(out, "\n");
                        break;
                    }
                    case IR_OP_GT: {
                        bool is_flt = inst->operands[0]->type && (inst->operands[0]->type->kind == TYPE_F32 || inst->operands[0]->type->kind == TYPE_F64);
                        fprintf(out, "%%r%zu = %s %s ",
                                inst->dest->reg_id,
                                is_flt ? "fcmp ogt" : "icmp sgt",
                                llvm_type_str(inst->operands[0]->type));
                        dump_llvm_val(out, inst->operands[0]);
                        fprintf(out, ", ");
                        dump_llvm_val(out, inst->operands[1]);
                        fprintf(out, "\n");
                        break;
                    }
                    case IR_OP_GE: {
                        bool is_flt = inst->operands[0]->type && (inst->operands[0]->type->kind == TYPE_F32 || inst->operands[0]->type->kind == TYPE_F64);
                        fprintf(out, "%%r%zu = %s %s ",
                                inst->dest->reg_id,
                                is_flt ? "fcmp oge" : "icmp sge",
                                llvm_type_str(inst->operands[0]->type));
                        dump_llvm_val(out, inst->operands[0]);
                        fprintf(out, ", ");
                        dump_llvm_val(out, inst->operands[1]);
                        fprintf(out, "\n");
                        break;
                    }
                    case IR_OP_CALL:
                        if (inst->dest) {
                            fprintf(out, "%%r%zu = ", inst->dest->reg_id);
                        }
                        fprintf(out, "call %s @%.*s(",
                                inst->dest ? llvm_type_str(inst->dest->type) : "void",
                                (int)inst->extra_name.length, inst->extra_name.data);
                        for (size_t a = 0; a < inst->operand_count; a++) {
                            if (a > 0) fprintf(out, ", ");
                            fprintf(out, "%s ", (inst->operands[a] && inst->operands[a]->type) ? llvm_type_str(inst->operands[a]->type) : "ptr");
                            dump_llvm_val(out, inst->operands[a]);
                        }
                        fprintf(out, ")\n");
                        break;
                    case IR_OP_JUMP:
                        fprintf(out, "br label %%%.*s\n",
                                inst->target_true ? (int)inst->target_true->name.length : 2, inst->target_true ? inst->target_true->name.data : "bb");
                        break;
                    case IR_OP_BRANCH:
                        fprintf(out, "br i1 ");
                        dump_llvm_val(out, inst->operands[0]);
                        fprintf(out, ", label %%%.*s, label %%%.*s\n",
                                inst->target_true ? (int)inst->target_true->name.length : 2, inst->target_true ? inst->target_true->name.data : "bb",
                                inst->target_false ? (int)inst->target_false->name.length : 2, inst->target_false ? inst->target_false->name.data : "bb");
                        break;
                    case IR_OP_RETURN:
                        if (inst->operand_count > 0 && inst->operands[0]) {
                            fprintf(out, "ret %s ", llvm_type_str(inst->operands[0]->type));
                            dump_llvm_val(out, inst->operands[0]);
                            fprintf(out, "\n");
                        } else {
                            fprintf(out, "ret void\n");
                        }
                        break;
                    case IR_OP_GET_FIELD:
                        if (inst->dest) {
                            fprintf(out, "%%r%zu = load %s, ptr ",
                                    inst->dest->reg_id,
                                    llvm_type_str(inst->dest->type));
                            dump_llvm_val(out, inst->operands[0]);
                            fprintf(out, "\n");
                        }
                        break;
                    case IR_OP_INDEX:
                        if (inst->dest) {
                            fprintf(out, "%%r%zu_ptr = getelementptr inbounds %s, ptr ",
                                    inst->dest->reg_id,
                                    llvm_type_str(inst->dest->type));
                            dump_llvm_val(out, inst->operands[0]);
                            fprintf(out, ", i64 ");
                            dump_llvm_val(out, inst->operands[1]);
                            fprintf(out, "\n    %%r%zu = load %s, ptr %%r%zu_ptr\n",
                                    inst->dest->reg_id,
                                    llvm_type_str(inst->dest->type),
                                    inst->dest->reg_id);
                        }
                        break;
                    case IR_OP_CAST:
                        if (inst->dest) {
                            fprintf(out, "%%r%zu = ", inst->dest->reg_id);
                            if (inst->operands[0]->type && inst->dest->type && inst->dest->type->kind == TYPE_I64) {
                                fprintf(out, "sext i32 ");
                            } else {
                                fprintf(out, "trunc i64 ");
                            }
                            dump_llvm_val(out, inst->operands[0]);
                            fprintf(out, " to %s\n", llvm_type_str(inst->dest->type));
                        }
                        break;
                    case IR_OP_MATCH_BRANCH:
                        if (inst->target_true) {
                            fprintf(out, "br label %%%.*s\n",
                                    (int)inst->target_true->name.length, inst->target_true->name.data);
                        } else {
                            fprintf(out, "; empty match arm\n");
                        }
                        break;
                    default:
                        fprintf(out, "; unhandled SonIR op\n");
                        break;
                }
                bool is_term = (inst->op == IR_OP_RETURN || inst->op == IR_OP_JUMP || inst->op == IR_OP_BRANCH);
                inst = inst->next;
                if (is_term) break;
            }

            if (!block->first_inst) {
                if (block->next) {
                    fprintf(out, "    br label %%%.*s\n", (int)block->next->name.length, block->next->name.data);
                } else {
                    fprintf(out, "    ret i32 0\n");
                }
            }

            block = block->next;
        }

        fprintf(out, "}\n\n");
        func = func->next;
    }

    return true;
}
