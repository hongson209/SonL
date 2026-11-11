#ifndef SON_CODEGEN_H
#define SON_CODEGEN_H

#include "son_common.h"
#include "son_ast.h"

typedef struct {
    FILE *out;
    size_t indent_level;
    bool has_error;
} SonCodeGen;

void son_codegen_init(SonCodeGen *gen, FILE *output);
bool son_codegen_emit_module(SonCodeGen *gen, SonModule *module);

#endif // SON_CODEGEN_H
