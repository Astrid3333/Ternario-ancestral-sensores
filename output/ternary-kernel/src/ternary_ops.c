/**
 * ternary_ops.c — Operaciones ternarias avanzadas para Tritos
 *
 * Aritmética ternaria completa, conversión, visualización Maya
 */

#include "../include/ternary.h"

// =============================================================================
// BALANCED TERNARY ARITHMETIC (variable length)
// =============================================================================

// Add two balanced ternary numbers
int ternary_add(const trit* a, const trit* b, trit* result, int max_len) {
    trit carry = 0;
    for (int i = 0; i < max_len; i++) {
        int sum = (int)a[i] + (int)b[i] + (int)carry;
        
        if (sum == -3) { result[i] = 0; carry = -1; }
        else if (sum == -2) { result[i] = 1; carry = -1; }
        else if (sum == 3) { result[i] = 0; carry = 1; }
        else if (sum == 2) { result[i] = -1; carry = 1; }
        else { result[i] = (trit)sum; carry = 0; }
    }
    return (int)carry;
}

// Subtract two balanced ternary numbers
void ternary_sub(const trit* a, const trit* b, trit* result, int max_len) {
    for (int i = 0; i < max_len; i++) {
        result[i] = trit_add(a[i], trit_neg(b[i]));
    }
}

// Multiply two balanced ternary numbers (schoolbook)
void ternary_mul(const trit* a, int a_len, const trit* b, int b_len, 
                 trit* result, int max_len) {
    // Clear result
    for (int i = 0; i < max_len; i++) result[i] = 0;
    
    // Schoolbook multiplication
    for (int i = 0; i < a_len && i < max_len; i++) {
        if (a[i] == 0) continue;
        
        trit carry = 0;
        for (int j = 0; j < b_len && (i + j) < max_len; j++) {
            int prod = (int)a[i] * (int)b[j] + (int)result[i + j] + (int)carry;
            
            if (prod == -3) { result[i + j] = 0; carry = -1; }
            else if (prod == -2) { result[i + j] = 1; carry = -1; }
            else if (prod == 3) { result[i + j] = 0; carry = 1; }
            else if (prod == 2) { result[i + j] = -1; carry = 1; }
            else { result[i + j] = (trit)prod; carry = 0; }
        }
        if (carry != 0 && (i + b_len) < max_len) {
            result[i + b_len] = carry;
        }
    }
}

// =============================================================================
// INTEGER ↔ BALANCED TERNARY CONVERSION
// =============================================================================

// Convert integer to balanced ternary
int int_to_ternary(int n, trit* trits, int max_len) {
    int neg = 0;
    int num = n;
    
    if (num < 0) { neg = 1; num = -num; }
    
    int len = 0;
    while (num > 0 && len < max_len) {
        int rem = num % 3;
        num = num / 3;
        
        if (rem == 2) {
            trits[len] = -1;  // -1
            num++;            // carry
        } else {
            trits[len] = (trit)rem;
        }
        len++;
    }
    
    if (len == 0) { trits[0] = 0; len = 1; }
    
    // Negate if negative
    if (neg) {
        for (int i = 0; i < len; i++) {
            trits[i] = trit_neg(trits[i]);
        }
    }
    
    return len;
}

// Convert balanced ternary to integer
int ternary_to_int(const trit* trits, int len) {
    int result = 0;
    int power = 1;
    
    for (int i = 0; i < len; i++) {
        result += (int)trits[i] * power;
        power *= 3;
    }
    
    return result;
}

// =============================================================================
// TERNARY STRING OPERATIONS
// =============================================================================

// Convert ternary string to trits
int ternary_str_to_trits(const char* str, trit* trits, int max_len) {
    int len = 0;
    int i = 0;
    
    // Skip leading zeros
    while (str[i] == '0') i++;
    
    // Parse
    while (str[i] && len < max_len) {
        if (str[i] == '+') trits[len] = 1;
        else if (str[i] == '-') trits[len] = -1;
        else if (str[i] == '0') trits[len] = 0;
        else break;
        len++;
        i++;
    }
    
    if (len == 0) { trits[0] = 0; len = 1; }
    return len;
}

// Convert trits to ternary string
void trits_to_ternary_str(const trit* trits, int len, char* str) {
    int i = 0;
    
    // Skip leading zeros (but keep if all zeros)
    while (i < len - 1 && trits[i] == 0) i++;
    
    int j = 0;
    for (; i < len; i++) {
        str[j++] = trit_to_char(trits[i]);
    }
    str[j] = 0;
}

// =============================================================================
// BASE-60 (MAYA) OPERATIONS
// =============================================================================

// Convert integer to base-60
int int_to_base60(int n, int* digits, int max_digits) {
    int len = 0;
    int num = n;
    
    if (num == 0) {
        digits[0] = 0;
        return 1;
    }
    
    while (num > 0 && len < max_digits) {
        digits[len++] = num % 60;
        num /= 60;
    }
    
    return len;
}

// Convert base-60 to integer
int base60_to_int(const int* digits, int len) {
    int result = 0;
    int power = 1;
    
    for (int i = 0; i < len; i++) {
        result += digits[i] * power;
        power *= 60;
    }
    
    return result;
}

// Maya digit to string (0-19: dots and bar)
void maya_digit_to_str(int digit, char* str) {
    if (digit == 0) {
        str[0] = 'O';  // Shell shape for zero
        str[1] = 0;
        return;
    }
    
    int bars = digit / 5;
    int dots = digit % 5;
    
    int i = 0;
    for (int b = 0; b < bars; b++) str[i++] = '=';  // Bar
    for (int d = 0; d < dots; d++) str[i++] = '.';   // Dot
    str[i] = 0;
}

// =============================================================================
// TERNARY DISPLAY COMMANDS
// =============================================================================

// Command: trinary — advanced ternary operations
void cmd_trinary(const char* args) {
    if (args[0] == 0) {
        vga_puts("\n  [Ternary Operations]\n\n");
        vga_puts("  Commands:\n");
        vga_puts("    trinary add <a> <b>     Add two numbers\n");
        vga_puts("    trinary sub <a> <b>     Subtract\n");
        vga_puts("    trinary mul <a> <b>     Multiply\n");
        vga_puts("    trinary conv <n>        Integer → ternary\n");
        vga_puts("    trinary maya <n>        Integer → Maya (base 60)\n");
        vga_puts("    trinary info            About ternary\n\n");
        return;
    }
    
    const char* p = args;
    
    // Parse first argument
    char cmd1[16] = {0};
    int i = 0;
    while (*p && *p != ' ' && i < 15) cmd1[i++] = *p++;
    while (*p == ' ') p++;
    
    if (strcmp_t(cmd1, "add") == 0 || strcmp_t(cmd1, "sub") == 0 || 
        strcmp_t(cmd1, "mul") == 0) {
        // Parse a and b
        int a = 0, b = 0;
        int neg = 0;
        
        while (*p == ' ') p++;
        if (*p == '-') { neg = 1; p++; }
        while (*p >= '0' && *p <= '9') { a = a * 10 + (*p - '0'); p++; }
        if (neg) a = -a;
        
        while (*p == ' ') p++;
        neg = 0;
        if (*p == '-') { neg = 1; p++; }
        while (*p >= '0' && *p <= '9') { b = b * 10 + (*p - '0'); p++; }
        if (neg) b = -b;
        
        // Convert to trits
        trit at[16], bt[16], rt[16];
        int al = int_to_ternary(a, at, 16);
        int bl = int_to_ternary(b, bt, 16);
        
        // Perform operation
        char op_str[8];
        int result;
        
        if (strcmp_t(cmd1, "add") == 0) {
            ternary_add(at, bt, rt, 16);
            result = ternary_to_int(rt, 16);
            strcpy_t(op_str, "+");
        } else if (strcmp_t(cmd1, "sub") == 0) {
            ternary_sub(at, bt, rt, 16);
            result = ternary_to_int(rt, 16);
            strcpy_t(op_str, "-");
        } else {
            ternary_mul(at, al, bt, bl, rt, 16);
            result = ternary_to_int(rt, 16);
            strcpy_t(op_str, "*");
        }
        
        // Convert to strings
        char a_str[32], b_str[32], r_str[32];
        trits_to_ternary_str(at, al, a_str);
        trits_to_ternary_str(bt, bl, b_str);
        trits_to_ternary_str(rt, 16, r_str);
        
        vga_puts("\n  ");
        { char nb[8]; num_to_str(a, nb); vga_puts(nb); }
        vga_puts(" (");
        vga_puts(a_str);
        vga_puts(") ");
        vga_puts(op_str);
        vga_puts(" ");
        { char nb[8]; num_to_str(b, nb); vga_puts(nb); }
        vga_puts(" (");
        vga_puts(b_str);
        vga_puts(")\n");
        vga_puts("  = ");
        { char nb[8]; num_to_str(result, nb); vga_puts(nb); }
        vga_puts(" (");
        vga_puts(r_str);
        vga_puts(")\n\n");
        
    } else if (strcmp_t(cmd1, "conv") == 0) {
        // Convert integer to ternary
        while (*p == ' ') p++;
        int n = 0;
        int neg = 0;
        if (*p == '-') { neg = 1; p++; }
        while (*p >= '0' && *p <= '9') { n = n * 10 + (*p - '0'); p++; }
        if (neg) n = -n;
        
        trit trits[16];
        int len = int_to_ternary(n, trits, 16);
        char ter[32];
        trits_to_ternary_str(trits, len, ter);
        
        vga_puts("\n  Decimal: ");
        { char nb[8]; num_to_str(n, nb); vga_puts(nb); }
        vga_puts("\n  Ternary: ");
        vga_puts(ter);
        vga_puts("\n  Trits: ");
        for (int i = 0; i < len; i++) {
            vga_putc(trit_to_char(trits[i]));
            vga_putc(' ');
        }
        vga_puts("\n\n");
        
    } else if (strcmp_t(cmd1, "maya") == 0) {
        // Convert integer to Maya (base 60)
        while (*p == ' ') p++;
        int n = 0;
        while (*p >= '0' && *p <= '9') { n = n * 10 + (*p - '0'); p++; }
        
        int digits[8];
        int len = int_to_base60(n, digits, 8);
        
        vga_puts("\n  Decimal: ");
        { char nb[8]; num_to_str(n, nb); vga_puts(nb); }
        vga_puts("\n  Base-60: ");
        for (int i = len - 1; i >= 0; i--) {
            char nb[4];
            num_to_str(digits[i], nb);
            vga_puts(nb);
            if (i > 0) vga_puts(".");
        }
        vga_puts("\n  Maya: ");
        for (int i = len - 1; i >= 0; i--) {
            char maya[16];
            maya_digit_to_str(digits[i], maya);
            vga_puts(maya);
            if (i > 0) vga_putc(' ');
        }
        vga_puts("\n\n");
        
    } else if (strcmp_t(cmd1, "info") == 0) {
        vga_puts("\n  [Ternary Number System]\n\n");
        vga_puts("  Balanced ternary uses 3 digits:\n");
        vga_puts("    - (negative one)\n");
        vga_puts("    0 (zero)\n");
        vga_puts("    + (positive one)\n\n");
        vga_puts("  Examples:\n");
        vga_puts("    + = 1\n");
        vga_puts("    +0 = 3\n");
        vga_puts("    ++ = 4\n");
        vga_puts("    +- = 2\n");
        vga_puts("    +0- = 8\n\n");
        vga_puts("  Base-60 (Maya):\n");
        vga_puts("    Used by ancient Maya civilization\n");
        vga_puts("    Digits: 0-59\n");
        vga_puts("    Symbols: dots (.) and bars (=)\n\n");
    }
}

// =============================================================================
// TERNARY CALCULATOR (enhanced)
// =============================================================================

// Command: tcalc — ternary calculator with trit display
void cmd_tcalc(const char* args) {
    if (args[0] == 0) {
        vga_puts("\n  [Ternary Calculator]\n\n");
        vga_puts("  Shows results in decimal + ternary + Maya\n\n");
        vga_puts("  Usage: tcalc <expression>\n");
        vga_puts("  Example: tcalc 5 + 3\n\n");
        return;
    }
    
    // Parse expression (simplified: a op b)
    const char* p = args;
    int a = 0, b = 0;
    char op = '+';
    
    while (*p == ' ') p++;
    int neg = 0;
    if (*p == '-') { neg = 1; p++; }
    while (*p >= '0' && *p <= '9') { a = a * 10 + (*p - '0'); p++; }
    if (neg) a = -a;
    
    while (*p == ' ') p++;
    op = *p++;
    
    while (*p == ' ') p++;
    neg = 0;
    if (*p == '-') { neg = 1; p++; }
    while (*p >= '0' && *p <= '9') { b = b * 10 + (*p - '0'); p++; }
    if (neg) b = -b;
    
    // Calculate
    int result;
    switch (op) {
        case '+': result = a + b; break;
        case '-': result = a - b; break;
        case '*': result = a * b; break;
        case '/': result = (b != 0) ? a / b : 0; break;
        default: result = 0;
    }
    
    // Convert to all bases
    trit trits[16];
    int len = int_to_ternary(result, trits, 16);
    char ter[32];
    trits_to_ternary_str(trits, len, ter);
    
    int base60[8];
    int b60_len = int_to_base60(result, base60, 8);
    
    vga_puts("\n  Result:\n");
    vga_puts("  Decimal: ");
    { char nb[8]; num_to_str(result, nb); vga_puts(nb); }
    vga_puts("\n  Ternary: ");
    vga_puts(ter);
    vga_puts("\n  Base-60: ");
    for (int i = b60_len - 1; i >= 0; i--) {
        char nb[4];
        num_to_str(base60[i], nb);
        vga_puts(nb);
        if (i > 0) vga_puts(".");
    }
    vga_puts("\n  Binary:  ");
    for (int i = 31; i >= 0; i--) {
        vga_putc((result >> i) & 1 ? '1' : '0');
    }
    vga_puts("\n\n");
}
