/**
 * doh.c — DNS over HTTPS para kernel ternario ancestral
 *
 * Consulta DNS cifrada a través de HTTPS
 */

#include "../include/ternary.h"

#define DOH_MAX_CACHE 16

typedef struct {
    char     name[64];
    uint32_t ip;
    uint32_t expires;
    uint8_t  valid;
} doh_cache_t;

static doh_cache_t doh_cache[DOH_MAX_CACHE];
static uint8_t doh_enabled = 0;

void doh_init(void) {
    vga_puts("[DOH] DNS over HTTPS initialized\n");
    doh_enabled = 0;
    for (int i = 0; i < DOH_MAX_CACHE; i++) {
        doh_cache[i].valid = 0;
    }
}

void doh_enable(void) {
    doh_enabled = 1;
    vga_puts("[DOH] DNS over HTTPS enabled\n");
}

void doh_disable(void) {
    doh_enabled = 0;
    vga_puts("[DOH] DNS over HTTPS disabled\n");
}

uint32_t doh_resolve(const char* hostname) {
    // Check cache first
    for (int i = 0; i < DOH_MAX_CACHE; i++) {
        if (doh_cache[i].valid && strcmp_t(doh_cache[i].name, hostname) == 0) {
            return doh_cache[i].ip;
        }
    }
    
    // Use Cloudflare DoH
    char url[256];
    strcpy_t(url, "https://cloudflare-dns.com/dns-query?name=");
    strcat_t(url, hostname);
    strcat_t(url, "&type=A");
    
    uint8_t buf[4096];
    uint32_t received = https_get(url, buf, sizeof(buf));
    
    if (received > 0) {
        // Parse response (simplified)
        // Look for IP address in JSON response
        for (uint32_t i = 0; i < received - 15; i++) {
            if (buf[i] == '"' && buf[i+1] == 'd' && buf[i+2] == 'a' && buf[i+3] == 't' && buf[i+4] == 'a' && buf[i+5] == '"') {
                // Found data field, extract IP
                uint32_t ip = 0;
                uint8_t octets = 0;
                uint32_t num = 0;
                
                for (uint32_t j = i + 7; j < received && octets < 4; j++) {
                    if (buf[j] >= '0' && buf[j] <= '9') {
                        num = num * 10 + (buf[j] - '0');
                    } else if (buf[j] == '.' || buf[j] == ']') {
                        ip = (ip << 8) | (num & 0xFF);
                        num = 0;
                        octets++;
                    } else if (buf[j] == '"') {
                        break;
                    }
                }
                
                if (octets == 4) {
                    ip = (ip << 24) | (ip >> 8); // Fix byte order
                    
                    // Cache result
                    for (int k = 0; k < DOH_MAX_CACHE; k++) {
                        if (!doh_cache[k].valid) {
                            strcpy_t(doh_cache[k].name, hostname);
                            doh_cache[k].ip = ip;
                            doh_cache[k].valid = 1;
                            break;
                        }
                    }
                    
                    return ip;
                }
            }
        }
    }
    
    return 0;
}

void doh_status(void) {
    vga_puts("\n  DNS over HTTPS: ");
    if (doh_enabled) {
        vga_set_color(0x0A, 0);
        vga_puts("ENABLED");
    } else {
        vga_set_color(0x0C, 0);
        vga_puts("DISABLED");
    }
    vga_set_color(0x07, 0);
    vga_puts("\n");
    
    uint8_t cached = 0;
    for (int i = 0; i < DOH_MAX_CACHE; i++) {
        if (doh_cache[i].valid) cached++;
    }
    
    vga_puts("  Cache entries: ");
    { char nb[4]; num_to_str(cached, nb); vga_puts(nb); }
    vga_puts("\n");
}
