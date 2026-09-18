/**
 * window.c — Sistema de ventanas para kernel ternario ancestral
 *
 * Window manager con drag, resize, minimize, close
 */

#include "../include/ternary.h"

// Window limits
#define MAX_WINDOWS 16
#define MAX_TITLE_LEN 32

// Window states
#define WIN_STATE_NORMAL  0
#define WIN_STATE_MINIMIZED 1
#define WIN_STATE_MAXIMIZED 2

// Title bar height
#define TITLE_BAR_HEIGHT 20
#define BORDER_WIDTH 2

// Window structure
typedef struct {
    uint8_t id;
    char title[MAX_TITLE_LEN];
    int32_t x, y;
    int32_t width, height;
    uint32_t bg_color;
    uint32_t title_color;
    uint8_t state;
    uint8_t visible;
    uint8_t focused;
    uint8_t has_close;
    uint8_t has_minimize;
    uint8_t has_maximize;
} window_t;

// Window manager state
static window_t windows[MAX_WINDOWS];
static uint8_t window_count = 0;
static int8_t focused_window = -1;
static int8_t drag_window = -1;
static int32_t drag_offset_x, drag_offset_y;
static uint8_t wm_initialized = 0;

// Desktop colors — Ternary ancestral theme
#define DESKTOP_BG      TRIT_000    // Fondo: vacío ternario
#define TITLE_BAR_BG    TRIT_100    // Título: espíritu
#define TITLE_BAR_FOCUSED TRIT_200  // Título activo: sangre
#define TITLE_TEXT       TRIT_221    // Texto: luz suave
#define CLOSE_BTN_COLOR TRIT_202    // Cerrar: sangre intensa
#define MINIMIZE_BTN_COLOR TRIT_120 // Minimizar: sol
#define MAXIMIZE_BTN_COLOR TRIT_110 // Maximizar: vida
#define BORDER_COLOR    TRIT_011    // Borde: tierra fuerte
#define BORDER_FOCUSED  TRIT_101    // Borde activo: espíritu+agua

// Initialize window manager
void wm_init(void) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].id = 0;
        windows[i].visible = 0;
    }
    window_count = 0;
    focused_window = -1;
    wm_initialized = 1;
    
    // Fill desktop
    uint32_t fb_w, fb_h;
    fb_get_info(&fb_w, &fb_h);
    fb_fill(DESKTOP_BG);
}

// Create window
int8_t wm_create_window(const char* title, int32_t x, int32_t y,
                        int32_t w, int32_t h, uint32_t bg) {
    if (window_count >= MAX_WINDOWS) return -1;
    
    window_t* win = &windows[window_count];
    win->id = window_count + 1;
    strncpy(win->title, title, MAX_TITLE_LEN - 1);
    win->x = x;
    win->y = y;
    win->width = w;
    win->height = h;
    win->bg_color = bg;
    win->title_color = TITLE_BAR_BG;
    win->state = WIN_STATE_NORMAL;
    win->visible = 1;
    win->focused = 0;
    win->has_close = 1;
    win->has_minimize = 1;
    win->has_maximize = 1;
    
    window_count++;
    focused_window = win->id - 1;
    
    return win->id;
}

// Draw title bar
static void wm_draw_title_bar(window_t* win) {
    uint32_t bar_color = win->focused ? TITLE_BAR_FOCUSED : TITLE_BAR_BG;
    uint32_t border_color = win->focused ? BORDER_FOCUSED : BORDER_COLOR;
    
    // Title bar background
    fb_draw_rect(win->x, win->y, win->width, TITLE_BAR_HEIGHT, bar_color);
    
    // Border
    fb_draw_rect(win->x, win->y, win->width, BORDER_WIDTH, border_color);
    fb_draw_rect(win->x, win->y + win->height - BORDER_WIDTH, win->width, BORDER_WIDTH, border_color);
    fb_draw_rect(win->x, win->y, BORDER_WIDTH, win->height, border_color);
    fb_draw_rect(win->x + win->width - BORDER_WIDTH, win->y, BORDER_WIDTH, win->height, border_color);
    
    // Title text
    fb_draw_string(win->x + 8, win->y + 6, win->title, TITLE_TEXT, bar_color);
    
    // Close button (X)
    if (win->has_close) {
        int32_t bx = win->x + win->width - 22;
        int32_t by = win->y + 4;
        fb_draw_rect(bx, by, 16, 14, CLOSE_BTN_COLOR);
        fb_draw_string(bx + 4, by + 3, "X", 0xFFFFFF, CLOSE_BTN_COLOR);
    }
    
    // Minimize button (_)
    if (win->has_minimize) {
        int32_t bx = win->x + win->width - 42;
        int32_t by = win->y + 4;
        fb_draw_rect(bx, by, 16, 14, MINIMIZE_BTN_COLOR);
        fb_draw_string(bx + 4, by + 3, "_", 0xFFFFFF, MINIMIZE_BTN_COLOR);
    }
    
    // Maximize button ([])
    if (win->has_maximize) {
        int32_t bx = win->x + win->width - 62;
        int32_t by = win->y + 4;
        fb_draw_rect(bx, by, 16, 14, MAXIMIZE_BTN_COLOR);
        fb_draw_string(bx + 2, by + 3, "[]", 0xFFFFFF, MAXIMIZE_BTN_COLOR);
    }
}

// Draw window content area
static void wm_draw_content(window_t* win) {
    // Content background
    fb_draw_rect(win->x + BORDER_WIDTH, win->y + TITLE_BAR_HEIGHT,
                 win->width - 2 * BORDER_WIDTH,
                 win->height - TITLE_BAR_HEIGHT - BORDER_WIDTH,
                 win->bg_color);
}

// Draw a window
static void wm_draw_window(window_t* win) {
    if (!win->visible || win->state == WIN_STATE_MINIMIZED) return;
    
    wm_draw_title_bar(win);
    wm_draw_content(win);
}

// Redraw all windows
void wm_redraw(void) {
    // Fill desktop
    uint32_t fb_w, fb_h;
    fb_get_info(&fb_w, &fb_h);
    fb_fill(DESKTOP_BG);
    
    // Draw taskbar
    fb_draw_rect(0, fb_h - 30, fb_w, 30, 0x0F3460);
    fb_draw_string(8, fb_h - 22, "Tritos", 0xE0E0E0, 0x0F3460);
    
    // Draw each window
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].visible) {
            wm_draw_window(&windows[i]);
        }
    }
    
    // Draw mouse cursor
    mouse_draw_cursor();
}

// Close window
void wm_close_window(int8_t id) {
    if (id < 1 || id > MAX_WINDOWS) return;
    
    window_t* win = &windows[id - 1];
    win->visible = 0;
    
    // Focus next visible window
    if (focused_window == id - 1) {
        focused_window = -1;
        for (int i = 0; i < MAX_WINDOWS; i++) {
            if (windows[i].visible) {
                focused_window = i;
                windows[i].focused = 1;
            } else {
                windows[i].focused = 0;
            }
        }
    }
    
    wm_redraw();
}

// Minimize window
void wm_minimize_window(int8_t id) {
    if (id < 1 || id > MAX_WINDOWS) return;
    
    window_t* win = &windows[id - 1];
    win->state = WIN_STATE_MINIMIZED;
    win->visible = 0;
    
    wm_redraw();
}

// Restore window
void wm_restore_window(int8_t id) {
    if (id < 1 || id > MAX_WINDOWS) return;
    
    window_t* win = &windows[id - 1];
    win->state = WIN_STATE_NORMAL;
    win->visible = 1;
    
    wm_redraw();
}

// Maximize window
void wm_maximize_window(int8_t id) {
    if (id < 1 || id > MAX_WINDOWS) return;
    
    window_t* win = &windows[id - 1];
    
    if (win->state == WIN_STATE_MAXIMIZED) {
        // Restore
        win->state = WIN_STATE_NORMAL;
        // Would need to save previous size
    } else {
        win->state = WIN_STATE_MAXIMIZED;
        uint32_t fb_w, fb_h;
        fb_get_info(&fb_w, &fb_h);
        win->x = 0;
        win->y = 0;
        win->width = fb_w;
        win->height = fb_h - 30; // Leave taskbar
    }
    
    wm_redraw();
}

// Focus window
void wm_focus_window(int8_t id) {
    if (id < 1 || id > MAX_WINDOWS) return;
    
    // Unfocus all
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].focused = 0;
    }
    
    // Focus this one
    windows[id - 1].focused = 1;
    focused_window = id - 1;
    
    wm_redraw();
}

// Move window
void wm_move_window(int8_t id, int32_t x, int32_t y) {
    if (id < 1 || id > MAX_WINDOWS) return;
    
    window_t* win = &windows[id - 1];
    win->x = x;
    win->y = y;
    
    wm_redraw();
}

// Resize window
void wm_resize_window(int8_t id, int32_t w, int32_t h) {
    if (id < 1 || id > MAX_WINDOWS) return;
    
    window_t* win = &windows[id - 1];
    win->width = w;
    win->height = h;
    
    wm_redraw();
}

// Handle mouse click
void wm_handle_click(int32_t mx, int32_t my) {
    // Check taskbar clicks (restore/minimize)
    uint32_t fb_w, fb_h;
    fb_get_info(&fb_w, &fb_h);
    
    if (my >= (int32_t)(fb_h - 30)) {
        // Taskbar click - restore minimized windows
        for (int i = 0; i < MAX_WINDOWS; i++) {
            if (!windows[i].visible && windows[i].state == WIN_STATE_MINIMIZED) {
                wm_restore_window(windows[i].id);
                return;
            }
        }
        return;
    }
    
    // Check window clicks (reverse order for z-order)
    for (int i = MAX_WINDOWS - 1; i >= 0; i--) {
        if (!windows[i].visible) continue;
        
        window_t* win = &windows[i];
        
        // Check title bar
        if (mx >= win->x && mx < win->x + win->width &&
            my >= win->y && my < win->y + TITLE_BAR_HEIGHT) {
            
            // Check close button
            if (win->has_close) {
                int32_t bx = win->x + win->width - 22;
                int32_t by = win->y + 4;
                if (mx >= bx && mx < bx + 16 && my >= by && my < by + 14) {
                    wm_close_window(win->id);
                    return;
                }
            }
            
            // Check minimize button
            if (win->has_minimize) {
                int32_t bx = win->x + win->width - 42;
                int32_t by = win->y + 4;
                if (mx >= bx && mx < bx + 16 && my >= by && my < by + 14) {
                    wm_minimize_window(win->id);
                    return;
                }
            }
            
            // Check maximize button
            if (win->has_maximize) {
                int32_t bx = win->x + win->width - 62;
                int32_t by = win->y + 4;
                if (mx >= bx && mx < bx + 16 && my >= by && my < by + 14) {
                    wm_maximize_window(win->id);
                    return;
                }
            }
            
            // Start drag
            wm_focus_window(win->id);
            drag_window = i;
            drag_offset_x = mx - win->x;
            drag_offset_y = my - win->y;
            return;
        }
        
        // Check content area
        if (mx >= win->x && mx < win->x + win->width &&
            my >= win->y + TITLE_BAR_HEIGHT && my < win->y + win->height) {
            wm_focus_window(win->id);
            return;
        }
    }
}

// Handle mouse drag
void wm_handle_drag(int32_t mx, int32_t my) {
    if (drag_window < 0) return;
    
    window_t* win = &windows[drag_window];
    win->x = mx - drag_offset_x;
    win->y = my - drag_offset_y;
    
    wm_redraw();
}

// Handle mouse release
void wm_handle_release(void) {
    drag_window = -1;
}

// Get window count
uint8_t wm_get_window_count(void) {
    return window_count;
}

// Get focused window
int8_t wm_get_focused(void) {
    return focused_window;
}

// Window manager status
void wm_status(void) {
    printf("\n  Window Manager:\n\n");
    printf("  Windows: %d/%d\n", window_count, MAX_WINDOWS);
    printf("  Focused: %d\n", focused_window + 1);
    printf("  Desktop: DESKTOP_BG\n");
    
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].visible) {
            printf("  [%d] %s at (%d,%d) %dx%d\n",
                   windows[i].id, windows[i].title,
                   windows[i].x, windows[i].y,
                   windows[i].width, windows[i].height);
        }
    }
}
