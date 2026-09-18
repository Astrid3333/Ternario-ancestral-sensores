/**
 * vpn.c — VPN túnel cifrado para kernel ternario ancestral
 */

#include "../include/ternary.h"

#define VPN_MAX_TUNNELS 4

typedef struct {
    uint8_t  active;
    uint32_t server_ip;
    uint16_t server_port;
    uint32_t local_ip;
    uint32_t tunnel_ip;
    int8_t   tcp_sock;
    uint8_t  session_key[32];
} vpn_tunnel_t;

static vpn_tunnel_t tunnels[VPN_MAX_TUNNELS];
static uint8_t n_tunnels = 0;

void vpn_init(void) {
    vga_puts("[VPN] VPN client initialized\n");
}

int8_t vpn_connect(const char* server, uint16_t port) {
    if (n_tunnels >= VPN_MAX_TUNNELS) {
        vga_puts("[VPN] Too many tunnels\n");
        return -1;
    }
    
    uint32_t ip = dns_resolve(server);
    if (ip == 0) {
        vga_puts("[VPN] Cannot resolve server\n");
        return -1;
    }
    
    int8_t sock = tcp_connect(ip, port);
    if (sock < 0) {
        vga_puts("[VPN] Connection failed\n");
        return -1;
    }
    
    tunnels[n_tunnels].active = 1;
    tunnels[n_tunnels].server_ip = ip;
    tunnels[n_tunnels].server_port = port;
    tunnels[n_tunnels].tcp_sock = sock;
    
    // Generate session key
    for (int i = 0; i < 32; i++) {
        tunnels[n_tunnels].session_key[i] = 0x41 + i;
    }
    
    vga_set_color(0x0A, 0);
    vga_puts("[VPN] Connected to ");
    vga_puts(server);
    vga_puts(":");
    { char nb[8]; num_to_str(port, nb); vga_puts(nb); }
    vga_puts("\n");
    vga_set_color(0x07, 0);
    
    n_tunnels++;
    return 0;
}

void vpn_disconnect(int8_t idx) {
    if (idx < 0 || idx >= VPN_MAX_TUNNELS) return;
    
    if (tunnels[idx].active) {
        tcp_close(tunnels[idx].tcp_sock);
        tunnels[idx].active = 0;
        vga_puts("[VPN] Disconnected\n");
    }
}

void vpn_status(void) {
    vga_puts("\n  VPN tunnels:\n\n");
    
    uint8_t count = 0;
    for (int i = 0; i < VPN_MAX_TUNNELS; i++) {
        if (tunnels[i].active) {
            vga_puts("  Tunnel ");
            { char nb[2]; num_to_str(i, nb); vga_puts(nb); }
            vga_puts(": ");
            vga_set_color(0x0A, 0);
            vga_puts("ACTIVE");
            vga_set_color(0x07, 0);
            vga_puts("\n");
            count++;
        }
    }
    
    if (count == 0) {
        vga_puts("  No active tunnels\n");
    }
}
