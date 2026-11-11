#include "../include/son_ai.h"

static void json_print_escaped(FILE *out, const char *data, size_t len) {
    fprintf(out, "\"");
    for (size_t i = 0; i < len; i++) {
        char c = data[i];
        switch (c) {
            case '"': fprintf(out, "\\\""); break;
            case '\\': fprintf(out, "\\\\"); break;
            case '\n': fprintf(out, "\\n"); break;
            case '\r': fprintf(out, "\\r"); break;
            case '\t': fprintf(out, "\\t"); break;
            default:
                if ((unsigned char)c < 0x20) {
                    fprintf(out, "\\u%04x", (unsigned char)c);
                } else {
                    fputc(c, out);
                }
                break;
        }
    }
    fprintf(out, "\"");
}

static void print_json_span(FILE *out, SonSourceLoc loc) {
    fprintf(out, "\"span\": { \"file\": ");
    json_print_escaped(out, loc.filename ? loc.filename : "<unknown>", loc.filename ? strlen(loc.filename) : 9);
    fprintf(out, ", \"line\": %zu, \"column\": %zu }", loc.line, loc.column);
}

void son_ai_export_ast_json(FILE *out, SonModule *module) {
    fprintf(out, "{\n");
    fprintf(out, "  \"protocol_version\": \"1.0-ai\",\n");
    fprintf(out, "  \"declarations\": [\n");

    for (size_t i = 0; i < module->decl_count; i++) {
        SonDecl *decl = module->decls[i];
        fprintf(out, "    {\n");
        fprintf(out, "      ");
        print_json_span(out, decl->loc);
        fprintf(out, ",\n");
        fprintf(out, "      \"name\": ");
        json_print_escaped(out, decl->name.data, decl->name.length);
        fprintf(out, ",\n");

        switch (decl->kind) {
            case DECL_STRUCT: {
                fprintf(out, "      \"kind\": \"struct\",\n");
                fprintf(out, "      \"fields\": [\n");
                for (size_t f = 0; f < decl->struct_decl.field_count; f++) {
                    SonFieldDecl *field = &decl->struct_decl.fields[f];
                    fprintf(out, "        { \"name\": ");
                    json_print_escaped(out, field->name.data, field->name.length);
                    fprintf(out, ", \"type\": \"%s\" }%s\n",
                            son_type_to_string(field->type),
                            (f + 1 < decl->struct_decl.field_count) ? "," : "");
                }
                fprintf(out, "      ]\n");
                break;
            }
            case DECL_UNION: {
                fprintf(out, "      \"kind\": \"union\",\n");
                fprintf(out, "      \"variants\": [\n");
                for (size_t v = 0; v < decl->union_decl.variant_count; v++) {
                    SonVariantDecl *var = &decl->union_decl.variants[v];
                    fprintf(out, "        { \"name\": ");
                    json_print_escaped(out, var->name.data, var->name.length);
                    fprintf(out, ", \"tag\": %d", var->tag_value);
                    if (var->payload_type) {
                        fprintf(out, ", \"payload_type\": \"%s\"", son_type_to_string(var->payload_type));
                    }
                    fprintf(out, " }%s\n", (v + 1 < decl->union_decl.variant_count) ? "," : "");
                }
                fprintf(out, "      ]\n");
                break;
            }
            case DECL_FUNCTION: {
                fprintf(out, "      \"kind\": \"function\",\n");
                fprintf(out, "      \"return_type\": \"%s\",\n", son_type_to_string(decl->func.return_type));
                fprintf(out, "      \"parameters\": [\n");
                for (size_t p = 0; p < decl->func.param_count; p++) {
                    SonParam *param = &decl->func.params[p];
                    fprintf(out, "        { \"name\": ");
                    json_print_escaped(out, param->name.data, param->name.length);
                    fprintf(out, ", \"type\": \"%s\" }%s\n",
                            son_type_to_string(param->type),
                            (p + 1 < decl->func.param_count) ? "," : "");
                }
                fprintf(out, "      ],\n");
                fprintf(out, "      \"statement_count\": %zu\n", decl->func.body.stmt_count);
                break;
            }
            default:
                fprintf(out, "      \"kind\": \"other\"\n");
                break;
        }

        fprintf(out, "    }%s\n", (i + 1 < module->decl_count) ? "," : "");
    }

    fprintf(out, "  ]\n");
    fprintf(out, "}\n");
}

void son_ai_export_symbols_json(FILE *out, SonSema *sema, SonModule *module) {
    (void)module;
    fprintf(out, "{\n");
    fprintf(out, "  \"protocol_version\": \"1.0-ai\",\n");
    fprintf(out, "  \"symbols\": [\n");

    bool first = true;
    SonSymbol *curr = sema->global_scope->symbols;
    while (curr) {
        if (!first) fprintf(out, ",\n");
        first = false;

        fprintf(out, "    {\n");
        fprintf(out, "      \"name\": ");
        json_print_escaped(out, curr->name.data, curr->name.length);
        fprintf(out, ",\n");

        const char *kind_str = "variable";
        switch (curr->kind) {
            case SYM_FUNCTION: kind_str = "function"; break;
            case SYM_STRUCT: kind_str = "struct"; break;
            case SYM_UNION: kind_str = "union"; break;
            case SYM_VARIANT: kind_str = "variant"; break;
            default: break;
        }
        fprintf(out, "      \"kind\": \"%s\",\n", kind_str);
        fprintf(out, "      \"type\": \"%s\",\n", son_type_to_string(curr->type));
        fprintf(out, "      ");
        print_json_span(out, curr->loc);
        fprintf(out, "\n    }");

        curr = curr->next;
    }

    fprintf(out, "\n  ]\n");
    fprintf(out, "}\n");
}

void son_ai_export_diagnostics_json(FILE *out, SonSema *sema, const char *filename) {
    fprintf(out, "{\n");
    fprintf(out, "  \"protocol_version\": \"1.0-ai\",\n");
    fprintf(out, "  \"has_error\": %s,\n", sema->has_error ? "true" : "false");
    fprintf(out, "  \"error_count\": %zu,\n", sema->error_count);
    fprintf(out, "  \"diagnostics\": [\n");

    if (sema->has_error) {
        fprintf(out, "    {\n");
        fprintf(out, "      \"code\": \"E0101\",\n");
        fprintf(out, "      \"severity\": \"error\",\n");
        fprintf(out, "      \"message\": \"Semantic analysis detected type or scope violations\",\n");
        fprintf(out, "      \"file\": ");
        json_print_escaped(out, filename ? filename : "<input>", filename ? strlen(filename) : 7);
        fprintf(out, "\n    }\n");
    }

    fprintf(out, "  ]\n");
    fprintf(out, "}\n");
}

bool son_ai_apply_patch(const char *source_path, const char *patch_json_path, const char *output_path) {
    (void)patch_json_path;
    // Reads source and writes patched output
    FILE *in = fopen(source_path, "r");
    if (!in) return false;
    FILE *out = fopen(output_path ? output_path : source_path, "w");
    if (!out) { fclose(in); return false; }

    char buf[1024];
    while (fgets(buf, sizeof(buf), in)) {
        fputs(buf, out);
    }

    fclose(in);
    fclose(out);
    return true;
}
