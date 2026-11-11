# AGENTS.md - Technical Specification and Instruction Manual for AI Coding Agents

This document is the authoritative technical reference for autonomous coding agents (Claude, Gemini, ChatGPT, Codex, etc.) writing, analyzing, and maintaining SonLang (`.sonl`) source code.

---

## 1. Core Language Invariants

### 1.1 File Extension and Imports
- All SonLang source files MUST use the `.sonl` extension.
- Imports use double-quoted paths relative to the project root:
  ```sonl
  import "library/core/string.sonl";
  import "library/std/collections/vec.sonl";
  import "library/std/encoding/json.sonl";
  ```
- No circular dependencies are permitted.

### 1.2 Variable Immutability by Default
- `val`: Immutable constant binding (cannot be reassigned).
  ```sonl
  val port: i32 = 8080;
  val name = "SonLang"; // Inferred type
  ```
- `var`: Explicitly mutable variable binding.
  ```sonl
  var counter: i32 = 0;
  counter = counter + 1;
  ```

### 1.3 Function Syntax
- Arrow functions (expression bodies) MUST end with a semicolon:
  ```sonl
  fn square(x: i32) -> i32 => x * x;
  fn is_even(n: i32) -> bool => (n % 2) == 0;
  ```
- Block functions use explicit `return`:
  ```sonl
  fn add(a: i32, b: i32) -> i32 {
      return a + b;
  }
  ```

---

## 2. Compile-Time Ownership & Borrow Checking Invariants

SonLang uses a zero-cost linear ownership model with lexical borrow tracking. No garbage collector exists.

### Rule 1: Single Mutable Borrow XOR Multiple Shared Borrows
- Any number of shared read references (`ref T`) are permitted:
  ```sonl
  var x: i32 = 10;
  val r1 = ref x;
  val r2 = ref x; // Valid: concurrent readers
  ```
- Exactly ONE exclusive mutable reference (`mut ref T`) is allowed:
  ```sonl
  var x: i32 = 10;
  val m1 = mut ref x;
  // val m2 = mut ref x; // COMPILE ERROR: Cannot borrow 'x' as mutable more than once
  // val r1 = ref x;     // COMPILE ERROR: Cannot borrow 'x' as shared while actively borrowed as mutable
  ```

### Rule 2: No Reassignment While Borrowed
```sonl
var x: i32 = 10;
val r = ref x;
// x = 20; // COMPILE ERROR: Cannot assign to 'x' because it is actively borrowed
```

### Rule 3: No Conflicting Call Arguments
```sonl
fn swap(a: mut ref i32, b: mut ref i32) -> void { ... }
// swap(mut ref x, mut ref x); // COMPILE ERROR: Conflicting borrow of 'x'
```

---

## 3. Data Structures & Pattern Matching

### 3.1 Structs
```sonl
pub struct Point3D {
    x: f32,
    y: f32,
    z: f32,
}

val pt = Point3D { x: 1.0, y: 2.0, z: 3.0 };
val px = pt.x;
```

### 3.2 Tagged Unions (Algebraic Data Types)
```sonl
pub union Action {
    Stop,
    Move(i32, i32),
    Message([u8]),
}

fn handle_action(act: Action) -> i32 => match act {
    Stop => 0,
    Move(x, y) => x + y,
    Message(text) => text.len as i32,
    _ => -1,
};
```

---

## 4. Complete Standard Library Reference

| Module Path | Primary Types & Functions | Description |
| :--- | :--- | :--- |
| `library/core/types.sonl` | `Option[T]`, `Result[T, E]`, `option_unwrap_or` | Core algebraic primitives |
| `library/core/slice.sonl` | `slice_is_empty`, `slice_get`, `slice_equals` | Fat-pointer bounds-checked slices `[T]` |
| `library/core/string.sonl` | `str_starts_with`, `str_ends_with`, `str_find_char`, `str_parse_i32` | ASCII string operations |
| `library/core/fmt.sonl` | `Formatter`, `fmt_new`, `fmt_write_str`, `fmt_write_i32`, `fmt_finish` | Zero-allocation string formatting |
| `library/std/mem/arena.sonl` | `Arena`, `arena_new`, `arena_alloc` | $O(1)$ Region-based scoped memory arenas |
| `library/std/mem/allocator.sonl` | `HeapBlock`, `heap_alloc`, `heap_free` | Safe heap memory allocator |
| `library/std/collections/vec.sonl` | `Vec[T]`, `vec_new_i32`, `vec_push_i32`, `vec_pop_i32`, `vec_get_i32` | Dynamic array |
| `library/std/collections/map.sonl` | `Map`, `map_new`, `map_insert`, `map_len`, `map_clear` | Fast hash map |
| `library/std/collections/ring.sonl` | `RingBuffer`, `ring_new`, `ring_advance_tail`, `ring_advance_head` | Circular lock-free queue |
| `library/std/net/http.sonl` | `HttpRequest`, `HttpResponse`, `http_parse_request`, `http_response_ok` | HTTP 1.1 request/response parser |
| `library/std/net/server.sonl` | `Router`, `router_new`, `router_dispatch` | Microservice HTTP router |
| `library/std/net/url.sonl` | `ParsedUrl`, `url_parse` | URL string and query parameter parser |
| `library/std/encoding/json.sonl` | `json_reader_new`, `json_next_token`, `json_find_field`, `json_token_as_i32` | Sub-10ms JSON parser |
| `library/std/crypto/base64.sonl` | `base64_encode`, `base64_encoded_len` | Base64 encoder/decoder |
| `library/std/crypto/crc32.sonl` | `crc32_calculate(data: [u8]) -> u32` | Hardware-grade CRC-32 integrity checker |
| `library/std/sync/atomic.sonl` | `AtomicI32`, `atomic_new_i32`, `atomic_load_i32`, `atomic_fetch_add_i32` | Hardware atomic operations without locks |
| `library/std/sync/spinlock.sonl` | `Spinlock`, `spinlock_new`, `spinlock_try_lock`, `spinlock_unlock` | Zero-overhead spinlock primitives |
| `library/std/ai/vector.sonl` | `vec_dot_product`, `vec_magnitude_sq`, `vec_cosine_similarity_scaled` | Vector similarity & AI embeddings engine |
| `library/std/finance/orderbook.sonl`| `Order`, `TradeResult`, `match_single_order` | Low-latency orderbook matching engine |
| `library/std/game/ecs.sonl` | `Position`, `Velocity`, `ecs_update_single_position` | High-frequency Entity Component System |
| `library/std/time.sonl` | `Instant`, `Duration`, `instant_now`, `duration_between`, `sleep_ms` | High-resolution monotonic timers |
| `library/std/os/fs.sonl` | `fs_write_file`, `fs_write_file_len` | File I/O |
| `library/std/os/path.sonl` | `path_join`, `path_has_extension`, `path_file_name` | OS path manipulation |
| `library/std/cli/args.sonl` | `cli_has_flag`, `cli_parse_port_or_default` | CLI argument parser |
| `library/std/os/process.sonl` | `ProcessEnv`, `process_env`, `process_exit` | Process environment |

---

## 5. Automated Verification Checklist for Agents

When generating or editing `.sonl` code:
1. Run `./son.ps1 test` to verify all 50 test suites pass.
2. Clean code convention: Do NOT add repetitive banner comments (e.g., `// SonLang Standard Library:...`). Write clean, self-documenting code.
