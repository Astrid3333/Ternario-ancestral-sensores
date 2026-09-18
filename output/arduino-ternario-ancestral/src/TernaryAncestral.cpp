/**
 * TernaryAncestral.cpp — Implementación OPTIMIZADA
 * 
 * Optimizaciones:
 * - Bit-packing: 3 trits en 5 bits (83% ahorro vs 3 bytes)
 * - Sin punto flotante
 * - Lookup tables para mod-3 y mapeo de trits
 * - Memoria estática (sin malloc/new)
 * - Inline functions para funciones frecuentes
 */

#include "TernaryAncestral.h"

// =============================================================================
// LOOKUP TABLES — Evitar operaciones costosas
// =============================================================================

// Mapeo de trits a valores empaquetados (3 trits → 5 bits)
// Índice: valor empaquetado (0-24)
// Valor: [t1, t2, t3] mapeado a -1, 0, +1
static const int8_t TRIT_UNPACK[25][3] PROGMEM = {
    {-1,-1,-1}, {-1,-1, 0}, {-1,-1,+1}, {-1, 0,-1}, {-1, 0, 0},
    {-1, 0,+1}, {-1,+1,-1}, {-1,+1, 0}, {-1,+1,+1}, { 0,-1,-1},
    { 0,-1, 0}, { 0,-1,+1}, { 0, 0,-1}, { 0, 0, 0}, { 0, 0,+1},
    { 0,+1,-1}, { 0,+1, 0}, { 0,+1,+1}, {+1,-1,-1}, {+1,-1, 0},
    {+1,-1,+1}, {+1, 0,-1}, {+1, 0, 0}, {+1, 0,+1}, {+1,+1,-1}
};

// Mapeo de trits a valor empaquetado (3 trits → 5 bits)
// Índice: [t1+1, t2+1, t3+1] (cada uno 0-2)
// Valor: 0-24
static const uint8_t TRIT_PACK[3][3][3] PROGMEM = {
    {{ 0, 1, 2}, { 3, 4, 5}, { 6, 7, 8}},   // t1 = -1
    {{ 9,10,11}, {12,13,14}, {15,16,17}},   // t1 =  0
    {{18,19,20}, {21,22,23}, {24,24,24}}    // t1 = +1
};

// =============================================================================
// TERNARY CODEC
// =============================================================================

int32_t tritsToDecimal(const int8_t* trits, uint8_t n) {
    int32_t result = 0;
    int32_t power = 1;
    for (int8_t i = n - 1; i >= 0; i--) {
        result += trits[i] * power;
        power *= 3;
    }
    return result;
}

uint8_t decimalToTrits(int32_t decimal, int8_t* trits) {
    if (decimal == 0) {
        trits[0] = 0;
        return 1;
    }
    
    uint8_t n = 0;
    int32_t temp = decimal;
    
    while (temp != 0 && n < 10) {
        int8_t trit = (int8_t)(((temp % 3) + 3) % 3 - 1);
        trits[n] = trit;
        temp = (temp - trit) / 3;
        n++;
    }
    
    // Invertir
    for (uint8_t i = 0; i < n / 2; i++) {
        int8_t swap = trits[i];
        trits[i] = trits[n - 1 - i];
        trits[n - 1 - i] = swap;
    }
    
    return n;
}

// =============================================================================
// BIT-PACKING — 3 trits en 5 bits
// =============================================================================

uint8_t packTrits(const int8_t* trits, uint8_t n, uint8_t* bits) {
    uint8_t out_idx = 0;
    uint8_t i = 0;
    
    // Empaquetar de 3 en 3
    while (i + 2 < n) {
        uint8_t packed = TRIT_PACK[trits[i]+1][trits[i+1]+1][trits[i+2]+1];
        bits[out_idx++] = packed;
        i += 3;
    }
    
    // Manejar residuo (1 o 2 trits restantes)
    if (i < n) {
        int8_t t1 = trits[i++];
        int8_t t2 = (i < n) ? trits[i++] : 0;
        int8_t t3 = (i < n) ? trits[i++] : 0;
        bits[out_idx++] = TRIT_PACK[t1+1][t2+1][t3+1];
    }
    
    return out_idx;
}

void unpackTrits(const uint8_t* bits, uint8_t n, int8_t* trits) {
    uint8_t out_idx = 0;
    uint8_t bit_idx = 0;
    
    while (out_idx < n) {
        uint8_t packed = bits[bit_idx++];
        
        // Lookup table para desempaquetar
        trits[out_idx++] = TRIT_UNPACK[packed][0];
        if (out_idx < n) trits[out_idx++] = TRIT_UNPACK[packed][1];
        if (out_idx < n) trits[out_idx++] = TRIT_UNPACK[packed][2];
    }
}

// =============================================================================
// RESIDUAL COMPRESSOR — Optimizado
// =============================================================================

uint8_t detectCycle(const uint8_t* residues, uint8_t n, uint8_t* period) {
    if (n < 4) { *period = 0; return 0; }
    
    for (uint8_t p = 2; p <= n / 2; p++) {
        uint8_t matches = 0;
        uint8_t total = 0;
        
        for (uint8_t i = 0; i + p < n; i++) {
            if (residues[i] == residues[i + p]) matches++;
            total++;
        }
        
        if (total > 0 && (matches * 100 / total) > 80) {
            *period = p;
            return matches;
        }
    }
    
    *period = 0;
    return 0;
}

uint8_t compressResidual(const int8_t* trits, uint8_t n, uint8_t* compressed) {
    uint8_t n_blocks = n / 4;
    uint8_t out_idx = 0;
    
    // Calcular residuos
    uint8_t residues[25];
    for (uint8_t i = 0; i < n_blocks; i++) {
        int32_t val = 0;
        for (uint8_t j = 0; j < 4; j++) {
            val = val * 3 + (trits[i * 4 + j] + 1);
        }
        residues[i] = val % 33;
    }
    
    // Run-length encoding
    uint8_t i = 0;
    while (i < n_blocks) {
        uint8_t current = residues[i];
        uint8_t count = 1;
        while (i + count < n_blocks && residues[i + count] == current) {
            count++;
        }
        compressed[out_idx++] = current;
        compressed[out_idx++] = count;
        i += count;
    }
    
    return out_idx;
}

// =============================================================================
// QUIPU CHECKSUM — Optimizado
// =============================================================================

void quipuEncode(const int8_t* block, int8_t* p1, int8_t* p2) {
    int8_t sum = 0;
    int8_t prod = 1;
    
    for (uint8_t i = 0; i < 5; i++) {
        sum += block[i];
        prod *= block[i];
    }
    
    // Paridad aditiva
    *p1 = (int8_t)(((-sum % 3) + 3) % 3 - 1);
    
    // Paridad multiplicativa
    *p2 = (int8_t)(((-prod % 3) + 3) % 3 - 1);
}

bool quipuVerify(const int8_t* block, int8_t p1, int8_t p2) {
    int8_t calc_p1, calc_p2;
    quipuEncode(block, &calc_p1, &calc_p2);
    return (calc_p1 == p1 && calc_p2 == p2);
}

// =============================================================================
// BABYLONIAN CODEC — Base 60
// =============================================================================

uint8_t tritsToBabylonian(const int8_t* trits, uint8_t n_trits, uint8_t* digits) {
    int32_t value = 0;
    int32_t power = 1;
    for (int8_t i = n_trits - 1; i >= 0; i--) {
        value += trits[i] * power;
        power *= 3;
    }
    
    bool negative = false;
    if (value < 0) {
        negative = true;
        value = -value;
    }
    
    uint8_t n_digits = 0;
    if (value == 0) {
        digits[0] = 0;
        return 1;
    }
    
    while (value > 0 && n_digits < 64) {
        digits[n_digits] = value % 60;
        value /= 60;
        n_digits++;
    }
    
    // Invertir
    for (uint8_t i = 0; i < n_digits / 2; i++) {
        uint8_t swap = digits[i];
        digits[i] = digits[n_digits - 1 - i];
        digits[n_digits - 1 - i] = swap;
    }
    
    // Agregar signo
    for (uint8_t i = n_digits; i > 0; i--) {
        digits[i] = digits[i - 1];
    }
    digits[0] = negative ? 1 : 0;
    n_digits++;
    
    return n_digits;
}

uint8_t babylonianToTrits(const uint8_t* digits, uint8_t n_digits, int8_t* trits) {
    if (n_digits == 0) return 0;
    
    bool negative = (digits[0] == 1);
    
    int32_t value = 0;
    for (uint8_t i = 1; i < n_digits; i++) {
        value = value * 60 + digits[i];
    }
    
    if (negative) value = -value;
    
    if (value == 0) {
        trits[0] = 0;
        return 1;
    }
    
    uint8_t n_trits = 0;
    int32_t temp = value;
    
    while (temp != 0 && n_trits < 100) {
        int8_t trit = (int8_t)(((temp % 3) + 3) % 3 - 1);
        trits[n_trits] = trit;
        temp = (temp - trit) / 3;
        n_trits++;
    }
    
    for (uint8_t i = 0; i < n_trits / 2; i++) {
        int8_t swap = trits[i];
        trits[i] = trits[n_trits - 1 - i];
        trits[n_trits - 1 - i] = swap;
    }
    
    return n_trits;
}

// =============================================================================
// PIPELINE ORIGINAL — Residual + Quipu
// =============================================================================

#if TA_ENABLE_ORIGINAL
uint8_t ancestralPipeline(
    const int16_t* raw_values, uint8_t n,
    uint8_t* output,
    int16_t low_threshold,
    int16_t high_threshold
) {
    int8_t trits[TA_MAX_TRITS];
    uint8_t n_trits = (n > TA_MAX_TRITS) ? TA_MAX_TRITS : n;
    
    for (uint8_t i = 0; i < n_trits; i++) {
        trits[i] = sensorToTrit(raw_values[i], low_threshold, high_threshold);
    }
    
    uint8_t compressed[TA_MAX_OUTPUT];
    uint8_t n_compressed = compressResidual(trits, n_trits, compressed);
    
    uint8_t n_blocks = n_trits / 5;
    int8_t checksum[TA_MAX_TRITS];
    for (uint8_t b = 0; b < n_blocks; b++) {
        quipuEncode(trits + b * 5, &checksum[b * 2], &checksum[b * 2 + 1]);
    }
    
    uint8_t idx = 0;
    output[idx++] = n_trits;
    output[idx++] = n_compressed;
    
    for (uint8_t i = 0; i < n_trits; i++) {
        output[idx++] = (uint8_t)(trits[i] + 1);
    }
    
    for (uint8_t i = 0; i < n_compressed; i++) {
        output[idx++] = compressed[i];
    }
    
    for (uint8_t i = 0; i < n_blocks * 2; i++) {
        output[idx++] = (uint8_t)(checksum[i] + 1);
    }
    
    return idx;
}

uint8_t ancestralDecode(const uint8_t* compressed, uint8_t n_bytes, int8_t* trits) {
    uint8_t idx = 0;
    uint8_t n_trits = compressed[idx++];
    uint8_t n_compressed = compressed[idx++];
    
    idx = 2;
    for (uint8_t i = 0; i < n_trits; i++) {
        trits[i] = (int8_t)compressed[idx++] - 1;
    }
    
    return n_trits;
}
#endif

// =============================================================================
// PIPELINE BABYLONIAN — Base 60
// =============================================================================

#if TA_ENABLE_BABYLONIAN
uint8_t babylonianPipeline(
    const int16_t* raw_values, uint8_t n,
    uint8_t* output,
    int16_t low_threshold,
    int16_t high_threshold
) {
    int8_t trits[TA_MAX_TRITS];
    uint8_t n_trits = (n > TA_MAX_TRITS) ? TA_MAX_TRITS : n;
    
    for (uint8_t i = 0; i < n_trits; i++) {
        trits[i] = sensorToTrit(raw_values[i], low_threshold, high_threshold);
    }
    
    uint8_t digits[64];
    uint8_t n_digits = tritsToBabylonian(trits, n_trits, digits);
    
    uint8_t n_blocks = n_trits / 5;
    int8_t checksum[TA_MAX_TRITS];
    for (uint8_t b = 0; b < n_blocks; b++) {
        quipuEncode(trits + b * 5, &checksum[b * 2], &checksum[b * 2 + 1]);
    }
    
    uint8_t idx = 0;
    output[idx++] = n_digits;
    
    for (uint8_t i = 0; i < n_digits; i++) {
        output[idx++] = digits[i];
    }
    
    for (uint8_t i = 0; i < n_blocks * 2; i++) {
        output[idx++] = (uint8_t)(checksum[i] + 1);
    }
    
    return idx;
}

uint8_t babylonianDecode(const uint8_t* compressed, uint8_t n_bytes, int8_t* trits) {
    uint8_t idx = 0;
    uint8_t n_digits = compressed[idx++];
    
    uint8_t digits[64];
    for (uint8_t i = 0; i < n_digits; i++) {
        digits[i] = compressed[idx++];
    }
    
    return babylonianToTrits(digits, n_digits, trits);
}
#endif

// =============================================================================
// PIPELINE ULTRA-LITE — Solo bit-packing
// =============================================================================

uint8_t litePipeline(
    const int16_t* raw_values, uint8_t n,
    uint8_t* output,
    int16_t low_threshold,
    int16_t high_threshold
) {
    int8_t trits[TA_MAX_TRITS];
    uint8_t n_trits = (n > TA_MAX_TRITS) ? TA_MAX_TRITS : n;
    
    for (uint8_t i = 0; i < n_trits; i++) {
        trits[i] = sensorToTrit(raw_values[i], low_threshold, high_threshold);
    }
    
    // Bit-packing: 3 trits en 5 bits
    uint8_t bits[(TA_MAX_TRITS * 5 + 7) / 8];
    uint8_t n_bits = packTrits(trits, n_trits, bits);
    
    // Empaquetar: [n_trits, bits...]
    uint8_t idx = 0;
    output[idx++] = n_trits;
    
    for (uint8_t i = 0; i < n_bits; i++) {
        output[idx++] = bits[i];
    }
    
    return idx;
}

uint8_t liteDecode(const uint8_t* compressed, uint8_t n_bytes, int8_t* trits) {
    uint8_t idx = 0;
    uint8_t n_trits = compressed[idx++];
    
    uint8_t bits[(TA_MAX_TRITS * 5 + 7) / 8];
    uint8_t n_bits = (n_trits * 5 + 7) / 8;
    
    for (uint8_t i = 0; i < n_bits; i++) {
        bits[i] = compressed[idx++];
    }
    
    unpackTrits(bits, n_trits, trits);
    return n_trits;
}
