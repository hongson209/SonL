#ifndef SON_AI_H
#define SON_AI_H

#include "son_common.h"
#include "son_ast.h"
#include "son_sema.h"

// Machine-readable AI Semantic Query APIs
void son_ai_export_ast_json(FILE *out, SonModule *module);
void son_ai_export_symbols_json(FILE *out, SonSema *sema, SonModule *module);
void son_ai_export_diagnostics_json(FILE *out, SonSema *sema, const char *filename);
bool son_ai_apply_patch(const char *source_path, const char *patch_json_path, const char *output_path);

#endif // SON_AI_H
