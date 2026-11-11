#ifndef SON_IR_H
#define SON_IR_H

#include "son_common.h"
#include "son_ast.h"

typedef enum {
    IR_VAL_CONST_INT,
    IR_VAL_CONST_FLOAT,
    IR_VAL_CONST_BOOL,
    IR_VAL_CONST_CHAR,
    IR_VAL_CONST_STRING,
    IR_VAL_REG,
    IR_VAL_PARAM,
    IR_VAL_GLOBAL,
} SonIRValueKind;

typedef struct SonIRValue SonIRValue;

struct SonIRValue {
    SonIRValueKind kind;
    SonType *type;
    union {
        int64_t int_val;
        double float_val;
        char char_val;
        bool bool_val;
        SonStringSlice string_val;
        size_t reg_id;
        SonStringSlice name;
    };
};

typedef enum {
    // Memory
    IR_OP_ALLOCA,
    IR_OP_LOAD,
    IR_OP_STORE,
    // Arithmetic & Logic
    IR_OP_ADD,
    IR_OP_SUB,
    IR_OP_MUL,
    IR_OP_DIV,
    IR_OP_REM,
    IR_OP_AND,
    IR_OP_OR,
    IR_OP_NOT,
    // Comparisons
    IR_OP_EQ,
    IR_OP_NE,
    IR_OP_LT,
    IR_OP_LE,
    IR_OP_GT,
    IR_OP_GE,
    // Aggregates & Calls
    IR_OP_CALL,
    IR_OP_GET_FIELD,
    IR_OP_INDEX,
    IR_OP_CAST,
    // Control flow terminators
    IR_OP_JUMP,
    IR_OP_BRANCH,
    IR_OP_MATCH_BRANCH,
    IR_OP_RETURN,
} SonIROpKind;

typedef struct SonIRBlock SonIRBlock;
typedef struct SonIRInst SonIRInst;

struct SonIRInst {
    SonIROpKind op;
    SonIRValue *dest; // NULL if instruction does not produce a value (e.g. store, jump, ret)
    SonIRValue **operands;
    size_t operand_count;

    // Branches
    SonIRBlock *target_true;
    SonIRBlock *target_false;

    // Field access or symbol name
    SonStringSlice extra_name;

    SonIRInst *next;
};

struct SonIRBlock {
    SonStringSlice name;
    size_t id;
    SonIRInst *first_inst;
    SonIRInst *last_inst;
    SonIRBlock *next;
};

typedef struct SonIRFunc SonIRFunc;

struct SonIRFunc {
    SonStringSlice name;
    bool is_extern;
    SonType *return_type;
    SonIRValue **params;
    size_t param_count;

    SonIRBlock *entry_block;
    SonIRBlock *first_block;
    SonIRBlock *last_block;

    size_t next_reg_id;
    size_t next_block_id;

    SonIRFunc *next;
};

typedef struct {
    SonArena *arena;
    SonModule *ast_module;
    SonIRFunc *first_func;
    SonIRFunc *last_func;
} SonIRModule;

// Lowering & Printing API
SonIRModule* son_ir_build_module(SonArena *arena, SonModule *ast_module);
void son_ir_dump(FILE *out, SonIRModule *ir_module);
bool son_ir_emit_c(FILE *out, SonIRModule *ir_module);

#endif // SON_IR_H
