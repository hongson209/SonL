#ifndef SON_PARSER_H
#define SON_PARSER_H

#include "son_common.h"
#include "son_lexer.h"
#include "son_ast.h"

typedef struct {
    SonLexer lexer;
    SonToken current_token;
    SonToken peek_token;
    SonArena *arena;
    bool has_error;
    SonStringSlice *current_type_params;
    size_t current_type_param_count;
} SonParser;

void son_parser_init(SonParser *parser, SonArena *arena, const char *filename, const char *source, size_t length);
SonModule* son_parser_parse_module(SonParser *parser);

#endif // SON_PARSER_H
