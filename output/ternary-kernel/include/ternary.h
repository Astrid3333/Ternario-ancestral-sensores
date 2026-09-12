/**
 * ternary.h — Definiciones core del kernel ternario ancestral (v2)
 * 
 * Corregido:
 * - Agregados outb/inb como inline assembly
 * - Fix return types para(mem_alloc, mem_get_base)
 * - Agregadas validaciones
 * - Agregadas funciones de libc ternaria
 */

#ifndef TERNARY_H
#define TERNARY_H

#include <stdint.h>

// =============================================================================
// I/O PORTS — Inline assembly para x86
// =============================================================================

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

// =============================================================================
// TIPOS TERNARIOS
// =============================================================================

typedef int8_t trit_t;
typedef uint8_t trit_packed_t;

typedef struct {
    uint8_t high;
    uint8_t low;
} __attribute__((packed)) babilonian_addr_t;

typedef struct {
    uint16_t tzolkin;
    uint8_t haab;
} __attribute__((packed)) maya_pid_t;

// =============================================================================
// CONSTANTES
// =============================================================================

#define TRIT_NEG    (-1)
#define TRIT_ZERO   (0)
#define TRIT_POS    (1)

#define BASE_60     60
#define TZOLKIN     260
#define HAAB        365
#define MAX_PROCS   33
#define MEM_BLOCKS  60
#define BLOCK_SIZE  60

#define COLOR_FREE      0
#define COLOR_KERNEL    1
#define COLOR_PROCESS   2
#define COLOR_STACK     3
#define COLOR_CODE      4
#define COLOR_DATA      5

// =============================================================================
// OPERACIONES TERNARIAS
// =============================================================================

static inline trit_t trit_add(trit_t a, trit_t b) {
    trit_t sum = a + b;
    if (sum > 1) return 1;
    if (sum < -1) return -1;
    return sum;
}

static inline trit_t trit_mul(trit_t a, trit_t b) {
    if (a == 0 || b == 0) return 0;
    return (a == b) ? 1 : -1;
}

static inline trit_t trit_neg(trit_t a) {
    return -a;
}

static inline trit_t trit_cmp(trit_t a, trit_t b) {
    if (a > b) return 1;
    if (a < b) return -1;
    return 0;
}

// =============================================================================
// EMPAQUETADO DE TRITS
// =============================================================================

static inline trit_packed_t pack_trits(trit_t t1, trit_t t2, trit_t t3) {
    uint8_t v1 = (uint8_t)(t1 + 1);
    uint8_t v2 = (uint8_t)(t2 + 1);
    uint8_t v3 = (uint8_t)(t3 + 1);
    return v1 * 9 + v2 * 3 + v3;
}

static inline void unpack_trits(trit_packed_t packed, trit_t* t1, trit_t* t2, trit_t* t3) {
    *t1 = (trit_t)((packed / 9) % 3) - 1;
    *t2 = (trit_t)((packed / 3) % 3) - 1;
    *t3 = (trit_t)(packed % 3) - 1;
}

// =============================================================================
// DIRECCIONES BABILÓNICAS
// =============================================================================

static inline babilonian_addr_t linear_to_babilonian(uint16_t addr) {
    babilonian_addr_t result;
    result.high = addr / BASE_60;
    result.low = addr % BASE_60;
    return result;
}

static inline uint16_t babilonian_to_linear(babilonian_addr_t addr) {
    return addr.high * BASE_60 + addr.low;
}

// =============================================================================
// IDs MAYAS
// =============================================================================

static inline maya_pid_t make_maya_pid(uint32_t counter) {
    maya_pid_t pid;
    pid.tzolkin = (counter % TZOLKIN) + 1;
    pid.haab = (counter % HAAB) + 1;
    return pid;
}

static inline trit_t compare_maya_pid(maya_pid_t a, maya_pid_t b) {
    if (a.tzolkin != b.tzolkin) {
        return (a.tzolkin > b.tzolkin) ? 1 : -1;
    }
    if (a.haab != b.haab) {
        return (a.haab > b.haab) ? 1 : -1;
    }
    return 0;
}

// =============================================================================
// LIBC TERNARIA MÍNIMA
// =============================================================================

static inline uint16_t strlen_t(const char* str) {
    uint16_t len = 0;
    while (str[len]) len++;
    return len;
}

static inline int8_t strcmp_t(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return (*a > *b) ? 1 : -1;
        a++;
        b++;
    }
    if (*a) return 1;
    if (*b) return -1;
    return 0;
}

static inline void strcpy_t(char* dst, const char* src) {
    while (*src) {
        *dst++ = *src++;
    }
    *dst = 0;
}

static inline void memset_t(void* ptr, uint8_t val, uint16_t size) {
    uint8_t* p = (uint8_t*)ptr;
    for (uint16_t i = 0; i < size; i++) {
        p[i] = val;
    }
}

static inline void memcpy_t(void* dst, const void* src, uint16_t size) {
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;
    for (uint16_t i = 0; i < size; i++) {
        d[i] = s[i];
    }
}

static inline int8_t strncmp_t(const char* a, const char* b, uint8_t n) {
    for (uint8_t i = 0; i < n; i++) {
        if (a[i] != b[i]) return (a[i] > b[i]) ? 1 : -1;
        if (a[i] == 0) return 0;
    }
    return 0;
}

static inline void strcat_t(char* dst, const char* src) {
    while (*dst) dst++;
    while (*src) *dst++ = *src++;
    *dst = 0;
}

static inline int8_t is_digit(char c) {
    return c >= '0' && c <= '9';
}

static inline int16_t parse_num(const char* str) {
    int16_t num = 0;
    int8_t neg = 0;
    if (*str == '-') { neg = 1; str++; }
    while (*str >= '0' && *str <= '9') {
        num = num * 10 + (*str - '0');
        str++;
    }
    return neg ? -num : num;
}

#endif // TERNARY_H
