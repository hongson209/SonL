#ifndef SON_LEXER_H
#define SON_LEXER_H

#include "son_common.h"

typedef enum {
    TOKEN_EOF = 0,
    TOKEN_IDENTIFIER,
    TOKEN_INT_LITERAL,
    TOKEN_FLOAT_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_CHAR_LITERAL,

    // Keywords
    TOKEN_KW_VAL,
    TOKEN_KW_VAR,
    TOKEN_KW_FN,
    TOKEN_KW_STRUCT,
    TOKEN_KW_UNION,
    TOKEN_KW_CONTRACT,
    TOKEN_KW_IMPL,
    TOKEN_KW_CONST,
    TOKEN_KW_IF,
    TOKEN_KW_ELSE,
    TOKEN_KW_MATCH,
    TOKEN_KW_WHILE,
    TOKEN_KW_FOR,
    TOKEN_KW_IN,
    TOKEN_KW_RETURN,
    TOKEN_KW_BREAK,
    TOKEN_KW_CONTINUE,
    TOKEN_KW_OWN,
    TOKEN_KW_REF,
    TOKEN_KW_MUT,
    TOKEN_KW_REGION,
    TOKEN_KW_UNSAFE,
    TOKEN_KW_PUB,
    TOKEN_KW_TRUE,
    TOKEN_KW_FALSE,
    TOKEN_KW_NULL,
    TOKEN_KW_AS,
    TOKEN_KW_IMPORT,
    TOKEN_KW_EXTERN,

    // Operators and Punctuators
    TOKEN_PLUS,         // +
    TOKEN_MINUS,        // -
    TOKEN_STAR,         // *
    TOKEN_SLASH,        // /
    TOKEN_PERCENT,      // %
    TOKEN_CARET,        // ^
    TOKEN_EQUAL,        // =
    TOKEN_EQ_EQ,        // ==
    TOKEN_BANG_EQ,      // !=
    TOKEN_LESS,         // <
    TOKEN_LESS_EQ,      // <=
    TOKEN_GREATER,      // >
    TOKEN_GREATER_EQ,   // >=
    TOKEN_SHL,          // <<
    TOKEN_SHR,          // >>
    TOKEN_AMP,          // &
    TOKEN_PIPE,         // |
    TOKEN_TILDE,        // ~
    TOKEN_AMP_AMP,      // &&
    TOKEN_PIPE_PIPE,    // ||
    TOKEN_BANG,         // !
    TOKEN_ARROW,        // ->
    TOKEN_FAT_ARROW,    // =>
    TOKEN_COLON,        // :
    TOKEN_COLON_COLON,  // ::
    TOKEN_SEMICOLON,    // ;
    TOKEN_COMMA,        // ,
    TOKEN_DOT,          // .
    TOKEN_LPAREN,       // (
    TOKEN_RPAREN,       // )
    TOKEN_LBRACE,       // {
    TOKEN_RBRACE,       // }
    TOKEN_LBRACKET,     // [
    TOKEN_RBRACKET,     // ]
} SonTokenKind;

typedef struct {
    SonTokenKind kind;
    SonStringSlice text;
    SonSourceLoc loc;
    union {
        int64_t int_val;
        double float_val;
        char char_val;
    };
} SonToken;

typedef struct {
    const char *source;
    size_t length;
    size_t cursor;
    size_t line;
    size_t column;
    const char *filename;
} SonLexer;

void son_lexer_init(SonLexer *lexer, const char *filename, const char *source, size_t length);
SonToken son_lexer_next_token(SonLexer *lexer);
const char* son_token_kind_str(SonTokenKind kind);

#endif // SON_LEXER_H
