/**
 * framebuffer.c — Framebuffer para GUI ternaria ancestral
 *
 * Dibujar píxeles, líneas, rectángulos, texto con colores ternarios
 */

#include "../include/ternary.h"

// Framebuffer info
static uint32_t* framebuffer = 0;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint32_t fb_pitch = 0;
static uint8_t fb_bpp = 0;

// =============================================================================
// TERNARY COLOR PALETTE — Colores basados en el sistema ternario (base-3)
// Los 3 dígitos ternarios generan una paleta de 27 colores (3³)
// =============================================================================

// Ternary primaries: Oth (fuego), Haab (agua), Tzolkin (tierra)
#define TRIT_OTH        0xE85D04  // Naranja fuego
#define TRIT_HAAB       0x0077B6  // Azul agua
#define TRIT_TZOLKIN    0x2D6A4F  // Verde tierra

// Ternary secondaries: combinaciones de 2
#define TRIT_OTH_HAAB   0x9B5DE5  // Púrpura (fuego+agua)
#define TRIT_OTH_TZOL   0xF4A261  // Dorado (fuego+tierra)
#define TRIT_HAAB_TZOL  0x00B4D8  // Cian (agua+tierra)

// Ternary tertiary: combinaciones de 3
#define TRIT_FULL       0xF72585  // Magenta (todos)
#define TRIT_EMPTY      0x1B1B2E  // Vacío (ninguno)

// Ternary intensity levels (0, 1, 2) × 3 channels = 27 colors
#define TRIT_000 0x0B0B1A  // Vacío total (negro ternario)
#define TRIT_001 0x003566  // Agua pura
#define TRIT_002 0x0077B6  // Agua fuerte
#define TRIT_010 0x0D4A0D  // Tierra pura
#define TRIT_011 0x2D6A4F  // Tierra fuerte
#define TRIT_012 0x40916C  // Tierra + agua
#define TRIT_020 0x9B2226  // Fuego puro
#define TRIT_021 0xBB3E03  // Fuego fuerte
#define TRIT_022 0xE85D04  // Fuego intenso
#define TRIT_100 0x3A0CA3  // Espíritu
#define TRIT_101 0x7209B7  // Espíritu + agua
#define TRIT_102 0x560BAD  // Espíritu + fuego
#define TRIT_110 0x38B000  // Vida
#define TRIT_111 0x70E000  // Vida brillante
#define TRIT_112 0xCCD700  // Vida dorada
#define TRIT_120 0xF77F00  // Sol
#define TRIT_121 0xFCBF49  // Sol brillante
#define TRIT_122 0xFFD166  // Sol dorado
#define TRIT_200 0xE63946  // Sangre
#define TRIT_201 0xD62828  // Sangre oscura
#define TRIT_202 0xC1121F  // Sangre intensa
#define TRIT_210 0xF4845F  // Coral
#define TRIT_211 0xF7B267  // Arena
#define TRIT_212 0xF7D794  // Crema
#define TRIT_220 0xFFFFFF  // Luz total (blanco)
#define TRIT_221 0xE0E0E0  // Luz suave
#define TRIT_222 0xCCCCCC  // Luz media

// Desktop theme — Ternary ancestral
#define DESKTOP_BG      TRIT_000    // Fondo: vacío ternario
#define TITLE_BAR_BG    TRIT_100    // Título: espíritu
#define TITLE_BAR_FOCUSED TRIT_200  // Título activo: sangre
#define TITLE_TEXT       TRIT_221    // Texto: luz suave
#define CLOSE_BTN_COLOR TRIT_202    // Cerrar: sangre intensa
#define MINIMIZE_BTN_COLOR TRIT_120 // Minimizar: sol
#define MAXIMIZE_BTN_COLOR TRIT_110 // Maximizar: vida
#define BORDER_COLOR    TRIT_011    // Borde: tierra fuerte
#define BORDER_FOCUSED  TRIT_101    // Borde activo: espíritu+agua

// Ternary accent colors for widgets
#define ACCENT_PRIMARY   TRIT_OTH
#define ACCENT_SECONDARY TRIT_HAAB
#define ACCENT_TERTIARY  TRIT_TZOLKIN

// Initialize framebuffer (from multiboot info)
void framebuffer_init(uint32_t addr, uint32_t width, uint32_t height, uint32_t pitch, uint8_t bpp) {
    framebuffer = (uint32_t*)addr;
    fb_width = width;
    fb_height = height;
    fb_bpp = bpp;
    // Always compute pitch from width and bpp (GRUB pitch can be wrong)
    fb_pitch = width * (bpp / 8);
    
    vga_puts("[FB] Framebuffer initialized\n");
    vga_puts("[FB] Resolution: ");
    { char nb[8]; num_to_str(width, nb); vga_puts(nb); vga_puts("x"); }
    { char nb[8]; num_to_str(height, nb); vga_puts(nb); }
    vga_puts("\n");
    vga_puts("[FB] Pitch: ");
    { char nb[8]; num_to_str(fb_pitch, nb); vga_puts(nb); }
    vga_puts(" bytes\n");
    vga_puts("[FB] Ternary color palette loaded (27 colors)\n");
}

uint8_t fb_is_active(void) {
    return framebuffer != 0;
}

// Set pixel
void fb_set_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb_width || y >= fb_height) return;
    
    uint32_t offset = y * (fb_pitch / 4) + x;
    framebuffer[offset] = color;
}

// Get pixel
uint32_t fb_get_pixel(uint32_t x, uint32_t y) {
    if (x >= fb_width || y >= fb_height) return 0;
    
    uint32_t offset = y * (fb_pitch / 4) + x;
    return framebuffer[offset];
}

// Fill screen
void fb_fill(uint32_t color) {
    for (uint32_t y = 0; y < fb_height; y++) {
        for (uint32_t x = 0; x < fb_width; x++) {
            fb_set_pixel(x, y, color);
        }
    }
}

// Draw rectangle
void fb_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t dy = 0; dy < h; dy++) {
        for (uint32_t dx = 0; dx < w; dx++) {
            fb_set_pixel(x + dx, y + dy, color);
        }
    }
}

// Draw line (Bresenham)
void fb_draw_line(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        fb_set_pixel(x0, y0, color);
        
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// Draw circle
void fb_draw_circle(uint32_t cx, uint32_t cy, uint32_t r, uint32_t color) {
    int x = r;
    int y = 0;
    int err = 1 - r;
    
    while (x >= y) {
        fb_set_pixel(cx + x, cy + y, color);
        fb_set_pixel(cx + y, cy + x, color);
        fb_set_pixel(cx - y, cy + x, color);
        fb_set_pixel(cx - x, cy + y, color);
        fb_set_pixel(cx - x, cy - y, color);
        fb_set_pixel(cx - y, cy - x, color);
        fb_set_pixel(cx + y, cy - x, color);
        fb_set_pixel(cx + x, cy - y, color);
        
        y++;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}

// Draw character (8x8 font) with ternary colors
void fb_draw_char(uint32_t x, uint32_t y, char c, uint32_t color, uint32_t bg) {
    // Simple 8x8 font (partial ASCII)
    static const uint8_t font8x8[][8] = {
        [0x20] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // space
        [0x21] = {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, // !
        [0x41] = {0x3C,0x66,0x66,0x7E,0x66,0x66,0x66,0x00}, // A
        [0x42] = {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00}, // B
        [0x43] = {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00}, // C
        [0x44] = {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00}, // D
        [0x45] = {0x7E,0x60,0x60,0x78,0x60,0x60,0x7E,0x00}, // E
        [0x46] = {0x7E,0x60,0x60,0x78,0x60,0x60,0x60,0x00}, // F
        [0x47] = {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0x00}, // G
        [0x48] = {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00}, // H
        [0x49] = {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // I
        [0x4A] = {0x1E,0x0C,0x0C,0x0C,0x0C,0x6C,0x38,0x00}, // J
        [0x4B] = {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00}, // K
        [0x4C] = {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00}, // L
        [0x4D] = {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00}, // M
        [0x4E] = {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00}, // N
        [0x4F] = {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // O
        [0x50] = {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00}, // P
        [0x51] = {0x3C,0x66,0x66,0x66,0x66,0x3C,0x0E,0x00}, // Q
        [0x52] = {0x7C,0x66,0x66,0x7C,0x78,0x6C,0x66,0x00}, // R
        [0x53] = {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00}, // S
        [0x54] = {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, // T
        [0x55] = {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // U
        [0x56] = {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00}, // V
        [0x57] = {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, // W
        [0x58] = {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00}, // X
        [0x59] = {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00}, // Y
        [0x5A] = {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00}, // Z
        [0x30] = {0x3C,0x66,0x66,0x6E,0x76,0x66,0x3C,0x00}, // 0
        [0x31] = {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00}, // 1
        [0x32] = {0x3C,0x66,0x06,0x0C,0x30,0x60,0x7E,0x00}, // 2
        [0x33] = {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00}, // 3
    };
    
    if (c < 0x20 || c > 0x5A) return;
    
    const uint8_t* glyph = font8x8[c];
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (glyph[row] & (1 << (7 - col))) {
                fb_set_pixel(x + col, y + row, color);
            } else if (bg != TRIT_000) {
                fb_set_pixel(x + col, y + row, bg);
            }
        }
    }
}

// Draw string
void fb_draw_string(uint32_t x, uint32_t y, const char* str, uint32_t color, uint32_t bg) {
    while (*str) {
        fb_draw_char(x, y, *str, color, bg);
        x += 8;
        str++;
    }
}

// Get framebuffer info
void fb_get_info(uint32_t* width, uint32_t* height) {
    if (width) *width = fb_width;
    if (height) *height = fb_height;
}

// Get ternary color by index (0-26)
uint32_t fb_get_ternary_color(uint8_t index) {
    static const uint32_t ternary_palette[27] = {
        TRIT_000, TRIT_001, TRIT_002,
        TRIT_010, TRIT_011, TRIT_012,
        TRIT_020, TRIT_021, TRIT_022,
        TRIT_100, TRIT_101, TRIT_102,
        TRIT_110, TRIT_111, TRIT_112,
        TRIT_120, TRIT_121, TRIT_122,
        TRIT_200, TRIT_201, TRIT_202,
        TRIT_210, TRIT_211, TRIT_212,
        TRIT_220, TRIT_221, TRIT_222
    };
    
    if (index >= 27) return TRIT_000;
    return ternary_palette[index];
}

// Draw ternary number (trits) as colored blocks
void fb_draw_trit_blocks(uint32_t x, uint32_t y, uint8_t trit0, uint8_t trit1, uint8_t trit2, uint32_t size) {
    // Draw 3 blocks representing ternary digits
    uint32_t colors[3] = {TRIT_000, TRIT_001, TRIT_002}; // Dark, medium, bright
    
    fb_draw_rect(x, y, size, size, colors[trit0 % 3]);
    fb_draw_rect(x + size + 2, y, size, size, colors[trit1 % 3]);
    fb_draw_rect(x + 2 * (size + 2), y, size, size, colors[trit2 % 3]);
}

// Draw ternary grid pattern (ancestral decoration)
void fb_draw_ternary_grid(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    for (uint32_t row = 0; row < h; row += 6) {
        for (uint32_t col = 0; col < w; col += 6) {
            uint8_t trit = (row / 6 + col / 6) % 3;
            uint32_t color;
            switch (trit) {
                case 0: color = TRIT_000; break;
                case 1: color = TRIT_011; break;
                case 2: color = TRIT_022; break;
            }
            fb_draw_rect(x + col, y + row, 4, 4, color);
        }
    }
}

// =============================================================================
// FRAMEBUFFER CONSOLE — Text rendering on pixel framebuffer
// =============================================================================

#define FB_CHAR_W 8
#define FB_CHAR_H 8

static uint32_t fb_con_w = 0;   // console width in chars
static uint32_t fb_con_h = 0;   // console height in chars
static uint32_t fb_con_x = 0;   // cursor x (chars)
static uint32_t fb_con_y = 0;   // cursor y (chars)
static uint32_t fb_con_fg = 0xCCCCCC; // foreground color
static uint32_t fb_con_bg = 0x1E1E2E; // background color

void fb_console_init(uint32_t width, uint32_t height) {
    fb_con_w = width / FB_CHAR_W;
    fb_con_h = height / FB_CHAR_H;
    fb_con_x = 0;
    fb_con_y = 0;
}

static void fb_con_scroll(void) {
    // Move everything up by one char row
    uint32_t row_bytes = fb_pitch;
    uint32_t char_row_bytes = FB_CHAR_H * row_bytes;
    uint32_t total = char_row_bytes * (fb_con_h - 1);
    
    // Copy pixel data up
    uint8_t* dst = (uint8_t*)framebuffer;
    uint8_t* src = dst + char_row_bytes;
    for (uint32_t i = 0; i < total; i++) dst[i] = src[i];
    
    // Clear last row
    uint8_t* last = dst + total;
    uint32_t clear_bytes = char_row_bytes;
    for (uint32_t i = 0; i < clear_bytes; i++) last[i] = 0;
    
    fb_con_y = fb_con_h - 1;
}

// VGA color attribute to RGB
static uint32_t vga_attr_to_rgb(uint8_t attr) {
    uint8_t fg_idx = attr & 0x0F;
    uint8_t bg_idx = (attr >> 4) & 0x0F;
    
    // Standard VGA colors
    static const uint32_t vga_colors[16] = {
        0x000000, 0x0000AA, 0x00AA00, 0x00AAAA,
        0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
        0x555555, 0x5555FF, 0x55FF55, 0x55FFFF,
        0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF
    };
    
    // Use fg for text, bg for background
    return vga_colors[fg_idx]; // We draw fg pixels only, bg is already set
}

void fb_console_putc(char c, uint8_t color) {
    if (!framebuffer) return;
    
    uint32_t fg = vga_attr_to_rgb(color);
    uint32_t bg = vga_attr_to_rgb((color >> 4) & 0x0F);
    
    if (c == '\n') {
        fb_con_x = 0;
        fb_con_y++;
        if (fb_con_y >= fb_con_h) fb_con_scroll();
        return;
    }
    if (c == '\b') {
        if (fb_con_x > 0) {
            fb_con_x--;
            // Clear the character
            fb_draw_rect(fb_con_x * FB_CHAR_W, fb_con_y * FB_CHAR_H, FB_CHAR_W, FB_CHAR_H, bg);
        }
        return;
    }
    if (c == '\t') {
        fb_con_x = (fb_con_x + 4) & ~3;
        if (fb_con_x >= fb_con_w) {
            fb_con_x = 0;
            fb_con_y++;
            if (fb_con_y >= fb_con_h) fb_con_scroll();
        }
        return;
    }
    
    // Draw character
    fb_draw_char(fb_con_x * FB_CHAR_W, fb_con_y * FB_CHAR_H, c, fg, bg);
    
    fb_con_x++;
    if (fb_con_x >= fb_con_w) {
        fb_con_x = 0;
        fb_con_y++;
        if (fb_con_y >= fb_con_h) fb_con_scroll();
    }
}
