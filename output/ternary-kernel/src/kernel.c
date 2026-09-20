/**
 * kernel.c — Kernel Ternario Ancestral v3 (Nivel 3 + Red)
 * 
 * Nivel 3: Terminal + Shell + Network
 * - VGA con scroll
 * - Terminal con buffer de línea
 * - Shell con comandos: ps, mem, fs, cal, fork, kill, help, clear, echo
 * - Keyboard driver completo con scancode set 1
 * - Serial como logging secundario
 * - Network: RTL8139 + Ethernet + ARP + IP + ICMP + UDP
 * 
 * Tamaño: ~12KB de código
 * Memoria: 3.5KB (60 bloques × 60 bytes)
 */

#include "../include/ternary.h"

// =============================================================================
// VGA — Pantalla con scroll (0xB8000)
// =============================================================================

#define VGA_ADDR  0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static uint16_t* vga_buffer = (uint16_t*)VGA_ADDR;
static uint8_t vga_x = 0;
static uint8_t vga_y = 0;
static uint8_t vga_color = 0x07;
static volatile uint8_t serial_input_enabled = 0;

static const char* color_names[] = {
    "black", "blue", "green", "cyan",
    "red", "magenta", "brown", "lightgray",
    "darkgray", "lightblue", "lightgreen", "lightcyan",
    "lightred", "lightmagenta", "yellow", "white"
};

void vga_init(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = 0x0720;
    }
    vga_x = 0;
    vga_y = 0;
}

static void vga_scroll(void) {
    for (int i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
        vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
    }
    for (int i = VGA_WIDTH * (VGA_HEIGHT - 1); i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = 0x0720;
    }
    vga_y = VGA_HEIGHT - 1;
}

void vga_putc(char c) {
    if (serial_input_enabled) serial_putc(c);
    
    if (c == '\n') {
        vga_x = 0;
        vga_y++;
        if (vga_y >= VGA_HEIGHT) vga_scroll();
        return;
    }
    if (c == '\b') {
        if (vga_x > 0) {
            vga_x--;
            vga_buffer[vga_y * VGA_WIDTH + vga_x] = 0x0720;
        }
        return;
    }
    if (c == '\t') {
        vga_x = (vga_x + 8) & ~7;
        if (vga_x >= VGA_WIDTH) {
            vga_x = 0;
            vga_y++;
            if (vga_y >= VGA_HEIGHT) vga_scroll();
        }
        return;
    }
    
    uint16_t entry = (vga_color << 8) | (uint8_t)c;
    vga_buffer[vga_y * VGA_WIDTH + vga_x] = entry;
    
    vga_x++;
    if (vga_x >= VGA_WIDTH) {
        vga_x = 0;
        vga_y++;
        if (vga_y >= VGA_HEIGHT) vga_scroll();
    }
}

void vga_puts(const char* str) {
    while (*str) vga_putc(*str++);
}

void vga_set_color(uint8_t fg, uint8_t bg) {
    vga_color = (bg << 4) | fg;
}

void vga_puts_at(int x, int y, const char* str, uint8_t color) {
    while (*str && x < 80) {
        vga_buffer[y * 80 + x] = (color << 8) | (uint8_t)*str;
        x++;
        str++;
    }
}

void vga_draw_window(int x, int y, int w, int h, const char* title, uint8_t border_color, uint8_t title_color) {
    // Top border
    vga_buffer[y * 80 + x] = (border_color << 8) | 0xDA;
    for (int i = 1; i < w - 1; i++) vga_buffer[y * 80 + x + i] = (border_color << 8) | 0xC4;
    vga_buffer[y * 80 + x + w - 1] = (border_color << 8) | 0xBF;
    
    // Title in top border
    int tx = x + 2;
    while (*title && tx < x + w - 2) {
        vga_buffer[y * 80 + tx] = (title_color << 8) | (uint8_t)*title;
        tx++;
        title++;
    }
    
    // Side borders and empty content
    for (int row = 1; row < h - 1; row++) {
        vga_buffer[(y + row) * 80 + x] = (border_color << 8) | 0xB3;
        for (int col = 1; col < w - 1; col++) {
            vga_buffer[(y + row) * 80 + x + col] = 0x0720;
        }
        vga_buffer[(y + row) * 80 + x + w - 1] = (border_color << 8) | 0xB3;
    }
    
    // Bottom border
    vga_buffer[(y + h - 1) * 80 + x] = (border_color << 8) | 0xC0;
    for (int i = 1; i < w - 1; i++) vga_buffer[(y + h - 1) * 80 + x + i] = (border_color << 8) | 0xC4;
    vga_buffer[(y + h - 1) * 80 + x + w - 1] = (border_color << 8) | 0xD9;
}

void vga_print_trit(trit_t t) {
    if (t == -1) {
        vga_set_color(0x04, 0);
        vga_putc('-');
    } else if (t == 0) {
        vga_set_color(0x07, 0);
        vga_putc('0');
    } else {
        vga_set_color(0x02, 0);
        vga_putc('+');
    }
    vga_set_color(0x07, 0);
}

// =============================================================================
// SERIAL — Logging (0x3F8)
// =============================================================================

#define SERIAL_PORT 0x3F8

void serial_init(void) {
    outb(SERIAL_PORT + 1, 0x00);
    outb(SERIAL_PORT + 3, 0x80);
    outb(SERIAL_PORT + 0, 0x0C);
    outb(SERIAL_PORT + 1, 0x00);
    outb(SERIAL_PORT + 3, 0x03);
    outb(SERIAL_PORT + 2, 0xC7);
    outb(SERIAL_PORT + 4, 0x0B);
}

void serial_putc(char c) {
    while (!(inb(SERIAL_PORT + 5) & 0x20));
    outb(SERIAL_PORT, c);
}

void serial_puts(const char* str) {
    while (*str) serial_putc(*str++);
}

void serial_print_num(int16_t num) {
    char buf[8];
    int i = 0;
    
    if (num < 0) {
        serial_putc('-');
        num = -num;
    }
    if (num == 0) {
        serial_putc('0');
        return;
    }
    while (num > 0 && i < 7) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    while (i > 0) serial_putc(buf[--i]);
}

// =============================================================================
// NÚMEROS A STRING (ahora en ternary.h como static inline)

// =============================================================================
// QUIPU FILESYSTEM
// =============================================================================

#define MAX_FILES 33

typedef struct {
    char name[8];
    uint8_t size;
    uint8_t block;
    uint8_t type;
    uint8_t permissions;
} __attribute__((packed)) quipu_file_t;

static quipu_file_t files[MAX_FILES];
static uint8_t n_files = 0;
static uint8_t quipu_data[MAX_FILES][60]; // in-memory file data

void fs_quipu_init(void) {
    memset_t(files, 0, sizeof(files));
    memset_t(quipu_data, 0, sizeof(quipu_data));
    
    files[0].name[0] = '/';
    for (uint8_t j = 1; j < 8; j++) files[0].name[j] = 0;
    files[0].type = 1;
    n_files = 1;
}

int8_t quipu_create(const char* name, uint8_t type) {
    if (n_files >= MAX_FILES) return -1;
    if (strlen_t(name) > 7) return -1;
    
    strcpy_t(files[n_files].name, name);
    files[n_files].type = type;
    files[n_files].size = 0;
    files[n_files].block = 0;
    files[n_files].permissions = 0x07;
    
    n_files++;
    return (int8_t)(n_files - 1);
}

int8_t fs_find(const char* name) {
    uint8_t len = strlen_t(name);
    if (len > 7) return -1;
    
    for (uint8_t i = 0; i < n_files; i++) {
        if (strcmp_t(files[i].name, name) == 0) return i;
    }
    return -1;
}

int8_t quipu_delete(uint8_t file_id) {
    if (file_id >= n_files || file_id == 0) return -1;
    
    for (uint8_t i = file_id; i < n_files - 1; i++) {
        files[i] = files[i + 1];
    }
    n_files--;
    return 0;
}

uint8_t fs_count(void) {
    return n_files;
}

const char* fs_get_name(uint8_t idx) {
    if (idx >= n_files) return "?";
    return files[idx].name;
}

uint8_t fs_get_type(uint8_t idx) {
    if (idx >= n_files) return 0;
    return files[idx].type;
}

// =============================================================================
// IDT — Interrupciones
// =============================================================================

typedef struct {
    uint16_t base_lo;
    uint16_t sel;
    uint8_t always0;
    uint8_t flags;
    uint16_t base_hi;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

static idt_entry_t idt[256];
static idt_ptr_t idtp;

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_lo = base & 0xFFFF;
    idt[num].base_hi = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

void idt_install(void) {
    idtp.limit = sizeof(idt_entry_t) * 256 - 1;
    idtp.base = (uint32_t)&idt;
    
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    
    idt_set_gate(32, 0, 0x08, 0x8E);
    idt_set_gate(33, 0, 0x08, 0x8E);
    
    asm volatile("lidt %0" : : "m"(idtp));
}

// =============================================================================
// KEYBOARD DRIVER — Scancode Set 1
// =============================================================================

static const char scancode_ascii[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']', '\n',
    0, 'a','s','d','f','g','h','j','k','l',';','\'','`',
    0, '\\','z','x','c','v','b','n','m',',','.','/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

// =============================================================================
// SHELL — Terminal interactiva
// =============================================================================

#define CMD_MAX 64

static char cmd_buf[CMD_MAX];
static uint8_t cmd_idx = 0;

static void shell_prompt(void) {
    vga_set_color(0x0A, 0);
    vga_puts("ternary");
    vga_set_color(0x07, 0);
    vga_putc('@');
    vga_set_color(0x09, 0);
    vga_puts("mayan");
    vga_set_color(0x07, 0);
    vga_puts("$ ");
}

static void cmd_help(void) {
    vga_puts("\n");
    vga_puts("  help        Show this message\n");
    vga_puts("  ps          List processes\n");
    vga_puts("  mem         Memory status\n");
    vga_puts("  fs          List files\n");
    vga_puts("  cal         Maya calendar\n");
    vga_puts("  fork        Create process\n");
    vga_puts("  kill <pid>  Kill process\n");
    vga_puts("  echo <msg>  Print message\n");
    vga_puts("  clear       Clear screen\n");
    vga_puts("  trit <n>    Show ternary of number\n");
    vga_puts("  b60 <n>     Show Babylonian address\n");
    vga_puts("  ping <ip>   Ping host\n");
    vga_puts("  ifconfig    Network interface config\n");
    vga_puts("  arp         ARP cache\n");
    vga_puts("  netstat     Network status\n");
    vga_puts("  dhcp        Get IP via DHCP\n");
    vga_puts("  dns <host>  Resolve hostname\n");
    vga_puts("  tcp <cmd>   TCP connect/listen\n");
    vga_puts("  wget <url>  Download file via HTTP\n");
    vga_puts("  curl <url>  Show HTTP response\n");
    vga_puts("  ftp <host>  FTP download\n");
    vga_puts("  pkg <cmd>   Package manager\n");
    vga_puts("  fw <cmd>    Firewall\n");
    vga_puts("  dl <cmd>    Download manager\n");
    vga_puts("  mail <cmd>  Email client\n");
    vga_puts("  chat <cmd>  Chat client\n");
    vga_puts("  vpn <cmd>   VPN client\n");
    vga_puts("  doh <cmd>   DNS over HTTPS\n");
    vga_puts("  block <cmd> Domain blocker\n");
    vga_puts("  parental <cmd> Parental control\n");
    vga_puts("  ai <cmd>    AI client\n");
    vga_puts("  sock <cmd>  Socket API\n");
    vga_puts("  wm <cmd>    Window manager\n");
    vga_puts("  user        Launch user shell (Ring 3)\n");
    vga_puts("  proc <cmd>  Process management\n");
    vga_puts("  touch <fn>  Create file\n");
    vga_puts("  write <fn>  Write to file\n");
    vga_puts("  rm <fn>     Delete file\n");
    vga_puts("  cat <fn>    Show file contents\n");
    vga_puts("  exec <fn>   Execute ELF program\n");
    vga_puts("  conv <n>    Convert between bases\n");
    vga_puts("  repl        Ternary REPL\n");
    vga_puts("  ver <a>op<b> Visualize operation\n");
    vga_puts("  tutorial <n> Interactive lesson\n");
    vga_puts("  edu         Education status\n");
    vga_puts("  usb         USB status\n");
    vga_puts("  sound <cmd> Sound/Audio\n");
    vga_puts("  halt        Shutdown\n");
    vga_puts("\n");
}

static void cmd_ps(void) {
    vga_puts("\n  PID  STATE    PRI  CPU  MEM\n");
    vga_puts("  ---  -------  ---  ---  ---\n");
    
    for (uint8_t i = 0; i < MAX_PROCS; i++) {
        process_t info;
        if (sched_get_info(i, &info) == 0) {
            char num_buf[8];
            
            vga_putc(' ');
            num_to_str(i, num_buf);
            while (strlen_t(num_buf) < 4) {
                vga_putc(' ');
                strcat_t(num_buf, " ");
            }
            vga_puts(num_buf);
            
            if (info.state == PROC_ACTIVE) {
                vga_set_color(0x0A, 0);
                vga_puts(" ACTIVE ");
            } else if (info.state == PROC_SLEEPING) {
                vga_set_color(0x0E, 0);
                vga_puts(" SLEEP  ");
            } else {
                vga_set_color(0x08, 0);
                vga_puts(" DEAD   ");
            }
            vga_set_color(0x07, 0);
            
            vga_putc(' ');
            vga_print_trit(info.priority);
            
            vga_puts("  ");
            num_to_str(info.cpu_cycles, num_buf);
            vga_puts(num_buf);
            
            vga_puts("  ");
            num_to_str(info.memory_block, num_buf);
            vga_puts(num_buf);
            
            vga_putc('\n');
        }
    }
    vga_putc('\n');
}

static void cmd_mem(void) {
    uint8_t used, free_count, locked;
    mem_get_status(&used, &free_count, &locked);
    
    char num_buf[8];
    vga_puts("\n  Memory (Base 60 Quipu):\n");
    vga_puts("    Used:   "); num_to_str(used, num_buf); vga_puts(num_buf); vga_putc('\n');
    vga_puts("    Free:   "); num_to_str(free_count, num_buf); vga_puts(num_buf); vga_putc('\n');
    vga_puts("    Locked: "); num_to_str(locked, num_buf); vga_puts(num_buf); vga_putc('\n');
    vga_puts("    Total:  60 blocks x 60 bytes = 3600B\n");
    
    vga_puts("\n    Block map: ");
    for (uint8_t i = 0; i < MEM_BLOCKS; i++) {
        uint8_t c = mem_get_color(i);
        if (c == COLOR_FREE) {
            vga_set_color(0x08, 0);
            vga_putc('.');
        } else if (c == COLOR_KERNEL) {
            vga_set_color(0x0C, 0);
            vga_putc('K');
        } else if (c == COLOR_PROCESS) {
            vga_set_color(0x0A, 0);
            vga_putc('P');
        } else {
            vga_set_color(0x0B, 0);
            vga_putc('#');
        }
        if ((i + 1) % 20 == 0) {
            vga_set_color(0x07, 0);
            vga_puts("\n              ");
        }
    }
    vga_set_color(0x07, 0);
    vga_putc('\n');
}

static void cmd_fs(void) {
    vga_puts("\n  Files (Quipu):\n");
    for (uint8_t i = 0; i < fs_count(); i++) {
        vga_putc(' ');
        vga_set_color(0x09, 0);
        vga_puts(fs_get_name(i));
        vga_set_color(0x07, 0);
        if (fs_get_type(i) == 1) vga_puts("/");
        if (files[i].size > 0) {
            vga_puts("  (");
            char nb[8]; num_to_str(files[i].size, nb);
            vga_puts(nb); vga_puts("B)");
        }
        vga_putc('\n');
    }
    vga_putc('\n');
}

static void cmd_cal(void) {
    uint32_t tick;
    uint8_t tzolkin, haab;
    trit_t load;
    sched_get_state(&tick, &tzolkin, &haab, &load);
    
    char num_buf[8];
    vga_puts("\n  Maya Calendar:\n");
    vga_puts("    Global tick:  "); num_to_str(tick, num_buf); vga_puts(num_buf); vga_putc('\n');
    vga_puts("    Tzolkin day:  "); num_to_str(tzolkin, num_buf); vga_puts(num_buf);
    vga_puts(" / 260\n");
    vga_puts("    Haab day:     "); num_to_str(haab, num_buf); vga_puts(num_buf);
    vga_puts(" / 365\n");
    vga_puts("    System load:  ");
    if (load == -1) vga_puts("LOW");
    else if (load == 0) vga_puts("MEDIUM");
    else vga_puts("HIGH");
    vga_putc('\n');
}

static void cmd_fork(void) {
    int8_t new_pid = sys_fork();
    if (new_pid >= 0) {
        char num_buf[8];
        vga_puts("  Process created: PID ");
        num_to_str(new_pid, num_buf);
        vga_puts(num_buf);
        vga_putc('\n');
    } else {
        vga_puts("  Error: cannot create process\n");
    }
}

static void cmd_kill(const char* arg) {
    if (arg[0] == 0) {
        vga_puts("  Usage: kill <pid>\n");
        return;
    }
    
    int16_t pid = 0;
    const char* p = arg;
    while (*p >= '0' && *p <= '9') {
        pid = pid * 10 + (*p - '0');
        p++;
    }
    
    if (pid <= 0 || pid >= MAX_PROCS) {
        vga_puts("  Invalid PID\n");
        return;
    }
    
    if (sched_kill((uint8_t)pid) == 0) {
        vga_puts("  Process killed\n");
    } else {
        vga_puts("  Error killing process\n");
    }
}

static void cmd_echo(const char* arg) {
    vga_puts("  ");
    vga_puts(arg);
    vga_putc('\n');
}

static void cmd_trit(const char* arg) {
    int16_t num = 0;
    const char* p = arg;
    int neg = 0;
    if (*p == '-') { neg = 1; p++; }
    while (*p >= '0' && *p <= '9') {
        num = num * 10 + (*p - '0');
        p++;
    }
    if (neg) num = -num;
    
    char num_buf[8];
    vga_puts("  ");
    num_to_str(num, num_buf);
    vga_puts(num_buf);
    vga_puts(" in ternary: ");
    
    if (num > 0) {
        for (int i = 7; i >= 0; i--) {
            int digit = num % 3;
            if (digit == 0) vga_putc('0');
            else if (digit == 1) vga_putc('+');
            else vga_putc('-');
            num /= 3;
            if (num == 0) break;
        }
    } else if (num == 0) {
        vga_putc('0');
    } else {
        vga_putc('-');
        num = -num;
        while (num > 0) {
            int digit = num % 3;
            if (digit == 0) vga_putc('0');
            else if (digit == 1) vga_putc('+');
            else vga_putc('-');
            num /= 3;
        }
    }
    vga_putc('\n');
}

static void cmd_b60(const char* arg) {
    int16_t num = 0;
    const char* p = arg;
    while (*p >= '0' && *p <= '9') {
        num = num * 10 + (*p - '0');
        p++;
    }
    
    babilonian_addr_t addr = linear_to_babilonian((uint16_t)num);
    char num_buf[8];
    
    vga_puts("  Linear ");
    num_to_str(num, num_buf);
    vga_puts(num_buf);
    vga_puts(" = Babylonian (");
    num_to_str(addr.high, num_buf);
    vga_puts(num_buf);
    vga_putc(':');
    num_to_str(addr.low, num_buf);
    vga_puts(num_buf);
    vga_puts(")\n");
}

// =============================================================================
// NETWORK COMMANDS
// =============================================================================

static void cmd_ping(const char* arg) {
    if (*arg == 0) {
        vga_puts("  Usage: ping <ip>\n");
        return;
    }

    // Parse IP address
    uint32_t ip = 0;
    uint32_t part = 0;
    int dots = 0;
    const char* p = arg;
    while (*p && dots < 4) {
        if (*p == '.') {
            ip = (ip << 8) | part;
            part = 0;
            dots++;
        } else if (*p >= '0' && *p <= '9') {
            part = part * 10 + (*p - '0');
        }
        p++;
    }
    ip = (ip << 8) | part;

    vga_puts("  Pinging ");
    vga_puts(arg);
    vga_puts("...\n");

    // Send ARP request first
    arp_resolve(ip);

    // Wait briefly for ARP reply
    for (volatile int i = 0; i < 100000; i++);

    // Send ICMP echo request
    icmp_echo_request(ip);

    vga_puts("  Packet sent\n");
}

static void cmd_ifconfig(void) {
    vga_puts("\n  Network Interface: rtl8139\n");
    vga_puts("  IP: 10.0.2.15\n");
    vga_puts("  Gateway: 10.0.2.2\n");
    vga_puts("  Subnet: 255.255.255.0\n\n");
}

static void cmd_arp(void) {
    vga_puts("\n  ARP Cache (use 'arp' to query):\n\n");
}

static void cmd_netstat(void) {
    vga_puts("\n  Network Status:\n");
    vga_puts("  RTL8139: active\n");
    vga_puts("  Link: up\n\n");
}

static void cmd_dns(const char* arg) {
    if (arg[0] == 0) {
        vga_puts("  Usage: dns <hostname>\n");
        vga_puts("  Example: dns google.com\n");
        return;
    }
    vga_puts("  Resolving: ");
    vga_puts(arg);
    vga_puts("\n");
    uint32_t ip = dns_resolve(arg);
    if (ip) {
        vga_puts("  IP: ");
        { char nb[8]; num_to_str((ip >> 24) & 0xFF, nb); vga_puts(nb); vga_puts("."); }
        { char nb[8]; num_to_str((ip >> 16) & 0xFF, nb); vga_puts(nb); vga_puts("."); }
        { char nb[8]; num_to_str((ip >> 8) & 0xFF, nb); vga_puts(nb); vga_puts("."); }
        { char nb[8]; num_to_str(ip & 0xFF, nb); vga_puts(nb); vga_puts("\n"); }
    } else {
        vga_puts("  Failed to resolve\n");
    }
}

static void cmd_tcp(const char* arg) {
    if (strncmp_t(arg, "connect", 7) == 0) {
        vga_puts("  TCP connect: not yet interactive\n");
    } else if (strncmp_t(arg, "listen", 6) == 0) {
        vga_puts("  TCP listen: not yet interactive\n");
    } else {
        vga_puts("  Usage: tcp connect <ip> <port>\n");
        vga_puts("         tcp listen <port>\n");
    }
}

// Forward declarations for new commands
static void cmd_pkg(const char* args);
static void cmd_fw(const char* args);
static void cmd_dl(const char* args);
static void cmd_mail(const char* args);
static void cmd_chat(const char* args);
static void cmd_vpn(const char* args);
static void cmd_doh(const char* args);
static void cmd_block(const char* args);
static void cmd_parental(const char* args);
static void cmd_ai(const char* args);
static void cmd_sock(const char* args);

// Educational commands
static void cmd_conv(const char* args);
static void cmd_repl(const char* args);
static void cmd_ver(const char* args);
static void cmd_wm(const char* args);
static void cmd_proc(const char* args);
static void cmd_usb(const char* args);
static void cmd_sound(const char* args);
static void cmd_touch(const char* args);
static void cmd_write(const char* args);
static void cmd_rm(const char* args);
static void cmd_cat(const char* args);
static void cmd_exec(const char* args);

// =============================================================================
// NEW COMMAND HANDLERS — USB Storage, Dynamic ELF, Multi-User
// =============================================================================

static void cmd_usbs(const char* args) {
    if (strcmp_t(args, "status") == 0) {
        usb_storage_status();
    } else if (strcmp_t(args, "detect") == 0) {
        usb_storage_detect();
    } else if (strncmp_t(args, "mount", 5) == 0) {
        const char* p = args + 6;
        uint8_t dev = 0;
        while (*p >= '0' && *p <= '9') { dev = dev * 10 + (*p - '0'); p++; }
        if (*p == ' ') p++;
        usb_storage_mount(dev, p);
    } else if (strncmp_t(args, "unmount", 7) == 0) {
        const char* p = args + 8;
        uint8_t dev = 0;
        while (*p >= '0' && *p <= '9') { dev = dev * 10 + (*p - '0'); p++; }
        usb_storage_unmount(dev);
    } else {
        vga_puts("  usbs detect          - Detect USB storage\n");
        vga_puts("  usbs status          - Show status\n");
        vga_puts("  usbs mount <N> <mnt> - Mount device N\n");
        vga_puts("  usbs unmount <N>     - Unmount device N\n");
    }
}

static void cmd_dyn(const char* args) {
    if (strcmp_t(args, "syms") == 0) {
        dyn_list_symbols();
    } else if (strcmp_t(args, "libs") == 0) {
        dyn_list_libraries();
    } else {
        vga_puts("  dyn syms             - List symbols\n");
        vga_puts("  dyn libs             - List libraries\n");
    }
}

static void cmd_usermgt(const char* args) {
    if (strcmp_t(args, "status") == 0) {
        user_status();
    } else if (strncmp_t(args, "login", 5) == 0) {
        const char* p = args + 6;
        char name[16], pass[16];
        int ni = 0;
        while (*p && *p != ' ' && ni < 15) { name[ni++] = *p++; }
        name[ni] = 0;
        if (*p == ' ') p++;
        int pi = 0;
        while (*p && pi < 15) { pass[pi++] = *p++; }
        pass[pi] = 0;
        user_login(name, pass);
    } else if (strcmp_t(args, "logout") == 0) {
        user_logout();
    } else if (strncmp_t(args, "add", 3) == 0) {
        const char* p = args + 4;
        char name[16], pass[16];
        int ni = 0;
        while (*p && *p != ' ' && ni < 15) { name[ni++] = *p++; }
        name[ni] = 0;
        if (*p == ' ') p++;
        int pi = 0;
        while (*p && pi < 15) { pass[pi++] = *p++; }
        pass[pi] = 0;
        int uid = user_create(name, pass, 1);
        if (uid >= 0) {
            vga_puts("  Created user: ");
            vga_puts(name);
            vga_puts(" (UID=");
            { char nb[4]; num_to_str(uid, nb); vga_puts(nb); }
            vga_puts(")\n");
        } else {
            vga_puts("  Error creating user\n");
        }
    } else if (strncmp_t(args, "del", 3) == 0) {
        if (user_delete(args + 4) == 0) {
            vga_puts("  Deleted user\n");
        } else {
            vga_puts("  Error deleting user\n");
        }
    } else {
        vga_puts("  usermgt status          - Show users\n");
        vga_puts("  usermgt login <n> <p>   - Login\n");
        vga_puts("  usermgt logout          - Logout\n");
        vga_puts("  usermgt add <n> <p>     - Add user\n");
        vga_puts("  usermgt del <name>      - Delete user\n");
    }
}

// =============================================================================
// DISPATCH TABLE
// =============================================================================

static void shell_process(const char* cmd) {
    if (cmd[0] == 0) return;
    
    if (strcmp_t(cmd, "help") == 0 || cmd[0] == '?') {
        cmd_help();
    } else if (strcmp_t(cmd, "ps") == 0) {
        cmd_ps();
    } else if (strcmp_t(cmd, "mem") == 0) {
        cmd_mem();
    } else if (strcmp_t(cmd, "fs") == 0) {
        cmd_fs();
    } else if (strcmp_t(cmd, "cal") == 0) {
        cmd_cal();
    } else if (strcmp_t(cmd, "fork") == 0) {
        cmd_fork();
    } else if (strncmp_t(cmd, "kill", 4) == 0) {
        cmd_kill(cmd + 5);
    } else if (strncmp_t(cmd, "echo", 4) == 0) {
        cmd_echo(cmd + 5);
    } else if (strcmp_t(cmd, "clear") == 0 || cmd[0] == 12) {
        vga_init();
    } else if (strncmp_t(cmd, "trit", 4) == 0) {
        cmd_trit(cmd + 5);
    } else if (strncmp_t(cmd, "b60", 3) == 0) {
        cmd_b60(cmd + 4);
    } else if (strcmp_t(cmd, "halt") == 0) {
        vga_puts("\n  Shutting down...\n");
        serial_puts("[HALT] System shutdown\n");
        while (1) { asm volatile("hlt"); }
    } else if (strncmp_t(cmd, "ping", 4) == 0) {
        cmd_ping(cmd + 5);
    } else if (strcmp_t(cmd, "ifconfig") == 0) {
        cmd_ifconfig();
    } else if (strcmp_t(cmd, "arp") == 0) {
        cmd_arp();
    } else if (strcmp_t(cmd, "netstat") == 0) {
        cmd_netstat();
    } else if (strcmp_t(cmd, "dhcp") == 0) {
        dhcp_start();
    } else if (strncmp_t(cmd, "dns", 3) == 0) {
        cmd_dns(cmd + 4);
    } else if (strncmp_t(cmd, "tcp", 3) == 0) {
        cmd_tcp(cmd + 4);
    } else if (strncmp_t(cmd, "wget", 4) == 0) {
        cmd_wget(cmd + 5);
    } else if (strncmp_t(cmd, "curl", 4) == 0) {
        cmd_curl(cmd + 5);
    } else if (strncmp_t(cmd, "pkg", 3) == 0) {
        cmd_pkg(cmd + 4);
    } else if (strncmp_t(cmd, "fw", 2) == 0) {
        cmd_fw(cmd + 3);
    } else if (strncmp_t(cmd, "dl", 2) == 0) {
        cmd_dl(cmd + 3);
    } else if (strncmp_t(cmd, "mail", 4) == 0) {
        cmd_mail(cmd + 5);
    } else if (strncmp_t(cmd, "chat", 4) == 0) {
        cmd_chat(cmd + 5);
    } else if (strncmp_t(cmd, "vpn", 3) == 0) {
        cmd_vpn(cmd + 4);
    } else if (strncmp_t(cmd, "doh", 3) == 0) {
        cmd_doh(cmd + 4);
    } else if (strncmp_t(cmd, "block", 5) == 0) {
        cmd_block(cmd + 6);
    } else if (strncmp_t(cmd, "parental", 8) == 0) {
        cmd_parental(cmd + 9);
    } else if (strncmp_t(cmd, "ai", 2) == 0) {
        cmd_ai(cmd + 3);
    } else if (strncmp_t(cmd, "sock", 4) == 0) {
        cmd_sock(cmd + 5);
    } else if (strncmp_t(cmd, "wm", 2) == 0) {
        cmd_wm(cmd + 3);
    } else if (strncmp_t(cmd, "usermgt", 7) == 0) {
        cmd_usermgt(cmd + 8);
    } else if (strncmp_t(cmd, "user", 4) == 0) {
        vga_puts("\n  Launching user shell (Ring 3)...\n");
        shell_user_main();
    } else if (strncmp_t(cmd, "proc", 4) == 0) {
        cmd_proc(cmd + 5);
    } else if (strncmp_t(cmd, "usbs", 4) == 0) {
        cmd_usbs(cmd + 5);
    } else if (strncmp_t(cmd, "usb", 3) == 0) {
        cmd_usb(cmd + 4);
    } else if (strncmp_t(cmd, "sound", 5) == 0) {
        cmd_sound(cmd + 6);
    } else if (strncmp_t(cmd, "touch", 5) == 0) {
        cmd_touch(cmd + 6);
    } else if (strncmp_t(cmd, "write", 5) == 0) {
        cmd_write(cmd + 6);
    } else if (strncmp_t(cmd, "rm", 2) == 0) {
        cmd_rm(cmd + 3);
    } else if (strncmp_t(cmd, "cat", 3) == 0) {
        cmd_cat(cmd + 4);
    } else if (strncmp_t(cmd, "exec", 4) == 0) {
        cmd_exec(cmd + 5);
    } else if (strncmp_t(cmd, "conv", 4) == 0) {
        cmd_conv(cmd + 5);
    } else if (strncmp_t(cmd, "repl", 4) == 0) {
        cmd_repl(cmd + 5);
    } else if (strncmp_t(cmd, "ver", 3) == 0) {
        cmd_ver(cmd + 4);
    } else if (strncmp_t(cmd, "tutorial", 8) == 0) {
        cmd_tutorial(cmd + 9);
    } else if (strcmp_t(cmd, "edu") == 0) {
        edu_status();
    } else if (strncmp_t(cmd, "tri", 3) == 0) {
        cmd_tri(cmd + 4);
    } else if (strcmp_t(cmd, "bench") == 0) {
        cmd_bench("");
    } else if (strcmp_t(cmd, "savings") == 0) {
        cmd_savings("");
    } else if (strcmp_t(cmd, "history") == 0) {
        cmd_hist("");
    } else if (strncmp_t(cmd, "dual", 4) == 0) {
        cmd_dual(cmd + 5);
    } else if (strcmp_t(cmd, "phase3") == 0) {
        phase3_status();
    } else if (strncmp_t(cmd, "vm", 2) == 0) {
        cmd_vm(cmd + 3);
    } else if (strcmp_t(cmd, "logic") == 0) {
        cmd_logic("");
    } else if (strncmp_t(cmd, "compile", 7) == 0) {
        cmd_compile(cmd + 8);
    } else if (strcmp_t(cmd, "phase4") == 0) {
        phase4_status();
    } else if (strncmp_t(cmd, "calc", 4) == 0) {
        cmd_calc(cmd + 5);
    } else if (strcmp_t(cmd, "clock") == 0) {
        cmd_clock("");
    } else if (strncmp_t(cmd, "convert", 7) == 0) {
        cmd_convert(cmd + 8);
    } else if (strncmp_t(cmd, "tetris", 6) == 0) {
        cmd_tetris(cmd + 7);
    } else if (strncmp_t(cmd, "snake", 5) == 0) {
        cmd_snake(cmd + 6);
    } else if (strncmp_t(cmd, "pong", 4) == 0) {
        cmd_pong(cmd + 5);
    } else if (strcmp_t(cmd, "apps") == 0) {
        apps_status();
    } else if (strcmp_t(cmd, "trinary") == 0) {
        cmd_trinary(cmd + 7);
    } else if (strcmp_t(cmd, "tcalc") == 0) {
        cmd_tcalc(cmd + 5);
    } else if (strncmp_t(cmd, "tric", 4) == 0) {
        cmd_tric(cmd + 5);
    } else if (strncmp_t(cmd, "math", 4) == 0) {
        cmd_math(cmd + 5);
    } else if (strncmp_t(cmd, "formula", 7) == 0) {
        cmd_formula(cmd + 8);
    } else if (strncmp_t(cmd, "lab", 3) == 0) {
        const char* sub = cmd + 4;
        if (strncmp_t(sub, "experimento", 11) == 0) {
            cmd_lab_experimento(sub + 12);
        } else if (strncmp_t(sub, "tutorial", 8) == 0) {
            cmd_lab_tutorial(sub + 9);
        } else {
            cmd_lab(sub + 4);
        }
    } else if (strncmp_t(cmd, "dyn", 3) == 0) {
        cmd_dyn(cmd + 4);
    } else {
        vga_set_color(0x0C, 0);
        vga_puts("  Unknown command: ");
        vga_puts(cmd);
        vga_set_color(0x07, 0);
        vga_puts(". Type 'help' for commands.\n");
    }
}

// Command handlers for new modules
static void cmd_pkg(const char* args) {
    if (strncmp_t(args, "install", 7) == 0) {
        pkg_install(args + 8);
    } else if (strncmp_t(args, "remove", 6) == 0) {
        pkg_remove(args + 7);
    } else if (strcmp_t(args, "list") == 0) {
        pkg_list();
    } else if (strncmp_t(args, "search", 6) == 0) {
        pkg_search(args + 7);
    } else if (strcmp_t(args, "update") == 0) {
        pkg_update();
    } else if (strcmp_t(args, "upgrade") == 0) {
        pkg_upgrade();
    } else if (strncmp_t(args, "info", 4) == 0) {
        pkg_info(args + 5);
    } else if (strcmp_t(args, "repos") == 0) {
        pkg_list_repos();
    } else {
        vga_puts("  pkg install <name>  - Install package\n");
        vga_puts("  pkg remove <name>   - Remove package\n");
        vga_puts("  pkg list            - List installed\n");
        vga_puts("  pkg search <query>  - Search packages\n");
        vga_puts("  pkg update          - Update lists\n");
        vga_puts("  pkg upgrade         - Upgrade all\n");
        vga_puts("  pkg info <name>     - Package info\n");
        vga_puts("  pkg repos           - List repos\n");
    }
}

static void cmd_fw(const char* args) {
    if (strcmp_t(args, "enable") == 0) {
        fw_enable();
    } else if (strcmp_t(args, "disable") == 0) {
        fw_disable();
    } else if (strcmp_t(args, "status") == 0) {
        fw_status();
    } else if (strcmp_t(args, "rules") == 0) {
        fw_list_rules();
    } else if (strcmp_t(args, "blocked") == 0) {
        fw_list_blocked();
    } else {
        vga_puts("  fw enable     - Enable firewall\n");
        vga_puts("  fw disable    - Disable firewall\n");
        vga_puts("  fw status     - Show status\n");
        vga_puts("  fw rules      - List rules\n");
        vga_puts("  fw blocked    - List blocked IPs\n");
    }
}

static void cmd_dl(const char* args) {
    if (strcmp_t(args, "list") == 0) {
        dl_list();
    } else if (strcmp_t(args, "status") == 0) {
        dl_status();
    } else {
        vga_puts("  dl list       - List downloads\n");
        vga_puts("  dl status     - Show status\n");
    }
}

static void cmd_mail(const char* args) {
    if (strcmp_t(args, "list") == 0) {
        mail_list();
    } else if (strcmp_t(args, "receive") == 0) {
        mail_receive();
    } else if (strcmp_t(args, "accounts") == 0) {
        mail_accounts();
    } else if (strcmp_t(args, "status") == 0) {
        mail_status();
    } else {
        vga_puts("  mail list      - List emails\n");
        vga_puts("  mail receive   - Check inbox\n");
        vga_puts("  mail accounts  - List accounts\n");
        vga_puts("  mail status    - Show status\n");
    }
}

static void cmd_chat(const char* args) {
    if (strcmp_t(args, "users") == 0) {
        chat_users();
    } else if (strcmp_t(args, "rooms") == 0) {
        chat_rooms();
    } else if (strcmp_t(args, "history") == 0) {
        chat_history();
    } else if (strcmp_t(args, "status") == 0) {
        chat_status();
    } else {
        vga_puts("  chat users     - List users\n");
        vga_puts("  chat rooms     - List rooms\n");
        vga_puts("  chat history   - Show history\n");
        vga_puts("  chat status    - Show status\n");
    }
}

static void cmd_vpn(const char* args) {
    if (strcmp_t(args, "status") == 0) {
        vpn_status();
    } else {
        vga_puts("  vpn status     - Show status\n");
    }
}

static void cmd_doh(const char* args) {
    if (strcmp_t(args, "enable") == 0) {
        doh_enable();
    } else if (strcmp_t(args, "disable") == 0) {
        doh_disable();
    } else if (strcmp_t(args, "status") == 0) {
        doh_status();
    } else {
        vga_puts("  doh enable     - Enable DoH\n");
        vga_puts("  doh disable    - Disable DoH\n");
        vga_puts("  doh status     - Show status\n");
    }
}

static void cmd_block(const char* args) {
    if (strcmp_t(args, "enable") == 0) {
        block_enable();
    } else if (strcmp_t(args, "disable") == 0) {
        block_disable();
    } else if (strcmp_t(args, "list") == 0) {
        block_list();
    } else if (strcmp_t(args, "status") == 0) {
        block_status();
    } else {
        vga_puts("  block enable   - Enable blocking\n");
        vga_puts("  block disable  - Disable blocking\n");
        vga_puts("  block list     - List blocked domains\n");
        vga_puts("  block status   - Show status\n");
    }
}

static void cmd_parental(const char* args) {
    if (strcmp_t(args, "enable") == 0) {
        parental_enable();
    } else if (strcmp_t(args, "disable") == 0) {
        parental_disable();
    } else if (strcmp_t(args, "status") == 0) {
        parental_status();
    } else if (strcmp_t(args, "filters") == 0) {
        parental_list_filters();
    } else {
        vga_puts("  parental enable    - Enable parental control\n");
        vga_puts("  parental disable   - Disable parental control\n");
        vga_puts("  parental status    - Show status\n");
        vga_puts("  parental filters   - List filters\n");
    }
}

static void cmd_ai(const char* args) {
    if (strcmp_t(args, "enable") == 0) {
        ai_enable();
    } else if (strcmp_t(args, "disable") == 0) {
        ai_disable();
    } else if (strcmp_t(args, "status") == 0) {
        ai_status();
    } else if (strcmp_t(args, "history") == 0) {
        ai_history_show();
    } else {
        vga_puts("  ai enable     - Enable AI client\n");
        vga_puts("  ai disable    - Disable AI client\n");
        vga_puts("  ai status     - Show status\n");
        vga_puts("  ai history    - Show history\n");
    }
}

static void cmd_sock(const char* args) {
    if (strcmp_t(args, "list") == 0) {
        sock_list();
    } else {
        vga_puts("  sock list     - List sockets\n");
    }
}

// =============================================================================
// EDUCATIONAL COMMANDS
// =============================================================================

// Forward declarations for edu.c functions
extern void edu_status(void);

static void cmd_conv(const char* args) {
    if (args[0] == 0) {
        vga_puts("  Usage: conv <number>\n");
        vga_puts("  Convert between bases:\n");
        vga_puts("    Decimal:  255\n");
        vga_puts("    Ternary:  +0--0 (balanced)\n");
        vga_puts("    Binary:   11111111\n");
        vga_puts("    Base 60:  4:15\n");
        return;
    }
    
    // Parse number
    int decimal = 0;
    int is_ternary = 0;
    
    for (const char* p = args; *p; p++) {
        if (*p == '+' || *p == '-') {
            is_ternary = 1;
            break;
        }
    }
    
    if (is_ternary) {
        // Ternary to decimal
        decimal = 0;
        const char* p = args;
        while (*p) {
            if (*p == '+') {
                decimal = decimal * 3 + 1;
            } else if (*p == '-') {
                decimal = decimal * 3 - 1;
            } else if (*p >= '0' && *p <= '2') {
                decimal = decimal * 3 + (*p - '0');
            }
            p++;
        }
    } else {
        // Decimal
        const char* p = args;
        int neg = 0;
        if (*p == '-') { neg = 1; p++; }
        while (*p >= '0' && *p <= '9') {
            decimal = decimal * 10 + (*p - '0');
            p++;
        }
        if (neg) decimal = -decimal;
    }
    
    char buf[32];
    
    vga_puts("\n  Conversion for ");
    vga_puts(args);
    vga_puts(" (decimal: ");
    { char nb[8]; num_to_str(decimal, nb); vga_puts(nb); }
    vga_puts(")\n\n");
    
    // Binary
    vga_puts("  Binary:   ");
    if (decimal == 0) {
        vga_puts("0");
    } else {
        int num = decimal < 0 ? -decimal : decimal;
        char temp[32];
        int len = 0;
        while (num > 0) {
            temp[len++] = '0' + (num & 1);
            num >>= 1;
        }
        if (decimal < 0) { vga_puts("-"); }
        for (int i = len - 1; i >= 0; i--) {
            vga_putc(temp[i]);
        }
    }
    vga_puts("\n");
    
    // Ternary
    vga_puts("  Ternary:  ");
    if (decimal == 0) {
        vga_puts("0");
    } else {
        int num = decimal < 0 ? -decimal : decimal;
        char temp[32];
        int len = 0;
        while (num > 0) {
            int rem = num % 3;
            num = num / 3;
            if (rem == 2) {
                temp[len++] = '-';
                num++;
            } else {
                temp[len++] = '0' + rem;
            }
        }
        if (decimal < 0) { vga_puts("-"); }
        for (int i = len - 1; i >= 0; i--) {
            vga_putc(temp[i]);
        }
    }
    vga_puts("\n");
    
    // Base 60
    vga_puts("  Base 60:  ");
    if (decimal == 0) {
        vga_puts("0");
    } else {
        int num = decimal < 0 ? -decimal : decimal;
        char temp[32];
        int len = 0;
        while (num > 0) {
            temp[len++] = '0' + (num % 60);
            num /= 60;
        }
        if (decimal < 0) { vga_puts("-"); }
        for (int i = len - 1; i >= 0; i--) {
            { char nb[4]; num_to_str(temp[i] - '0', nb); vga_puts(nb); }
            if (i > 0) vga_puts(":");
        }
    }
    vga_puts("\n\n");
}

static void cmd_repl(const char* args) {
    if (strcmp_t(args, "help") == 0) {
        vga_puts("\n  Ternary REPL - Interactive Calculator\n\n");
        vga_puts("  Syntax: <ternary> <op> <ternary>\n");
        vga_puts("  Operators: +, -, *, /\n");
        vga_puts("  Ternary digits: + (positive), 0 (zero), - (negative)\n");
        vga_puts("  Example: +1 + +1 = +0- (decimal: 1+1=2)\n\n");
        return;
    }
    
    vga_puts("\n  [Ternary REPL v0.1]\n\n");
    vga_puts("  Ternary digits: + (positive), 0 (zero), - (negative)\n\n");
    vga_puts("  Examples:\n");
    vga_puts("    +1 + +1   = +0-  (decimal: 2)\n");
    vga_puts("    +1 - +1   = 0    (decimal: 0)\n");
    vga_puts("    +1 * +1   = +1   (decimal: 1)\n");
    vga_puts("    +0- * +1  = +0-  (decimal: 2)\n");
    vga_puts("    +0- + +1  = +00  (decimal: 3)\n");
    vga_puts("    +0- * +0- = +1-  (decimal: 4)\n\n");
    
    vga_puts("  Use 'conv <number>' to convert bases\n");
    vga_puts("  Use 'ver <a>op<b>' to visualize operations\n\n");
}

static void cmd_ver(const char* args) {
    if (args[0] == 0) {
        vga_puts("  Usage: ver <a> <op> <b>\n");
        vga_puts("  Show step-by-step binary and ternary operations\n");
        vga_puts("  Example: ver 5 + 3\n");
        return;
    }
    
    // Parse a, op, b
    int a = 0, b = 0;
    char op = '+';
    
    const char* p = args;
    while (*p == ' ') p++;
    
    // Parse a
    while (*p >= '0' && *p <= '9') {
        a = a * 10 + (*p - '0');
        p++;
    }
    
    while (*p == ' ') p++;
    if (*p) op = *p++;
    while (*p == ' ') p++;
    
    // Parse b
    while (*p >= '0' && *p <= '9') {
        b = b * 10 + (*p - '0');
        p++;
    }
    
    int result = 0;
    switch (op) {
        case '+': result = a + b; break;
        case '-': result = a - b; break;
        case '*': result = a * b; break;
        case '/': result = b != 0 ? a / b : 0; break;
    }
    
    char buf[32];
    
    vga_puts("\n  Step-by-step visualization\n\n");
    
    // Binary
    vga_puts("  BINARY:\n");
    vga_puts("    ");
    // a in binary
    { int num = a; char t[32]; int l = 0;
      while (num > 0) { t[l++] = '0' + (num & 1); num >>= 1; }
      for (int i = l-1; i >= 0; i--) vga_putc(t[i]); }
    vga_puts("\n    ");
    vga_putc(op);
    vga_puts(" ");
    // b in binary
    { int num = b; char t[32]; int l = 0;
      while (num > 0) { t[l++] = '0' + (num & 1); num >>= 1; }
      for (int i = l-1; i >= 0; i--) vga_putc(t[i]); }
    vga_puts("\n    -----\n    ");
    // result in binary
    { int num = result; char t[32]; int l = 0;
      if (num == 0) { vga_putc('0'); }
      else { while (num > 0) { t[l++] = '0' + (num & 1); num >>= 1; }
      for (int i = l-1; i >= 0; i--) vga_putc(t[i]); } }
    vga_puts("  = ");
    { char nb[8]; num_to_str(result, nb); vga_puts(nb); }
    vga_puts("\n\n");
    
    // Ternary
    vga_puts("  TERNARY:\n");
    vga_puts("    ");
    // a in ternary
    { int num = a < 0 ? -a : a; char t[32]; int l = 0;
      while (num > 0) { int r = num % 3; num /= 3;
        if (r == 2) { t[l++] = '-'; num++; } else { t[l++] = '0' + r; } }
      if (a < 0) vga_putc('-');
      for (int i = l-1; i >= 0; i--) vga_putc(t[i]); }
    vga_puts("\n    ");
    vga_putc(op);
    vga_puts(" ");
    // b in ternary
    { int num = b < 0 ? -b : b; char t[32]; int l = 0;
      while (num > 0) { int r = num % 3; num /= 3;
        if (r == 2) { t[l++] = '-'; num++; } else { t[l++] = '0' + r; } }
      if (b < 0) vga_putc('-');
      for (int i = l-1; i >= 0; i--) vga_putc(t[i]); }
    vga_puts("\n    -------\n    ");
    // result in ternary
    { int num = result < 0 ? -result : result; char t[32]; int l = 0;
      if (num == 0) { vga_putc('0'); }
      else { while (num > 0) { int r = num % 3; num /= 3;
        if (r == 2) { t[l++] = '-'; num++; } else { t[l++] = '0' + r; } }
      if (result < 0) vga_putc('-');
      for (int i = l-1; i >= 0; i--) vga_putc(t[i]); } }
    vga_puts("  = ");
    { char nb[8]; num_to_str(result, nb); vga_puts(nb); }
    vga_puts("\n\n");
}

static void cmd_wm(const char* args) {
    if (strcmp_t(args, "status") == 0) {
        wm_status();
    } else if (strcmp_t(args, "redraw") == 0) {
        wm_redraw();
    } else if (strncmp_t(args, "create", 6) == 0) {
        int8_t id = wm_create_window("Window", 10, 3, 40, 15, 0x1E);
        if (id > 0) {
            vga_puts("  Created window ");
            { char nb[4]; num_to_str(id, nb); vga_puts(nb); }
            vga_puts("\n");
            wm_redraw();
        }
    } else if (strncmp_t(args, "close", 5) == 0) {
        const char* p = args + 6;
        int8_t id = 0;
        while (*p >= '0' && *p <= '9') { id = id * 10 + (*p - '0'); p++; }
        if (id > 0) { wm_close_window(id); }
    } else if (strncmp_t(args, "demo", 4) == 0) {
        // Modern Linux-style desktop (GNOME/Fedora look)
        
        // === BACKGROUND — dark charcoal ===
        for (int i = 0; i < 80*25; i++) vga_buffer[i] = 0x0820;
        
        // === TOP PANEL (GNOME-style) — dark gray bar ===
        for (int x = 0; x < 80; x++) vga_buffer[x] = (0x07 << 8) | ' ';
        // Left: Activities
        vga_puts_at(1, 0, "Activities", 0x0F);
        vga_puts_at(11, 0, "|", 0x08);
        vga_puts_at(13, 0, "Tritos", 0x0F);
        // Center: clock
        vga_puts_at(34, 0, "Tue Sep 20  01:42", 0x0F);
        // Right: system tray
        vga_puts_at(56, 0, "|", 0x08);
        vga_puts_at(58, 0, "vol", 0x07);
        vga_puts_at(62, 0, "net", 0x0A);
        vga_puts_at(66, 0, "bat", 0x0B);
        vga_puts_at(70, 0, "|", 0x08);
        vga_puts_at(72, 0, "power", 0x0C);
        
        // === BOTTOM DOCK (GNOME-style) — centered icons ===
        for (int x = 0; x < 80; x++) vga_buffer[24*80+x] = (0x07 << 8) | ' ';
        // Dock icons centered
        vga_puts_at(25, 24, "[Term]", 0x0B);
        vga_puts_at(32, 24, "[Files]", 0x0B);
        vga_puts_at(39, 24, "[Sys]", 0x0B);
        vga_puts_at(45, 24, "[Net]", 0x0B);
        vga_puts_at(51, 24, "[Mem]", 0x0B);
        // Active indicator (dot under Term)
        vga_puts_at(27, 23, ".", 0x0A);
        
        // === WINDOW 1: Terminal (top-left, dark bg) ===
        // Title bar
        for (int x = 2; x < 38; x++) vga_buffer[2*80+x] = (0x07 << 8) | ' ';
        vga_buffer[2*80+2] = (0x0A << 8) | ' ';
        vga_puts_at(4, 2, "Terminal", 0x0F);
        vga_buffer[2*80+37] = (0x08 << 8) | '_';
        // Close/minimize/maximize buttons (right side of title)
        vga_puts_at(34, 2, "-", 0x0A);
        vga_puts_at(35, 2, "+", 0x0A);
        vga_puts_at(36, 2, "x", 0x0C);
        // Border
        for (int y = 3; y < 14; y++) {
            vga_buffer[y*80+2] = (0x08 << 8) | '|';
            vga_buffer[y*80+37] = (0x08 << 8) | '|';
        }
        for (int x = 2; x < 38; x++) {
            vga_buffer[14*80+x] = (0x08 << 8) | '_';
        }
        // Content — dark bg (0x00)
        for (int y = 3; y < 14; y++)
            for (int x = 3; x < 37; x++)
                vga_buffer[y*80+x] = 0x0020;
        // Terminal text
        vga_puts_at(3, 3, "astrid@tritos:~$ neofetch", 0x0A);
        vga_puts_at(3, 4, "       ___          ", 0x0C);
        vga_puts_at(3, 5, "      /   \\  OS: TritOS v4.5", 0x07);
        vga_puts_at(3, 6, "     / \\ / \\ Kernel: ternary-ancestral", 0x07);
        vga_puts_at(3, 7, "    /  0 +  -\\ Shell: bash 1.0", 0x07);
        vga_puts_at(3, 8, "   /___|_|___\\ DE: TritDE 1.0", 0x07);
        vga_puts_at(3, 9, "   Ternary Ancestral", 0x0B);
        vga_puts_at(3, 10, " ", 0x00);
        vga_puts_at(3, 11, " Uptime: 1 tick", 0x08);
        vga_puts_at(3, 12, " Memory: 3600B / 3600B", 0x08);
        vga_puts_at(3, 13, "astrid@tritos:~$ _", 0x0A);
        
        // === WINDOW 2: Files (right, with sidebar) ===
        for (int x = 40; x < 78; x++) vga_buffer[2*80+x] = (0x07 << 8) | ' ';
        vga_buffer[2*80+40] = (0x0B << 8) | ' ';
        vga_puts_at(42, 2, "Files", 0x0F);
        vga_buffer[2*80+77] = (0x08 << 8) | '_';
        vga_puts_at(74, 2, "-", 0x0A);
        vga_puts_at(75, 2, "+", 0x0A);
        vga_puts_at(76, 2, "x", 0x0C);
        for (int y = 3; y < 12; y++) {
            vga_buffer[y*80+40] = (0x08 << 8) | '|';
            vga_buffer[y*80+77] = (0x08 << 8) | '|';
        }
        for (int x = 40; x < 78; x++) vga_buffer[12*80+x] = (0x08 << 8) | '_';
        // Sidebar
        for (int y = 3; y < 12; y++) {
            vga_buffer[y*80+50] = (0x08 << 8) | '|';
            for (int x = 41; x < 50; x++) vga_buffer[y*80+x] = 0x0720;
        }
        for (int x = 41; x < 50; x++) vga_buffer[3*80+x] = (0x07 << 8) | '-';
        // Sidebar items
        vga_puts_at(41, 4, " > Home", 0x0F);
        vga_puts_at(41, 5, "   Docs", 0x07);
        vga_puts_at(41, 6, "   Downloads", 0x07);
        vga_puts_at(41, 7, "   Music", 0x07);
        vga_puts_at(41, 8, "   Pictures", 0x07);
        vga_puts_at(41, 9, "   Trash", 0x08);
        // File list
        vga_puts_at(52, 4, "readme.txt    128B", 0x07);
        vga_puts_at(52, 5, "config.cfg     32B", 0x07);
        vga_puts_at(52, 6, "data/          <DIR>", 0x0B);
        vga_puts_at(52, 7, "image.bmp     256B", 0x07);
        vga_puts_at(52, 8, "notes.md       64B", 0x07);
        // Empty space
        for (int y = 9; y < 12; y++)
            for (int x = 41; x < 77; x++) vga_buffer[y*80+x] = 0x0720;
        
        // === WINDOW 3: System Monitor (bottom-left) ===
        for (int x = 2; x < 38; x++) vga_buffer[15*80+x] = (0x07 << 8) | ' ';
        vga_buffer[15*80+2] = (0x0C << 8) | ' ';
        vga_puts_at(4, 15, "System Monitor", 0x0F);
        vga_buffer[15*80+37] = (0x08 << 8) | '_';
        vga_puts_at(34, 15, "-", 0x0A);
        vga_puts_at(35, 15, "+", 0x0A);
        vga_puts_at(36, 15, "x", 0x0C);
        for (int y = 16; y < 23; y++) {
            vga_buffer[y*80+2] = (0x08 << 8) | '|';
            vga_buffer[y*80+37] = (0x08 << 8) | '|';
        }
        for (int x = 2; x < 38; x++) vga_buffer[23*80+x] = (0x08 << 8) | '_';
        // Content
        vga_puts_at(3, 16, "  CPU:  0%  [          ]", 0x07);
        vga_puts_at(3, 17, "  MEM:  2%  [##        ]", 0x0A);
        vga_puts_at(3, 18, "  DISK: 0%  [          ]", 0x07);
        vga_puts_at(3, 19, " ", 0x00);
        vga_puts_at(3, 20, "  Processes: 3", 0x07);
        vga_puts_at(3, 21, "  Threads:   3", 0x07);
        vga_puts_at(3, 22, "  Uptime:    1 tick", 0x08);
        
        // === WINDOW 4: Network (bottom-right) ===
        for (int x = 40; x < 78; x++) vga_buffer[13*80+x] = (0x07 << 8) | ' ';
        vga_buffer[13*80+40] = (0x0D << 8) | ' ';
        vga_puts_at(42, 13, "Network", 0x0F);
        vga_buffer[13*80+77] = (0x08 << 8) | '_';
        vga_puts_at(74, 13, "-", 0x0A);
        vga_puts_at(75, 13, "+", 0x0A);
        vga_puts_at(76, 13, "x", 0x0C);
        for (int y = 14; y < 23; y++) {
            vga_buffer[y*80+40] = (0x08 << 8) | '|';
            vga_buffer[y*80+77] = (0x08 << 8) | '|';
        }
        for (int x = 40; x < 78; x++) vga_buffer[23*80+x] = (0x08 << 8) | '_';
        // Content
        vga_puts_at(42, 14, "Interface: eth0", 0x07);
        vga_puts_at(42, 15, "Status:    up", 0x0A);
        vga_puts_at(42, 16, "IP:        10.0.2.15", 0x07);
        vga_puts_at(42, 17, "Gateway:   10.0.2.2", 0x07);
        vga_puts_at(42, 18, "DNS:       8.8.8.8", 0x07);
        vga_puts_at(42, 19, " ", 0x00);
        vga_puts_at(42, 20, "  TX:      0 B", 0x08);
        vga_puts_at(42, 21, "  RX:      0 B", 0x08);
        
        // === DESKTOP ICONS ===
        vga_puts_at(1, 3, " >", 0x0B);
        vga_puts_at(4, 3, "Home", 0x07);
        vga_puts_at(1, 5, " >", 0x0B);
        vga_puts_at(4, 5, "Trash", 0x07);
        
        vga_puts("  Desktop ready\n");
    } else {
        vga_puts("  wm status          - Show status\n");
        vga_puts("  wm redraw          - Redraw desktop\n");
        vga_puts("  wm create          - Create window\n");
        vga_puts("  wm close <id>      - Close window\n");
        vga_puts("  wm demo            - Create demo windows\n");
    }
}

static void cmd_proc(const char* args) {
    if (strcmp_t(args, "list") == 0 || strcmp_t(args, "ps") == 0) {
        process_list();
    } else if (strncmp_t(args, "create", 6) == 0) {
        const char* name = args + 7;
        if (!*name) name = "user_proc";
        int8_t pid = process_create(name, 0, 0x02);
        if (pid > 0) {
            vga_puts("  Created process '");
            vga_puts(name);
            vga_puts("' with PID ");
            { char nb[4]; num_to_str(pid, nb); vga_puts(nb); }
            vga_puts("\n");
        }
    } else if (strncmp_t(args, "kill", 4) == 0) {
        const char* p = args + 5;
        uint8_t pid = 0;
        while (*p >= '0' && *p <= '9') { pid = pid * 10 + (*p - '0'); p++; }
        if (pid > 0) {
            process_exit(0);
            vga_puts("  Killed process ");
            { char nb[4]; num_to_str(pid, nb); vga_puts(nb); }
            vga_puts("\n");
        }
    } else if (strcmp_t(args, "test") == 0) {
        vga_puts("  Creating test processes...\n");
        int8_t p1 = process_create("test_a", 0, 0x02);
        int8_t p2 = process_create("test_b", 0, 0x02);
        int8_t p3 = process_create("test_c", 0, 0x02);
        vga_puts("  Created PIDs: ");
        { char nb[4]; num_to_str(p1, nb); vga_puts(nb); vga_puts(", "); }
        { char nb[4]; num_to_str(p2, nb); vga_puts(nb); vga_puts(", "); }
        { char nb[4]; num_to_str(p3, nb); vga_puts(nb); }
        vga_puts("\n");
        process_list();
    } else {
        vga_puts("  proc list / ps  - List processes\n");
        vga_puts("  proc create     - Create process\n");
        vga_puts("  proc kill <pid> - Kill process\n");
        vga_puts("  proc test       - Create test processes\n");
    }
}

static void cmd_usb(const char* args) {
    if (strcmp_t(args, "status") == 0 || strcmp_t(args, "") == 0) {
        usb_status();
    } else {
        vga_puts("  usb status  - Show USB status\n");
    }
}

static void cmd_sound(const char* args) {
    if (strcmp_t(args, "status") == 0 || strcmp_t(args, "") == 0) {
        ac97_status();
    } else if (strncmp_t(args, "beep", 4) == 0) {
        ac97_beep(440, 200);
        vga_puts("  Beep!\n");
    } else if (strncmp_t(args, "vol", 3) == 0) {
        ac97_set_volume(31, 31);
        ac97_set_pcm_volume(31, 31);
        vga_puts("  Volume set to max\n");
    } else {
        vga_puts("  sound status  - Show sound status\n");
        vga_puts("  sound beep    - Play beep\n");
        vga_puts("  sound vol     - Set volume max\n");
    }
}

static void cmd_touch(const char* args) {
    if (args[0] == 0) {
        vga_puts("  Usage: touch <filename>\n");
        return;
    }
    
    // Truncate to 7 chars for Quipu
    char name[8];
    int len = strlen_t(args);
    if (len > 7) len = 7;
    for (int i = 0; i < len; i++) name[i] = args[i];
    name[len] = 0;
    
    int8_t idx = quipu_create(name, 0);
    if (idx >= 0) {
        vga_puts("  Created: ");
        vga_puts(name);
        vga_puts("\n");
    } else {
        vga_puts("  Error creating file\n");
    }
}

static void cmd_write(const char* args) {
    // write <filename> <data>
    if (args[0] == 0) {
        vga_puts("  Usage: write <filename> <data>\n");
        return;
    }
    
    // Find space separator
    const char* space = 0;
    for (const char* p = args; *p; p++) {
        if (*p == ' ') { space = p; break; }
    }
    
    if (!space) {
        vga_puts("  Usage: write <filename> <data>\n");
        return;
    }
    
    // Extract filename (up to 7 chars for Quipu)
    char filename[8];
    int nlen = space - args;
    if (nlen > 7) nlen = 7;
    for (int i = 0; i < nlen; i++) filename[i] = args[i];
    filename[nlen] = 0;
    
    const char* data = space + 1;
    uint32_t dlen = strlen_t(data);
    if (dlen > 59) dlen = 59;
    
    // Find existing or create
    int8_t idx = fs_find(filename);
    if (idx < 0) {
        idx = quipu_create(filename, 0);
        if (idx < 0) {
            vga_puts("  Error creating file\n");
            return;
        }
    }
    
    // Copy data to Quipu block
    for (uint32_t i = 0; i < dlen; i++) quipu_data[idx][i] = data[i];
    quipu_data[idx][dlen] = 0;
    files[idx].size = dlen;
    
    vga_puts("  Wrote ");
    char nb[8];
    num_to_str(dlen, nb);
    vga_puts(nb);
    vga_puts(" bytes to ");
    vga_puts(filename);
    vga_puts("\n");
}

static void cmd_rm(const char* args) {
    if (args[0] == 0) {
        vga_puts("  Usage: rm <filename>\n");
        return;
    }
    
    int8_t idx = fs_find(args);
    if (idx < 0) {
        vga_puts("  File not found\n");
        return;
    }
    if (idx == 0) {
        vga_puts("  Cannot delete root\n");
        return;
    }
    
    if (quipu_delete(idx) == 0) {
        vga_puts("  Deleted: ");
        vga_puts(args);
        vga_puts("\n");
    } else {
        vga_puts("  Error deleting file\n");
    }
}

static void cmd_cat(const char* args) {
    if (args[0] == 0) {
        vga_puts("  Usage: cat <filename>\n");
        return;
    }
    
    int8_t idx = fs_find(args);
    if (idx < 0) {
        vga_puts("  File not found\n");
        return;
    }
    
    if (files[idx].size == 0) {
        vga_puts("  (empty)\n");
        return;
    }
    
    for (uint8_t i = 0; i < files[idx].size; i++) {
        vga_putc(quipu_data[idx][i]);
    }
    vga_putc('\n');
}

static void cmd_exec(const char* args) {
    if (args[0] == 0) {
        vga_puts("  Usage: exec <filename>\n");
        vga_puts("  Load and execute ELF32 program\n");
        return;
    }
    
    // Open file
    int8_t fd = fs_open(args, 0);
    if (fd < 0) {
        vga_puts("  Error: cannot open ");
        vga_puts(args);
        vga_puts("\n");
        return;
    }
    
    // Get file size
    int32_t fsize = fs_get_size(args);
    if (fsize <= 0) {
        vga_puts("  Error: empty file\n");
        fs_close(fd);
        return;
    }
    
    // Read file into memory
    uint8_t* buf = (uint8_t*)malloc(fsize);
    if (!buf) {
        vga_puts("  Error: out of memory\n");
        fs_close(fd);
        return;
    }
    
    int32_t read = fs_read(fd, buf, fsize);
    fs_close(fd);
    
    if (read != fsize) {
        vga_puts("  Error: read incomplete\n");
        free(buf);
        return;
    }
    
    // Execute
    vga_puts("  Executing ");
    vga_puts(args);
    vga_puts("...\n");
    
    elf_execute(buf, fsize);
    
    free(buf);
}

// =============================================================================
// KERNEL MAIN — Nivel 2
// =============================================================================

void kernel_main(uint32_t magic, uint32_t mboot_addr) {
    serial_init();
    vga_init();
    serial_input_enabled = 1;  // Enable serial mirroring from start
    serial_puts("[BOOT] Ternary Ancestral Kernel v4.4\n");
    
    vga_set_color(0x0B, 0);
    vga_puts("========================================\n");
    vga_puts("   TERNARY ANCESTRAL KERNEL v4.4\n");
    vga_puts("   GUI Ternaria + FAT16 + USB + Sound\n");
    vga_puts("========================================\n");
    vga_set_color(0x07, 0);
    vga_puts("\n");
    
    // Check multiboot magic
    if (magic != 0x2BADB002) {
        vga_puts("[BOOT] ERROR: Invalid multiboot magic\n");
        return;
    }
    
    // Parse multiboot info
    uint32_t* mboot = (uint32_t*)mboot_addr;
    uint32_t flags = mboot[0];
    
    vga_puts("[BOOT] Multiboot flags: ");
    { char nb[12]; num_to_hex(flags, nb); vga_puts(nb); }
    vga_puts("\n");
    
    // VGA text mode
    vga_puts("[BOOT] VGA text mode 80x25\n");
    
    vga_puts("  [");
    vga_set_color(0x0A, 0);
    vga_puts("OK");
    vga_set_color(0x07, 0);
    vga_puts("] Memory (Base 60): 60 blocks x 60B = 3.5KB\n");
    mem_init();
    
    vga_puts("  [");
    vga_set_color(0x0A, 0);
    vga_puts("OK");
    vga_set_color(0x07, 0);
    vga_puts("] Scheduler (Maya cycles): 33 slots\n");
    sched_init();
    
    vga_puts("  [");
    vga_set_color(0x0A, 0);
    vga_puts("OK");
    vga_set_color(0x07, 0);
    vga_puts("] Filesystem (Quipu): 33 max files\n");
    fs_quipu_init();
    
    vga_puts("  [");
    vga_set_color(0x0A, 0);
    vga_puts("OK");
    vga_set_color(0x07, 0);
    vga_puts("] Network (RTL8139): Ethernet + ARP + IP\n");
    net_init();
    
    // Initialize new modules
    pkg_init();
    fw_init();
    dl_init();
    mail_init();
    chat_init();
    vpn_init();
    doh_init();
    block_init();
    parental_init();
    ai_client_init();
    sockets_init();
    
    vga_puts("  [");
    vga_set_color(0x0A, 0);
    vga_puts("OK");
    vga_set_color(0x07, 0);
    vga_puts("] IDT: 256 gates\n");
    idt_install();
    
    // Initialize OS components (after IDT so faults are handled)
    vga_puts("[INIT] About to load GDT...\n");
    gdt_init();
    vga_puts("[INIT] GDT done, enabling paging...\n");
    paging_init();
    vga_puts("[INIT] Paging done\n");
    vga_puts("[INIT] Setting up syscalls...\n");
    syscalls_init();
    vga_puts("[INIT] Syscalls done, init mouse...\n");
    mouse_init();
    vga_puts("[INIT] Mouse done, init window manager...\n");
    wm_init();
    vga_puts("[INIT] Window manager done, init processes...\n");
    process_init();
    vga_puts("[INIT] Processes done, init USB...\n");
    usb_init();
    vga_puts("[INIT] USB done, init USB storage...\n");
    usb_storage_detect();
    vga_puts("[INIT] USB storage done, init user system...\n");
    user_init();
    vga_puts("[INIT] User system done, init sound...\n");
    ac97_init();
    
    vga_puts("  [");
    vga_set_color(0x0A, 0);
    vga_puts("OK");
    vga_set_color(0x07, 0);
    vga_puts("] Keyboard: scancode set 1\n");
    
    vga_puts("  [");
    vga_set_color(0x0A, 0);
    vga_puts("OK");
    vga_set_color(0x07, 0);
    vga_puts("] Serial: 9600 baud\n\n");
    
    vga_set_color(0x0E, 0);
    vga_puts("  System ready. Type 'help' for commands.\n");
    vga_set_color(0x07, 0);
    vga_puts("\n");
    
    shell_prompt();
    serial_input_enabled = 1;
    
    while (1) {
        // Poll network
        net_poll();
        
        if (inb(0x64) & 1) {
            uint8_t status = inb(0x64);
            uint8_t scancode = inb(0x60);
            
            // Skip mouse data (bit 5 = mouse data available)
            if (status & 0x20) continue;
            
            if (scancode & 0x80) continue;
            if (scancode == 0 || scancode >= 128) continue;
            
            char c = scancode_ascii[scancode];
            
            if (c == '\n') {
                vga_putc('\n');
                cmd_buf[cmd_idx] = 0;
                shell_process(cmd_buf);
                cmd_idx = 0;
                shell_prompt();
            } else if (c == '\b') {
                if (cmd_idx > 0) {
                    cmd_idx--;
                    vga_putc('\b');
                }
            } else if (c != 0 && cmd_idx < CMD_MAX - 1) {
                cmd_buf[cmd_idx++] = c;
                vga_putc(c);
            }
        }
        
        // Also check serial input (for -nographic QEMU)
        // Only poll serial if no PS/2 data available (avoids conflict with GPIO display)
        if (!(inb(0x64) & 1)) {
            if (inb(0x3FD) & 0x01) {  // LSR bit 0 = data ready
                char sc = inb(0x3F8);  // Read from serial port
                if (sc == '\r' || sc == '\n') {
                    vga_putc('\n');
                    cmd_buf[cmd_idx] = 0;
                    shell_process(cmd_buf);
                    cmd_idx = 0;
                    shell_prompt();
                } else if (sc == '\b' || sc == 0x7F) {
                    if (cmd_idx > 0) {
                        cmd_idx--;
                        vga_putc('\b');
                    }
                } else if (sc >= 0x20 && sc < 0x7F && cmd_idx < CMD_MAX - 1) {
                    cmd_buf[cmd_idx++] = sc;
                    vga_putc(sc);
                }
            }
        }
        // No hlt — poll continuously for serial input
    }
}
