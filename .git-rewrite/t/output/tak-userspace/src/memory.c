/**
 * memory.c — Ternary memory manager (Base 60)
 *
 * Wraps malloc with ternary tracking.
 * 60 "blocks" tracked, each can hold data up to TAK_BLOCK_SIZE.
 * Actual allocation uses malloc underneath; we just track metadata.
 */

#include "ternary.h"

static mem_block_t blocks[TAK_MAX_MEM];
static uint32_t total_allocated = 0;
static uint32_t total_freed = 0;

void mem_init(void) {
    memset(blocks, 0, sizeof(blocks));
    total_allocated = 0;
    total_freed = 0;

    /* Mark first 5 as kernel */
    for (int i = 0; i < 5; i++) {
        blocks[i].color = 1;
        blocks[i].owner = 0;
        blocks[i].flags = 1;
        blocks[i].label[0] = 'K';
    }
}

int mem_alloc(int owner, uint8_t color, uint32_t size, const char* label) {
    /* Find free slot */
    for (int i = 0; i < TAK_MAX_MEM; i++) {
        if (!blocks[i].flags) {
            blocks[i].ptr = malloc(size);
            if (!blocks[i].ptr) return -1;
            blocks[i].size = size;
            blocks[i].owner = owner;
            blocks[i].color = color;
            blocks[i].flags = 1;
            if (label) strncpy(blocks[i].label, label, 15);
            total_allocated += size;
            return i;
        }
    }
    return -1;
}

int mem_free(int block) {
    if (block < 0 || block >= TAK_MAX_MEM) return -1;
    if (!blocks[block].flags) return -1;

    free(blocks[block].ptr);
    total_freed += blocks[block].size;
    memset(&blocks[block], 0, sizeof(mem_block_t));
    return 0;
}

void mem_free_owner(int owner) {
    for (int i = 0; i < TAK_MAX_MEM; i++) {
        if (blocks[i].flags && blocks[i].owner == owner) {
            mem_free(i);
        }
    }
}

void* mem_get_ptr(int block) {
    if (block < 0 || block >= TAK_MAX_MEM) return NULL;
    return blocks[block].ptr;
}

uint32_t mem_get_size(int block) {
    if (block < 0 || block >= TAK_MAX_MEM) return 0;
    return blocks[block].size;
}

int mem_get_active_count(void) {
    int c = 0;
    for (int i = 0; i < TAK_MAX_MEM; i++) {
        if (blocks[i].flags) c++;
    }
    return c;
}

uint32_t mem_get_total_allocated(void) {
    return total_allocated;
}

mem_block_t* mem_get_block(int i) {
    if (i < 0 || i >= TAK_MAX_MEM) return NULL;
    return &blocks[i];
}

const char* mem_color_name(uint8_t color) {
    switch (color) {
        case 0: return "free";
        case 1: return "kernel";
        case 2: return "process";
        case 3: return "data";
        case 4: return "code";
        case 5: return "stack";
        default: return "unknown";
    }
}

const char* mem_color_ansi(uint8_t color) {
    switch (color) {
        case 0: return COLOR_GRAY;
        case 1: return COLOR_RED;
        case 2: return COLOR_GREEN;
        case 3: return COLOR_BLUE;
        case 4: return COLOR_YELLOW;
        case 5: return COLOR_MAGENTA;
        default: return COLOR_WHITE;
    }
}
