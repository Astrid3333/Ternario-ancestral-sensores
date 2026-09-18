/**
 * trit_rust.h — Declaraciones de funciones Rust para kernel C
 */

#ifndef TRIT_RUST_H
#define TRIT_RUST_H

#include <stdint.h>

// Trit operations
int8_t trit_add_rust(int8_t a, int8_t b);
int8_t trit_mul_rust(int8_t a, int8_t b);
int8_t trit_neg_rust(int8_t a);
uint8_t trit_to_char_rust(int8_t t);

// Packed trits (5 per byte)
int8_t trits5_get_rust(uint8_t packed, int i);
void trits5_set_rust(uint8_t* packed, int i, int8_t value);
int16_t trits5_to_i16_rust(uint8_t packed);
uint8_t trits5_from_i16_rust(int16_t v);

// Variable-length ternary numbers
typedef struct {
    int8_t trits[32];
    int32_t len;
} ternary_num_t;

ternary_num_t ternary_from_i32_rust(int32_t v);
int32_t ternary_to_i32_rust(const ternary_num_t* num);
void ternary_add_rust(const ternary_num_t* a, const ternary_num_t* b, ternary_num_t* result);
void ternary_mul_rust(const ternary_num_t* a, const ternary_num_t* b, ternary_num_t* result);
void ternary_to_str_rust(const ternary_num_t* num, uint8_t* buf);

// Benchmark
uint64_t rdtsc_rust(void);
uint64_t bench_ternary_add_rust(uint32_t iterations);
uint64_t bench_ternary_mul_rust(uint32_t iterations);

#endif
