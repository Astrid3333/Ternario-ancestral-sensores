/**
 * block.c — Bloqueo de dominios para kernel ternario ancestral
 *
 * Bloquea Google, Meta, Grok y otros dominios
 */

#include "../include/ternary.h"

#define BLOCK_MAX 32

typedef struct {
    char     domain[64];
    uint8_t  active;
} block_entry_t;

static block_entry_t blocked[BLOCK_MAX];
static uint8_t n_blocked = 0;
static uint8_t blocking_enabled = 0;

void block_init(void) {
    vga_puts("[BLOCK] Domain blocking initialized\n");
    
    // Add default blocked domains
    block_add("google.com");
    block_add("googleapis.com");
    block_add("gstatic.com");
    block_add("youtube.com");
    block_add("facebook.com");
    block_add("fbcdn.net");
    block_add("instagram.com");
    block_add("twitter.com");
    block_add("x.com");
    block_add("grok.com");
    block_add("tiktok.com");
    block_add("reddit.com");
    
    blocking_enabled = 1;
    vga_puts("[BLOCK] Default blocklist loaded (12 domains)\n");
}

void block_enable(void) {
    blocking_enabled = 1;
    vga_puts("[BLOCK] Blocking enabled\n");
}

void block_disable(void) {
    blocking_enabled = 0;
    vga_puts("[BLOCK] Blocking disabled\n");
}

int8_t block_add(const char* domain) {
    if (n_blocked >= BLOCK_MAX) {
        vga_puts("[BLOCK] Block list full\n");
        return -1;
    }
    
    // Check if already blocked
    for (int i = 0; i < n_blocked; i++) {
        if (strcmp_t(blocked[i].domain, domain) == 0) {
            return 0; // Already blocked
        }
    }
    
    strcpy_t(blocked[n_blocked].domain, domain);
    blocked[n_blocked].active = 1;
    n_blocked++;
    
    return 0;
}

void block_remove(const char* domain) {
    for (int i = 0; i < n_blocked; i++) {
        if (strcmp_t(blocked[i].domain, domain) == 0) {
            blocked[i].active = 0;
            vga_puts("[BLOCK] Removed: ");
            vga_puts(domain);
            vga_puts("\n");
            return;
        }
    }
    vga_puts("[BLOCK] Domain not found\n");
}

uint8_t block_check(const char* domain) {
    if (!blocking_enabled) return 0;
    
    for (int i = 0; i < n_blocked; i++) {
        if (blocked[i].active) {
            // Check if domain ends with blocked domain
            const char* d = domain;
            const char* b = blocked[i].domain;
            uint8_t match = 1;
            
            while (*d && *b) {
                if (*d != *b) { match = 0; break; }
                d++; b++;
            }
            
            if (match && (*d == 0 || *d == '.') && *b == 0) {
                return 1; // Blocked
            }
        }
    }
    
    return 0;
}

void block_list(void) {
    vga_puts("\n  Blocked domains:\n\n");
    
    for (int i = 0; i < n_blocked; i++) {
        if (blocked[i].active) {
            vga_puts("  ");
            vga_set_color(0x0C, 0);
            vga_puts("x");
            vga_set_color(0x07, 0);
            vga_puts(" ");
            vga_puts(blocked[i].domain);
            vga_puts("\n");
        }
    }
}

void block_status(void) {
    vga_puts("\n  Domain blocking: ");
    if (blocking_enabled) {
        vga_set_color(0x0A, 0);
        vga_puts("ENABLED");
    } else {
        vga_set_color(0x0C, 0);
        vga_puts("DISABLED");
    }
    vga_set_color(0x07, 0);
    vga_puts("\n");
    
    vga_puts("  Blocked domains: ");
    { char nb[4]; num_to_str(n_blocked, nb); vga_puts(nb); }
    vga_puts("\n");
}
