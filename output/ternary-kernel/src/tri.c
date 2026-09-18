/**
 * tri.c — Lenguaje Ternario + Editor + Intérprete para Tritos
 *
 * Lenguaje de programación ternario mínimo
 * Editor de código integrado en kernel
 * Intérprete de programas .tri
 */

#include "../include/ternary.h"

// =============================================================================
// TOKENIZER
// =============================================================================

#define MAX_TOKENS 256
#define MAX_VARS   32
#define MAX_LINE   128
#define MAX_CODE   4096

// Token types
typedef enum {
    TOK_NONE,
    TOK_NUMBER,      // 0, 1, 2, 10, 12...
    TOK_IDENT,       // variable names
    TOK_PLUS,        // +
    TOK_MINUS,       // -
    TOK_STAR,        // *
    TOK_SLASH,       // /
    TOK_ASSIGN,      // =
    TOK_LPAREN,      // (
    TOK_RPAREN,      // )
    TOK_LBRACE,      // {
    TOK_RBRACE,      // }
    TOK_SEMICOL,     // ;
    TOK_COMMA,       // ,
    TOK_PRINT,       // print
    TOK_VAR,         // var
    TOK_IF,          // if
    TOK_ELSE,        // else
    TOK_WHILE,       // while
    TOK_FOR,         // for
    TOK_FUNC,        // func
    TOK_RETURN,      // return
    TOK_TRUE,        // true (1)
    TOK_FALSE,       // false (0)
    TOK_EOF,
    TOK_ERROR
} token_type_t;

typedef struct {
    token_type_t type;
    char value[32];
    int int_value;
} token_t;

typedef struct {
    const char* src;
    int pos;
    int len;
    token_t tokens[MAX_TOKENS];
    int token_count;
} tokenizer_t;

// Check if char is identifier start
static int is_ident_start(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

// Check if char is identifier part
static int is_ident_part(char c) {
    return is_ident_start(c) || (c >= '0' && c <= '9');
}

// Tokenize source code
static int tokenize(tokenizer_t* tz) {
    tz->token_count = 0;
    
    while (tz->pos < tz->len) {
        char c = tz->src[tz->pos];
        
        // Skip whitespace
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            tz->pos++;
            continue;
        }
        
        // Skip comments
        if (c == '/' && tz->pos + 1 < tz->len && tz->src[tz->pos + 1] == '/') {
            while (tz->pos < tz->len && tz->src[tz->pos] != '\n') tz->pos++;
            continue;
        }
        
        token_t* tok = &tz->tokens[tz->token_count];
        
        // Numbers
        if (c >= '0' && c <= '9') {
            tok->type = TOK_NUMBER;
            int i = 0;
            while (tz->pos < tz->len && tz->src[tz->pos] >= '0' && tz->src[tz->pos] <= '9') {
                tok->value[i++] = tz->src[tz->pos++];
            }
            tok->value[i] = 0;
            tok->int_value = 0;
            for (int j = 0; j < i; j++) {
                tok->int_value = tok->int_value * 10 + (tok->value[j] - '0');
            }
            tz->token_count++;
            continue;
        }
        
        // Identifiers and keywords
        if (is_ident_start(c)) {
            int i = 0;
            while (tz->pos < tz->len && is_ident_part(tz->src[tz->pos])) {
                tok->value[i++] = tz->src[tz->pos++];
            }
            tok->value[i] = 0;
            
            // Check keywords
            if (strcmp_t(tok->value, "print") == 0) tok->type = TOK_PRINT;
            else if (strcmp_t(tok->value, "var") == 0) tok->type = TOK_VAR;
            else if (strcmp_t(tok->value, "if") == 0) tok->type = TOK_IF;
            else if (strcmp_t(tok->value, "else") == 0) tok->type = TOK_ELSE;
            else if (strcmp_t(tok->value, "while") == 0) tok->type = TOK_WHILE;
            else if (strcmp_t(tok->value, "for") == 0) tok->type = TOK_FOR;
            else if (strcmp_t(tok->value, "func") == 0) tok->type = TOK_FUNC;
            else if (strcmp_t(tok->value, "return") == 0) tok->type = TOK_RETURN;
            else if (strcmp_t(tok->value, "true") == 0) { tok->type = TOK_TRUE; tok->int_value = 1; }
            else if (strcmp_t(tok->value, "false") == 0) { tok->type = TOK_FALSE; tok->int_value = 0; }
            else tok->type = TOK_IDENT;
            
            tz->token_count++;
            continue;
        }
        
        // Symbols
        switch (c) {
            case '+': tok->type = TOK_PLUS; break;
            case '-': tok->type = TOK_MINUS; break;
            case '*': tok->type = TOK_STAR; break;
            case '/': tok->type = TOK_SLASH; break;
            case '=': tok->type = TOK_ASSIGN; break;
            case '(': tok->type = TOK_LPAREN; break;
            case ')': tok->type = TOK_RPAREN; break;
            case '{': tok->type = TOK_LBRACE; break;
            case '}': tok->type = TOK_RBRACE; break;
            case ';': tok->type = TOK_SEMICOL; break;
            case ',': tok->type = TOK_COMMA; break;
            default: tok->type = TOK_ERROR; break;
        }
        tz->pos++;
        tz->token_count++;
    }
    
    // Add EOF
    tz->tokens[tz->token_count].type = TOK_EOF;
    tz->token_count++;
    
    return tz->token_count;
}

// =============================================================================
// INTERPRETER
// =============================================================================

// Variable storage
typedef struct {
    char name[32];
    int value;
    int is_array;
    int array_size;
    int* array_data;
} var_t;

typedef struct {
    var_t vars[MAX_VARS];
    int var_count;
    token_t* tokens;
    int pos;
    char output[2048];
    int output_len;
} interpreter_t;

// Find or create variable
static var_t* interp_find_var(interpreter_t* interp, const char* name) {
    for (int i = 0; i < interp->var_count; i++) {
        if (strcmp_t(interp->vars[i].name, name) == 0) {
            return &interp->vars[i];
        }
    }
    // Create new
    if (interp->var_count < MAX_VARS) {
        var_t* v = &interp->vars[interp->var_count++];
        strcpy_t(v->name, name);
        v->value = 0;
        v->is_array = 0;
        return v;
    }
    return 0;
}

// Get current token
static token_t* interp_current(interpreter_t* interp) {
    return &interp->tokens[interp->pos];
}

// Advance token
static void interp_advance(interpreter_t* interp) {
    interp->pos++;
}

// Expect token type
static int interp_expect(interpreter_t* interp, token_type_t type) {
    if (interp_current(interp)->type != type) return -1;
    interp_advance(interp);
    return 0;
}

// Print to output
static void interp_print(interpreter_t* interp, const char* str) {
    while (*str && interp->output_len < 2041) {
        interp->output[interp->output_len++] = *str++;
    }
    interp->output[interp->output_len] = 0;
}

static void interp_print_char(interpreter_t* interp, char c) {
    if (interp->output_len < 2041) {
        interp->output[interp->output_len++] = c;
        interp->output[interp->output_len] = 0;
    }
}

// Convert decimal to ternary
static void interp_dec_to_ternary(int n, char* out) {
    if (n == 0) {
        out[0] = '0';
        out[1] = 0;
        return;
    }
    
    char temp[32];
    int len = 0;
    int num = n < 0 ? -n : n;
    
    while (num > 0) {
        int rem = num % 3;
        num = num / 3;
        if (rem == 2) {
            temp[len++] = '-';
            num++;
        } else {
            temp[len++] = '0' + rem;
        }
    }
    
    if (n < 0) {
        out[0] = '-';
        for (int i = 0; i < len; i++) out[i + 1] = temp[len - 1 - i];
        out[len + 1] = 0;
    } else {
        for (int i = 0; i < len; i++) out[i] = temp[len - 1 - i];
        out[len] = 0;
    }
}

// Expression parser (recursive descent)
static int interp_expr(interpreter_t* interp);

// Parse primary expression
static int interp_primary(interpreter_t* interp) {
    token_t* tok = interp_current(interp);
    
    if (tok->type == TOK_NUMBER) {
        interp_advance(interp);
        return tok->int_value;
    }
    
    if (tok->type == TOK_TRUE || tok->type == TOK_FALSE) {
        interp_advance(interp);
        return tok->int_value;
    }
    
    if (tok->type == TOK_IDENT) {
        var_t* v = interp_find_var(interp, tok->value);
        interp_advance(interp);
        return v ? v->value : 0;
    }
    
    if (tok->type == TOK_LPAREN) {
        interp_advance(interp);
        int val = interp_expr(interp);
        interp_expect(interp, TOK_RPAREN);
        return val;
    }
    
    if (tok->type == TOK_MINUS) {
        interp_advance(interp);
        return -interp_primary(interp);
    }
    
    return 0;
}

// Parse multiplicative expression
static int interp_mul_expr(interpreter_t* interp) {
    int left = interp_primary(interp);
    
    while (1) {
        token_t* tok = interp_current(interp);
        if (tok->type == TOK_STAR) {
            interp_advance(interp);
            left *= interp_primary(interp);
        } else if (tok->type == TOK_SLASH) {
            interp_advance(interp);
            int right = interp_primary(interp);
            if (right != 0) left /= right;
        } else {
            break;
        }
    }
    
    return left;
}

// Parse expression
static int interp_expr(interpreter_t* interp) {
    int left = interp_mul_expr(interp);
    
    while (1) {
        token_t* tok = interp_current(interp);
        if (tok->type == TOK_PLUS) {
            interp_advance(interp);
            left += interp_mul_expr(interp);
        } else if (tok->type == TOK_MINUS) {
            interp_advance(interp);
            left -= interp_mul_expr(interp);
        } else {
            break;
        }
    }
    
    return left;
}

// Parse statement
static void interp_stmt(interpreter_t* interp);

// Parse block
static void interp_block(interpreter_t* interp) {
    interp_expect(interp, TOK_LBRACE);
    while (interp_current(interp)->type != TOK_RBRACE && 
           interp_current(interp)->type != TOK_EOF) {
        interp_stmt(interp);
    }
    interp_expect(interp, TOK_RBRACE);
}

// Parse statement
static void interp_stmt(interpreter_t* interp) {
    token_t* tok = interp_current(interp);
    
    // Variable declaration
    if (tok->type == TOK_VAR) {
        interp_advance(interp);
        token_t* name = interp_current(interp);
        if (name->type == TOK_IDENT) {
            var_t* v = interp_find_var(interp, name->value);
            interp_advance(interp);
            
            if (interp_current(interp)->type == TOK_ASSIGN) {
                interp_advance(interp);
                v->value = interp_expr(interp);
            }
            interp_expect(interp, TOK_SEMICOL);
        }
        return;
    }
    
    // Print statement
    if (tok->type == TOK_PRINT) {
        interp_advance(interp);
        interp_expect(interp, TOK_LPAREN);
        
        // Print expression (as number and ternary)
        int val = interp_expr(interp);
        
        char buf[32];
        interp_dec_to_ternary(val, buf);
        
        interp_print(interp, "  ");
        { char nb[8]; num_to_str(val, nb); interp_print(interp, nb); }
        interp_print(interp, " (ternary: ");
        interp_print(interp, buf);
        interp_print(interp, ")\n");
        
        interp_expect(interp, TOK_RPAREN);
        interp_expect(interp, TOK_SEMICOL);
        return;
    }
    
    // Print ternary (printT)
    if (tok->type == TOK_IDENT && strcmp_t(tok->value, "printT") == 0) {
        interp_advance(interp);
        interp_expect(interp, TOK_LPAREN);
        
        int val = interp_expr(interp);
        char buf[32];
        interp_dec_to_ternary(val, buf);
        interp_print(interp, buf);
        
        interp_expect(interp, TOK_RPAREN);
        interp_expect(interp, TOK_SEMICOL);
        return;
    }
    
    // If statement
    if (tok->type == TOK_IF) {
        interp_advance(interp);
        interp_expect(interp, TOK_LPAREN);
        int cond = interp_expr(interp);
        interp_expect(interp, TOK_RPAREN);
        
        if (cond) {
            interp_block(interp);
        } else {
            // Skip block
            int depth = 0;
            while (interp_current(interp)->type != TOK_EOF) {
                if (interp_current(interp)->type == TOK_LBRACE) depth++;
                if (interp_current(interp)->type == TOK_RBRACE) {
                    if (depth == 0) break;
                    depth--;
                }
                interp_advance(interp);
            }
            interp_expect(interp, TOK_RBRACE);
            
            // Check for else
            if (interp_current(interp)->type == TOK_ELSE) {
                interp_advance(interp);
                interp_block(interp);
            }
        }
        return;
    }
    
    // While loop
    if (tok->type == TOK_WHILE) {
        interp_advance(interp);
        interp_expect(interp, TOK_LPAREN);
        
        int loop_start = interp->pos;
        
        while (1) {
            interp->pos = loop_start;
            int cond = interp_expr(interp);
            interp_expect(interp, TOK_RPAREN);
            
            if (!cond) {
                // Skip block
                int depth = 0;
                while (interp_current(interp)->type != TOK_EOF) {
                    if (interp_current(interp)->type == TOK_LBRACE) depth++;
                    if (interp_current(interp)->type == TOK_RBRACE) {
                        if (depth == 0) break;
                        depth--;
                    }
                    interp_advance(interp);
                }
                interp_expect(interp, TOK_RBRACE);
                break;
            }
            
            interp_block(interp);
        }
        return;
    }
    
    // For loop (simplified: for (var i = 0; i < n; i++))
    if (tok->type == TOK_FOR) {
        interp_advance(interp);
        interp_expect(interp, TOK_LPAREN);
        
        // Init
        if (interp_current(interp)->type == TOK_VAR) {
            interp_advance(interp);
            token_t* name = interp_current(interp);
            if (name->type == TOK_IDENT) {
                var_t* v = interp_find_var(interp, name->value);
                interp_advance(interp);
                interp_expect(interp, TOK_ASSIGN);
                v->value = interp_expr(interp);
                interp_expect(interp, TOK_SEMICOL);
            }
        }
        
        int loop_start = interp->pos;
        char cond_var[32] = "";
        int cond_op = 0; // 0=<, 1=>, 2==
        int cond_val = 0;
        
        // Condition
        if (interp_current(interp)->type == TOK_IDENT) {
            strcpy_t(cond_var, interp_current(interp)->value);
            interp_advance(interp);
            
            if (interp_current(interp)->type == TOK_PLUS) {
                cond_op = 1; // >
                interp_advance(interp);
            } else if (interp_current(interp)->type == TOK_ASSIGN) {
                cond_op = 2; // ==
                interp_advance(interp);
            }
            // Default is <
            
            cond_val = interp_expr(interp);
            interp_expect(interp, TOK_SEMICOL);
        }
        
        // Increment (skip for now)
        while (interp_current(interp)->type != TOK_RPAREN && 
               interp_current(interp)->type != TOK_EOF) {
            interp_advance(interp);
        }
        interp_expect(interp, TOK_RPAREN);
        
        int body_start = interp->pos;
        
        // Execute loop
        while (1) {
            interp->pos = loop_start;
            
            // Evaluate condition
            var_t* cv = interp_find_var(interp, cond_var);
            int cond = 0;
            switch (cond_op) {
                case 0: cond = cv->value < cond_val; break;  // <
                case 1: cond = cv->value > cond_val; break;  // >
                case 2: cond = cv->value == cond_val; break;  // ==
            }
            
            if (!cond) {
                // Skip block
                interp->pos = body_start;
                int depth = 0;
                while (interp_current(interp)->type != TOK_EOF) {
                    if (interp_current(interp)->type == TOK_LBRACE) depth++;
                    if (interp_current(interp)->type == TOK_RBRACE) {
                        if (depth == 0) break;
                        depth--;
                    }
                    interp_advance(interp);
                }
                interp_expect(interp, TOK_RBRACE);
                break;
            }
            
            // Execute body
            interp->pos = body_start;
            interp_block(interp);
            
            // Increment
            cv->value++;
        }
        return;
    }
    
    // Assignment
    if (tok->type == TOK_IDENT) {
        var_t* v = interp_find_var(interp, tok->value);
        interp_advance(interp);
        
        if (interp_current(interp)->type == TOK_ASSIGN) {
            interp_advance(interp);
            v->value = interp_expr(interp);
            interp_expect(interp, TOK_SEMICOL);
        }
        return;
    }
    
    // Skip unknown
    interp_advance(interp);
}

// Run ternary program
int tri_run(const char* source) {
    tokenizer_t tz;
    tz.src = source;
    tz.pos = 0;
    tz.len = strlen_t(source);
    
    tokenize(&tz);
    
    interpreter_t interp;
    interp.var_count = 0;
    interp.tokens = tz.tokens;
    interp.pos = 0;
    interp.output_len = 0;
    interp.output[0] = 0;
    
    while (interp_current(&interp)->type != TOK_EOF) {
        interp_stmt(&interp);
    }
    
    // Print output
    vga_puts(interp.output);
    
    return 0;
}

// =============================================================================
// EDITOR — editor de código ternario integrado
// =============================================================================

#define EDITOR_MAX_LINES 64
#define EDITOR_LINE_LEN  80

static char editor_lines[EDITOR_MAX_LINES][EDITOR_LINE_LEN];
static int editor_line_count = 0;
static int editor_cursor_x = 0;
static int editor_cursor_y = 0;
static int editor_modified = 0;
static char editor_filename[32] = "untitled.tri";

// Initialize editor
void editor_init(void) {
    editor_line_count = 1;
    editor_cursor_x = 0;
    editor_cursor_y = 0;
    editor_modified = 0;
    strcpy_t(editor_filename, "untitled.tri");
    
    for (int i = 0; i < EDITOR_MAX_LINES; i++) {
        editor_lines[i][0] = 0;
    }
}

// Insert character at cursor
void editor_insert_char(char c) {
    if (editor_line_count >= EDITOR_MAX_LINES) return;
    if (editor_cursor_y >= EDITOR_MAX_LINES) return;
    
    char* line = editor_lines[editor_cursor_y];
    int len = strlen_t(line);
    
    if (editor_cursor_x >= EDITOR_LINE_LEN - 1) return;
    
    // Shift right
    for (int i = len; i >= editor_cursor_x; i--) {
        line[i + 1] = line[i];
    }
    
    line[editor_cursor_x] = c;
    editor_cursor_x++;
    editor_modified = 1;
}

// Insert newline
void editor_insert_newline(void) {
    if (editor_line_count >= EDITOR_MAX_LINES) return;
    
    // Split line
    char* line = editor_lines[editor_cursor_y];
    char* rest = &line[editor_cursor_x];
    
    // Move rest to new line
    for (int i = editor_line_count; i > editor_cursor_y + 1; i--) {
        strcpy_t(editor_lines[i], editor_lines[i - 1]);
    }
    
    strcpy_t(editor_lines[editor_cursor_y + 1], rest);
    line[editor_cursor_x] = 0;
    
    editor_line_count++;
    editor_cursor_y++;
    editor_cursor_x = 0;
    editor_modified = 1;
}

// Delete character before cursor
void editor_backspace(void) {
    if (editor_cursor_x > 0) {
        char* line = editor_lines[editor_cursor_y];
        int len = strlen_t(line);
        
        for (int i = editor_cursor_x - 1; i < len; i++) {
            line[i] = line[i + 1];
        }
        
        editor_cursor_x--;
        editor_modified = 1;
    } else if (editor_cursor_y > 0) {
        // Merge with previous line
        char* prev = editor_lines[editor_cursor_y - 1];
        char* curr = editor_lines[editor_cursor_y];
        int prev_len = strlen_t(prev);
        
        strcpy_t(&prev[prev_len], curr);
        
        // Shift lines up
        for (int i = editor_cursor_y; i < editor_line_count - 1; i++) {
            strcpy_t(editor_lines[i], editor_lines[i + 1]);
        }
        
        editor_line_count--;
        editor_cursor_y--;
        editor_cursor_x = prev_len;
        editor_modified = 1;
    }
}

// Move cursor
void editor_move_cursor(int dx, int dy) {
    editor_cursor_x += dx;
    editor_cursor_y += dy;
    
    if (editor_cursor_x < 0) editor_cursor_x = 0;
    if (editor_cursor_y < 0) editor_cursor_y = 0;
    if (editor_cursor_y >= editor_line_count) editor_cursor_y = editor_line_count - 1;
    
    int len = strlen_t(editor_lines[editor_cursor_y]);
    if (editor_cursor_x > len) editor_cursor_x = len;
}

// Save file
void editor_save(void) {
    int fd = fs_create(editor_filename);
    if (fd < 0) {
        vga_puts("  Error: cannot create file\n");
        return;
    }
    
    int8_t f = fs_open(editor_filename, 1);
    if (f < 0) {
        vga_puts("  Error: cannot open file\n");
        return;
    }
    
    for (int i = 0; i < editor_line_count; i++) {
        fs_write(f, (uint8_t*)editor_lines[i], strlen_t(editor_lines[i]));
        fs_write(f, (uint8_t*)"\n", 1);
    }
    
    fs_close(f);
    editor_modified = 0;
    
    vga_puts("  Saved: ");
    vga_puts(editor_filename);
    vga_puts("\n");
}

// Load file
void editor_load(const char* filename) {
    strcpy_t(editor_filename, filename);
    
    int fd = fs_open(filename, 0);
    if (fd < 0) {
        vga_puts("  New file: ");
        vga_puts(filename);
        vga_puts("\n");
        editor_line_count = 1;
        editor_lines[0][0] = 0;
        return;
    }
    
    int32_t size = fs_get_size(filename);
    if (size <= 0) {
        fs_close(fd);
        return;
    }
    
    char buf[4096];
    int32_t read = fs_read(fd, (uint8_t*)buf, size < 4095 ? size : 4095);
    fs_close(fd);
    
    if (read <= 0) return;
    
    // Parse into lines
    editor_line_count = 0;
    int line_start = 0;
    
    for (int i = 0; i <= read; i++) {
        if (i == read || buf[i] == '\n') {
            if (editor_line_count < EDITOR_MAX_LINES) {
                int line_len = i - line_start;
                if (line_len >= EDITOR_LINE_LEN) line_len = EDITOR_LINE_LEN - 1;
                memcpy_t(editor_lines[editor_line_count], &buf[line_start], line_len);
                editor_lines[editor_line_count][line_len] = 0;
                editor_line_count++;
            }
            line_start = i + 1;
        }
    }
    
    if (editor_line_count == 0) {
        editor_line_count = 1;
        editor_lines[0][0] = 0;
    }
    
    editor_cursor_x = 0;
    editor_cursor_y = 0;
    editor_modified = 0;
    
    vga_puts("  Loaded: ");
    vga_puts(filename);
    vga_puts(" (");
    { char nb[4]; num_to_str(editor_line_count, nb); vga_puts(nb); }
    vga_puts(" lines)\n");
}

// Display editor
void editor_display(void) {
    vga_puts("\n  === Ternary Editor ===\n\n");
    
    int start = editor_cursor_y - 5;
    if (start < 0) start = 0;
    int end = start + 15;
    if (end > editor_line_count) end = editor_line_count;
    
    for (int i = start; i < end; i++) {
        // Line number
        vga_set_color(0x08, 0);
        { char nb[4]; num_to_str(i + 1, nb); vga_puts(nb); }
        vga_puts(": ");
        vga_set_color(0x07, 0);
        
        // Line content
        vga_puts(editor_lines[i]);
        
        // Cursor indicator
        if (i == editor_cursor_y) {
            vga_set_color(0x0A, 0);
            vga_puts(" <--");
            vga_set_color(0x07, 0);
        }
        vga_puts("\n");
    }
    
    // Status bar
    vga_puts("\n  [");
    vga_puts(editor_filename);
    if (editor_modified) vga_puts(" *");
    vga_puts("] Line ");
    { char nb[4]; num_to_str(editor_cursor_y + 1, nb); vga_puts(nb); }
    vga_puts(", Col ");
    { char nb[4]; num_to_str(editor_cursor_x + 1, nb); vga_puts(nb); }
    vga_puts("\n");
    vga_puts("  Commands: save, run, quit\n\n");
}

// Run current file
void editor_run(void) {
    // Concatenate all lines
    char code[4096];
    int pos = 0;
    
    for (int i = 0; i < editor_line_count; i++) {
        int len = strlen_t(editor_lines[i]);
        memcpy_t(&code[pos], editor_lines[i], len);
        pos += len;
        code[pos++] = '\n';
    }
    code[pos] = 0;
    
    vga_puts("\n  [Running ");
    vga_puts(editor_filename);
    vga_puts("]\n\n");
    
    tri_run(code);
}

// =============================================================================
// EDITOR COMMANDS — integrados al shell
// =============================================================================

// Editor state
static uint8_t editor_active = 0;

// Start editor
void tri_editor(const char* filename) {
    editor_init();
    
    if (filename && filename[0]) {
        editor_load(filename);
    }
    
    editor_active = 1;
    editor_display();
    
    vga_puts("  Editor mode: type code, 'save' to save, 'run' to execute, 'quit' to exit\n\n");
}

// Process editor command
int tri_editor_cmd(const char* cmd) {
    if (!editor_active) return -1;
    
    if (strcmp_t(cmd, "quit") == 0 || strcmp_t(cmd, "q") == 0) {
        editor_active = 0;
        vga_puts("  Editor closed\n");
        return 0;
    }
    
    if (strcmp_t(cmd, "save") == 0 || strcmp_t(cmd, "s") == 0) {
        editor_save();
        return 1;
    }
    
    if (strcmp_t(cmd, "run") == 0 || strcmp_t(cmd, "r") == 0) {
        editor_run();
        return 1;
    }
    
    if (strcmp_t(cmd, "list") == 0 || strcmp_t(cmd, "l") == 0) {
        editor_display();
        return 1;
    }
    
    // Insert text (simplified - one line at a time)
    if (editor_line_count < EDITOR_MAX_LINES) {
        strcpy_t(editor_lines[editor_line_count], cmd);
        editor_line_count++;
        editor_cursor_y = editor_line_count - 1;
        editor_cursor_x = strlen_t(cmd);
        editor_modified = 1;
    }
    
    return 1;
}

// =============================================================================
// SHELL COMMANDS
// =============================================================================

// Command: tri <file> — run ternary program
void cmd_tri(const char* args) {
    if (args[0] == 0) {
        vga_puts("  Usage:\n");
        vga_puts("    tri <file.tri>     - Run program\n");
        vga_puts("    tri edit <file>    - Edit program\n");
        vga_puts("    tri new <file>     - New program\n");
        vga_puts("    tri examples       - Show examples\n");
        return;
    }
    
    if (strncmp_t(args, "edit", 4) == 0) {
        tri_editor(args + 5);
        return;
    }
    
    if (strncmp_t(args, "new", 3) == 0) {
        tri_editor(args + 4);
        return;
    }
    
    if (strcmp_t(args, "examples") == 0) {
        vga_puts("\n  [Ternary Language Examples]\n\n");
        vga_puts("  // Hello world\n");
        vga_puts("  print(42);\n\n");
        vga_puts("  // Variables\n");
        vga_puts("  var x = 10;\n");
        vga_puts("  var y = 20;\n");
        vga_puts("  print(x + y);\n\n");
        vga_puts("  // For loop\n");
        vga_puts("  for (var i = 0; i < 5; i++) {\n");
        vga_puts("    print(i);\n");
        vga_puts("  }\n\n");
        vga_puts("  // Conditionals\n");
        vga_puts("  var a = 10;\n");
        vga_puts("  if (a > 5) {\n");
        vga_puts("    print(1);\n");
        vga_puts("  } else {\n");
        vga_puts("    print(0);\n");
        vga_puts("  }\n\n");
        return;
    }
    
    // Run file
    int fd = fs_open(args, 0);
    if (fd < 0) {
        vga_puts("  Error: file not found: ");
        vga_puts(args);
        vga_puts("\n");
        return;
    }
    
    int32_t size = fs_get_size(args);
    if (size <= 0) {
        vga_puts("  Error: empty file\n");
        fs_close(fd);
        return;
    }
    
    char buf[4096];
    int32_t read = fs_read(fd, (uint8_t*)buf, size < 4095 ? size : 4095);
    fs_close(fd);
    
    if (read <= 0) {
        vga_puts("  Error: cannot read file\n");
        return;
    }
    
    buf[read] = 0;
    
    vga_puts("\n  [Running ");
    vga_puts(args);
    vga_puts("]\n\n");
    
    tri_run(buf);
}

// Get editor state
uint8_t tri_editor_is_active(void) {
    return editor_active;
}
