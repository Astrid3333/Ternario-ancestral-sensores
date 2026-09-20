/**
 * window.c — VGA text-mode window manager for Tritos kernel
 * 80x25 characters with color attributes
 */

#include "../include/ternary.h"

#define MAX_WINDOWS 16
#define MAX_TITLE_LEN 40
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

typedef struct {
    uint8_t id;
    char title[MAX_TITLE_LEN];
    uint8_t col, row;       // position in text cells
    uint8_t w, h;           // size in text cells
    uint8_t bg_attr;        // VGA color attribute
    uint8_t title_attr;
    uint8_t state;          // 0=normal, 1=minimized, 2=maximized
    uint8_t visible;
    uint8_t focused;
} window_t;

static window_t windows[MAX_WINDOWS];
static uint8_t window_count = 0;
static int8_t focused_window = -1;

// VGA text buffer
static volatile uint16_t* const VGA = (uint16_t*)0xB8000;

static void vga_cell(uint8_t x, uint8_t y, char c, uint8_t attr) {
    if (x < VGA_WIDTH && y < VGA_HEIGHT)
        VGA[y * VGA_WIDTH + x] = (uint16_t)attr << 8 | c;
}

static void vga_hline(uint8_t x, uint8_t y, uint8_t len, char c, uint8_t attr) {
    for (uint8_t i = 0; i < len; i++) vga_cell(x + i, y, c, attr);
}

static void vga_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, char c, uint8_t attr) {
    for (uint8_t j = 0; j < h; j++)
        for (uint8_t i = 0; i < w; i++)
            vga_cell(x + i, y + j, c, attr);
}

static void vga_str(uint8_t x, uint8_t y, const char* s, uint8_t attr) {
    while (*s && x < VGA_WIDTH) { vga_cell(x++, y, *s, attr); s++; }
}

// Draw single window
static void wm_draw(window_t* w) {
    if (!w->visible || w->state == 1) return;

    uint8_t ta = w->focused ? 0x70 : 0x30; // title bar: white-on-blue or white-on-cyan
    uint8_t ba = w->bg_attr;                // body attribute

    // Top border with title
    vga_cell(w->col, w->row, '+', ta);
    vga_hline(w->col + 1, w->row, w->w - 2, '-', ta);
    vga_cell(w->col + w->w - 1, w->row, '+', ta);

    // Title text centered
    uint8_t tlen = 0;
    { const char* p = w->title; while (*p++) tlen++; }
    uint8_t tx = w->col + 1 + (w->w - 2 - tlen) / 2;
    if (tx + tlen > w->col + w->w - 1) tx = w->col + 1;
    vga_str(tx, w->row, w->title, ta);

    // Close/minimize buttons on right
    vga_cell(w->col + w->w - 3, w->row, '[', 0x4F);
    vga_cell(w->col + w->w - 2, w->row, 'X', 0x4F);
    vga_cell(w->col + w->w - 1, w->row, ']', 0x4F);

    // Side borders + body
    for (uint8_t j = 1; j < w->h - 1; j++) {
        vga_cell(w->col, w->row + j, '|', ta);
        vga_hline(w->col + 1, w->row + j, w->w - 2, ' ', ba);
        vga_cell(w->col + w->w - 1, w->row + j, '|', ta);
    }

    // Bottom border
    vga_cell(w->col, w->row + w->h - 1, '+', ta);
    vga_hline(w->col + 1, w->row + w->h - 1, w->w - 2, '-', ta);
    vga_cell(w->col + w->w - 1, w->row + w->h - 1, '+', ta);
}

// Init
void wm_init(void) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].id = 0;
        windows[i].visible = 0;
    }
    window_count = 0;
    focused_window = -1;
}

// Create window (sizes in text cells)
int8_t wm_create_window(const char* title, uint8_t col, uint8_t row,
                        uint8_t w, uint8_t h, uint8_t bg_attr) {
    if (window_count >= MAX_WINDOWS) return -1;
    if (col + w > VGA_WIDTH) w = VGA_WIDTH - col;
    if (row + h > VGA_HEIGHT) h = VGA_HEIGHT - row;

    window_t* win = &windows[window_count];
    win->id = window_count + 1;
    uint8_t i = 0;
    while (*title && i < MAX_TITLE_LEN - 1) { win->title[i++] = *title++; }
    win->title[i] = 0;
    win->col = col;
    win->row = row;
    win->w = w;
    win->h = h;
    win->bg_attr = bg_attr;
    win->state = 0;
    win->visible = 1;
    win->focused = 1;

    window_count++;
    focused_window = win->id - 1;

    return win->id;
}

// Redraw desktop
void wm_redraw(void) {
    // Fill screen
    for (uint8_t y = 0; y < VGA_HEIGHT; y++)
        for (uint8_t x = 0; x < VGA_WIDTH; x++)
            vga_cell(x, y, ' ', 0x07);

    // Title bar at top
    vga_hline(0, 0, VGA_WIDTH, ' ', 0x70);
    vga_str(2, 0, "TRITOS OS v4.5", 0x70);
    vga_str(60, 0, "Ternary Ancestral", 0x70);

    // Taskbar at bottom
    vga_hline(0, VGA_HEIGHT - 1, VGA_WIDTH, ' ', 0x1F);
    vga_str(2, VGA_HEIGHT - 1, "[TRITOS]", 0x1F);
    vga_str(12, VGA_HEIGHT - 1, "mem: 3600B", 0x1E);
    vga_str(30, VGA_HEIGHT - 1, "users: 2", 0x1E);
    vga_str(45, VGA_HEIGHT - 1, "proc: 2", 0x1E);

    // Draw windows
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].visible) wm_draw(&windows[i]);
    }
}

// Close window
void wm_close_window(int8_t id) {
    if (id < 1 || id > MAX_WINDOWS) return;
    windows[id - 1].visible = 0;
    if (focused_window == id - 1) {
        focused_window = -1;
        for (int i = 0; i < MAX_WINDOWS; i++) {
            if (windows[i].visible) {
                focused_window = i;
                windows[i].focused = 1;
                break;
            }
        }
    }
    wm_redraw();
}

void wm_minimize_window(int8_t id) {
    if (id < 1 || id > MAX_WINDOWS) return;
    windows[id - 1].state = 1;
    windows[id - 1].visible = 0;
    wm_redraw();
}

void wm_restore_window(int8_t id) {
    if (id < 1 || id > MAX_WINDOWS) return;
    windows[id - 1].state = 0;
    windows[id - 1].visible = 1;
    wm_redraw();
}

void wm_maximize_window(int8_t id) {
    if (id < 1 || id > MAX_WINDOWS) return;
    window_t* w = &windows[id - 1];
    if (w->state == 2) {
        w->state = 0;
    } else {
        w->state = 2;
        w->col = 0; w->row = 1;
        w->w = VGA_WIDTH; w->h = VGA_HEIGHT - 2;
    }
    wm_redraw();
}

void wm_focus_window(int8_t id) {
    if (id < 1 || id > MAX_WINDOWS) return;
    for (int i = 0; i < MAX_WINDOWS; i++) windows[i].focused = 0;
    windows[id - 1].focused = 1;
    focused_window = id - 1;
    wm_redraw();
}

void wm_move_window(int8_t id, uint8_t col, uint8_t row) {
    if (id < 1 || id > MAX_WINDOWS) return;
    windows[id - 1].col = col;
    windows[id - 1].row = row;
    wm_redraw();
}

void wm_resize_window(int8_t id, uint8_t w, uint8_t h) {
    if (id < 1 || id > MAX_WINDOWS) return;
    windows[id - 1].w = w;
    windows[id - 1].h = h;
    wm_redraw();
}

void wm_handle_click(uint8_t x, uint8_t y) {
    // Check window clicks (reverse order for z-order)
    for (int i = MAX_WINDOWS - 1; i >= 0; i--) {
        if (!windows[i].visible) continue;
        window_t* w = &windows[i];
        if (x >= w->col && x < w->col + w->w && y >= w->row && y < w->row + w->h) {
            wm_focus_window(w->id);
            return;
        }
    }
}

uint8_t wm_get_window_count(void) { return window_count; }
int8_t wm_get_focused(void) { return focused_window; }

void wm_status(void) {
    vga_puts("\n  Window Manager:\n\n");
    vga_puts("  Windows: ");
    { char nb[4]; num_to_str(window_count, nb); vga_puts(nb); }
    vga_puts("/");
    vga_puts("16\n");
    vga_puts("  Focused: ");
    { char nb[4]; num_to_str(focused_window + 1, nb); vga_puts(nb); }
    vga_puts("\n\n");
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].visible) {
            vga_puts("  [");
            { char nb[4]; num_to_str(windows[i].id, nb); vga_puts(nb); }
            vga_puts("] ");
            vga_puts(windows[i].title);
            vga_puts("\n");
        }
    }
}
