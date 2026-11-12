#ifndef SON_AST_H
#define SON_AST_H

#include "son_common.h"
#include "son_lexer.h"

typedef struct SonType SonType;
typedef struct SonExpr SonExpr;
typedef struct SonStmt SonStmt;
typedef struct SonDecl SonDecl;
typedef struct SonBlock SonBlock;
typedef struct SonMatchArm SonMatchArm;

// Stage 0 Type Representation
typedef enum {
    TYPE_VOID,
    TYPE_BOOL,
    TYPE_CHAR,
    TYPE_I8, TYPE_I16, TYPE_I32, TYPE_I64, TYPE_ISIZE,
    TYPE_U8, TYPE_U16, TYPE_U32, TYPE_U64, TYPE_USIZE,
    TYPE_F32, TYPE_F64,
    TYPE_STRUCT,
    TYPE_UNION,
    TYPE_POINTER,
    TYPE_REFERENCE,
    TYPE_SLICE,
    TYPE_ARRAY,
    TYPE_FUNCTION,
    TYPE_GENERIC_PARAM,
} SonTypeKind;

struct SonType {
    SonTypeKind kind;
    SonStringSlice name;
    SonType *sub_type;
    SonType **type_args;
    size_t type_arg_count;
    bool is_mut;
    size_t array_len;
    SonDecl *decl; // points to struct or union declaration
};

struct SonBlock {
    SonStmt **stmts;
    size_t stmt_count;
};

struct SonMatchArm {
    SonStringSlice variant_name;
    SonStringSlice payload_var; // optional binding variable for payload
    bool has_payload;
    bool is_default; // `_ =>`
    bool is_expr; // true if arm is an expression arm (=> expr)
    SonExpr *expr; // expression if is_expr
    SonBlock body; // block if !is_expr
};

// Expression AST
typedef enum {
    EXPR_INT_LITERAL,
    EXPR_FLOAT_LITERAL,
    EXPR_STRING_LITERAL,
    EXPR_CHAR_LITERAL,
    EXPR_BOOL_LITERAL,
    EXPR_IDENTIFIER,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_CALL,
    EXPR_MEMBER,
    EXPR_INDEX,
    EXPR_CAST,
    EXPR_STRUCT_INIT,
    EXPR_MATCH,
    EXPR_ARRAY_LITERAL,
} SonExprKind;

typedef struct SonFieldInit {
    SonStringSlice name;
    SonExpr *value;
} SonFieldInit;

struct SonExpr {
    SonExprKind kind;
    SonSourceLoc loc;
    SonType *inferred_type;
    union {
        int64_t int_val;
        double float_val;
        char char_val;
        SonStringSlice string_val;
        bool bool_val;
        SonStringSlice ident_name;

        struct {
            SonTokenKind op;
            SonExpr *left;
            SonExpr *right;
        } binary;

        struct {
            SonTokenKind op;
            SonExpr *operand;
        } unary;

        struct {
            SonExpr *callee;
            SonExpr **args;
            size_t arg_count;
        } call;

        struct {
            SonExpr *target;
            SonStringSlice member;
        } member;

        struct {
            SonExpr *target;
            SonExpr *index;
        } index;

        struct {
            SonExpr *expr;
            SonType *target_type;
        } cast;

        struct {
            SonStringSlice struct_name;
            SonFieldInit *fields;
            size_t field_count;
        } struct_init;

        struct {
            SonExpr *target;
            SonMatchArm *arms;
            size_t arm_count;
        } match_expr;

        struct {
            SonExpr **elements;
            size_t element_count;
        } array_literal;
    };
};

// Statement AST
typedef enum {
    STMT_VAR_DECL,
    STMT_ASSIGN,
    STMT_EXPR,
    STMT_RETURN,
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    STMT_MATCH,
    STMT_BLOCK,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_DEFER,
} SonStmtKind;

struct SonStmt {
    SonStmtKind kind;
    SonSourceLoc loc;
    union {
        struct {
            bool is_var; // var (mutable) vs val (immutable)
            SonStringSlice name;
            SonType *type_annot;
            SonExpr *init_expr;
        } var_decl;

        struct {
            SonExpr *lvalue;
            SonExpr *rvalue;
        } assign;

        SonExpr *expr;

        struct {
            SonExpr *expr;
        } ret;

        struct {
            SonExpr *cond;
            SonBlock then_block;
            SonBlock else_block;
            bool has_else;
        } if_stmt;

        struct {
            SonExpr *cond;
            SonBlock body;
        } while_stmt;

        struct {
            SonStringSlice var_name;
            SonExpr *start;
            SonExpr *end;
            SonBlock body;
        } for_stmt;

        struct {
            SonStmt *stmt;
        } defer_stmt;

        struct {
            SonExpr *expr;
            SonMatchArm *arms;
            size_t arm_count;
        } match_stmt;

        SonBlock block;
    };
};

// Declaration AST
typedef enum {
    DECL_FUNCTION,
    DECL_STRUCT,
    DECL_UNION,
    DECL_CONST,
} SonDeclKind;

typedef struct {
    SonStringSlice name;
    SonType *type;
} SonParam;

typedef struct {
    SonStringSlice name;
    SonType *type;
} SonFieldDecl;

typedef struct {
    SonStringSlice name;
    SonType *payload_type; // optional payload type (e.g. Identifier(String) or Number(i64))
    int32_t tag_value;
} SonVariantDecl;

struct SonDecl {
    SonDeclKind kind;
    SonSourceLoc loc;
    bool is_pub;
    bool is_extern;
    SonStringSlice name;
    SonStringSlice *type_params;
    size_t type_param_count;
    union {
        struct {
            SonParam *params;
            size_t param_count;
            SonType *return_type;
            SonBlock body;
        } func;

        struct {
            SonFieldDecl *fields;
            size_t field_count;
        } struct_decl;

        struct {
            SonVariantDecl *variants;
            size_t variant_count;
        } union_decl;

        struct {
            SonType *type;
            SonExpr *val_expr;
        } const_decl;
    };
};

typedef struct {
    SonDecl **decls;
    size_t decl_count;
} SonModule;

#endif // SON_AST_H
