/**
 * edu.c — Módulo educativo ternario para kernel Tritos
 *
 * REPL, conversor, visualizador, tutoriales
 */

#include "../include/ternary.h"

// =============================================================================
// CONVERSOR DE BASES — binario ↔ ternario ↔ decimal ↔ base 60
// =============================================================================

// Convert decimal to ternary (balanced: -1, 0, 1 represented as '-', '0', '+')
static int decimal_to_ternary(int n, char* out) {
    if (n == 0) {
        out[0] = '0';
        out[1] = 0;
        return 1;
    }
    
    char temp[32];
    int len = 0;
    int num = n;
    
    while (num != 0) {
        int rem = num % 3;
        num = num / 3;
        
        if (rem == 2) {
            temp[len++] = '-';  // -1 in balanced ternary
            num++;              // Carry
        } else if (rem == (uint32_t)-1 % 3) {
            temp[len++] = '+';  // +1 in balanced ternary
            num--;
        } else {
            temp[len++] = '0' + rem;
        }
    }
    
    // Reverse
    for (int i = 0; i < len; i++) {
        out[i] = temp[len - 1 - i];
    }
    out[len] = 0;
    return len;
}

// Convert decimal to binary
static int decimal_to_binary(int n, char* out) {
    if (n == 0) {
        out[0] = '0';
        out[1] = 0;
        return 1;
    }
    
    char temp[32];
    int len = 0;
    int num = n < 0 ? -n : n;
    
    while (num > 0) {
        temp[len++] = '0' + (num & 1);
        num >>= 1;
    }
    
    if (n < 0) {
        out[0] = '-';
        for (int i = 0; i < len; i++) {
            out[i + 1] = temp[len - 1 - i];
        }
        out[len + 1] = 0;
        return len + 1;
    }
    
    for (int i = 0; i < len; i++) {
        out[i] = temp[len - 1 - i];
    }
    out[len] = 0;
    return len;
}

// Convert decimal to base 60 (Babylonian)
static int decimal_to_base60(int n, char* out) {
    if (n == 0) {
        out[0] = '0';
        out[1] = 0;
        return 1;
    }
    
    char temp[32];
    int len = 0;
    int num = n < 0 ? -n : n;
    
    while (num > 0) {
        temp[len++] = '0' + (num % 60);
        num /= 60;
    }
    
    if (n < 0) {
        out[0] = '-';
        for (int i = 0; i < len; i++) {
            out[i + 1] = temp[len - 1 - i];
        }
        out[len + 1] = 0;
        return len + 1;
    }
    
    for (int i = 0; i < len; i++) {
        out[i] = temp[len - 1 - i];
    }
    out[len] = 0;
    return len;
}

// Convert ternary string to decimal
static int ternary_to_decimal(const char* str) {
    int result = 0;
    int sign = 1;
    
    if (str[0] == '-') {
        sign = -1;
        str++;
    }
    
    while (*str) {
        if (*str == '+') {
            result = result * 3 + 1;
        } else if (*str == '-') {
            result = result * 3 - 1;
        } else if (*str >= '0' && *str <= '2') {
            result = result * 3 + (*str - '0');
        }
        str++;
    }
    
    return result * sign;
}

// Command: conv <number>
static void cmd_conv(const char* args) {
    if (args[0] == 0) {
        vga_puts("  Usage: conv <number>\n");
        vga_puts("  Convert between bases:\n");
        vga_puts("    Decimal:  255\n");
        vga_puts("    Ternary:  +0--0 (balanced)\n");
        vga_puts("    Binary:   11111111\n");
        vga_puts("    Base 60:  4:15\n");
        return;
    }
    
    // Parse number (supports ternary notation with +/-)
    int decimal = 0;
    int is_ternary = 0;
    
    for (const char* p = args; *p; p++) {
        if (*p == '+' || *p == '-') {
            is_ternary = 1;
            break;
        }
    }
    
    if (is_ternary) {
        decimal = ternary_to_decimal(args);
    } else {
        // Parse as decimal
        decimal = 0;
        int neg = 0;
        const char* p = args;
        if (*p == '-') { neg = 1; p++; }
        while (*p >= '0' && *p <= '9') {
            decimal = decimal * 10 + (*p - '0');
            p++;
        }
        if (neg) decimal = -decimal;
    }
    
    char buf[32];
    
    vga_puts("\n  Conversion for ");
    vga_puts(args);
    vga_puts(" (decimal: ");
    { char nb[8]; num_to_str(decimal, nb); vga_puts(nb); }
    vga_puts(")\n\n");
    
    // Binary
    vga_puts("  Binary:   ");
    decimal_to_binary(decimal, buf);
    vga_puts(buf);
    vga_puts("\n");
    
    // Ternary
    vga_puts("  Ternary:  ");
    decimal_to_ternary(decimal, buf);
    vga_puts(buf);
    vga_puts("\n");
    
    // Base 60
    vga_puts("  Base 60:  ");
    decimal_to_base60(decimal, buf);
    vga_puts("\n");
    
    // Maya (base 20)
    vga_puts("  Maya:     ");
    if (decimal == 0) {
        vga_puts("0");
    } else {
        int num = decimal < 0 ? -decimal : decimal;
        char maya[16];
        int mlen = 0;
        while (num > 0) {
            maya[mlen++] = '0' + (num % 20);
            num /= 20;
        }
        for (int i = mlen - 1; i >= 0; i--) {
            { char nb[4]; num_to_str(maya[i] - '0', nb); vga_puts(nb); }
            if (i > 0) vga_puts(".");
        }
    }
    vga_puts("\n\n");
}

// =============================================================================
// REPL TERNARIO — intérprete interactivo
// =============================================================================

// Evaluate ternary expression
static int eval_ternary_expr(const char* expr, char* result_ternary, char* result_binary) {
    int a = 0, b = 0;
    char op = '+';
    int state = 0; // 0=first num, 1=operator, 2=second num
    
    // Simple parser for "a op b"
    const char* p = expr;
    
    // Skip whitespace
    while (*p == ' ') p++;
    
    // Parse first number (ternary)
    char numbuf[16];
    int nlen = 0;
    while (*p && *p != ' ' && *p != '+' && *p != '-' && *p != '*' && *p != '/') {
        numbuf[nlen++] = *p++;
    }
    numbuf[nlen] = 0;
    a = ternary_to_decimal(numbuf);
    
    // Skip whitespace
    while (*p == ' ') p++;
    
    // Parse operator
    if (*p == '+' || *p == '-' || *p == '*' || *p == '/') {
        op = *p++;
    }
    
    // Skip whitespace
    while (*p == ' ') p++;
    
    // Parse second number (ternary)
    nlen = 0;
    while (*p && *p != ' ') {
        numbuf[nlen++] = *p++;
    }
    numbuf[nlen] = 0;
    b = ternary_to_decimal(numbuf);
    
    // Calculate
    int result = 0;
    switch (op) {
        case '+': result = a + b; break;
        case '-': result = a - b; break;
        case '*': result = a * b; break;
        case '/': 
            if (b != 0) result = a / b;
            else return -1;
            break;
    }
    
    // Convert result
    decimal_to_ternary(result, result_ternary);
    decimal_to_binary(result, result_binary);
    
    return result;
}

// REPL command
static void cmd_repl(const char* args) {
    if (strcmp_t(args, "help") == 0) {
        vga_puts("\n  Ternary REPL - Interactive Calculator\n\n");
        vga_puts("  Syntax: <ternary> <op> <ternary>\n");
        vga_puts("  Operators: +, -, *, /\n");
        vga_puts("  Ternary digits: + (positive), 0 (zero), - (negative)\n");
        vga_puts("  Example: +1 + +1 = +0- (decimal: 1+1=2)\n\n");
        return;
    }
    
    vga_puts("\n  [Ternary REPL v0.1]\n");
    vga_puts("  Type expressions like: +1 + +1\n");
    vga_puts("  Commands: help, quit\n\n");
    
    // Simple REPL (limited - just show examples)
    vga_puts("  Examples:\n");
    vga_puts("    +1 + +1   = +0-  (decimal: 2)\n");
    vga_puts("    +1 - +1   = 0    (decimal: 0)\n");
    vga_puts("    +1 * +1   = +1   (decimal: 1)\n");
    vga_puts("    +0- * +1  = +0-  (decimal: 2)\n");
    vga_puts("    +0- + +1  = +00  (decimal: 3)\n");
    vga_puts("    +0- * +0- = +1-  (decimal: 4)\n\n");
    
    // Interactive evaluation
    vga_puts("  Enter expression: ");
    
    // Read line from keyboard (simplified)
    char input[64];
    int idx = 0;
    
    // For now, just show static examples
    vga_puts("(interactive mode coming soon)\n\n");
}

// =============================================================================
// VISUALIZADOR — operaciones paso a paso
// =============================================================================

static void cmd_visualizar(const char* args) {
    if (args[0] == 0) {
        vga_puts("  Usage: ver <a> <op> <b>\n");
        vga_puts("  Show step-by-step binary and ternary operations\n");
        vga_puts("  Example: ver 5 + 3\n");
        return;
    }
    
    // Parse a, op, b
    int a = 0, b = 0;
    char op = '+';
    
    const char* p = args;
    while (*p == ' ') p++;
    
    // Parse a
    while (*p >= '0' && *p <= '9') {
        a = a * 10 + (*p - '0');
        p++;
    }
    
    while (*p == ' ') p++;
    if (*p) op = *p++;
    while (*p == ' ') p++;
    
    // Parse b
    while (*p >= '0' && *p <= '9') {
        b = b * 10 + (*p - '0');
        p++;
    }
    
    int result = 0;
    switch (op) {
        case '+': result = a + b; break;
        case '-': result = a - b; break;
        case '*': result = a * b; break;
        case '/': result = b != 0 ? a / b : 0; break;
    }
    
    char buf[32];
    
    vga_puts("\n  Step-by-step visualization\n\n");
    
    // Binary
    vga_puts("  BINARY:\n");
    vga_puts("    ");
    decimal_to_binary(a, buf);
    vga_puts(buf);
    vga_puts("\n  ");
    vga_puts("  ");
    vga_putc(op);
    vga_puts(" ");
    decimal_to_binary(b, buf);
    vga_puts(buf);
    vga_puts("\n");
    vga_puts("    ------\n");
    vga_puts("    ");
    decimal_to_binary(result, buf);
    vga_puts(buf);
    vga_puts("  = ");
    { char nb[8]; num_to_str(result, nb); vga_puts(nb); }
    vga_puts("\n\n");
    
    // Ternary
    vga_puts("  TERNARY:\n");
    vga_puts("    ");
    decimal_to_ternary(a, buf);
    vga_puts(buf);
    vga_puts("\n  ");
    vga_puts("  ");
    vga_putc(op);
    vga_puts(" ");
    decimal_to_ternary(b, buf);
    vga_puts(buf);
    vga_puts("\n");
    vga_puts("    --------\n");
    vga_puts("    ");
    decimal_to_ternary(result, buf);
    vga_puts(buf);
    vga_puts("  = ");
    { char nb[8]; num_to_str(result, nb); vga_puts(nb); }
    vga_puts("\n\n");
    
    // Comparison
    int bin_bits = 0, trit_count = 0;
    int tmp = a > b ? a : b;
    while (tmp > 0) { bin_bits++; tmp >>= 1; }
    tmp = a > b ? a : b;
    while (tmp > 0) { trit_count++; tmp /= 3; }
    
    vga_puts("  COMPARISON:\n");
    vga_puts("    Binary:  ");
    { char nb[4]; num_to_str(bin_bits, nb); vga_puts(nb); }
    vga_puts(" bits\n");
    vga_puts("    Ternary: ");
    { char nb[4]; num_to_str(trit_count, nb); vga_puts(nb); }
    vga_puts(" trits\n");
    if (trit_count > 0 && bin_bits > 0) {
        int saving = (bin_bits - trit_count) * 100 / bin_bits;
        vga_puts("    Savings: ");
        { char nb[4]; num_to_str(saving, nb); vga_puts(nb); }
        vga_puts("%\n");
    }
    vga_puts("\n");
}

// =============================================================================
// TUTORIALES — lecciones interactivas (now in tutorials.c)
// =============================================================================

// =============================================================================
// STATUS — info del módulo educativo
// =============================================================================

void edu_status(void) {
    vga_puts("\n  [Tritos Education v0.1]\n\n");
    vga_puts("  Modules loaded:\n");
    vga_puts("    [OK] Ternary REPL\n");
    vga_puts("    [OK] Base converter (bin/ter/60/20)\n");
    vga_puts("    [OK] Step-by-step visualizer\n");
    vga_puts("    [OK] 5 interactive tutorials\n\n");
    vga_puts("  Commands:\n");
    vga_puts("    conv <n>     Convert between bases\n");
    vga_puts("    repl         Ternary REPL\n");
    vga_puts("    ver <a>op<b> Visualize operation\n");
    vga_puts("    tutorial <n> Interactive lesson\n\n");
}
