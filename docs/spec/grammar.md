# SonLang Grammar & Lexical Specification

## 1. Lexical Structure

### 1.1 Source Encoding
SonLang source code must be encoded in UTF-8. The official file extension is `.sonl`.

### 1.2 Comments
- Line comments begin with `//` and extend to the end of the line.
- Block comments are enclosed in `/*` and `*/` and support arbitrary nesting.
- Doc comments begin with `///` (outer) or `//!` (module-level) and are captured as first-class semantic metadata by the compiler.

### 1.3 Identifiers
- Identifiers begin with an ASCII letter or underscore `_`, followed by alphanumeric characters or underscores:
  `[a-zA-Z_][a-zA-Z0-9_]*`
- Convention:
  - Types and Traits/Contracts: `PascalCase` (e.g., `StringSlice`, `FileStream`)
  - Functions, methods, and variables: `snake_case` (e.g., `parse_tokens`, `read_bytes`)
  - Constants: `SCREAMING_SNAKE_CASE` (e.g., `MAX_BUFFER_SIZE`)

### 1.4 Keywords
The following keywords are reserved in SonLang:

| Category | Keywords |
|---|---|
| **Declaration** | `def`, `val`, `var`, `fn`, `type`, `struct`, `union`, `contract`, `impl`, `const` |
| **Control Flow** | `if`, `else`, `match`, `while`, `for`, `in`, `return`, `break`, `continue`, `yield` |
| **Memory & Capability** | `own`, `ref`, `mut`, `region`, `alloc`, `drop` |
| **Safety & Concurrency** | `unsafe`, `spawn`, `channel`, `atomic` |
| **Module System** | `import`, `export`, `as`, `pub`, `pkg` |
| **Literals & Primitives** | `true`, `false`, `null`, `self`, `Self` |

---

## 2. Declarations and Bindings

### 2.1 Variable Bindings
SonLang provides explicit immutability by default:
- `val`: Immutable value binding.
- `var`: Mutable variable binding.

```sonl
val count: i32 = 42;
var index: usize = 0;
index = index + 1;
```

Type inference is supported when the initial expression is unambiguous:
```sonl
val name = "SonLang"; // Inferred as [u8] slice
val total = 100;      // Inferred as i32
var is_active = true; // Inferred as bool
```

### 2.2 Function Declarations
Functions are introduced with `fn`. SonLang supports both standard block functions and concise expression-bodied functions:

```sonl
// Expression-bodied function (1-liner, zero boilerplate)
pub fn add(a: i32, b: i32) -> i32 => a + b;

// Standard block function
pub fn calculate_hash(data: ref [u8], seed: u64) -> u64 {
    var hash = seed;
    var i: usize = 0;
    while i < data.len {
        hash = (hash ^ data[i] as u64) * 1099511628211;
        i = i + 1;
    }
    return hash;
}
```

### 2.3 User-Defined Types

#### Structs & Generic Structs
```sonl
pub struct Point {
    x: f64,
    y: f64,
}

// Generic Struct with Monomorphization
pub struct Vec[T] {
    storage: [T],
    length: usize,
}
```

#### Tagged Unions (Algebraic Data Types)
```sonl
// Generic Tagged Unions with payload
pub union Option[T] {
    Some(T),
    None,
}

pub union Result[T, E] {
    Ok(T),
    Err(E),
}
```

#### Contracts (Type Interfaces)
Contracts define semantic capabilities and behaviors:
```sonl
pub contract Printable {
    fn print(self: ref Self) -> void;
}
```

---

## 3. Control Flow

### 3.1 Conditional Expressions
Conditionals do not require parentheses around conditions:
```sonl
val label = if status == 0 { "ok" } else { "failure" };

if counter > 10 {
    reset();
}
```

### 3.2 First-Class Pattern Matching (`match`)
Pattern matching can be used as both statements and first-class expressions:
```sonl
// Match Expression returning a value
val unwrapped = match opt {
    Some(v) => v,
    None => default_val,
};

// Expression-bodied unwrapper
pub fn option_unwrap_or(opt: Option[i32], default_val: i32) -> i32 => match opt {
    Some(v) => v,
    None => default_val,
};
```

### 3.3 Loops
```son
// While loop
while condition {
    step();
}

// Iterator loop
for item in collection {
    process(item);
}
```

---

## 4. Formal EBNF Grammar (Core Subset)

```ebnf
Module            ::= ( ImportDecl | TopLevelDecl )*

ImportDecl        ::= "import" QualifiedPath ( "as" Identifier )? ";"
QualifiedPath     ::= Identifier ( "::" Identifier )*

TopLevelDecl      ::= "pub"? ( FunctionDecl | StructDecl | UnionDecl | ContractDecl | ImplDecl | ConstDecl )

FunctionDecl      ::= "fn" Identifier "(" ParameterList? ")" ( "->" Type )? Block
ParameterList     ::= Parameter ( "," Parameter )*
Parameter         ::= ( "self" ( ":" Type )? ) | ( Identifier ":" Type )

StructDecl        ::= "struct" Identifier "{" StructFieldList? "}"
StructFieldList   ::= StructField ( "," StructField )* ","?
StructField       ::= "pub"? Identifier ":" Type

UnionDecl         ::= "union" Identifier "{" UnionVariantList? "}"
UnionVariantList  ::= UnionVariant ( "," UnionVariant )* ","?
UnionVariant      ::= Identifier ( "(" TypeList ")" )?

ContractDecl      ::= "contract" Identifier "{" ContractMethodDecl* "}"
ContractMethodDecl::= "fn" Identifier "(" ParameterList? ")" ( "->" Type )? ";"

ImplDecl          ::= "impl" ( Identifier "for" )? Type "{" FunctionDecl* "}"

Block             ::= "{" Statement* Expression? "}"

Statement         ::= VarDecl
                    | AssignmentStmt
                    | ExpressionStmt
                    | ReturnStmt
                    | WhileStmt
                    | ForStmt
                    | BreakStmt
                    | ContinueStmt

VarDecl           ::= ( "val" | "var" ) Identifier ( ":" Type )? ( "=" Expression )? ";"
AssignmentStmt    ::= LValue "=" Expression ";"
ReturnStmt        ::= "return" Expression? ";"
WhileStmt         ::= "while" Expression Block
ForStmt           ::= "for" Identifier "in" Expression Block

Expression        ::= LogicalOrExpr
LogicalOrExpr     ::= LogicalAndExpr ( "||" LogicalAndExpr )*
LogicalAndExpr    ::= EqualityExpr ( "&&" EqualityExpr )*
EqualityExpr      ::= RelationalExpr ( ( "==" | "!=" ) RelationalExpr )*
RelationalExpr    ::= AdditiveExpr ( ( "<" | "<=" | ">" | ">=" ) AdditiveExpr )*
AdditiveExpr      ::= MultiplicativeExpr ( ( "+" | "-" ) MultiplicativeExpr )*
MultiplicativeExpr::= UnaryExpr ( ( "*" | "/" | "%" ) UnaryExpr )*
UnaryExpr         ::= ( "!" | "-" | "~" | "ref" | "mut ref" )? PrimaryExpr

PrimaryExpr       ::= Literal
                    | Identifier
                    | FunctionCall
                    | MemberAccess
                    | MatchExpr
                    | IfExpr
                    | Block
                    | "(" Expression ")"
```
