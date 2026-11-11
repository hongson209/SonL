#include "../include/son_common.h"
#include "../include/son_lexer.h"
#include "../include/son_parser.h"
#include "../include/son_sema.h"
#include "../include/son_ir.h"
#include "../include/son_llvm.h"
#include "../include/son_ai.h"
#include "../include/son_codegen.h"

static char* read_file_to_string(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "error: failed to open file '%s'\n", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size < 0) {
        fprintf(stderr, "error: failed to determine size of file '%s'\n", path);
        fclose(f);
        return NULL;
    }

    char *buf = (char*)malloc(size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);

    if (out_len) *out_len = (size_t)size;
    return buf;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("SonLang Stage 0 Bootstrap Compiler (son0)\n");
        printf("Usage: son0 <input.sonl> [-o <output.c>]\n");
        return 1;
    }

    const char *input_path = argv[1];
    const char *output_path = NULL;
    bool emit_ir = false;
    bool emit_llvm = false;
    bool query_ast = false;
    bool query_symbols = false;
    bool query_diagnostics = false;

    // Validate file extension
    size_t path_len = strlen(input_path);
    if (path_len >= 5 && strcmp(input_path + path_len - 5, ".sonl") == 0) {
        // Official extension (.sonl)
    } else if (path_len >= 4 && strcmp(input_path + path_len - 4, ".son") == 0) {
        fprintf(stderr, "warning: '.son' extension is deprecated; use official '.sonl' extension\n");
    } else {
        fprintf(stderr, "warning: input file '%s' does not have official '.sonl' extension\n", input_path);
    }

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_path = argv[++i];
        } else if (strcmp(argv[i], "--emit-ir") == 0) {
            emit_ir = true;
        } else if (strcmp(argv[i], "--emit-llvm") == 0) {
            emit_llvm = true;
        } else if (strcmp(argv[i], "--query-ast") == 0) {
            query_ast = true;
        } else if (strcmp(argv[i], "--query-symbols") == 0) {
            query_symbols = true;
        } else if (strcmp(argv[i], "--query-diagnostics") == 0) {
            query_diagnostics = true;
        }
    }

    size_t source_len = 0;
    char *source = read_file_to_string(input_path, &source_len);
    if (!source) return 1;

    SonArena arena;
    son_arena_init(&arena, 256 * 1024);

    SonParser parser;
    son_parser_init(&parser, &arena, input_path, source, source_len);

    SonModule *module = son_parser_parse_module(&parser);
    if (parser.has_error || !module) {
        if (query_diagnostics) {
            SonSema dummy;
            dummy.has_error = true;
            dummy.error_count = 1;
            son_ai_export_diagnostics_json(stdout, &dummy, input_path);
            free(source);
            son_arena_free_all(&arena);
            return 1;
        }
        fprintf(stderr, "Compilation aborted due to syntax errors.\n");
        free(source);
        son_arena_free_all(&arena);
        return 1;
    }

    if (query_ast) {
        FILE *ast_out = stdout;
        if (output_path) ast_out = fopen(output_path, "w");
        son_ai_export_ast_json(ast_out, module);
        if (output_path) fclose(ast_out);
        free(source);
        son_arena_free_all(&arena);
        return 0;
    }

    SonSema sema;
    son_sema_init(&sema, &arena);
    bool sema_ok = son_sema_analyze_module(&sema, module);

    if (query_symbols) {
        FILE *sym_out = stdout;
        if (output_path) sym_out = fopen(output_path, "w");
        son_ai_export_symbols_json(sym_out, &sema, module);
        if (output_path) fclose(sym_out);
        free(source);
        son_arena_free_all(&arena);
        return 0;
    }

    if (query_diagnostics) {
        FILE *diag_out = stdout;
        if (output_path) diag_out = fopen(output_path, "w");
        son_ai_export_diagnostics_json(diag_out, &sema, input_path);
        if (output_path) fclose(diag_out);
        free(source);
        son_arena_free_all(&arena);
        return sema.has_error ? 1 : 0;
    }

    if (!sema_ok || sema.has_error) {
        fprintf(stderr, "Compilation aborted due to semantic errors (%zu error%s).\n",
                sema.error_count, sema.error_count == 1 ? "" : "s");
        free(source);
        son_arena_free_all(&arena);
        return 1;
    }

    // Build SonIR SSA module
    SonIRModule *ir_module = son_ir_build_module(&arena, module);

    if (emit_ir) {
        FILE *ir_out = stdout;
        if (output_path) {
            ir_out = fopen(output_path, "w");
            if (!ir_out) {
                fprintf(stderr, "error: failed to open output file '%s'\n", output_path);
                free(source);
                son_arena_free_all(&arena);
                return 1;
            }
        }
        son_ir_dump(ir_out, ir_module);
        if (output_path) {
            fclose(ir_out);
        }
        free(source);
        son_arena_free_all(&arena);
        return 0;
    }

    if (emit_llvm) {
        FILE *llvm_out = stdout;
        if (output_path) {
            llvm_out = fopen(output_path, "w");
            if (!llvm_out) {
                fprintf(stderr, "error: failed to open output file '%s'\n", output_path);
                free(source);
                son_arena_free_all(&arena);
                return 1;
            }
        }
        bool ok = son_llvm_emit(llvm_out, ir_module);
        if (output_path) {
            fclose(llvm_out);
        }
        free(source);
        son_arena_free_all(&arena);
        return ok ? 0 : 1;
    }

    FILE *out_file = stdout;
    if (output_path) {
        out_file = fopen(output_path, "w");
        if (!out_file) {
            fprintf(stderr, "error: failed to open output file '%s'\n", output_path);
            free(source);
            son_arena_free_all(&arena);
            return 1;
        }
    }

    SonCodeGen codegen;
    son_codegen_init(&codegen, out_file);
    bool ok = son_codegen_emit_module(&codegen, module);

    if (output_path) {
        fclose(out_file);
    }

    free(source);
    son_arena_free_all(&arena);

    return ok ? 0 : 1;
}
