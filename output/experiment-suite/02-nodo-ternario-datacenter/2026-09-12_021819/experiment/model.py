"""model.py — Motor ternario ancestral: codec, compresión residual, quipu checksum."""

import numpy as np
import math

MODULUS = 33
QUIPU_BLOCK = 5
BOLTZMANN_K = 1.380649e-23


# =============================================================================
# TERNARY CODEC
# =============================================================================

def trits_to_decimal(trits):
    """Convierte trits balanceados a decimal (secuencias cortas)."""
    result = 0
    for i, t in enumerate(trits):
        result += t * (3 ** i)
    return result


def decimal_to_trits(decimal, max_trits=10):
    """Convierte decimal a trits balanceados."""
    trits = []
    n = abs(decimal)
    while n > 0:
        remainder = n % 3
        if remainder == 2:
            trits.append(-1)
            n = n // 3 + 1
        else:
            trits.append(remainder)
            n = n // 3
    if decimal < 0:
        trits = [-t for t in trits]
    if not trits:
        trits = [0]
    return trits[:max_trits]


# =============================================================================
# RESIDUAL COMPRESSOR — Mod-33
# =============================================================================

def block_residue(block):
    """Calcula residuo mod-33 de un bloque de trits."""
    decimal = trits_to_decimal(block)
    return decimal % MODULUS


def compress_residual(trits):
    """Comprime secuencia de trits usando residuos mod-33."""
    block_size = 4
    compressed = []
    for i in range(0, len(trits), block_size):
        block = trits[i:i + block_size]
        if len(block) < block_size:
            block = list(block) + [0] * (block_size - len(block))
        residue = block_residue(block)
        # Run-length encoding
        count = 1
        while i + count * block_size < len(trits):
            next_block = trits[i + count * block_size:i + (count + 1) * block_size]
            if len(next_block) < block_size:
                next_block = list(next_block) + [0] * (block_size - len(next_block))
            if block_residue(next_block) == residue:
                count += 1
            else:
                break
        compressed.extend([residue, count])
    return compressed


def decompress_residual(compressed, total_trits):
    """Descomprime residuos mod-33 a trits (aproximación)."""
    # Nota: la descompresión exacta requiere lookup table
    # Aquí reconstruimos los trits desde los residuos
    trits = []
    i = 0
    while i < len(compressed) and len(trits) < total_trits:
        residue = compressed[i]
        count = compressed[i + 1] if i + 1 < len(compressed) else 1
        # Generar trits desde el residuo (simplificado)
        for _ in range(count):
            for shift in range(4):
                if len(trits) >= total_trits:
                    break
                trits.append((residue >> (shift * 2)) % 3 - 1)
        i += 2
    return trits[:total_trits]


# =============================================================================
# QUIPU CHECKSUM — Parity Ternary
# =============================================================================

def quipu_encode(block):
    """Genera checksum quipu para bloque de 5 trits."""
    p1 = -(sum(block)) % 3  # paridad aditiva
    product = 1
    for t in block:
        product *= (t + 2)  # mapear {-1,0,1} a {1,2,3}
    p2 = -(product) % 3     # paridad multiplicativa
    # Convertir a trits balanceados
    p1_bal = p1 if p1 <= 1 else p1 - 3
    p2_bal = p2 if p2 <= 1 else p2 - 3
    return p1_bal, p2_bal


def quipu_verify(block, p1, p2):
    """Verifica integridad de bloque con checksum."""
    p1_calc, p2_calc = quipu_encode(block)
    return p1_calc == p1 and p2_calc == p2


def quipu_checksum_full(data):
    """Calcula checksum completo para array largo."""
    checksum = []
    block_size = QUIPU_BLOCK
    for i in range(0, len(data), block_size):
        block = data[i:i + block_size]
        if len(block) < block_size:
            block = list(block) + [0] * (block_size - len(block))
        p1, p2 = quipu_encode(block)
        checksum.extend([p1, p2])
    return checksum


# =============================================================================
# PIPELINE COMPLETO
# =============================================================================

def ancestral_pipeline(raw_values, low=20.0, high=25.0):
    """Pipeline completo: sensor → ternario → comprimido → checksum."""
    # 1. Convertir a trits
    trits = np.array([sensor_to_trit(v, low, high) for v in raw_values])
    
    # 2. Comprimir con residuos mod-33
    compressed = compress_residual(trits)
    
    # 3. Generar checksum quipu
    checksum = quipu_checksum_full(trits)
    
    return {
        'trits': trits.tolist(),
        'n_trits': len(trits),
        'compressed': compressed,
        'n_compressed': len(compressed),
        'checksum': checksum,
        'n_checksum': len(checksum)
    }


def sensor_to_trit(value, low=20.0, high=25.0):
    """Convierte un valor de sensor a trit balanceado."""
    if value < low:
        return -1
    elif value > high:
        return 1
    else:
        return 0


# =============================================================================
# MÉTRICAS DE COMPRESIÓN
# =============================================================================

def compression_ratio(original_bits, compressed_bits):
    """Calcula ratio de compresión."""
    if compressed_bits == 0:
        return float('inf')
    return original_bits / compressed_bits


def energy_saving(original_bits, compressed_bits, k=BOLTZMANN_K, T=300):
    """Calcula ahorro energético según Landauer."""
    E_original = original_bits * k * T * math.log(2)
    E_compressed = compressed_bits * k * T * math.log(3)
    if E_original == 0:
        return 0
    return 1 - (E_compressed / E_original)
