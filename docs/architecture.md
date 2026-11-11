# SonLang Architecture & System Design

## 1. Executive Summary

SonLang is an independent, first-class systems programming language designed for maximum performance, deterministic memory safety without a garbage collector, zero-cost abstractions, and a native semantic interface for AI-assisted software engineering.

SonLang is an entirely distinct programming language with its own grammar, intermediate representation (SonIR), native compiler, standard library, and build system. It is not a wrapper, dialect, transpiler, or frontend for any existing language.

---

## 2. Core Pillars of SonLang

1. **Language Independence**:
   - Native lexical grammar and syntax.
   - Distinct type system with algebraic data types, linear ownership, and region capabilities.
   - First-class Intermediate Representation (**SonIR**).
   - Independent standard library (`library/std/` and `library/core/`).
   - Unified build system and package manager (`son`).

2. **Deterministic Safety without Garbage Collection**:
   - Strict linear ownership tracking at compile-time.
   - Capability-based borrowing preventing data races and iterator invalidation.
   - Explicit memory allocation interfaces with support for custom allocators and scoped regions.

3. **AI-Native Semantic Architecture**:
   - The compiler exposes a structured, queryable semantic layer (AST, CFG, ownership graph, symbol resolution) over standard machine protocols.
   - Enables real-time deep semantic comprehension, automated refactoring, and formal verification without text-based scraping.

4. **Self-Hosting Compiler**:
   - The official compiler (`compiler/`) is written entirely in SonLang (`.sonl`).
   - A minimal, temporary bootstrap compiler (`bootstrap/`) written in C11 is used strictly as a Stage 0 stepping stone and kept isolated from the language's design.

---

## 3. Repository Directory Structure

```
SonLang/
├── docs/                           # Authoritative specifications and documentation
│   ├── architecture.md             # This document: system architecture and principles
│   ├── spec/                       # Formal language specification
│   │   ├── grammar.md              # Lexical rules, EBNF syntax, and operator precedence
│   │   ├── types.md                # Primitive, compound, and algebraic type rules
│   │   ├── memory_model.md         # Linear ownership, borrowing capabilities, and regions
│   │   └── abi.md                  # Calling conventions, data layout, and FFI
│   ├── sonir/                      # SonIR specification and design
│   │   ├── specification.md        # SSA instruction set, basic blocks, and type metadata
│   │   └── passes.md               # Semantic verification, safety analysis, and optimization
│   └── ai_interface/               # AI-native semantic query protocol
│       └── semantic_protocol.md    # Machine-readable AST/CFG/Type query interfaces
│
├── bootstrap/                      # TEMPORARY Stage 0 Bootstrap Compiler (C11)
│   ├── include/                    # Internal bootstrap headers
│   │   ├── son_lexer.h             # Token definitions and scanner
│   │   ├── son_parser.h            # AST node definitions and recursive-descent parser
│   │   ├── son_sema.h              # Stage 0 symbol table and type checker
│   │   ├── son_codegen.h           # Stage 0 target emitter (C11 output)
│   │   └── son_common.h            # Core utilities, memory arenas, error diagnostics
│   ├── src/                        # Bootstrap implementation (.c)
│   ├── tests/                      # Bootstrap compiler test suite
│   ├── CMakeLists.txt              # Zero-dependency build script
│   └── README.md                   # Bootstrap boundaries and isolation guidelines
│
├── compiler/                       # NATIVE SONLANG COMPILER (Written in .sonl)
│   ├── ast/                        # AST definitions and visitor patterns
│   ├── lexer/                      # Lexical scanner
│   ├── parser/                     # Full language grammar parser
│   ├── sema/                       # Type checking, ownership verification, capability checks
│   ├── sonir/                      # SonIR generator and optimization passes
│   ├── backend/                    # Target code generation (LLVM / native machine code)
│   ├── ai/                         # Semantic query server and LSP protocol handler
│   └── main.sonl                   # Compiler driver entrypoint
│
├── library/                        # SONLANG STANDARD LIBRARY (Written in .sonl)
│   ├── core/                       # Zero-allocation runtime kernel, memory primitives, panic
│   └── std/                        # Full standard library: collections, I/O, networking, time
│
├── tests/                          # Integration & Self-Hosting Verification Suites
│   ├── spec/                       # Conformance test suite for language spec compliance
│   └── selfhost/                   # Verification of reproducibility across bootstrap stages
│
└── SonLang.png                     # Official emblem and branding asset
```

---

## 4. The 7-Stage Self-Hosting Roadmap

The path from bootstrap to complete language autonomy is divided into seven rigorous phases:

```
[Stage 0: C11 Bootstrap]
        │
        ▼ compiles
[Stage 1: SonLang Compiler v0.1 (written in SonLang Kernel)]
        │
        ▼ compiles
[Stage 2: SonLang Compiler Binary v0.1]
        │
        ▼ compiles native compiler source
[Stage 3: SonLang Compiler Binary v0.2]
        │
        ▼ compiles native compiler source
[Stage 4: SonLang Compiler Binary v0.3]
        │
        ▼ bit-level / semantic verification (v0.2 == v0.3)
[Stage 5: Reproducible Self-Hosting Achieved]
        │
        ▼ full feature set & standard library expanded in .sonl
[Stage 6: Bootstrap Decoupled & Archived as Optional Fallback]
```

1. **Stage 0 — Minimal Bootstrap**:
   - Write a single-pass / simple multi-pass C11 compiler targeting ANSI C11 output.
   - Implement only the **SonLang Kernel Subset** (types, structs, tagged unions, functions, basic control flow).
   - Zero external libraries or language runtimes.

2. **Stage 1 — Native Compiler v0.1 in SonLang**:
   - Write the native compiler frontend (Lexer, Parser, AST, Sema) entirely in `.sonl` using the Kernel Subset.

3. **Stage 2 — First Native Binary**:
   - Compile Native Compiler v0.1 using the C11 bootstrap compiler, yielding executable `sonc-v0.1`.

4. **Stage 3 — First Self-Compilation**:
   - Use `sonc-v0.1` to compile its own source code, producing `sonc-v0.2`.

5. **Stage 4 — Verification of Convergence**:
   - Use `sonc-v0.2` to compile its own source code, producing `sonc-v0.3`.
   - Verify reproducibility: the output of `sonc-v0.2` and `sonc-v0.3` must be semantically equivalent and stable.

6. **Stage 5 — Full Language Maturation**:
   - Expand the full language features (SonIR optimizations, advanced pattern matching, capability system, standard library) directly within native SonLang.

7. **Stage 6 — Full Independence**:
   - Self-hosted SonLang is the primary distribution. The C11 bootstrap compiler is preserved solely as an archival bootstrap route for foreign architectures.
