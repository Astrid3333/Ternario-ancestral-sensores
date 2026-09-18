/**
 * ftp.c — Cliente FTP para kernel ternario ancestral
 *
 * Descargar archivos desde servidores FTP
 */

#include "../include/ternary.h"

#define FTP_MAX_PATH 128

// FTP commands
static void ftp_send_cmd(int8_t sock, const char* cmd) {
    tcp_send(sock, (uint8_t*)cmd, strlen_t(cmd));
    tcp_send(sock, (uint8_t*)"\r\n", 2);
}

// FTP login
static int8_t ftp_login(int8_t sock, const char* user, const char* pass) {
    uint8_t buf[512];
    
    // Receive greeting
    tcp_recv(sock, buf, sizeof(buf));
    
    // Send USER
    ftp_send_cmd(sock, "USER ");
    ftp_send_cmd(sock, user);
    tcp_recv(sock, buf, sizeof(buf));
    
    // Send PASS
    ftp_send_cmd(sock, "PASS ");
    ftp_send_cmd(sock, pass);
    tcp_recv(sock, buf, sizeof(buf));
    
    return 0;
}

// FTP connect
int8_t ftp_connect(const char* host, uint16_t port) {
    vga_puts("[FTP] Connecting to ");
    vga_puts(host);
    vga_puts(":");
    { char nb[8]; num_to_str(port, nb); vga_puts(nb); }
    vga_puts("\n");
    
    uint32_t ip = dns_resolve(host);
    if (ip == 0) {
        vga_puts("[FTP] Cannot resolve host\n");
        return -1;
    }
    
    int8_t sock = tcp_connect(ip, port);
    if (sock < 0) {
        vga_puts("[FTP] Connection failed\n");
        return -1;
    }
    
    return sock;
}

// FTP download file
int8_t ftp_get(const char* host, const char* path, const char* filename) {
    int8_t sock = ftp_connect(host, 21);
    if (sock < 0) return -1;
    
    // Login as anonymous
    ftp_login(sock, "anonymous", "tritos@");
    
    // Set binary mode
    ftp_send_cmd(sock, "TYPE I");
    uint8_t buf[512];
    tcp_recv(sock, buf, sizeof(buf));
    
    // Get file size
    ftp_send_cmd(sock, "SIZE ");
    ftp_send_cmd(sock, path);
    tcp_recv(sock, buf, sizeof(buf));
    
    // Enter passive mode
    ftp_send_cmd(sock, "PASV");
    tcp_recv(sock, buf, sizeof(buf));
    
    // Parse PASV response to get data port
    // Format: 227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)
    uint16_t data_port = 0;
    for (int i = 0; i < sizeof(buf) - 1; i++) {
        if (buf[i] == '(' && buf[i+1] >= '0' && buf[i+1] <= '9') {
            // Skip 4 commas to get port
            int commas = 0;
            int j = i + 1;
            while (commas < 4 && j < sizeof(buf)) {
                if (buf[j] == ',') commas++;
                j++;
            }
            // Parse port
            data_port = 0;
            while (buf[j] >= '0' && buf[j] <= '9') {
                data_port = data_port * 10 + (buf[j] - '0');
                j++;
            }
            data_port = data_port * 256;
            j++; // skip comma
            while (buf[j] >= '0' && buf[j] <= '9') {
                data_port = data_port + (buf[j] - '0');
                j++;
            }
            break;
        }
    }
    
    if (data_port == 0) {
        vga_puts("[FTP] PASV parse error\n");
        tcp_close(sock);
        return -1;
    }
    
    // Connect to data port
    uint32_t data_ip = dns_resolve(host);
    int8_t data_sock = tcp_connect(data_ip, data_port);
    if (data_sock < 0) {
        vga_puts("[FTP] Data connection failed\n");
        tcp_close(sock);
        return -1;
    }
    
    // Request file
    ftp_send_cmd(sock, "RETR ");
    ftp_send_cmd(sock, path);
    tcp_recv(sock, buf, sizeof(buf));
    
    // Receive file
    uint8_t* file_buf = (uint8_t*)0x200000;
    uint32_t total = 0;
    uint32_t timeout = 50000000;
    
    while (total < 0x100000 && timeout > 0) {
        int32_t n = tcp_recv(data_sock, file_buf + total, 0x100000 - total);
        if (n > 0) {
            total += n;
            timeout = 50000000;
        } else {
            timeout--;
            if (timeout == 0) break;
        }
    }
    
    tcp_close(data_sock);
    
    // Receive response
    tcp_recv(sock, buf, sizeof(buf));
    
    // Quit
    ftp_send_cmd(sock, "QUIT");
    tcp_recv(sock, buf, sizeof(buf));
    
    tcp_close(sock);
    
    if (total > 0) {
        vga_set_color(0x0A, 0);
        vga_puts("[FTP] Downloaded: ");
        vga_puts(filename);
        vga_puts(" (");
        { char nb[8]; num_to_str(total, nb); vga_puts(nb); }
        vga_puts(" bytes)\n");
        vga_set_color(0x07, 0);
        return 0;
    }
    
    return -1;
}

// FTP list files
void ftp_list(const char* host) {
    int8_t sock = ftp_connect(host, 21);
    if (sock < 0) return;
    
    ftp_login(sock, "anonymous", "tritos@");
    
    uint8_t buf[4096];
    
    // Enter passive mode
    ftp_send_cmd(sock, "PASV");
    tcp_recv(sock, buf, sizeof(buf));
    
    // Parse data port (simplified)
    uint16_t data_port = 20; // Default
    
    uint32_t data_ip = dns_resolve(host);
    int8_t data_sock = tcp_connect(data_ip, data_port);
    if (data_sock < 0) {
        tcp_close(sock);
        return;
    }
    
    // List files
    ftp_send_cmd(sock, "LIST");
    tcp_recv(sock, buf, sizeof(buf));
    
    // Receive file list
    uint32_t total = 0;
    while (total < sizeof(buf) - 1) {
        int32_t n = tcp_recv(data_sock, buf + total, sizeof(buf) - total - 1);
        if (n > 0) total += n;
        else break;
    }
    buf[total] = 0;
    
    tcp_close(data_sock);
    tcp_recv(sock, buf, sizeof(buf));
    
    ftp_send_cmd(sock, "QUIT");
    tcp_recv(sock, buf, sizeof(buf));
    
    tcp_close(sock);
    
    // Print file list
    vga_puts("\n  FTP listing:\n\n");
    for (uint32_t i = 0; i < total; i++) {
        if (buf[i] >= 32 && buf[i] < 127) {
            vga_putc(buf[i]);
        } else if (buf[i] == '\n') {
            vga_putc('\n');
        }
    }
    vga_puts("\n");
}
