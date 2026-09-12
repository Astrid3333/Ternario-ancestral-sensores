/**
 * TernaryAncestral.cpp — Implementación del Motor Ternario Ancestral
 * 
 * Optimizado para ATmega328 (2KB RAM, sin punto flotante)
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
    
    // Calcular número de trits necesarios
    while (temp != 0 && n < 10) {
        int8_t trit = ((temp % 3) + 3) % 3 - 1;  // balanceado
        trits[n] = trit;
        temp = (temp - trit) / 3;
        n++;
    }
    
    // Invertir (los trits se calcularon de derecha a izquierda)
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
    // Convertir bloque de 4 trits a valor decimal
    int32_t val = 0;
    for (uint8_t i = 0; i < 4; i++) {
        val = val * 3 + (block[i] + 1);  // mapear -1,0,1 → 0,1,2
    }
    return val % 33;
}

uint8_t detectCycle(const uint8_t* residues, uint8_t n, uint8_t* period) {
    if (n < 4) { *period = 0; return 0; }
    
    // Buscar períodos de 2 a n/2
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
    
    // Calcular residuos
    uint8_t residues[25];  // máximo 100 trits / 4 = 25 bloques
    for (uint8_t i = 0; i < n_blocks; i++) {
        residues[i] = blockResidue(trits + i * 4);
    }
    
    // Run-length encoding sobre residuos
    uint8_t i = 0;
    while (i < n_blocks) {
        uint8_t current = residues[i];
        uint8_t count = 1;
        while (i + count < n_blocks && residues[i + count] == current) {
            count++;
        }
        compressed[out_idx++] = current;   // residuo (0-32)
        compressed[out_idx++] = count;     // conteo
        i += count;
    }
    
    return out_idx;  // bytes escritos
}

// =============================================================================
// QUIPU CHECKSUM
// =============================================================================

void quipuEncode(const int8_t* block, int8_t* p1, int8_t* p2) {
    // Paridad aditiva: p1 = -(suma) mod 3 (balanceado)
    int8_t sum = 0;
    int8_t prod = 1;
    for (uint8_t i = 0; i < 5; i++) {
        sum += block[i];
        prod *= block[i];
    }
    
    // Mapear a balanceado: resultado mod-3 → -1,0,+1
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
// PIPELINE COMPLETO
// =============================================================================

uint8_t ancestralPipeline(
    const int16_t* raw_values, uint8_t n,
    uint8_t* output,
    int16_t low_threshold,
    int16_t high_threshold
) {
    // Paso 1: Convertir a trits
    int8_t trits[100];  // máximo 100 muestras
    uint8_t n_trits = n;
    if (n_trits > 100) n_trits = 100;
    
    for (uint8_t i = 0; i < n_trits; i++) {
        trits[i] = sensorToTrit(raw_values[i], low_threshold, high_threshold);
    }
    
    // Paso 2: Comprimir por residuos
    uint8_t compressed[200];
    uint8_t n_compressed = compressResidual(trits, n_trits, compressed);
    
    // Paso 3: Checksum quipu
    uint8_t n_blocks = n_trits / 5;
    int8_t checksum[50];
    quipuChecksumFull(trits, n_blocks * 5, checksum);
    
    // Empaquetar en output:
    // [n_trits, n_compressed, trits..., compressed..., checksum...]
    uint8_t idx = 0;
    output[idx++] = n_trits;
    output[idx++] = n_compressed;
    
    // Trits (cada trit es 1 byte con valor -1,0,1)
    for (uint8_t i = 0; i < n_trits; i++) {
        output[idx++] = (uint8_t)(trits[i] + 1);  // mapear a 0,1,2
    }
    
    // Datos comprimidos
    for (uint8_t i = 0; i < n_compressed; i++) {
        output[idx++] = compressed[i];
    }
    
    // Checksum
    for (uint8_t i = 0; i < n_blocks * 2; i++) {
        output[idx++] = (uint8_t)(checksum[i] + 1);  // mapear a 0,1,2
    }
    
    return idx;  // total bytes
}

uint8_t ancestralDecode(const uint8_t* compressed, uint8_t n_bytes, int8_t* trits) {
    uint8_t idx = 0;
    uint8_t n_trits = compressed[idx++];
    uint8_t n_compressed = compressed[idx++];
    
    // Saltar datos comprimidos y checksum
    idx += n_trits;           // trits
    idx += n_compressed;      // comprimidos
    // idx += n_blocks * 2;   // checksum (no necesario para decode)
    
    // Releer trits del buffer
    idx = 2;  // volver al inicio de trits
    for (uint8_t i = 0; i < n_trits; i++) {
        trits[i] = (int8_t)compressed[idx++] - 1;  // mapear de 0,1,2 a -1,0,+1
    }
    
    return n_trits;
}
