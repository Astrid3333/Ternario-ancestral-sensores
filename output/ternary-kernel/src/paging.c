/**
 * paging.c — Sistema de paginación para kernel ternario ancestral
 *
 * Page tables, virtual memory, page fault handler
 */

#include "../include/ternary.h"

// Page directory and page table
static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t page_table[1024] __attribute__((aligned(4096)));

// Page fault counter
static uint32_t page_fault_count = 0;

// Page directory entry flags
#define PAGE_PRESENT     0x01
#define PAGE_WRITABLE    0x02
#define PAGE_USER        0x04
#define PAGE_WRITE_THRU  0x08
#define PAGE_CACHE_DISABLE 0x10
#define PAGE_ACCESSED    0x20
#define PAGE_DIRTY       0x40
#define PAGE_SIZE        0x80
#define PAGE_GLOBAL      0x100

// Initialize paging
void paging_init(void) {
    vga_puts("[PAGING] Initializing page tables...\n");
    
    // Clear page directory
    for (int i = 0; i < 1024; i++) {
        page_directory[i] = 0;
    }
    
    // Map first 4MB (identity mapping)
    // Page table entries
    for (int i = 0; i < 1024; i++) {
        page_table[i] = (i * 0x1000) | PAGE_PRESENT | PAGE_WRITABLE;
    }
    
    // First directory entry points to page table
    page_directory[0] = (uint32_t)&page_table | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    
    // Enable paging
    asm volatile("mov %0, %%cr3" : : "r"(page_directory));
    
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // Set PG bit
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
    
    vga_puts("[PAGING] Paging enabled\n");
    vga_puts("[PAGING] Mapped 4MB identity\n");
}

// Map a page
static uint32_t pt_alloc_ptr = 0x200000; // Free area after 2MB for new page tables
void paging_map_page(uint32_t virtual, uint32_t physical, uint32_t flags) {
    uint32_t dir_index = virtual >> 22;
    uint32_t table_index = (virtual >> 12) & 0x3FF;
    
    uint32_t* pt_ptr;
    if (page_directory[dir_index] & PAGE_PRESENT) {
        pt_ptr = (uint32_t*)(page_directory[dir_index] & 0xFFFFF000);
    } else {
        pt_ptr = (uint32_t*)pt_alloc_ptr;
        pt_alloc_ptr += 0x1000;
        for (int i = 0; i < 1024; i++) pt_ptr[i] = 0;
        page_directory[dir_index] = (uint32_t)pt_ptr | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    }
    
    pt_ptr[table_index] = (physical & 0xFFFFF000) | flags;
    asm volatile("invlpg (%0)" : : "r"(virtual) : "memory");
}

// Unmap a page
void paging_unmap_page(uint32_t virtual) {
    uint32_t dir_index = virtual >> 22;
    uint32_t table_index = (virtual >> 12) & 0x3FF;
    
    if (page_directory[dir_index] & PAGE_PRESENT) {
        uint32_t* page_table_ptr = (uint32_t*)(page_directory[dir_index] & 0xFFFFF000);
        page_table_ptr[table_index] = 0;
        
        // Invalidate TLB
        asm volatile("invlpg (%0)" : : "r"(virtual) : "memory");
    }
}

// Get physical address from virtual
uint32_t paging_get_physical(uint32_t virtual) {
    uint32_t dir_index = virtual >> 22;
    uint32_t table_index = (virtual >> 12) & 0x3FF;
    uint32_t offset = virtual & 0xFFF;
    
    if (!(page_directory[dir_index] & PAGE_PRESENT)) {
        return 0;
    }
    
    uint32_t* page_table_ptr = (uint32_t*)(page_directory[dir_index] & 0xFFFFF000);
    
    if (!(page_table_ptr[table_index] & PAGE_PRESENT)) {
        return 0;
    }
    
    return (page_table_ptr[table_index] & 0xFFFFF000) | offset;
}

// Page fault handler
void page_fault_handler(uint32_t error_code) {
    page_fault_count++;
    
    // Get faulting address
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r"(faulting_address));
    
    // Print error info
    vga_puts("\n[PAGE FAULT] at address ");
    { char nb[12]; num_to_hex(faulting_address, nb); vga_puts(nb); }
    vga_puts("\n");
    
    vga_puts("  Error code: ");
    { char nb[8]; num_to_str(error_code, nb); vga_puts(nb); }
    vga_puts("\n");
    
    vga_puts("  Details:\n");
    if (error_code & 0x01) vga_puts("    - Page not present\n");
    if (error_code & 0x02) vga_puts("    - Write operation\n");
    if (error_code & 0x04) vga_puts("    - User mode\n");
    if (error_code & 0x08) vga_puts("    - Reserved bit set\n");
    if (error_code & 0x10) vga_puts("    - Instruction fetch\n");
    
    // Halt system
    vga_puts("\n[PAGING] System halted due to page fault\n");
    while (1) { asm volatile("hlt"); }
}

// Get page fault count
uint32_t paging_get_fault_count(void) {
    return page_fault_count;
}

// Print paging status
void paging_status(void) {
    vga_puts("\n  Paging status:\n\n");
    
    vga_puts("  Page directory: ");
    { char nb[12]; num_to_hex((uint32_t)page_directory, nb); vga_puts(nb); }
    vga_puts("\n");
    
    vga_puts("  Page table: ");
    { char nb[12]; num_to_hex((uint32_t)page_table, nb); vga_puts(nb); }
    vga_puts("\n");
    
    vga_puts("  Page faults: ");
    { char nb[8]; num_to_str(page_fault_count, nb); vga_puts(nb); }
    vga_puts("\n");
    
    // Check if paging is enabled
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    vga_puts("  Paging: ");
    if (cr0 & 0x80000000) {
        vga_set_color(0x0A, 0);
        vga_puts("ENABLED");
    } else {
        vga_set_color(0x0C, 0);
        vga_puts("DISABLED");
    }
    vga_set_color(0x07, 0);
    vga_puts("\n");
}
