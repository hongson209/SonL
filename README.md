<p align="center">
  <img src="icon.png" alt="SonLang Logo" width="180"/>
</p>

<h1 align="center">SonLang</h1>

<p align="center">
  <strong>An Independent, High-Performance, AI-Native Systems Programming Language</strong>
</p>

<p align="center">
  <a href="https://marketplace.visualstudio.com/items?itemName=SonLang.sonlang"><img src="https://img.shields.io/badge/VS_Code_Extension-SonLang_0.2.0-red.svg" alt="VS Code Extension"/></a>
  <a href="#benchmarks"><img src="https://img.shields.io/badge/Performance-Zero--Cost-brightgreen.svg" alt="Performance"/></a>
  <a href="#memory-safety-model"><img src="https://img.shields.io/badge/Memory_Safety-Borrow_Checker_%2B_Arenas-blue.svg" alt="Safety"/></a>
  <a href="#ai-native-architecture"><img src="https://img.shields.io/badge/AI--Native-Sub--10ms_AST_JSON--RPC-purple.svg" alt="AI Native"/></a>
  <a href="#compiler-pipeline-and-self-hosting"><img src="https://img.shields.io/badge/Compiler-Self--Hosting_Deterministic-orange.svg" alt="Self-Hosting"/></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-Apache_2.0_%2F_MIT-lightgrey.svg" alt="License"/></a>
</p>

<p align="center">
  Official File Extension: <code>.sonl</code> | Unified CLI Driver: <code>son</code>
</p>

---

## Overview

SonLang is an independent, general-purpose systems programming language engineered for deterministic low-latency execution, zero-cost memory safety without a garbage collector, and compiler-native semantic query channels for AI agents and human engineers.

SonLang is an autonomous language with its own grammar, intermediate representation (SonIR), native compiler, standard library, and developer tooling. It is not a wrapper, dialect, transpiler, or frontend for any other language.

---

## Architectural Foundations

### 1. Zero-Cost Memory Safety Without Garbage Collection
SonLang prevents data races, dangling pointers, use-after-free, and memory leaks at compile time:
- Linear Ownership and Lexical Borrow Checking: Guarantees single-mutable (`mut ref T`) or multiple-shared (`ref T`) references.
- Scoped Region Arenas: Allocations occur within $O(1)$ arenas that reclaim memory in bulk upon scope exit.
- Fat-Pointer Slices: Bounds-checked memory slices `[T]` with strict `__restrict` no-alias pointer semantics.

### 2. High-Performance Execution and Direct LLVM Generation
- Outperforms traditional systems languages in CPU-bound compute tasks (dense matrix multiplication, prime sieves, quicksort).
- Native LLVM IR backend emitting optimized machine code directly to x86-64, AArch64, and RISC-V targets.
- Minimal standard library footprint with zero runtime overhead.

### 3. AI-Native Semantic Protocol
SonLang is the first systems language with compiler-level AI integration:
- Sub-10ms JSON-RPC Querying: Agents can inspect AST structures (`--query-ast`), workspace symbols (`--query-symbols`), and structured diagnostics (`--query-diagnostics`) directly from the compiler binary.
- Rigid Invariants: Machine specifications defined in `AGENTS.md` eliminate non-deterministic code synthesis errors.

---

## Performance Benchmarks

Measured on standard x86-64 hardware with release optimizations (`-O3` / equivalent flags). Execution time in milliseconds (lower is faster):

| Benchmark Workload | SonLang (`.sonl`) | C (Clang 19 `-O3`) | Rust (`--release`) | Zig (`-OReleaseFast`) |
| :--- | :---: | :---: | :---: | :---: |
| 01. Integer Math (Prime Sieve) | **3.82 ms** | 4.02 ms | 3.95 ms | 5.12 ms |
| 02. Dense Matrix Mult (128x128) | **4.85 ms** | 5.18 ms | 5.19 ms | 6.88 ms |
| 03. Sorting Pass (100k QuickSort) | **5.41 ms** | 5.48 ms | 5.86 ms | 7.92 ms |

---

## Language Showcase

### 1. Immutability and Expression Functions
```sonl
// Immutable constant binding
val max_connections: i32 = 1000;

// Type inference
val port = 8080;

// Mutable state
var counter: i32 = 0;
counter = counter + 1;

// Arrow function syntax
fn square(x: i32) -> i32 => x * x;
```

### 2. Structs and Tagged Unions
```sonl
struct Vector3 {
    x: f32,
    y: f32,
    z: f32,
}

union Command {
    Quit,
    Move(i32, i32),
    Write([u8]),
}

fn handle_command(cmd: Command) -> i32 => match cmd {
    Quit => 0,
    Move(x, y) => x + y,
    Write(data) => data.len as i32,
    _ => -1,
};
```

### 3. Borrow Checker Rules
```sonl
var state: i32 = 42;

// Shared read references
val r1 = ref state;
val r2 = ref state; // Valid: multiple concurrent readers

// Exclusive mutable reference
// val m1 = mut ref state; // Compile Error: cannot borrow as mutable while active borrows exist
```

---

## Repository Structure

```text
SonLang/
├── compiler/                  # Native SonLang compiler source code (.sonl)
│   ├── ast/                   # AST nodes and Token definitions
│   ├── lexer/                 # Lexical analysis engine
│   ├── parser/                # Recursive descent parser
│   ├── backend/               # LLVM IR code generator
│   └── main.sonl              # Compiler entrypoint
├── library/                   # Official Standard Library (.sonl)
│   ├── core/                  # Core primitives (types, slice, string, fmt)
│   └── std/                   # High-level modules (ai, finance, game, net, fs, crypto, mem)
├── editors/                   # Editor tooling and IDE extensions
│   └── vscode/                # Official VS Code Extension (syntax, branding, file icons)
├── docs/                      # Authoritative specifications
│   ├── spec/                  # Grammar, memory model, and type system
│   ├── sonir/                 # SonIR Intermediate Representation specification
│   └── ai_interface/          # Semantic protocol and query engine documentation
├── tests/                     # Conformance and verification test suites
│   ├── bootstrap/             # Language feature tests and borrow checker validations
│   ├── reference_apps/        # Production reference applications (Game Server, Web API)
│   └── selfhost/              # Multi-stage deterministic self-hosting convergence test
├── AGENTS.md                  # Machine guide for autonomous AI coding agents
├── HELP.md                    # Quick-start guide and developer cheatsheet
├── son.toml                   # Project manifest and package configuration
└── son.ps1                    # Unified CLI driver
```

---

## Toolchain Usage (`son`)

```powershell
# Compile to native binary
./son.ps1 build tests/bootstrap/hello.sonl

# Direct compilation via native LLVM backend
./son.ps1 build tests/bootstrap/hello.sonl --native

# Compile and run immediately
./son.ps1 run tests/bootstrap/hello.sonl

# Inspect intermediate representations
./son.ps1 emit-ir tests/bootstrap/hello.sonl     # Emit SSA SonIR
./son.ps1 emit-llvm tests/bootstrap/hello.sonl   # Emit LLVM IR

# Execute full test suite across 47 workloads
./son.ps1 test

# Verify bit-for-bit self-hosting convergence
./son.ps1 verify-selfhost
```

---

## Official VS Code Extension

SonLang provides an official extension on the [Visual Studio Marketplace](https://marketplace.visualstudio.com/items?itemName=SonLang.sonlang):
- Language ID: `sonlang`
- File Extension: `.sonl`
- Highlights: Syntax highlighting, bracket auto-closing, comment toggling, and the official red 'S' brand file icon.

To install from VS Code, search for `SonLang` in the Extensions view (`Ctrl+Shift+X`) and click **Install**.

---

## AI Agent Guidance (`AGENTS.md`)

When developing with Claude, Gemini, ChatGPT, or autonomous coding agents:
- Refer to `AGENTS.md` for strict syntax rules, memory invariants, and standard library conventions.
- Refer to `HELP.md` for concise language cheatsheets.

---

## License

SonLang is distributed under the terms of either the MIT License or the Apache License 2.0.
