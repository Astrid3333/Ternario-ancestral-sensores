/**
 * math_shell.c — Comandos matemáticos para shell Tritos
 *
 * Integración con octave-mcp vía shell
 */

#include "../include/ternary.h"

// =============================================================================
// MATH COMMANDS
// =============================================================================

// Command: math — operaciones matemáticas ternarias
void cmd_math(const char* args) {
    if (args[0] == 0 || strcmp_t(args, "help") == 0) {
        vga_puts("\n  [Math Commands]\n\n");
        vga_puts("  Comandos:\n");
        vga_puts("    math add <a> <b>       Suma ternaria\n");
        vga_puts("    math sub <a> <b>       Resta ternaria\n");
        vga_puts("    math mul <a> <b>       Multiplicación ternaria\n");
        vga_puts("    math conv <n>          Decimal → ternario\n");
        vga_puts("    math maya <n>          Decimal → Maya (base 60)\n");
        vga_puts("    math landauer <n>      Límite de Landauer\n");
        vga_puts("    math compare <n>       Comparar sistemas\n");
        vga_puts("    math info              Info sobre sistemas\n\n");
        return;
    }
    
    const char* p = args;
    char cmd[16] = {0};
    int i = 0;
    while (*p && *p != ' ' && i < 15) cmd[i++] = *p++;
    while (*p == ' ') p++;
    
    // Parse numbers
    int a = 0, b = 0;
    int neg = 0;
    
    if (*p == '-') { neg = 1; p++; }
    while (*p >= '0' && *p <= '9') { a = a * 10 + (*p - '0'); p++; }
    if (neg) a = -a;
    
    while (*p == ' ') p++;
    neg = 0;
    if (*p == '-') { neg = 1; p++; }
    while (*p >= '0' && *p <= '9') { b = b * 10 + (*p - '0'); p++; }
    if (neg) b = -b;
    
    if (strcmp_t(cmd, "add") == 0) {
        trit at[16], bt[16], rt[16];
        int al = int_to_ternary(a, at, 16);
        int bl = int_to_ternary(b, bt, 16);
        ternary_add(at, bt, rt, 16);
        int result = ternary_to_int(rt, 16);
        
        char a_str[32], b_str[32], r_str[32];
        trits_to_ternary_str(at, al, a_str);
        trits_to_ternary_str(bt, bl, b_str);
        trits_to_ternary_str(rt, 16, r_str);
        
        vga_puts("\n  ");
        { char nb[8]; num_to_str(a, nb); vga_puts(nb); }
        vga_puts(" (");
        vga_puts(a_str);
        vga_puts(") + ");
        { char nb[8]; num_to_str(b, nb); vga_puts(nb); }
        vga_puts(" (");
        vga_puts(b_str);
        vga_puts(")\n  = ");
        { char nb[8]; num_to_str(result, nb); vga_puts(nb); }
        vga_puts(" (");
        vga_puts(r_str);
        vga_puts(")\n\n");
        
    } else if (strcmp_t(cmd, "sub") == 0) {
        trit at[16], bt[16], rt[16];
        int al = int_to_ternary(a, at, 16);
        int bl = int_to_ternary(b, bt, 16);
        ternary_sub(at, bt, rt, 16);
        int result = ternary_to_int(rt, 16);
        
        char a_str[32], b_str[32], r_str[32];
        trits_to_ternary_str(at, al, a_str);
        trits_to_ternary_str(bt, bl, b_str);
        trits_to_ternary_str(rt, 16, r_str);
        
        vga_puts("\n  ");
        { char nb[8]; num_to_str(a, nb); vga_puts(nb); }
        vga_puts(" (");
        vga_puts(a_str);
        vga_puts(") - ");
        { char nb[8]; num_to_str(b, nb); vga_puts(nb); }
        vga_puts(" (");
        vga_puts(b_str);
        vga_puts(")\n  = ");
        { char nb[8]; num_to_str(result, nb); vga_puts(nb); }
        vga_puts(" (");
        vga_puts(r_str);
        vga_puts(")\n\n");
        
    } else if (strcmp_t(cmd, "mul") == 0) {
        trit at[16], bt[16], rt[16];
        int al = int_to_ternary(a, at, 16);
        int bl = int_to_ternary(b, bt, 16);
        ternary_mul(at, al, bt, bl, rt, 16);
        int result = ternary_to_int(rt, 16);
        
        char a_str[32], b_str[32], r_str[32];
        trits_to_ternary_str(at, al, a_str);
        trits_to_ternary_str(bt, bl, b_str);
        trits_to_ternary_str(rt, 16, r_str);
        
        vga_puts("\n  ");
        { char nb[8]; num_to_str(a, nb); vga_puts(nb); }
        vga_puts(" (");
        vga_puts(a_str);
        vga_puts(") × ");
        { char nb[8]; num_to_str(b, nb); vga_puts(nb); }
        vga_puts(" (");
        vga_puts(b_str);
        vga_puts(")\n  = ");
        { char nb[8]; num_to_str(result, nb); vga_puts(nb); }
        vga_puts(" (");
        vga_puts(r_str);
        vga_puts(")\n\n");
        
    } else if (strcmp_t(cmd, "conv") == 0) {
        trit trits[16];
        int len = int_to_ternary(a, trits, 16);
        char ter[32];
        trits_to_ternary_str(trits, len, ter);
        
        vga_puts("\n  Decimal: ");
        { char nb[8]; num_to_str(a, nb); vga_puts(nb); }
        vga_puts("\n  Ternary: ");
        vga_puts(ter);
        vga_puts("\n  Trits: ");
        for (i = 0; i < len; i++) {
            vga_putc(trit_to_char(trits[i]));
            vga_putc(' ');
        }
        vga_puts("\n\n");
        
    } else if (strcmp_t(cmd, "maya") == 0) {
        int digits[8];
        int len = int_to_base60(a, digits, 8);
        
        vga_puts("\n  Decimal: ");
        { char nb[8]; num_to_str(a, nb); vga_puts(nb); }
        vga_puts("\n  Base-60: ");
        for (i = len - 1; i >= 0; i--) {
            char nb[4];
            num_to_str(digits[i], nb);
            vga_puts(nb);
            if (i > 0) vga_puts(".");
        }
        vga_puts("\n  Maya: ");
        for (i = len - 1; i >= 0; i--) {
            char maya[16];
            maya_digit_to_str(digits[i], maya);
            vga_puts(maya);
            if (i > 0) vga_putc(' ');
        }
        vga_puts("\n\n");
        
    } else if (strcmp_t(cmd, "landauer") == 0) {
        // Landauer limit: E = kT * ln(2)
        // For ternary: E = kT * ln(3)
        vga_puts("\n  [Límite de Landauer]\n\n");
        vga_puts("  Fórmula: E = kT × ln(base)\n");
        vga_puts("  k = 1.380649 × 10⁻²³ J/K\n");
        vga_puts("  T = 300K (temperatura ambiente)\n\n");
        
        // Calculate for different bases
        vga_puts("  Base 2 (binario):  2.87 × 10⁻²¹ J/bit\n");
        vga_puts("  Base 3 (ternario): 4.56 × 10⁻²¹ J/trit\n");
        vga_puts("  Base 10:           9.56 × 10⁻²¹ J/dígito\n\n");
        
        // For n symbols
        if (a > 0) {
            double energy_bin = a * 2.87e-21;
            double energy_ter = a * 4.56e-21;
            vga_puts("  Para ");
            { char nb[8]; num_to_str(a, nb); vga_puts(nb); }
            vga_puts(" símbolos:\n");
            vga_puts("    Binario: ");
            // Simple scientific notation
            int exp_bin = 0;
            double val_bin = energy_bin;
            while (val_bin >= 10) { val_bin /= 10; exp_bin++; }
            while (val_bin < 1) { val_bin *= 10; exp_bin--; }
            { char nb[4]; num_to_str((int)val_bin, nb); vga_puts(nb); }
            vga_puts(" × 10^");
            { char nb[4]; num_to_str(exp_bin, nb); vga_puts(nb); }
            vga_puts(" J\n");
            
            vga_puts("    Ternario: ");
            int exp_ter = 0;
            double val_ter = energy_ter;
            while (val_ter >= 10) { val_ter /= 10; exp_ter++; }
            while (val_ter < 1) { val_ter *= 10; exp_ter--; }
            { char nb[4]; num_to_str((int)val_ter, nb); vga_puts(nb); }
            vga_puts(" × 10^");
            { char nb[4]; num_to_str(exp_ter, nb); vga_puts(nb); }
            vga_puts(" J\n");
        }
        vga_puts("\n");
        
    } else if (strcmp_t(cmd, "compare") == 0) {
        vga_puts("\n  [Comparación de Sistemas]\n\n");
        vga_puts("  Número: ");
        { char nb[8]; num_to_str(a, nb); vga_puts(nb); }
        vga_puts("\n\n");
        
        // Binary
        vga_puts("  Binario:    ");
        for (i = 31; i >= 0; i--) {
            vga_putc((a >> i) & 1 ? '1' : '0');
        }
        vga_puts("\n");
        
        // Ternary
        trit trits[16];
        int len = int_to_ternary(a, trits, 16);
        vga_puts("  Ternario:   ");
        for (i = len - 1; i >= 0; i--) {
            vga_putc(trit_to_char(trits[i]));
        }
        vga_puts("\n");
        
        // Maya
        int digits[8];
        int dlen = int_to_base60(a, digits, 8);
        vga_puts("  Maya:       ");
        for (i = dlen - 1; i >= 0; i--) {
            char nb[4];
            num_to_str(digits[i], nb);
            vga_puts(nb);
            if (i > 0) vga_puts(".");
        }
        vga_puts("\n\n");
        
    } else if (strcmp_t(cmd, "info") == 0) {
        vga_puts("\n  [Sistemas Numéricos]\n\n");
        vga_puts("  Binario (base 2):\n");
        vga_puts("    Dígitos: 0, 1\n");
        vga_puts("    Usado por: computadoras modernas\n\n");
        vga_puts("  Ternario balanceado (base 3):\n");
        vga_puts("    Dígitos: -, 0, +\n");
        vga_puts("    Usado por: Setun, investigación\n\n");
        vga_puts("  Maya (base 20):\n");
        vga_puts("    Dígitos: 0-19 (puntos y barras)\n");
        vga_puts("    Usado por: civilización Maya\n\n");
        vga_puts("  Babilónico (base 60):\n");
        vga_puts("    Dígitos: 0-59\n");
        vga_puts("    Usado por: astronomía antigua\n\n");
    }
}
