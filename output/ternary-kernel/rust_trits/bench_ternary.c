/**
 * bench_ternary.c — Benchmark ternario C vs Rust
 *
 * Compilar: gcc -O3 -march=native -o bench_c bench_ternary.c
 * Ejecutar: ./bench_c
 */

#include <stdio.h>
#include <stdint.h>
#include <time.h>

// =============================================================================
// TRIT TYPES
// =============================================================================

typedef int8_t trit;  // -1, 0, +1

// Add two trits
trit trit_add(trit a, trit b) {
    int sum = (int)a + (int)b;
    if (sum == -2) return 1;
    if (sum == 2) return -1;
    return (trit)sum;
}

// Packed trits (5 per byte)
typedef struct {
    uint8_t packed;
} trits5_t;

trit trits5_get(trits5_t t, int i) {
    return ((t.packed / (1 << (i * 2))) % 3) - 1;
}

void trits5_set(trits5_t* t, int i, trit value) {
    int current = trits5_get(*t, i);
    int diff = (int)value - current;
    t->packed += diff * (1 << (i * 2));
}

// =============================================================================
// TERNARY NUMBER (variable length)
// =============================================================================

#define MAX_TRITS 32

typedef struct {
    int8_t trits[MAX_TRITS];
    int len;
} ternary_num_t;

ternary_num_t ternary_from_int(int32_t v) {
    ternary_num_t num;
    num.len = 0;
    
    if (v == 0) {
        num.trits[0] = 0;
        num.len = 1;
        return num;
    }
    
    while (v != 0 && num.len < MAX_TRITS) {
        int rem = ((v % 3) + 3) % 3;
        v = (v - rem) / 3;
        num.trits[num.len++] = (int8_t)(rem - 1);
    }
    
    return num;
}

int32_t ternary_to_int(ternary_num_t* num) {
    int32_t result = 0;
    for (int i = 0; i < num->len; i++) {
        result += (int32_t)num->trits[i] * (1 << (i * 2));
    }
    return result;
}

ternary_num_t ternary_add(ternary_num_t* a, ternary_num_t* b) {
    ternary_num_t result;
    int max_len = a->len > b->len ? a->len : b->len;
    int8_t carry = 0;
    
    for (int i = 0; i < max_len; i++) {
        int8_t va = i < a->len ? a->trits[i] : 0;
        int8_t vb = i < b->len ? b->trits[i] : 0;
        int sum = va + vb + carry;
        
        if (sum == -3) { result.trits[i] = 0; carry = -1; }
        else if (sum == -2) { result.trits[i] = 1; carry = -1; }
        else if (sum == 3) { result.trits[i] = 0; carry = 1; }
        else if (sum == 2) { result.trits[i] = -1; carry = 1; }
        else { result.trits[i] = (int8_t)sum; carry = 0; }
    }
    
    if (carry != 0 && max_len < MAX_TRITS) {
        result.trits[max_len] = carry;
        result.len = max_len + 1;
    } else {
        result.len = max_len;
    }
    
    return result;
}

ternary_num_t ternary_mul(ternary_num_t* a, ternary_num_t* b) {
    ternary_num_t result;
    int max_len = a->len + b->len;
    if (max_len > MAX_TRITS) max_len = MAX_TRITS;
    
    for (int i = 0; i < max_len; i++) result.trits[i] = 0;
    
    for (int i = 0; i < a->len && i < MAX_TRITS; i++) {
        if (a->trits[i] == 0) continue;
        
        int8_t carry = 0;
        for (int j = 0; j < b->len && (i + j) < MAX_TRITS; j++) {
            int prod = a->trits[i] * b->trits[j] + result.trits[i + j] + carry;
            
            if (prod == -3) { result.trits[i + j] = 0; carry = -1; }
            else if (prod == -2) { result.trits[i + j] = 1; carry = -1; }
            else if (prod == 3) { result.trits[i + j] = 0; carry = 1; }
            else if (prod == 2) { result.trits[i + j] = -1; carry = 1; }
            else { result.trits[i + j] = (int8_t)prod; carry = 0; }
        }
        if (carry != 0 && i + b->len < max_len) {
            result.trits[i + b->len] = carry;
        }
    }
    
    result.len = max_len;
    return result;
}

// =============================================================================
// RDTSC
// =============================================================================

static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

// =============================================================================
// BENCHMARKS
// =============================================================================

uint64_t bench_trit_add(uint32_t iterations) {
    uint64_t start = rdtsc();
    trit result = 0;
    
    for (uint32_t i = 0; i < iterations; i++) {
        result = trit_add(result, 1);
        result = trit_add(result, -1);
        result = trit_add(result, 0);
    }
    
    return rdtsc() - start;
}

uint64_t bench_trits5(uint32_t iterations) {
    uint64_t start = rdtsc();
    trits5_t a = {0};
    trits5_t b = {255};
    
    for (uint32_t i = 0; i < iterations; i++) {
        trits5_t result = {0};
        for (int j = 0; j < 5; j++) {
            trit ta = trits5_get(a, j);
            trit tb = trits5_get(b, j);
            trits5_set(&result, j, trit_add(ta, tb));
        }
        a = result;
        b.packed++;
    }
    
    return rdtsc() - start;
}

uint64_t bench_ternary_add(uint32_t iterations) {
    uint64_t start = rdtsc();
    ternary_num_t a = ternary_from_int(12345);
    ternary_num_t b = ternary_from_int(67890);
    
    for (uint32_t i = 0; i < iterations; i++) {
        a = ternary_add(&a, &b);
        b = ternary_add(&b, &a);
    }
    
    return rdtsc() - start;
}

uint64_t bench_ternary_mul(uint32_t iterations) {
    uint64_t start = rdtsc();
    ternary_num_t a = ternary_from_int(123);
    ternary_num_t b = ternary_from_int(456);
    
    for (uint32_t i = 0; i < iterations; i++) {
        a = ternary_mul(&a, &b);
        b = ternary_from_int((ternary_to_int(&b) % 1000) + 1);
    }
    
    return rdtsc() - start;
}

uint64_t bench_to_ternary(uint32_t iterations) {
    uint64_t start = rdtsc();
    
    for (uint32_t i = 0; i < iterations; i++) {
        ternary_from_int((int32_t)i);
    }
    
    return rdtsc() - start;
}

uint64_t bench_from_ternary(uint32_t iterations) {
    uint64_t start = rdtsc();
    ternary_num_t num = ternary_from_int(12345);
    
    for (uint32_t i = 0; i < iterations; i++) {
        ternary_to_int(&num);
        num = ternary_add(&num, &(ternary_num_t){{1}, 1});
    }
    
    return rdtsc() - start;
}

// =============================================================================
// MAIN
// =============================================================================

int main(void) {
    printf("\n  [Ternary C Benchmark]\n\n");
    
    uint32_t iterations = 100000;
    
    uint64_t trit_add_cycles = bench_trit_add(iterations);
    uint64_t trits5_cycles = bench_trits5(iterations);
    uint64_t ternary_add_cycles = bench_ternary_add(iterations);
    uint64_t ternary_mul_cycles = bench_ternary_mul(iterations);
    uint64_t to_ternary_cycles = bench_to_ternary(iterations);
    uint64_t from_ternary_cycles = bench_from_ternary(iterations);
    
    printf("  Trit add (100k): %lu cycles\n", trit_add_cycles);
    printf("  Trits5 packed (100k): %lu cycles\n", trits5_cycles);
    printf("  Ternary add (100k): %lu cycles\n", ternary_add_cycles);
    printf("  Ternary mul (100k): %lu cycles\n", ternary_mul_cycles);
    printf("  Int→ternary (100k): %lu cycles\n", to_ternary_cycles);
    printf("  Ternary→int (100k): %lu cycles\n", from_ternary_cycles);
    
    uint64_t total = trit_add_cycles + trits5_cycles + ternary_add_cycles +
                     ternary_mul_cycles + to_ternary_cycles + from_ternary_cycles;
    printf("  Total: %lu cycles\n", total);
    
    printf("\n  Done!\n");
    
    return 0;
}
