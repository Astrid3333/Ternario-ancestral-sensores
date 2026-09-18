/**
 * TernaryAncestral.h — Motor Ternario Ancestral OPTIMIZADO
 * 
 * Versión ultra-optimizada para sistemas con memoria limitada:
 * - ATmega328 (2KB RAM, 32KB flash)
 * - ESP8266 (80KB RAM)
 * - Sistemas Linux minimalistas (BusyBox, Tiny Core)
 * 
 * Optimizaciones aplicadas:
 * - Bit-packing: 3 trits en 5 bits (vs 3 bytes = 83% ahorro)
 * - Sin punto flotante
 * - Lookup tables para operaciones frecuentes
 * - Memoria estática (sin malloc)
 */

#ifndef TERNARY_ANCESTRAL_H
#define TERNARY_ANCESTRAL_H

#include <Arduino.h>

// =============================================================================
// CONFIGURACIÓN — Descomentar para habilitar/deshabilitar
// =============================================================================

#define TA_ENABLE_ORIGINAL    1    // Pipeline original (residuos mod-33)
#define TA_ENABLE_BABYLONIAN  1    // Pipeline babilónico (base 60)
#define TA_ENABLE_LITE        0    // Versión lite (solo funciones esenciales)

// =============================================================================
// CONSTANTES
// =============================================================================

#define TA_MAX_TRITS      100     // Máximo trits por operación
#define TA_MAX_OUTPUT     128     // Máximo bytes de salida
#define TA_BLOCK_SIZE     5       // Tamaño de bloque para quipu
#define TA_SYNC_ORIGINAL  0xAA    // Sync modo original
#define TA_SYNC_BABYLON   0xBB    // Sync modo babilónico

// =============================================================================
// TERNARY CODEC — Balanced Ternary {-1, 0, +1}
// =============================================================================

/**
 * Clasifica un valor de sensor en trit balanceado
 * @param value Valor crudo del sensor
 * @param low_threshold Umbral bajo
 * @param high_threshold Umbral alto
 * @return -1 (frío/bajo), 0 (normal), +1 (caliente/alto)
 */
static inline int8_t sensorToTrit(int16_t value, int16_t low_threshold, int16_t high_threshold) {
    if (value < low_threshold) return -1;
    if (value > high_threshold) return 1;
    return 0;
}

/**
 * Convierte trits a entero (máximo 10 trits)
 */
int32_t tritsToDecimal(const int8_t* trits, uint8_t n);

/**
 * Convierte entero a trits balanceados
 */
uint8_t decimalToTrits(int32_t decimal, int8_t* trits);

// =============================================================================
// BIT-PACKING — 3 trits en 5 bits
// =============================================================================

/**
 * Empaqueta 3 trits en 5 bits (valor 0-24)
 * @param t1, t2, t3 Trets (-1, 0, +1)
 * @return Valor empaquetado (0-24)
 */
static inline uint8_t packTrits3(int8_t t1, int8_t t2, int8_t t3) {
    // Mapear -1,0,+1 a 0,1,2
    uint8_t v1 = (uint8_t)(t1 + 1);
    uint8_t v2 = (uint8_t)(t2 + 1);
    uint8_t v3 = (uint8_t)(t3 + 1);
    return v1 * 9 + v2 * 3 + v3;  // 0-24, cabe en 5 bits
}

/**
 * Desempaqueta 5 bits a 3 trits
 * @param packed Valor empaquetado (0-24)
 * @param t1, t2, t3 Output: trits
 */
static inline void unpackTrits3(uint8_t packed, int8_t* t1, int8_t* t2, int8_t* t3) {
    *t1 = (int8_t)((packed / 9) % 3) - 1;
    *t2 = (int8_t)((packed / 3) % 3) - 1;
    *t3 = (int8_t)(packed % 3) - 1;
}

/**
 * Empaqueta array de trits en buffer de bits
 * @param trits Input: array de trits
 * @param n Input: número de trits (máximo TA_MAX_TRITS)
 * @param bits Output: buffer empaquetado (mínimo (n*5+7)/8 bytes)
 * @return Número de bytes escritos
 */
uint8_t packTrits(const int8_t* trits, uint8_t n, uint8_t* bits);

/**
 * Desempaqueta buffer de bits a array de trits
 * @param bits Input: buffer empaquetado
 * @param n Input: número de trits
 * @param trits Output: array de trits
 */
void unpackTrits(const uint8_t* bits, uint8_t n, int8_t* trits);

// =============================================================================
// RESIDUAL COMPRESSOR — Mod-33 (optimized)
// =============================================================================

/**
 * Comprime trits usando residuos mod-33 + run-length
 * @param trits Input: array de trits
 * @param n Input: número de trits
 * @param compressed Output: buffer comprimido
 * @return Número de bytes escritos
 */
uint8_t compressResidual(const int8_t* trits, uint8_t n, uint8_t* compressed);

/**
 * Detecta ciclos en secuencia de residuos
 */
uint8_t detectCycle(const uint8_t* residues, uint8_t n, uint8_t* period);

// =============================================================================
// QUIPU CHECKSUM — Parity Ternary (optimized)
// =============================================================================

/**
 * Genera checksum quipu para bloque de 5 trits
 */
void quipuEncode(const int8_t* block, int8_t* p1, int8_t* p2);

/**
 * Verifica integridad de bloque
 */
bool quipuVerify(const int8_t* block, int8_t p1, int8_t p2);

// =============================================================================
// BABYLONIAN CODEC — Base 60 (3.91 trits/digit)
// =============================================================================

/**
 * Convierte trits a dígitos babilónicos (base 60)
 */
uint8_t tritsToBabylonian(const int8_t* trits, uint8_t n_trits, uint8_t* digits);

/**
 * Convierte dígitos babilónicos a trits
 */
uint8_t babylonianToTrits(const uint8_t* digits, uint8_t n_digits, int8_t* trits);

// =============================================================================
// PIPELINES — Todas las opciones de compresión
// =============================================================================

#if TA_ENABLE_ORIGINAL
/**
 * Pipeline original: sensor → trits → residuos → output
 * Compresión: 2x vs binario
 * @return Número de bytes escritos
 */
uint8_t ancestralPipeline(
    const int16_t* raw_values, uint8_t n,
    uint8_t* output,
    int16_t low_threshold,
    int16_t high_threshold
);

/**
 * Decodifica pipeline original
 */
uint8_t ancestralDecode(const uint8_t* compressed, uint8_t n_bytes, int8_t* trits);
#endif

#if TA_ENABLE_BABYLONIAN
/**
 * Pipeline babilónico: sensor → trits → base60 → output
 * Compresión: 5x vs binario
 * @return Número de bytes escritos
 */
uint8_t babylonianPipeline(
    const int16_t* raw_values, uint8_t n,
    uint8_t* output,
    int16_t low_threshold,
    int16_t high_threshold
);

/**
 * Decodifica pipeline babilónico
 */
uint8_t babylonianDecode(const uint8_t* compressed, uint8_t n_bytes, int8_t* trits);
#endif

// =============================================================================
// PIPELINE ULTRA-LITE — Para sistemas con memoria extrema
// =============================================================================

/**
 * Pipeline mínimo: sensor → trits empaquetados → output
 * Sin compresión adicional, solo bit-packing
 * Compresión: 2.5x vs binario
 * RAM: ~100 bytes
 * Flash: ~500 bytes
 * @return Número de bytes escritos
 */
uint8_t litePipeline(
    const int16_t* raw_values, uint8_t n,
    uint8_t* output,
    int16_t low_threshold,
    int16_t high_threshold
);

/**
 * Decodifica pipeline lite
 */
uint8_t liteDecode(const uint8_t* compressed, uint8_t n_bytes, int8_t* trits);

#endif // TERNARY_ANCESTRAL_H
