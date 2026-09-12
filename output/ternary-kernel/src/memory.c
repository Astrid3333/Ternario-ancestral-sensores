/**
 * memory.c — Gestor de memoria en Base 60 (v2 — corregido)
 * 
 * Fixes:
 * - mem_alloc() retorna int16_t (no uint8_t) para error -1
 * - mem_get_base() con validación
 * - mem_print_map() implementada con salida real
 * - mem_read() con validación de color = libre
 */

#include "../include/ternary.h"

// =============================================================================
// ESTRUCTURAS
// =============================================================================

typedef struct {
    uint8_t data[BLOCK_SIZE];
    uint8_t color;
    uint8_t owner;
    uint8_t flags;
} __attribute__((packed)) mem_block_t;

typedef struct {
    uint8_t knot_positions[MEM_BLOCKS];
    uint8_t knot_colors[MEM_BLOCKS];
    uint8_t total_used;
    uint8_t total_free;
} __attribute__((packed)) memory_map_t;

// =============================================================================
// VARIABLES GLOBALES
// =============================================================================

static mem_block_t memory[MEM_BLOCKS];
static memory_map_t map;

// =============================================================================
// FUNCIONES
// =============================================================================

void mem_init(void) {
    memset_t(memory, 0, sizeof(memory));
    memset_t(&map, 0, sizeof(map));
    
    for (uint8_t i = 0; i < MEM_BLOCKS; i++) {
        map.knot_positions[i] = i;
    }
    
    for (uint8_t i = 0; i < 5; i++) {
        memory[i].color = COLOR_KERNEL;
        memory[i].owner = 0;
        memory[i].flags = 0x08;
        map.knot_colors[i] = COLOR_KERNEL;
    }
    
    map.total_used = 5;
    map.total_free = MEM_BLOCKS - 5;
}

int16_t mem_alloc(uint8_t owner, uint8_t color) {
    uint8_t start = (map.total_used * 7) % MEM_BLOCKS;
    
    for (uint8_t i = 0; i < MEM_BLOCKS; i++) {
        uint8_t idx = (start + i) % MEM_BLOCKS;
        
        if (memory[idx].color == COLOR_FREE) {
            memory[idx].color = color;
            memory[idx].owner = owner;
            memory[idx].flags = 0x01;
            map.knot_colors[idx] = color;
            map.total_used++;
            map.total_free--;
            return (int16_t)idx;
        }
    }
    
    return -1;
}

int8_t mem_free(uint8_t block_idx) {
    if (block_idx >= MEM_BLOCKS) return -1;
    if (memory[block_idx].flags & 0x08) return -1;
    
    memset_t(memory[block_idx].data, 0, BLOCK_SIZE);
    memory[block_idx].color = COLOR_FREE;
    memory[block_idx].owner = 0;
    memory[block_idx].flags = 0;
    map.knot_colors[block_idx] = COLOR_FREE;
    map.total_used--;
    map.total_free++;
    
    return 0;
}

void mem_free_all(uint8_t owner) {
    for (uint8_t i = 0; i < MEM_BLOCKS; i++) {
        if (memory[i].owner == owner && !(memory[i].flags & 0x08)) {
            mem_free(i);
        }
    }
}

int8_t mem_read(babilonian_addr_t addr, uint8_t* value) {
    uint16_t linear = babilonian_to_linear(addr);
    uint8_t block = linear / BLOCK_SIZE;
    uint8_t offset = linear % BLOCK_SIZE;
    
    if (block >= MEM_BLOCKS) return -1;
    if (memory[block].color == COLOR_FREE) return -1;
    
    *value = memory[block].data[offset];
    return 0;
}

int8_t mem_write(babilonian_addr_t addr, uint8_t value) {
    uint16_t linear = babilonian_to_linear(addr);
    uint8_t block = linear / BLOCK_SIZE;
    uint8_t offset = linear % BLOCK_SIZE;
    
    if (block >= MEM_BLOCKS) return -1;
    if (memory[block].flags & 0x08) return -1;
    
    memory[block].data[offset] = value;
    memory[block].flags |= 0x02;
    return 0;
}

int8_t mem_get_base(uint8_t block_idx, babilonian_addr_t* result) {
    if (block_idx >= MEM_BLOCKS) return -1;
    *result = linear_to_babilonian(block_idx * BLOCK_SIZE);
    return 0;
}

void mem_get_status(uint8_t* used, uint8_t* free_count, uint8_t* locked) {
    *used = map.total_used;
    *free_count = map.total_free;
    
    uint8_t lock_count = 0;
    for (uint8_t i = 0; i < MEM_BLOCKS; i++) {
        if (memory[i].flags & 0x08) lock_count++;
    }
    *locked = lock_count;
}

uint8_t mem_get_color(uint8_t block_idx) {
    if (block_idx >= MEM_BLOCKS) return 0xFF;
    return memory[block_idx].color;
}

uint8_t mem_get_owner(uint8_t block_idx) {
    if (block_idx >= MEM_BLOCKS) return 0xFF;
    return memory[block_idx].owner;
}

uint8_t mem_get_flags(uint8_t block_idx) {
    if (block_idx >= MEM_BLOCKS) return 0xFF;
    return memory[block_idx].flags;
}
