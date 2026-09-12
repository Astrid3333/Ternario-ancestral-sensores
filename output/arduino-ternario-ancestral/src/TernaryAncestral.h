/**
 * TernaryAncestral.h — Motor Ternario Ancestral para Arduino
 * 
 * Codificación ternaria, compresión por residuos mod-33,
 * y checksum tipo quipu para sensores IoT.
 * 
 * Inspiración: aritmética ternaria rusa, calendario persa,
 * quipu inca.
 * 
 * Compatible con: Arduino Uno/Nano (ATmega328), ESP32, STM32
 */

#ifndef TERNARY_ANCESTRAL_H
#define TERNARY_ANCESTRAL_H

#include <Arduino.h>

// =============================================================================
// TERNARY CODEC — Balanced Ternary {-1, 0, +1}
// =============================================================================

/**
 * Clasifica un valor de sensor en trit balanceado
 * @param value Valor crudo del sensor
 * @param low_threshold Umbral bajo (default 340 para DHT11 en A0)
 * @param high_threshold Umbral alto (default 700)
 * @return -1 (frío/bajo), 0 (normal), +1 (caliente/alto)
 */
int8_t sensorToTrit(int16_t value, int16_t low_threshold = 340, int16_t high_threshold = 700);

/**
 * Convierte trits balanceados a decimal (para secuencias cortas ≤10 trits)
 * @param trits Array de trits
 * @param n Número de trits
 * @return Valor decimal
 */
int32_t tritsToDecimal(const int8_t* trits, uint8_t n);

/**
 * Convierte decimal a trits balanceados
 * @param decimal Valor a convertir
 * @param trits Output array (mínimo 10 elementos)
 * @return Número de trits escritos
 */
uint8_t decimalToTrits(int32_t decimal, int8_t* trits);

// =============================================================================
// RESIDUAL COMPRESSOR — Mod-33 (Persian Cycle)
// =============================================================================

/**
 * Calcula residuo mod-33 de un bloque de trits
 * @param block Array de 4 trits
 * @return Residuo (0-32)
 */
uint8_t blockResidue(const int8_t* block);

/**
 * Detecta ciclos en secuencia de residuos
 * @param residues Array de residuos
 * @param n Número de residuos
 * @param period Output: período detectado (0 = sin ciclo)
 * @return Número de ocurrencias del ciclo
 */
uint8_t detectCycle(const uint8_t* residues, uint8_t n, uint8_t* period);

/**
 * Comprime secuencia de trits usando residuos mod-33
 * @param trits Input: secuencia de trits
 * @param n Input: longitud
 * @param compressed Output: array comprimido [residuo, conteo, ...]
 * @return Número de elementos en compressed
 */
uint8_t compressResidual(const int8_t* trits, uint8_t n, uint8_t* compressed);

// =============================================================================
// QUIPU CHECKSUM — Parity Ternary
// =============================================================================

/**
 * Genera checksum quipu para bloque de 5 trits
 * @param block Array de 5 trits
 * @param p1 Output: paridad aditiva (trit)
 * @param p2 Output: paridad multiplicativa (trit)
 */
void quipuEncode(const int8_t* block, int8_t* p1, int8_t* p2);

/**
 * Verifica integridad de bloque con checksum
 * @param block Array de 5 trits
 * @param p1 Paridad aditiva recibida
 * @param p2 Paridad multiplicativa recibida
 * @return true si sin errores
 */
bool quipuVerify(const int8_t* block, int8_t p1, int8_t p2);

/**
 * Calcula checksum completo para array largo
 * @param data Input: datos
 * @param n Input: longitud (debe ser múltiplo de 5)
 * @param checksum Output: array de pares [p1, p2] por bloque
 */
void quipuChecksumFull(const int8_t* data, uint8_t n, int8_t* checksum);

// =============================================================================
// PIPELINE COMPLETO — Sensor → Ternario → Comprimido → Checksum
// =============================================================================

/**
 * Pipeline completo: codifica, comprime y genera checksum
 * @param raw_values Valores crudos del sensor (array)
 * @param n Número de muestras
 * @param output Buffer de salida (máximo 2*n bytes)
 * @param thresholds [low, high] umbrales
 * @return Número de bytes escritos en output
 */
uint8_t ancestralPipeline(
    const int16_t* raw_values, uint8_t n,
    uint8_t* output,
    int16_t low_threshold = 340,
    int16_t high_threshold = 700
);

/**
 * Decodifica pipeline: comprimido → valores originales
 * @param compressed Datos comprimidos
 * @param n_bytes Número de bytes
 * @param trits Output: array de trits decodificados
 * @return Número de trits escritos
 */
uint8_t ancestralDecode(const uint8_t* compressed, uint8_t n_bytes, int8_t* trits);

#endif // TERNARY_ANCESTRAL_H
