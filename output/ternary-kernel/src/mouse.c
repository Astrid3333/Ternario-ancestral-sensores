/**
 * mouse.c — Driver PS/2 Mouse (polling) + cursor grafico sobre el framebuffer
 *
 * Los bytes del mouse los entrega el loop principal (bit 5 del puerto 0x64)
 * a mouse_feed(); no hace falta IRQ12 ni IDT.
 */

#include "../include/ternary.h"

#define CUR_W 12
#define CUR_H 16

// 'X' = contorno negro, 'O' = relleno blanco, ' ' = transparente
static const char* const cursor_sprite[CUR_H] = {
    "X           ",
    "XX          ",
    "XOX         ",
    "XOOX        ",
    "XOOOX       ",
    "XOOOOX      ",
    "XOOOOOX     ",
    "XOOOOOOX    ",
    "XOOOOOOOX   ",
    "XOOOOOXXXXX ",
    "XOOXOOX     ",
    "XOX XOOX    ",
    "XX  XOOX    ",
    "X    XOOX   ",
    "     XOOX   ",
    "      XX    "
};

static int32_t cursor_x = 0;
static int32_t cursor_y = 0;
static uint8_t mouse_buttons = 0;
static uint8_t mouse_cycle = 0;
static uint8_t mouse_bytes[3];

static uint8_t  cur_visible = 0;
static uint8_t  cur_dirty = 1;
static int32_t  cur_draw_x = 0, cur_draw_y = 0;
static uint32_t cur_saved[CUR_W * CUR_H];

// ---------------------------------------------------------------- controlador PS/2
static int mouse_wait_out(void) {      // espera dato para leer
    for (uint32_t t = 100000; t; t--) if (inb(0x64) & 1) return 1;
    return 0;
}

static int mouse_wait_in(void) {       // espera que se pueda escribir
    for (uint32_t t = 100000; t; t--) if ((inb(0x64) & 2) == 0) return 1;
    return 0;
}

static void mouse_write(uint8_t data) {
    mouse_wait_in();
    outb(0x64, 0xD4);
    mouse_wait_in();
    outb(0x60, data);
}

static int mouse_read_byte(void) {     // -1 si no hay respuesta
    if (!mouse_wait_out()) return -1;
    return inb(0x60);
}

static void mouse_flush(void) {
    for (int i = 0; i < 32 && (inb(0x64) & 1); i++) inb(0x60);
}

void mouse_init(void) {
    vga_puts("[MOUSE] Initializing PS/2 mouse...\n");

    mouse_flush();

    mouse_wait_in();
    outb(0x64, 0xA8);                  // habilitar puerto auxiliar

    // Modo polling: sin IRQ12 (bit 1 = 0) y con reloj del mouse activo (bit 5 = 0)
    mouse_wait_in();
    outb(0x64, 0x20);
    int st = mouse_read_byte();
    if (st >= 0) {
        uint8_t cfg = (uint8_t)st & (uint8_t)~0x22;
        mouse_wait_in();
        outb(0x64, 0x60);
        mouse_wait_in();
        outb(0x60, cfg);
    }

    mouse_write(0xFF);                 // reset: ACK(0xFA), BAT(0xAA), ID(0x00)
    mouse_read_byte();
    mouse_read_byte();
    mouse_read_byte();

    mouse_write(0xF6);                 // valores por defecto
    mouse_read_byte();
    mouse_write(0xF4);                 // activar envio de datos
    int ack = mouse_read_byte();

    mouse_flush();
    mouse_cycle = 0;
    mouse_buttons = 0;

    uint32_t fb_w = 0, fb_h = 0;
    fb_get_info(&fb_w, &fb_h);
    cursor_x = (int32_t)(fb_w / 2);
    cursor_y = (int32_t)(fb_h / 2);
    cur_dirty = 1;

    vga_puts(ack == 0xFA ? "[MOUSE] Mouse initialized\n" : "[MOUSE] No ACK from mouse (continuing)\n");
}

// Recibe un byte crudo del mouse (desde el loop principal)
void mouse_feed(uint8_t b) {
    if (mouse_cycle == 0 && !(b & 0x08)) return;   // bit 3 siempre 1: si no, resincronizar

    mouse_bytes[mouse_cycle++] = b;
    if (mouse_cycle < 3) return;
    mouse_cycle = 0;

    uint8_t b0 = mouse_bytes[0];
    if (b0 & 0xC0) return;                         // overflow en X/Y: descartar paquete

    int32_t dx = (int32_t)mouse_bytes[1] - ((b0 & 0x10) ? 256 : 0);
    int32_t dy = (int32_t)mouse_bytes[2] - ((b0 & 0x20) ? 256 : 0);
    mouse_buttons = b0 & 0x07;

    cursor_x += dx;
    cursor_y -= dy;                                // Y de PS/2 va hacia arriba

    uint32_t fb_w = 0, fb_h = 0;
    fb_get_info(&fb_w, &fb_h);
    if (cursor_x < 0) cursor_x = 0;
    if (cursor_y < 0) cursor_y = 0;
    if (fb_w && cursor_x >= (int32_t)fb_w) cursor_x = (int32_t)fb_w - 1;
    if (fb_h && cursor_y >= (int32_t)fb_h) cursor_y = (int32_t)fb_h - 1;
    cur_dirty = 1;
}

// ---------------------------------------------------------------- cursor grafico
void mouse_cursor_hide(void) {
    if (!cur_visible) return;
    for (int y = 0; y < CUR_H; y++)
        for (int x = 0; x < CUR_W; x++)
            fb_set_pixel(cur_draw_x + x, cur_draw_y + y, cur_saved[y * CUR_W + x]);
    cur_visible = 0;
}

void mouse_update_cursor(void) {
    if (!fb_is_active()) return;
    if (cur_visible && !cur_dirty) return;

    mouse_cursor_hide();
    cur_draw_x = cursor_x;
    cur_draw_y = cursor_y;

    for (int y = 0; y < CUR_H; y++)
        for (int x = 0; x < CUR_W; x++)
            cur_saved[y * CUR_W + x] = fb_get_pixel(cur_draw_x + x, cur_draw_y + y);

    for (int y = 0; y < CUR_H; y++) {
        for (int x = 0; x < CUR_W; x++) {
            char c = cursor_sprite[y][x];
            if (c == 'X') fb_set_pixel(cur_draw_x + x, cur_draw_y + y, 0x000000);
            else if (c == 'O') fb_set_pixel(cur_draw_x + x, cur_draw_y + y, 0xFFFFFF);
        }
    }
    cur_visible = 1;
    cur_dirty = 0;
}

void mouse_draw_cursor(void) {
    mouse_update_cursor();
}

// ---------------------------------------------------------------- consulta
void mouse_get_position(int32_t* x, int32_t* y) {
    if (x) *x = cursor_x;
    if (y) *y = cursor_y;
}

uint8_t mouse_get_buttons(void) { return mouse_buttons; }
uint8_t mouse_left_button(void) { return mouse_buttons & 0x01; }
uint8_t mouse_right_button(void) { return mouse_buttons & 0x02; }
uint8_t mouse_middle_button(void) { return mouse_buttons & 0x04; }

void mouse_status(void) {
    vga_puts("\n  Mouse status:\n\n");

    vga_puts("  Position: ");
    { char nb[12]; num_to_str((uint32_t)cursor_x, nb); vga_puts(nb); vga_puts(","); }
    { char nb[12]; num_to_str((uint32_t)cursor_y, nb); vga_puts(nb); }
    vga_puts("\n");

    vga_puts("  Buttons: ");
    if (mouse_left_button()) vga_puts("L ");
    if (mouse_right_button()) vga_puts("R ");
    if (mouse_middle_button()) vga_puts("M ");
    if (mouse_buttons == 0) vga_puts("none");
    vga_puts("\n");
}
