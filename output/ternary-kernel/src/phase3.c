/**
 * phase3.c — Fase 3: Benchmarks, Historial, Modo Dual, Ahorro
 */

#include "../include/ternary.h"

// =============================================================================
// HISTORIAL — persistencia de sesiones
// =============================================================================

#define MAX_HISTORY 32
#define MAX_CMD_LEN 64

static char history[MAX_HISTORY][MAX_CMD_LEN];
static int history_count = 0;
static int history_pos = 0;

// Add command to history
void history_add(const char* cmd) {
    if (history_count >= MAX_HISTORY) {
        // Shift down
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            strcpy_t(history[i], history[i + 1]);
        }
        history_count = MAX_HISTORY - 1;
    }
    
    strcpy_t(history[history_count], cmd);
    history_count++;
}

// Show history
void history_show(void) {
    vga_puts("\n  [Session History]\n\n");
    
    if (history_count == 0) {
        vga_puts("  No commands yet.\n\n");
        return;
    }
    
    for (int i = 0; i < history_count; i++) {
        vga_puts("  ");
        { char nb[4]; num_to_str(i + 1, nb); vga_puts(nb); }
        vga_puts(". ");
        vga_puts(history[i]);
        vga_puts("\n");
    }
    vga_puts("\n");
}

// Save history to file
void history_save(void) {
    int fd = fs_create("HISTORY.TRI");
    if (fd < 0) return;
    
    int8_t f = fs_open("HISTORY.TRI", 1);
    if (f < 0) return;
    
    for (int i = 0; i < history_count; i++) {
        fs_write(f, (uint8_t*)history[i], strlen_t(history[i]));
        fs_write(f, (uint8_t*)"\n", 1);
    }
    
    fs_close(f);
    vga_puts("  History saved to HISTORY.TRI\n");
}

// Load history from file
void history_load(void) {
    int fd = fs_open("HISTORY.TRI", 0);
    if (fd < 0) return;
    
    int32_t size = fs_get_size("HISTORY.TRI");
    if (size <= 0) { fs_close(fd); return; }
    
    char buf[2048];
    int32_t read = fs_read(fd, (uint8_t*)buf, size < 2047 ? size : 2047);
    fs_close(fd);
    
    if (read <= 0) return;
    
    history_count = 0;
    int line_start = 0;
    
    for (int i = 0; i <= read; i++) {
        if (i == read || buf[i] == '\n') {
            if (history_count < MAX_HISTORY) {
                int len = i - line_start;
                if (len >= MAX_CMD_LEN) len = MAX_CMD_LEN - 1;
                memcpy_t(history[history_count], &buf[line_start], len);
                history[history_count][len] = 0;
                history_count++;
            }
            line_start = i + 1;
        }
    }
}

// Command: hist
void cmd_hist(const char* args) {
    if (strcmp_t(args, "save") == 0) {
        history_save();
    } else if (strcmp_t(args, "load") == 0) {
        history_load();
        vga_puts("  History loaded\n");
    } else {
        history_show();
    }
}

// =============================================================================
// BENCHMARKS — comparación binario vs ternario
// =============================================================================

// Count bits needed
static int count_bits(int n) {
    if (n == 0) return 1;
    int bits = 0;
    int num = n < 0 ? -n : n;
    while (num > 0) { bits++; num >>= 1; }
    return bits;
}

// Count trits needed
static int count_trits(int n) {
    if (n == 0) return 1;
    int trits = 0;
    int num = n < 0 ? -n : n;
    while (num > 0) { trits++; num /= 3; }
    return trits;
}

// Benchmark: addition
static void bench_add(int a, int b) {
    int result = a + b;
    int bin_bits = count_bits(a) + count_bits(b) + count_bits(result);
    int ter_trits = count_trits(a) + count_trits(b) + count_trits(result);
    
    vga_puts("  ADD ");
    { char nb[8]; num_to_str(a, nb); vga_puts(nb); }
    vga_puts(" + ");
    { char nb[8]; num_to_str(b, nb); vga_puts(nb); }
    vga_puts(" = ");
    { char nb[8]; num_to_str(result, nb); vga_puts(nb); }
    vga_puts("\n");
    vga_puts("    Binary:  ");
    { char nb[4]; num_to_str(bin_bits, nb); vga_puts(nb); }
    vga_puts(" bits\n");
    vga_puts("    Ternary: ");
    { char nb[4]; num_to_str(ter_trits, nb); vga_puts(nb); }
    vga_puts(" trits\n");
}

// Benchmark: multiplication
static void bench_mul(int a, int b) {
    int result = a * b;
    int bin_bits = count_bits(a) + count_bits(b) + count_bits(result);
    int ter_trits = count_trits(a) + count_trits(b) + count_trits(result);
    
    vga_puts("  MUL ");
    { char nb[8]; num_to_str(a, nb); vga_puts(nb); }
    vga_puts(" * ");
    { char nb[8]; num_to_str(b, nb); vga_puts(nb); }
    vga_puts(" = ");
    { char nb[8]; num_to_str(result, nb); vga_puts(nb); }
    vga_puts("\n");
    vga_puts("    Binary:  ");
    { char nb[4]; num_to_str(bin_bits, nb); vga_puts(nb); }
    vga_puts(" bits\n");
    vga_puts("    Ternary: ");
    { char nb[4]; num_to_str(ter_trits, nb); vga_puts(nb); }
    vga_puts(" trits\n");
}

// Command: bench
void cmd_bench(const char* args) {
    vga_puts("\n  [Binary vs Ternary Benchmarks]\n\n");
    
    vga_puts("  --- Addition ---\n");
    bench_add(5, 3);
    bench_add(100, 200);
    bench_add(1024, 2048);
    bench_add(65535, 1);
    
    vga_puts("\n  --- Multiplication ---\n");
    bench_mul(5, 3);
    bench_mul(100, 100);
    bench_mul(32, 32);
    bench_mul(256, 256);
    
    vga_puts("\n  --- Storage Efficiency ---\n");
    
    int values[] = {1, 2, 3, 7, 15, 31, 63, 127, 255, 1023, 4095};
    int count = 11;
    
    vga_puts("  Value  Binary  Ternary  Savings\n");
    vga_puts("  -----  ------  -------  -------\n");
    
    int total_bin = 0, total_ter = 0;
    
    for (int i = 0; i < count; i++) {
        int v = values[i];
        int b = count_bits(v);
        int t = count_trits(v);
        total_bin += b;
        total_ter += t;
        
        vga_puts("  ");
        { char nb[6]; num_to_str(v, nb); vga_puts(nb); }
        
        // Padding
        if (v < 10) vga_puts("    ");
        else if (v < 100) vga_puts("   ");
        else if (v < 1000) vga_puts("  ");
        else vga_puts(" ");
        
        { char nb[4]; num_to_str(b, nb); vga_puts(nb); }
        vga_puts("      ");
        { char nb[4]; num_to_str(t, nb); vga_puts(nb); }
        vga_puts("       ");
        
        if (b > t) {
            int saving = (b - t) * 100 / b;
            { char nb[4]; num_to_str(saving, nb); vga_puts(nb); }
            vga_puts("%");
        }
        vga_puts("\n");
    }
    
    vga_puts("\n  Total: Binary=");
    { char nb[4]; num_to_str(total_bin, nb); vga_puts(nb); }
    vga_puts(" bits, Ternary=");
    { char nb[4]; num_to_str(total_ter, nb); vga_puts(nb); }
    vga_puts(" trits\n");
    
    if (total_bin > total_ter) {
        int saving = (total_bin - total_ter) * 100 / total_bin;
        vga_puts("  Overall savings: ");
        { char nb[4]; num_to_str(saving, nb); vga_puts(nb); }
        vga_puts("%\n");
    }
    vga_puts("\n");
}

// =============================================================================
// MODO DUAL — Linux ↔ ternario
// =============================================================================

static uint8_t dual_mode = 0; // 0=ternary, 1=linux

// Command: dual
void cmd_dual(const char* args) {
    if (strcmp_t(args, "linux") == 0 || strcmp_t(args, "l") == 0) {
        dual_mode = 1;
        vga_puts("\n  Switched to Linux mode\n");
        vga_puts("  (Use 'dual ternary' to switch back)\n\n");
        vga_puts("  Type shell commands directly.\n");
        vga_puts("  Example: ls, cat, echo, etc.\n\n");
    } else if (strcmp_t(args, "ternary") == 0 || strcmp_t(args, "t") == 0) {
        dual_mode = 0;
        vga_puts("\n  Switched to Ternary mode\n");
        vga_puts("  Use: conv, repl, ver, tutorial, tri\n\n");
    } else if (strcmp_t(args, "status") == 0 || strcmp_t(args, "s") == 0) {
        vga_puts("\n  Mode: ");
        vga_puts(dual_mode ? "Linux" : "Ternary");
        vga_puts("\n\n");
    } else {
        vga_puts("\n  [Dual Mode]\n\n");
        vga_puts("  Switch between Linux and Ternary modes:\n\n");
        vga_puts("    dual linux    - Switch to Linux shell\n");
        vga_puts("    dual ternary  - Switch to Ternary mode\n");
        vga_puts("    dual status   - Show current mode\n\n");
    }
}

// Get current mode
uint8_t dual_get_mode(void) {
    return dual_mode;
}

// =============================================================================
// VISUALIZACIÓN DE AHORRO — estadísticas detalladas
// =============================================================================

// Command: savings
void cmd_savings(const char* args) {
    vga_puts("\n  [Ternary Savings Analysis]\n\n");
    
    // Information density
    vga_puts("  Information Density:\n");
    vga_puts("    Binary:  log2(n) bits per number\n");
    vga_puts("    Ternary: log3(n) trits per number\n");
    vga_puts("    Ratio:   log2(3) = 1.585x more efficient\n\n");
    
    // Practical comparison
    vga_puts("  Practical Comparison:\n\n");
    
    struct { int max_val; int bin_bits; int ter_trits; } ranges[] = {
        {15, 4, 3},
        {63, 6, 4},
        {255, 8, 5},
        {1023, 10, 7},
        {65535, 16, 10},
        {1048575, 20, 13}
    };
    
    vga_puts("  Max Value    Binary    Ternary    Savings\n");
    vga_puts("  ---------    ------    -------    -------\n");
    
    for (int i = 0; i < 6; i++) {
        vga_puts("  ");
        { char nb[10]; num_to_str(ranges[i].max_val, nb); vga_puts(nb); }
        
        // Padding
        { char nb[10]; num_to_str(ranges[i].max_val, nb);
          for (int j = strlen_t(nb); j < 10; j++) vga_puts(" "); }
        
        { char nb[4]; num_to_str(ranges[i].bin_bits, nb); vga_puts(nb); }
        vga_puts(" bits     ");
        { char nb[4]; num_to_str(ranges[i].ter_trits, nb); vga_puts(nb); }
        vga_puts(" trits    ");
        
        int saving = (ranges[i].bin_bits - ranges[i].ter_trits) * 100 / ranges[i].bin_bits;
        { char nb[4]; num_to_str(saving, nb); vga_puts(nb); }
        vga_puts("%\n");
    }
    
    vga_puts("\n  Key Insight:\n");
    vga_puts("    Ternary uses ~37% fewer symbols than binary\n");
    vga_puts("    for the same range of numbers.\n\n");
    
    vga_puts("  Real-world applications:\n");
    vga_puts("    - IoT sensors: smaller data packets\n");
    vga_puts("    - Data compression: more efficient encoding\n");
    vga_puts("    - Memory: less storage per value\n");
    vga_puts("    - Transmission: fewer bits to send\n\n");
}

// =============================================================================
// RESUMEN — comandos Fase 3
// =============================================================================

void phase3_status(void) {
    vga_puts("\n  [Phase 3: Education Complete]\n\n");
    vga_puts("  Commands:\n");
    vga_puts("    bench       Binary vs ternary benchmarks\n");
    vga_puts("    savings     Savings analysis\n");
    vga_puts("    history     Session history\n");
    vga_puts("    dual <mode> Switch Linux/Ternary mode\n\n");
}
