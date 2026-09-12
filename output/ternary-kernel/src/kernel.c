/**
 * kernel.c — Kernel Ternario Ancestral
 * 
 * Sistema operativo ultra-liviano basado en:
 * - Lógica ternaria {-1, 0, +1}
 * - Memoria en Base 60 (Babilónico)
 * - Planificación por ciclos Mayas
 * - Sistema de archivos Quipu
 * 
 * Tamaño total: ~4KB de código
 * Memoria: 3.5KB (60 bloques × 60 bytes)
 */

#include "../include/ternary.h"

// Forward declarations
void mem_init(void);
int8_t mem_alloc(uint8_t owner, uint8_t color);
int8_t mem_free(uint8_t block_idx);
void mem_free_all(uint8_t owner);

void sched_init(void);
int8_t sched_create(uint8_t parent, trit_t priority);
uint8_t sched_tick(void);
uint8_t sched_get_current(void);

// =============================================================================
// I/O — Puerto serial (0x3F8)
// =============================================================================

#define SERIAL_PORT 0x3F8

void serial_init() {
    // Configurar puerto serial a 9600 baud
    outb(SERIAL_PORT + 1, 0x00);    // Disable interrupts
    outb(SERIAL_PORT + 3, 0x80);    // Enable DLAB
    outb(SERIAL_PORT + 0, 0x0C);    // Set divisor lo byte (12 -> 9600 baud)
    outb(SERIAL_PORT + 1, 0x00);    // Set divisor hi byte
    outb(SERIAL_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(SERIAL_PORT + 2, 0xC7);    // Enable FIFO
    outb(SERIAL_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

void serial_putc(char c) {
    while (!(inb(SERIAL_PORT + 5) & 0x20));
    outb(SERIAL_PORT, c);
}

void serial_puts(const char* str) {
    while (*str) {
        serial_putc(*str++);
    }
}

void serial_print_trit(trit_t t) {
    if (t == -1) serial_putc('-');
    else if (t == 0) serial_putc('0');
    else serial_putc('+');
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
    
    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    while (i > 0) {
        serial_putc(buf[--i]);
    }
}

// =============================================================================
// VGA — Pantalla de texto (0xB8000)
// =============================================================================

#define VGA_ADDR 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static uint16_t* vga_buffer = (uint16_t*)VGA_ADDR;
static uint8_t vga_x = 0;
static uint8_t vga_y = 0;
static uint8_t vga_color = 0x07;  // Blanco sobre negro

void vga_init() {
    // Limpiar pantalla
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = 0x0720;  // Espacio en blanco
    }
    vga_x = 0;
    vga_y = 0;
}

void vga_putc(char c) {
    if (c == '\n') {
        vga_x = 0;
        vga_y++;
        if (vga_y >= VGA_HEIGHT) vga_y = 0;
        return;
    }
    
    uint16_t entry = (vga_color << 8) | c;
    vga_buffer[vga_y * VGA_WIDTH + vga_x] = entry;
    
    vga_x++;
    if (vga_x >= VGA_WIDTH) {
        vga_x = 0;
        vga_y++;
        if (vga_y >= VGA_HEIGHT) vga_y = 0;
    }
}

void vga_puts(const char* str) {
    while (*str) {
        vga_putc(*str++);
    }
}

void vga_print_trit(trit_t t) {
    if (t == -1) {
        vga_color = 0x04;  // Rojo
        vga_putc('-');
    } else if (t == 0) {
        vga_color = 0x07;  // Blanco
        vga_putc('0');
    } else {
        vga_color = 0x02;  // Verde
        vga_putc('+');
    }
    vga_color = 0x07;  // Restaurar
}

// =============================================================================
// SISTEMA DE ARCHIVOS QUIPU
// =============================================================================

#define MAX_FILES 33  // mod-33

typedef struct {
    char name[8];       // Nombre (8 chars)
    uint8_t size;       // Tamaño en bloques
    uint8_t block;      // Bloque inicial
    uint8_t type;       // Tipo (0=archivo, 1=directorio, 2=ejecutable)
    uint8_t permissions; // Permisos (bits)
} __attribute__((packed)) quipu_file_t;

static quipu_file_t files[MAX_FILES];
static uint8_t n_files = 0;

void fs_init() {
    for (uint8_t i = 0; i < MAX_FILES; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            files[i].name[j] = 0;
        }
        files[i].size = 0;
        files[i].block = 0;
        files[i].type = 0;
        files[i].permissions = 0;
    }
    
    // Crear directorio raíz
    for (uint8_t j = 0; j < 8; j++) {
        files[0].name[j] = "/";
    }
    files[0].type = 1;  // directorio
    files[0].block = 0;
    n_files = 1;
}

int8_t fs_create(const char* name, uint8_t type) {
    if (n_files >= MAX_FILES) return -1;
    
    // Copiar nombre
    uint8_t i;
    for (i = 0; i < 8 && name[i]; i++) {
        files[n_files].name[i] = name[i];
    }
    for (; i < 8; i++) {
        files[n_files].name[i] = 0;
    }
    
    files[n_files].type = type;
    files[n_files].size = 0;
    files[n_files].block = 0;
    files[n_files].permissions = 0x07;  // rw-
    
    n_files++;
    return n_files - 1;
}

int8_t fs_find(const char* name) {
    for (uint8_t i = 0; i < n_files; i++) {
        uint8_t match = 1;
        for (uint8_t j = 0; j < 8; j++) {
            if (files[i].name[j] != name[j]) {
                match = 0;
                break;
            }
        }
        if (match) return i;
    }
    return -1;
}

int8_t fs_delete(uint8_t file_id) {
    if (file_id >= n_files) return -1;
    if (file_id == 0) return -1;  // No borrar raíz
    
    // Mover archivos hacia atrás
    for (uint8_t i = file_id; i < n_files - 1; i++) {
        files[i] = files[i + 1];
    }
    
    n_files--;
    return 0;
}

// =============================================================================
// INTERRUPCiones — IDT básica
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

void idt_install() {
    idtp.limit = sizeof(idt_entry_t) * 256 - 1;
    idtp.base = (uint32_t)&idt;
    
    for (uint8_t i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    
    // Cargar IDT
    asm volatile("lidt %0" : : "m"(idtp));
}

// =============================================================================
// HANDLERS DE INTERRUPCIONES
// =============================================================================

void isr_handler(uint32_t interrupt) {
    if (interrupt == 32) {
        // Timer tick — scheduler
        uint8_t switch_task = sched_tick();
        
        if (switch_task) {
            // Aquí iría el cambio de contexto real
            // Por ahora, solo imprimimos
            serial_puts("[TICK] Context switch\n");
        }
    } else if (interrupt == 33) {
        // Teclado
        uint8_t scancode = inb(0x60);
        // Manejar tecla...
    }
}

// =============================================================================
// KERNEL MAIN
// =============================================================================

void kernel_main() {
    // Inicializar subsistemas
    serial_init();
    vga_init();
    
    // Mostrar banner
    vga_puts("========================================\n");
    vga_puts("   TERNARY ANCESTRAL KERNEL v0.1\n");
    vga_puts("========================================\n\n");
    
    vga_puts("Initializing memory (Base 60)...\n");
    mem_init();
    
    vga_puts("Initializing scheduler (Maya cycles)...\n");
    sched_init();
    
    vga_puts("Initializing filesystem (Quipu)...\n");
    fs_init();
    
    vga_puts("Installing IDT...\n");
    idt_install();
    
    vga_puts("\n");
    vga_puts("System ready.\n");
    vga_puts("\n");
    
    // Demo: crear procesos
    vga_puts("Creating processes...\n");
    
    int8_t proc1 = sched_create(0, 1);   // Alta prioridad
    int8_t proc2 = sched_create(0, 0);   // Media prioridad
    int8_t proc3 = sched_create(0, -1);  // Baja prioridad
    
    vga_puts("  PID 1 (high):   ");
    vga_print_trit(1);
    vga_puts("\n");
    
    vga_puts("  PID 2 (medium): ");
    vga_print_trit(0);
    vga_puts("\n");
    
    vga_puts("  PID 3 (low):    ");
    vga_print_trit(-1);
    vga_puts("\n");
    
    // Demo: memoria
    vga_puts("\nMemory blocks (Babylonian):\n");
    uint8_t mem_used, mem_free, mem_locked;
    mem_get_status(&mem_used, &mem_free, &mem_locked);
    
    vga_puts("  Used:  ");
    serial_print_num(mem_used);
    vga_puts("\n");
    
    vga_puts("  Free:  ");
    serial_print_num(mem_free);
    vga_puts("\n");
    
    vga_puts("  Locked: ");
    serial_print_num(mem_locked);
    vga_puts("\n");
    
    // Demo: archivos
    vga_puts("\nFiles (Quipu):\n");
    fs_create("test.bin", 2);
    fs_create("data.txt", 0);
    fs_create("config", 1);
    
    vga_puts("  Created: test.bin\n");
    vga_puts("  Created: data.txt\n");
    vga_puts("  Created: config\n");
    
    // Demo: calendar Maya
    vga_puts("\nMaya Calendar:\n");
    uint32_t tick;
    uint8_t tzolkin, haab;
    trit_t load;
    sched_get_state(&tick, &tzolkin, &haab, &load);
    
    vga_puts("  Global tick: ");
    serial_print_num(tick);
    vga_puts("\n");
    
    vga_puts("  Tzolkin day: ");
    serial_print_num(tzolkin);
    vga_puts("/260\n");
    
    vga_puts("  Haab day:    ");
    serial_print_num(haab);
    vga_puts("/365\n");
    
    vga_puts("\n");
    vga_puts("Kernel running. Type commands:\n");
    vga_puts("  ps   - List processes\n");
    vga_puts("  mem  - Memory status\n");
    vga_puts("  fs   - List files\n");
    vga_puts("  cal  - Maya calendar\n");
    vga_puts("  halt - Shutdown\n");
    vga_puts("\n");
    
    // Loop principal
    char cmd_buf[16];
    uint8_t cmd_idx = 0;
    
    while (1) {
        // Leer teclado (simplificado)
        if (inb(0x64) & 1) {
            uint8_t scancode = inb(0x60);
            
            // Convertir scancode a ASCII (simplificado)
            if (scancode < 0x80) {
                char c = 0;
                if (scancode >= 0x02 && scancode <= 0x0B) {
                    c = '0' + (scancode - 0x02);
                } else if (scancode >= 0x1E && scancode <= 0x26) {
                    c = 'a' + (scancode - 0x1E);
                } else if (scancode == 0x1C) {
                    // Enter
                    cmd_buf[cmd_idx] = 0;
                    
                    // Procesar comando
                    if (cmd_buf[0] == 'p' && cmd_buf[1] == 's') {
                        vga_puts("\nProcesses:\n");
                        for (uint8_t i = 0; i < MAX_PROCS; i++) {
                            // Mostrar procesos activos
                        }
                    } else if (cmd_buf[0] == 'm' && cmd_buf[1] == 'e' && cmd_buf[2] == 'm') {
                        vga_puts("\nMemory: ");
                        serial_print_num(mem_used);
                        vga_puts(" used, ");
                        serial_print_num(mem_free);
                        vga_puts(" free\n");
                    } else if (cmd_buf[0] == 'c' && cmd_buf[1] == 'a' && cmd_buf[2] == 'l') {
                        vga_puts("\nMaya Calendar:\n");
                        vga_puts("  Tzolkin: ");
                        serial_print_num(tzolkin);
                        vga_puts("/260\n");
                        vga_puts("  Haab: ");
                        serial_print_num(haab);
                        vga_puts("/365\n");
                    } else if (cmd_buf[0] == 'h' && cmd_buf[1] == 'a' && cmd_buf[2] == 'l' && cmd_buf[3] == 't') {
                        vga_puts("\nShutting down...\n");
                        while (1);
                    }
                    
                    vga_putc('\n');
                    cmd_idx = 0;
                } else if (scancode == 0x0E) {
                    // Backspace
                    if (cmd_idx > 0) {
                        cmd_idx--;
                        vga_putc('\b');
                    }
                } else if (c) {
                    cmd_buf[cmd_idx++] = c;
                    vga_putc(c);
                }
            }
        }
        
        // Yield CPU
        asm volatile("hlt");
    }
}
