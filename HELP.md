# HELP.md - SonLang Quick-Start Guide and Developer Cheatsheet

Welcome to **SonLang**! This guide provides everything human developers and AI assistants need to start writing, compiling, and running SonLang programs immediately.

---

## 1. Quick Syntax Overview

### Hello World
```sonl
import "library/std/io.sonl";

fn main() -> i32 {
    // Return 0 for success
    return 0;
}
```

### Variables & Mutability
```sonl
// Immutable (Default)
val port: i32 = 8080;
val host = "localhost";

// Mutable
var active_connections = 0;
active_connections = active_connections + 1;
```

### Functions
```sonl
// One-liner arrow function
fn add(a: i32, b: i32) -> i32 => a + b;

// Multi-line block function
fn compute_area(width: i32, height: i32) -> i32 {
    val area = width * height;
    return area;
}
```

### Control Flow
```sonl
// If-else
val status = if is_ready { 1 } else { 0 };

// While Loop
var i: usize = 0;
while i < 10 {
    i = i + 1;
}

// Pattern Matching
val result = match action {
    Start => 1,
    Stop => 0,
    _ => -1,
};
```

---

## 2. Common Tasks & Standard Library Cheatsheet

### 1. Working with Strings & Formatting
```sonl
import "library/core/string.sonl";
import "library/core/fmt.sonl";

var buffer: [u8; 64] = [0, 0, 0, 0, ...];
var f = fmt_new(buffer);
fmt_write_str(mut ref f, "Status: ");
fmt_write_i32(mut ref f, 200);
val len = fmt_finish(ref f);
```

### 2. JSON Parsing
```sonl
import "library/std/encoding/json.sonl";

val json = "{\"name\": \"SonLang\", \"port\": 8080}";
val port_tok = json_find_field(json, "port");
val port = json_token_as_i32(json, port_tok); // 8080
```

### 3. Dynamic Arrays (`Vec[T]`)
```sonl
import "library/std/collections/vec.sonl";

var storage: [i32; 16] = [0, 0, 0, ...];
var v = vec_new_i32(storage);
vec_push_i32(mut ref v, 42);
val item = vec_get_i32(ref v, 0); // 42
```

### 4. HTTP Server Routing
```sonl
import "library/std/net/server.sonl";

val req = "GET /api/v1/health HTTP/1.1\r\nHost: localhost\r\n\r\n";
val resp = router_dispatch(req);
// resp.status_code == 200
```

---

## 3. Native CLI Commands Reference (`son`)

The native `son` compiler (`bin/son.exe`) works directly in **CMD, PowerShell, Bash, and Zsh** without requiring any PowerShell wrapper scripts, and automatically resolves `import "std/..."` / `import "core/..."` from the global SDK (`SON_HOME` or relative to `son.exe`) — **no need to copy `library/` into your project**.

```bash
# Build native executable
son build src/main.sonl

# Compile with direct LLVM backend
son build src/main.sonl --native

# Build and execute immediately in any terminal
son run src/main.sonl

# Create a clean project (uses global stdlib automatically)
son new my_app

# Inspect intermediate representations
son emit-ir src/main.sonl      # SonIR SSA format
son emit-llvm src/main.sonl    # LLVM IR (.ll)

# Run repository conformance suite (Windows PowerShell)
./son.ps1 test
```

---

## 4. VS Code Integration

Install the official extension from the Visual Studio Marketplace:
- [https://marketplace.visualstudio.com/items?itemName=SonLang.sonlang](https://marketplace.visualstudio.com/items?itemName=SonLang.sonlang)
- Features: Autocomplete, Hover documentation, Bracket matching, and Official Red 'S' icon.
