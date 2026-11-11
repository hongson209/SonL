#include "../include/son_lexer.h"
#include <ctype.h>

void son_lexer_init(SonLexer *lexer, const char *filename, const char *source, size_t length) {
    lexer->filename = filename;
    lexer->source = source;
    lexer->length = length;
    lexer->cursor = 0;
    lexer->line = 1;
    lexer->column = 1;
}

static char lexer_peek(SonLexer *lexer) {
    if (lexer->cursor >= lexer->length) return '\0';
    return lexer->source[lexer->cursor];
}

static char lexer_peek_next(SonLexer *lexer) {
    if (lexer->cursor + 1 >= lexer->length) return '\0';
    return lexer->source[lexer->cursor + 1];
}

static char lexer_advance(SonLexer *lexer) {
    if (lexer->cursor >= lexer->length) return '\0';
    char c = lexer->source[lexer->cursor++];
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    return c;
}

static void skip_whitespace_and_comments(SonLexer *lexer) {
    while (lexer->cursor < lexer->length) {
        char c = lexer_peek(lexer);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            lexer_advance(lexer);
        } else if (c == '/' && lexer_peek_next(lexer) == '/') {
            // Line comment
            lexer_advance(lexer);
            lexer_advance(lexer);
            while (lexer->cursor < lexer->length && lexer_peek(lexer) != '\n') {
                lexer_advance(lexer);
            }
        } else if (c == '/' && lexer_peek_next(lexer) == '*') {
            // Block comment
            lexer_advance(lexer);
            lexer_advance(lexer);
            int depth = 1;
            while (lexer->cursor < lexer->length && depth > 0) {
                if (lexer_peek(lexer) == '/' && lexer_peek_next(lexer) == '*') {
                    lexer_advance(lexer);
                    lexer_advance(lexer);
                    depth++;
                } else if (lexer_peek(lexer) == '*' && lexer_peek_next(lexer) == '/') {
                    lexer_advance(lexer);
                    lexer_advance(lexer);
                    depth--;
                } else {
                    lexer_advance(lexer);
                }
            }
        } else {
            break;
        }
    }
}

static SonTokenKind check_keyword(SonStringSlice text) {
    struct { const char *kw; SonTokenKind kind; } keywords[] = {
        {"val", TOKEN_KW_VAL},
        {"var", TOKEN_KW_VAR},
        {"fn", TOKEN_KW_FN},
        {"struct", TOKEN_KW_STRUCT},
        {"union", TOKEN_KW_UNION},
        {"contract", TOKEN_KW_CONTRACT},
        {"impl", TOKEN_KW_IMPL},
        {"const", TOKEN_KW_CONST},
        {"if", TOKEN_KW_IF},
        {"else", TOKEN_KW_ELSE},
        {"match", TOKEN_KW_MATCH},
        {"while", TOKEN_KW_WHILE},
        {"for", TOKEN_KW_FOR},
        {"in", TOKEN_KW_IN},
        {"return", TOKEN_KW_RETURN},
        {"break", TOKEN_KW_BREAK},
        {"continue", TOKEN_KW_CONTINUE},
        {"own", TOKEN_KW_OWN},
        {"ref", TOKEN_KW_REF},
        {"mut", TOKEN_KW_MUT},
        {"region", TOKEN_KW_REGION},
        {"unsafe", TOKEN_KW_UNSAFE},
        {"pub", TOKEN_KW_PUB},
        {"true", TOKEN_KW_TRUE},
        {"false", TOKEN_KW_FALSE},
        {"null", TOKEN_KW_NULL},
        {"as", TOKEN_KW_AS},
        {"import", TOKEN_KW_IMPORT},
        {"extern", TOKEN_KW_EXTERN},
    };

    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if (son_slice_eq_cstr(text, keywords[i].kw)) {
            return keywords[i].kind;
        }
    }
    return TOKEN_IDENTIFIER;
}

SonToken son_lexer_next_token(SonLexer *lexer) {
    skip_whitespace_and_comments(lexer);

    SonToken token;
    memset(&token, 0, sizeof(token));
    token.loc.filename = lexer->filename;
    token.loc.line = lexer->line;
    token.loc.column = lexer->column;
    token.text.data = lexer->source + lexer->cursor;
    token.text.length = 0;

    if (lexer->cursor >= lexer->length) {
        token.kind = TOKEN_EOF;
        return token;
    }

    char c = lexer_peek(lexer);

    // Identifiers and Keywords
    if (isalpha((unsigned char)c) || c == '_') {
        size_t start = lexer->cursor;
        while (lexer->cursor < lexer->length) {
            char ch = lexer_peek(lexer);
            if (isalnum((unsigned char)ch) || ch == '_') {
                lexer_advance(lexer);
            } else {
                break;
            }
        }
        token.text.data = lexer->source + start;
        token.text.length = lexer->cursor - start;
        token.kind = check_keyword(token.text);
        return token;
    }

    // Numbers (Integer, Hex, Binary, or Float)
    if (isdigit((unsigned char)c)) {
        size_t start = lexer->cursor;
        int64_t val = 0;

        if (c == '0' && (lexer_peek_next(lexer) == 'x' || lexer_peek_next(lexer) == 'X')) {
            lexer_advance(lexer); // '0'
            lexer_advance(lexer); // 'x'
            while (lexer->cursor < lexer->length && isxdigit((unsigned char)lexer_peek(lexer))) {
                char hc = lexer_advance(lexer);
                int digit = 0;
                if (hc >= '0' && hc <= '9') digit = hc - '0';
                else if (hc >= 'a' && hc <= 'f') digit = 10 + (hc - 'a');
                else if (hc >= 'A' && hc <= 'F') digit = 10 + (hc - 'A');
                val = (val << 4) | digit;
            }
            token.text.data = lexer->source + start;
            token.text.length = lexer->cursor - start;
            token.kind = TOKEN_INT_LITERAL;
            token.int_val = val;
            return token;
        } else if (c == '0' && (lexer_peek_next(lexer) == 'b' || lexer_peek_next(lexer) == 'B')) {
            lexer_advance(lexer); // '0'
            lexer_advance(lexer); // 'b'
            while (lexer->cursor < lexer->length && (lexer_peek(lexer) == '0' || lexer_peek(lexer) == '1')) {
                val = (val << 1) | (lexer_advance(lexer) - '0');
            }
            token.text.data = lexer->source + start;
            token.text.length = lexer->cursor - start;
            token.kind = TOKEN_INT_LITERAL;
            token.int_val = val;
            return token;
        }

        while (lexer->cursor < lexer->length && isdigit((unsigned char)lexer_peek(lexer))) {
            val = val * 10 + (lexer_advance(lexer) - '0');
        }

        // Check for decimal point followed by digit -> Float
        if (lexer_peek(lexer) == '.' && isdigit((unsigned char)lexer_peek_next(lexer))) {
            lexer_advance(lexer); // consume '.'
            while (lexer->cursor < lexer->length && isdigit((unsigned char)lexer_peek(lexer))) {
                lexer_advance(lexer);
            }
            token.text.data = lexer->source + start;
            token.text.length = lexer->cursor - start;
            token.kind = TOKEN_FLOAT_LITERAL;
            token.float_val = strtod(token.text.data, NULL);
            return token;
        }

        token.text.data = lexer->source + start;
        token.text.length = lexer->cursor - start;
        token.kind = TOKEN_INT_LITERAL;
        token.int_val = val;
        return token;
    }

    // Character literals
    if (c == '\'') {
        lexer_advance(lexer); // skip opening quote
        size_t start = lexer->cursor;
        char char_val = '\0';
        if (lexer_peek(lexer) == '\\') {
            lexer_advance(lexer);
            char esc = lexer_advance(lexer);
            switch (esc) {
                case 'n': char_val = '\n'; break;
                case 't': char_val = '\t'; break;
                case 'r': char_val = '\r'; break;
                case '0': char_val = '\0'; break;
                case '\\': char_val = '\\'; break;
                case '\'': char_val = '\''; break;
                default: char_val = esc; break;
            }
        } else {
            char_val = lexer_advance(lexer);
        }
        if (lexer_peek(lexer) == '\'') {
            lexer_advance(lexer); // skip closing quote
        }
        token.text.data = lexer->source + start;
        token.text.length = lexer->cursor - start;
        token.kind = TOKEN_CHAR_LITERAL;
        token.char_val = char_val;
        return token;
    }

    // String literals
    if (c == '"') {
        lexer_advance(lexer); // skip opening quote
        size_t start = lexer->cursor;
        while (lexer->cursor < lexer->length && lexer_peek(lexer) != '"') {
            if (lexer_peek(lexer) == '\\' && lexer->cursor + 1 < lexer->length) {
                lexer_advance(lexer); // skip escape slash
            }
            lexer_advance(lexer);
        }
        token.text.data = lexer->source + start;
        token.text.length = lexer->cursor - start;
        if (lexer_peek(lexer) == '"') {
            lexer_advance(lexer); // skip closing quote
        }
        token.kind = TOKEN_STRING_LITERAL;
        return token;
    }

    // Operators and Delimiters
    size_t start_op = lexer->cursor;
    lexer_advance(lexer);
    token.text.data = lexer->source + start_op;
    token.text.length = 1;

    switch (c) {
        case '+': token.kind = TOKEN_PLUS; break;
        case '-': 
            if (lexer_peek(lexer) == '>') {
                lexer_advance(lexer);
                token.text.length = 2;
                token.kind = TOKEN_ARROW;
            } else {
                token.kind = TOKEN_MINUS;
            }
            break;
        case '*': token.kind = TOKEN_STAR; break;
        case '/': token.kind = TOKEN_SLASH; break;
        case '%': token.kind = TOKEN_PERCENT; break;
        case '^': token.kind = TOKEN_CARET; break;
        case '=':
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                token.text.length = 2;
                token.kind = TOKEN_EQ_EQ;
            } else if (lexer_peek(lexer) == '>') {
                lexer_advance(lexer);
                token.text.length = 2;
                token.kind = TOKEN_FAT_ARROW;
            } else {
                token.kind = TOKEN_EQUAL;
            }
            break;
        case '!':
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                token.text.length = 2;
                token.kind = TOKEN_BANG_EQ;
            } else {
                token.kind = TOKEN_BANG;
            }
            break;
        case '<':
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                token.text.length = 2;
                token.kind = TOKEN_LESS_EQ;
            } else if (lexer_peek(lexer) == '<') {
                lexer_advance(lexer);
                token.text.length = 2;
                token.kind = TOKEN_SHL;
            } else {
                token.kind = TOKEN_LESS;
            }
            break;
        case '>':
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                token.text.length = 2;
                token.kind = TOKEN_GREATER_EQ;
            } else if (lexer_peek(lexer) == '>') {
                lexer_advance(lexer);
                token.text.length = 2;
                token.kind = TOKEN_SHR;
            } else {
                token.kind = TOKEN_GREATER;
            }
            break;
        case '&':
            if (lexer_peek(lexer) == '&') {
                lexer_advance(lexer);
                token.text.length = 2;
                token.kind = TOKEN_AMP_AMP;
            } else {
                token.kind = TOKEN_AMP;
            }
            break;
        case '|':
            if (lexer_peek(lexer) == '|') {
                lexer_advance(lexer);
                token.text.length = 2;
                token.kind = TOKEN_PIPE_PIPE;
            } else {
                token.kind = TOKEN_PIPE;
            }
            break;
        case '~':
            token.kind = TOKEN_TILDE;
            break;
        case ':':
            if (lexer_peek(lexer) == ':') {
                lexer_advance(lexer);
                token.text.length = 2;
                token.kind = TOKEN_COLON_COLON;
            } else {
                token.kind = TOKEN_COLON;
            }
            break;
        case ';': token.kind = TOKEN_SEMICOLON; break;
        case ',': token.kind = TOKEN_COMMA; break;
        case '.': token.kind = TOKEN_DOT; break;
        case '(': token.kind = TOKEN_LPAREN; break;
        case ')': token.kind = TOKEN_RPAREN; break;
        case '{': token.kind = TOKEN_LBRACE; break;
        case '}': token.kind = TOKEN_RBRACE; break;
        case '[': token.kind = TOKEN_LBRACKET; break;
        case ']': token.kind = TOKEN_RBRACKET; break;
        default:
            son_error_report(token.loc, "Unexpected character: '%c'", c);
            token.kind = TOKEN_EOF;
            break;
    }

    return token;
}

const char* son_token_kind_str(SonTokenKind kind) {
    switch (kind) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_IDENTIFIER: return "Identifier";
        case TOKEN_INT_LITERAL: return "Integer";
        case TOKEN_FLOAT_LITERAL: return "Float";
        case TOKEN_STRING_LITERAL: return "String";
        case TOKEN_CHAR_LITERAL: return "Char";
        case TOKEN_KW_VAL: return "'val'";
        case TOKEN_KW_VAR: return "'var'";
        case TOKEN_KW_FN: return "'fn'";
        case TOKEN_KW_STRUCT: return "'struct'";
        case TOKEN_KW_UNION: return "'union'";
        case TOKEN_KW_CONTRACT: return "'contract'";
        case TOKEN_KW_IMPL: return "'impl'";
        case TOKEN_KW_CONST: return "'const'";
        case TOKEN_KW_IF: return "'if'";
        case TOKEN_KW_ELSE: return "'else'";
        case TOKEN_KW_MATCH: return "'match'";
        case TOKEN_KW_WHILE: return "'while'";
        case TOKEN_KW_FOR: return "'for'";
        case TOKEN_KW_IN: return "'in'";
        case TOKEN_KW_RETURN: return "'return'";
        case TOKEN_KW_BREAK: return "'break'";
        case TOKEN_KW_CONTINUE: return "'continue'";
        case TOKEN_KW_OWN: return "'own'";
        case TOKEN_KW_REF: return "'ref'";
        case TOKEN_KW_MUT: return "'mut'";
        case TOKEN_KW_REGION: return "'region'";
        case TOKEN_KW_UNSAFE: return "'unsafe'";
        case TOKEN_KW_PUB: return "'pub'";
        case TOKEN_KW_TRUE: return "'true'";
        case TOKEN_KW_FALSE: return "'false'";
        case TOKEN_KW_NULL: return "'null'";
        case TOKEN_KW_AS: return "'as'";
        case TOKEN_KW_IMPORT: return "'import'";
        case TOKEN_KW_EXTERN: return "'extern'";
        case TOKEN_PLUS: return "'+'";
        case TOKEN_MINUS: return "'-'";
        case TOKEN_STAR: return "'*'";
        case TOKEN_SLASH: return "'/''";
        case TOKEN_PERCENT: return "'%'";
        case TOKEN_CARET: return "'^'";
        case TOKEN_EQUAL: return "'='";
        case TOKEN_EQ_EQ: return "'=='";
        case TOKEN_BANG_EQ: return "'!='";
        case TOKEN_LESS: return "'<'";
        case TOKEN_LESS_EQ: return "'<='";
        case TOKEN_GREATER: return "'>'";
        case TOKEN_GREATER_EQ: return "'>='";
        case TOKEN_SHL: return "'<<'";
        case TOKEN_SHR: return "'>>'";
        case TOKEN_AMP: return "'&'";
        case TOKEN_PIPE: return "'|'";
        case TOKEN_TILDE: return "'~'";
        case TOKEN_AMP_AMP: return "'&&'";
        case TOKEN_PIPE_PIPE: return "'||'";
        case TOKEN_BANG: return "'!'";
        case TOKEN_ARROW: return "'->'";
        case TOKEN_FAT_ARROW: return "'=>'";
        case TOKEN_COLON: return "':'";
        case TOKEN_COLON_COLON: return "'::'";
        case TOKEN_SEMICOLON: return "';'";
        case TOKEN_COMMA: return "','";
        case TOKEN_DOT: return "'.'";
        case TOKEN_LPAREN: return "'('";
        case TOKEN_RPAREN: return "')'";
        case TOKEN_LBRACE: return "'{'";
        case TOKEN_RBRACE: return "'}'";
        case TOKEN_LBRACKET: return "'['";
        case TOKEN_RBRACKET: return "']'";
        default: return "Unknown";
    }
}
