/**
 * fonts.c — Fuentes mejoradas para GUI ternaria
 *
 * Bitmap font 16x16 + fuentes escalables
 */

#include "../include/ternary.h"

// =============================================================================
// FONT 16x16 — fuente bitmap mejorada
// =============================================================================

// Font metrics
#define FONT16_W 16
#define FONT16_H 16

// Character data (16x16 bitmap, 2 bytes per row)
static const uint16_t font16_data[][16] = {
    // Space (0x20)
    {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
     0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000},
    // ! (0x21)
    {0x0000,0x0000,0x0180,0x0180,0x0180,0x0180,0x0180,0x0180,
     0x0180,0x0000,0x0180,0x0180,0x0000,0x0000,0x0000,0x0000},
    // A (0x41)
    {0x0000,0x0000,0x03C0,0x0660,0x0660,0x0660,0x0C30,0x0C30,
     0x0FF0,0x0C30,0x0C30,0x1818,0x1818,0x1818,0x0000,0x0000},
    // B (0x42)
    {0x0000,0x0000,0x0FC0,0x0C60,0x0C60,0x0C60,0x0FC0,0x0C60,
     0x0C60,0x0C60,0x0C60,0x0FC0,0x0000,0x0000,0x0000,0x0000},
    // C (0x43)
    {0x0000,0x0000,0x03C0,0x0660,0x0C30,0x0C00,0x0C00,0x0C00,
     0x0C00,0x0C00,0x0C30,0x0660,0x03C0,0x0000,0x0000,0x0000},
    // D (0x44)
    {0x0000,0x0000,0x0F80,0x0CC0,0x0C60,0x0C60,0x0C60,0x0C60,
     0x0C60,0x0C60,0x0C60,0x0CC0,0x0F80,0x0000,0x0000,0x0000},
    // E (0x45)
    {0x0000,0x0000,0x0FF0,0x0C00,0x0C00,0x0C00,0x0FC0,0x0C00,
     0x0C00,0x0C00,0x0C00,0x0C00,0x0FF0,0x0000,0x0000,0x0000},
    // F (0x46)
    {0x0000,0x0000,0x0FF0,0x0C00,0x0C00,0x0C00,0x0FC0,0x0C00,
     0x0C00,0x0C00,0x0C00,0x0C00,0x0C00,0x0000,0x0000,0x0000},
    // G (0x47)
    {0x0000,0x0000,0x03C0,0x0660,0x0C30,0x0C00,0x0C00,0x0CF0,
     0x0C30,0x0C30,0x0C30,0x0660,0x03C0,0x0000,0x0000,0x0000},
    // H (0x48)
    {0x0000,0x0000,0x0C30,0x0C30,0x0C30,0x0C30,0x0FF0,0x0C30,
     0x0C30,0x0C30,0x0C30,0x0C30,0x0C30,0x0000,0x0000,0x0000},
    // I (0x49)
    {0x0000,0x0000,0x03C0,0x0180,0x0180,0x0180,0x0180,0x0180,
     0x0180,0x0180,0x0180,0x0180,0x03C0,0x0000,0x0000,0x0000},
    // J (0x4A)
    {0x0000,0x0000,0x00F0,0x0060,0x0060,0x0060,0x0060,0x0060,
     0x0060,0x0060,0x0C60,0x0660,0x03C0,0x0000,0x0000,0x0000},
    // K (0x4B)
    {0x0000,0x0000,0x0C60,0x0CC0,0x0D80,0x0F00,0x0E00,0x0F00,
     0x0D80,0x0CC0,0x0C60,0x0C30,0x0C18,0x0000,0x0000,0x0000},
    // L (0x4C)
    {0x0000,0x0000,0x0C00,0x0C00,0x0C00,0x0C00,0x0C00,0x0C00,
     0x0C00,0x0C00,0x0C00,0x0C00,0x0FF0,0x0000,0x0000,0x0000},
    // M (0x4D)
    {0x0000,0x0000,0x1818,0x1C38,0x1E78,0x1BD8,0x1998,0x1818,
     0x1818,0x1818,0x1818,0x1818,0x1818,0x0000,0x0000,0x0000},
    // N (0x4E)
    {0x0000,0x0000,0x1818,0x1C18,0x1E18,0x1B18,0x1998,0x18B8,
     0x1878,0x1838,0x1818,0x1818,0x1818,0x0000,0x0000,0x0000},
    // O (0x4F)
    {0x0000,0x0000,0x03C0,0x0660,0x0C30,0x0C30,0x0C30,0x0C30,
     0x0C30,0x0C30,0x0C30,0x0660,0x03C0,0x0000,0x0000,0x0000},
    // P (0x50)
    {0x0000,0x0000,0x0FC0,0x0C60,0x0C60,0x0C60,0x0FC0,0x0C00,
     0x0C00,0x0C00,0x0C00,0x0C00,0x0C00,0x0000,0x0000,0x0000},
    // Q (0x51)
    {0x0000,0x0000,0x03C0,0x0660,0x0C30,0x0C30,0x0C30,0x0C30,
     0x0C30,0x0C30,0x0D30,0x0660,0x03E0,0x0018,0x0000,0x0000},
    // R (0x52)
    {0x0000,0x0000,0x0FC0,0x0C60,0x0C60,0x0C60,0x0FC0,0x0CC0,
     0x0C60,0x0C60,0x0C30,0x0C30,0x0C18,0x0000,0x0000,0x0000},
    // S (0x53)
    {0x0000,0x0000,0x03C0,0x0660,0x0C30,0x0C00,0x0600,0x01C0,
     0x0060,0x0030,0x0C30,0x0660,0x03C0,0x0000,0x0000,0x0000},
    // T (0x54)
    {0x0000,0x0000,0x0FF0,0x0180,0x0180,0x0180,0x0180,0x0180,
     0x0180,0x0180,0x0180,0x0180,0x0180,0x0000,0x0000,0x0000},
    // U (0x55)
    {0x0000,0x0000,0x0C30,0x0C30,0x0C30,0x0C30,0x0C30,0x0C30,
     0x0C30,0x0C30,0x0C30,0x0660,0x03C0,0x0000,0x0000,0x0000},
    // V (0x56)
    {0x0000,0x0000,0x1818,0x1818,0x1818,0x1818,0x1818,0x1818,
     0x0C30,0x0C30,0x0C30,0x0660,0x03C0,0x0000,0x0000,0x0000},
    // W (0x57)
    {0x0000,0x0000,0x1818,0x1818,0x1818,0x1818,0x1818,0x1BD8,
     0x1E78,0x1C38,0x1818,0x1818,0x1818,0x0000,0x0000,0x0000},
    // X (0x58)
    {0x0000,0x0000,0x1818,0x1818,0x0C30,0x0660,0x03C0,0x03C0,
     0x0660,0x0C30,0x1818,0x1818,0x1818,0x0000,0x0000,0x0000},
    // Y (0x59)
    {0x0000,0x0000,0x1818,0x1818,0x0C30,0x0C30,0x0660,0x03C0,
     0x0180,0x0180,0x0180,0x0180,0x0180,0x0000,0x0000,0x0000},
    // Z (0x5A)
    {0x0000,0x0000,0x0FF0,0x0030,0x0060,0x00C0,0x0180,0x0300,
     0x0600,0x0C00,0x1800,0x1800,0x1FF8,0x0000,0x0000,0x0000},
    // 0 (0x30)
    {0x0000,0x0000,0x03C0,0x0660,0x0C30,0x0C30,0x0C30,0x0DB0,
     0x0C30,0x0C30,0x0C30,0x0660,0x03C0,0x0000,0x0000,0x0000},
    // 1 (0x31)
    {0x0000,0x0000,0x0180,0x0380,0x0780,0x0180,0x0180,0x0180,
     0x0180,0x0180,0x0180,0x0180,0x07E0,0x0000,0x0000,0x0000},
    // 2 (0x32)
    {0x0000,0x0000,0x03C0,0x0660,0x0C30,0x0030,0x0060,0x0180,
     0x0300,0x0600,0x0C00,0x0C00,0x0FF0,0x0000,0x0000,0x0000},
    // 3 (0x33)
    {0x0000,0x0000,0x03C0,0x0660,0x0030,0x0030,0x01C0,0x0030,
     0x0030,0x0030,0x0C30,0x0660,0x03C0,0x0000,0x0000,0x0000},
};

// =============================================================================
// WIDGET SYSTEM
// =============================================================================

#define MAX_WIDGETS 32
#define MAX_WIDGET_TEXT 64

// Static widget storage
static widget_t widgets[MAX_WIDGETS];
static uint8_t widget_count = 0;
static uint8_t focused_widget = 0xFF;

// Initialize widgets
void widgets_init(void) {
    for (int i = 0; i < MAX_WIDGETS; i++) {
        widgets[i].id = 0;
        widgets[i].visible = 0;
    }
    widget_count = 0;
    focused_widget = -1;
}

// Create button
uint8_t widget_create_button(int32_t x, int32_t y, int32_t w, int32_t h,
                             const char* text, uint32_t bg, uint32_t fg) {
    if (widget_count >= MAX_WIDGETS) return 0;
    
    widget_t* wgt = &widgets[widget_count];
    wgt->id = widget_count + 1;
    wgt->type = WIDGET_BUTTON;
    wgt->x = x;
    wgt->y = y;
    wgt->w = w;
    wgt->h = h;
    strcpy_t(wgt->text, text);
    wgt->bg_color = bg;
    wgt->fg_color = fg;
    wgt->visible = 1;
    wgt->enabled = 1;
    wgt->focused = 0;
    wgt->on_click = 0;
    
    widget_count++;
    return wgt->id;
}

// Create label
uint8_t widget_create_label(int32_t x, int32_t y, const char* text, uint32_t fg) {
    if (widget_count >= MAX_WIDGETS) return 0;
    
    widget_t* wgt = &widgets[widget_count];
    wgt->id = widget_count + 1;
    wgt->type = WIDGET_LABEL;
    wgt->x = x;
    wgt->y = y;
    wgt->w = strlen_t(text) * 8;
    wgt->h = 16;
    strcpy_t(wgt->text, text);
    wgt->bg_color = TRIT_000;
    wgt->fg_color = fg;
    wgt->visible = 1;
    wgt->enabled = 1;
    
    widget_count++;
    return wgt->id;
}

// Create scrollbar
uint8_t widget_create_scrollbar(int32_t x, int32_t y, int32_t h,
                                int32_t min_val, int32_t max_val, int32_t initial) {
    if (widget_count >= MAX_WIDGETS) return 0;
    
    widget_t* wgt = &widgets[widget_count];
    wgt->id = widget_count + 1;
    wgt->type = WIDGET_SCROLLBAR;
    wgt->x = x;
    wgt->y = y;
    wgt->w = 16;
    wgt->h = h;
    wgt->bg_color = TRIT_011;
    wgt->fg_color = TRIT_221;
    wgt->visible = 1;
    wgt->enabled = 1;
    wgt->value = initial;
    wgt->min_val = min_val;
    wgt->max_val = max_val;
    wgt->on_change = 0;
    
    widget_count++;
    return wgt->id;
}

// Create panel
uint8_t widget_create_panel(int32_t x, int32_t y, int32_t w, int32_t h,
                            const char* title, uint32_t bg) {
    if (widget_count >= MAX_WIDGETS) return 0;
    
    widget_t* wgt = &widgets[widget_count];
    wgt->id = widget_count + 1;
    wgt->type = WIDGET_PANEL;
    wgt->x = x;
    wgt->y = y;
    wgt->w = w;
    wgt->h = h;
    strcpy_t(wgt->text, title);
    wgt->bg_color = bg;
    wgt->fg_color = TRIT_221;
    wgt->visible = 1;
    wgt->enabled = 1;
    
    widget_count++;
    return wgt->id;
}

// Draw button
static void widget_draw_button(widget_t* wgt) {
    // Button background
    fb_draw_rect(wgt->x, wgt->y, wgt->w, wgt->h, wgt->bg_color);
    
    // Border
    fb_draw_rect(wgt->x, wgt->y, wgt->w, 2, TRIT_222);
    fb_draw_rect(wgt->x, wgt->y, 2, wgt->h, TRIT_222);
    fb_draw_rect(wgt->x + wgt->w - 2, wgt->y, 2, wgt->h, TRIT_011);
    fb_draw_rect(wgt->x, wgt->y + wgt->h - 2, wgt->w, 2, TRIT_011);
    
    // Focus indicator
    if (wgt->focused) {
        fb_draw_rect(wgt->x - 2, wgt->y - 2, wgt->w + 4, wgt->h + 4, TRIT_101);
    }
    
    // Text (centered)
    int text_w = strlen_t(wgt->text) * 8;
    int text_x = wgt->x + (wgt->w - text_w) / 2;
    int text_y = wgt->y + (wgt->h - 8) / 2;
    fb_draw_string(text_x, text_y, wgt->text, wgt->fg_color, wgt->bg_color);
}

// Draw label
static void widget_draw_label(widget_t* wgt) {
    fb_draw_string(wgt->x, wgt->y, wgt->text, wgt->fg_color, TRIT_000);
}

// Draw scrollbar
static void widget_draw_scrollbar(widget_t* wgt) {
    // Track
    fb_draw_rect(wgt->x, wgt->y, wgt->w, wgt->h, wgt->bg_color);
    
    // Thumb
    int range = wgt->max_val - wgt->min_val;
    if (range > 0) {
        int thumb_h = (wgt->h * 20) / range;
        if (thumb_h < 8) thumb_h = 8;
        
        int thumb_y = wgt->y + ((wgt->value - wgt->min_val) * (wgt->h - thumb_h)) / range;
        fb_draw_rect(wgt->x + 2, thumb_y, wgt->w - 4, thumb_h, TRIT_221);
    }
    
    // Arrows
    fb_draw_string(wgt->x + 4, wgt->y, "^", TRIT_221, wgt->bg_color);
    fb_draw_string(wgt->x + 4, wgt->y + wgt->h - 12, "v", TRIT_221, wgt->bg_color);
}

// Draw panel
static void widget_draw_panel(widget_t* wgt) {
    // Background
    fb_draw_rect(wgt->x, wgt->y, wgt->w, wgt->h, wgt->bg_color);
    
    // Border
    fb_draw_rect(wgt->x, wgt->y, wgt->w, 1, TRIT_222);
    fb_draw_rect(wgt->x, wgt->y, 1, wgt->h, TRIT_222);
    fb_draw_rect(wgt->x + wgt->w - 1, wgt->y, 1, wgt->h, TRIT_011);
    fb_draw_rect(wgt->x, wgt->y + wgt->h - 1, wgt->w, 1, TRIT_011);
    
    // Title bar
    fb_draw_rect(wgt->x + 1, wgt->y + 1, wgt->w - 2, 16, TRIT_100);
    fb_draw_string(wgt->x + 4, wgt->y + 2, wgt->text, TRIT_221, TRIT_100);
}

// Draw all visible widgets
void widgets_draw(void) {
    for (int i = 0; i < widget_count; i++) {
        if (!widgets[i].visible) continue;
        
        switch (widgets[i].type) {
            case WIDGET_BUTTON: widget_draw_button(&widgets[i]); break;
            case WIDGET_LABEL: widget_draw_label(&widgets[i]); break;
            case WIDGET_SCROLLBAR: widget_draw_scrollbar(&widgets[i]); break;
            case WIDGET_PANEL: widget_draw_panel(&widgets[i]); break;
            default: break;
        }
    }
}

// Check if point is inside widget
static int widget_hit_test(widget_t* wgt, int32_t x, int32_t y) {
    return (x >= wgt->x && x < wgt->x + wgt->w &&
            y >= wgt->y && y < wgt->y + wgt->h);
}

// Handle mouse click on widgets
void widgets_click(int32_t x, int32_t y) {
    for (int i = 0; i < widget_count; i++) {
        if (!widgets[i].visible || !widgets[i].enabled) continue;
        
        if (widget_hit_test(&widgets[i], x, y)) {
            // Focus this widget
            if (focused_widget != widgets[i].id) {
                if (focused_widget != (uint8_t)-1) {
                    for (int j = 0; j < widget_count; j++) {
                        if (widgets[j].id == focused_widget) {
                            widgets[j].focused = 0;
                        }
                    }
                }
                widgets[i].focused = 1;
                focused_widget = widgets[i].id;
            }
            
            // Handle click
            if (widgets[i].type == WIDGET_BUTTON && widgets[i].on_click) {
                widgets[i].on_click(widgets[i].id);
            }
            
            // Handle scrollbar
            if (widgets[i].type == WIDGET_SCROLLBAR) {
                int rel_y = y - widgets[i].y;
                int range = widgets[i].max_val - widgets[i].min_val;
                if (range > 0) {
                    widgets[i].value = widgets[i].min_val + (rel_y * range) / widgets[i].h;
                    if (widgets[i].value < widgets[i].min_val) widgets[i].value = widgets[i].min_val;
                    if (widgets[i].value > widgets[i].max_val) widgets[i].value = widgets[i].max_val;
                    if (widgets[i].on_change) widgets[i].on_change(widgets[i].id, widgets[i].value);
                }
            }
            
            break;
        }
    }
}

// Get widget by id
widget_t* widgets_get(uint8_t id) {
    for (int i = 0; i < widget_count; i++) {
        if (widgets[i].id == id) return &widgets[i];
    }
    return 0;
}

// =============================================================================
// GAMES — Snake, Pong
// =============================================================================

#define SNAKE_MAX 100
#define SNAKE_SIZE 4

// Snake game state
static int snake_x[SNAKE_MAX], snake_y[SNAKE_MAX];
static int snake_len = 3;
static int snake_dir = 0; // 0=right, 1=down, 2=left, 3=up
static int food_x, food_y;
static int snake_score = 0;
static int snake_game_over = 0;

static void snake_place_food(void) {
    food_x = 1 + (snake_x[0] * 7 + snake_y[0] * 13) % 18;
    food_y = 1 + (snake_x[0] * 11 + snake_y[0] * 5) % 18;
}

static void snake_init(void) {
    snake_len = 3;
    snake_dir = 0;
    snake_score = 0;
    snake_game_over = 0;
    
    for (int i = 0; i < snake_len; i++) {
        snake_x[i] = 10 - i;
        snake_y[i] = 10;
    }
    
    snake_place_food();
}

static void snake_update(void) {
    if (snake_game_over) return;
    
    // Move head
    int new_x = snake_x[0];
    int new_y = snake_y[0];
    
    switch (snake_dir) {
        case 0: new_x++; break; // right
        case 1: new_y++; break; // down
        case 2: new_x--; break; // left
        case 3: new_y--; break; // up
    }
    
    // Check collision with walls
    if (new_x < 0 || new_x >= 20 || new_y < 0 || new_y >= 20) {
        snake_game_over = 1;
        return;
    }
    
    // Check collision with self
    for (int i = 0; i < snake_len; i++) {
        if (snake_x[i] == new_x && snake_y[i] == new_y) {
            snake_game_over = 1;
            return;
        }
    }
    
    // Check food
    int ate = (new_x == food_x && new_y == food_y);
    
    // Move body
    if (!ate) {
        for (int i = snake_len - 1; i > 0; i--) {
            snake_x[i] = snake_x[i - 1];
            snake_y[i] = snake_y[i - 1];
        }
    } else {
        snake_len++;
        if (snake_len >= SNAKE_MAX) snake_len = SNAKE_MAX - 1;
        for (int i = snake_len - 1; i > 0; i--) {
            snake_x[i] = snake_x[i - 1];
            snake_y[i] = snake_y[i - 1];
        }
        snake_score += 10;
        snake_place_food();
    }
    
    snake_x[0] = new_x;
    snake_y[0] = new_y;
}

static void snake_draw(void) {
    vga_puts("\n  [Snake] Score: ");
    { char nb[8]; num_to_str(snake_score, nb); vga_puts(nb); }
    vga_puts("\n\n");
    
    // Draw border
    for (int x = 0; x < 22; x++) vga_putc('#');
    vga_puts("\n");
    
    for (int y = 0; y < 20; y++) {
        vga_puts("  #");
        for (int x = 0; x < 20; x++) {
            int drawn = 0;
            
            // Snake
            for (int i = 0; i < snake_len; i++) {
                if (snake_x[i] == x && snake_y[i] == y) {
                    vga_putc(i == 0 ? '@' : 'o');
                    drawn = 1;
                    break;
                }
            }
            
            // Food
            if (!drawn && food_x == x && food_y == y) {
                vga_putc('*');
                drawn = 1;
            }
            
            if (!drawn) vga_putc(' ');
        }
        vga_puts("#\n");
    }
    
    for (int x = 0; x < 22; x++) vga_putc('#');
    vga_puts("\n");
    
    vga_puts("\n  Controls: w/a/s/d = move\n");
    vga_puts("  Press 'q' to quit\n\n");
}

// Command: snake
void cmd_snake(const char* args) {
    if (strcmp_t(args, "quit") == 0 || strcmp_t(args, "q") == 0) {
        vga_puts("  Game over! Score: ");
        { char nb[8]; num_to_str(snake_score, nb); vga_puts(nb); }
        vga_puts("\n");
        return;
    }
    
    if (strcmp_t(args, "start") == 0 || strcmp_t(args, "") == 0) {
        snake_init();
        vga_puts("\n  [Starting Snake]\n\n");
        snake_draw();
        return;
    }
    
    if (snake_game_over) {
        vga_puts("  Game over! Score: ");
        { char nb[8]; num_to_str(snake_score, nb); vga_puts(nb); }
        vga_puts("\n  Type 'snake start' to play again\n");
        return;
    }
    
    // Process input
    if (strcmp_t(args, "w") == 0 && snake_dir != 1) snake_dir = 3;
    else if (strcmp_t(args, "s") == 0 && snake_dir != 3) snake_dir = 1;
    else if (strcmp_t(args, "a") == 0 && snake_dir != 0) snake_dir = 2;
    else if (strcmp_t(args, "d") == 0 && snake_dir != 2) snake_dir = 0;
    
    snake_update();
    snake_draw();
}

// Command: pong (simplified single player)
static int pong_y = 10;
static int pong_score = 0;
static int pong_ball_x = 10, pong_ball_y = 10;
static int pong_ball_dx = 1, pong_ball_dy = 1;
static int pong_game_over = 0;

static void pong_init(void) {
    pong_y = 10;
    pong_score = 0;
    pong_ball_x = 10;
    pong_ball_y = 10;
    pong_ball_dx = 1;
    pong_ball_dy = 1;
    pong_game_over = 0;
}

static void pong_update(void) {
    if (pong_game_over) return;
    
    // Move ball
    pong_ball_x += pong_ball_dx;
    pong_ball_y += pong_ball_dy;
    
    // Bounce off top/bottom
    if (pong_ball_y <= 0 || pong_ball_y >= 19) {
        pong_ball_dy = -pong_ball_dy;
    }
    
    // Bounce off right wall
    if (pong_ball_x >= 19) {
        pong_ball_dx = -pong_ball_dx;
    }
    
    // Check paddle hit
    if (pong_ball_x == 1 && pong_ball_y >= pong_y && pong_ball_y < pong_y + 4) {
        pong_ball_dx = 1;
        pong_score += 10;
    }
    
    // Check miss
    if (pong_ball_x < 0) {
        pong_game_over = 1;
    }
}

static void pong_draw(void) {
    vga_puts("\n  [Pong] Score: ");
    { char nb[8]; num_to_str(pong_score, nb); vga_puts(nb); }
    vga_puts("\n\n");
    
    for (int y = 0; y < 20; y++) {
        vga_puts("  ");
        for (int x = 0; x < 20; x++) {
            if (x == 0 && y >= pong_y && y < pong_y + 4) {
                vga_putc('|');
            } else if (x == (int)pong_ball_x && y == (int)pong_ball_y) {
                vga_putc('O');
            } else {
                vga_putc('.');
            }
        }
        vga_puts("\n");
    }
    
    vga_puts("\n  Controls: w/s = move paddle\n");
    vga_puts("  Press 'q' to quit\n\n");
}

// Command: pong
void cmd_pong(const char* args) {
    if (strcmp_t(args, "quit") == 0 || strcmp_t(args, "q") == 0) {
        vga_puts("  Game over! Score: ");
        { char nb[8]; num_to_str(pong_score, nb); vga_puts(nb); }
        vga_puts("\n");
        return;
    }
    
    if (strcmp_t(args, "start") == 0 || strcmp_t(args, "") == 0) {
        pong_init();
        vga_puts("\n  [Starting Pong]\n\n");
        pong_draw();
        return;
    }
    
    if (pong_game_over) {
        vga_puts("  Game over! Score: ");
        { char nb[8]; num_to_str(pong_score, nb); vga_puts(nb); }
        vga_puts("\n  Type 'pong start' to play again\n");
        return;
    }
    
    // Process input
    if (strcmp_t(args, "w") == 0 && pong_y > 0) pong_y--;
    else if (strcmp_t(args, "s") == 0 && pong_y < 16) pong_y++;
    
    pong_update();
    pong_draw();
}
