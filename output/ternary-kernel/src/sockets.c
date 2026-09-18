/**
 * sockets.c — API de sockets para kernel ternario ancestral
 *
 * Abstracción de sockets para programación de red
 */

#include "../include/ternary.h"

#define SOCK_MAX 16

typedef struct {
    uint8_t  in_use;
    uint8_t  type;     // 1=TCP, 2=UDP
    uint8_t  state;    // 0=closed, 1=connected, 2=listening
    uint32_t remote_ip;
    uint16_t remote_port;
    uint16_t local_port;
    int8_t   tcp_sock;
    uint8_t  rx_buf[4096];
    uint16_t rx_len;
} socket_t;

static socket_t sockets[SOCK_MAX];
static uint16_t next_port = 49152;

void sockets_init(void) {
    vga_puts("[SOCK] Socket API initialized\n");
    for (int i = 0; i < SOCK_MAX; i++) {
        sockets[i].in_use = 0;
    }
}

// Create socket
int8_t sock_create(uint8_t type) {
    int8_t fd = -1;
    for (int i = 0; i < SOCK_MAX; i++) {
        if (!sockets[i].in_use) {
            fd = i;
            break;
        }
    }
    
    if (fd < 0) {
        vga_puts("[SOCK] Too many sockets\n");
        return -1;
    }
    
    sockets[fd].in_use = 1;
    sockets[fd].type = type;
    sockets[fd].state = 0;
    sockets[fd].rx_len = 0;
    sockets[fd].local_port = next_port++;
    
    return fd;
}

// Connect to remote
int8_t sock_connect(int8_t fd, uint32_t ip, uint16_t port) {
    if (fd < 0 || fd >= SOCK_MAX || !sockets[fd].in_use) return -1;
    
    if (sockets[fd].type == 1) { // TCP
        sockets[fd].tcp_sock = tcp_connect(ip, port);
        if (sockets[fd].tcp_sock >= 0) {
            sockets[fd].remote_ip = ip;
            sockets[fd].remote_port = port;
            sockets[fd].state = 1;
            return 0;
        }
    }
    
    return -1;
}

// Listen
int8_t sock_listen(int8_t fd, uint16_t port) {
    if (fd < 0 || fd >= SOCK_MAX || !sockets[fd].in_use) return -1;
    
    if (sockets[fd].type == 1) { // TCP
        sockets[fd].tcp_sock = tcp_listen(port);
        if (sockets[fd].tcp_sock >= 0) {
            sockets[fd].local_port = port;
            sockets[fd].state = 2;
            return 0;
        }
    }
    
    return -1;
}

// Send data
int32_t sock_send(int8_t fd, const uint8_t* data, uint16_t len) {
    if (fd < 0 || fd >= SOCK_MAX || !sockets[fd].in_use) return -1;
    
    if (sockets[fd].type == 1) { // TCP
        return tcp_send(sockets[fd].tcp_sock, (uint8_t*)data, len);
    }
    
    return -1;
}

// Receive data
int32_t sock_recv(int8_t fd, uint8_t* buf, uint16_t len) {
    if (fd < 0 || fd >= SOCK_MAX || !sockets[fd].in_use) return -1;
    
    if (sockets[fd].type == 1) { // TCP
        return tcp_recv(sockets[fd].tcp_sock, buf, len);
    }
    
    return -1;
}

// Close socket
void sock_close(int8_t fd) {
    if (fd < 0 || fd >= SOCK_MAX || !sockets[fd].in_use) return;
    
    if (sockets[fd].type == 1 && sockets[fd].state == 1) {
        tcp_close(sockets[fd].tcp_sock);
    }
    
    sockets[fd].in_use = 0;
    sockets[fd].state = 0;
}

// Get socket info
void sock_info(int8_t fd) {
    if (fd < 0 || fd >= SOCK_MAX || !sockets[fd].in_use) {
        vga_puts("[SOCK] Invalid socket\n");
        return;
    }
    
    vga_puts("\n  Socket ");
    { char nb[2]; num_to_str(fd, nb); vga_puts(nb); }
    vga_puts(":\n");
    
    vga_puts("    Type: ");
    if (sockets[fd].type == 1) vga_puts("TCP");
    else vga_puts("UDP");
    vga_puts("\n");
    
    vga_puts("    State: ");
    switch (sockets[fd].state) {
        case 0: vga_puts("closed"); break;
        case 1: vga_puts("connected"); break;
        case 2: vga_puts("listening"); break;
    }
    vga_puts("\n");
    
    if (sockets[fd].state == 1) {
        vga_puts("    Remote: ");
        { char nb[4]; num_to_str((sockets[fd].remote_ip >> 24) & 0xFF, nb); vga_puts(nb); vga_puts("."); }
        { char nb[4]; num_to_str((sockets[fd].remote_ip >> 16) & 0xFF, nb); vga_puts(nb); vga_puts("."); }
        { char nb[4]; num_to_str((sockets[fd].remote_ip >> 8) & 0xFF, nb); vga_puts(nb); vga_puts("."); }
        { char nb[4]; num_to_str(sockets[fd].remote_ip & 0xFF, nb); vga_puts(nb); }
        vga_puts(":");
        { char nb[8]; num_to_str(sockets[fd].remote_port, nb); vga_puts(nb); }
        vga_puts("\n");
    }
    
    vga_puts("    Local port: ");
    { char nb[8]; num_to_str(sockets[fd].local_port, nb); vga_puts(nb); }
    vga_puts("\n");
}

// List sockets
void sock_list(void) {
    vga_puts("\n  Sockets:\n\n");
    
    uint8_t count = 0;
    for (int i = 0; i < SOCK_MAX; i++) {
        if (sockets[i].in_use) {
            sock_info(i);
            count++;
        }
    }
    
    if (count == 0) {
        vga_puts("  No open sockets\n");
    }
}
