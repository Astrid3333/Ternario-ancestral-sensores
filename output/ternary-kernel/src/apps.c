/**
 * apps.c — Aplicaciones de usuario para Tritos
 *
 * Calculadora, editor, juegos, reloj, conversor
 */

#include "../include/ternary.h"

// =============================================================================
// CALCULADORA TERNARIA
// =============================================================================

static int calc_parse_number(const char** p) {
    while (**p == ' ') (*p)++;
    
    int result = 0;
    int neg = 0;
    
    if (**p == '-') { neg = 1; (*p)++; }
    
    while (**p >= '0' && **p <= '9') {
        result = result * 10 + (**p - '0');
        (*p)++;
    }
    
    return neg ? -result : result;
}

static int calc_expr(const char** p);

static int calc_primary(const char** p) {
    while (**p == ' ') (*p)++;
    
    if (**p == '(') {
        (*p)++;
        int val = calc_expr(p);
        while (**p == ' ') (*p)++;
        if (**p == ')') (*p)++;
        return val;
    }
    
    return calc_parse_number(p);
}

static int calc_mul_div(const char** p) {
    int left = calc_primary(p);
    
    while (1) {
        while (**p == ' ') (*p)++;
        
        if (**p == '*') {
            (*p)++;
            left *= calc_primary(p);
        } else if (**p == '/') {
            (*p)++;
            int right = calc_primary(p);
            if (right != 0) left /= right;
        } else {
            break;
        }
    }
    
    return left;
}

static int calc_expr(const char** p) {
    int left = calc_mul_div(p);
    
    while (1) {
        while (**p == ' ') (*p)++;
        
        if (**p == '+') {
            (*p)++;
            left += calc_mul_div(p);
        } else if (**p == '-') {
            (*p)++;
            left -= calc_mul_div(p);
        } else {
            break;
        }
    }
    
    return left;
}

// Convert to ternary string
static void calc_to_ternary(int n, char* out) {
    if (n == 0) { out[0] = '0'; out[1] = 0; return; }
    
    char temp[32];
    int len = 0;
    int num = n < 0 ? -n : n;
    
    while (num > 0) {
        int rem = num % 3;
        num = num / 3;
        if (rem == 2) { temp[len++] = '-'; num++; }
        else { temp[len++] = '0' + rem; }
    }
    
    if (n < 0) { out[0] = '-'; for (int i = 0; i < len; i++) out[i+1] = temp[len-1-i]; out[len+1]=0; }
    else { for (int i = 0; i < len; i++) out[i] = temp[len-1-i]; out[len]=0; }
}

// Command: calc
void cmd_calc(const char* args) {
    if (args[0] == 0) {
        vga_puts("\n  [Ternary Calculator]\n\n");
        vga_puts("  Usage: calc <expression>\n");
        vga_puts("  Example: calc 5 + 3 * 2\n\n");
        vga_puts("  Supports: +, -, *, /, ()\n");
        vga_puts("  Output: decimal and ternary\n\n");
        return;
    }
    
    const char* p = args;
    int result = calc_expr(&p);
    
    char ter[32];
    calc_to_ternary(result, ter);
    
    vga_puts("\n  Result: ");
    { char nb[8]; num_to_str(result, nb); vga_puts(nb); }
    vga_puts("\n  Ternary: ");
    vga_puts(ter);
    vga_puts("\n\n");
}

// =============================================================================
// RELOJ
// =============================================================================

static uint32_t clock_seconds = 0;
static uint8_t clock_initialized = 0;

// Initialize clock (from RTC or PIT)
void clock_init(void) {
    // Read CMOS RTC
    outb(0x70, 0x00); // Seconds
    uint8_t sec = inb(0x71);
    outb(0x70, 0x02); // Minutes
    uint8_t min = inb(0x71);
    outb(0x70, 0x04); // Hours
    uint8_t hour = inb(0x71);
    
    // Convert BCD
    sec = (sec & 0x0F) + ((sec >> 4) * 10);
    min = (min & 0x0F) + ((min >> 4) * 10);
    hour = (hour & 0x0F) + ((hour >> 4) * 10);
    
    clock_seconds = hour * 3600 + min * 60 + sec;
    clock_initialized = 1;
}

// Update clock (called from timer)
void clock_tick(void) {
    clock_seconds++;
    if (clock_seconds >= 86400) clock_seconds = 0; // 24 hours
}

// Get time components
void clock_get_time(uint8_t* hour, uint8_t* min, uint8_t* sec) {
    uint32_t t = clock_seconds;
    if (hour) *hour = t / 3600;
    if (min) *min = (t % 3600) / 60;
    if (sec) *sec = t % 60;
}

// Command: clock
void cmd_clock(const char* args) {
    if (!clock_initialized) clock_init();
    
    uint8_t h, m, s;
    clock_get_time(&h, &m, &s);
    
    vga_puts("\n  [Clock]\n\n");
    vga_puts("  Time: ");
    { char nb[3]; num_to_str(h, nb); if (h < 10) vga_putc('0'); vga_puts(nb); }
    vga_puts(":");
    { char nb[3]; num_to_str(m, nb); if (m < 10) vga_putc('0'); vga_puts(nb); }
    vga_puts(":");
    { char nb[3]; num_to_str(s, nb); if (s < 10) vga_putc('0'); vga_puts(nb); }
    vga_puts("\n\n");
    
    vga_puts("  Ternary time: ");
    // Convert to base-3 representation
    int total = h * 3600 + m * 60 + s;
    char ter[32];
    calc_to_ternary(total, ter);
    vga_puts(ter);
    vga_puts("\n\n");
}

// =============================================================================
// CONVERSOR DE UNIDADES
// =============================================================================

// Command: convert
void cmd_convert(const char* args) {
    if (args[0] == 0) {
        vga_puts("\n  [Unit Converter]\n\n");
        vga_puts("  Usage: convert <value> <from> <to>\n\n");
        vga_puts("  Length:\n");
        vga_puts("    m, km, cm, mm, mi, ft, in\n\n");
        vga_puts("  Weight:\n");
        vga_puts("    kg, g, mg, lb, oz\n\n");
        vga_puts("  Temperature:\n");
        vga_puts("    c, f, k\n\n");
        vga_puts("  Example: convert 100 m ft\n\n");
        return;
    }
    
    // Parse: value from to
    const char* p = args;
    double value = 0;
    int neg = 0;
    
    while (*p == ' ') p++;
    if (*p == '-') { neg = 1; p++; }
    while (*p >= '0' && *p <= '9') {
        value = value * 10 + (*p - '0');
        p++;
    }
    if (*p == '.') {
        p++;
        double dec = 0.1;
        while (*p >= '0' && *p <= '9') {
            value += (*p - '0') * dec;
            dec /= 10;
            p++;
        }
    }
    if (neg) value = -value;
    
    while (*p == ' ') p++;
    
    // Parse from unit
    char from[8] = {0};
    int i = 0;
    while (*p && *p != ' ' && i < 7) from[i++] = *p++;
    from[i] = 0;
    
    while (*p == ' ') p++;
    
    // Parse to unit
    char to[8] = {0};
    i = 0;
    while (*p && *p != ' ' && i < 7) to[i++] = *p++;
    to[i] = 0;
    
    // Conversion factors (to base unit)
    double result = value;
    int converted = 0;
    
    // Length to meters
    double from_m = 1.0, to_m = 1.0;
    if (strcmp_t(from, "km") == 0) from_m = 1000;
    else if (strcmp_t(from, "cm") == 0) from_m = 0.01;
    else if (strcmp_t(from, "mm") == 0) from_m = 0.001;
    else if (strcmp_t(from, "mi") == 0) from_m = 1609.344;
    else if (strcmp_t(from, "ft") == 0) from_m = 0.3048;
    else if (strcmp_t(from, "in") == 0) from_m = 0.0254;
    
    if (strcmp_t(to, "km") == 0) to_m = 1000;
    else if (strcmp_t(to, "cm") == 0) to_m = 0.01;
    else if (strcmp_t(to, "mm") == 0) to_m = 0.001;
    else if (strcmp_t(to, "mi") == 0) to_m = 1609.344;
    else if (strcmp_t(to, "ft") == 0) to_m = 0.3048;
    else if (strcmp_t(to, "in") == 0) to_m = 0.0254;
    
    if (strcmp_t(from, "m") == 0 || strcmp_t(from, "km") == 0 || 
        strcmp_t(from, "cm") == 0 || strcmp_t(from, "mm") == 0 ||
        strcmp_t(from, "mi") == 0 || strcmp_t(from, "ft") == 0 ||
        strcmp_t(from, "in") == 0) {
        if (strcmp_t(to, "m") == 0 || strcmp_t(to, "km") == 0 || 
            strcmp_t(to, "cm") == 0 || strcmp_t(to, "mm") == 0 ||
            strcmp_t(to, "mi") == 0 || strcmp_t(to, "ft") == 0 ||
            strcmp_t(to, "in") == 0) {
            result = value * from_m / to_m;
            converted = 1;
        }
    }
    
    // Weight
    double from_g = 1.0, to_g = 1.0;
    if (strcmp_t(from, "kg") == 0) from_g = 1000;
    else if (strcmp_t(from, "mg") == 0) from_g = 0.001;
    else if (strcmp_t(from, "lb") == 0) from_g = 453.592;
    else if (strcmp_t(from, "oz") == 0) from_g = 28.3495;
    
    if (strcmp_t(to, "kg") == 0) to_g = 1000;
    else if (strcmp_t(to, "mg") == 0) to_g = 0.001;
    else if (strcmp_t(to, "lb") == 0) to_g = 453.592;
    else if (strcmp_t(to, "oz") == 0) to_g = 28.3495;
    
    if (strcmp_t(from, "g") == 0 || strcmp_t(from, "kg") == 0 || 
        strcmp_t(from, "mg") == 0 || strcmp_t(from, "lb") == 0 ||
        strcmp_t(from, "oz") == 0) {
        if (strcmp_t(to, "g") == 0 || strcmp_t(to, "kg") == 0 || 
            strcmp_t(to, "mg") == 0 || strcmp_t(to, "lb") == 0 ||
            strcmp_t(to, "oz") == 0) {
            result = value * from_g / to_g;
            converted = 1;
        }
    }
    
    // Temperature
    if ((strcmp_t(from, "c") == 0 && strcmp_t(to, "f") == 0)) {
        result = value * 9.0 / 5.0 + 32;
        converted = 1;
    } else if (strcmp_t(from, "f") == 0 && strcmp_t(to, "c") == 0) {
        result = (value - 32) * 5.0 / 9.0;
        converted = 1;
    } else if (strcmp_t(from, "c") == 0 && strcmp_t(to, "k") == 0) {
        result = value + 273.15;
        converted = 1;
    } else if (strcmp_t(from, "k") == 0 && strcmp_t(to, "c") == 0) {
        result = value - 273.15;
        converted = 1;
    } else if (strcmp_t(from, "f") == 0 && strcmp_t(to, "k") == 0) {
        result = (value - 32) * 5.0 / 9.0 + 273.15;
        converted = 1;
    } else if (strcmp_t(from, "k") == 0 && strcmp_t(to, "f") == 0) {
        result = (value - 273.15) * 9.0 / 5.0 + 32;
        converted = 1;
    }
    
    if (converted) {
        vga_puts("\n  ");
        { char nb[8]; num_to_str((int)value, nb); vga_puts(nb); }
        vga_puts(" ");
        vga_puts(from);
        vga_puts(" = ");
        { char nb[8]; num_to_str((int)result, nb); vga_puts(nb); }
        vga_puts(" ");
        vga_puts(to);
        vga_puts("\n\n");
    } else {
        vga_puts("  Unknown conversion: ");
        vga_puts(from);
        vga_puts(" → ");
        vga_puts(to);
        vga_puts("\n\n");
    }
}

// =============================================================================
// JUEGO: TERNARY TETRIS
// =============================================================================

#define TETRIS_W 10
#define TETRIS_H 20

static uint8_t tetris_board[TETRIS_H][TETRIS_W];
static int tetris_piece, tetris_x, tetris_y;
static int tetris_score = 0;
static int tetris_game_over = 0;

// Piece shapes (4x4)
static const uint8_t tetris_pieces[7][4][4] = {
    // I
    {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}},
    // O
    {{0,0,0,0}, {0,1,1,0}, {0,1,1,0}, {0,0,0,0}},
    // T
    {{0,0,0,0}, {0,1,0,0}, {1,1,1,0}, {0,0,0,0}},
    // S
    {{0,0,0,0}, {0,1,1,0}, {1,1,0,0}, {0,0,0,0}},
    // Z
    {{0,0,0,0}, {1,1,0,0}, {0,1,1,0}, {0,0,0,0}},
    // J
    {{0,0,0,0}, {1,0,0,0}, {1,1,1,0}, {0,0,0,0}},
    // L
    {{0,0,0,0}, {0,0,1,0}, {1,1,1,0}, {0,0,0,0}}
};

// Piece colors (ternary palette)
static const uint32_t tetris_colors[7] = {
    0x0077B6, // I - blue
    0xE85D04, // O - orange
    0x7209B7, // T - purple
    0x2D6A4F, // S - green
    0xE63946, // Z - red
    0x3A0CA3, // J - indigo
    0xF77F00  // L - yellow
};

static void tetris_new_piece(void) {
    tetris_piece = 0; // Will be randomized
    tetris_x = TETRIS_W / 2 - 2;
    tetris_y = 0;
    tetris_game_over = 0;
}

static void tetris_init(void) {
    for (int y = 0; y < TETRIS_H; y++)
        for (int x = 0; x < TETRIS_W; x++)
            tetris_board[y][x] = 0;
    tetris_score = 0;
    tetris_new_piece();
}

static int tetris_check(int dx, int dy) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (tetris_pieces[tetris_piece][y][x]) {
                int nx = tetris_x + x + dx;
                int ny = tetris_y + y + dy;
                if (nx < 0 || nx >= TETRIS_W || ny >= TETRIS_H) return 0;
                if (ny >= 0 && tetris_board[ny][nx]) return 0;
            }
        }
    }
    return 1;
}

static void tetris_lock(void) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (tetris_pieces[tetris_piece][y][x]) {
                int nx = tetris_x + x;
                int ny = tetris_y + y;
                if (ny >= 0 && ny < TETRIS_H && nx >= 0 && nx < TETRIS_W) {
                    tetris_board[ny][nx] = tetris_piece + 1;
                }
            }
        }
    }
    
    // Clear lines
    for (int y = TETRIS_H - 1; y >= 0; y--) {
        int full = 1;
        for (int x = 0; x < TETRIS_W; x++) {
            if (!tetris_board[y][x]) { full = 0; break; }
        }
        if (full) {
            // Shift down
            for (int yy = y; yy > 0; yy--) {
                for (int x = 0; x < TETRIS_W; x++) {
                    tetris_board[yy][x] = tetris_board[yy-1][x];
                }
            }
            tetris_score += 100;
            y++; // Check same line again
        }
    }
    
    tetris_new_piece();
    if (!tetris_check(0, 0)) tetris_game_over = 1;
}

static void tetris_draw(void) {
    vga_puts("\n  [Ternary Tetris] Score: ");
    { char nb[8]; num_to_str(tetris_score, nb); vga_puts(nb); }
    vga_puts("\n\n");
    
    // Draw board
    for (int y = 0; y < TETRIS_H; y++) {
        vga_puts("  ");
        for (int x = 0; x < TETRIS_W; x++) {
            int drawn = 0;
            
            // Check current piece
            for (int py = 0; py < 4; py++) {
                for (int px = 0; px < 4; px++) {
                    if (tetris_pieces[tetris_piece][py][px]) {
                        if (tetris_x + px == x && tetris_y + py == y) {
                            vga_putc('#');
                            drawn = 1;
                        }
                    }
                }
            }
            
            if (!drawn) {
                if (tetris_board[y][x]) {
                    vga_putc('O');
                } else {
                    vga_putc('.');
                }
            }
        }
        vga_puts("\n");
    }
    
    vga_puts("\n  Controls: a=left, d=right, s=down, w=rotate\n");
    vga_puts("  Press 'q' to quit\n\n");
}

// Command: tetris
void cmd_tetris(const char* args) {
    if (strcmp_t(args, "quit") == 0 || strcmp_t(args, "q") == 0) {
        vga_puts("  Game over! Final score: ");
        { char nb[8]; num_to_str(tetris_score, nb); vga_puts(nb); }
        vga_puts("\n");
        return;
    }
    
    if (strcmp_t(args, "start") == 0 || strcmp_t(args, "") == 0) {
        tetris_init();
        vga_puts("\n  [Starting Ternary Tetris]\n\n");
        tetris_draw();
        return;
    }
    
    if (tetris_game_over) {
        vga_puts("  Game over! Score: ");
        { char nb[8]; num_to_str(tetris_score, nb); vga_puts(nb); }
        vga_puts("\n  Type 'tetris start' to play again\n");
        return;
    }
    
    // Process move
    if (strcmp_t(args, "a") == 0) {
        if (tetris_check(-1, 0)) tetris_x--;
    } else if (strcmp_t(args, "d") == 0) {
        if (tetris_check(1, 0)) tetris_x++;
    } else if (strcmp_t(args, "s") == 0) {
        if (tetris_check(0, 1)) {
            tetris_y++;
        } else {
            tetris_lock();
        }
    } else if (strcmp_t(args, "w") == 0) {
        // Rotate (simplified - just try different pieces)
        int old = tetris_piece;
        tetris_piece = (tetris_piece + 1) % 7;
        if (!tetris_check(0, 0)) tetris_piece = old;
    }
    
    tetris_draw();
}

// =============================================================================
// STATUS
// =============================================================================

void apps_status(void) {
    vga_puts("\n  [Tritos Applications]\n\n");
    vga_puts("  Commands:\n");
    vga_puts("    calc <expr>       Ternary calculator\n");
    vga_puts("    clock             Show time\n");
    vga_puts("    convert <v> <f> <t> Unit converter\n");
    vga_puts("    tetris            Play Tetris\n\n");
}
