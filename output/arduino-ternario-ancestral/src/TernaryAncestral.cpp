/**
 * TernaryAncestral.cpp — Implementación del Motor Ternario Ancestral
 * 
 * Optimizado para ATmega328 (2KB RAM, sin punto flotante)
 * 
 * Incluye:
 * - Ternario original: residuos mod-33 + quipu
 * - Babilónico (base 60): 3.91 trits/dígito, 74% menos bytes
 */

#include "TernaryAncestral.h"

// =============================================================================
// TERNARY CODEC
// =============================================================================

int8_t sensorToTrit(int16_t value, int16_t low_threshold, int16_t high_threshold) {
    if (value < low_threshold) return -1;   // frío/bajo
    if (value > high_threshold) return 1;   // caliente/alto
    return 0;                               // normal
}

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
        int8_t trit = ((temp % 3) + 3) % 3 - 1;
        trits[n] = trit;
        temp = (temp - trit) / 3;
        n++;
    }
    
    for (uint8_t i = 0; i < n / 2; i++) {
        int8_t swap = trits[i];
        trits[i] = trits[n - 1 - i];
        trits[n - 1 - i] = swap;
    }
    
    return n;
}

// =============================================================================
// RESIDUAL COMPRESSOR
// =============================================================================

uint8_t blockResidue(const int8_t* block) {
    int32_t val = 0;
    for (uint8_t i = 0; i < 4; i++) {
        val = val * 3 + (block[i] + 1);
    }
    return val % 33;
}

uint8_t detectCycle(const uint8_t* residues, uint8_t n, uint8_t* period) {
    if (n < 4) { *period = 0; return 0; }
    
    for (uint8_t p = 2; p <= n / 2; p++) {
        uint8_t matches = 0;
        uint8_t total_checks = 0;
        
        for (uint8_t i = 0; i + p < n; i++) {
            if (residues[i] == residues[i + p]) {
                matches++;
            }
            total_checks++;
        }
        
        if (total_checks > 0 && (matches * 100 / total_checks) > 80) {
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
    
    uint8_t residues[25];
    for (uint8_t i = 0; i < n_blocks; i++) {
        residues[i] = blockResidue(trits + i * 4);
    }
    
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
// QUIPU CHECKSUM
// =============================================================================

void quipuEncode(const int8_t* block, int8_t* p1, int8_t* p2) {
    int8_t sum = 0;
    int8_t prod = 1;
    for (uint8_t i = 0; i < 5; i++) {
        sum += block[i];
        prod *= block[i];
    }
    
    *p1 = -sum;
    *p1 = ((*p1 % 3) + 3) % 3 - 1;
    
    *p2 = -prod;
    *p2 = ((*p2 % 3) + 3) % 3 - 1;
}

bool quipuVerify(const int8_t* block, int8_t p1, int8_t p2) {
    int8_t calc_p1, calc_p2;
    quipuEncode(block, &calc_p1, &calc_p2);
    return (calc_p1 == p1 && calc_p2 == p2);
}

void quipuChecksumFull(const int8_t* data, uint8_t n, int8_t* checksum) {
    uint8_t n_blocks = n / 5;
    for (uint8_t b = 0; b < n_blocks; b++) {
        int8_t p1, p2;
        quipuEncode(data + b * 5, &p1, &p2);
        checksum[b * 2] = p1;
        checksum[b * 2 + 1] = p2;
    }
}

// =============================================================================
// BABYLONIAN CODEC — Base 60 (3.91 trits per digit)
// =============================================================================

uint8_t tritsToBabylonian(const int8_t* trits, uint8_t n_trits, uint8_t* digits) {
    // Convertir trits a valor entero
    int32_t value = 0;
    int32_t power = 1;
    for (int8_t i = n_trits - 1; i >= 0; i--) {
        value += trits[i] * power;
        power *= 3;
    }
    
    // Manejar valor negativo
    bool negative = false;
    if (value < 0) {
        negative = true;
        value = -value;
    }
    
    // Convertir a base 60
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
    
    // Agregar signo como primer dígito
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
        int8_t trit = ((temp % 3) + 3) % 3 - 1;
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

uint8_t babylonianPipeline(
    const int16_t* raw_values, uint8_t n,
    uint8_t* output,
    int16_t low_threshold,
    int16_t high_threshold
) {
    // Convertir a trits
    int8_t trits[100];
    uint8_t n_trits = n;
    if (n_trits > 100) n_trits = 100;
    
    for (uint8_t i = 0; i < n_trits; i++) {
        trits[i] = sensorToTrit(raw_values[i], low_threshold, high_threshold);
    }
    
    // Convertir a dígitos babilónicos
    uint8_t digits[64];
    uint8_t n_digits = tritsToBabylonian(trits, n_trits, digits);
    
    // Checksum quipu
    uint8_t n_blocks = n_trits / 5;
    int8_t checksum[50];
    quipuChecksumFull(trits, n_blocks * 5, checksum);
    
    // Empaquetar: [n_digits, digits..., checksum...]
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

// =============================================================================
// PIPELINE ORIGINAL — Residual + Quipu
// =============================================================================

uint8_t ancestralPipeline(
    const int16_t* raw_values, uint8_t n,
    uint8_t* output,
    int16_t low_threshold,
    int16_t high_threshold
) {
    int8_t trits[100];
    uint8_t n_trits = n;
    if (n_trits > 100) n_trits = 100;
    
    for (uint8_t i = 0; i < n_trits; i++) {
        trits[i] = sensorToTrit(raw_values[i], low_threshold, high_threshold);
    }
    
    uint8_t compressed[200];
    uint8_t n_compressed = compressResidual(trits, n_trits, compressed);
    
    uint8_t n_blocks = n_trits / 5;
    int8_t checksum[50];
    quipuChecksumFull(trits, n_blocks * 5, checksum);
    
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
    
    idx += n_trits;
    idx += n_compressed;
    
    idx = 2;
    for (uint8_t i = 0; i < n_trits; i++) {
        trits[i] = (int8_t)compressed[idx++] - 1;
    }
    
    return n_trits;
}
