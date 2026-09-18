/**
 * ternary_terminal.c — Terminal Binaria/Ternaria Interactiva TRITOS
 *
 * Consola de cómputo ancestral con soporte para:
 * - Decimal, Binario, Ternario, Base 60 (Babilonia)
 * - Ternario balanceado (-1, 0, +1) → (⊖, 0, ⊕)
 * - Aritmética ternaria: +, -, ×, ÷, módulo, potencia
 * - Calendarios: Maya (Tzolkin/Haab/Long Count), Persa, Azteca
 * - Tablas de verdad ternarias (AND, OR, XOR, NOT, IMPLIES, IFF)
 * - Compresión ternaria en tiempo real
 * - Modo REPL interactivo con historial
 *
 * Uso: ./tritos_terminal [comando]
 *   Sin argumentos: REPL interactivo
 *   Con argumento: ejecuta y sale
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>

/* ==================== BALANCED TERNARY ==================== */

/* Balanced ternary digit: -1 (⊖), 0 (0), +1 (⊕) */
typedef int8_t trit_t;

/* Maximum trits for a number */
#define MAX_TRITS 64

/* Balanced ternary number */
typedef struct {
    trit_t d[MAX_TRITS];
    int n;
    int negative;
} bal_ternary_t;

/* Convert decimal to balanced ternary */
bal_ternary_t decimal_to_bal_ternary(int n) {
    bal_ternary_t bt = {.n = 0, .negative = 0};
    if (n < 0) { bt.negative = 1; n = -n; }
    if (n == 0) { bt.d[0] = 0; bt.n = 1; return bt; }

    while (n > 0) {
        int r = n % 3;
        n /= 3;
        if (r == 2) { bt.d[bt.n++] = -1; n++; }      /* ⊖ = -1, carry */
        else if (r == 0) bt.d[bt.n++] = 0;            /* 0 */
        else bt.d[bt.n++] = 1;                         /* ⊕ = +1 */
    }
    return bt;
}

/* Convert balanced ternary to decimal */
int bal_ternary_to_decimal(bal_ternary_t bt) {
    int val = 0;
    int power = 1;
    for (int i = 0; i < bt.n; i++) {
        val += bt.d[i] * power;
        power *= 3;
    }
    return bt.negative ? -val : val;
}

/* Print balanced ternary with symbols */
void bal_ternary_print(bal_ternary_t bt) {
    if (bt.negative) printf("-");
    for (int i = bt.n - 1; i >= 0; i--) {
        switch (bt.d[i]) {
            case -1: printf("⊖"); break;
            case  0: printf("0"); break;
            case  1: printf("⊕"); break;
        }
    }
}

/* Print balanced ternary as digits */
void bal_ternary_print_digits(bal_ternary_t bt) {
    if (bt.negative) printf("-");
    for (int i = bt.n - 1; i >= 0; i--) {
        printf("%d", bt.d[i] + 1);  /* map: -1→0, 0→1, +1→2 */
    }
}

/* ==================== STANDARD TERNARY ==================== */

/* Convert decimal to standard ternary (base 3) */
void decimal_to_ternary(int n, char *buf, int bufsize) {
    if (n == 0) { snprintf(buf, bufsize, "0"); return; }
    char tmp[64];
    int neg = n < 0;
    if (neg) n = -n;
    int i = 0;
    while (n > 0) { tmp[i++] = '0' + (n % 3); n /= 3; }
    int j = 0;
    if (neg) buf[j++] = '-';
    for (int k = i - 1; k >= 0 && j < bufsize - 1; k--) buf[j++] = tmp[k];
    buf[j] = '\0';
}

/* Convert standard ternary string to decimal */
int ternary_to_decimal(const char *s) {
    int val = 0;
    int neg = 0;
    if (*s == '-') { neg = 1; s++; }
    while (*s) {
        val = val * 3 + (*s - '0');
        s++;
    }
    return neg ? -val : val;
}

/* ==================== BASE 60 (BABYLONIAN) ==================== */

/* Convert decimal to base 60 */
void decimal_to_base60(int n, char *buf, int bufsize) {
    if (n == 0) { snprintf(buf, bufsize, "0:0"); return; }
    int neg = n < 0;
    if (neg) n = -n;
    int digits[8];
    int nd = 0;
    while (n > 0) { digits[nd++] = n % 60; n /= 60; }
    int j = 0;
    if (neg) buf[j++] = '-';
    for (int i = nd - 1; i >= 0 && j < bufsize - 1; i--) {
        j += snprintf(buf + j, bufsize - j, "%s%d", (i < nd - 1 && digits[i] < 10 ? "0" : ""), digits[i]);
        if (i > 0 && j < bufsize - 1) buf[j++] = ':';
    }
    buf[j] = '\0';
}

/* Convert base 60 string to decimal */
int base60_to_decimal(const char *s) {
    int val = 0;
    int neg = 0;
    if (*s == '-') { neg = 1; s++; }
    while (*s) {
        int d = 0;
        while (*s && *s != ':') { d = d * 10 + (*s - '0'); s++; }
        val = val * 60 + d;
        if (*s == ':') s++;
    }
    return neg ? -val : val;
}

/* ==================== BINARY ==================== */

/* Convert decimal to binary */
void decimal_to_binary(int n, char *buf, int bufsize) {
    if (n == 0) { snprintf(buf, bufsize, "0"); return; }
    char tmp[64];
    int neg = n < 0;
    if (neg) n = -n;
    int i = 0;
    while (n > 0) { tmp[i++] = '0' + (n % 2); n /= 2; }
    int j = 0;
    if (neg) buf[j++] = '-';
    for (int k = i - 1; k >= 0 && j < bufsize - 1; k--) buf[j++] = tmp[k];
    buf[j] = '\0';
}

/* Convert binary string to decimal */
int binary_to_decimal(const char *s) {
    int val = 0;
    int neg = 0;
    if (*s == '-') { neg = 1; s++; }
    while (*s) { val = val * 2 + (*s - '0'); s++; }
    return neg ? -val : val;
}

/* ==================== ARITHMETIC ==================== */

/* Ternary addition */
int tern_add(int a, int b) { return a + b; }

/* Ternary subtraction */
int tern_sub(int a, int b) { return a - b; }

/* Ternary multiplication */
int tern_mul(int a, int b) { return a * b; }

/* Ternary division */
int tern_div(int a, int b) {
    if (b == 0) { printf("Error: división por cero\n"); return 0; }
    return a / b;
}

/* Ternary modulo */
int tern_mod(int a, int b) {
    if (b == 0) { printf("Error: módulo por cero\n"); return 0; }
    return a % b;
}

/* Ternary power */
int tern_pow(int base, int exp) {
    if (exp < 0) return 0;
    int result = 1;
    for (int i = 0; i < exp; i++) result *= base;
    return result;
}

/* ==================== BALANCED TERNARY ARITHMETIC ==================== */

/* Add two balanced ternary numbers */
bal_ternary_t bal_tern_add(bal_ternary_t a, bal_ternary_t b) {
    int da = bal_ternary_to_decimal(a);
    int db = bal_ternary_to_decimal(b);
    return decimal_to_bal_ternary(da + db);
}

bal_ternary_t bal_tern_sub(bal_ternary_t a, bal_ternary_t b) {
    int da = bal_ternary_to_decimal(a);
    int db = bal_ternary_to_decimal(b);
    return decimal_to_bal_ternary(da - db);
}

bal_ternary_t bal_tern_mul(bal_ternary_t a, bal_ternary_t b) {
    int da = bal_ternary_to_decimal(a);
    int db = bal_ternary_to_decimal(b);
    return decimal_to_bal_ternary(da * db);
}

bal_ternary_t bal_tern_div(bal_ternary_t a, bal_ternary_t b) {
    int da = bal_ternary_to_decimal(a);
    int db = bal_ternary_to_decimal(b);
    if (db == 0) { printf("Error: división por cero\n"); return decimal_to_bal_ternary(0); }
    return decimal_to_bal_ternary(da / db);
}

/* ==================== TRUTH TABLES ==================== */

typedef enum { TF = -1, TU = 0, TT = 1 } tlogic_t;

const char* tlogic_str(tlogic_t v) {
    switch (v) { case TF: return "⊖"; case TU: return "0"; case TT: return "⊕"; default: return "?"; }
}

tlogic_t t_and(tlogic_t a, tlogic_t b) {
    if (a == TF || b == TF) return TF;
    if (a == TT && b == TT) return TT;
    return TU;
}

tlogic_t t_or(tlogic_t a, tlogic_t b) {
    if (a == TT || b == TT) return TT;
    if (a == TF && b == TF) return TF;
    return TU;
}

tlogic_t t_xor(tlogic_t a, tlogic_t b) {
    if (a == TU || b == TU) return TU;
    return (a == b) ? TF : TT;
}

tlogic_t t_not(tlogic_t a) { return -a; }

tlogic_t t_implies(tlogic_t a, tlogic_t b) {
    if (a == TF) return TT;
    if (a == TT && b == TT) return TT;
    if (a == TT && b == TF) return TF;
    return TU;
}

tlogic_t t_iff(tlogic_t a, tlogic_t b) {
    if (a == b) return TT;
    if (a == TU || b == TU) return TU;
    return TF;
}

void print_truth_table(const char *name, tlogic_t (*op)(tlogic_t, tlogic_t)) {
    tlogic_t vals[3] = {TF, TU, TT};
    printf("  %s:\n", name);
    printf("  ┌───┬───┬────┐\n");
    printf("  │ A │ B │ R  │\n");
    printf("  ├───┼───┼────┤\n");
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            printf("  │ %s │ %s │ %s  │\n", tlogic_str(vals[i]), tlogic_str(vals[j]), tlogic_str(op(vals[i], vals[j])));
    printf("  └───┴───┴────┘\n");
}

void print_not_table(void) {
    tlogic_t vals[3] = {TF, TU, TT};
    printf("  NOT:\n");
    printf("  ┌───┬────┐\n");
    printf("  │ A │ R  │\n");
    printf("  ├───┼────┤\n");
    for (int i = 0; i < 3; i++)
        printf("  │ %s │ %s  │\n", tlogic_str(vals[i]), tlogic_str(t_not(vals[i])));
    printf("  └───┴────┘\n");
}

/* ==================== CALENDARS ==================== */

/* Maya calendar */
void calendar_maya(void) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    int doy = tm->tm_yday + 1;

    const char *tzolkin_names[] = {
        "Imix","Ik","Akbal","Kan","Chicchan","Cimi","Manik",
        "Lamat","Muluk","Ok","Chuen","Eb","Ben","Ix",
        "Men","Cib","Caban","Etznab","Cauac","Ahau"
    };
    const char *haab_months[] = {
        "Pop","Wo","Sip","Sotz","Sek","Xul","Yaxkin","Mol","Chen","Yax",
        "Sak","Keh","Mak","Kankin","Muwan","Pax","Koyab","Wayeb",""
    };

    int tzolkin_n = ((doy - 1) % 13) + 1;
    const char *tzolkin_name = tzolkin_names[(doy - 1) % 20];
    int tzolkin_day = ((doy - 1) % 260) + 1;

    /* Haab */
    int haab_day_of_year = ((doy - 1) % 365);
    int haab_month = 0;
    int haab_day = haab_day_of_year;
    int month_days[] = {20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,5};
    for (int i = 0; i < 18; i++) {
        if (haab_day >= month_days[i]) { haab_day -= month_days[i]; haab_month++; }
        else break;
    }

    /* Long Count from Dec 21, 2012 */
    struct tm epoch = {0};
    epoch.tm_year = 112; epoch.tm_mon = 11; epoch.tm_mday = 21;
    epoch.tm_isdst = -1;
    time_t epoch_t = mktime(&epoch);
    long lc_days = (long)difftime(mktime(tm), epoch_t) / 86400;
    int baktun = lc_days / 144000;
    int katun = (lc_days % 144000) / 7200;
    int tun = (lc_days % 7200) / 360;
    int uinal = (lc_days % 360) / 20;
    int kin = lc_days % 20;

    printf("┌─────────────────────────────────────────┐\n");
    printf("│        🌺 CALENDARIO MAYA 🌺           │\n");
    printf("├─────────────────────────────────────────┤\n");
    printf("│  Fecha:    %02d/%02d/%d %02d:%02d         │\n",
           tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900, tm->tm_hour, tm->tm_min);
    printf("├─────────────────────────────────────────┤\n");
    printf("│  Tzolkin:  %d %s (día %d/260)          │\n", tzolkin_n, tzolkin_name, tzolkin_day);
    printf("│  Haab:     %s %d/365                    │\n", haab_months[haab_month], haab_day + 1);
    printf("│  Larga:    %ld días desde 21/12/2012    │\n", lc_days);
    printf("│  Cuenta:   %d.%d.%d.%d.%d               │\n", baktun, katun, tun, uinal, kin);
    printf("└─────────────────────────────────────────┘\n");
}

/* Persian calendar (approximate) */
void calendar_persian(void) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    int greg_day = tm->tm_yday + 1;

    /* Persian year starts ~March 20/21 */
    int persian_year = tm->tm_year + 1900 - 621;
    int persian_day = greg_day - 79;
    if (persian_day <= 0) { persian_day += 365; persian_year--; }

    int persian_month = (persian_day - 1) / 31;
    int persian_day_in_month = (persian_day - 1) % 31 + 1;

    const char *persian_months[] = {
        "Farvardin","Ordibehesht","Khordad","Tir","Mordad","Shahrivar",
        "Mehr","Aban","Azar","Dey","Bahman","Esfand"
    };

    printf("┌─────────────────────────────────────────┐\n");
    printf("│        🏛️  CALENDARIO PERSA 🏛️          │\n");
    printf("├─────────────────────────────────────────┤\n");
    printf("│  Fecha:    %02d/%02d/%d                  │\n", tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);
    printf("├─────────────────────────────────────────┤\n");
    printf("│  solar:    %d %s                    │\n", persian_day_in_month, persian_months[persian_month]);
    printf("│  año:      %d SH (de Solar Hijri)      │\n", persian_year);
    printf("│  día/año:  %d/365                       │\n", persian_day);
    printf("└─────────────────────────────────────────┘\n");
}

/* Aztec calendar (Tonalpohualli - 260 day cycle) */
void calendar_aztec(void) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    int doy = tm->tm_yday + 1;

    const char *day_names[] = {
        "Cipactli","Ehecatl","Calli","Cuetzpalin","Coatl",
        "Miquiztli","Mazatl","Tochtli","Atl","Itzcuintli",
        "Ozomahtli","Malinalli","Acatl","Ocelotl","Cuauhtli",
        "Cozcacuauhtli","Ollin","Tecpatl","Quiahuitl","Xochitl"
    };
    const char *trecena_names[] = {
        "Ce","Ome","Yeyi","Nahui","Macuil","Chicome","Chicuei",
        "Chicnahui","Mahtlactli","Lamat"
    };

    int tonal_day = (doy - 1) % 260;
    int trecena = tonal_day / 13;
    int day_num = tonal_day % 13;

    printf("┌─────────────────────────────────────────┐\n");
    printf("│       🌋 CALENDARIO AZTECA 🌋           │\n");
    printf("├─────────────────────────────────────────┤\n");
    printf("│  Fecha:    %02d/%02d/%d                  │\n", tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);
    printf("├─────────────────────────────────────────┤\n");
    printf("│  Tonalpohualli:                        │\n");
    printf("│    día %d/13 × %d/20                   │\n", day_num + 1, trecena + 1);
    printf("│    %d %s (Trecena %d: %s)        │\n",
           day_num + 1, day_names[tonal_day % 20], trecena + 1, trecena_names[trecena]);
    printf("│    ciclo: %d/260                        │\n", tonal_day + 1);
    printf("└─────────────────────────────────────────┘\n");
}

/* ==================== COMPRESSION ==================== */

/* Ternary run-length encoding */
int ternary_compress(const int *data, int ndata, int *out, int maxout) {
    int oi = 0;
    int i = 0;
    while (i < ndata && oi < maxout - 2) {
        int val = data[i];
        int count = 1;
        while (i + count < ndata && data[i + count] == val && count < 255) count++;
        out[oi++] = val;
        out[oi++] = count;
        i += count;
    }
    return oi;
}

/* Ternary run-length decompression */
int ternary_decompress(const int *data, int ndata, int *out, int maxout) {
    int oi = 0;
    for (int i = 0; i < ndata && i + 1 < ndata; i += 2) {
        int val = data[i];
        int count = data[i + 1];
        for (int j = 0; j < count && oi < maxout; j++) out[oi++] = val;
    }
    return oi;
}

/* ==================== REPL ==================== */

void print_help(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║         🌿 TRITOS — Terminal Binaria / Ternaria 🌿          ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║                                                            ║\n");
    printf("║  CONVERSIÓN:                                               ║\n");
    printf("║    dec <n>          Decimal a todo                         ║\n");
    printf("║    0b<n>           Binario (prefijo 0b)                    ║\n");
    printf("║    0t<n>           Ternario (prefijo 0t)                   ║\n");
    printf("║    bal <n>          Decimal a ternario balanceado            ║\n");
    printf("║    b60 <n>          Decimal a Base 60 (Babilonia)           ║\n");
    printf("║                                                            ║\n");
    printf("║  Aritmética:                                               ║\n");
    printf("║    <a> + <b>        Suma ternaria                           ║\n");
    printf("║    <a> - <b>        Resta ternaria                          ║\n");
    printf("║    <a> * <b>        Multiplicación ternaria                 ║\n");
    printf("║    <a> / <b>        División ternaria                       ║\n");
    printf("║    <a> % <b>        Módulo ternario                         ║\n");
    printf("║    <a> ^ <b>        Potencia ternaria                       ║\n");
    printf("║                                                            ║\n");
    printf("║  Lógica ternaria:                                          ║\n");
    printf("║    and <a> <b>      AND ternario                            ║\n");
    printf("║    or <a> <b>       OR ternario                             ║\n");
    printf("║    xor <a> <b>      XOR ternario                            ║\n");
    printf("║    not <a>          NOT ternario                            ║\n");
    printf("║    implies <a> <b>  IMPLIES ternario                        ║\n");
    printf("║    iff <a> <b>      IFF ternario                            ║\n");
    printf("║    truth            Tablas de verdad completas              ║\n");
    printf("║                                                            ║\n");
    printf("║  Calendarios:                                              ║\n");
    printf("║    maya             Calendario Maya (Tzolkin/Haab/Larga)    ║\n");
    printf("║    persa            Calendario Persa (Solar Hijri)          ║\n");
    printf("║    azteca           Calendario Azteca (Tonalpohualli)       ║\n");
    printf("║                                                            ║\n");
    printf("║  Compresión:                                               ║\n");
    printf("║    compress <a,b..> Comprimir secuencia ternaria (RLE)      ║\n");
    printf("║    decompress <v,c> Descomprimir RLE ternario               ║\n");
    printf("║                                                            ║\n");
    printf("║  Utilidades:                                               ║\n");
    printf("║    help             Mostrar esta ayuda                      ║\n");
    printf("║    exit             Salir                                   ║\n");
    printf("║                                                            ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");
}

/* Parse input, detect if it's a ternary number (contains only 0,1,2) */
int is_ternary_string(const char *s) {
    if (*s == '-') s++;
    if (!*s) return 0;
    while (*s) { if (*s < '0' || *s > '2') return 0; s++; }
    return 1;
}

/* Parse input, detect if it's a binary string */
int is_binary_string(const char *s) {
    if (*s == '-') s++;
    if (!*s) return 0;
    while (*s) { if (*s != '0' && *s != '1') return 0; s++; }
    return 1;
}

/* Parse input, detect if it's a balanced ternary string */
int is_bal_ternary_string(const char *s) {
    if (*s == '-') s++;
    if (!*s) return 0;
    while (*s) {
        if (*s != '0' && *s != '1' && *s != '2') return 0;
        s++;
    }
    return 1;
}

/* Parse number: auto-detect base */
int parse_number(const char *s, const char **used) {
    /* Skip whitespace */
    while (*s == ' ') s++;

    /* Check for prefix */
    if (s[0] == '0' && s[1] == 'b') { *used = s + 2; return binary_to_decimal(s + 2); }
    if (s[0] == '0' && s[1] == 't') { *used = s + 2; return ternary_to_decimal(s + 2); }
    if (s[0] == '0' && s[1] == 'x') { *used = s + 2; return (int)strtol(s + 2, NULL, 16); }

    /* Auto-detect: default to decimal always */
    /* User must prefix 0b for binary, 0t for ternary explicitly */

    /* Default: decimal */
    char *end;
    long v = strtol(s, &end, 10);
    *used = end;
    return (int)v;
}

/* Execute a single command, returns 1 if should exit */
int execute_command(const char *input) {
    char cmd[256];
    strncpy(cmd, input, 255);
    cmd[255] = '\0';

    /* Trim leading/trailing spaces */
    char *start = cmd;
    while (*start == ' ' || *start == '\t') start++;
    char *end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\n')) *end-- = '\0';

    if (!*start) return 0;

    /* Help */
    if (strcmp(start, "help") == 0 || strcmp(start, "?") == 0) { print_help(); return 0; }

    /* Exit */
    if (strcmp(start, "exit") == 0 || strcmp(start, "quit") == 0) return 1;

    /* Truth tables */
    if (strcmp(start, "truth") == 0) {
        printf("\n  Tablas de Verdad Ternarias:\n\n");
        print_truth_table("AND", t_and);
        print_truth_table("OR", t_or);
        print_truth_table("XOR", t_xor);
        print_not_table();
        print_truth_table("IMPLIES", t_implies);
        print_truth_table("IFF", t_iff);
        return 0;
    }

    /* Calendars */
    if (strcmp(start, "maya") == 0) { calendar_maya(); return 0; }
    if (strcmp(start, "persa") == 0) { calendar_persian(); return 0; }
    if (strcmp(start, "azteca") == 0) { calendar_aztec(); return 0; }

    /* Conversion commands */
    if (strncmp(start, "dec ", 4) == 0 || strncmp(start, "decimal ", 8) == 0) {
        const char *arg = start + (start[3] == ' ' ? 4 : 8);
        const char *used;
        int n = parse_number(arg, &used);
        char buf[64];
        printf("\n  Decimal:      %d\n", n);
        decimal_to_binary(n, buf, sizeof(buf));
        printf("  Binario:      0b%s\n", buf);
        decimal_to_ternary(n, buf, sizeof(buf));
        printf("  Ternario:     0t%s\n", buf);
        bal_ternary_t bt = decimal_to_bal_ternary(n);
        printf("  Balanceado:   "); bal_ternary_print(bt); printf("\n");
        decimal_to_base60(n, buf, sizeof(buf));
        printf("  Base 60:      %s\n", buf);
        printf("\n");
        return 0;
    }

    if (strncmp(start, "bin ", 4) == 0 || strncmp(start, "binary ", 7) == 0) {
        const char *arg = start + (start[3] == ' ' ? 4 : 7);
        int n = binary_to_decimal(arg);
        printf("  %s₂ = %d₁₀\n", arg, n);
        return 0;
    }

    if (strncmp(start, "tern ", 5) == 0 || strncmp(start, "ternary ", 8) == 0) {
        const char *arg = start + (start[4] == ' ' ? 5 : 8);
        int n = ternary_to_decimal(arg);
        printf("  %s₃ = %d₁₀\n", arg, n);
        return 0;
    }

    if (strncmp(start, "bal ", 4) == 0 || strncmp(start, "balanced ", 9) == 0) {
        const char *arg = start + (start[3] == ' ' ? 4 : 9);
        const char *used;
        int n = parse_number(arg, &used);
        bal_ternary_t bt = decimal_to_bal_ternary(n);
        printf("  %d → ", n);
        bal_ternary_print(bt);
        printf("  (símbolos)\n");
        printf("  %d → ", n);
        bal_ternary_print_digits(bt);
        printf("  (dígitos)\n");
        return 0;
    }

    if (strncmp(start, "b60 ", 4) == 0 || strncmp(start, "base60 ", 7) == 0) {
        const char *arg = start + (start[3] == ' ' ? 4 : 7);
        const char *used;
        int n = parse_number(arg, &used);
        char buf[64];
        decimal_to_base60(n, buf, sizeof(buf));
        printf("  %d → %s\n", n, buf);
        return 0;
    }

    /* Logic commands */
    if (strncmp(start, "and ", 4) == 0) {
        int a, b;
        const char *p = start + 4;
        const char *used;
        a = parse_number(p, &used);
        p = used;
        while (*p == ' ') p++;
        b = parse_number(p, &used);
        printf("  %d AND %d = %d\n", a, b, t_and(a > 0 ? TT : (a < 0 ? TF : TU), b > 0 ? TT : (b < 0 ? TF : TU)));
        return 0;
    }

    if (strncmp(start, "or ", 3) == 0) {
        int a, b;
        const char *p = start + 3;
        const char *used;
        a = parse_number(p, &used);
        p = used;
        while (*p == ' ') p++;
        b = parse_number(p, &used);
        printf("  %d OR %d = %d\n", a, b, t_or(a > 0 ? TT : (a < 0 ? TF : TU), b > 0 ? TT : (b < 0 ? TF : TU)));
        return 0;
    }

    if (strncmp(start, "not ", 4) == 0) {
        const char *arg = start + 4;
        const char *used;
        int a = parse_number(arg, &used);
        printf("  NOT %d = %d\n", a, t_not(a > 0 ? TT : (a < 0 ? TF : TU)));
        return 0;
    }

    if (strncmp(start, "xor ", 4) == 0) {
        int a, b;
        const char *p = start + 4;
        const char *used;
        a = parse_number(p, &used);
        p = used;
        while (*p == ' ') p++;
        b = parse_number(p, &used);
        printf("  %d XOR %d = %d\n", a, b, t_xor(a > 0 ? TT : (a < 0 ? TF : TU), b > 0 ? TT : (b < 0 ? TF : TU)));
        return 0;
    }

    if (strncmp(start, "implies ", 8) == 0) {
        int a, b;
        const char *p = start + 8;
        const char *used;
        a = parse_number(p, &used);
        p = used;
        while (*p == ' ') p++;
        b = parse_number(p, &used);
        printf("  %d → %d = %d\n", a, b, t_implies(a > 0 ? TT : (a < 0 ? TF : TU), b > 0 ? TT : (b < 0 ? TF : TU)));
        return 0;
    }

    if (strncmp(start, "iff ", 4) == 0) {
        int a, b;
        const char *p = start + 4;
        const char *used;
        a = parse_number(p, &used);
        p = used;
        while (*p == ' ') p++;
        b = parse_number(p, &used);
        printf("  %d ↔ %d = %d\n", a, b, t_iff(a > 0 ? TT : (a < 0 ? TF : TU), b > 0 ? TT : (b < 0 ? TF : TU)));
        return 0;
    }

    /* Compression */
    if (strncmp(start, "compress ", 9) == 0) {
        int data[256], out[512];
        int ndata = 0;
        const char *p = start + 9;
        while (*p && ndata < 256) {
            while (*p == ' ' || *p == ',') p++;
            if (!*p) break;
            data[ndata++] = atoi(p);
            while (*p && *p != ',' && *p != ' ') p++;
        }
        int compressed = ternary_compress(data, ndata, out, 512);
        printf("  Original:     %d trits\n", ndata);
        printf("  Comprimido:   %d valores\n", compressed);
        printf("  Ratio:        %.1f%%\n", 100.0 * compressed / ndata);
        printf("  Datos:        ");
        for (int i = 0; i < compressed; i += 2) printf("[%d×%d] ", out[i], out[i+1]);
        printf("\n");
        return 0;
    }

    if (strncmp(start, "decompress ", 11) == 0) {
        int data[512], out[1024];
        int ndata = 0;
        const char *p = start + 11;
        while (*p && ndata < 512) {
            while (*p == ' ' || *p == ',' || *p == '*') p++;
            if (!*p) break;
            data[ndata++] = atoi(p);
            while (*p && *p != ',' && *p != ' ' && *p != '*') p++;
        }
        int decompressed = ternary_decompress(data, ndata, out, 1024);
        printf("  Comprimido:   %d valores\n", ndata);
        printf("  Original:     %d trits\n", decompressed);
        printf("  Datos:        ");
        for (int i = 0; i < decompressed; i++) printf("%d ", out[i]);
        printf("\n");
        return 0;
    }

    /* Try to parse as expression: a OP b */
    {
        const char *p = start;
        const char *used;
        int a = parse_number(p, &used);

        if (used > start) {
            p = used;
            while (*p == ' ') p++;

            char op = 0;
            if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '%' || *p == '^' || *p == 'x') {
                op = *p;
                if (op == 'x') op = '*';
                p++;
            }

            if (op) {
                while (*p == ' ') p++;
                int b = parse_number(p, &used);
                if (used > p) {
                    int result = 0;
                    switch (op) {
                        case '+': result = tern_add(a, b); break;
                        case '-': result = tern_sub(a, b); break;
                        case '*': result = tern_mul(a, b); break;
                        case '/': result = tern_div(a, b); break;
                        case '%': result = tern_mod(a, b); break;
                        case '^': result = tern_pow(a, b); break;
                    }

                    char buf[64];
                    printf("\n  ─── Aritmética Ternaria ───\n");
                    printf("  Decimal:    %d %c %d = %d\n", a, op, b, result);
                    decimal_to_ternary(a, buf, sizeof(buf));
                    printf("  %s₃", buf);
                    decimal_to_ternary(b, buf, sizeof(buf));
                    printf(" %c %s₃", op, buf);
                    decimal_to_ternary(result, buf, sizeof(buf));
                    printf(" = %s₃\n", buf);
                    bal_ternary_t ba = decimal_to_bal_ternary(a);
                    bal_ternary_t bb = decimal_to_bal_ternary(b);
                    bal_ternary_t br = decimal_to_bal_ternary(result);
                    printf("  Balanceado: ");
                    bal_ternary_print(ba);
                    printf(" %c ", op);
                    bal_ternary_print(bb);
                    printf(" = ");
                    bal_ternary_print(br);
                    printf("\n");
                    printf("\n");
                    return 0;
                }
            }
        }
    }

    /* Unknown command */
    printf("  Comando no reconocido: '%s'\n", start);
    printf("  Escribí 'help' para ver los comandos disponibles.\n");
    return 0;
}

/* ==================== MAIN ==================== */

int main(int argc, char *argv[]) {
    /* Non-interactive mode */
    if (argc > 1) {
        char full[1024] = "";
        for (int i = 1; i < argc; i++) {
            if (i > 1) strcat(full, " ");
            strcat(full, argv[i]);
        }
        return execute_command(full);
    }

    /* Interactive REPL */
    print_help();

    char input[512];
    while (1) {
        printf("  tritos₃> ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;
        if (execute_command(input)) break;
    }

    printf("\n  ¡Hasta luego! 🌿\n\n");
    return 0;
}
