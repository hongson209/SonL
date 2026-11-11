#ifndef SON_SEMA_H
#define SON_SEMA_H

#include "son_common.h"
#include "son_ast.h"

typedef enum {
    SYM_VARIABLE,
    SYM_FUNCTION,
    SYM_STRUCT,
    SYM_UNION,
    SYM_VARIANT,
} SonSymbolKind;

typedef struct SonSymbol SonSymbol;

struct SonSymbol {
    SonSymbolKind kind;
    SonStringSlice name;
    SonType *type;
    SonSourceLoc loc;
    bool is_mut;
    int borrow_count;
    bool is_mutably_borrowed;
    SonSymbol *borrowed_from;
    SonDecl *decl;
    SonSymbol *next;
};

typedef struct SonScope SonScope;

struct SonScope {
    SonScope *parent;
    SonSymbol *symbols;
    SonArena *arena;
    bool is_function_scope;
    SonType *expected_return_type;
};

typedef struct {
    SonArena *arena;
    SonScope *global_scope;
    SonScope *current_scope;
    SonModule *current_module;
    SonType *current_expected_type;
    bool has_error;
    size_t error_count;
} SonSema;

void son_sema_init(SonSema *sema, SonArena *arena);
bool son_sema_analyze_module(SonSema *sema, SonModule *module);

bool son_type_equals(SonType *a, SonType *b);
const char* son_type_to_string(SonType *type);

#endif // SON_SEMA_H
