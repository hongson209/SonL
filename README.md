<p align="center">
  <img src="icon.png" alt="SonLang Logo" width="180"/>
</p>

<h1 align="center">SonLang v1.4.0</h1>

<p align="center">
  <strong>An Independent, High-Performance, AI-Native Systems Programming Language</strong>
</p>

<p align="center">
  <a href="https://marketplace.visualstudio.com/items?itemName=SonLang.sonlang"><img src="https://img.shields.io/badge/VS_Code_Extension-SonLang_v0.3.0-red.svg" alt="VS Code Extension"/></a>
  <a href="#benchmarks"><img src="https://img.shields.io/badge/Performance-Zero--Cost-brightgreen.svg" alt="Performance"/></a>
  <a href="#memory-safety-model"><img src="https://img.shields.io/badge/Release-v1.4.0_Production_Ready-blue.svg" alt="Release"/></a>
  <a href="#native-self-host-backend"><img src="https://img.shields.io/badge/Backend-Native_x86--64_%2B_PE%2FELF-purple.svg" alt="Native Backend"/></a>
  <a href="#ai-native-architecture"><img src="https://img.shields.io/badge/AI--Native-Tensor_%2B_Channel_%2B_DAG-brightgreen.svg" alt="AI Native"/></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-Apache_2.0_%2F_MIT-lightgrey.svg" alt="License"/></a>
</p>

<p align="center">
  Official File Extension: <code>.sonl</code> | Unified CLI Driver: <code>son</code>
</p>

---

## What's New in v1.4.0

- 🛡️ **Deterministic Resource Cleanup (`defer`)**:
  - `defer <stmt>;` executes cleanup statements in strict LIFO order before scope exit, early `return`, `break`, or `continue` without garbage collection overhead.
- 🔁 **Ergonomic Range Loops & Control Flow (`for .. in ..`, `break`, `continue`)**:
  - `for i in 0..10 { ... }` range iteration with static loop-scope type checking for `break` and `continue`.
- 🖨️ **Native Console I/O (`std/io.sonl`)**:
  - Real `print`, `println`, `eprintln`, `print_i32`, `print_i64`, and `print_f64` backed by slice-safe stdout/stderr output.
- 🌍 **Global SDK Resolution & Standalone CLI (`son.exe`)**:
  - Run `son run`, `son build`, `son check`, and `son new` directly in **CMD, Bash, Zsh, or PowerShell** from any directory.
  - `import "std/..."`, `"core/..."`, and `"library/..."` auto-resolve globally via `SON_HOME` or relative to `son.exe` (zero need to copy `library/` into projects).
- 🧪 **74 Automated Test Suites**: Positive + negative semantic/borrow-checker tests verified at 100% PASS.

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
│   ├── backend/               # Native x86-64 machine emitter, C99, and LLVM IR emitters
│   ├── linker/                # Standalone PE32+ (Windows) and ELF64 (Linux) linkers
│   └── main.sonl              # Compiler entrypoint
├── library/                   # Official Standard Library (.sonl)
│   ├── core/                  # Core primitives (types, slice, string, fmt)
│   └── std/                   # High-level modules (ai, sync, collections, protocol, crypto, net, fs, mem)
├── examples/                  # Production showcase examples (Echo server, Minecraft ping, High-perf API)
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
├── bootstrap/                 # Stage 0 C11 compiler source (builds bin/son.exe; keep intact)
├── AGENTS.md                  # Machine guide for autonomous AI coding agents
├── HELP.md                    # Quick-start guide and developer cheatsheet
├── son.toml                   # Project manifest and package configuration
└── son.ps1                    # Optional Windows PowerShell wrapper & test runner
```

---

## Toolchain Usage (`son`)

`son` (`bin/son.exe`) is a native CLI executable that runs in **CMD, Bash, Zsh, and PowerShell** without needing wrapper scripts. Add `bin/` to your `PATH` (or set `SON_HOME`) and compile/run from **any directory** — `import "std/..."`, `"core/..."`, and `"library/..."` resolve globally without copying `library/` into projects:

```bash
# Build native Stage 0 compiler binary once (if bin/son.exe is not present)
clang -std=c11 -O2 -Ibootstrap/include bootstrap/src/*.c -o bin/son.exe -lws2_32

# Compile and run directly in any terminal (no PowerShell needed)
bin\son.exe run tests/bootstrap/hello.sonl

# Compile to native executable (C or --native LLVM IR backend)
son build app.sonl
son build app.sonl --native

# Scaffold a clean external project (automatically links global SDK stdlib)
son new my_project

# Inspect intermediate representations
son emit-ir tests/bootstrap/hello.sonl
son emit-llvm tests/bootstrap/hello.sonl

# Execute full test suite across 70 workloads & verify self-hosting (PowerShell)
./son.ps1 test
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
