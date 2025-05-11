# Noroshi Compiler (`n0ryst`) Documentation

## Overview

`n0ryst` is a compiler for the Noroshi language, written in C. Noroshi is a minimalist programming language for console applications. The compiler transforms `.nrs` files into assembly (`.asm`) and builds native executables for macOS (macho64), FreeBSD (ELF64), and Linux (ELF64) using `nasm` and platform-specific linkers (`ld`). Dependencies are managed via `.noi` files.

- **Goals**:
  - Simple syntax for rapid prototyping.
  - Efficient native code generation.
  - Cross-platform support (macOS, FreeBSD, Linux).
- **Version**: 1.09 (2024-2025).
- **Platforms**: macOS, FreeBSD, Linux.
- **Dependencies**: `nasm`, `ld` (macOS), `ld.bfd` (FreeBSD/Linux), `gcc`/`clang`.
- **License**: BSL 1.0.

## Installation

### macOS
- **Requirements**:
  - Xcode 11+ (`xcode-select --install`).
  - `nasm`:
    ```bash
    sudo port install nasm
    ```
- **Build**:
  ```bash
  cd /Users/retochan/Projects/n0ryst-project
  make
  ```
- **Verify**:
  ```bash
  ./n0ryst --version
  ```
  Output: `n0ryst ver. 1.09, 2024-2025`.

### FreeBSD
- **Requirements**:
  - FreeBSD 12+.
  - `nasm`, `binutils`, `gcc`/`clang`:
    ```bash
    sudo pkg install nasm binutils gcc
    ```
- **Build**:
  ```bash
  cd /path/to/n0ryst-project
  gmake
  ```
- **Verify**:
  ```bash
  ./n0ryst --version
  ```

### Linux (Ubuntu/Debian)
- **Requirements**:
  - `nasm`, `binutils`, `gcc`:
    ```bash
    sudo apt update
    sudo apt install nasm binutils gcc
    ```
- **Build**:
  ```bash
  cd /path/to/n0ryst-project
  make
  ```
- **Verify**:
  ```bash
  ./n0ryst --version
  ```

### Environment Check
- NASM: `nasm -v`
- GCC/Clang: `gcc --version` or `clang --version`
- LD: `ld --version`
- FreeBSD/Linux libraries: `/usr/include`, `/usr/lib`.

## Usage

### Command Line
```bash
n0ryst [options] [path]
```
- `path`: Directory with `.nrs` (code) and `.noi` (configuration).
- Options:
  - `--help`: Show help.
  - `--version`: Show version.
  - `--target <platform>`: Target platform (macos, freebsd, linux).

Example:
```bash
./n0ryst examples
```

### Project Structure
```
project/
├── main.nrs        # Main code
├── module1.nrs     # Dependencies
└── config.noi      # Configuration
```

### Configuration (.noi)
Example:
```noi
kernel: N0roshi
deps: module1.nrs
exit_key: "q"
start: main
mem: 16M
level: debug
prompt: "> "
```
- `kernel`: Output executable name.
- `deps`: Dependency files (up to 16).
- `exit_key`: Program exit key.
- `start`, `mem`, `level`, `prompt`: Reserved for future use.

### Compilation Output
```
n0ryst ver. 1.09, 2024-2025
Starting compiling
Compiling dependency: examples/module1.nrs
[Parsing] 50%
[Parsing] 100%
[Type Checking] 0%
[Type Checking] 100%
[Codegen] 0%
[Codegen] 100%
Compiling main: examples/main.nrs
[Parsing] 50%
[Parsing] 100%
[Type Checking] 0%
[Type Checking] 100%
[Codegen] 0%
[Codegen] 100%
Compiled in 0.XX seconds
```

## Noroshi Language

### Syntax

#### Blocks
```
/+[ <commands> /=]
```

#### Variables
```
let x = 42
```
- Types: `Int64`, strings.
- Stored on stack.

#### Output
```
pnt "Hello"
```
- Outputs via `printf` (FreeBSD/Linux) or `_printf` (macOS).

#### Keyboard Check
```
kbchk
```
- Reads input via `getchar` (FreeBSD/Linux) or `_getchar` (macOS).
- Exits on `exit_key`.

#### Example
`main.nrs`:
```
/+[ let score = 0 pnt "Game started" kbchk /=]
```

### Limitations
- No loops, conditionals, or functions.
- Types: numbers, strings, `=` symbol.
- Limits: 1024 tokens, 2048 AST nodes, 4096-byte output.

## Compiler Internals

### Compilation Stages
1. **Read .noi**:
   - Parses `kernel`, `deps`, `exit_key`.
2. **Find .nrs**:
   - Main file not listed in `deps`.
3. **Lexical Analysis**:
   - Tokens: `TOKEN_OP_BLOCK_START` (`/+[`), `TOKEN_STRING`, `TOKEN_EOF`, etc.
4. **Parsing**:
   - AST: `AST_BLOCK`, `AST_VARDECL`, `AST_PRINT`, `AST_KBCHK`, `AST_END`.
5. **Code Generation**:
   - Generates assembly for macho64 (macOS) or ELF64 (FreeBSD/Linux).
   - Main file: `_main` (macOS) or `main` (FreeBSD/Linux).
   - Dependencies: `module_init`.
6. **Assembly**:
   - macOS: `nasm -f macho64`, `ld -lSystem`.
   - FreeBSD: `nasm -f elf64`, `ld.bfd -lc`.
   - Linux: `nasm -f elf64`, `ld -lc`.
7. **Cleanup**:
   - Removes `.o`, `.asm`.

### Assembly (macOS)
```nasm
section .data
msg db 'N0roshi running...', 10, 0
input_buf db 0
section .text
extern _getchar
extern _printf
global _main
kbhit:
  mov rax, 0x2000003
  mov rdi, 0
  syscall
  cmp rax, -1
  je .no_key
  mov byte [rel input_buf], al
  mov rax, 1
  ret
.no_key:
  xor rax, rax
  ret
_main:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  lea rdi, [rel msg]
  xor rax, rax
  call _printf
  call kbhit
  test rax, rax
  jz .no_input
  mov al, byte [rel input_buf]
  cmp al, 'q'
  je .exit
.no_input:
.exit:
  mov rsp, rbp
  pop rbp
  mov rax, 0x2000001
  mov rdi, 0
  syscall
```

### Assembly (FreeBSD/Linux)
```nasm
section .data
msg db 'N0roshi running...', 10, 0
input_buf db 0
section .text
extern getchar
extern printf
global main
kbhit:
  mov rax, 0
  mov rdi, 0
  mov rsi, input_buf
  mov rdx, 1
  syscall
  cmp rax, 0
  je .no_key
  mov rax, 1
  ret
.no_key:
  xor rax, rax
  ret
main:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  lea rdi, [msg]
  xor rax, rax
  call printf
  call kbhit
  test rax, rax
  jz .no_input
  mov al, [input_buf]
  cmp al, 'q'
  je .exit
.no_input:
.exit:
  mov rsp, rbp
  pop rbp
  mov rax, 60
  xor rdi, rdi
  syscall
```

## Cross-Compilation

### macOS
- **Command**:
  ```bash
  ./n0ryst --target macos examples
  ```
- **Linker**: `ld -lSystem -syslibroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk`.

### FreeBSD
- **Command**:
  ```bash
  ./n0ryst --target freebsd examples
  ```
- **Linker**: `ld.bfd -lc`.
- **Libraries**: `/usr/lib` (FreeBSD `libc`).

### Linux
- **Command**:
  ```bash
  ./n0ryst --target linux examples
  ```
- **Linker**: `ld -lc`.
- **Libraries**: `/usr/lib` (GNU `glibc`).

## Examples

### Simple Program
`main.nrs`:
```
/+[ pnt "Welcome" kbchk /=]
```
`config.noi`:
```noi
kernel: Welcome
exit_key: "q"
```
```bash
./n0ryst --target macos .
./Welcome
```
Output: `N0roshi running...`

### With Variable
`main.nrs`:
```
/+[ let x = 10 pnt "Score set" kbchk /=]
```
- Sets `x = 10`.
- Outputs `Score set`.

### With Dependency
`module1.nrs`:
```
/+[ pnt "Module loaded" /=]
```
`main.nrs`:
```
/+[ pnt "Main started" kbchk /=]
```
`config.noi`:
```noi
kernel: Game
deps: module1.nrs
exit_key: "q"
```

## Debugging

### Compilation Errors
- **Lexical**:
  ```
  Lexing error at position X, character 'Y'
  ```
  - Check `.nrs` at position `X`.
- **Parsing**:
  ```
  Parsing error: expected block start
  ```
  - Ensure code starts with `/+[` and ends with `/=]`.
- **Files**:
  ```
  Error: No main .nrs file found
  ```
  - Add `.nrs` not listed in `deps`.

### Build Errors
- **macOS**:
  ```
  ld: library 'System' not found
  ```
  - Verify SDK: `/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk`.
- **FreeBSD**:
  ```
  ld: cannot find -lc
  ```
  - Install `libc`: `sudo pkg install lib32`.
- **Linux**:
  ```
  ld: cannot find -lc
  ```
  - Install `libc6-dev`: `sudo apt install libc6-dev`.

### Runtime Errors
- No output:
  - Check `pnt` in code.
- No exit on `q`:
  - Verify `exit_key` in `.noi`.

## Enhancements
1. **Syntax**:
   - Add loops (`while`), conditionals (`if`), functions.
   - Support `Float`, arrays.
2. **Compiler**:
   - Optimize AST.
   - Detailed error messages with line numbers.
3. **Cross-Compilation**:
   - Auto-detect platform.
4. **Logging**:
   - Use `level`, `prompt` from `.noi`.
   - Accurate compilation time.

## Resources

- **Build**:
  ```bash
  make
  ./n0ryst --target macos examples
  ./N0roshi
  ```

---

# Noroshiコンパイラ（`n0ryst`）ドキュメント

## 概要

`n0ryst`は、C言語で書かれたNoroshi言語用のコンパイラです。Noroshiは、コンソールアプリケーション向けのミニマリストなプログラミング言語です。コンパイラは`.nrs`ファイルをアセンバリ（`.asm`）に変換し、macOS（macho64）、FreeBSD（ELF64）、Linux（ELF64）向けのネイティブ実行ファイルを生成します。`nasm`とプラットフォーム固有のリンカ（`ld`）を使用します。依存関係は`.noi`ファイルで管理されます。

- **目的**:
  - シンプルな構文で迅速なプロトタイピング。
  - 効率的なネイティブコード生成。
  - クロスプラットフォームサポート（macOS、FreeBSD、Linux）。
- **バージョン**: 1.09（2024-2025）。
- **プラットフォーム**: macOS、FreeBSD、Linux。
- **依存関係**: `nasm`、macOS用`ld`、FreeBSD/Linux用`ld.bfd`、 `gcc`/`clang`。
- **ライセンス**: BSL 1.0。

## インストール

### macOS
- **要件**:
  - Xcode 11+（`xcode-select --install`）。
  - `nasm`:
    ```bash
    sudo port install nasm
    ```
- **ビルド**:
  ```bash
  cd /Users/retochan/Projects/n0ryst-project
  make
  ```
- **確認**:
  ```bash
  ./n0ryst --version
  ```
  出力: `n0ryst ver. 1.09, 2024-2025`。

### FreeBSD
- **要件**:
  - FreeBSD 12+。
  - `nasm`、`binutils`、`gcc`/`clang`:
    ```bash
    sudo pkg install nasm binutils gcc
    ```
- **ビルド**:
  ```bash
  cd /path/to/n0ryst-project
  gmake
  ```
- **確認**:
  ```bash
  ./n0ryst --version
  ```

### Linux（Ubuntu/Debian）
- **要件**:
  - `nasm`、`binutils`、`gcc`:
    ```bash
    sudo apt update
    sudo apt install nasm binutils gcc
    ```
- **ビルド**:
  ```bash
  cd /path/to/n0ryst-project
  make
  ```
- **確認**:
  ```bash
  ./n0ryst --version
  ```

### 環境チェック
- NASM: `nasm -v`
- GCC/Clang: `gcc --version` または `clang --version`
- LD: `ld --version`
- FreeBSD/Linuxライブラリ: `/usr/include`、`/usr/lib`。

## 使用方法

### コマンドライン
```bash
n0ryst [options] [path]
```
- `path`: `.nrs`（コード）と`.noi`（設定）を含むディレクトリ。
- オプション:
  - `--help`: ヘルプを表示。
  - `--version`: バージョンを表示。
  - `--target <platform>`: ターゲットプラットフォーム（macos、推奨、freebsd、linux）。

例:
```bash
./n0ryst examples
```

### プロジェクト構造
```
project/
├── main.nrs        # メインコード
├── module1.nrs     # 依存関係
└── config.noi      # 設定
```

### 設定（.noi）
例:
```noi
kernel: N0roshi
deps: module1.nrs
exit_key: "q"
start: main
mem: 16M
level: debug
prompt: "> "
```
- `kernel`: 出力実行ファイル名。
- `deps`: 依存ファイル（最大16）。
- `exit_key`: プログラム終了キー。
- `start`、`mem`、`level`、`prompt`: 将来の使用のために予約。

### コンパイル出力
```
n0ryst ver. 1.09, 2024-2025
Starting compiling
Compiling dependency: examples/module1.nrs
[Parsing] 50%
[Parsing] 100%
[Type Checking] 0%
[Type Checking] 100%
[Codegen] 0%
[Codegen] 100%
Compiling main: examples/main.nrs
[Parsing] 50%
[Parsing] 100%
[Type Checking] 0%
[Type Checking] 100%
[Codegen] 0%
[Codegen] 100%
Compiled in 0.XX seconds
```

## Noroshi言語

### 構文

#### ブロック
```
/+[ <コマンド> /=]
```

#### 変数
```
let x = 42
```
- 型: `Int64`、文字列。
- スタックに保存。

#### 出力
```
pnt "Hello"
```
- FreeBSD/Linuxでは`printf`、macOSでは`_printf`で出力。

#### キーボードチェック
```
kbchk
```
- FreeBSD/Linuxでは`getchar`、macOSでは`_getchar`で入力。
- `exit_key`で終了。

#### 例
`main.nrs`:
```
/+[ let score = 0 pnt "Game started" kbchk /=]
```

### 制限
- ループ、条件、関数なし。
- 型: 数値、文字列、`=`シンボル。
- 制限: 1024トークン、2048 ASTノード、4096バイト出力。

## コンパイラ内部

### コンパイル段階
1. **.noiの読み込み**:
   - `kernel`、`deps`、`exit_key`を解析。
2. **.nrsの検索**:
   - `deps`に記載されていないメインファイル。
3. **字句解析**:
   - トークン: `TOKEN_OP_BLOCK_START`（`/+[`）、`TOKEN_STRING`、`TOKEN_EOF`など。
4. **構文解析**:
   - AST: `AST_BLOCK`、`AST_VARDECL`、`AST_PRINT`、`AST_KBCHK`、`AST_END`。
5. **コード生成**:
   - macOS用macho64またはFreeBSD/Linux用ELF64のアセンブリを生成。
   - メインファイル: macOSでは`_main`、FreeBSD/Linuxでは`main`。
   - 依存関係: `module_init`。
6. **アセンブリ**:
   - macOS: `nasm -f macho64`、`ld -lSystem`。
   - FreeBSD: `nasm -f elf64`、`ld.bfd -lc`。
   - Linux: `nasm -f elf64`、`ld -lc`。
7. **クリーンアップ**:
   - `.o`、`.asm`を削除。

### アセンブリ（macOS）
```nasm
section .data
msg db 'N0roshi running...', 10, 0
input_buf db 0
section .text
extern _getchar
extern _printf
global _main
kbhit:
  mov rax, 0x2000003
  mov rdi, 0
  syscall
  cmp rax, -1
  je .no_key
  mov byte [rel input_buf], al
  mov rax, 1
  ret
.no_key:
  xor rax, rax
  ret
_main:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  lea rdi, [rel msg]
  xor rax, rax
  call _printf
  call kbhit
  test rax, rax
  jz .no_input
  mov al, byte [rel input_buf]
  cmp al, 'q'
  je .exit
.no_input:
.exit:
  mov rsp, rbp
  pop rbp
  mov rax, 0x2000001
  mov rdi, 0
  syscall
```

### アセンブリ（FreeBSD/Linux）
```nasm
section .data
msg db 'N0roshi running...', 10, 0
input_buf db 0
section .text
extern getchar
extern printf
global main
kbhit:
  mov rax, 0
  mov rdi, 0
  mov rsi, input_buf
  mov rdx, 1
  syscall
  cmp rax, 0
  je .no_key
  mov rax, 1
  ret
.no_key:
  xor rax, rax
  ret
main:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  lea rdi, [msg]
  xor rax, rax
  call printf
  call kbhit
  test rax, rax
  jz .no_input
  mov al, [input_buf]
  cmp al, 'q'
  je .exit
.no_input:
.exit:
  mov rsp, rbp
  pop rbp
  mov rax, 60
  xor rdi, rdi
  syscall
```

## クロスコンパイル

### macOS
- **コマンド**:
  ```bash
  ./n0ryst --target macos examples
  ```
- **リンカ**: `ld -lSystem -syslibroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk`。

### FreeBSD
- **コマンド**:
  ```bash
  ./n0ryst --target freebsd examples
  ```
- **リンカ**: `ld.bfd -lc`。
- **ライブラリ**: `/usr/lib`（FreeBSD `libc`）。

### Linux
- **コマンド**:
  ```bash
  ./n0ryst --target linux examples
  ```
- **リンカ**: `ld -lc`。
- **ライブラリ**: `/usr/lib`（GNU `glibc`）。

## 例

### シンプルなプログラム
`main.nrs`:
```
/+[ pnt "Welcome" kbchk /=]
```
`config.noi`:
```noi
kernel: Welcome
exit_key: "q"
```
```bash
./n0ryst --target macos .
./Welcome
```
出力: `N0roshi running...`

### 変数付き
`main.nrs`:
```
/+[ let x = 10 pnt "Score set" kbchk /=]
```
- `x = 10`を設定。
- `Score set`を出力。

### 依存関係付き
`module1.nrs`:
```
/+[ pnt "Module loaded" /=]
```
`main.nrs`:
```
/+[ pnt "Main started" kbchk /=]
```
`config.noi`:
```noi
kernel: Game
deps: module1.nrs
exit_key: "q"
```

## デバッグ

### コンパイルエラー
- **字句**:
  ```
  Lexing error at position X, character 'Y'
  ```
  - `.nrs`の位置`X`を確認。
- **構文**:
  ```
  Parsing error: expected block start
  ```
  - コードが`/+[`で始まり、`/=]`で終わることを確認。
- **ファイル**:
  ```
  Error: No main .nrs file found
  ```
  - `deps`に記載されていない`.nrs`を追加。

### ビルドエラー
- **macOS**:
  ```
  ld: library 'System' not found
  ```
  - SDKを確認: `/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk`。
- **FreeBSD**:
  ```
  ld: cannot find -lc
  ```
  - `libc`をインストール: `sudo pkg install lib32`。
- **Linux**:
  ```
  ld: cannot find -lc
  ```
  - `libc6-dev`をインストール: `sudo apt install libc6-dev`。

### 実行時エラー
- 出力なし:
  - コードに`pnt`があるか確認。
- `q`で終了しない:
  - `.noi`の`exit_key`を確認。

## 改善案
1. **構文**:
   - ループ（`while`）、条件（`if`）、関数を追加。
   - `Float`、配列をサポート。
2. **コンパイラ**:
   - ASTを最適化。
   - 行番号付きの詳細なエラーメッセージ。
3. **クロスコンパイル**:
   - プラットフォームの自動検出。
4. **ログ**:
   - `.noi`の`level`、`prompt`を使用。
   - 正確なコンパイル時間。

## リソース

- **ビルド**:
  ```bash
  make
  ./n0ryst --target macos examples
  ./N0roshi
  ```
