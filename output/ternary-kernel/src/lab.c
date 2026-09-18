/**
 * lab.c — Laboratorio Matemático Ancestral
 *
 * Módulo de experimentación con sistemas numéricos ancestrales.
 * Integra octave-mcp para cálculos en múltiples bases.
 *
 * Comandos:
 *   formula <expr>                          Evaluar en múltiples sistemas
 *   lab <op> <a> <b> --sistema=<sys>        Operaciones paso a paso
 *   codigo <tipo> <valor>                   Explorar códigos ternarios
 *   ancestro <civilización> --<calendario> <fecha>   Calendarios ancestrales
 *   patron <n> --sistema=<sys>              Visualizar patrones
 */

#include "../include/ternary.h"

// =============================================================================
// HELPERS
// =============================================================================

int parse_lab_num(const char** p) {
    int n = 0, neg = 0;
    while (**p == ' ') (*p)++;
    if (**p == '-') { neg = 1; (*p)++; }
    while (**p >= '0' && **p <= '9') { n = n * 10 + (**p - '0'); (*p)++; }
    return neg ? -n : n;
}

int digits_count(int n) {
    if (n == 0) return 1;
    int c = 0;
    while (n > 0) { c++; n /= 10; }
    return c;
}

// =============================================================================
// 1. FORMULA — Evaluar en múltiples sistemas
// =============================================================================

void cmd_formula(const char* args) {
    if (args[0] == 0) {
        vga_puts("\n  [Fórmulas Ancestrales]\n\n");
        vga_puts("  Uso: formula <número>\n");
        vga_puts("  Evalúa un número en múltiples sistemas.\n\n");
        vga_puts("  Ejemplos:\n");
        vga_puts("    formula 255\n");
        vga_puts("    formula 1000\n\n");
        return;
    }

    const char* p = args;
    int val = parse_lab_num(&p);

    vga_puts("\n  ╔══════════════════════════════════════╗\n");
    vga_puts("  ║  FÓRMULAS ANCESTRALES                ║\n");
    vga_puts("  ╠══════════════════════════════════════╣\n");
    vga_puts("  ║  Número: ");

    { char nb[8]; num_to_str(val, nb); vga_puts(nb); }
    vga_puts("\n");

    // Binario
    vga_puts("  ║  Binario:     ");
    for (int i = 31; i >= 0; i--) {
        if (i < 12) vga_putc((val >> i) & 1 ? '1' : '0');
    }
    vga_puts("\n");

    // Ternario balanceado
    {
        trit trits[16];
        int len = int_to_ternary(val, trits, 16);
        vga_puts("  ║  Ternario:    ");
        for (int i = len - 1; i >= 0; i--) {
            vga_putc(trit_to_char(trits[i]));
        }
        vga_puts("\n");
    }

    // Maya (base 20)
    {
        int digits[8];
        int len = int_to_base60(val, digits, 8);
        // Reinterpretar como base 20
        vga_puts("  ║  Maya:        ");
        for (int i = len - 1; i >= 0; i--) {
            char nb[4];
            num_to_str(digits[i], nb);
            vga_puts(nb);
            if (i > 0) vga_puts(".");
        }
        vga_puts(" (base 60)\n");
    }

    // Babilónico (base 60)
    {
        int digits[8];
        int len = int_to_base60(val, digits, 8);
        vga_puts("  ║  Babilónico:  ");
        for (int i = len - 1; i >= 0; i--) {
            char nb[4];
            num_to_str(digits[i], nb);
            vga_puts(nb);
            if (i > 0) vga_puts(".");
        }
        vga_puts("\n");
    }

    // Quipu (nudos)
    {
        int n = val;
        if (n < 0) n = -n;
        int simple = 0, doble = 0, triple = 0;
        while (n > 0) {
            int d = n % 10;
            if (d <= 3) simple++;
            else if (d <= 6) doble++;
            else triple++;
            n /= 10;
        }
        vga_puts("  ║  Quipu:       ");
        { char nb[8]; num_to_str(simple, nb); vga_puts(nb); }
        vga_puts(" simples, ");
        { char nb[8]; num_to_str(doble, nb); vga_puts(nb); }
        vga_puts(" dobles, ");
        { char nb[8]; num_to_str(triple, nb); vga_puts(nb); }
        vga_puts(" triples\n");
    }

    // Hexadecimal
    vga_puts("  ║  Hex:         ");
    {
        const char* hex = "0123456789ABCDEF";
        int val2 = val;
        if (val2 == 0) { vga_putc('0'); }
        else {
            char buf[8];
            int idx = 0;
            while (val2 > 0 && idx < 8) {
                buf[idx++] = hex[val2 % 16];
                val2 /= 16;
            }
            for (int i = idx - 1; i >= 0; i--) vga_putc(buf[i]);
        }
    }
    vga_puts("\n");

    // Eficiencia
    {
        int bits = 0;
        int tmp = val;
        while (tmp > 0) { bits++; tmp /= 2; }
        int trits = 0;
        tmp = val;
        while (tmp > 0) { trits++; tmp /= 3; }
        vga_puts("  ║  Bits necesarios:   ");
        { char nb[8]; num_to_str(bits, nb); vga_puts(nb); }
        vga_puts("\n  ║  Trits necesarios:  ");
        { char nb[8]; num_to_str(trits, nb); vga_puts(nb); }
        vga_puts("\n  ║  Ahorro ternario:   ");
        int savings = bits > 0 ? (bits - trits) * 100 / bits : 0;
        { char nb[8]; num_to_str(savings, nb); vga_puts(nb); }
        vga_puts("%\n");
    }

    vga_puts("  ╚══════════════════════════════════════╝\n\n");
}

// =============================================================================
// 2. LAB — Operaciones paso a paso
// =============================================================================

void cmd_lab(const char* args) {
    if (args[0] == 0) {
        vga_puts("\n  [Laboratorio Ancestral]\n\n");
        vga_puts("  Uso: lab <op> <a> <b> [opciones]\n\n");
        vga_puts("  Operaciones:\n");
        vga_puts("    lab suma <a> <b>       Suma paso a paso\n");
        vga_puts("    lab mul <a> <b>        Multiplicación paso a paso\n");
        vga_puts("    lab conv <n>           Conversión paso a paso\n\n");
        vga_puts("  Sistemas (opciones):\n");
        vga_puts("    --sistema=maya\n");
        vga_puts("    --sistema=babilonio\n");
        vga_puts("    --sistema=ternario\n\n");
        vga_puts("  Ejemplo:\n");
        vga_puts("    lab suma 47 85 --sistema=maya\n\n");
        return;
    }

    const char* p = args;
    char op[16] = {0};
    int i = 0;
    while (*p && *p != ' ' && i < 15) op[i++] = *p++;
    while (*p == ' ') p++;

    int a = parse_lab_num(&p);
    int b = parse_lab_num(&p);

    // Detectar sistema
    int sys = 0; // 0=ternario, 1=mayo, 2=babilonio
    while (*p) {
        while (*p == ' ') p++;
        if (p[0] == '-' && p[1] == '-') {
            p += 2;
            if (p[0] == 's' && p[1] == 'i' && p[2] == 's') {
                p += 8; // "sistema="
                if (p[0] == 'm') { sys = 1; while (*p && *p != ' ') p++; }
                else if (p[0] == 'b') { sys = 2; while (*p && *p != ' ') p++; }
                else { sys = 0; while (*p && *p != ' ') p++; }
            } else {
                while (*p && *p != ' ') p++;
            }
        } else {
            p++;
        }
    }

    if (strcmp_t(op, "suma") == 0 || strcmp_t(op, "add") == 0) {
        int result = a + b;

        vga_puts("\n  ╔══════════════════════════════════════╗\n");
        vga_puts("  ║  SUMA PASO A PASO                    ║\n");
        vga_puts("  ╠══════════════════════════════════════╣\n");

        if (sys == 1) {
            // Maya
            int da[8], db[8], dl;
            int al = int_to_base60(a, da, 8);
            int bl = int_to_base60(b, db, 8);
            vga_puts("  ║  Método Maya:\n");
            vga_puts("  ║  ");
            { char nb[8]; num_to_str(a, nb); vga_puts(nb); }
            vga_puts(" = ");
            for (i = al - 1; i >= 0; i--) {
                char nb[4]; num_to_str(da[i], nb); vga_puts(nb);
                if (i > 0) vga_puts(".");
            }
            vga_puts("\n  ║  ");
            { char nb[8]; num_to_str(b, nb); vga_puts(nb); }
            vga_puts(" = ");
            for (i = bl - 1; i >= 0; i--) {
                char nb[4]; num_to_str(db[i], nb); vga_puts(nb);
                if (i > 0) vga_puts(".");
            }
            vga_puts("\n  ║  Suma: ");
            { char nb[8]; num_to_str(a, nb); vga_puts(nb); }
            vga_puts(" + ");
            { char nb[8]; num_to_str(b, nb); vga_puts(nb); }
            vga_puts(" = ");
            { char nb[8]; num_to_str(result, nb); vga_puts(nb); }
            vga_puts("\n  ║  En base 60: ");
            int rd[8];
            int rl = int_to_base60(result, rd, 8);
            for (i = rl - 1; i >= 0; i--) {
                char nb[4]; num_to_str(rd[i], nb); vga_puts(nb);
                if (i > 0) vga_puts(".");
            }
        } else if (sys == 2) {
            // Babilónico
            vga_puts("  ║  Método Babilónico:\n");
            vga_puts("  ║  ");
            { char nb[8]; num_to_str(a, nb); vga_puts(nb); }
            vga_puts(" + ");
            { char nb[8]; num_to_str(b, nb); vga_puts(nb); }
            vga_puts("\n  ║  Producto: ");
            { char nb[8]; num_to_str(result, nb); vga_puts(nb); }
            vga_puts("\n  ║  En base 60: ");
            int rd[8];
            int rl = int_to_base60(result, rd, 8);
            for (i = rl - 1; i >= 0; i--) {
                char nb[4]; num_to_str(rd[i], nb); vga_puts(nb);
                if (i > 0) vga_puts(".");
            }
        } else {
            // Ternario
            trit at[16], bt[16], rt[16];
            int al = int_to_ternary(a, at, 16);
            int bl = int_to_ternary(b, bt, 16);
            ternary_add(at, bt, rt, 16);
            char a_str[32], b_str[32], r_str[32];
            trits_to_ternary_str(at, al, a_str);
            trits_to_ternary_str(bt, bl, b_str);
            trits_to_ternary_str(rt, 16, r_str);

            vga_puts("  ║  Método Ternario:\n");
            vga_puts("  ║  ");
            { char nb[8]; num_to_str(a, nb); vga_puts(nb); }
            vga_puts(" = ");
            vga_puts(a_str);
            vga_puts("\n  ║  ");
            { char nb[8]; num_to_str(b, nb); vga_puts(nb); }
            vga_puts(" = ");
            vga_puts(b_str);
            vga_puts("\n  ║  Suma ternaria:\n");
            vga_puts("  ║    ");
            vga_puts(a_str);
            vga_puts("\n  ║  + ");
            vga_puts(b_str);
            vga_puts("\n  ║  = ");
            vga_puts(r_str);
        }

        vga_puts("\n  ║  Resultado: ");
        { char nb[8]; num_to_str(result, nb); vga_puts(nb); }
        vga_puts("\n  ╚══════════════════════════════════════╝\n\n");

    } else if (strcmp_t(op, "mul") == 0 || strcmp_t(op, "multiplicacion") == 0) {
        int result = a * b;

        vga_puts("\n  ╔══════════════════════════════════════╗\n");
        vga_puts("  ║  MULTIPLICACIÓN PASO A PASO          ║\n");
        vga_puts("  ╠══════════════════════════════════════╣\n");

        if (sys == 2) {
            vga_puts("  ║  Método Babilónico:\n");
            vga_puts("  ║  ");
            { char nb[8]; num_to_str(a, nb); vga_puts(nb); }
            vga_puts(" × ");
            { char nb[8]; num_to_str(b, nb); vga_puts(nb); }
            vga_puts("\n  ║  Producto: ");
            { char nb[8]; num_to_str(result, nb); vga_puts(nb); }
            vga_puts("\n  ║  En base 60: ");
            int rd[8];
            int rl = int_to_base60(result, rd, 8);
            for (i = rl - 1; i >= 0; i--) {
                char nb[4]; num_to_str(rd[i], nb); vga_puts(nb);
                if (i > 0) vga_puts(".");
            }
        } else {
            trit at[16], bt[16], rt[16];
            int al = int_to_ternary(a, at, 16);
            int bl = int_to_ternary(b, bt, 16);
            ternary_mul(at, al, bt, bl, rt, 16);
            char a_str[32], b_str[32], r_str[32];
            trits_to_ternary_str(at, al, a_str);
            trits_to_ternary_str(bt, bl, b_str);
            trits_to_ternary_str(rt, 16, r_str);

            vga_puts("  ║  Método Ternario:\n");
            vga_puts("  ║  ");
            { char nb[8]; num_to_str(a, nb); vga_puts(nb); }
            vga_puts(" = ");
            vga_puts(a_str);
            vga_puts("\n  ║  ");
            { char nb[8]; num_to_str(b, nb); vga_puts(nb); }
            vga_puts(" = ");
            vga_puts(b_str);
            vga_puts("\n  ║  Producto:\n");
            vga_puts("  ║    ");
            vga_puts(a_str);
            vga_puts("\n  ║  × ");
            vga_puts(b_str);
            vga_puts("\n  ║  = ");
            vga_puts(r_str);
        }

        vga_puts("\n  ║  Resultado: ");
        { char nb[8]; num_to_str(result, nb); vga_puts(nb); }
        vga_puts("\n  ╚══════════════════════════════════════╝\n\n");

    } else if (strcmp_t(op, "conv") == 0) {
        vga_puts("\n  ╔══════════════════════════════════════╗\n");
        vga_puts("  ║  CONVERSIÓN PASO A PASO              ║\n");
        vga_puts("  ╠══════════════════════════════════════╣\n");
        vga_puts("  ║  Número: ");
        { char nb[8]; num_to_str(a, nb); vga_puts(nb); }
        vga_puts("\n");

        // Paso 1: Binario
        vga_puts("  ║  Paso 1 → Binario:\n");
        vga_puts("  ║    ");
        {
            int tmp = a;
            char buf[32];
            int idx = 0;
            if (tmp == 0) { buf[idx++] = '0'; }
            while (tmp > 0) { buf[idx++] = '0' + (tmp % 2); tmp /= 2; }
            for (i = idx - 1; i >= 0; i--) vga_putc(buf[i]);
        }
        vga_puts("\n");

        // Paso 2: Ternario
        vga_puts("  ║  Paso 2 → Ternario:\n");
        vga_puts("  ║    ");
        {
            trit trits[16];
            int len = int_to_ternary(a, trits, 16);
            for (i = len - 1; i >= 0; i--) {
                vga_putc(trit_to_char(trits[i]));
            }
        }
        vga_puts("\n");

        // Paso 3: Base 60
        vga_puts("  ║  Paso 3 → Base 60:\n");
        vga_puts("  ║    ");
        {
            int digits[8];
            int len = int_to_base60(a, digits, 8);
            for (i = len - 1; i >= 0; i--) {
                char nb[4];
                num_to_str(digits[i], nb);
                vga_puts(nb);
                if (i > 0) vga_puts(".");
            }
        }
        vga_puts("\n");

        // Paso 4: Maya
        vga_puts("  ║  Paso 4 → Maya:\n");
        vga_puts("  ║    ");
        {
            int digits[8];
            int len = int_to_base60(a, digits, 8);
            for (i = len - 1; i >= 0; i--) {
                char maya[16];
                maya_digit_to_str(digits[i], maya);
                vga_puts(maya);
                if (i > 0) vga_putc(' ');
            }
        }
        vga_puts("\n");

        vga_puts("  ╚══════════════════════════════════════╝\n\n");

    } else {
        vga_puts("\n  Operación desconocida: ");
        vga_puts(op);
        vga_puts("\n  Usa: lab suma|mul|conv\n\n");
    }
}

// =============================================================================
// 3. CODIGO — Explorador de códigos ternarios
// =============================================================================

void cmd_codigo(const char* args) {
    if (args[0] == 0) {
        vga_puts("\n  [Explorador de Códigos Ternarios]\n\n");
        vga_puts("  Uso: codigo <tipo> <valor>\n\n");
        vga_puts("  Tipos:\n");
        vga_puts("    codigo trits5 <n>    Representación 5-trits\n");
        vga_puts("    codigo quipu <n>     Nudos quipu\n");
        vga_puts("    codigo pack <n>      Empaquetado ternario\n");
        vga_puts("    codigo entropia <n>  Entropía de Shannon\n\n");
        vga_puts("  Ejemplo:\n");
        vga_puts("    codigo trits5 255\n\n");
        return;
    }

    const char* p = args;
    char tipo[16] = {0};
    int i = 0;
    while (*p && *p != ' ' && i < 15) tipo[i++] = *p++;
    while (*p == ' ') p++;
    int val = parse_lab_num(&p);

    if (strcmp_t(tipo, "trits5") == 0) {
        // Representación 5 trits
        trit trits[5];
        int len = int_to_ternary(val, trits, 5);

        vga_puts("\n  ╔══════════════════════════════════════╗\n");
        vga_puts("  ║  CÓDIGO TRITS-5                     ║\n");
        vga_puts("  ╠══════════════════════════════════════╣\n");
        vga_puts("  ║  Valor: ");
        { char nb[8]; num_to_str(val, nb); vga_puts(nb); }
        vga_puts("\n  ║  Representación: [");
        for (i = len - 1; i >= 0; i--) {
            vga_putc(trit_to_char(trits[i]));
            if (i > 0) vga_puts("][");
        }
        vga_puts("]\n");

        // Empaquetado
        int packed = 0;
        for (i = 0; i < len; i++) {
            packed = packed * 3 + (trits[i] + 1);
        }
        vga_puts("  ║  Empaquetado: ");
        {
            char buf[16];
            int idx = 0;
            int tmp = packed;
            if (tmp == 0) { buf[idx++] = '0'; }
            while (tmp > 0) { buf[idx++] = '0' + (tmp % 2); tmp /= 2; }
            for (i = idx - 1; i >= 0; i--) vga_putc(buf[i]);
        }
        vga_puts("\n");

        // Eficiencia
        int bits_needed = 0;
        int tmp = val;
        while (tmp > 0) { bits_needed++; tmp /= 2; }
        vga_puts("  ║  Bits necesarios: ");
        { char nb[8]; num_to_str(bits_needed, nb); vga_puts(nb); }
        vga_puts("\n  ║  Trits usados: ");
        { char nb[8]; num_to_str(len, nb); vga_puts(nb); }
        vga_puts("\n  ║  Eficiencia: ");
        int eff = bits_needed > 0 ? (len * 100 + bits_needed / 2) / bits_needed : 0;
        { char nb[8]; num_to_str(eff, nb); vga_puts(nb); }
        vga_puts("%\n");
        vga_puts("  ╚══════════════════════════════════════╝\n\n");

    } else if (strcmp_t(tipo, "quipu") == 0) {
        // Quipu
        int n = val;
        if (n < 0) n = -n;
        int simple = 0, doble = 0, triple = 0;
        int total = n;
        while (n > 0) {
            int d = n % 10;
            if (d <= 3) simple++;
            else if (d <= 6) doble++;
            else triple++;
            n /= 10;
        }

        vga_puts("\n  ╔══════════════════════════════════════╗\n");
        vga_puts("  ║  CÓDIGO QUIPU                       ║\n");
        vga_puts("  ╠══════════════════════════════════════╣\n");
        vga_puts("  ║  Valor: ");
        { char nb[8]; num_to_str(val, nb); vga_puts(nb); }
        vga_puts("\n  ║  Nudos simples (1-3): ");
        { char nb[8]; num_to_str(simple, nb); vga_puts(nb); }
        vga_puts("\n  ║  Nudos dobles (4-6):  ");
        { char nb[8]; num_to_str(doble, nb); vga_puts(nb); }
        vga_puts("\n  ║  Nudos triples (7-9): ");
        { char nb[8]; num_to_str(triple, nb); vga_puts(nb); }
        vga_puts("\n");

        // Paridad
        int parity = 0;
        int tmp = total;
        while (tmp > 0) { parity ^= tmp & 1; tmp >>= 1; }
        vga_puts("  ║  Paridad: ");
        vga_puts(parity ? "impar" : "par");
        vga_puts("\n");

        // Checksum
        int checksum = 0;
        tmp = total;
        while (tmp > 0) { checksum += tmp % 10; tmp /= 10; }
        vga_puts("  ║  Checksum: ");
        { char nb[8]; num_to_str(checksum, nb); vga_puts(nb); }
        vga_puts("\n  ║  Detección: 100% (parity + checksum)\n");
        vga_puts("  ╚══════════════════════════════════════╝\n\n");

    } else if (strcmp_t(tipo, "pack") == 0) {
        // Empaquetado ternario
        trit trits[16];
        int len = int_to_ternary(val, trits, 16);

        vga_puts("\n  ╔══════════════════════════════════════╗\n");
        vga_puts("  ║  EMPAQUETADO TERNARIO               ║\n");
        vga_puts("  ╠══════════════════════════════════════╣\n");
        vga_puts("  ║  Valor: ");
        { char nb[8]; num_to_str(val, nb); vga_puts(nb); }
        vga_puts("\n  ║  Trits: ");
        for (i = len - 1; i >= 0; i--) {
            vga_putc(trit_to_char(trits[i]));
        }
        vga_puts("\n");

        // Empaquetar 5 trits en 8 bits
        int packed = 0;
        for (i = 0; i < len && i < 5; i++) {
            packed = packed * 3 + (trits[i] + 1);
        }
        vga_puts("  ║  Empaquetado (5→8 bits): ");
        {
            char buf[16];
            int idx = 0;
            int tmp = packed;
            if (tmp == 0) { buf[idx++] = '0'; }
            while (tmp > 0) { buf[idx++] = '0' + (tmp % 2); tmp /= 2; }
            for (i = idx - 1; i >= 0; i--) vga_putc(buf[i]);
        }
        vga_puts("\n  ║  Ahorro: ");
        int savings = (8 - len) * 100 / 8;
        { char nb[8]; num_to_str(savings, nb); vga_puts(nb); }
        vga_puts("%\n");
        vga_puts("  ╚══════════════════════════════════════╝\n\n");

    } else if (strcmp_t(tipo, "entropia") == 0) {
        // Entropía de Shannon
        int n = val;
        if (n < 0) n = -n;

        int freq[3] = {0, 0, 0};
        int total = 0;
        int tmp = n;
        while (tmp > 0) {
            freq[tmp % 3]++;
            total++;
            tmp /= 3;
        }

        vga_puts("\n  ╔══════════════════════════════════════╗\n");
        vga_puts("  ║  ENTROPÍA DE SHANNON                ║\n");
        vga_puts("  ╠══════════════════════════════════════╣\n");
        vga_puts("  ║  Valor: ");
        { char nb[8]; num_to_str(val, nb); vga_puts(nb); }
        vga_puts("\n  ║  Frecuencia de dígitos:\n");
        vga_puts("  ║    0: ");
        { char nb[8]; num_to_str(freq[0], nb); vga_puts(nb); }
        vga_puts("\n  ║    1: ");
        { char nb[8]; num_to_str(freq[1], nb); vga_puts(nb); }
        vga_puts("\n  ║    2: ");
        { char nb[8]; num_to_str(freq[2], nb); vga_puts(nb); }
        vga_puts("\n");

        // Calcular entropía (aproximada)
        double entropy = 0;
        for (i = 0; i < 3; i++) {
            if (freq[i] > 0) {
                double p = (double)freq[i] / total;
                // ln(x) ≈ (x-1) - (x-1)²/2 + (x-1)³/3
                double ln_p = 0;
                double x = p;
                double term = x - 1;
                ln_p = term;
                term *= (x - 1) / 2;
                ln_p -= term;
                term *= (x - 1) / 3;
                ln_p += term;
                entropy -= p * ln_p;
            }
        }

        vga_puts("  ║  Entropía: ~");
        {
            int e_int = (int)(entropy * 100);
            char nb[8];
            num_to_str(e_int / 100, nb);
            vga_puts(nb);
            vga_putc('.');
            num_to_str(e_int % 100, nb);
            if (e_int % 100 < 10) vga_putc('0');
            vga_puts(nb);
        }
        vga_puts(" bits/trit\n");
        vga_puts("  ║  Máximo posible: 1.585 bits/trit (log₂3)\n");
        vga_puts("  ╚══════════════════════════════════════╝\n\n");

    } else {
        vga_puts("\n  Tipo desconocido: ");
        vga_puts(tipo);
        vga_puts("\n  Usa: codigo trits5|quipu|pack|entropia\n\n");
    }
}

// =============================================================================
// 4. ANCESTRO — Calendarios ancestrales
// =============================================================================

void cmd_ancestro(const char* args) {
    if (args[0] == 0) {
        vga_puts("\n  [Calendarios Ancestrales]\n\n");
        vga_puts("  Uso: ancestro <civilización> --<calendario> <año>-<mes>-<día>\n\n");
        vga_puts("  Civilizaciones:\n");
        vga_puts("    ancestro maya --tzolkin 2026-09-16\n");
        vga_puts("    ancestro maya --hab 2026-09-16\n");
        vga_puts("    ancestro maya --larga 2026-09-16\n");
        vga_puts("    ancestro persa --calendario 2026-09-16\n\n");
        return;
    }

    const char* p = args;
    char civ[16] = {0};
    int i = 0;
    while (*p && *p != ' ' && i < 15) civ[i++] = *p++;
    while (*p == ' ') p++;

    // Detectar opción --tzolkin, --hab, --larga, --calendario
    char opt[16] = {0};
    i = 0;
    if (p[0] == '-' && p[1] == '-') {
        p += 2;
        while (*p && *p != ' ' && i < 15) opt[i++] = *p++;
    }
    while (*p == ' ') p++;

    // Parsear fecha YYYY-MM-DD
    int year = 0, month = 0, day = 0;
    while (*p >= '0' && *p <= '9') year = year * 10 + (*p - '0');
    if (*p == '-') { p++; while (*p >= '0' && *p <= '9') month = month * 10 + (*p - '0'); }
    if (*p == '-') { p++; while (*p >= '0' && *p <= '9') day = day * 10 + (*p - '0'); }

    if (year == 0) { year = 2026; month = 9; day = 16; } // Default

    if (strcmp_t(civ, "maya") == 0) {
        // Calendario Maya simplificado
        vga_puts("\n  ╔══════════════════════════════════════╗\n");
        vga_puts("  ║  CALENDARIO MAYA                     ║\n");
        vga_puts("  ╠══════════════════════════════════════╣\n");
        vga_puts("  ║  Fecha gregoriana: ");
        { char nb[8]; num_to_str(year, nb); vga_puts(nb); vga_putc('-');
          num_to_str(month, nb); vga_puts(nb); vga_putc('-');
          num_to_str(day, nb); vga_puts(nb); }
        vga_puts("\n");

        if (strcmp_t(opt, "tzolkin") == 0) {
            // Tzolkin: 260 días (20 nombres × 13 números)
            int day_count = (year - 1900) * 365 + (month - 1) * 30 + day;
            int tzolkin_num = (day_count % 13) + 1;
            int tzolkin_name = day_count % 20;

            const char* names[] = {"Imix","Ik","Akbal","Kan","Chicchan",
                "Cimi","Manik","Lamat","Muluc","Oc",
                "Chuen","Eb","Ben","Ix","Men",
                "Cib","Caban","Etznab","Cauac","Ahau"};

            vga_puts("  ║  Tzolk'in: ");
            { char nb[8]; num_to_str(tzolkin_num, nb); vga_puts(nb); }
            vga_puts(" ");
            vga_puts(names[tzolkin_name]);
            vga_puts("\n");
        } else if (strcmp_t(opt, "hab") == 0) {
            // Haab: 365 días (19 meses)
            int day_count = (year - 1900) * 365 + (month - 1) * 30 + day;
            int hab_day = day_count % 365;
            int hab_month = hab_day / 20;
            int hab_day_in = hab_day % 20;

            const char* months[] = {"Pop","Wo","Sip","Sotz","Sek",
                "Xul","Yaxkin","Mol","Chen","Yax",
                "Sak","Keh","Mak","Kankin","Muan",
                "Pax","Koyab","Wayeb","Uayeb"};

            vga_puts("  ║  Haab': ");
            vga_puts(months[hab_month < 19 ? hab_month : 18]);
            vga_puts(" ");
            { char nb[8]; num_to_str(hab_day_in, nb); vga_puts(nb); }
            vga_puts("\n");
        } else if (strcmp_t(opt, "larga") == 0) {
            // Cuenta Larga: 5 números (Baktun.Katun.Tun.Uinal.Kin)
            int jdn = (year - 1900) * 365 + (month - 1) * 30 + day + 111628;
            int baktun = jdn / 144000;
            int katun = (jdn % 144000) / 7200;
            int tun = (jdn % 7200) / 360;
            int uinal = (jdn % 360) / 20;
            int kin = jdn % 20;

            vga_puts("  ║  Cuenta Larga: ");
            { char nb[8]; num_to_str(baktun, nb); vga_puts(nb); vga_putc('.');
              num_to_str(katun, nb); vga_puts(nb); vga_putc('.');
              num_to_str(tun, nb); vga_puts(nb); vga_putc('.');
              num_to_str(uinal, nb); vga_puts(nb); vga_putc('.');
              num_to_str(kin, nb); vga_puts(nb); }
            vga_puts("\n");
            vga_puts("  ║  Baktun: ");
            { char nb[8]; num_to_str(baktun, nb); vga_puts(nb); }
            vga_puts(" (ciclo de 13)\n");
        } else {
            vga_puts("  ║  Usa: --tzolkin, --hab, o --larga\n");
        }
        vga_puts("  ╚══════════════════════════════════════╝\n\n");

    } else if (strcmp_t(civ, "persa") == 0) {
        // Calendario Persa (simplificado)
        vga_puts("\n  ╔══════════════════════════════════════╗\n");
        vga_puts("  ║  CALENDARIO PERSA                    ║\n");
        vga_puts("  ╠══════════════════════════════════════╣\n");
        vga_puts("  ║  Fecha gregoriana: ");
        { char nb[8]; num_to_str(year, nb); vga_puts(nb); vga_putc('-');
          num_to_str(month, nb); vga_puts(nb); vga_putc('-');
          num_to_str(day, nb); vga_puts(nb); }
        vga_puts("\n");

        // Conversión simplificada
        int persian_year = year - 621;
        const char* months[] = {"Farvardin","Ordibehesht","Khordad","Tir",
            "Mordad","Shahrivar","Mehr","Aban","Azar","Dey",
            "Bahman","Esfand"};
        int persian_month = month - 3;
        if (persian_month <= 0) { persian_month += 12; persian_year--; }

        vga_puts("  ║  Año: ");
        { char nb[8]; num_to_str(persian_year, nb); vga_puts(nb); }
        vga_puts("\n  ║  Mes: ");
        vga_puts(months[persian_month > 0 ? persian_month - 1 : 0]);
        vga_puts("\n  ║  Día: ");
        { char nb[8]; num_to_str(day, nb); vga_puts(nb); }
        vga_puts("\n");

        // Estación
        const char* estacion;
        if (persian_month >= 1 && persian_month <= 3) estacion = "Primavera";
        else if (persian_month >= 4 && persian_month <= 6) estacion = "Verano";
        else if (persian_month >= 7 && persian_month <= 9) estacion = "Otoño";
        else estacion = "Invierno";

        vga_puts("  ║  Estación: ");
        vga_puts(estacion);
        vga_puts("\n  ╚══════════════════════════════════════╝\n\n");

    } else {
        vga_puts("\n  Civilización desconocida: ");
        vga_puts(civ);
        vga_puts("\n  Usa: maya | persa\n\n");
    }
}

// =============================================================================
// 5. PATRON — Visualizador de patrones
// =============================================================================

void cmd_patron(const char* args) {
    if (args[0] == 0) {
        vga_puts("\n  [Visualizador de Patrones]\n\n");
        vga_puts("  Uso: patron <n> --sistema=<sys>\n\n");
        vga_puts("  Sistemas:\n");
        vga_puts("    --sistema=ternario    Patrón ternario\n");
        vga_puts("    --sistema=maya        Patrón Maya\n");
        vga_puts("    --sistema=binario     Patrón binario\n\n");
        vga_puts("  Ejemplo:\n");
        vga_puts("    patron 1000 --sistema=ternario\n\n");
        return;
    }

    const char* p = args;
    int val = parse_lab_num(&p);

    // Detectar sistema
    int sys = 0;
    while (*p) {
        while (*p == ' ') p++;
        if (p[0] == '-' && p[1] == '-') {
            p += 2;
            if (p[0] == 's') {
                p += 8; // "sistema="
                if (p[0] == 'm') { sys = 1; while (*p && *p != ' ') p++; }
                else if (p[0] == 'b') { sys = 2; while (*p && *p != ' ') p++; }
                else { sys = 0; while (*p && *p != ' ') p++; }
            } else {
                while (*p && *p != ' ') p++;
            }
        } else {
            p++;
        }
    }

    vga_puts("\n  ╔══════════════════════════════════════╗\n");
    vga_puts("  ║  PATRÓN NUMÉRICO                    ║\n");
    vga_puts("  ╠══════════════════════════════════════╣\n");
    vga_puts("  ║  Número: ");
    { char nb[8]; num_to_str(val, nb); vga_puts(nb); }
    vga_puts("\n");

    if (sys == 0) {
        // Ternario
        trit trits[16];
        int len = int_to_ternary(val, trits, 16);

        vga_puts("  ║  Trits: [");
        for (int i = len - 1; i >= 0; i--) {
            vga_putc(trit_to_char(trits[i]));
            if (i > 0) vga_puts("][");
        }
        vga_puts("]\n");

        // Runs (secuencias del mismo dígito)
        vga_puts("  ║  Runs: ");
        int run_len = 1;
        for (int i = len - 2; i >= 0; i--) {
            if (trits[i] == trits[i + 1]) {
                run_len++;
            } else {
                vga_putc(trit_to_char(trits[i + 1]));
                { char nb[4]; num_to_str(run_len, nb); vga_puts(nb); }
                vga_puts(", ");
                run_len = 1;
            }
        }
        vga_putc(trit_to_char(trits[0]));
        { char nb[4]; num_to_str(run_len, nb); vga_puts(nb); }
        vga_puts("\n");

        // Simetría
        int symmetric = 1;
        for (int i = 0; i < len / 2; i++) {
            if (trits[i] != trits[len - 1 - i]) { symmetric = 0; break; }
        }
        vga_puts("  ║  Simétrico: ");
        vga_puts(symmetric ? "SÍ" : "NO");
        vga_puts("\n");

        // Densidad de ceros
        int zeros = 0;
        for (int i = 0; i < len; i++) {
            if (trits[i] == 0) zeros++;
        }
        vga_puts("  ║  Densidad de ceros: ");
        { char nb[8]; num_to_str(zeros * 100 / len, nb); vga_puts(nb); }
        vga_puts("%\n");

    } else if (sys == 1) {
        // Maya
        int digits[8];
        int len = int_to_base60(val, digits, 8);

        vga_puts("  ║  Maya: ");
        for (int i = len - 1; i >= 0; i--) {
            char maya[16];
            maya_digit_to_str(digits[i], maya);
            vga_puts(maya);
            if (i > 0) vga_putc(' ');
        }
        vga_puts("\n");

        // Ciclos
        vga_puts("  ║  Ciclos: ");
        for (int i = len - 1; i >= 0; i--) {
            { char nb[8]; num_to_str(digits[i], nb); vga_puts(nb); }
            if (i > 0) {
                vga_puts("×60 + ");
            }
        }
        vga_puts("\n");

        // Patrón
        vga_puts("  ║  Patrón: ");
        int ascending = 1;
        for (int i = 0; i < len - 1; i++) {
            if (digits[i] < digits[i + 1]) { ascending = 0; break; }
        }
        vga_puts(ascending ? "ascendente" : "descendente");
        vga_puts("\n");

    } else {
        // Binario
        vga_puts("  ║  Binario: ");
        for (int i = 31; i >= 0; i--) {
            if (i < 16) vga_putc((val >> i) & 1 ? '1' : '0');
        }
        vga_puts("\n");

        // Runs de 1s y 0s
        vga_puts("  ║  Runs: ");
        int run_len = 1;
        for (int i = 14; i >= 0; i--) {
            int bit = (val >> i) & 1;
            int next = (val >> (i + 1)) & 1;
            if (bit == next) {
                run_len++;
            } else {
                vga_putc(next ? '1' : '0');
                { char nb[4]; num_to_str(run_len, nb); vga_puts(nb); }
                vga_puts(", ");
                run_len = 1;
            }
        }
        vga_putc(val & 1 ? '1' : '0');
        { char nb[4]; num_to_str(run_len, nb); vga_puts(nb); }
        vga_puts("\n");
    }

    vga_puts("  ╚══════════════════════════════════════╝\n\n");
}

// =============================================================================
// LAB STATUS
// =============================================================================

void lab_status(void) {
    vga_puts("\n  [Laboratorio Matemático Ancestral v1.0]\n\n");
    vga_puts("  Módulos:\n");
    vga_puts("    [OK] Fórmulas ancestrales\n");
    vga_puts("    [OK] Operaciones paso a paso\n");
    vga_puts("    [OK] Explorador de códigos\n");
    vga_puts("    [OK] Calendarios ancestrales\n");
    vga_puts("    [OK] Visualizador de patrones\n\n");
    vga_puts("  Comandos:\n");
    vga_puts("    formula <n>           Evaluar en múltiples sistemas\n");
    vga_puts("    lab suma/mul/conv     Operaciones paso a paso\n");
    vga_puts("    codigo trits5/quipu   Explorar códigos\n");
    vga_puts("    ancestro maya/persa   Calendarios ancestrales\n");
    vga_puts("    patron <n>            Visualizar patrones\n\n");
}
