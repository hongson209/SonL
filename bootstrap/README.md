# SonLang Stage 0 Bootstrap Compiler (Isolated C11)

## 1. Strictly Temporary Scaffold

This directory (`bootstrap/`) contains the **temporary, minimal Stage 0 compiler** for SonLang.

Its **sole purpose** is to compile the first version of the Native SonLang Compiler (`compiler/`) written in the SonLang Kernel Subset.

## 2. Non-Negotiable Boundaries

1. **Not SonLang's Identity**:
   - The implementation language (C11) and internal choices made here are strictly disposable scaffolding.
   - SonLang syntax, semantics, and design must NEVER be shaped or compromised by C limitations or C idioms.

2. **Zero Foreign Language Ecosystems**:
   - NO Cargo.
   - NO Rust.
   - NO Zig.
   - Only standard, portable C11 with zero external third-party dependencies.

3. **Strict Scope**:
   - The bootstrap compiler implements ONLY the **SonLang Kernel Subset** needed to compile the native compiler.
   - It does not attempt to implement the full SonLang feature set.

4. **Deprecation Path**:
   - Once the native compiler achieves self-hosting (compiling itself reproducibly), this bootstrap code is frozen and archived.
