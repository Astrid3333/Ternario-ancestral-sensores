/**
 * kernel.c — Kernel Ternario Ancestral v2 (Nivel 2)
 * 
 * Nivel 2: Terminal + Shell completo
 * - VGA con scroll
 * - Terminal con buffer de línea
 * - Shell con comandos: ps, mem, fs, cal, fork, kill, help, clear, echo
 * - Keyboard driver completo con scancode set 1
 * - Serial como logging secundario
 * 
 * Tamaño: ~6KB de código
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
// NÚMEROS A STRING
// =============================================================================

void num_to_str(int16_t num, char* buf) {
    int i = 0;
    int neg = 0;
    
    if (num < 0) {
        neg = 1;
        num = -num;
    }
    if (num == 0) {
        buf[0] = '0';
        buf[1] = 0;
        return;
    }
    while (num > 0 && i < 6) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    if (neg) buf[i++] = '-';
    
    for (int j = 0; j < i / 2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i - j - 1];
        buf[i - j - 1] = tmp;
    }
    buf[i] = 0;
}

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

void fs_init(void) {
    memset_t(files, 0, sizeof(files));
    
    files[0].name[0] = '/';
    for (uint8_t j = 1; j < 8; j++) files[0].name[j] = 0;
    files[0].type = 1;
    n_files = 1;
}

int8_t fs_create(const char* name, uint8_t type) {
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

int8_t fs_delete(uint8_t file_id) {
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
    
    for (uint8_t i = 0; i < 256; i++) {
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
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
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
    } else {
        vga_set_color(0x0C, 0);
        vga_puts("  Unknown command: ");
        vga_puts(cmd);
        vga_set_color(0x07, 0);
        vga_puts(". Type 'help' for commands.\n");
    }
}

// =============================================================================
// KERNEL MAIN — Nivel 2
// =============================================================================

void kernel_main(void) {
    serial_init();
    vga_init();
    serial_puts("[BOOT] Ternary Ancestral Kernel v2.0\n");
    
    vga_set_color(0x0B, 0);
    vga_puts("========================================\n");
    vga_puts("   TERNARY ANCESTRAL KERNEL v2.0\n");
    vga_puts("   Nivel 2: Terminal + Shell\n");
    vga_puts("========================================\n");
    vga_set_color(0x07, 0);
    vga_puts("\n");
    
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
    fs_init();
    
    vga_puts("  [");
    vga_set_color(0x0A, 0);
    vga_puts("OK");
    vga_set_color(0x07, 0);
    vga_puts("] IDT: 256 gates\n");
    idt_install();
    
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
    
    while (1) {
        if (inb(0x64) & 1) {
            uint8_t scancode = inb(0x60);
            
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
        
        asm volatile("hlt");
    }
}
