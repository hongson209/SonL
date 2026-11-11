#ifndef SON_LLVM_H
#define SON_LLVM_H

#include "son_common.h"
#include "son_ir.h"

// Emit native LLVM IR (.ll) representation directly from SonIR module
bool son_llvm_emit(FILE *out, SonIRModule *ir_module);

#endif // SON_LLVM_H
