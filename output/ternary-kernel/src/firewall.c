/**
 * firewall.c — Firewall básico para kernel ternario ancestral
 *
 * Filtrar tráfico por IP, puerto, protocolo
 */

#include "../include/ternary.h"

#define FW_MAX_RULES 32
#define FW_MAX_BLOCK 16

typedef struct {
    uint32_t ip;
    uint32_t mask;
    uint16_t port;
    uint8_t  protocol; // 0=any, 6=TCP, 17=UDP, 1=ICMP
    uint8_t  action;   // 0=allow, 1=deny
    uint8_t  active;
} fw_rule_t;

typedef struct {
    uint32_t ip;
    uint8_t  active;
} fw_block_t;

static fw_rule_t fw_rules[FW_MAX_RULES];
static fw_block_t fw_blocked[FW_MAX_BLOCK];
static uint8_t n_rules = 0;
static uint8_t n_blocked = 0;
static uint8_t fw_enabled = 0;

// Initialize firewall
void fw_init(void) {
    vga_puts("[FW] Firewall initialized\n");
    
    // Default: allow all
    n_rules = 0;
    n_blocked = 0;
    fw_enabled = 0;
}

// Enable firewall
void fw_enable(void) {
    fw_enabled = 1;
    vga_puts("[FW] Firewall enabled\n");
}

// Disable firewall
void fw_disable(void) {
    fw_enabled = 0;
    vga_puts("[FW] Firewall disabled\n");
}

// Add rule
int8_t fw_add_rule(uint32_t ip, uint32_t mask, uint16_t port, 
                   uint8_t protocol, uint8_t action) {
    if (n_rules >= FW_MAX_RULES) {
        vga_puts("[FW] Too many rules\n");
        return -1;
    }
    
    fw_rules[n_rules].ip = ip;
    fw_rules[n_rules].mask = mask;
    fw_rules[n_rules].port = port;
    fw_rules[n_rules].protocol = protocol;
    fw_rules[n_rules].action = action;
    fw_rules[n_rules].active = 1;
    n_rules++;
    
    vga_puts("[FW] Rule added: ");
    if (action == 0) {
        vga_set_color(0x0A, 0);
        vga_puts("ALLOW");
    } else {
        vga_set_color(0x0C, 0);
        vga_puts("DENY");
    }
    vga_set_color(0x07, 0);
    vga_puts(" ");
    
    // Print IP
    { char nb[4]; num_to_str((ip >> 24) & 0xFF, nb); vga_puts(nb); vga_puts("."); }
    { char nb[4]; num_to_str((ip >> 16) & 0xFF, nb); vga_puts(nb); vga_puts("."); }
    { char nb[4]; num_to_str((ip >> 8) & 0xFF, nb); vga_puts(nb); vga_puts("."); }
    { char nb[4]; num_to_str(ip & 0xFF, nb); vga_puts(nb); }
    
    if (port > 0) {
        vga_puts(":");
        { char nb[8]; num_to_str(port, nb); vga_puts(nb); }
    }
    
    vga_puts("\n");
    return 0;
}

// Block IP
void fw_block_ip(uint32_t ip) {
    if (n_blocked >= FW_MAX_BLOCK) {
        vga_puts("[FW] Block list full\n");
        return;
    }
    
    // Check if already blocked
    for (uint8_t i = 0; i < n_blocked; i++) {
        if (fw_blocked[i].ip == ip) {
            vga_puts("[FW] IP already blocked\n");
            return;
        }
    }
    
    fw_blocked[n_blocked].ip = ip;
    fw_blocked[n_blocked].active = 1;
    n_blocked++;
    
    vga_puts("[FW] Blocked IP: ");
    { char nb[4]; num_to_str((ip >> 24) & 0xFF, nb); vga_puts(nb); vga_puts("."); }
    { char nb[4]; num_to_str((ip >> 16) & 0xFF, nb); vga_puts(nb); vga_puts("."); }
    { char nb[4]; num_to_str((ip >> 8) & 0xFF, nb); vga_puts(nb); vga_puts("."); }
    { char nb[4]; num_to_str(ip & 0xFF, nb); vga_puts(nb); }
    vga_puts("\n");
}

// Unblock IP
void fw_unblock_ip(uint32_t ip) {
    for (uint8_t i = 0; i < n_blocked; i++) {
        if (fw_blocked[i].ip == ip) {
            fw_blocked[i].active = 0;
            vga_puts("[FW] Unblocked IP\n");
            return;
        }
    }
    vga_puts("[FW] IP not found in block list\n");
}

// Check if packet is allowed
uint8_t fw_check(uint32_t src_ip, uint16_t port, uint8_t protocol) {
    if (!fw_enabled) return 1; // Allow all if disabled
    
    // Check block list first
    for (uint8_t i = 0; i < n_blocked; i++) {
        if (fw_blocked[i].active && fw_blocked[i].ip == src_ip) {
            return 0; // Denied
        }
    }
    
    // Check rules (last match wins)
    uint8_t result = 1; // Default allow
    for (uint8_t i = 0; i < n_rules; i++) {
        if (!fw_rules[i].active) continue;
        
        // Check IP match
        uint32_t masked_ip = src_ip & fw_rules[i].mask;
        if (masked_ip != (fw_rules[i].ip & fw_rules[i].mask)) continue;
        
        // Check port match
        if (fw_rules[i].port > 0 && fw_rules[i].port != port) continue;
        
        // Check protocol match
        if (fw_rules[i].protocol > 0 && fw_rules[i].protocol != protocol) continue;
        
        result = (fw_rules[i].action == 0) ? 1 : 0;
    }
    
    return result;
}

// List rules
void fw_list_rules(void) {
    vga_puts("\n  Firewall rules:\n\n");
    
    if (n_rules == 0) {
        vga_puts("  No rules defined\n");
        return;
    }
    
    for (uint8_t i = 0; i < n_rules; i++) {
        if (!fw_rules[i].active) continue;
        
        vga_puts("  ");
        { char nb[4]; num_to_str(i + 1, nb); vga_puts(nb); }
        vga_puts(". ");
        
        if (fw_rules[i].action == 0) {
            vga_set_color(0x0A, 0);
            vga_puts("ALLOW");
        } else {
            vga_set_color(0x0C, 0);
            vga_puts("DENY");
        }
        vga_set_color(0x07, 0);
        vga_puts(" ");
        
        // IP
        { char nb[4]; num_to_str((fw_rules[i].ip >> 24) & 0xFF, nb); vga_puts(nb); vga_puts("."); }
        { char nb[4]; num_to_str((fw_rules[i].ip >> 16) & 0xFF, nb); vga_puts(nb); vga_puts("."); }
        { char nb[4]; num_to_str((fw_rules[i].ip >> 8) & 0xFF, nb); vga_puts(nb); vga_puts("."); }
        { char nb[4]; num_to_str(fw_rules[i].ip & 0xFF, nb); vga_puts(nb); }
        
        if (fw_rules[i].port > 0) {
            vga_puts(":");
            { char nb[8]; num_to_str(fw_rules[i].port, nb); vga_puts(nb); }
        }
        
        // Protocol
        vga_puts(" ");
        if (fw_rules[i].protocol == 6) vga_puts("TCP");
        else if (fw_rules[i].protocol == 17) vga_puts("UDP");
        else if (fw_rules[i].protocol == 1) vga_puts("ICMP");
        else vga_puts("ANY");
        
        vga_puts("\n");
    }
}

// List blocked IPs
void fw_list_blocked(void) {
    vga_puts("\n  Blocked IPs:\n\n");
    
    uint8_t count = 0;
    for (uint8_t i = 0; i < n_blocked; i++) {
        if (fw_blocked[i].active) {
            vga_puts("  ");
            { char nb[4]; num_to_str((fw_blocked[i].ip >> 24) & 0xFF, nb); vga_puts(nb); vga_puts("."); }
            { char nb[4]; num_to_str((fw_blocked[i].ip >> 16) & 0xFF, nb); vga_puts(nb); vga_puts("."); }
            { char nb[4]; num_to_str((fw_blocked[i].ip >> 8) & 0xFF, nb); vga_puts(nb); vga_puts("."); }
            { char nb[4]; num_to_str(fw_blocked[i].ip & 0xFF, nb); vga_puts(nb); }
            vga_puts("\n");
            count++;
        }
    }
    
    if (count == 0) {
        vga_puts("  No IPs blocked\n");
    }
}

// Show status
void fw_status(void) {
    vga_puts("\n  Firewall status: ");
    if (fw_enabled) {
        vga_set_color(0x0A, 0);
        vga_puts("ENABLED");
    } else {
        vga_set_color(0x0C, 0);
        vga_puts("DISABLED");
    }
    vga_set_color(0x07, 0);
    vga_puts("\n");
    
    vga_puts("  Rules: ");
    { char nb[4]; num_to_str(n_rules, nb); vga_puts(nb); }
    vga_puts("\n");
    
    vga_puts("  Blocked IPs: ");
    { char nb[4]; num_to_str(n_blocked, nb); vga_puts(nb); }
    vga_puts("\n");
}
