# SonLang Type System Specification

## 1. Type Categories

SonLang's type system is statically typed, strongly checked at compile time, and monomorphized for maximum zero-cost runtime performance.

```
Type
├── Primitive Types
│   ├── Integer: i8, i16, i32, i64, i128, isize
│   ├── Unsigned: u8, u16, u32, u64, u128, usize
│   ├── Floating: f32, f64
│   ├── Boolean: bool
│   └── Void / Unit: void
│
├── Compound Types
│   ├── Arrays: [T; N]
│   ├── Slices: [T]
│   ├── Tuples: (T1, T2, ...)
│   └── Structs: struct { ... }
│
├── Algebraic Types
│   └── Tagged Unions: union { ... }
│
├── Reference & Ownership Types
│   ├── Owned: own T
│   ├── Immutable Reference: ref T
│   ├── Mutable Reference: mut ref T
│   └── Raw Pointer: *const T, *mut T (unsafe contexts only)
│
└── Function Types
    └── fn(T1, T2) -> R
```

---

## 2. Primitive Types

| Type | Bit-width | Description |
|---|---|---|
| `i8`, `i16`, `i32`, `i64`, `i128` | 8, 16, 32, 64, 128 | Two's-complement signed integers |
| `u8`, `u16`, `u32`, `u64`, `u128` | 8, 16, 32, 64, 128 | Unsigned integers |
| `isize`, `usize` | Target pointer width | Pointer-sized signed and unsigned integers |
| `f32`, `f64` | 32, 64 | IEEE-754 floating-point numbers |
| `bool` | 8 | Boolean value (`true` or `false`) |
| `char` | 32 | UTF-32 scalar value |
| `void` | 0 | Unit / empty return type |

---

## 3. Reference and Ownership Semantics

SonLang distinguishes between owned values and borrowed references without requiring pervasive lifetime annotations:

### 3.1 Owned Values (`own T`)
An owned value possesses exclusive authority over its backing memory.
- When an owned value is passed by value, ownership is transferred (**moved**).
- When an owned value falls out of scope, its deterministic destructor/drop logic is executed immediately.

### 3.2 Borrowed References
- `ref T`: An immutable borrow. Multiple concurrent `ref T` references to the same data are permitted.
- `mut ref T`: A mutable, exclusive borrow. Only one active `mut ref T` reference is permitted at any point in time, guaranteeing data-race freedom.

---

## 4. Algebraic Data Types & Pattern Matching

SonLang natively supports tagged unions where each variant can optionally carry payload data:

```son
pub union Result[T, E] {
    Ok(T),
    Err(E),
}

pub union Option[T] {
    Some(T),
    None,
}
```

Pattern matching over tagged unions must be exhaustive. The compiler verifies at compile time that every possible variant is handled.

---

## 5. Type Conversions and Coercions

- **No Implicit Widening**: SonLang prohibits implicit numeric conversions (e.g., passing an `i32` where an `i64` is expected is a compile-time error).
- **Explicit Casts**: Casts between compatible primitive types are performed via `val as TargetType`:
  ```son
  val x: i32 = 100;
  val y: i64 = x as i64;
  ```
- **Unsafe Casts**: Reinterpreting memory bit-patterns or raw pointers requires an `unsafe` block:
  ```son
  unsafe {
      val raw_ptr = ptr as *mut u8;
  }
  ```
