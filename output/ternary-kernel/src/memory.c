/**
 * memory.c — Gestor de memoria en Base 60 (Babilónico)
 * 
 * Memoria organizada en 60 bloques de 60 bytes cada uno
 * Total: 3,600 bytes (3.5KB)
 * 
 * Cada bloque tiene un "color" (tipo) como los quipus:
 * - 0: libre
 * - 1: kernel
 * - 2: datos de proceso
 * - 3: stack
 * - 4: código
 */

#include "../include/ternary.h"

// =============================================================================
// ESTRUCTURAS
// =============================================================================

// Bloque de memoria
typedef struct {
    uint8_t data[BLOCK_SIZE];    // Datos del bloque
    uint8_t color;               // Tipo de bloque (0-7)
    uint8_t owner;               // ID del proceso dueño
    uint8_t flags;               // Bits: [used][dirty][locked][...]
} __attribute__((packed)) mem_block_t;

// Mapa de memoria (quipu simplificado)
typedef struct {
    uint8_t knot_positions[MEM_BLOCKS];  // Posición de cada "nudo"
    uint8_t knot_colors[MEM_BLOCKS];     // Color de cada "nudo"
    uint8_t total_used;                  // Bloques en uso
    uint8_t total_free;                  // Bloques libres
} __attribute__((packed)) memory_map_t;

// =============================================================================
// VARIABLES GLOBALES
// =============================================================================

static mem_block_t memory[MEM_BLOCKS];
static memory_map_t map;

// =============================================================================
// FUNCIONES
// =============================================================================

// Inicializar memoria
void mem_init() {
    // Limpiar todos los bloques
    for (uint8_t i = 0; i < MEM_BLOCKS; i++) {
        for (uint8_t j = 0; j < BLOCK_SIZE; j++) {
            memory[i].data[j] = 0;
        }
        memory[i].color = 0;   // libre
        memory[i].owner = 0;   // sin dueño
        memory[i].flags = 0;
        
        // Inicializar mapa quipu
        map.knot_positions[i] = i;
        map.knot_colors[i] = 0;
    }
    
    // Marcar bloques del kernel
    for (uint8_t i = 0; i < 5; i++) {
        memory[i].color = 1;    // kernel
        memory[i].owner = 0;    // kernel = PID 0
        memory[i].flags = 0x08; // locked
        map.knot_colors[i] = 1;
    }
    
    map.total_used = 5;
    map.total_free = MEM_BLOCKS - 5;
}

// Asignar bloque (con color)
int8_t mem_alloc(uint8_t owner, uint8_t color) {
    // Buscar primer bloque libre (residuo mod-33 para balance)
    uint8_t start = (map.total_used * 7) % MEM_BLOCKS;  // Hash simple
    
    for (uint8_t i = 0; i < MEM_BLOCKS; i++) {
        uint8_t idx = (start + i) % MEM_BLOCKS;
        
        if (memory[idx].color == 0) {  // libre
            memory[idx].color = color;
            memory[idx].owner = owner;
            memory[idx].flags = 0x01;  // used
            
            map.knot_colors[idx] = color;
            map.total_used++;
            map.total_free--;
            
            return idx;  // Retorna índice del bloque
        }
    }
    
    return -1;  // Sin memoria
}

// Liberar bloque
int8_t mem_free(uint8_t block_idx) {
    if (block_idx >= MEM_BLOCKS) return -1;
    if (memory[block_idx].flags & 0x08) return -1;  // locked
    
    // Limpiar bloque
    for (uint8_t j = 0; j < BLOCK_SIZE; j++) {
        memory[block_idx].data[j] = 0;
    }
    
    memory[block_idx].color = 0;
    memory[block_idx].owner = 0;
    memory[block_idx].flags = 0;
    
    map.knot_colors[block_idx] = 0;
    map.total_used--;
    map.total_free++;
    
    return 0;
}

// Liberar todos los bloques de un proceso
void mem_free_all(uint8_t owner) {
    for (uint8_t i = 0; i < MEM_BLOCKS; i++) {
        if (memory[i].owner == owner && !(memory[i].flags & 0x08)) {
            mem_free(i);
        }
    }
}

// Leer byte de memoria (dirección babilónica)
int8_t mem_read(babilonian_addr_t addr, uint8_t* value) {
    uint16_t linear = babilonian_to_linear(addr);
    uint8_t block = linear / BLOCK_SIZE;
    uint8_t offset = linear % BLOCK_SIZE;
    
    if (block >= MEM_BLOCKS) return -1;
    if (memory[block].color == 0) return -1;  // libre
    
    *value = memory[block].data[offset];
    return 0;
}

// Escribir byte en memoria (dirección babilónica)
int8_t mem_write(babilonian_addr_t addr, uint8_t value) {
    uint16_t linear = babilonian_to_linear(addr);
    uint8_t block = linear / BLOCK_SIZE;
    uint8_t offset = linear % BLOCK_SIZE;
    
    if (block >= MEM_BLOCKS) return -1;
    if (memory[block].flags & 0x08) return -1;  // locked
    
    memory[block].data[offset] = value;
    memory[block].flags |= 0x02;  // dirty
    return 0;
}

// Obtener dirección base de un bloque
babilonian_addr_t mem_get_base(uint8_t block_idx) {
    uint16_t linear = block_idx * BLOCK_SIZE;
    return linear_to_babilonian(linear);
}

// Obtener estado de la memoria (para debug)
void mem_get_status(uint8_t* used, uint8_t* free, uint8_t* locked) {
    *used = map.total_used;
    *free = map.total_free;
    
    uint8_t lock_count = 0;
    for (uint8_t i = 0; i < MEM_BLOCKS; i++) {
        if (memory[i].flags & 0x08) lock_count++;
    }
    *locked = lock_count;
}

// Imprimir mapa de memoria (debug)
void mem_print_map() {
    // Esta función sería implementada con I/O de video
    // Por ahora, solo retorna información
    uint8_t used, free, locked;
    mem_get_status(&used, &free, &locked);
    
    // En un sistema real, imprimiría:
    // "Memory: X used, Y free, Z locked"
    // Con colores显示 los bloques usados
}
