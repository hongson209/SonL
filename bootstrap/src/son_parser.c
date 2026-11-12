#include "../include/son_parser.h"
#include <ctype.h>

void son_parser_init(SonParser *parser, SonArena *arena, const char *filename, const char *source, size_t length) {
    son_lexer_init(&parser->lexer, filename, source, length);
    parser->arena = arena;
    parser->has_error = false;
    parser->current_type_params = NULL;
    parser->current_type_param_count = 0;
    parser->current_token = son_lexer_next_token(&parser->lexer);
    parser->peek_token = son_lexer_next_token(&parser->lexer);
}

static SonToken advance(SonParser *parser) {
    SonToken prev = parser->current_token;
    parser->current_token = parser->peek_token;
    parser->peek_token = son_lexer_next_token(&parser->lexer);
    return prev;
}

static bool check(SonParser *parser, SonTokenKind kind) {
    return parser->current_token.kind == kind;
}

static bool match(SonParser *parser, SonTokenKind kind, SonToken *out_token) {
    if (check(parser, kind)) {
        SonToken prev = advance(parser);
        if (out_token) *out_token = prev;
        return true;
    }
    return false;
}

static SonToken expect(SonParser *parser, SonTokenKind kind, const char *msg) {
    if (check(parser, kind)) {
        return advance(parser);
    }
    son_error_report(parser->current_token.loc, "Expected %s %s, got %s", son_token_kind_str(kind), msg, son_token_kind_str(parser->current_token.kind));
    parser->has_error = true;
    return parser->current_token;
}

// Forward declarations
static SonExpr* parse_expression(SonParser *parser);
static SonStmt* parse_statement(SonParser *parser);
static SonBlock parse_block(SonParser *parser);
static SonType* parse_type(SonParser *parser);

static SonType* parse_type(SonParser *parser) {
    SonType *type = (SonType*)son_arena_alloc(parser->arena, sizeof(SonType), sizeof(void*));
    memset(type, 0, sizeof(SonType));

    if (match(parser, TOKEN_KW_REF, NULL)) {
        type->kind = TYPE_REFERENCE;
        type->is_mut = false;
        type->sub_type = parse_type(parser);
        return type;
    }
    if (match(parser, TOKEN_KW_MUT, NULL)) {
        expect(parser, TOKEN_KW_REF, "after 'mut'");
        type->kind = TYPE_REFERENCE;
        type->is_mut = true;
        type->sub_type = parse_type(parser);
        return type;
    }
    if (match(parser, TOKEN_STAR, NULL)) {
        type->kind = TYPE_POINTER;
        if (match(parser, TOKEN_KW_MUT, NULL)) {
            type->is_mut = true;
        } else {
            match(parser, TOKEN_KW_CONST, NULL);
            type->is_mut = false;
        }
        type->sub_type = parse_type(parser);
        return type;
    }
    if (match(parser, TOKEN_LBRACKET, NULL)) {
        SonType *elem_t = parse_type(parser);
        if (match(parser, TOKEN_SEMICOLON, NULL)) {
            SonToken len_tok = expect(parser, TOKEN_INT_LITERAL, "for fixed array length");
            expect(parser, TOKEN_RBRACKET, "to close array type");
            type->kind = TYPE_ARRAY;
            type->sub_type = elem_t;
            type->array_len = (size_t)len_tok.int_val;
            return type;
        }
        type->kind = TYPE_SLICE;
        type->sub_type = elem_t;
        expect(parser, TOKEN_RBRACKET, "to close slice type");
        return type;
    }

    SonToken ident = expect(parser, TOKEN_IDENTIFIER, "for type name");
    type->name = ident.text;

    for (size_t i = 0; i < parser->current_type_param_count; i++) {
        if (son_slice_eq(parser->current_type_params[i], ident.text)) {
            type->kind = TYPE_GENERIC_PARAM;
            return type;
        }
    }

    if (son_slice_eq_cstr(ident.text, "void")) type->kind = TYPE_VOID;
    else if (son_slice_eq_cstr(ident.text, "char")) type->kind = TYPE_CHAR;
    else if (son_slice_eq_cstr(ident.text, "bool")) type->kind = TYPE_BOOL;
    else if (son_slice_eq_cstr(ident.text, "i8")) type->kind = TYPE_I8;
    else if (son_slice_eq_cstr(ident.text, "i16")) type->kind = TYPE_I16;
    else if (son_slice_eq_cstr(ident.text, "i32")) type->kind = TYPE_I32;
    else if (son_slice_eq_cstr(ident.text, "i64")) type->kind = TYPE_I64;
    else if (son_slice_eq_cstr(ident.text, "isize")) type->kind = TYPE_ISIZE;
    else if (son_slice_eq_cstr(ident.text, "u8")) type->kind = TYPE_U8;
    else if (son_slice_eq_cstr(ident.text, "u16")) type->kind = TYPE_U16;
    else if (son_slice_eq_cstr(ident.text, "u32")) type->kind = TYPE_U32;
    else if (son_slice_eq_cstr(ident.text, "u64")) type->kind = TYPE_U64;
    else if (son_slice_eq_cstr(ident.text, "usize")) type->kind = TYPE_USIZE;
    else if (son_slice_eq_cstr(ident.text, "f32")) type->kind = TYPE_F32;
    else if (son_slice_eq_cstr(ident.text, "f64")) type->kind = TYPE_F64;
    else type->kind = TYPE_STRUCT;

    if (match(parser, TOKEN_LBRACKET, NULL)) {
        type->type_args = (SonType**)son_arena_alloc(parser->arena, sizeof(SonType*) * 4, sizeof(void*));
        type->type_arg_count = 0;
        do {
            type->type_args[type->type_arg_count++] = parse_type(parser);
        } while (match(parser, TOKEN_COMMA, NULL));
        expect(parser, TOKEN_RBRACKET, "to close generic type arguments");
    }

    return type;
}

static SonExpr* parse_primary(SonParser *parser) {
    SonExpr *expr = (SonExpr*)son_arena_alloc(parser->arena, sizeof(SonExpr), sizeof(void*));
    memset(expr, 0, sizeof(SonExpr));
    expr->loc = parser->current_token.loc;

    SonToken tok;
    if (match(parser, TOKEN_INT_LITERAL, &tok)) {
        expr->kind = EXPR_INT_LITERAL;
        expr->int_val = tok.int_val;
        return expr;
    }
    if (match(parser, TOKEN_FLOAT_LITERAL, &tok)) {
        expr->kind = EXPR_FLOAT_LITERAL;
        expr->float_val = tok.float_val;
        return expr;
    }
    if (match(parser, TOKEN_CHAR_LITERAL, &tok)) {
        expr->kind = EXPR_CHAR_LITERAL;
        expr->char_val = tok.char_val;
        return expr;
    }
    if (match(parser, TOKEN_STRING_LITERAL, &tok)) {
        expr->kind = EXPR_STRING_LITERAL;
        expr->string_val = tok.text;
        return expr;
    }
    if (match(parser, TOKEN_KW_TRUE, &tok)) {
        expr->kind = EXPR_BOOL_LITERAL;
        expr->bool_val = true;
        return expr;
    }
    if (match(parser, TOKEN_KW_FALSE, &tok)) {
        expr->kind = EXPR_BOOL_LITERAL;
        expr->bool_val = false;
        return expr;
    }
    if (match(parser, TOKEN_IDENTIFIER, &tok)) {
        SonStringSlice name = tok.text;

        // Check if struct literal: PascalCase Identifier followed by '{'
        if (name.length > 0 && isupper((unsigned char)name.data[0]) && check(parser, TOKEN_LBRACE)) {
            advance(parser); // consume '{'
            expr->kind = EXPR_STRUCT_INIT;
            expr->struct_init.struct_name = name;
            
            size_t cap = 64;
            expr->struct_init.fields = (SonFieldInit*)son_arena_alloc(parser->arena, sizeof(SonFieldInit) * cap, sizeof(void*));
            expr->struct_init.field_count = 0;

            while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
                SonToken f_name = expect(parser, TOKEN_IDENTIFIER, "for struct field name");
                expect(parser, TOKEN_COLON, "after struct field name");
                SonExpr *f_val = parse_expression(parser);

                expr->struct_init.fields[expr->struct_init.field_count].name = f_name.text;
                expr->struct_init.fields[expr->struct_init.field_count].value = f_val;
                expr->struct_init.field_count++;

                if (!match(parser, TOKEN_COMMA, NULL)) break;
            }
            expect(parser, TOKEN_RBRACE, "to close struct literal");
            return expr;
        }

        expr->kind = EXPR_IDENTIFIER;
        expr->ident_name = name;
        return expr;
    }
    if (match(parser, TOKEN_KW_MATCH, NULL)) {
        expr->kind = EXPR_MATCH;
        expr->match_expr.target = parse_expression(parser);
        expect(parser, TOKEN_LBRACE, "to open match body");

        size_t cap = 16;
        expr->match_expr.arms = (SonMatchArm*)son_arena_alloc(parser->arena, sizeof(SonMatchArm) * cap, sizeof(void*));
        expr->match_expr.arm_count = 0;

        while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
            SonMatchArm *arm = &expr->match_expr.arms[expr->match_expr.arm_count++];
            memset(arm, 0, sizeof(SonMatchArm));

            SonToken pattern_tok = expect(parser, TOKEN_IDENTIFIER, "for match pattern or '_'");
            if (son_slice_eq_cstr(pattern_tok.text, "_")) {
                arm->is_default = true;
            } else {
                arm->variant_name = pattern_tok.text;
                if (match(parser, TOKEN_LPAREN, NULL)) {
                    arm->has_payload = true;
                    SonToken binding_tok = expect(parser, TOKEN_IDENTIFIER, "for payload variable name");
                    arm->payload_var = binding_tok.text;
                    expect(parser, TOKEN_RPAREN, "to close payload binding");
                }
            }

            expect(parser, TOKEN_FAT_ARROW, "after match pattern ('=>')");
            if (check(parser, TOKEN_LBRACE)) {
                arm->is_expr = false;
                arm->body = parse_block(parser);
            } else {
                arm->is_expr = true;
                arm->expr = parse_expression(parser);
            }

            match(parser, TOKEN_COMMA, NULL);
        }
        expect(parser, TOKEN_RBRACE, "to close match body");
        return expr;
    }
    if (match(parser, TOKEN_LBRACKET, NULL)) {
        expr->kind = EXPR_ARRAY_LITERAL;
        size_t cap = 256;
        expr->array_literal.elements = (SonExpr**)son_arena_alloc(parser->arena, sizeof(SonExpr*) * cap, sizeof(void*));
        expr->array_literal.element_count = 0;

        if (!check(parser, TOKEN_RBRACKET)) {
            do {
                if (expr->array_literal.element_count >= cap) {
                    cap *= 2;
                    SonExpr **new_elems = (SonExpr**)son_arena_alloc(parser->arena, sizeof(SonExpr*) * cap, sizeof(void*));
                    memcpy(new_elems, expr->array_literal.elements, sizeof(SonExpr*) * expr->array_literal.element_count);
                    expr->array_literal.elements = new_elems;
                }
                expr->array_literal.elements[expr->array_literal.element_count++] = parse_expression(parser);
            } while (match(parser, TOKEN_COMMA, NULL) && !check(parser, TOKEN_RBRACKET));
        }
        expect(parser, TOKEN_RBRACKET, "to close array literal");
        return expr;
    }
    if (match(parser, TOKEN_LPAREN, NULL)) {
        expr = parse_expression(parser);
        expect(parser, TOKEN_RPAREN, "to close grouping parenthesis");
        return expr;
    }

    son_error_report(parser->current_token.loc, "Unexpected token in expression: %s", son_token_kind_str(parser->current_token.kind));
    parser->has_error = true;
    advance(parser);
    return expr;
}

static SonExpr* parse_postfix(SonParser *parser) {
    SonExpr *expr = parse_primary(parser);

    while (true) {
        if (match(parser, TOKEN_LPAREN, NULL)) {
            // Function call
            SonExpr *call = (SonExpr*)son_arena_alloc(parser->arena, sizeof(SonExpr), sizeof(void*));
            call->kind = EXPR_CALL;
            call->loc = expr->loc;
            call->call.callee = expr;

            size_t cap = 16;
            call->call.args = (SonExpr**)son_arena_alloc(parser->arena, sizeof(SonExpr*) * cap, sizeof(void*));
            call->call.arg_count = 0;

            if (!check(parser, TOKEN_RPAREN)) {
                do {
                    call->call.args[call->call.arg_count++] = parse_expression(parser);
                } while (match(parser, TOKEN_COMMA, NULL));
            }
            expect(parser, TOKEN_RPAREN, "to close function call argument list");
            expr = call;
        } else if (match(parser, TOKEN_DOT, NULL)) {
            // Member access
            SonToken member = expect(parser, TOKEN_IDENTIFIER, "for member access");
            SonExpr *mem_expr = (SonExpr*)son_arena_alloc(parser->arena, sizeof(SonExpr), sizeof(void*));
            mem_expr->kind = EXPR_MEMBER;
            mem_expr->loc = expr->loc;
            mem_expr->member.target = expr;
            mem_expr->member.member = member.text;
            expr = mem_expr;
        } else if (match(parser, TOKEN_LBRACKET, NULL)) {
            // Index access: expr[idx]
            SonExpr *index_expr = parse_expression(parser);
            expect(parser, TOKEN_RBRACKET, "to close array/slice index");
            SonExpr *idx_node = (SonExpr*)son_arena_alloc(parser->arena, sizeof(SonExpr), sizeof(void*));
            idx_node->kind = EXPR_INDEX;
            idx_node->loc = expr->loc;
            idx_node->index.target = expr;
            idx_node->index.index = index_expr;
            expr = idx_node;
        } else if (match(parser, TOKEN_KW_AS, NULL)) {
            // Cast expression
            SonType *target_type = parse_type(parser);
            SonExpr *cast_expr = (SonExpr*)son_arena_alloc(parser->arena, sizeof(SonExpr), sizeof(void*));
            cast_expr->kind = EXPR_CAST;
            cast_expr->loc = expr->loc;
            cast_expr->cast.expr = expr;
            cast_expr->cast.target_type = target_type;
            expr = cast_expr;
        } else {
            break;
        }
    }

    return expr;
}

static SonExpr* parse_unary(SonParser *parser) {
    SonToken op;
    if (match(parser, TOKEN_KW_MUT, &op)) {
        expect(parser, TOKEN_KW_REF, "after 'mut' in reference expression");
        SonExpr *operand = parse_unary(parser);
        SonExpr *expr = (SonExpr*)son_arena_alloc(parser->arena, sizeof(SonExpr), sizeof(void*));
        expr->kind = EXPR_UNARY;
        expr->loc = op.loc;
        expr->unary.op = TOKEN_KW_MUT;
        expr->unary.operand = operand;
        return expr;
    }
    if (match(parser, TOKEN_KW_REF, &op)) {
        SonExpr *operand = parse_unary(parser);
        SonExpr *expr = (SonExpr*)son_arena_alloc(parser->arena, sizeof(SonExpr), sizeof(void*));
        expr->kind = EXPR_UNARY;
        expr->loc = op.loc;
        expr->unary.op = TOKEN_KW_REF;
        expr->unary.operand = operand;
        return expr;
    }
    if (match(parser, TOKEN_MINUS, &op) || match(parser, TOKEN_BANG, &op) || match(parser, TOKEN_TILDE, &op)) {
        SonExpr *operand = parse_unary(parser);
        SonExpr *expr = (SonExpr*)son_arena_alloc(parser->arena, sizeof(SonExpr), sizeof(void*));
        expr->kind = EXPR_UNARY;
        expr->loc = op.loc;
        expr->unary.op = op.kind;
        expr->unary.operand = operand;
        return expr;
    }
    return parse_postfix(parser);
}

static SonExpr* parse_multiplicative(SonParser *parser) {
    SonExpr *expr = parse_unary(parser);
    while (check(parser, TOKEN_STAR) || check(parser, TOKEN_SLASH) || check(parser, TOKEN_PERCENT)) {
        SonToken op = advance(parser);
        SonExpr *right = parse_unary(parser);
        SonExpr *bin = (SonExpr*)son_arena_alloc(parser->arena, sizeof(SonExpr), sizeof(void*));
        bin->kind = EXPR_BINARY;
        bin->loc = op.loc;
        bin->binary.op = op.kind;
        bin->binary.left = expr;
        bin->binary.right = right;
        expr = bin;
    }
    return expr;
}

static SonExpr* parse_additive(SonParser *parser) {
    SonExpr *expr = parse_multiplicative(parser);
    while (check(parser, TOKEN_PLUS) || check(parser, TOKEN_MINUS)) {
        SonToken op = advance(parser);
        SonExpr *right = parse_multiplicative(parser);
        SonExpr *bin = (SonExpr*)son_arena_alloc(parser->arena, sizeof(SonExpr), sizeof(void*));
        bin->kind = EXPR_BINARY;
        bin->loc = op.loc;
        bin->binary.op = op.kind;
        bin->binary.left = expr;
        bin->binary.right = right;
        expr = bin;
    }
    return expr;
}

static SonExpr* parse_shift(SonParser *parser) {
    SonExpr *expr = parse_additive(parser);
    while (check(parser, TOKEN_SHL) || check(parser, TOKEN_SHR)) {
        SonToken op = advance(parser);
        SonExpr *right = parse_additive(parser);
        SonExpr *bin = (SonExpr*)son_arena_alloc(parser->arena, sizeof(SonExpr), sizeof(void*));
        bin->kind = EXPR_BINARY;
        bin->loc = op.loc;
        bin->binary.op = op.kind;
        bin->binary.left = expr;
        bin->binary.right = right;
        expr = bin;
    }
    return expr;
}

static SonExpr* parse_relational(SonParser *parser) {
    SonExpr *expr = parse_shift(parser);
    while (check(parser, TOKEN_LESS) || check(parser, TOKEN_LESS_EQ) ||
           check(parser, TOKEN_GREATER) || check(parser, TOKEN_GREATER_EQ)) {
        SonToken op = advance(parser);
        SonExpr *right = parse_shift(parser);
        SonExpr *bin = (SonExpr*)son_arena_alloc(parser->arena, sizeof(SonExpr), sizeof(void*));
        bin->kind = EXPR_BINARY;
        bin->loc = op.loc;
        bin->binary.op = op.kind;
        bin->binary.left = expr;
        bin->binary.right = right;
        expr = bin;
    }
    return expr;
}

static SonExpr* parse_equality(SonParser *parser) {
    SonExpr *expr = parse_relational(parser);
    while (check(parser, TOKEN_EQ_EQ) || check(parser, TOKEN_BANG_EQ)) {
        SonToken op = advance(parser);
        SonExpr *right = parse_relational(parser);
        SonExpr *bin = (SonExpr*)son_arena_alloc(parser->arena, sizeof(SonExpr), sizeof(void*));
        bin->kind = EXPR_BINARY;
        bin->loc = op.loc;
        bin->binary.op = op.kind;
        bin->binary.left = expr;
        bin->binary.right = right;
        expr = bin;
    }
    return expr;
}

static SonExpr* parse_bitwise_and(SonParser *parser) {
    SonExpr *expr = parse_equality(parser);
    while (check(parser, TOKEN_AMP)) {
        SonToken op = advance(parser);
        SonExpr *right = parse_equality(parser);
        SonExpr *bin = (SonExpr*)son_arena_alloc(parser->arena, sizeof(SonExpr), sizeof(void*));
        bin->kind = EXPR_BINARY;
        bin->loc = op.loc;
        bin->binary.op = op.kind;
        bin->binary.left = expr;
        bin->binary.right = right;
        expr = bin;
    }
    return expr;
}

static SonExpr* parse_bitwise_xor(SonParser *parser) {
    SonExpr *expr = parse_bitwise_and(parser);
    while (check(parser, TOKEN_CARET)) {
        SonToken op = advance(parser);
        SonExpr *right = parse_bitwise_and(parser);
        SonExpr *bin = (SonExpr*)son_arena_alloc(parser->arena, sizeof(SonExpr), sizeof(void*));
        bin->kind = EXPR_BINARY;
        bin->loc = op.loc;
        bin->binary.op = op.kind;
        bin->binary.left = expr;
        bin->binary.right = right;
        expr = bin;
    }
    return expr;
}

static SonExpr* parse_bitwise_or(SonParser *parser) {
    SonExpr *expr = parse_bitwise_xor(parser);
    while (check(parser, TOKEN_PIPE)) {
        SonToken op = advance(parser);
        SonExpr *right = parse_bitwise_xor(parser);
        SonExpr *bin = (SonExpr*)son_arena_alloc(parser->arena, sizeof(SonExpr), sizeof(void*));
        bin->kind = EXPR_BINARY;
        bin->loc = op.loc;
        bin->binary.op = op.kind;
        bin->binary.left = expr;
        bin->binary.right = right;
        expr = bin;
    }
    return expr;
}

static SonExpr* parse_logical_and(SonParser *parser) {
    SonExpr *expr = parse_bitwise_or(parser);
    while (check(parser, TOKEN_AMP_AMP)) {
        SonToken op = advance(parser);
        SonExpr *right = parse_bitwise_or(parser);
        SonExpr *bin = (SonExpr*)son_arena_alloc(parser->arena, sizeof(SonExpr), sizeof(void*));
        bin->kind = EXPR_BINARY;
        bin->loc = op.loc;
        bin->binary.op = op.kind;
        bin->binary.left = expr;
        bin->binary.right = right;
        expr = bin;
    }
    return expr;
}

static SonExpr* parse_logical_or(SonParser *parser) {
    SonExpr *expr = parse_logical_and(parser);
    while (check(parser, TOKEN_PIPE_PIPE)) {
        SonToken op = advance(parser);
        SonExpr *right = parse_logical_and(parser);
        SonExpr *bin = (SonExpr*)son_arena_alloc(parser->arena, sizeof(SonExpr), sizeof(void*));
        bin->kind = EXPR_BINARY;
        bin->loc = op.loc;
        bin->binary.op = op.kind;
        bin->binary.left = expr;
        bin->binary.right = right;
        expr = bin;
    }
    return expr;
}

static SonExpr* parse_expression(SonParser *parser) {
    return parse_logical_or(parser);
}

static SonBlock parse_block(SonParser *parser) {
    expect(parser, TOKEN_LBRACE, "to enter block");
    SonBlock block;
    size_t cap = 32;
    block.stmts = (SonStmt**)son_arena_alloc(parser->arena, sizeof(SonStmt*) * cap, sizeof(void*));
    block.stmt_count = 0;

    while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
        block.stmts[block.stmt_count++] = parse_statement(parser);
    }
    expect(parser, TOKEN_RBRACE, "to close block");
    return block;
}

static SonStmt* parse_statement(SonParser *parser) {
    SonStmt *stmt = (SonStmt*)son_arena_alloc(parser->arena, sizeof(SonStmt), sizeof(void*));
    memset(stmt, 0, sizeof(SonStmt));
    stmt->loc = parser->current_token.loc;

    SonToken kw;
    if (match(parser, TOKEN_KW_VAL, &kw) || match(parser, TOKEN_KW_VAR, &kw)) {
        stmt->kind = STMT_VAR_DECL;
        stmt->var_decl.is_var = (kw.kind == TOKEN_KW_VAR);
        SonToken name = expect(parser, TOKEN_IDENTIFIER, "for variable name");
        stmt->var_decl.name = name.text;

        if (match(parser, TOKEN_COLON, NULL)) {
            stmt->var_decl.type_annot = parse_type(parser);
        }
        if (match(parser, TOKEN_EQUAL, NULL)) {
            stmt->var_decl.init_expr = parse_expression(parser);
        }
        expect(parser, TOKEN_SEMICOLON, "after variable declaration");
        return stmt;
    }

    if (match(parser, TOKEN_KW_RETURN, &kw)) {
        stmt->kind = STMT_RETURN;
        if (!check(parser, TOKEN_SEMICOLON)) {
            stmt->ret.expr = parse_expression(parser);
        }
        expect(parser, TOKEN_SEMICOLON, "after return statement");
        return stmt;
    }

    if (match(parser, TOKEN_KW_IF, NULL)) {
        stmt->kind = STMT_IF;
        stmt->if_stmt.cond = parse_expression(parser);
        stmt->if_stmt.then_block = parse_block(parser);
        if (match(parser, TOKEN_KW_ELSE, NULL)) {
            stmt->if_stmt.has_else = true;
            if (check(parser, TOKEN_KW_IF)) {
                SonStmt *nested_if = parse_statement(parser);
                stmt->if_stmt.else_block.stmts = (SonStmt**)son_arena_alloc(parser->arena, sizeof(SonStmt*), sizeof(void*));
                stmt->if_stmt.else_block.stmts[0] = nested_if;
                stmt->if_stmt.else_block.stmt_count = 1;
            } else {
                stmt->if_stmt.else_block = parse_block(parser);
            }
        }
        return stmt;
    }

    if (match(parser, TOKEN_KW_WHILE, NULL)) {
        stmt->kind = STMT_WHILE;
        stmt->while_stmt.cond = parse_expression(parser);
        stmt->while_stmt.body = parse_block(parser);
        return stmt;
    }

    if (check(parser, TOKEN_KW_MATCH)) {
        SonExpr *m_expr = parse_expression(parser);
        stmt->kind = STMT_EXPR;
        stmt->expr = m_expr;
        match(parser, TOKEN_SEMICOLON, NULL);
        return stmt;
    }

    // Expression or Assignment
    SonExpr *expr = parse_expression(parser);
    if (match(parser, TOKEN_EQUAL, NULL)) {
        stmt->kind = STMT_ASSIGN;
        stmt->assign.lvalue = expr;
        stmt->assign.rvalue = parse_expression(parser);
        expect(parser, TOKEN_SEMICOLON, "after assignment statement");
        return stmt;
    }

    stmt->kind = STMT_EXPR;
    stmt->expr = expr;
    expect(parser, TOKEN_SEMICOLON, "after expression statement");
    return stmt;
}

static char* parser_read_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < 0) { fclose(f); return NULL; }
    char *buf = (char*)malloc(size + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    if (out_len) *out_len = (size_t)size;
    return buf;
}

static char* parser_read_file_resolved(const char *import_path, const char *current_file, size_t *out_len) {
    // 1. Try exact path (relative to CWD or absolute)
    char *buf = parser_read_file(import_path, out_len);
    if (buf) return buf;

    // 2. Try relative to current file's directory
    if (current_file) {
        char dir_buf[1024];
        snprintf(dir_buf, sizeof(dir_buf), "%s", current_file);
        for (char *p = dir_buf; *p; p++) {
            if (*p == '\\') *p = '/';
        }
        char *slash = strrchr(dir_buf, '/');
        if (slash) {
            *slash = '\0';
            char candidate[1024];
            snprintf(candidate, sizeof(candidate), "%s/%s", dir_buf, import_path);
            buf = parser_read_file(candidate, out_len);
            if (buf) return buf;
        }
    }

    // 3. Try global SDK root (<SON_HOME>/<import_path>)
    const char *sdk_root = son_get_sdk_root();
    if (sdk_root && sdk_root[0] != '\0') {
        char candidate[1024];
        snprintf(candidate, sizeof(candidate), "%s/%s", sdk_root, import_path);
        buf = parser_read_file(candidate, out_len);
        if (buf) return buf;

        // 4. Shorthand standard library import: e.g. import "std/os/net.sonl" or "core/string.sonl"
        snprintf(candidate, sizeof(candidate), "%s/library/%s", sdk_root, import_path);
        buf = parser_read_file(candidate, out_len);
        if (buf) return buf;
    }

    return NULL;
}

static SonDecl* parse_top_level_decl(SonParser *parser, SonModule *module) {
    if (match(parser, TOKEN_KW_IMPORT, NULL)) {
        SonToken path_tok = expect(parser, TOKEN_STRING_LITERAL, "for imported file path");
        expect(parser, TOKEN_SEMICOLON, "after import path");

        char import_path[512];
        snprintf(import_path, sizeof(import_path), "%.*s", (int)path_tok.text.length, path_tok.text.data);

        size_t imp_len = 0;
        char *imp_src_raw = parser_read_file_resolved(import_path, path_tok.loc.filename, &imp_len);
        if (!imp_src_raw) {
            son_error_report(path_tok.loc, "Cannot open imported file '%s'", import_path);
            parser->has_error = true;
            return NULL;
        }

        // Copy source buffer into persistent arena memory so AST slices stay valid
        char *imp_src = (char*)son_arena_alloc(parser->arena, imp_len + 1, 1);
        memcpy(imp_src, imp_src_raw, imp_len + 1);
        free(imp_src_raw);

        const char *fname_copy = (const char*)son_arena_alloc(parser->arena, strlen(import_path) + 1, 1);
        memcpy((void*)fname_copy, import_path, strlen(import_path) + 1);

        SonParser sub_parser;
        son_parser_init(&sub_parser, parser->arena, fname_copy, imp_src, imp_len);

        while (!check(&sub_parser, TOKEN_EOF) && !sub_parser.has_error) {
            SonDecl *d = parse_top_level_decl(&sub_parser, module);
            if (d && module) {
                // Check if decl name already imported to prevent duplicates
                bool exists = false;
                for (size_t i = 0; i < module->decl_count; i++) {
                    if (son_slice_eq(module->decls[i]->name, d->name)) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    module->decls[module->decl_count++] = d;
                }
            }
        }
        if (sub_parser.has_error) {
            parser->has_error = true;
        }
        return NULL;
    }

    if (match(parser, TOKEN_KW_EXTERN, NULL)) {
        if (check(parser, TOKEN_STRING_LITERAL)) {
            advance(parser); // "C"
            expect(parser, TOKEN_LBRACE, "after extern \"C\"");
            while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
                expect(parser, TOKEN_KW_FN, "for extern function declaration");
                SonDecl *fn_decl = (SonDecl*)son_arena_alloc(parser->arena, sizeof(SonDecl), sizeof(void*));
                memset(fn_decl, 0, sizeof(SonDecl));
                fn_decl->loc = parser->current_token.loc;
                fn_decl->kind = DECL_FUNCTION;
                fn_decl->is_extern = true;
                SonToken fn_name = expect(parser, TOKEN_IDENTIFIER, "for extern function name");
                fn_decl->name = fn_name.text;

                expect(parser, TOKEN_LPAREN, "for parameter list");
                size_t cap = 16;
                fn_decl->func.params = (SonParam*)son_arena_alloc(parser->arena, sizeof(SonParam) * cap, sizeof(void*));
                fn_decl->func.param_count = 0;
                if (!check(parser, TOKEN_RPAREN)) {
                    do {
                        SonToken p_name = expect(parser, TOKEN_IDENTIFIER, "for parameter name");
                        expect(parser, TOKEN_COLON, "after parameter name");
                        SonType *p_type = parse_type(parser);
                        fn_decl->func.params[fn_decl->func.param_count].name = p_name.text;
                        fn_decl->func.params[fn_decl->func.param_count].type = p_type;
                        fn_decl->func.param_count++;
                    } while (match(parser, TOKEN_COMMA, NULL));
                }
                expect(parser, TOKEN_RPAREN, "to close parameter list");

                if (match(parser, TOKEN_ARROW, NULL)) {
                    fn_decl->func.return_type = parse_type(parser);
                } else {
                    SonType *void_t = (SonType*)son_arena_alloc(parser->arena, sizeof(SonType), sizeof(void*));
                    void_t->kind = TYPE_VOID;
                    fn_decl->func.return_type = void_t;
                }
                expect(parser, TOKEN_SEMICOLON, "after extern function declaration");
                if (module) {
                    module->decls[module->decl_count++] = fn_decl;
                }
            }
            expect(parser, TOKEN_RBRACE, "to close extern block");
            return NULL;
        } else if (match(parser, TOKEN_KW_FN, NULL)) {
            SonDecl *fn_decl = (SonDecl*)son_arena_alloc(parser->arena, sizeof(SonDecl), sizeof(void*));
            memset(fn_decl, 0, sizeof(SonDecl));
            fn_decl->loc = parser->current_token.loc;
            fn_decl->kind = DECL_FUNCTION;
            fn_decl->is_extern = true;
            SonToken fn_name = expect(parser, TOKEN_IDENTIFIER, "for extern function name");
            fn_decl->name = fn_name.text;

            expect(parser, TOKEN_LPAREN, "for parameter list");
            size_t cap = 16;
            fn_decl->func.params = (SonParam*)son_arena_alloc(parser->arena, sizeof(SonParam) * cap, sizeof(void*));
            fn_decl->func.param_count = 0;
            if (!check(parser, TOKEN_RPAREN)) {
                do {
                    SonToken p_name = expect(parser, TOKEN_IDENTIFIER, "for parameter name");
                    expect(parser, TOKEN_COLON, "after parameter name");
                    SonType *p_type = parse_type(parser);
                    fn_decl->func.params[fn_decl->func.param_count].name = p_name.text;
                    fn_decl->func.params[fn_decl->func.param_count].type = p_type;
                    fn_decl->func.param_count++;
                } while (match(parser, TOKEN_COMMA, NULL));
            }
            expect(parser, TOKEN_RPAREN, "to close parameter list");

            if (match(parser, TOKEN_ARROW, NULL)) {
                fn_decl->func.return_type = parse_type(parser);
            } else {
                SonType *void_t = (SonType*)son_arena_alloc(parser->arena, sizeof(SonType), sizeof(void*));
                void_t->kind = TYPE_VOID;
                fn_decl->func.return_type = void_t;
            }
            expect(parser, TOKEN_SEMICOLON, "after extern function declaration");
            return fn_decl;
        }
    }

    SonDecl *decl = (SonDecl*)son_arena_alloc(parser->arena, sizeof(SonDecl), sizeof(void*));
    memset(decl, 0, sizeof(SonDecl));
    decl->loc = parser->current_token.loc;
    decl->is_pub = match(parser, TOKEN_KW_PUB, NULL);

    if (match(parser, TOKEN_KW_FN, NULL)) {
        decl->kind = DECL_FUNCTION;
        SonToken fn_name = expect(parser, TOKEN_IDENTIFIER, "for function name");
        decl->name = fn_name.text;

        expect(parser, TOKEN_LPAREN, "for parameter list");
        size_t cap = 16;
        decl->func.params = (SonParam*)son_arena_alloc(parser->arena, sizeof(SonParam) * cap, sizeof(void*));
        decl->func.param_count = 0;

        if (!check(parser, TOKEN_RPAREN)) {
            do {
                SonToken p_name = expect(parser, TOKEN_IDENTIFIER, "for parameter name");
                expect(parser, TOKEN_COLON, "after parameter name");
                SonType *p_type = parse_type(parser);
                decl->func.params[decl->func.param_count].name = p_name.text;
                decl->func.params[decl->func.param_count].type = p_type;
                decl->func.param_count++;
            } while (match(parser, TOKEN_COMMA, NULL));
        }
        expect(parser, TOKEN_RPAREN, "to close parameter list");

        if (match(parser, TOKEN_ARROW, NULL)) {
            decl->func.return_type = parse_type(parser);
        } else {
            SonType *void_t = (SonType*)son_arena_alloc(parser->arena, sizeof(SonType), sizeof(void*));
            void_t->kind = TYPE_VOID;
            decl->func.return_type = void_t;
        }

        if (match(parser, TOKEN_FAT_ARROW, NULL)) {
            SonExpr *expr = parse_expression(parser);
            expect(parser, TOKEN_SEMICOLON, "after expression-bodied function");

            SonStmt *ret_stmt = (SonStmt*)son_arena_alloc(parser->arena, sizeof(SonStmt), sizeof(void*));
            memset(ret_stmt, 0, sizeof(SonStmt));
            ret_stmt->kind = STMT_RETURN;
            ret_stmt->loc = expr->loc;
            ret_stmt->ret.expr = expr;

            decl->func.body.stmts = (SonStmt**)son_arena_alloc(parser->arena, sizeof(SonStmt*), sizeof(void*));
            decl->func.body.stmts[0] = ret_stmt;
            decl->func.body.stmt_count = 1;
            return decl;
        }

        decl->func.body = parse_block(parser);
        return decl;
    }

    if (match(parser, TOKEN_KW_STRUCT, NULL)) {
        decl->kind = DECL_STRUCT;
        SonToken s_name = expect(parser, TOKEN_IDENTIFIER, "for struct name");
        decl->name = s_name.text;

        if (match(parser, TOKEN_LBRACKET, NULL)) {
            decl->type_params = (SonStringSlice*)son_arena_alloc(parser->arena, sizeof(SonStringSlice) * 4, sizeof(void*));
            decl->type_param_count = 0;
            do {
                SonToken tp = expect(parser, TOKEN_IDENTIFIER, "for generic type parameter name");
                decl->type_params[decl->type_param_count++] = tp.text;
            } while (match(parser, TOKEN_COMMA, NULL));
            expect(parser, TOKEN_RBRACKET, "to close generic type parameters");
        }

        parser->current_type_params = decl->type_params;
        parser->current_type_param_count = decl->type_param_count;

        expect(parser, TOKEN_LBRACE, "to open struct body");
        size_t cap = 128;
        decl->struct_decl.fields = (SonFieldDecl*)son_arena_alloc(parser->arena, sizeof(SonFieldDecl) * cap, sizeof(void*));
        decl->struct_decl.field_count = 0;

        while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
            SonToken f_name = expect(parser, TOKEN_IDENTIFIER, "for field name");
            expect(parser, TOKEN_COLON, "after field name");
            SonType *f_type = parse_type(parser);
            decl->struct_decl.fields[decl->struct_decl.field_count].name = f_name.text;
            decl->struct_decl.fields[decl->struct_decl.field_count].type = f_type;
            decl->struct_decl.field_count++;

            match(parser, TOKEN_COMMA, NULL);
        }
        expect(parser, TOKEN_RBRACE, "to close struct body");
        parser->current_type_params = NULL;
        parser->current_type_param_count = 0;
        return decl;
    }

    if (match(parser, TOKEN_KW_UNION, NULL)) {
        decl->kind = DECL_UNION;
        SonToken u_name = expect(parser, TOKEN_IDENTIFIER, "for union name");
        decl->name = u_name.text;

        if (match(parser, TOKEN_LBRACKET, NULL)) {
            decl->type_params = (SonStringSlice*)son_arena_alloc(parser->arena, sizeof(SonStringSlice) * 4, sizeof(void*));
            decl->type_param_count = 0;
            do {
                SonToken tp = expect(parser, TOKEN_IDENTIFIER, "for generic type parameter name");
                decl->type_params[decl->type_param_count++] = tp.text;
            } while (match(parser, TOKEN_COMMA, NULL));
            expect(parser, TOKEN_RBRACKET, "to close generic type parameters");
        }

        parser->current_type_params = decl->type_params;
        parser->current_type_param_count = decl->type_param_count;

        expect(parser, TOKEN_LBRACE, "to open union body");
        size_t cap = 128;
        decl->union_decl.variants = (SonVariantDecl*)son_arena_alloc(parser->arena, sizeof(SonVariantDecl) * cap, sizeof(void*));
        decl->union_decl.variant_count = 0;

        int32_t tag = 0;
        while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
            SonToken v_name = expect(parser, TOKEN_IDENTIFIER, "for variant name");
            SonVariantDecl *var = &decl->union_decl.variants[decl->union_decl.variant_count++];
            var->name = v_name.text;
            var->tag_value = tag++;
            var->payload_type = NULL;

            if (match(parser, TOKEN_LPAREN, NULL)) {
                var->payload_type = parse_type(parser);
                expect(parser, TOKEN_RPAREN, "to close variant payload type");
            }

            match(parser, TOKEN_COMMA, NULL);
        }
        expect(parser, TOKEN_RBRACE, "to close union body");
        parser->current_type_params = NULL;
        parser->current_type_param_count = 0;
        return decl;
    }

    son_error_report(parser->current_token.loc, "Unexpected top-level token: %s", son_token_kind_str(parser->current_token.kind));
    parser->has_error = true;
    advance(parser);
    return NULL;
}

SonModule* son_parser_parse_module(SonParser *parser) {
    SonModule *module = (SonModule*)son_arena_alloc(parser->arena, sizeof(SonModule), sizeof(void*));
    size_t cap = 1024;
    module->decls = (SonDecl**)son_arena_alloc(parser->arena, sizeof(SonDecl*) * cap, sizeof(void*));
    module->decl_count = 0;

    while (!check(parser, TOKEN_EOF) && !parser->has_error) {
        SonDecl *decl = parse_top_level_decl(parser, module);
        if (decl) {
            module->decls[module->decl_count++] = decl;
        }
    }

    return module;
}
