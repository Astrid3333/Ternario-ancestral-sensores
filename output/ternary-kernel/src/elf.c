/**
 * elf.c — Cargador ELF para kernel ternario ancestral
 *
 * Carga ejecutables ELF 32-bit en memoria
 */

#include "../include/ternary.h"

// ELF header structures
typedef struct {
    uint8_t  e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) elf32_header_t;

// ELF program header
typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} __attribute__((packed)) elf32_phdr_t;

// ELF section header
typedef struct {
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;
} __attribute__((packed)) elf32_shdr_t;

// ELF constants
#define ELF_MAGIC       "\x7fELF"
#define ELF_CLASS32     1
#define ELF_DATA2LSB    1
#define ELF_TYPE_EXEC   2
#define ELF_MACHINE_386 3
#define ELF_PH_LOAD     1

// ELF flags
#define PF_X            0x1
#define PF_W            0x2
#define PF_R            0x4

// Check if data is ELF
static uint8_t is_elf(const uint8_t* data) {
    if (data[0] != 0x7F || data[1] != 'E' || data[2] != 'L' || data[3] != 'F') {
        return 0;
    }
    return 1;
}

// Load ELF executable
int8_t elf_load(const uint8_t* data, uint32_t size, uint32_t* entry_point) {
    // Validate ELF header
    if (size < sizeof(elf32_header_t)) {
        vga_puts("[ELF] File too small\n");
        return -1;
    }
    
    const elf32_header_t* header = (const elf32_header_t*)data;
    
    // Check magic
    if (!is_elf(header->e_ident)) {
        vga_puts("[ELF] Invalid magic number\n");
        return -1;
    }
    
    // Check class (32-bit)
    if (header->e_ident[4] != ELF_CLASS32) {
        vga_puts("[ELF] Not a 32-bit ELF\n");
        return -1;
    }
    
    // Check endianness (little endian)
    if (header->e_ident[5] != ELF_DATA2LSB) {
        vga_puts("[ELF] Not little endian\n");
        return -1;
    }
    
    // Check type (executable)
    if (header->e_type != ELF_TYPE_EXEC) {
        vga_puts("[ELF] Not an executable\n");
        return -1;
    }
    
    // Check machine (i386)
    if (header->e_machine != ELF_MACHINE_386) {
        vga_puts("[ELF] Not for i386\n");
        return -1;
    }
    
    vga_puts("[ELF] Valid ELF32 executable\n");
    vga_puts("[ELF] Entry point: ");
    { char nb[12]; num_to_hex(header->e_entry, nb); vga_puts(nb); }
    vga_puts("\n");
    
    // Load program headers
    vga_puts("[ELF] Loading ");
    { char nb[4]; num_to_str(header->e_phnum, nb); vga_puts(nb); }
    vga_puts(" program headers...\n");
    
    for (int i = 0; i < header->e_phnum; i++) {
        uint32_t ph_offset = header->e_phoff + i * header->e_phentsize;
        
        if (ph_offset + sizeof(elf32_phdr_t) > size) {
            vga_puts("[ELF] Invalid program header offset\n");
            return -1;
        }
        
        const elf32_phdr_t* phdr = (const elf32_phdr_t*)(data + ph_offset);
        
        // Only load PT_LOAD segments
        if (phdr->p_type != ELF_PH_LOAD) {
            continue;
        }
        
        vga_puts("[ELF] Segment ");
        { char nb[2]; num_to_str(i, nb); vga_puts(nb); }
        vga_puts(": vaddr=");
        { char nb[12]; num_to_hex(phdr->p_vaddr, nb); vga_puts(nb); }
        vga_puts(" filesz=");
        { char nb[8]; num_to_str(phdr->p_filesz, nb); vga_puts(nb); }
        vga_puts(" memsz=");
        { char nb[8]; num_to_str(phdr->p_memsz, nb); vga_puts(nb); }
        vga_puts("\n");
        
        // Copy segment data
        if (phdr->p_filesz > 0) {
            if (phdr->p_offset + phdr->p_filesz > size) {
                vga_puts("[ELF] Segment data beyond file\n");
                return -1;
            }
            
            // Map pages for this segment
            for (uint32_t addr = phdr->p_vaddr; addr < phdr->p_vaddr + phdr->p_memsz; addr += 0x1000) {
            uint32_t flags = 0x03;
            if (phdr->p_flags & PF_W) {
                flags |= 0x02;
            }
                paging_map_page(addr, addr, flags);
            }
            
            // Copy data
            memcpy_t((void*)phdr->p_vaddr, data + phdr->p_offset, phdr->p_filesz);
        }
        
        // Zero BSS
        if (phdr->p_memsz > phdr->p_filesz) {
            uint32_t bss_start = phdr->p_vaddr + phdr->p_filesz;
            uint32_t bss_size = phdr->p_memsz - phdr->p_filesz;
            memset_t((void*)bss_start, 0, bss_size);
        }
    }
    
    *entry_point = header->e_entry;
    
    vga_puts("[ELF] Loaded successfully\n");
    vga_puts("[ELF] Entry point: ");
    { char nb[12]; num_to_hex(header->e_entry, nb); vga_puts(nb); }
    vga_puts("\n");
    
    return 0;
}

// Execute ELF program
int8_t elf_execute(const uint8_t* data, uint32_t size) {
    uint32_t entry_point;
    
    if (elf_load(data, size, &entry_point) < 0) {
        return -1;
    }
    
    // Create new process
    int8_t pid = sched_create(sched_get_current(), TRIT_POS);
    if (pid < 0) {
        vga_puts("[ELF] Failed to create process\n");
        return -1;
    }
    
    // Get process info
    process_t* proc = 0;
    sched_get_info(pid, proc);
    
    // Allocate user stack (Ring 3, 64KB)
    uint32_t user_stack_base = 0x80000000;
    uint32_t user_stack_size = 0x10000; // 64KB
    
    // Map user stack pages
    for (uint32_t addr = user_stack_base; addr < user_stack_base + user_stack_size; addr += 0x1000) {
        paging_map_page(addr, addr, 0x07); // User R/W
    }
    
    // Clear user stack
    memset_t((void*)user_stack_base, 0, user_stack_size);
    
    // Set up process registers
    // Stack grows down, so start at top
    uint32_t user_esp = user_stack_base + user_stack_size - 4;
    
    vga_puts("[ELF] Process ");
    { char nb[4]; num_to_str(pid, nb); vga_puts(nb); }
    vga_puts(" starting at ");
    { char nb[12]; num_to_hex(entry_point, nb); vga_puts(nb); }
    vga_puts("\n");
    
    // Jump to user mode and execute
    // This is a simplified version - real implementation would use IRET
    asm volatile(
        "mov $0x23, %%ax\n"     // User data segment (0x23)
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "pushl $0x23\n"         // User SS
        "pushl %0\n"            // User ESP
        "pushf\n"               // EFLAGS
        "pushl $0x1B\n"         // User CS (0x1B)
        "pushl %1\n"            // EIP
        "iret\n"
        : : "r"(user_esp), "r"(entry_point) : "ax"
    );
    
    // Should not reach here
    return 0;
}
