#include "../include/son_common.h"
#include "../include/son_lexer.h"
#include "../include/son_parser.h"
#include "../include/son_sema.h"
#include "../include/son_ir.h"
#include "../include/son_llvm.h"
#include "../include/son_ai.h"
#include "../include/son_codegen.h"

#ifdef _WIN32
#include <direct.h>
#define son_mkdir(p) _mkdir(p)
#else
#include <sys/stat.h>
#define son_mkdir(p) mkdir(p, 0755)
#endif

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

static void print_help(void) {
    printf("SonLang Native Compiler & Toolchain (son) v1.3.0\n");
    printf("SDK / Standard Library Root: %s\n\n", son_get_sdk_root());
    printf("Usage:\n");
    printf("  son run <file.sonl> [options]     Compile and execute immediately (no PowerShell needed)\n");
    printf("  son build <file.sonl> [options]   Compile to native executable (--native for LLVM IR)\n");
    printf("  son new <project_name>            Create a clean SonLang project (no library copy needed)\n");
    printf("  son emit-llvm <file.sonl>         Emit native LLVM IR (.ll)\n");
    printf("  son emit-ir <file.sonl>           Emit SSA SonIR\n");
    printf("  son query <file.sonl> [flags]     AI semantic queries (--ast, --symbols, --diagnostics)\n");
    printf("  son <file.sonl> -o <out.c>        Direct C99 compiler invocation\n");
}

static void extract_stem(const char *path, char *out_stem, size_t max_len) {
    const char *base = path;
    for (const char *p = path; *p; p++) {
        if (*p == '/' || *p == '\\') base = p + 1;
    }
    snprintf(out_stem, max_len, "%s", base);
    char *dot = strrchr(out_stem, '.');
    if (dot) *dot = '\0';
}

static int compile_source_to_file(
    const char *input_path,
    const char *output_path,
    bool emit_ir,
    bool emit_llvm,
    bool query_ast,
    bool query_symbols,
    bool query_diagnostics
) {
    size_t path_len = strlen(input_path);
    if (path_len >= 5 && strcmp(input_path + path_len - 5, ".sonl") == 0) {
        // Official extension (.sonl)
    } else if (path_len >= 4 && strcmp(input_path + path_len - 4, ".son") == 0) {
        fprintf(stderr, "warning: '.son' extension is deprecated; use official '.sonl' extension\n");
    } else {
        fprintf(stderr, "warning: input file '%s' does not have official '.sonl' extension\n", input_path);
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
        if (output_path) fclose(ir_out);
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
        if (output_path) fclose(llvm_out);
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

    if (output_path) fclose(out_file);
    free(source);
    son_arena_free_all(&arena);
    return ok ? 0 : 1;
}

static int cmd_new_project(const char *project_name) {
    if (!project_name || project_name[0] == '\0') {
        fprintf(stderr, "error: missing project name. Usage: son new <project_name>\n");
        return 1;
    }

    char src_dir[512], toml_path[512], main_path[512];
    son_mkdir(project_name);
    snprintf(src_dir, sizeof(src_dir), "%s/src", project_name);
    son_mkdir(src_dir);

    snprintf(toml_path, sizeof(toml_path), "%s/son.toml", project_name);
    FILE *tf = fopen(toml_path, "w");
    if (tf) {
        fprintf(tf, "[package]\nname = \"%s\"\nversion = \"0.1.0\"\nedition = \"2026\"\nentry = \"src/main.sonl\"\n", project_name);
        fclose(tf);
    }

    snprintf(main_path, sizeof(main_path), "%s/src/main.sonl", project_name);
    FILE *mf = fopen(main_path, "w");
    if (mf) {
        fprintf(mf, "import \"core/types.sonl\";\n\nfn main() -> i32 {\n    return 0;\n}\n");
        fclose(mf);
    }

    printf("Created clean SonLang project '%s' (uses global SDK stdlib directly, no library/ copy needed).\n", project_name);
    printf("Run it from anywhere:\n  cd %s && son run src/main.sonl\n", project_name);
    return 0;
}

static int cmd_build_or_run(int argc, char **argv, bool auto_run) {
    if (argc < 3) {
        fprintf(stderr, "error: missing source file. Usage: son %s <file.sonl> [options]\n", auto_run ? "run" : "build");
        return 1;
    }

    const char *input_path = argv[2];
    const char *custom_out = NULL;
    bool use_native_llvm = false;
    bool use_freestanding = false;
    char extra_flags[1024] = "";

    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            custom_out = argv[++i];
        } else if (strcmp(argv[i], "--native") == 0 || strcmp(argv[i], "-native") == 0) {
            use_native_llvm = true;
        } else if (strcmp(argv[i], "--freestanding") == 0) {
            use_freestanding = true;
        } else if (strncmp(argv[i], "-l", 2) == 0 || strncmp(argv[i], "-L", 2) == 0 || strncmp(argv[i], "-Wl", 3) == 0) {
            strncat(extra_flags, " ", sizeof(extra_flags) - strlen(extra_flags) - 1);
            strncat(extra_flags, argv[i], sizeof(extra_flags) - strlen(extra_flags) - 1);
        }
    }

    char stem[256];
    extract_stem(input_path, stem, sizeof(stem));

    son_mkdir("build");

    char inter_path[512];
    char exe_path[512];
#ifdef _WIN32
    if (custom_out) snprintf(exe_path, sizeof(exe_path), "%s", custom_out);
    else snprintf(exe_path, sizeof(exe_path), "build/%s.exe", stem);
#else
    if (custom_out) snprintf(exe_path, sizeof(exe_path), "%s", custom_out);
    else snprintf(exe_path, sizeof(exe_path), "build/%s", stem);
#endif

    if (use_native_llvm || use_freestanding) {
        snprintf(inter_path, sizeof(inter_path), "build/%s.ll", stem);
    } else {
        snprintf(inter_path, sizeof(inter_path), "build/%s.c", stem);
    }

    int comp_rc = compile_source_to_file(input_path, inter_path, false, (use_native_llvm || use_freestanding), false, false, false);
    if (comp_rc != 0) return comp_rc;

    char cmd[2048];
#ifdef _WIN32
    if (use_freestanding) {
        snprintf(cmd, sizeof(cmd), "clang -O3 -nostdlib -fuse-ld=lld -Wno-override-module -Wl,/entry:main -Wl,/subsystem:console -lkernel32 \"%s\" -o \"%s\" %s", inter_path, exe_path, extra_flags);
    } else if (use_native_llvm) {
        snprintf(cmd, sizeof(cmd), "clang -O3 -Wno-override-module \"%s\" -o \"%s\" -lws2_32 %s", inter_path, exe_path, extra_flags);
    } else {
        snprintf(cmd, sizeof(cmd), "clang -O3 \"%s\" -o \"%s\" -lws2_32 %s", inter_path, exe_path, extra_flags);
    }
#else
    if (use_native_llvm || use_freestanding) {
        snprintf(cmd, sizeof(cmd), "clang -O3 -Wno-override-module \"%s\" -o \"%s\" -lpthread -lm %s", inter_path, exe_path, extra_flags);
    } else {
        snprintf(cmd, sizeof(cmd), "clang -O3 \"%s\" -o \"%s\" -lpthread -lm %s", inter_path, exe_path, extra_flags);
    }
#endif

    int link_rc = system(cmd);
    if (link_rc != 0) {
        fprintf(stderr, "error: native build/link failed (exit code %d)\n", link_rc);
        return link_rc;
    }

    if (!auto_run) {
        printf("Built native executable: %s\n", exe_path);
        return 0;
    }

    char run_cmd[560];
#ifdef _WIN32
    for (char *p = exe_path; *p; p++) { if (*p == '/') *p = '\\'; }
    snprintf(run_cmd, sizeof(run_cmd), "\"%s\"", exe_path);
#else
    snprintf(run_cmd, sizeof(run_cmd), "./\"%s\"", exe_path);
#endif
    return system(run_cmd);
}

int main(int argc, char **argv) {
    son_init_sdk_root(argv[0]);

    if (argc < 2) {
        print_help();
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        print_help();
        return 0;
    }
    if (strcmp(cmd, "--version") == 0 || strcmp(cmd, "version") == 0) {
        printf("SonLang Toolchain v1.3.0\n");
        return 0;
    }
    if (strcmp(cmd, "new") == 0) {
        return cmd_new_project(argc >= 3 ? argv[2] : NULL);
    }
    if (strcmp(cmd, "build") == 0) {
        return cmd_build_or_run(argc, argv, false);
    }
    if (strcmp(cmd, "run") == 0) {
        return cmd_build_or_run(argc, argv, true);
    }
    if (strcmp(cmd, "emit-ir") == 0 && argc >= 3) {
        return compile_source_to_file(argv[2], (argc >= 5 && strcmp(argv[3], "-o") == 0) ? argv[4] : NULL, true, false, false, false, false);
    }
    if (strcmp(cmd, "emit-llvm") == 0 && argc >= 3) {
        return compile_source_to_file(argv[2], (argc >= 5 && strcmp(argv[3], "-o") == 0) ? argv[4] : NULL, false, true, false, false, false);
    }
    if (strcmp(cmd, "query") == 0 && argc >= 3) {
        bool q_ast = false, q_sym = false, q_diag = false;
        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "--ast") == 0) q_ast = true;
            else if (strcmp(argv[i], "--symbols") == 0) q_sym = true;
            else if (strcmp(argv[i], "--diagnostics") == 0) q_diag = true;
        }
        return compile_source_to_file(argv[2], NULL, false, false, q_ast, q_sym, q_diag);
    }

    // Legacy direct compiler mode: son <input.sonl> [-o <out.c>] [--emit-llvm ...]
    const char *input_path = argv[1];
    const char *output_path = NULL;
    bool emit_ir = false;
    bool emit_llvm = false;
    bool query_ast = false;
    bool query_symbols = false;
    bool query_diagnostics = false;

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

    return compile_source_to_file(input_path, output_path, emit_ir, emit_llvm, query_ast, query_symbols, query_diagnostics);
}
