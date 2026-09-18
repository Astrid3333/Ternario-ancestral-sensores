/**
 * bench_vs.c — Benchmark: Rust vs C ternary operations
 *
 * Compilar:
 *   gcc -O3 -march=native -o bench_vs bench_vs.c -L. -ltrit_64
 */

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "trit_rust.h"

// =============================================================================
// C IMPLEMENTATIONS
// =============================================================================

int8_t trit_add_c(int8_t a, int8_t b) {
    int sum = a + b;
    if (sum == -2) return 1;
    if (sum == 2) return -1;
    return sum;
}

typedef struct {
    int8_t trits[32];
    int32_t len;
} ternary_c;

ternary_c ternary_from_i32_c(int32_t v) {
    ternary_c num;
    num.len = 0;
    if (v == 0) { num.trits[0] = 0; num.len = 1; return num; }
    while (v != 0 && num.len < 32) {
        int rem = ((v % 3) + 3) % 3;
        v = (v - rem) / 3;
        num.trits[num.len++] = rem - 1;
    }
    return num;
}

int32_t ternary_to_i32_c(ternary_c* num) {
    int32_t result = 0;
    for (int i = 0; i < num->len; i++)
        result += num->trits[i] * (1 << (i * 2));
    return result;
}

void ternary_add_c(ternary_c* a, ternary_c* b, ternary_c* result) {
    int max_len = a->len > b->len ? a->len : b->len;
    int8_t carry = 0;
    for (int i = 0; i < max_len && i < 31; i++) {
        int8_t va = i < a->len ? a->trits[i] : 0;
        int8_t vb = i < b->len ? b->trits[i] : 0;
        int sum = va + vb + carry;
        if (sum == -3) { result->trits[i] = 0; carry = -1; }
        else if (sum == -2) { result->trits[i] = 1; carry = -1; }
        else if (sum == 3) { result->trits[i] = 0; carry = 1; }
        else if (sum == 2) { result->trits[i] = -1; carry = 1; }
        else { result->trits[i] = sum; carry = 0; }
    }
    if (carry && max_len < 31) {
        result->trits[max_len] = carry;
        result->len = max_len + 1;
    } else {
        result->len = max_len;
    }
}

void ternary_mul_c(ternary_c* a, ternary_c* b, ternary_c* result) {
    int max_len = a->len + b->len;
    if (max_len > 32) max_len = 32;
    for (int i = 0; i < max_len; i++) result->trits[i] = 0;
    for (int i = 0; i < a->len && i < 31; i++) {
        if (a->trits[i] == 0) continue;
        int8_t carry = 0;
        for (int j = 0; j < b->len && (i + j) < 31; j++) {
            int prod = a->trits[i] * b->trits[j] + result->trits[i + j] + carry;
            if (prod == -3) { result->trits[i+j] = 0; carry = -1; }
            else if (prod == -2) { result->trits[i+j] = 1; carry = -1; }
            else if (prod == 3) { result->trits[i+j] = 0; carry = 1; }
            else if (prod == 2) { result->trits[i+j] = -1; carry = 1; }
            else { result->trits[i+j] = prod; carry = 0; }
        }
        if (carry && i + b->len < 31) result->trits[i + b->len] = carry;
    }
    result->len = max_len;
}

// =============================================================================
// BENCHMARK
// =============================================================================

static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

int main(void) {
    printf("\n  ════════════════════════════════════════════\n");
    printf("  ║  TERNARY BENCHMARK: Rust vs C (100k)   ║\n");
    printf("  ════════════════════════════════════════════\n\n");

    uint32_t N = 100000;

    // --- Trit add ---
    uint64_t start, rust_time, c_time;

    start = rdtsc();
    int8_t r_res = 0;
    for (uint32_t i = 0; i < N; i++) {
        r_res = trit_add_rust(r_res, 1);
        r_res = trit_add_rust(r_res, -1);
    }
    rust_time = rdtsc() - start;

    start = rdtsc();
    int8_t c_res = 0;
    for (uint32_t i = 0; i < N; i++) {
        c_res = trit_add_c(c_res, 1);
        c_res = trit_add_c(c_res, -1);
    }
    c_time = rdtsc() - start;

    printf("  Trit add:       Rust %8lu  C %8lu  ", rust_time, c_time);
    printf(rust_time < c_time ? "RUST WINS\n" : "C WINS\n");

    // --- Ternary add ---
    start = rdtsc();
    ternary_num_t ra = ternary_from_i32_rust(12345);
    ternary_num_t rb = ternary_from_i32_rust(67890);
    ternary_num_t rr;
    for (uint32_t i = 0; i < N; i++) {
        ternary_add_rust(&ra, &rb, &rr);
        ra = rr;
    }
    rust_time = rdtsc() - start;

    start = rdtsc();
    ternary_c ca = ternary_from_i32_c(12345);
    ternary_c cb = ternary_from_i32_c(67890);
    ternary_c cr;
    for (uint32_t i = 0; i < N; i++) {
        ternary_add_c(&ca, &cb, &cr);
        ca = cr;
    }
    c_time = rdtsc() - start;

    printf("  Ternary add:    Rust %8lu  C %8lu  ", rust_time, c_time);
    printf(rust_time < c_time ? "RUST WINS\n" : "C WINS\n");

    // --- Ternary mul ---
    start = rdtsc();
    ra = ternary_from_i32_rust(123);
    rb = ternary_from_i32_rust(456);
    for (uint32_t i = 0; i < N; i++) {
        ternary_mul_rust(&ra, &rb, &rr);
        ra = rr;
    }
    rust_time = rdtsc() - start;

    start = rdtsc();
    ca = ternary_from_i32_c(123);
    cb = ternary_from_i32_c(456);
    for (uint32_t i = 0; i < N; i++) {
        ternary_mul_c(&ca, &cb, &cr);
        ca = cr;
    }
    c_time = rdtsc() - start;

    printf("  Ternary mul:    Rust %8lu  C %8lu  ", rust_time, c_time);
    printf(rust_time < c_time ? "RUST WINS\n" : "C WINS\n");

    // --- Int→Ternary ---
    start = rdtsc();
    for (uint32_t i = 0; i < N; i++) {
        ternary_from_i32_rust(i);
    }
    rust_time = rdtsc() - start;

    start = rdtsc();
    for (uint32_t i = 0; i < N; i++) {
        ternary_from_i32_c(i);
    }
    c_time = rdtsc() - start;

    printf("  Int->Ternary:   Rust %8lu  C %8lu  ", rust_time, c_time);
    printf(rust_time < c_time ? "RUST WINS\n" : "C WINS\n");

    // --- Ternary→Int ---
    start = rdtsc();
    ra = ternary_from_i32_rust(12345);
    for (uint32_t i = 0; i < N; i++) {
        ternary_to_i32_rust(&ra);
    }
    rust_time = rdtsc() - start;

    start = rdtsc();
    ca = ternary_from_i32_c(12345);
    for (uint32_t i = 0; i < N; i++) {
        ternary_to_i32_c(&ca);
    }
    c_time = rdtsc() - start;

    printf("  Ternary->Int:   Rust %8lu  C %8lu  ", rust_time, c_time);
    printf(rust_time < c_time ? "RUST WINS\n" : "C WINS\n");

    printf("\n  ════════════════════════════════════════════\n");
    printf("  DONE\n\n");

    return 0;
}
