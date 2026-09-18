/**
 * mouse.c — Driver PS/2 Mouse para kernel ternario ancestral
 *
 * Maneja ratón PS/2 básico
 */

#include "../include/ternary.h"

// Mouse state
static int8_t mouse_x = 0;
static int8_t mouse_y = 0;
static uint8_t mouse_buttons = 0;
static uint8_t mouse_cycle = 0;
static int8_t mouse_bytes[3];

// Mouse cursor position
static uint32_t cursor_x = 0;
static uint32_t cursor_y = 0;

// Wait for mouse output
static void mouse_wait(uint8_t a_type) {
    uint32_t timeout = 100000;
    if (a_type == 0) {
        while (timeout--) {
            if ((inb(0x64) & 1) == 1) return;
        }
    } else {
        while (timeout--) {
            if ((inb(0x64) & 2) == 0) return;
        }
    }
}

// Send mouse command
static void mouse_send(uint8_t data) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, data);
}

// Read mouse output
static uint8_t mouse_read(void) {
    mouse_wait(0);
    return inb(0x60);
}

// Mouse IRQ handler (IRQ12)
void mouse_handler(uint8_t* regs) {
    uint8_t data = inb(0x60);
    
    mouse_bytes[mouse_cycle] = data;
    mouse_cycle++;
    
    if (mouse_cycle == 3) {
        mouse_cycle = 0;
        
        // Parse mouse packet
        mouse_buttons = mouse_bytes[0] & 0x07;
        mouse_x = mouse_bytes[1];
        mouse_y = mouse_bytes[2];
        
        // Update cursor position
        cursor_x += mouse_x;
        cursor_y -= mouse_y;  // Y is inverted
        
        // Bounds checking
        uint32_t fb_w, fb_h;
        fb_get_info(&fb_w, &fb_h);
        
        if (cursor_x < 0) cursor_x = 0;
        if (cursor_x >= fb_w) cursor_x = fb_w - 1;
        if (cursor_y < 0) cursor_y = 0;
        if (cursor_y >= fb_h) cursor_y = fb_h - 1;
    }
}

// Initialize mouse
void mouse_init(void) {
    vga_puts("[MOUSE] Initializing PS/2 mouse...\n");
    
    // Enable auxiliary device
    mouse_wait(1);
    outb(0x64, 0xA8);
    
    // Enable interrupts
    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    uint8_t status = inb(0x60);
    status |= 0x02;
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);
    
    // Reset mouse
    mouse_send(0xFF);
    mouse_read();
    
    // Enable data reporting
    mouse_send(0xF4);
    mouse_read();
    
    // Set default values
    mouse_x = 0;
    mouse_y = 0;
    mouse_buttons = 0;
    mouse_cycle = 0;
    
    // Center cursor
    uint32_t fb_w, fb_h;
    fb_get_info(&fb_w, &fb_h);
    cursor_x = fb_w / 2;
    cursor_y = fb_h / 2;
    
    vga_puts("[MOUSE] Mouse initialized\n");
}

// Get mouse position
void mouse_get_position(int32_t* x, int32_t* y) {
    if (x) *x = cursor_x;
    if (y) *y = cursor_y;
}

// Get mouse buttons
uint8_t mouse_get_buttons(void) {
    return mouse_buttons;
}

// Check if left button pressed
uint8_t mouse_left_button(void) {
    return mouse_buttons & 0x01;
}

// Check if right button pressed
uint8_t mouse_right_button(void) {
    return mouse_buttons & 0x02;
}

// Check if middle button pressed
uint8_t mouse_middle_button(void) {
    return mouse_buttons & 0x04;
}

// Draw mouse cursor with ternary colors
void mouse_draw_cursor(void) {
    // Simple cross cursor with ternary colors
    int32_t x, y;
    mouse_get_position(&x, &y);
    
    // Draw crosshair with ternary color scheme
    fb_draw_line(x - 5, y, x + 5, y, TRIT_220);  // White line
    fb_draw_line(x, y - 5, x, y + 5, TRIT_220);
    fb_draw_line(x - 5, y, x - 4, y, TRIT_000);   // Dark outline
    fb_draw_line(x + 4, y, x + 5, y, TRIT_000);
    fb_draw_line(x, y - 5, x, y - 4, TRIT_000);
    fb_draw_line(x, y + 4, x, y + 5, TRIT_000);
    
    // Center point
    fb_set_pixel(x, y, TRIT_020);  // Fire red center
}

// Mouse status
void mouse_status(void) {
    vga_puts("\n  Mouse status:\n\n");
    
    vga_puts("  Position: ");
    { char nb[8]; num_to_str(cursor_x, nb); vga_puts(nb); vga_puts(","); }
    { char nb[8]; num_to_str(cursor_y, nb); vga_puts(nb); }
    vga_puts("\n");
    
    vga_puts("  Buttons: ");
    if (mouse_left_button()) vga_puts("L ");
    if (mouse_right_button()) vga_puts("R ");
    if (mouse_middle_button()) vga_puts("M ");
    if (mouse_buttons == 0) vga_puts("none");
    vga_puts("\n");
}
