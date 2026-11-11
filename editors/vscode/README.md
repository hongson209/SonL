# SonLang VS Code Official Extension

Official extension providing language recognition, rich syntax highlighting, and official logo recognition for the SonLang systems programming language (`.sonl`).

---

## Features

- **Official Language Recognition**: Registers `.sonl` with Language ID `sonlang`.
- **Accurate Syntax Highlighting**: Fully matches SonLang syntax including:
  - Declarations: `val`, `var`, `fn`, `struct`, `union`, `pub`
  - Control Flow: `if`, `else`, `while`, `match`, `return`, `import`
  - Borrow Checking Keywords: `ref`, `mut ref`
  - Compact Syntax: `=>`, `->`
  - Type System: `i32`, `i64`, `u8`, `usize`, `f64`, `bool`, etc.
- **Language Icon**: Contributes the official SonLang red 'S' vector icon for language files.
- **Language Configuration**: Bracket matching, auto-closing quotes and braces, and line commenting (`//`).

---

## Installation

### Install Pre-Packaged `.vsix`
In the terminal, run:
```bash
code --install-extension editors/vscode/sonlang-0.2.0.vsix
```
Or in VS Code:
1. Open the **Extensions** view (`Ctrl+Shift+X`).
2. Click the `...` (Views and More Actions) menu in the top right.
3. Select **Install from VSIX...**.
4. Choose `editors/vscode/sonlang-0.2.0.vsix`.
