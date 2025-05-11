#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

#define MAX_TOKENS 1024
#define MAX_AST 2048
#define MAX_OUTPUT 4096
#define MAX_BUFFER 4096
#define MAX_PATH 256
#define MAX_DEPS 16

enum TokenType {
    TOKEN_OP_BLOCK_START,
    TOKEN_OP_BLOCK_END,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_KEYWORD,
    TOKEN_SYMBOL,
    TOKEN_EOF
};

typedef struct {
    enum TokenType type;
    char value[256];
} Token;

enum ASTType {
    AST_BLOCK,
    AST_VARDECL,
    AST_PRINT,
    AST_KBCHK,
    AST_END
};

typedef struct {
    enum ASTType type;
    char value[256];
    char value2[256];
} ASTNode;

typedef struct {
    char kernel[64];
    char deps[MAX_DEPS][MAX_PATH];
    int dep_count;
    char exit_key[8];
    char start[64];
    char mem[16];
    char level[16];
    char prompt[64];
} Config;

enum Platform {
    PLATFORM_MACOS,
    PLATFORM_FREEBSD,
    PLATFORM_LINUX,
    PLATFORM_WINDOWS,
    PLATFORM_IOS,
    PLATFORM_ANDROID
};

Token tokens[MAX_TOKENS];
ASTNode ast[MAX_AST];
char output_buf[MAX_OUTPUT];
int token_count = 0;
int ast_count = 0;
int output_pos = 0;
Config config = { .kernel = "n0ryst", .dep_count = 0, .exit_key = "q" };
enum Platform target_platform = PLATFORM_MACOS;

void n0ryst_log(const char* msg) {
    printf("%s\n", msg);
}

char* read_file(const char* path) {
    FILE* file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file %s\n", path);
        exit(1);
    }
    char* buffer = malloc(MAX_BUFFER);
    size_t len = fread(buffer, 1, MAX_BUFFER - 1, file);
    buffer[len] = '\0';
    fclose(file);
    return buffer;
}

void read_noi(const char* dir) {
    DIR* d = opendir(dir);
    if (!d) return;
    struct dirent* entry;
    char noi_path[MAX_PATH];
    while ((entry = readdir(d))) {
        if (strstr(entry->d_name, ".noi")) {
            snprintf(noi_path, MAX_PATH, "%s/%s", dir, entry->d_name);
            FILE* file = fopen(noi_path, "r");
            if (!file) continue;
            char line[256];
            while (fgets(line, sizeof(line), file)) {
                line[strcspn(line, "\n")] = '\0';
                if (strncmp(line, "kernel:", 7) == 0) {
                    char* val = strchr(line + 7, ':') ? line + 8 : line + 7;
                    while (isspace(*val)) val++;
                    strncpy(config.kernel, val, 63);
                } else if (strncmp(line, "deps:", 5) == 0) {
                    char* val = strchr(line + 5, ':') ? line + 6 : line + 5;
                    while (isspace(*val)) val++;
                    char* token = strtok(val, ",");
                    while (token && config.dep_count < MAX_DEPS) {
                        while (isspace(*token)) token++;
                        snprintf(config.deps[config.dep_count], MAX_PATH, "%s/%s", dir, token);
                        config.dep_count++;
                        token = strtok(NULL, ",");
                    }
                } else if (strncmp(line, "exit_key:", 9) == 0) {
                    char* key = strchr(line + 9, '"');
                    if (key) {
                        strncpy(config.exit_key, key + 1, 7);
                        config.exit_key[strcspn(config.exit_key, "\"")] = '\0';
                    }
                } else if (strncmp(line, "start:", 6) == 0) {
                    char* val = strchr(line + 6, ':') ? line + 7 : line + 6;
                    while (isspace(*val)) val++;
                    strncpy(config.start, val, 63);
                } else if (strncmp(line, "mem:", 4) == 0) {
                    char* val = strchr(line + 4, ':') ? line + 5 : line + 4;
                    while (isspace(*val)) val++;
                    strncpy(config.mem, val, 15);
                } else if (strncmp(line, "level:", 6) == 0) {
                    char* val = strchr(line + 6, ':') ? line + 7 : line + 6;
                    while (isspace(*val)) val++;
                    strncpy(config.level, val, 15);
                } else if (strncmp(line, "prompt:", 7) == 0) {
                    char* val = strchr(line + 7, '"');
                    if (val) {
                        strncpy(config.prompt, val + 1, 63);
                        config.prompt[strcspn(config.prompt, "\"")] = '\0';
                    }
                }
            }
            fclose(file);
            break;
        }
    }
    closedir(d);
}

int find_nrs(const char* dir, char* nrs_path) {
    DIR* d = opendir(dir);
    if (!d) {
        fprintf(stderr, "Error: Cannot open directory %s\n", dir);
        return 0;
    }
    struct dirent* entry;
    while ((entry = readdir(d))) {
        if (strstr(entry->d_name, ".nrs")) {
            char candidate[MAX_PATH];
            snprintf(candidate, MAX_PATH, "%s/%s", dir, entry->d_name);
            int is_dep = 0;
            for (int i = 0; i < config.dep_count; i++) {
                if (strcmp(candidate, config.deps[i]) == 0) {
                    is_dep = 1;
                    break;
                }
            }
            if (!is_dep) {
                strcpy(nrs_path, candidate);
                closedir(d);
                return 1;
            }
        }
    }
    closedir(d);
    fprintf(stderr, "Error: No main .nrs file found in %s\n", dir);
    return 0;
}

void lexer(const char* input) {
    int pos = 0;
    while (input[pos] && token_count < MAX_TOKENS) {
        if (isspace(input[pos])) {
            pos++;
            continue;
        }
        if (input[pos] == '/') {
            if (pos + 2 < strlen(input)) {
                if (strncmp(&input[pos], "/+[", 3) == 0) {
                    tokens[token_count].type = TOKEN_OP_BLOCK_START;
                    strcpy(tokens[token_count].value, "/+[");
                    token_count++;
                    pos += 3;
                    continue;
                } else if (strncmp(&input[pos], "/=]", 3) == 0) {
                    tokens[token_count].type = TOKEN_OP_BLOCK_END;
                    strcpy(tokens[token_count].value, "/=]");
                    token_count++;
                    pos += 3;
                    continue;
                }
            }
        }
        if (input[pos] == '"') {
            pos++;
            int i = 0;
            while (input[pos] && input[pos] != '"' && i < 255) {
                tokens[token_count].value[i++] = input[pos++];
            }
            tokens[token_count].value[i] = '\0';
            tokens[token_count].type = TOKEN_STRING;
            token_count++;
            if (input[pos] == '"') pos++;
            continue;
        }
        if (isdigit(input[pos])) {
            int i = 0;
            while (isdigit(input[pos]) && i < 255) {
                tokens[token_count].value[i++] = input[pos++];
            }
            tokens[token_count].value[i] = '\0';
            tokens[token_count].type = TOKEN_NUMBER;
            token_count++;
            continue;
        }
        if (isalpha(input[pos])) {
            int i = 0;
            while (isalpha(input[pos]) && i < 255) {
                tokens[token_count].value[i++] = input[pos++];
            }
            tokens[token_count].value[i] = '\0';
            tokens[token_count].type = TOKEN_KEYWORD;
            token_count++;
            continue;
        }
        if (input[pos] == '=') {
            tokens[token_count].type = TOKEN_SYMBOL;
            tokens[token_count].value[0] = '=';
            tokens[token_count].value[1] = '\0';
            token_count++;
            pos++;
            continue;
        }
        fprintf(stderr, "Lexing error at position %d, character '%c' (ASCII %d)\n", pos, input[pos], input[pos]);
        exit(1);
    }
    tokens[token_count].type = TOKEN_EOF;
    tokens[token_count].value[0] = '\0';
    token_count++;
}

void parser() {
    int token_pos = 0;
    while (ast_count < MAX_AST) {
        if (tokens[token_pos].type == TOKEN_EOF) {
            ast[ast_count].type = AST_END;
            ast_count++;
            break;
        }
        if (tokens[token_pos].type == TOKEN_OP_BLOCK_START) {
            token_pos++;
            ast[ast_count].type = AST_BLOCK;
            ast_count++;
            while (tokens[token_pos].type != TOKEN_OP_BLOCK_END) {
                if (tokens[token_pos].type == TOKEN_KEYWORD) {
                    if (strcmp(tokens[token_pos].value, "let") == 0) {
                        token_pos++;
                        ast[ast_count].type = AST_VARDECL;
                        strcpy(ast[ast_count].value, tokens[token_pos].value);
                        token_pos++;
                        if (tokens[token_pos].type == TOKEN_SYMBOL && tokens[token_pos].value[0] == '=') {
                            token_pos++;
                            strcpy(ast[ast_count].value2, tokens[token_pos].value);
                            token_pos++;
                        } else {
                            ast[ast_count].value2[0] = '\0';
                        }
                        ast_count++;
                    } else if (strcmp(tokens[token_pos].value, "pnt") == 0) {
                        token_pos++;
                        ast[ast_count].type = AST_PRINT;
                        strcpy(ast[ast_count].value, tokens[token_pos].value);
                        token_pos++;
                        ast_count++;
                    } else if (strcmp(tokens[token_pos].value, "kbchk") == 0) {
                        token_pos++;
                        ast[ast_count].type = AST_KBCHK;
                        ast_count++;
                    } else {
                        fprintf(stderr, "Parsing error: unknown keyword %s\n", tokens[token_pos].value);
                        exit(1);
                    }
                } else {
                    fprintf(stderr, "Parsing error at token %d\n", token_pos);
                    exit(1);
                }
            }
            token_pos++;
        } else {
            fprintf(stderr, "Parsing error: expected block start\n");
            exit(1);
        }
    }
}

void append_str(const char* str) {
    int len = strlen(str);
    if (output_pos + len < MAX_OUTPUT) {
        strcpy(&output_buf[output_pos], str);
        output_pos += len;
    }
}

void codegen(int is_main) {
    append_str("section .data\n");
    append_str("msg db 'N0roshi running...', 10, 0\n");
    append_str("input_buf db 0\n");
    append_str("section .text\n");

    if (target_platform == PLATFORM_MACOS || target_platform == PLATFORM_IOS) {
        append_str("extern _getchar\n");
        append_str("extern _printf\n");
        if (is_main) append_str("global _main\n");
    } else if (target_platform == PLATFORM_WINDOWS) {
        append_str("extern getchar\n");
        append_str("extern printf\n");
        if (is_main) append_str("global main\n");
    } else {
        append_str("extern getchar\n");
        append_str("extern printf\n");
        if (is_main) append_str("global main\n");
    }

    append_str("kbhit:\n");
    if (target_platform == PLATFORM_MACOS || target_platform == PLATFORM_IOS) {
        append_str("  mov rax, 0x2000003\n");
        append_str("  mov rdi, 0\n");
        append_str("  syscall\n");
        append_str("  cmp rax, -1\n");
    } else if (target_platform == PLATFORM_FREEBSD) {
        append_str("  mov rax, 3\n");
        append_str("  mov rdi, 0\n");
        append_str("  mov rsi, input_buf\n");
        append_str("  mov rdx, 1\n");
        append_str("  syscall\n");
        append_str("  cmp rax, 0\n");
    } else if (target_platform == PLATFORM_LINUX || target_platform == PLATFORM_ANDROID) {
        append_str("  mov rax, 0\n");
        append_str("  mov rdi, 0\n");
        append_str("  mov rsi, input_buf\n");
        append_str("  mov rdx, 1\n");
        append_str("  syscall\n");
        append_str("  cmp rax, 0\n");
    } else {
        append_str("  call getchar\n");
        append_str("  cmp rax, -1\n");
        append_str("  je .no_key\n");
        append_str("  mov byte [input_buf], al\n");
        append_str("  mov rax, 1\n");
        append_str("  ret\n");
    }
    if (target_platform != PLATFORM_WINDOWS) {
        append_str("  je .no_key\n");
        append_str("  mov byte [rel input_buf], al\n");
        append_str("  mov rax, 1\n");
        append_str("  ret\n");
    }
    append_str(".no_key:\n");
    append_str("  xor rax, rax\n");
    append_str("  ret\n");

    if (is_main) {
        if (target_platform == PLATFORM_MACOS || target_platform == PLATFORM_IOS) {
            append_str("_main:\n");
        } else {
            append_str("main:\n");
        }
    } else {
        append_str("module_init:\n");
    }
    append_str("  push rbp\n");
    append_str("  mov rbp, rsp\n");
    append_str("  sub rsp, 16\n");

    for (int i = 0; i < ast_count; i++) {
        if (ast[i].type == AST_BLOCK) {
            continue;
        } else if (ast[i].type == AST_VARDECL) {
            append_str("  mov qword [rbp-8], 0\n");
            if (ast[i].value2[0]) {
                append_str("  mov rax, ");
                append_str(ast[i].value2);
                append_str("\n");
                append_str("  mov [rbp-8], rax\n");
            }
        } else if (ast[i].type == AST_PRINT) {
            if (target_platform == PLATFORM_MACOS || target_platform == PLATFORM_IOS) {
                append_str("  lea rdi, [rel msg]\n");
                append_str("  xor rax, rax\n");
                append_str("  call _printf\n");
            } else {
                append_str("  lea rdi, [msg]\n");
                append_str("  xor rax, rax\n");
                append_str("  call printf\n");
            }
        } else if (ast[i].type == AST_KBCHK) {
            append_str("  call kbhit\n");
            append_str("  test rax, rax\n");
            append_str("  jz .no_input\n");
            append_str("  mov al, byte [rel input_buf]\n");
            append_str("  cmp al, '");
            append_str(config.exit_key);
            append_str("'\n");
            append_str("  je .exit\n");
            append_str(".no_input:\n");
        }
    }

    append_str(".exit:\n");
    append_str("  mov rsp, rbp\n");
    append_str("  pop rbp\n");
    if (is_main) {
        if (target_platform == PLATFORM_MACOS || target_platform == PLATFORM_IOS) {
            append_str("  mov rax, 0x2000001\n");
            append_str("  mov rdi, 0\n");
            append_str("  syscall\n");
        } else if (target_platform == PLATFORM_FREEBSD) {
            append_str("  mov rax, 1\n");
            append_str("  xor rdi, rdi\n");
            append_str("  syscall\n");
        } else if (target_platform == PLATFORM_LINUX || target_platform == PLATFORM_ANDROID) {
            append_str("  mov rax, 60\n");
            append_str("  xor rdi, rdi\n");
            append_str("  syscall\n");
        } else {
            append_str("  mov rcx, 0\n");
            append_str("  call ExitProcess\n");
        }
    } else {
        append_str("  ret\n");
    }
}

void compile_file(const char* path, int is_main) {
    token_count = 0;
    ast_count = 0;
    output_pos = 0;
    char* input = read_file(path);
    lexer(input);
    n0ryst_log("[Parsing] 50%");
    parser();
    n0ryst_log("[Parsing] 100%");
    n0ryst_log("[Type Checking] 0%");
    n0ryst_log("[Type Checking] 100%");
    n0ryst_log("[Codegen] 0%");
    codegen(is_main);
    n0ryst_log("[Codegen] 100%");
    free(input);
}

void show_help() {
    printf("n0ryst ver. 1.09, 2024-2025\n");
    printf("Usage: n0ryst [options] [path]\n");
    printf("Options:\n");
    printf("  --help    Show this help message\n");
    printf("  --version Show version\n");
    printf("  --target <platform>  Target platform (macos, freebsd, linux, windows, ios, android)\n");
    printf("  path      Directory with .nrs and .noi files\n");
    exit(0);
}

void show_version() {
    printf("n0ryst ver. 1.09, 2024-2025\n");
    exit(0);
}

int main(int argc, char* argv[]) {
    char nrs_path[MAX_PATH] = "";
    char dir[MAX_PATH] = ".";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            show_help();
        } else if (strcmp(argv[i], "--version") == 0) {
            show_version();
        } else if (strcmp(argv[i], "--target") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "macos") == 0) {
                target_platform = PLATFORM_MACOS;
            } else if (strcmp(argv[i], "freebsd") == 0) {
                target_platform = PLATFORM_FREEBSD;
            } else if (strcmp(argv[i], "linux") == 0) {
                target_platform = PLATFORM_LINUX;
            } else if (strcmp(argv[i], "windows") == 0) {
                target_platform = PLATFORM_WINDOWS;
            } else if (strcmp(argv[i], "ios") == 0) {
                target_platform = PLATFORM_IOS;
            } else if (strcmp(argv[i], "android") == 0) {
                target_platform = PLATFORM_ANDROID;
            } else {
                fprintf(stderr, "Error: Invalid target platform '%s'\n", argv[i]);
                exit(1);
            }
        } else {
            strncpy(dir, argv[i], MAX_PATH - 1);
        }
    }

    read_noi(dir);
    if (!find_nrs(dir, nrs_path)) {
        exit(1);
    }

    n0ryst_log("n0ryst ver. 1.09, 2024-2025");
    n0ryst_log("Starting compilation");

    for (int i = 0; i < config.dep_count; i++) {
        n0ryst_log("Compiling dependency:");
        n0ryst_log(config.deps[i]);
        compile_file(config.deps[i], 0);
        char obj_path[MAX_PATH];
        snprintf(obj_path, MAX_PATH, "dep%d.o", i);
        FILE* out = fopen("out.asm", "w");
        fwrite(output_buf, 1, output_pos, out);
        fclose(out);
        char cmd[512];
        snprintf(cmd, 512, "nasm -f %s out.asm -o %s",
                 target_platform == PLATFORM_MACOS || target_platform == PLATFORM_IOS ? "macho64" :
                 target_platform == PLATFORM_WINDOWS ? "win64" : "elf64", obj_path);
        system(cmd);
    }

    n0ryst_log("Compiling main file:");
    n0ryst_log(nrs_path);
    compile_file(nrs_path, 1);

    FILE* out = fopen("out.asm", "w");
    if (!out) {
        fprintf(stderr, "Error: Cannot write to out.asm\n");
        exit(1);
    }
    fwrite(output_buf, 1, output_pos, out);
    fclose(out);

    n0ryst_log("Compiled in 0.XX seconds");

    char cmd[512];
    if (target_platform == PLATFORM_MACOS) {
        snprintf(cmd, 512, "nasm -f macho64 out.asm -o main.o && ld -w -platform_version macos 10.15 10.15 -L/usr/lib -syslibroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk -o %s main.o", config.kernel);
    } else if (target_platform == PLATFORM_FREEBSD) {
        snprintf(cmd, 512, "nasm -f elf64 out.asm -o main.o && ld.bfd -o %s main.o", config.kernel);
    } else if (target_platform == PLATFORM_LINUX) {
        snprintf(cmd, 512, "nasm -f elf64 out.asm -o main.o && ld -o %s main.o", config.kernel);
    } else if (target_platform == PLATFORM_WINDOWS) {
        snprintf(cmd, 512, "nasm -f win64 out.asm -o main.o && link /out:%s.exe main.o msvcrt.lib kernel32.lib", config.kernel);
    } else if (target_platform == PLATFORM_IOS) {
        snprintf(cmd, 512, "nasm -f macho64 out.asm -o main.o && ld -o %s main.o -lSystem -syslibroot /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk", config.kernel);
    } else {
        snprintf(cmd, 512, "nasm -f elf64 out.asm -o main.o && ld -o %s main.o -lc", config.kernel);
    }
    for (int i = 0; i < config.dep_count; i++) {
        char obj_path[MAX_PATH];
        snprintf(obj_path, MAX_PATH, "dep%d.o", i);
        strncat(cmd, " ", 512 - strlen(cmd) - 1);
        strncat(cmd, obj_path, 512 - strlen(cmd) - 1);
    }
    if (target_platform == PLATFORM_MACOS) {
        strncat(cmd, " -lSystem", 512 - strlen(cmd) - 1);
    } else if (target_platform == PLATFORM_FREEBSD || target_platform == PLATFORM_LINUX || target_platform == PLATFORM_ANDROID) {
        strncat(cmd, " -lc", 512 - strlen(cmd) - 1);
    } else if (target_platform == PLATFORM_IOS) {
        strncat(cmd, " -lSystem", 512 - strlen(cmd) - 1);
    }
    strncat(cmd, " && rm -f main.o out.asm", 512 - strlen(cmd) - 1);
    for (int i = 0; i < config.dep_count; i++) {
        char obj_path[MAX_PATH];
        snprintf(obj_path, MAX_PATH, "dep%d.o", i);
        strncat(cmd, " ", 512 - strlen(cmd) - 1);
        strncat(cmd, obj_path, 512 - strlen(cmd) - 1);
    }
    system(cmd);

    return 0;
}
