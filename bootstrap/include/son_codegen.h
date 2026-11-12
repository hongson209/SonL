#ifndef SON_CODEGEN_H
#define SON_CODEGEN_H

#include "son_common.h"
#include "son_ast.h"

#define SON_MAX_DEFER_STACK 64

typedef struct {
    FILE *out;
    size_t indent_level;
    bool has_error;
    SonType *current_fn_ret_type;
    SonStmt *defer_stack[SON_MAX_DEFER_STACK];
    int defer_top;
    int fn_defer_base;
    int loop_defer_base;
} SonCodeGen;

void son_codegen_init(SonCodeGen *gen, FILE *output);
bool son_codegen_emit_module(SonCodeGen *gen, SonModule *module);

#endif // SON_CODEGEN_H
