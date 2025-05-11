# N0ryst Compiler

N0ryst is a lightweight, cross-platform compiler for the Noroshi language, designed with a minimalist philosophy inspired by Japanese precision. It supports macOS, FreeBSD, Linux, Windows, iOS, and Android, generating platform-specific assembly (macho64 for macOS/iOS, ELF64 for FreeBSD/Linux/Android, PE32+ for Windows) and linking executables using native tools like `ld`, `lld`, or `link.exe`. N0ryst is ideal for hobbyists and developers seeking a simple, extensible compiler for small projects.

## Features

- **Cross-Platform Support**: Compile Noroshi code for macOS, FreeBSD, Linux, Windows, iOS, and Android with a single `--target` flag.
- **Minimalist Design**: Clean syntax, small footprint, and efficient code generation.
- **Platform-Specific Assembly**: Generates macho64, ELF64, or PE32+ tailored to each platform's ABI and system calls.
- **Dependency Management**: Supports up to 16 dependencies via `.noi` configuration.
- **Extensible**: Easy to extend with new Noroshi language features or platform support.
- **Open Source**: Licensed under Boost Software License 1.0 (BSL 1.0).

## License

Licensed under the Boost Software License 1.0 (BSL 1.0). See `LICENSE` for details.

## Installation

Ensure you have the necessary tools for your platform before building N0ryst.

### macOS
```bash
xcode-select --install
sudo port install nasm
make
```

### FreeBSD
```bash
sudo pkg install nasm binutils gcc
gmake
```

### Linux
```bash
sudo apt update
sudo apt install nasm binutils gcc
make
```

### Windows
- Install MSYS2: `pacman -S mingw-w64-x86_64-nasm mingw-w64-x86_64-gcc`
- Add MSVC `link.exe` (Visual Studio Build Tools) to PATH.
```bash
mingw32-make
```

### iOS
- Install Xcode and iOS SDK.
- Configure `xcodebuild` for linking.
```bash
make
```

### Android
- Install Android NDK: `sudo apt install android-ndk` (Linux) or equivalent.
- Use NDK's `lld` linker.
```bash
make
```

## Usage

```bash
n0ryst [options] [path]
```

- `path`: Directory containing `.nrs` (Noroshi code) and `.noi` (configuration) files.
- Options:
  - `--help`: Display help message.
  - `--version`: Show version (1.09, 2024-2025).
  - `--target <platform>`: Specify target platform (`macos`, `freebsd`, `linux`, `windows`, `ios`, `android`).

### Example
Compile for Linux:
```bash
n0ryst --target linux .
```
Run the output:
```bash
./N0roshi
```

## Project Structure

A typical N0ryst project includes:
- `.nrs` files: Noroshi source code (main and dependencies).
- `.noi` file: Configuration for kernel name, dependencies, and runtime settings.

Example structure:
```
project/
├── main.nrs        # Main Noroshi program
├── module1.nrs     # Dependency module
└── config.noi      # Project configuration
```

## Configuration (.noi)

The `.noi` file defines project settings. Example:
```noi
kernel: N0roshi
deps: module1.nrs
exit_key: "q"
start: main
mem: 16M
level: debug
prompt: "> "
```

- `kernel`: Name of the output executable.
- `deps`: Comma-separated list of dependency `.nrs` files (max 16).
- `exit_key`: Key to exit the program (default: `q`).
- `start`, `mem`, `level`, `prompt`: Reserved for future features (e.g., entry point, memory allocation, logging, UI prompt).

## Compilation Process

N0ryst processes `.nrs` files through lexing, parsing, type checking, and code generation, producing platform-specific assembly. The output is linked into an executable.

### Compilation Output
Example log for a project with one dependency:
```
n0ryst ver. 1.09, 2024-2025
Starting compilation
Compiling dependency: module1.nrs
[Parsing] 50%
[Parsing] 100%
[Type Checking] 0%
[Type Checking] 100%
[Codegen] 0%
[Codegen] 100%
Compiling main file: main.nrs
[Parsing] 50%
[Parsing] 100%
[Type Checking] 0%
[Type Checking] 100%
[Codegen] 0%
[Codegen] 100%
Compiled in 0.XX seconds
```

The resulting executable (e.g., `N0roshi`) outputs `N0roshi running...` and exits on the `exit_key`.

## Noroshi Language

Noroshi is a minimalist language designed for simplicity and clarity, inspired by Japanese aesthetics.

### Syntax

#### Blocks
Encapsulate code in blocks:
```
/+[ <commands> /=]
```

#### Variables
Declare and assign 64-bit integers or strings:
```
let x = 42
let name = "Shinobi"
```
- Stored on the stack.
- Supports `Int64` and strings.

#### Output
Print to console:
```
pnt "Hello, Noroshi!"
```
- Uses `printf` (FreeBSD/Linux/Windows/Android) or `_printf` (macOS/iOS).

#### Keyboard Check
Check for keyboard input:
```
kbchk
```
- Reads via `getchar` (FreeBSD/Linux/Windows/Android) or `_getchar` (macOS/iOS).
- Exits if the input matches `exit_key`.

#### Example Program
`main.nrs`:
```
/+[ 
  let score = 0 
  pnt "Game started" 
  kbchk 
/=]
```
Output: `Game started`, exits on `q`.

### Limitations
- No loops, conditionals, or functions (planned for future versions).
- Supported tokens: numbers, strings, `=`, `/+[`, `/=]`.
- Limits: 1024 tokens, 2048 AST nodes, 4096-byte assembly output.

## Platform Details

N0ryst adapts to each platform's architecture and system calls.

### macOS
- Assembly: macho64
- Syscalls: Mach (`0x2000003` for `read`, `0x2000001` for `exit`)
- Functions: `_printf`, `_getchar`
- Linker: `ld -lSystem -syslibroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk`
```bash
n0ryst --target macos .
```

### FreeBSD
- Assembly: ELF64
- Syscalls: `read: 3`, `exit: 1`
- Functions: `printf`, `getchar`
- Linker: `ld.bfd -lc`
```bash
n0ryst --target freebsd .
```

### Linux
- Assembly: ELF64
- Syscalls: `read: 0`, `exit: 60`
- Functions: `printf`, `getchar`
- Linker: `ld -lc`
```bash
n0ryst --target linux .
```

### Windows
- Assembly: PE32+
- Functions: `printf`, `getchar` (msvcrt), `ExitProcess` (kernel32)
- Linker: `link /out:<kernel>.exe msvcrt.lib kernel32.lib`
```bash
n0ryst --target windows .
```

### iOS
- Assembly: macho64
- Syscalls: Mach (`0x2000003` for `read`, `0x2000001` for `exit`)
- Functions: `_printf`, `_getchar`
- Linker: `ld -lSystem -syslibroot /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk`
```bash
n0ryst --target ios .
```

### Android
- Assembly: ELF64
- Syscalls: `read: 0`, `exit: 60`
- Functions: `printf`, `getchar`
- Linker: `ld -lc` (NDK's `lld`)
```bash
n0ryst --target android .
```

## Extending N0ryst

To add new features:
1. **Language Features**: Extend `lexer` and `parser` in `n0ryst.c` for new tokens (e.g., loops, conditionals).
2. **Platform Support**: Add new `Platform` enum values and update `codegen` for new syscalls/ABIs.
3. **Dependencies**: Modify `read_noi` to support additional `.noi` fields.
4. **Optimization**: Enhance `codegen` for smaller/faster assembly output.

Contribute via GitHub pull requests.

## Troubleshooting

### Compilation Errors
- **Lexical**: `Lexing error at position X, character 'Y'`
  - Check `.nrs` syntax at position `X`.
- **Parsing**: `Parsing error: expected block start`
  - Verify `/+[` and `/=]` pairs.
- **File Not Found**: `Error: No main .nrs file found`
  - Ensure a `.nrs` file exists outside `deps` in `.noi`.

### Build Errors
- **macOS/iOS**: `ld: library 'System' not found`
  - Verify Xcode SDK path.
- **FreeBSD**: `ld: cannot find -lc`
  - Install `lib32`: `sudo pkg install lib32`.
- **Linux**: `ld: cannot find -lc`
  - Install `libc6-dev`: `sudo apt install libc6-dev`.
- **Windows**: `LINK : fatal error`
  - Ensure MSVC `link.exe` and `msvcrt.lib` are in PATH.
- **Android**: Linker errors
  - Verify NDK path and `lld` configuration.

### Runtime Errors
- **No Output**: Check for `pnt` commands in `.nrs`.
- **No Exit on Key**: Verify `exit_key` in `.noi`.
- **Crashes**:
  - Unix: `gdb ./N0roshi`, `run`
  - Windows: `gdb N0roshi.exe`, `run`
  - iOS/Android: Use Xcode/ADB logs.

## Community

- Report issues or suggest features on GitHub.
- Join discussions on forums like XDA Developers for cross-platform tips.
- Share your Noroshi projects with the community!

---

# ノーリスト・コンパイラ

ノーリストは、ノロシ言語のための軽量かつクロスプラットフォームのコンパイラで、日本の精密さにインスパイアされたミニマリスト哲学に基づいて設計されています。macOS、FreeBSD、Linux、Windows、iOS、Androidをサポートし、プラットフォーム固有のアセンブリ（macOS/iOS用macho64、FreeBSD/Linux/Android用ELF64、Windows用PE32+）を生成し、`ld`、`lld`、または`link.exe`などのネイティブツールで実行ファイルをリンクします。ホビーストや小型プロジェクトを求める開発者に最適です。

## 特徴

- **クロスプラットフォーム対応**：単一の`--target`フラグでmacOS、FreeBSD、Linux、Windows、iOS、Android向けにノロシコードをコンパイル。
- **ミニマリスト設計**：クリーンな構文、軽量なフットプリント、効率的なコード生成。
- **プラットフォーム固有のアセンブリ**：各プラットフォームのABIおよびシステムコールに合わせたmacho64、ELF64、またはPE32+を生成。
- **依存関係管理**：`.noi`設定ファイルで最大16の依存関係をサポート。
- **拡張可能**：新しいノロシ言語機能やプラットフォーム対応を簡単に追加可能。
- **オープンソース**：Boost Software License 1.0 (BSL 1.0)の下でライセンス。

## ライセンス

Boost Software License 1.0 (BSL 1.0)の下でライセンスされています。詳細は`LICENSE`を参照してください。

## インストール

ビルド前にプラットフォームに必要なツールを準備してください。

### macOS
```bash
xcode-select --install
sudo port install nasm
make
```

### FreeBSD
```bash
sudo pkg install nasm binutils gcc
gmake
```

### Linux
```bash
sudo apt update
sudo apt install nasm binutils gcc
make
```

### Windows
- MSYS2をインストール：`pacman -S mingw-w64-x86_64-nasm mingw-w64-x86_64-gcc`
- MSVCの`link.exe`（Visual Studio Build Tools）をPATHに追加。
```bash
mingw32-make
```

### iOS
- XcodeとiOS SDKをインストール。
- リンクに`xcodebuild`を設定。
```bash
make
```

### Android
- Android NDKをインストール：`sudo apt install android-ndk`（Linux）または同等。
- NDKの`lld`リンカを使用。
```bash
make
```

## 使用方法

```bash
n0ryst [オプション] [パス]
```

- `path`：`.nrs`（ノロシコード）と`.noi`（設定）ファイルを含むディレクトリ。
- オプション：
  - `--help`：ヘルプメッセージを表示。
  - `--version`：バージョン（1.09, 2024-2025）を表示。
  - `--target <プラットフォーム>`：対象プラットフォーム（`macos`, `freebsd`, `linux`, `windows`, `ios`, `android`）を指定。

### 例
Linux向けにコンパイル：
```bash
n0ryst --target linux .
```
出力を実行：
```bash
./N0roshi
```

## プロジェクト構造

典型的なノーリストプロジェクトは以下を含みます：
- `.nrs`ファイル：ノロシのソースコード（メインおよび依存関係）。
- `.noi`ファイル：カーネル名、依存関係、ランタイム設定の構成。

例の構造：
```
project/
├── main.nrs        # メインプログラム
├── module1.nrs     # 依存モジュール
└── config.noi      # プロジェクト設定
```

## 設定ファイル (.noi)

`.noi`ファイルはプロジェクト設定を定義します。例：
```noi
kernel: N0roshi
deps: module1.nrs
exit_key: "q"
start: main
mem: 16M
level: debug
prompt: "> "
```

- `kernel`：出力実行ファイル名。
- `deps`：依存`.nrs`ファイルのコンマ区切りリスト（最大16）。
- `exit_key`：プログラム終了キー（デフォルト：`q`）。
- `start`, `mem`, `level`, `prompt`：将来の機能（例：エントリーポイント、メモリ割り当て、ログ、UIプロンプト）のために予約。

## コンパイルプロセス

ノーリストは`.nrs`ファイルを字句解析、構文解析、型チェック、コード生成を通じて処理し、プラットフォーム固有のアセンブリを生成します。出力は実行ファイルにリンクされます。

### コンパイル出力
依存関係が1つのプロジェクトのログ例：
```
n0ryst ver. 1.09, 2024-2025
Starting compilation
Compiling dependency: module1.nrs
[Parsing] 50%
[Parsing] 100%
[Type Checking] 0%
[Type Checking] 100%
[Codegen] 0%
[Codegen] 100%
Compiling main file: main.nrs
[Parsing] 50%
[Parsing] 100%
[Type Checking] 0%
[Type Checking] 100%
[Codegen] 0%
[Codegen] 100%
Compiled in 0.XX seconds
```

生成された実行ファイル（例：`N0roshi`）は`N0roshi running...`を出力し、`exit_key`で終了します。

## ノロシ言語

ノロシは、シンプルさと明瞭さを追求したミニマリスト言語で、日本の美学にインスパイアされています。

### 構文

#### ブロック
コードをブロックで囲みます：
```
/+[ <コマンド> /=]
```

#### 変数
64ビット整数または文字列を宣言および割り当て：
```
let x = 42
let name = "Shinobi"
```
- スタックに格納。
- `Int64`および文字列をサポート。

#### 出力
コンソールに出力：
```
pnt "Hello, Noroshi!"
```
- FreeBSD/Linux/Windows/Androidでは`printf`、macOS/iOSでは`_printf`を使用。

#### キーボードチェック
キーボード入力をチェック：
```
kbchk
```
- FreeBSD/Linux/Windows/Androidでは`getchar`、macOS/iOSでは`_getchar`で読み取り。
- 入力が`exit_key`と一致する場合に終了。

#### プログラム例
`main.nrs`:
```
/+[ 
  let score = 0 
  pnt "Game started" 
  kbchk 
/=]
```
出力：`Game started`、`q`で終了。

### 制限
- ループ、条件分岐、関数は未サポート（将来のバージョンで予定）。
- サポートされるトークン：数値、文字列、`=`、`/+[`, `/=]`。
- 制限：1024トークン、2048 ASTノード、4096バイトのアセンブリ出力。

## プラットフォーム詳細

ノーリストは各プラットフォームのアーキテクチャとシステムコールに適応します。

### macOS
- アセンブリ：macho64
- システムコール：Mach（`0x2000003`で`read`、`0x2000001`で`exit`）
- 関数：`_printf`, `_getchar`
- リンカ：`ld -lSystem -syslibroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk`
```bash
n0ryst --target macos .
```

### FreeBSD
- アセンブリ：ELF64
- システムコール：`read: 3`, `exit: 1`
- 関数：`printf`, `getchar`
- リンカ：`ld.bfd -lc`
```bash
n0ryst --target freebsd .
```

### Linux
- アセンブリ：ELF64
- システムコール：`read: 0`, `exit: 60`
- 関数：`printf`, `getchar`
- リンカ：`ld -lc`
```bash
n0ryst --target linux .
```

### Windows
- アセンブリ：PE32+
- 関数：`printf`, `getchar`（msvcrt）、`ExitProcess`（kernel32）
- リンカ：`link /out:<kernel>.exe msvcrt.lib kernel32.lib`
```bash
n0ryst --target windows .
```

### iOS
- アセンブリ：macho64
- システムコール：Mach（`0x2000003`で`read`、`0x2000001`で`exit`）
- 関数：`_printf`, `_getchar`
- リンカ：`ld -lSystem -syslibroot /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk`
```bash
n0ryst --target ios .
```

### Android
- アセンブリ：ELF64
- システムコール：`read: 0`, `exit: 60`
- 関数：`printf`, `getchar`
- リンカ：`ld -lc`（NDKの`lld`）
```bash
n0ryst --target android .
```

## ノーリストの拡張

新機能を追加するには：
1. **言語機能**：`n0ryst.c`の`lexer`および`parser`を拡張して新しいトークン（例：ループ、条件分岐）をサポート。
2. **プラットフォーム対応**：新しい`Platform`列挙値を追加し、`codegen`を新しいシステムコール/ABI用に更新。
3. **依存関係**：`read_noi`を変更して追加の`.noi`フィールドをサポート。
4. **最適化**：`codegen`を強化してより小さく/高速なアセンブリ出力を生成。

GitHubのプルリクエストで貢献してください。

## トラブルシューティング

### コンパイルエラー
- **字句解析**：`Lexing error at position X, character 'Y'`
  - `.nrs`の構文を位置`X`で確認。
- **構文解析**：`Parsing error: expected block start`
  - `/+[`と`/=]`のペアを確認。
- **ファイル未検出**：`Error: No main .nrs file found`
  - `.noi`の`deps`に含まれていない`.nrs`ファイルが存在することを確認。

### ビルドエラー
- **macOS/iOS**：`ld: library 'System' not found`
  - Xcode SDKパスを確認。
- **FreeBSD**：`ld: cannot find -lc`
  - `lib32`をインストール：`sudo pkg install lib32`。
- **Linux**：`ld: cannot find -lc`
  - `libc6-dev`をインストール：`sudo apt install libc6-dev`。
- **Windows**：`LINK : fatal error`
  - MSVCの`link.exe`と`msvcrt.lib`がPATHにあることを確認。
- **Android**：リンカエラー
  - NDKパスと`lld`設定を確認。

### 実行時エラー
- **出力なし**：`.nrs`に`pnt`コマンドを確認。
- **キーで終了しない**：`.noi`の`exit_key`を確認。
- **クラッシュ**：
  - Unix：`gdb ./N0roshi`, `run`
  - Windows：`gdb N0roshi.exe`, `run`
  - iOS/Android：Xcode/ADBログを使用。

## コミュニティ

- GitHubで問題を報告または新機能を提案。
- XDA Developersなどのフォーラムでクロスプラットフォームのヒントを議論。
- ノロシプロジェクトをコミュニティと共有！
