/**
 * ternary.h — Definiciones core del kernel ternario ancestral
 * 
 * Conceptos:
 * - Ternary: {-1, 0, +1} como unidad básica
 * - Base 60 (Babilónico): direcciones de memoria
 * - Ciclos Mayas: planificación de procesos
 * - Quipu: sistema de archivos
 */

#ifndef TERNARY_H
#define TERNARY_H

// =============================================================================
// TIPOS TERNARIOS
// =============================================================================

// Trit balanceado: -1, 0, +1
typedef int8_t trit_t;

// Valor ternario empaquetado (3 trits en 5 bits)
typedef uint8_t trit_packed_t;

// Dirección babilónica (base 60)
typedef struct {
    uint8_t high;   // dígito alto (0-59)
    uint8_t low;    // dígito bajo (0-59)
} __attribute__((packed)) babilonian_addr_t;

// ID de proceso maya
typedef struct {
    uint16_t tzolkin;   // 1-260 (ciclo sagrado)
    uint8_t haab;       // 1-365 (año solar)
} __attribute__((packed)) maya_pid_t;

// =============================================================================
// CONSTANTES
// =============================================================================

#define TRIT_NEG    (-1)
#define TRIT_ZERO   (0)
#define TRIT_POS    (1)

#define BASE_60     60
#define TZOLKIN     260     // Días en ciclo sagrado Maya
#define HAAB        365     // Días en año Maya
#define MAX_PROCS   33      // Máximo procesos (mod-33)
#define MEM_BLOCKS  60      // Bloques de memoria (base 60)
#define BLOCK_SIZE  60      // Bytes por bloque

// =============================================================================
// OPERACIONES TERNARIAS BÁSICAS
// =============================================================================

// Suma ternaria: {-1,0,+1} + {-1,0,+1} → {-2,-1,0,+1,+2}
static inline trit_t trit_add(trit_t a, trit_t b) {
    trit_t sum = a + b;
    if (sum > 1) return 1;
    if (sum < -1) return -1;
    return sum;
}

// Multiplicación ternaria
static inline trit_t trit_mul(trit_t a, trit_t b) {
    if (a == 0 || b == 0) return 0;
    return (a == b) ? 1 : -1;
}

// Negación ternaria
static inline trit_t trit_neg(trit_t a) {
    return -a;
}

// Comparación ternaria
static inline trit_t trit_cmp(trit_t a, trit_t b) {
    if (a > b) return 1;
    if (a < b) return -1;
    return 0;
}

// =============================================================================
// EMPAQUETADO DE TRITS (3 trits → 5 bits)
// =============================================================================

static inline trit_packed_t pack_trits(trit_t t1, trit_t t2, trit_t t3) {
    uint8_t v1 = (uint8_t)(t1 + 1);  // 0, 1, 2
    uint8_t v2 = (uint8_t)(t2 + 1);
    uint8_t v3 = (uint8_t)(t3 + 1);
    return v1 * 9 + v2 * 3 + v3;     // 0-24
}

static inline void unpack_trits(trit_packed_t packed, trit_t* t1, trit_t* t2, trit_t* t3) {
    *t1 = (trit_t)((packed / 9) % 3) - 1;
    *t2 = (trit_t)((packed / 3) % 3) - 1;
    *t3 = (trit_t)(packed % 3) - 1;
}

// =============================================================================
// DIRECCIONES BABILÓNICAS (Base 60)
// =============================================================================

// Convertir dirección lineal a babilónica
static inline babilonian_addr_t linear_to_babilonian(uint16_t addr) {
    babilonian_addr_t result;
    result.high = addr / BASE_60;
    result.low = addr % BASE_60;
    return result;
}

// Convertir dirección babilónica a lineal
static inline uint16_t babilonian_to_linear(babilonian_addr_t addr) {
    return addr.high * BASE_60 + addr.low;
}

// =============================================================================
// IDs MAYAS
// =============================================================================

// Crear ID maya desde contador
static inline maya_pid_t make_maya_pid(uint32_t counter) {
    maya_pid_t pid;
    pid.tzolkin = (counter % TZOLKIN) + 1;
    pid.haab = (counter % HAAB) + 1;
    return pid;
}

// Comparar IDs mayas
static inline trit_t compare_maya_pid(maya_pid_t a, maya_pid_t b) {
    if (a.tzolkin != b.tzolkin) {
        return (a.tzolkin > b.tzolkin) ? 1 : -1;
    }
    if (a.haab != b.haab) {
        return (a.haab > b.haab) ? 1 : -1;
    }
    return 0;
}

#endif // TERNARY_H
