/**
 * tric.c — Compilador C → Ternario para Tritos
 *
 * Compila un subconjunto de C a bytecode ternario ejecutable por la VM.
 *
 * ISA Ternaria:
 *   000 = NOP
 *   00+ = PUSH (inmediato)
 *   0+0 = POP
 *   0++ = ADD
 *   +00 = SUB
 *   +0+ = MUL
 *   ++0 = DIV
 *   +++ = MOD
 *   -00 = LOAD (variable)
 *   -0+ = STORE (variable)
 *   --0 = PRINT
 *   --- = PRINTT (ternario)
 *   0-0 = JMP
 *   0-- = JZ (jump if zero)
 *   0-+ = JNZ (jump if not zero)
 *   +-- = CMP (comparar)
 *   --- = HALT
 *
 * Sintaxis C soportada:
 *   int x = 5;
 *   x = x + 3;
 *   print(x);
 *   while (x > 0) { x = x - 1; }
 *   if (x == 0) { print(0); }
 */

#include "../include/ternary.h"

// =============================================================================
// TOKENIZER
// =============================================================================

typedef enum {
    TOK_INT, TOK_ID, TOK_NUM, TOK_STR,
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_MOD,
    TOK_ASSIGN, TOK_EQ, TOK_NEQ, TOK_LT, TOK_GT, TOK_LTE, TOK_GTE,
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE, TOK_SEMICOLON, TOK_COMMA,
    TOK_IF, TOK_ELSE, TOK_WHILE, TOK_FOR, TOK_RETURN, TOK_PRINT, TOK_PRINTT,
    TOK_EOF, TOK_ERROR
} token_type_t;

typedef struct {
    token_type_t type;
    char value[64];
    int line;
} token_t;

static const char* src_ptr;
static int src_line;

static void tokenizer_init(const char* src) {
    src_ptr = src;
    src_line = 1;
}

static token_t next_token(void) {
    token_t tok;
    tok.line = src_line;
    tok.value[0] = 0;

    // Skip whitespace
    while (*src_ptr == ' ' || *src_ptr == '\t' || *src_ptr == '\n') {
        if (*src_ptr == '\n') src_line++;
        src_ptr++;
    }

    if (*src_ptr == 0) { tok.type = TOK_EOF; return tok; }

    // Operators
    if (*src_ptr == '+') { tok.type = TOK_PLUS; src_ptr++; return tok; }
    if (*src_ptr == '-') { tok.type = TOK_MINUS; src_ptr++; return tok; }
    if (*src_ptr == '*') { tok.type = TOK_STAR; src_ptr++; return tok; }
    if (*src_ptr == '/') { tok.type = TOK_SLASH; src_ptr++; return tok; }
    if (*src_ptr == '%') { tok.type = TOK_MOD; src_ptr++; return tok; }
    if (*src_ptr == '(') { tok.type = TOK_LPAREN; src_ptr++; return tok; }
    if (*src_ptr == ')') { tok.type = TOK_RPAREN; src_ptr++; return tok; }
    if (*src_ptr == '{') { tok.type = TOK_LBRACE; src_ptr++; return tok; }
    if (*src_ptr == '}') { tok.type = TOK_RBRACE; src_ptr++; return tok; }
    if (*src_ptr == ';') { tok.type = TOK_SEMICOLON; src_ptr++; return tok; }
    if (*src_ptr == ',') { tok.type = TOK_COMMA; src_ptr++; return tok; }

    if (*src_ptr == '=') {
        src_ptr++;
        if (*src_ptr == '=') { tok.type = TOK_EQ; src_ptr++; }
        else { tok.type = TOK_ASSIGN; }
        return tok;
    }
    if (*src_ptr == '!') {
        src_ptr++;
        if (*src_ptr == '=') { tok.type = TOK_NEQ; src_ptr++; }
        else { tok.type = TOK_ERROR; }
        return tok;
    }
    if (*src_ptr == '<') {
        src_ptr++;
        if (*src_ptr == '=') { tok.type = TOK_LTE; src_ptr++; }
        else { tok.type = TOK_LT; }
        return tok;
    }
    if (*src_ptr == '>') {
        src_ptr++;
        if (*src_ptr == '=') { tok.type = TOK_GTE; src_ptr++; }
        else { tok.type = TOK_GT; }
        return tok;
    }

    // Number
    if (*src_ptr >= '0' && *src_ptr <= '9') {
        int i = 0;
        while (*src_ptr >= '0' && *src_ptr <= '9' && i < 63) {
            tok.value[i++] = *src_ptr++;
        }
        tok.value[i] = 0;
        tok.type = TOK_NUM;
        return tok;
    }

    // String
    if (*src_ptr == '"') {
        src_ptr++;
        int i = 0;
        while (*src_ptr != '"' && *src_ptr && i < 63) {
            tok.value[i++] = *src_ptr++;
        }
        if (*src_ptr == '"') src_ptr++;
        tok.value[i] = 0;
        tok.type = TOK_STR;
        return tok;
    }

    // Identifier or keyword
    if ((*src_ptr >= 'a' && *src_ptr <= 'z') || (*src_ptr >= 'A' && *src_ptr <= 'Z') || *src_ptr == '_') {
        int i = 0;
        while ((*src_ptr >= 'a' && *src_ptr <= 'z') || (*src_ptr >= 'A' && *src_ptr <= 'Z') ||
               (*src_ptr >= '0' && *src_ptr <= '9') || *src_ptr == '_') {
            tok.value[i++] = *src_ptr++;
        }
        tok.value[i] = 0;

        // Keywords
        if (strcmp_t(tok.value, "int") == 0) { tok.type = TOK_INT; return tok; }
        if (strcmp_t(tok.value, "if") == 0) { tok.type = TOK_IF; return tok; }
        if (strcmp_t(tok.value, "else") == 0) { tok.type = TOK_ELSE; return tok; }
        if (strcmp_t(tok.value, "while") == 0) { tok.type = TOK_WHILE; return tok; }
        if (strcmp_t(tok.value, "for") == 0) { tok.type = TOK_FOR; return tok; }
        if (strcmp_t(tok.value, "return") == 0) { tok.type = TOK_RETURN; return tok; }
        if (strcmp_t(tok.value, "print") == 0) { tok.type = TOK_PRINT; return tok; }
        if (strcmp_t(tok.value, "printt") == 0) { tok.type = TOK_PRINTT; return tok; }

        tok.type = TOK_ID;
        return tok;
    }

    tok.type = TOK_ERROR;
    src_ptr++;
    return tok;
}

// =============================================================================
// BYTECODE GENERATOR
// =============================================================================

#define MAX_CODE 4096
#define MAX_VARS 64
#define MAX_JUMPS 128

// Opcodes
#define OP_NOP    0
#define OP_PUSH   1
#define OP_POP    2
#define OP_ADD    3
#define OP_SUB    4
#define OP_MUL    5
#define OP_DIV    6
#define OP_MOD    7
#define OP_LOAD   8
#define OP_STORE  9
#define OP_PRINT  10
#define OP_PRINTT 11
#define OP_JMP    12
#define OP_JZ     13
#define OP_JNZ    14
#define OP_CMP    15
#define OP_HALT   16
#define OP_EQ     17
#define OP_NEQ    18
#define OP_LT     19
#define OP_GT     20
#define OP_LTE    21
#define OP_GTE    22

static uint8_t bytecode[MAX_CODE];
static int code_pos = 0;

// Variables
static struct { char name[32]; int addr; } vars[MAX_VARS];
static int var_count = 0;
static int stack_top = 0;

// Forward patches
static int jumps[MAX_JUMPS];
static int jump_count = 0;

static void emit(uint8_t op) {
    if (code_pos < MAX_CODE) bytecode[code_pos++] = op;
}

static void emit32(uint32_t val) {
    emit(val & 0xFF);
    emit((val >> 8) & 0xFF);
    emit((val >> 16) & 0xFF);
    emit((val >> 24) & 0xFF);
}

static int find_var(const char* name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp_t(vars[i].name, name) == 0) return vars[i].addr;
    }
    return -1;
}

static int add_var(const char* name) {
    int addr = find_var(name);
    if (addr >= 0) return addr;
    if (var_count >= MAX_VARS) return -1;
    strcpy_t(vars[var_count].name, name);
    vars[var_count].addr = stack_top++;
    return vars[var_count].addr;
}

// =============================================================================
// PARSER (recursive descent)
// =============================================================================

static token_t current_token;

static void eat(token_type_t expected) {
    if (current_token.type == expected) {
        current_token = next_token();
    } else {
        vga_puts("  [tric] Syntax error at line ");
        { char nb[8]; num_to_str(current_token.line, nb); vga_puts(nb); }
        vga_puts("\n");
    }
}

static void parse_expression(void);

static void parse_primary(void) {
    if (current_token.type == TOK_NUM) {
        int val = 0;
        for (int i = 0; current_token.value[i]; i++) {
            val = val * 10 + (current_token.value[i] - '0');
        }
        emit(OP_PUSH);
        emit32((uint32_t)val);
        current_token = next_token();
    } else if (current_token.type == TOK_LPAREN) {
        eat(TOK_LPAREN);
        parse_expression();
        eat(TOK_RPAREN);
    } else if (current_token.type == TOK_ID) {
        char name[32];
        strcpy_t(name, current_token.value);
        eat(TOK_ID);
        int addr = find_var(name);
        if (addr >= 0) {
            emit(OP_LOAD);
            emit32((uint32_t)addr);
        } else {
            vga_puts("  [tric] Undefined variable: ");
            vga_puts(name);
            vga_puts("\n");
        }
    } else if (current_token.type == TOK_PRINT) {
        eat(TOK_PRINT);
        eat(TOK_LPAREN);
        parse_expression();
        eat(TOK_RPAREN);
        emit(OP_PRINT);
    } else if (current_token.type == TOK_PRINTT) {
        eat(TOK_PRINTT);
        eat(TOK_LPAREN);
        parse_expression();
        eat(TOK_RPAREN);
        emit(OP_PRINTT);
    } else {
        vga_puts("  [tric] Unexpected token\n");
        current_token = next_token();
    }
}

static void parse_binary(void) {
    parse_primary();

    while (current_token.type == TOK_PLUS || current_token.type == TOK_MINUS ||
           current_token.type == TOK_STAR || current_token.type == TOK_SLASH ||
           current_token.type == TOK_MOD || current_token.type == TOK_EQ ||
           current_token.type == TOK_NEQ || current_token.type == TOK_LT ||
           current_token.type == TOK_GT || current_token.type == TOK_LTE ||
           current_token.type == TOK_GTE) {

        token_type_t op = current_token.type;
        current_token = next_token();
        parse_primary();

        switch (op) {
            case TOK_PLUS:  emit(OP_ADD); break;
            case TOK_MINUS: emit(OP_SUB); break;
            case TOK_STAR:  emit(OP_MUL); break;
            case TOK_SLASH: emit(OP_DIV); break;
            case TOK_MOD:   emit(OP_MOD); break;
            case TOK_EQ:    emit(OP_EQ); break;
            case TOK_NEQ:   emit(OP_NEQ); break;
            case TOK_LT:    emit(OP_LT); break;
            case TOK_GT:    emit(OP_GT); break;
            case TOK_LTE:   emit(OP_LTE); break;
            case TOK_GTE:   emit(OP_GTE); break;
            default: break;
        }
    }
}

static void parse_expression(void) {
    parse_binary();
}

static void parse_statement(void);

static void parse_block(void) {
    eat(TOK_LBRACE);
    while (current_token.type != TOK_RBRACE && current_token.type != TOK_EOF) {
        parse_statement();
    }
    eat(TOK_RBRACE);
}

static void parse_statement(void) {
    if (current_token.type == TOK_INT) {
        // Variable declaration
        eat(TOK_INT);
        char name[32];
        strcpy_t(name, current_token.value);
        eat(TOK_ID);
        int addr = add_var(name);

        if (current_token.type == TOK_ASSIGN) {
            eat(TOK_ASSIGN);
            parse_expression();
            emit(OP_STORE);
            emit32((uint32_t)addr);
        }
        eat(TOK_SEMICOLON);

    } else if (current_token.type == TOK_ID) {
        // Assignment
        char name[32];
        strcpy_t(name, current_token.value);
        eat(TOK_ID);
        int addr = find_var(name);
        eat(TOK_ASSIGN);
        parse_expression();
        emit(OP_STORE);
        emit32((uint32_t)addr);
        eat(TOK_SEMICOLON);

    } else if (current_token.type == TOK_PRINT || current_token.type == TOK_PRINTT) {
        parse_primary();
        eat(TOK_SEMICOLON);

    } else if (current_token.type == TOK_IF) {
        eat(TOK_IF);
        eat(TOK_LPAREN);
        parse_expression();
        eat(TOK_RPAREN);

        // JZ to else/end
        emit(OP_JZ);
        int patch_addr = code_pos;
        emit32(0); // placeholder

        parse_block();

        // Patch jump target
        bytecode[patch_addr] = code_pos & 0xFF;
        bytecode[patch_addr + 1] = (code_pos >> 8) & 0xFF;
        bytecode[patch_addr + 2] = (code_pos >> 16) & 0xFF;
        bytecode[patch_addr + 3] = (code_pos >> 24) & 0xFF;

    } else if (current_token.type == TOK_WHILE) {
        eat(TOK_WHILE);
        eat(TOK_LPAREN);

        int loop_start = code_pos;
        parse_expression();
        eat(TOK_RPAREN);

        emit(OP_JZ);
        int patch_addr = code_pos;
        emit32(0); // placeholder

        parse_block();

        // Jump back to start
        emit(OP_JMP);
        emit32((uint32_t)loop_start);

        // Patch jump target
        bytecode[patch_addr] = code_pos & 0xFF;
        bytecode[patch_addr + 1] = (code_pos >> 8) & 0xFF;
        bytecode[patch_addr + 2] = (code_pos >> 16) & 0xFF;
        bytecode[patch_addr + 3] = (code_pos >> 24) & 0xFF;

    } else if (current_token.type == TOK_RETURN) {
        eat(TOK_RETURN);
        parse_expression();
        eat(TOK_SEMICOLON);

    } else {
        // Expression statement
        parse_expression();
        eat(TOK_SEMICOLON);
    }
}

static void parse_program(void) {
    while (current_token.type != TOK_EOF) {
        parse_statement();
    }
    emit(OP_HALT);
}

// =============================================================================
// COMPILER INTERFACE
// =============================================================================

int tric_compile(const char* source, uint8_t* output, int* size) {
    // Initialize
    code_pos = 0;
    var_count = 0;
    stack_top = 0;

    // Tokenize and parse
    tokenizer_init(source);
    current_token = next_token();
    parse_program();

    // Copy output
    for (int i = 0; i < code_pos; i++) {
        output[i] = bytecode[i];
    }
    *size = code_pos;

    return 0; // success
}

// =============================================================================
// EXAMPLE PROGRAMS
// =============================================================================

static const char* example_hello =
    "print(72); print(101); print(108); print(108); print(111); print(10);";

static const char* example_fib =
    "int a = 0; int b = 1; int i = 0; "
    "while (i < 10) { "
    "  print(a); "
    "  int t = a + b; "
    "  a = b; "
    "  b = t; "
    "  i = i + 1; "
    "}";

static const char* example_sum =
    "int sum = 0; int i = 1; "
    "while (i <= 100) { "
    "  sum = sum + i; "
    "  i = i + 1; "
    "} "
    "print(sum);";

static const char* example_countdown =
    "int n = 10; "
    "while (n > 0) { "
    "  print(n); "
    "  n = n - 1; "
    "} "
    "print(0);";

// Command: tric
void cmd_tric(const char* args) {
    if (args[0] == 0 || strcmp_t(args, "help") == 0) {
        vga_puts("\n  [Tritos Compiler v0.1]\n\n");
        vga_puts("  Compila C a bytecode ternario.\n\n");
        vga_puts("  Usage:\n");
        vga_puts("    tric <code>         Compilar y ejecutar\n");
        vga_puts("    tric example hello  Ejemplo: Hello World\n");
        vga_puts("    tric example fib    Ejemplo: Fibonacci\n");
        vga_puts("    tric example sum    Ejemplo: Suma 1-100\n");
        vga_puts("    tric example count  Ejemplo: Cuenta regresiva\n");
        vga_puts("    tric isa           Ver ISA ternaria\n\n");
        vga_puts("  Soportado:\n");
        vga_puts("    int x = 5;         Declaración\n");
        vga_puts("    x = x + 1;         Asignación\n");
        vga_puts("    print(x);          Imprimir\n");
        vga_puts("    while (x > 0) {}   Bucle\n");
        vga_puts("    if (x == 0) {}     Condicional\n\n");
        return;
    }

    if (strcmp_t(args, "isa") == 0) {
        vga_puts("\n  [ISA Ternaria]\n\n");
        vga_puts("  Opcode  Instrucción\n");
        vga_puts("  ------  -----------\n");
        vga_puts("  000     NOP\n");
        vga_puts("  00+     PUSH inmediato\n");
        vga_puts("  0+0     POP\n");
        vga_puts("  0++     ADD\n");
        vga_puts("  +00     SUB\n");
        vga_puts("  +0+     MUL\n");
        vga_puts("  ++0     DIV\n");
        vga_puts("  +++     MOD\n");
        vga_puts("  -00     LOAD variable\n");
        vga_puts("  -0+     STORE variable\n");
        vga_puts("  --0     PRINT\n");
        vga_puts("  ---     HALT\n");
        vga_puts("  0-0     JMP\n");
        vga_puts("  0--     JZ\n");
        vga_puts("  0-+     JNZ\n\n");
        return;
    }

    if (strncmp_t(args, "example", 7) == 0) {
        const char* ex = args + 8;
        const char* code = 0;

        if (strcmp_t(ex, "hello") == 0) code = example_hello;
        else if (strcmp_t(ex, "fib") == 0) code = example_fib;
        else if (strcmp_t(ex, "sum") == 0) code = example_sum;
        else if (strcmp_t(ex, "count") == 0) code = example_countdown;
        else {
            vga_puts("  Available: hello, fib, sum, count\n");
            return;
        }

        vga_puts("\n  [Compiling example]\n");
        vga_puts("  Source: ");
        vga_puts(ex);
        vga_puts("\n\n");

        uint8_t code_buf[MAX_CODE];
        int code_size = 0;

        if (tric_compile(code, code_buf, &code_size) == 0) {
            vga_puts("  Compiled: ");
            { char nb[8]; num_to_str(code_size, nb); vga_puts(nb); }
            vga_puts(" bytes\n\n");

            // Execute
            vga_puts("  Output:\n");
            extern int vm_execute(const uint8_t* code, uint32_t size);
            vm_execute(code_buf, code_size);
        }
        return;
    }

    // Compile and execute direct code
    vga_puts("\n  [Compiling]\n");

    uint8_t code_buf[MAX_CODE];
    int code_size = 0;

    if (tric_compile(args, code_buf, &code_size) == 0) {
        vga_puts("  Compiled: ");
        { char nb[8]; num_to_str(code_size, nb); vga_puts(nb); }
        vga_puts(" bytes\n\n");

        vga_puts("  Output:\n");
        extern int vm_execute(const uint8_t* code, uint32_t size);
        vm_execute(code_buf, code_size);
    } else {
        vga_puts("  Compilation failed.\n");
    }
    vga_puts("\n");
}
