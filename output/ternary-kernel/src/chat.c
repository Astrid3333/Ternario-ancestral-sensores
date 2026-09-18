/**
 * chat.c — Cliente de chat para kernel ternario ancestral
 *
 * Mensajería simple entre usuarios
 */

#include "../include/ternary.h"

#define CHAT_MAX_MSG    256
#define CHAT_MAX_USERS  8
#define CHAT_MAX_MSGS   32
#define CHAT_MAX_ROOMS  4
#define CHAT_MAX_NAME   32

typedef struct {
    char     name[CHAT_MAX_NAME];
    uint32_t ip;
    uint8_t  online;
} chat_user_t;

typedef struct {
    char     from[CHAT_MAX_NAME];
    char     room[CHAT_MAX_NAME];
    char     message[CHAT_MAX_MSG];
    uint32_t timestamp;
    uint8_t  in_use;
} chat_msg_t;

typedef struct {
    char     name[CHAT_MAX_NAME];
    uint8_t  active;
} chat_room_t;

static chat_user_t users[CHAT_MAX_USERS];
static chat_msg_t messages[CHAT_MAX_MSGS];
static chat_room_t rooms[CHAT_MAX_ROOMS];
static uint8_t n_users = 0;
static uint8_t n_messages = 0;
static uint8_t n_rooms = 0;
static char current_room[CHAT_MAX_NAME] = "general";

// Initialize chat
void chat_init(void) {
    vga_puts("[CHAT] Chat client initialized\n");
    
    // Create default room
    strcpy_t(rooms[0].name, "general");
    rooms[0].active = 1;
    n_rooms = 1;
    
    // Add self
    strcpy_t(users[0].name, "tritos");
    users[0].ip = 0;
    users[0].online = 1;
    n_users = 1;
}

// Join room
void chat_join(const char* room) {
    // Check if room exists
    for (int i = 0; i < n_rooms; i++) {
        if (strcmp_t(rooms[i].name, room) == 0) {
            strcpy_t(current_room, room);
            vga_puts("[CHAT] Joined room: ");
            vga_puts(room);
            vga_puts("\n");
            return;
        }
    }
    
    // Create new room
    if (n_rooms < CHAT_MAX_ROOMS) {
        strcpy_t(rooms[n_rooms].name, room);
        rooms[n_rooms].active = 1;
        n_rooms++;
        
        strcpy_t(current_room, room);
        vga_puts("[CHAT] Created and joined room: ");
        vga_puts(room);
        vga_puts("\n");
    } else {
        vga_puts("[CHAT] Too many rooms\n");
    }
}

// Send message
void chat_send(const char* message) {
    if (n_messages >= CHAT_MAX_MSGS) {
        // Shift messages
        for (int i = 0; i < CHAT_MAX_MSGS - 1; i++) {
            messages[i] = messages[i + 1];
        }
        n_messages--;
    }
    
    strcpy_t(messages[n_messages].from, users[0].name);
    strcpy_t(messages[n_messages].room, current_room);
    strcpy_t(messages[n_messages].message, message);
    messages[n_messages].timestamp = 0;
    messages[n_messages].in_use = 1;
    n_messages++;
    
    // Show message
    vga_puts("[");
    vga_set_color(0x0B, 0);
    vga_puts(users[0].name);
    vga_set_color(0x07, 0);
    vga_puts("] ");
    vga_puts(message);
    vga_puts("\n");
}

// Show chat history
void chat_history(void) {
    vga_puts("\n  Chat history (");
    vga_puts(current_room);
    vga_puts("):\n\n");
    
    for (int i = 0; i < CHAT_MAX_MSGS; i++) {
        if (messages[i].in_use && strcmp_t(messages[i].room, current_room) == 0) {
            vga_puts("  [");
            vga_set_color(0x0B, 0);
            vga_puts(messages[i].from);
            vga_set_color(0x07, 0);
            vga_puts("] ");
            vga_puts(messages[i].message);
            vga_puts("\n");
        }
    }
}

// List users
void chat_users(void) {
    vga_puts("\n  Online users:\n\n");
    
    for (int i = 0; i < CHAT_MAX_USERS; i++) {
        if (users[i].online) {
            vga_puts("  ");
            vga_set_color(0x0A, 0);
            vga_puts("*");
            vga_set_color(0x07, 0);
            vga_puts(" ");
            vga_puts(users[i].name);
            
            if (i == 0) {
                vga_puts(" (you)");
            }
            
            vga_puts("\n");
        }
    }
}

// List rooms
void chat_rooms(void) {
    vga_puts("\n  Chat rooms:\n\n");
    
    for (int i = 0; i < CHAT_MAX_ROOMS; i++) {
        if (rooms[i].active) {
            vga_puts("  ");
            if (strcmp_t(rooms[i].name, current_room) == 0) {
                vga_set_color(0x0A, 0);
                vga_puts("*");
            } else {
                vga_puts(" ");
            }
            vga_set_color(0x07, 0);
            vga_puts(" ");
            vga_puts(rooms[i].name);
            vga_puts("\n");
        }
    }
}

// Leave room
void chat_leave(void) {
    strcpy_t(current_room, "general");
    vga_puts("[CHAT] Left room, now in: general\n");
}

// Show chat status
void chat_status(void) {
    vga_puts("\n  Chat status:\n\n");
    
    vga_puts("  Current room: ");
    vga_puts(current_room);
    vga_puts("\n");
    
    vga_puts("  Users online: ");
    { char nb[4]; num_to_str(n_users, nb); vga_puts(nb); }
    vga_puts("\n");
    
    vga_puts("  Messages: ");
    { char nb[4]; num_to_str(n_messages, nb); vga_puts(nb); }
    vga_puts("\n");
    
    vga_puts("  Rooms: ");
    { char nb[4]; num_to_str(n_rooms, nb); vga_puts(nb); }
    vga_puts("\n");
}

// Private message
void chat_pm(const char* user, const char* message) {
    vga_puts("[PM to ");
    vga_puts(user);
    vga_puts("] ");
    vga_puts(message);
    vga_puts("\n");
}
