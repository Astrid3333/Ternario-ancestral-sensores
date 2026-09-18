/**
 * elf_dynamic.c — Dynamic ELF Loader para Tritos OS
 *
 * Carga ELF shared libraries (.so) y resuelve símbolos dinámicamente.
 */

#include "../include/ternary.h"

// Page flags (same as paging.c)
#define PAGE_PRESENT     0x01
#define PAGE_WRITABLE    0x02
#define PAGE_USER        0x04

// ELF Dynamic types
#define PT_NULL     0
#define PT_LOAD     1
#define PT_DYNAMIC  2
#define PT_INTERP   3
#define PT_NOTE     4

// ELF Dynamic tags
#define DT_NULL     0
#define DT_NEEDED   1
#define DT_STRTAB   5
#define DT_SYMTAB   6
#define DT_STRSZ    10
#define DT_SYMENT   11
#define DT_REL      17
#define DT_RELA     23
#define DT_RELSZ    18
#define DT_RELASZ   20

// ELF symbol bindings
#define STB_LOCAL   0
#define STB_GLOBAL  1
#define STB_WEAK    2

// ELF symbol types
#define STT_NOTYPE  0
#define STT_OBJECT  1
#define STT_FUNC    2

// ELF Relocation types
#define R_386_NONE      0
#define R_386_32        1
#define R_386_PC32      2
#define R_386_GOT32     3
#define R_386_PLT32     4
#define R_386_RELATIVE  8
#define R_386_GOTOFF    9
#define R_386_GOTPC     10

// Max shared libraries
#define MAX_SHARED_LIBS 16
#define MAX_SYMBOLS     256

// ELF32 structures (packed)
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

typedef struct {
    uint32_t d_tag;
    union {
        uint32_t d_val;
        uint32_t d_ptr;
    };
} __attribute__((packed)) elf32_dyn_t;

typedef struct {
    uint32_t st_name;
    uint32_t st_value;
    uint32_t st_size;
    uint8_t  st_info;
    uint8_t  st_other;
    uint16_t st_shndx;
} __attribute__((packed)) elf32_sym_t;

typedef struct {
    uint32_t r_offset;
    uint32_t r_info;
} __attribute__((packed)) elf32_rel_t;

typedef struct {
    uint32_t r_offset;
    uint32_t r_info;
    int32_t  r_addend;
} __attribute__((packed)) elf32_rela_t;

// Shared library
typedef struct {
    uint8_t used;
    char name[32];
    uint32_t base_addr;
    uint32_t size;
    uint32_t entry;
    uint32_t dynsym;
    uint32_t dynstr;
    uint32_t rel;
    uint32_t rel_size;
    uint32_t rela;
    uint32_t rela_size;
} shared_lib_t;

static shared_lib_t shared_libs[MAX_SHARED_LIBS];
static uint8_t shared_lib_count = 0;

// Global symbol table
typedef struct {
    char name[32];
    uint32_t address;
    uint32_t size;
    uint8_t type;
    uint8_t bind;
} symbol_entry_t;

static symbol_entry_t global_symbols[MAX_SYMBOLS];
static uint8_t global_symbol_count = 0;

// =============================================================================
// Symbol Table Management
// =============================================================================

int dyn_add_symbol(const char* name, uint32_t addr, uint32_t size,
                   uint8_t type, uint8_t bind) {
    if (global_symbol_count >= MAX_SYMBOLS) return -1;

    symbol_entry_t* sym = &global_symbols[global_symbol_count];
    strncpy(sym->name, name, 31);
    sym->address = addr;
    sym->size = size;
    sym->type = type;
    sym->bind = bind;
    global_symbol_count++;

    return global_symbol_count - 1;
}

uint32_t dyn_find_symbol(const char* name) {
    for (int i = 0; i < global_symbol_count; i++) {
        if (strcmp_t(global_symbols[i].name, name) == 0) {
            return global_symbols[i].address;
        }
    }
    return 0; // Not found
}

void dyn_list_symbols(void) {
    vga_puts("\n  [Dynamic Symbols]\n\n");
    for (int i = 0; i < global_symbol_count; i++) {
        vga_puts("    ");
        vga_puts(global_symbols[i].name);
        vga_puts(" @ 0x");
        { char nb[8]; num_to_hex(global_symbols[i].address, nb); vga_puts(nb); }
        vga_puts(" (");
        { char nb[8]; num_to_str(global_symbols[i].size, nb); vga_puts(nb); }
        vga_puts(" bytes)\n");
    }
    vga_puts("\n");
}

// =============================================================================
// Dynamic ELF Loader
// =============================================================================

int elf_load_dynamic(const uint8_t* data, uint32_t size, uint32_t* entry_point) {
    // Validate ELF header
    if (size < 52) return -1;
    if (data[0] != 0x7F || data[1] != 'E' || data[2] != 'L' || data[3] != 'F') {
        return -1;
    }
    if (data[4] != 1) return -1; // 32-bit
    if (data[5] != 1) return -1; // Little endian

    // Get program headers
    uint32_t phoff = *(uint32_t*)(data + 28);
    uint16_t phnum = *(uint16_t*)(data + 44);
    uint16_t phentsize = *(uint16_t*)(data + 42);

    if (phoff + phnum * phentsize > size) return -1;

    // Find PT_DYNAMIC segment
    uint32_t dynamic_offset = 0;
    uint32_t dynamic_size = 0;

    for (int i = 0; i < phnum; i++) {
        elf32_phdr_t* phdr = (elf32_phdr_t*)(data + phoff + i * phentsize);

        if (phdr->p_type == PT_DYNAMIC) {
            dynamic_offset = phdr->p_offset;
            dynamic_size = phdr->p_memsz;
            break;
        }
    }

    if (dynamic_offset == 0) return -1;

    // Parse dynamic section
    uint32_t dynsym = 0, dynstr = 0, strsz = 0, syment = 0;
    uint32_t rel = 0, rel_size = 0, rela = 0, rela_size = 0;

    elf32_dyn_t* dyn = (elf32_dyn_t*)(data + dynamic_offset);
    while ((uint8_t*)dyn < data + dynamic_offset + dynamic_size) {
        if (dyn->d_tag == DT_NULL) break;

        switch (dyn->d_tag) {
            case DT_STRTAB: dynstr = dyn->d_ptr; break;
            case DT_SYMTAB: dynsym = dyn->d_ptr; break;
            case DT_STRSZ: strsz = dyn->d_val; break;
            case DT_SYMENT: syment = dyn->d_val; break;
            case DT_REL: rel = dyn->d_ptr; break;
            case DT_RELSZ: rel_size = dyn->d_val; break;
            case DT_RELA: rela = dyn->d_ptr; break;
            case DT_RELASZ: rela_size = dyn->d_val; break;
        }
        dyn++;
    }

    // Load PT_LOAD segments
    for (int i = 0; i < phnum; i++) {
        elf32_phdr_t* phdr = (elf32_phdr_t*)(data + phoff + i * phentsize);

        if (phdr->p_type == PT_LOAD) {
            // Map pages for this segment
            uint32_t vaddr = phdr->p_vaddr;
            uint32_t memsz = phdr->p_memsz;

            for (uint32_t off = 0; off < memsz; off += 4096) {
                uint32_t page_vaddr = vaddr + off;
                // Use mem_alloc to get a free memory block
                int16_t page_idx = mem_alloc(0, 0x02);
                if (page_idx < 0) return -1;
                // Convert block index to address (simple mapping)
                uint32_t page_paddr = 0x200000 + (uint32_t)page_idx * 4096;
                paging_map_page(page_vaddr, page_paddr, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
            }

            // Copy segment data
            if (phdr->p_filesz > 0 && phdr->p_offset + phdr->p_filesz <= size) {
                memcpy((void*)vaddr, data + phdr->p_offset, phdr->p_filesz);
            }

            // Zero BSS
            if (phdr->p_memsz > phdr->p_filesz) {
                memset((void*)(vaddr + phdr->p_filesz), 0, phdr->p_memsz - phdr->p_filesz);
            }
        }
    }

    // Parse symbols and add to global table
    if (dynsym && dynstr && syment) {
        elf32_sym_t* sym = (elf32_sym_t*)(data + dynsym);
        int num_syms = (rel_size + rela_size) / syment; // Approximate

        for (int i = 0; i < num_syms && i < 100; i++) {
            if (sym[i].st_name == 0) continue;

            const char* name = (const char*)(data + dynstr + sym[i].st_name);
            uint8_t bind = sym[i].st_info >> 4;
            uint8_t type = sym[i].st_info & 0x0F;

            if (bind == STB_GLOBAL || bind == STB_WEAK) {
                dyn_add_symbol(name, sym[i].st_value, sym[i].st_size, type, bind);
            }
        }
    }

    // Apply relocations
    if (rel) {
        elf32_rel_t* rel_entry = (elf32_rel_t*)(data + rel);
        int num_rel = rel_size / sizeof(elf32_rel_t);

        for (int i = 0; i < num_rel; i++) {
            uint32_t offset = rel_entry[i].r_offset;
            uint32_t info = rel_entry[i].r_info;
            uint8_t type = info & 0xFF;
            uint32_t sym_idx = info >> 8;

            switch (type) {
                case R_386_RELATIVE: {
                    uint32_t* loc = (uint32_t*)offset;
                    *loc += 0; // Base address (already loaded)
                    break;
                }
                case R_386_32: {
                    // Symbol + addend
                    if (sym_idx && dynsym) {
                        elf32_sym_t* sym = (elf32_sym_t*)(data + dynsym + sym_idx * syment);
                        const char* name = (const char*)(data + dynstr + sym->st_name);
                        uint32_t addr = dyn_find_symbol(name);
                        if (addr) {
                            uint32_t* loc = (uint32_t*)offset;
                            *loc = addr;
                        }
                    }
                    break;
                }
                case R_386_PC32: {
                    // PC-relative
                    break;
                }
            }
        }
    }

    *entry_point = *(uint32_t*)(data + 24); // e_entry

    return 0;
}

// =============================================================================
// Shared Library Management
// =============================================================================

int dyn_load_library(const char* name, const uint8_t* data, uint32_t size) {
    if (shared_lib_count >= MAX_SHARED_LIBS) return -1;

    shared_lib_t* lib = &shared_libs[shared_lib_count];
    lib->used = 1;
    strncpy(lib->name, name, 31);

    uint32_t entry;
    if (elf_load_dynamic(data, size, &entry) < 0) {
        lib->used = 0;
        return -1;
    }

    lib->entry = entry;
    shared_lib_count++;

    vga_puts("[DYN] Loaded library: ");
    vga_puts(name);
    vga_puts("\n");

    return shared_lib_count - 1;
}

void dyn_unload_library(const char* name) {
    for (int i = 0; i < shared_lib_count; i++) {
        if (shared_libs[i].used && strcmp_t(shared_libs[i].name, name) == 0) {
            // Unmap pages
            // (simplified - actual implementation needs page tracking)
            shared_libs[i].used = 0;

            // Remove symbols
            for (int j = 0; j < global_symbol_count; j++) {
                if (global_symbols[j].address >= shared_libs[i].base_addr &&
                    global_symbols[j].address < shared_libs[i].base_addr + shared_libs[i].size) {
                    // Remove by shifting
                    for (int k = j; k < global_symbol_count - 1; k++) {
                        global_symbols[k] = global_symbols[k + 1];
                    }
                    global_symbol_count--;
                    j--;
                }
            }

            vga_puts("[DYN] Unloaded library: ");
            vga_puts(name);
            vga_puts("\n");
            return;
        }
    }
}

void dyn_list_libraries(void) {
    vga_puts("\n  [Shared Libraries]\n\n");
    for (int i = 0; i < shared_lib_count; i++) {
        if (shared_libs[i].used) {
            vga_puts("    [OK] ");
            vga_puts(shared_libs[i].name);
            vga_puts("\n");
        }
    }
    vga_puts("\n");
}
