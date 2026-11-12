# AGENTS.md - Technical Specification and Instruction Manual for AI Coding Agents

This document is the authoritative technical reference for autonomous coding agents (Claude, Gemini, ChatGPT, Codex, etc.) writing, analyzing, and maintaining SonLang (`.sonl`) source code.

---

## 1. Core Language Invariants

### 1.1 File Extension, Imports, and Global SDK Resolution
- All SonLang source files MUST use the `.sonl` extension.
- **Do NOT copy/clone `library/` into external projects.** The compiler (`bin/son.exe`) automatically resolves standard library paths via `SON_HOME` or relative to the `son` binary:
  ```sonl
  import "core/string.sonl";          // Shorthand (resolved globally from SDK library/)
  import "std/collections/vec.sonl";  // Shorthand (works from any project folder)
  import "std/os/net.sonl";
  import "library/std/encoding/json.sonl"; // Full path (also supported)
  ```
- **Toolchain (`bin/son.exe` vs `bootstrap/`)**:
  - `bootstrap/` is the Stage 0 C11 compiler/CLI driver source that builds `bin/son.exe`. Do NOT delete `bootstrap/` when cleaning a repo, and do NOT confuse Stage 0 compiler source with user code.
  - Run `son run <file.sonl>` or `son build <file.sonl>` directly in CMD, Bash, or PowerShell (PowerShell `son.ps1` is optional).
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
| `library/std/mem/heap.sonl` | `alloc`, `mem_free`, `mem_realloc`, `mem_set`, `mem_copy` | Dynamic heap allocator |
| `library/std/mem/pool.sonl` | `FixedPool`, `pool_new`, `pool_alloc`, `pool_reset` | $O(1)$ Fixed-block pool allocator |
| `library/std/mem/allocator.sonl` | `HeapBlock`, `heap_alloc`, `heap_free` | Safe heap memory allocator |
| `library/std/collections/vec.sonl` | `Vec[T]`, `DynamicVecI32`, `dvec_push_i32`, `dvec_pop_i32` | Static & auto-resizing dynamic arrays |
| `library/std/collections/hashmap.sonl` | `HashMapI32`, `hashmap_new_i32`, `hashmap_insert_i32`, `hashmap_get_i32` | Open-addressing hash map |
| `library/std/collections/priority_queue.sonl` | `PriorityQueueI32`, `pq_new_i32`, `pq_push_i32`, `pq_pop_i32` | Binary Min/Max Heap ($O(\log N)$) |
| `library/std/collections/map.sonl` | `Map`, `map_new`, `map_insert`, `map_len`, `map_clear` | Fast key/value map |
| `library/std/collections/ring.sonl` | `RingBuffer`, `ring_new`, `ring_advance_tail`, `ring_advance_head` | Circular lock-free queue |
| `library/std/os/net.sonl` | `TcpSocket`, `tcp_listen`, `tcp_accept`, `tcp_poll`, `tcp_set_nonblocking`, `tcp_set_nodelay` | Non-blocking TCP sockets & WSAPoll/poll |
| `library/std/os/fs.sonl` | `File`, `file_open`, `file_seek`, `file_tell`, `file_read_exact`, `fs_write_file` | Sequential & 64-bit random-access File I/O |
| `library/std/protocol/bytebuffer.sonl` | `ByteBuffer`, `bytebuffer_write_i32_be`, `bytebuffer_read_varint` | Auto-resizing Big-Endian/VarInt buffer |
| `library/std/protocol/endian.sonl` | `swap_u16`, `swap_u32`, `to_be16`, `to_be32` | Network byte order converters |
| `library/std/protocol/varint.sonl` | 7-bit zigzag VarInt encoder/decoder | Minecraft/Protobuf VarInt |
| `library/std/protocol/minecraft.sonl` | Handshake & JSON Status Ping | Minecraft server protocol |
| `library/std/encoding/nbt.sonl` | `nbt_write_int`, `nbt_write_tag_header`, `nbt_write_end` | Minecraft NBT v1.20 binary serializer |
| `library/std/compress/zlib.sonl` | `compress_slice`, `decompress_slice` | Zero-dependency byte compression |
| `library/std/net/http.sonl` | `HttpRequest`, `HttpResponse`, `http_parse_request`, `http_response_ok` | HTTP 1.1 request/response parser |
| `library/std/net/server.sonl` | `Router`, `router_new`, `router_dispatch` | Microservice HTTP router |
| `library/std/net/url.sonl` | `ParsedUrl`, `url_parse` | URL string and query parameter parser |
| `library/std/encoding/json.sonl` | `json_reader_new`, `json_next_token`, `json_find_field`, `json_token_as_i32` | Sub-10ms JSON parser |
| `library/std/crypto/base64.sonl` | `base64_encode`, `base64_encoded_len` | Base64 encoder/decoder |
| `library/std/crypto/crc32.sonl` | `crc32_calculate(data: [u8]) -> u32` | Hardware-grade CRC-32 integrity checker |
| `library/std/crypto/sha256.sonl` | `Sha256State`, `sha256_init`, `sha256_update`, `sha256_digest_h0` | Pure SonLang SHA-256 primitives |
| `library/std/sync/thread.sonl` | `Thread`, `thread_spawn`, `thread_join` | Native OS threads (Win32 / pthread) |
| `library/std/sync/mutex.sonl` | `Mutex`, `mutex_new`, `mutex_lock`, `mutex_unlock` | Native OS mutual exclusion lock |
| `library/std/sync/channel.sonl` | `ChannelI32`, `channel_new_i32`, `channel_send_i32`, `channel_recv_i32` | Lock-free atomic MPSC ring channel |
| `library/std/sync/atomic.sonl` | `AtomicI32`, `atomic_new_i32`, `atomic_load_i32`, `atomic_fetch_add_i32` | Hardware atomic operations without locks |
| `library/std/sync/spinlock.sonl` | `Spinlock`, `spinlock_new`, `spinlock_try_lock`, `spinlock_unlock` | Zero-overhead spinlock primitives |
| `library/std/ai/tensor.sonl` | `Tensor2D`, `tensor_new`, `tensor_matmul`, `tensor_relu` | 32-byte aligned AI matrix/tensor engine |
| `library/std/ai/graph.sonl` | `ComputeGraph`, `graph_new`, `graph_add_node` | Static memory AI execution DAG |
| `library/std/ai/vector.sonl` | `vec_dot_product`, `vec_magnitude_sq`, `vec_cosine_similarity_scaled` | Vector similarity & AI embeddings engine |
| `library/std/math/bits.sonl` | `bswap16`, `bswap32`, `bswap64`, `extract_bits_u64` | 64-bit bitwise & packing intrinsics |
| `library/std/finance/orderbook.sonl`| `Order`, `TradeResult`, `match_single_order` | Low-latency orderbook matching engine |
| `library/std/game/ecs.sonl` | `Position`, `Velocity`, `ecs_update_single_position` | High-frequency Entity Component System |
| `library/std/time.sonl` | `Instant`, `instant_now`, `duration_between`, `time_nanos`, `sleep_us` | Monotonic nanosecond/microsecond timers |
| `library/std/io.sonl` | `print`, `println`, `eprintln`, `print_i32`, `print_i64`, `print_f64` | Slice-safe console stdout/stderr I/O |
| `library/std/os/path.sonl` | `path_join`, `path_has_extension`, `path_file_name` | OS path manipulation |
| `library/std/cli/args.sonl` | `cli_has_flag`, `cli_parse_port_or_default` | CLI argument parser |
| `library/std/os/process.sonl` | `ProcessEnv`, `process_env`, `process_exit` | Process environment |

---

## 5. Automated Verification Checklist for Agents

When generating or editing `.sonl` code:
1. Run `bin\son.exe run <file.sonl>` (or `son run <file.sonl>` when `bin/` is in `PATH`) directly in CMD/Bash/PowerShell — no PowerShell wrapper required for compiling/running user apps.
2. Run `bin\son.exe check <file.sonl>` for fast syntax/type/borrow verification, or `./son.ps1 test` inside the repo root to verify all 74 test suites pass.
3. Clean code convention: Do NOT add repetitive banner comments (e.g., `// SonLang Standard Library:...`). Write clean, self-documenting code.
