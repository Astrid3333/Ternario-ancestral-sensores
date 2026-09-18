/**
 * gdt.c — Global Descriptor Table para kernel ternario ancestral
 *
 * GDT 32-bit con segmentos de código, datos y kernel
 */

#include "../include/ternary.h"

// GDT entry structure
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed)) gdt_entry_t;

// GDT pointer structure
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

// TSS entry (for task switching)
typedef struct {
    uint16_t previous_tss;
    uint16_t esp0;
    uint16_t ss0;
    uint16_t esp1;
    uint16_t ss1;
    uint16_t esp2;
    uint16_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint16_t es;
    uint16_t cs;
    uint16_t ss;
    uint16_t ds;
    uint16_t fs;
    uint16_t gs;
    uint16_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed)) tss_entry_t;

// Global variables
static gdt_entry_t gdt[6];
static gdt_ptr_t gdtp;
static tss_entry_t tss;

// External GDT flush function (in gdt_flush.asm)
extern void gdt_flush(uint32_t gdt_ptr);

// Set GDT entry
static void gdt_set_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity) {
    gdt[num].base_low = base & 0xFFFF;
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    
    gdt[num].limit_low = limit & 0xFFFF;
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].granularity |= (granularity & 0xF0);
    
    gdt[num].access = access;
}

// Set TSS entry
static void tss_set_entry(uint16_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t)&tss;
    uint32_t limit = base + sizeof(tss_entry_t);
    
    gdt_set_entry(5, base, limit, 0xE9, 0x00);
    
    memset_t(&tss, 0, sizeof(tss_entry_t));
    
    tss.ss0 = ss0;
    tss.esp0 = esp0;
    tss.cs = 0x08;
    tss.ds = 0x10;
    tss.es = 0x10;
    tss.fs = 0x10;
    tss.gs = 0x10;
    tss.ss = 0x10;
}

// Initialize GDT
void gdt_init(void) {
    vga_puts("[GDT] Initializing Global Descriptor Table...\n");
    
    gdtp.limit = sizeof(gdt_entry_t) * 6 - 1;
    gdtp.base = (uint32_t)&gdt;
    
    // Null segment (required)
    gdt_set_entry(0, 0, 0, 0, 0);
    
    // Kernel code segment (0x08)
    // Base: 0, Limit: 4GB, Access: Present, Ring 0, Code, Readable
    gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    
    // Kernel data segment (0x10)
    // Base: 0, Limit: 4GB, Access: Present, Ring 0, Data, Writable
    gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    
    // User code segment (0x18)
    // Base: 0, Limit: 4GB, Access: Present, Ring 3, Code, Readable
    gdt_set_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    
    // User data segment (0x20)
    // Base: 0, Limit: 4GB, Access: Present, Ring 3, Data, Writable
    gdt_set_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
    
    // TSS segment (0x28)
    tss_set_entry(0x10, 0x90000);
    
    // Load GDT
    gdt_flush((uint32_t)&gdtp);
    
    // Load TSS
    asm volatile("mov $0x2B, %ax");
    asm volatile("ltr %ax");
    
    vga_puts("[GDT] GDT loaded with 6 segments\n");
    vga_puts("[GDT] Kernel: 0x08 (code), 0x10 (data)\n");
    vga_puts("[GDT] User: 0x18 (code), 0x20 (data)\n");
    vga_puts("[GDT] TSS: 0x28\n");
}

// Load GDT (called from assembly)
void gdt_load(uint32_t gdt_ptr_addr) {
    asm volatile("lgdt (%0)" : : "r"(gdt_ptr_addr));
}
