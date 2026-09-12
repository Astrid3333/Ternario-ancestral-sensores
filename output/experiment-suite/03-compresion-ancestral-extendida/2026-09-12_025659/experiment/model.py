"""model.py — Compresión ancestral extendida: Maya, Persa, Babilónico, Quipu."""

import numpy as np
from collections import OrderedDict


# =============================================================================
# BASE CONVERSION — Por chunks (evita overflow)
# =============================================================================

def trits_to_base_chunks(trits, base):
    """Convierte trits a dígitos en base dada, procesando por chunks."""
    # Calcular cuántos trits caben en un dígito de esta base
    # Máximo valor en base b es b-1, que equivale a log3(b) trits
    import math
    max_trits_per_digit = max(1, int(math.log(base) / math.log(3)))
    
    digits = []
    i = 0
    while i < len(trits):
        # Tomar chunk de max_trits_per_digit trits
        chunk = trits[i:i + max_trits_per_digit]
        
        # Convertir chunk a valor entero (base 3)
        value = 0
        for t in chunk:
            value = value * 3 + (t + 1)  # mapear {-1,0,1} a {0,1,2}
        
        # Si el valor es mayor que la base, reducir el chunk
        while value >= base and len(chunk) > 1:
            chunk = chunk[:-1]
            value = 0
            for t in chunk:
                value = value * 3 + (t + 1)
        
        digits.append(value)
        i += len(chunk)
    
    return digits


def base_chunks_to_trits(digits, base, n_trits):
    """Convierte dígitos en base dada a trits."""
    import math
    max_trits_per_digit = max(1, int(math.log(base) / math.log(3)))
    
    trits = []
    for digit in digits:
        # Convertir dígito a trits (base 3)
        value = digit
        chunk_trits = []
        while value > 0:
            value, remainder = divmod(value, 3)
            chunk_trits.append(remainder - 1)  # 0→-1, 1→0, 2→1
        
        # Rellenar a max_trits_per_digit si es necesario
        while len(chunk_trits) < max_trits_per_digit:
            chunk_trits.append(-1)
        
        # Invertir y agregar
        trits.extend(reversed(chunk_trits))
    
    return trits[:n_trits]


# =============================================================================
# MAYA VIGESIMAL (Base 20)
# =============================================================================

def maya_encode(trits):
    """Codifica trits en base 20 (maya vigesimal)."""
    return trits_to_base_chunks(trits, 20)


def maya_decode(digits, n_trits):
    """Decodifica base 20 a trits."""
    return base_chunks_to_trits(digits, 20, n_trits)


# =============================================================================
# PERSA CICLO-33 (Base 33)
# =============================================================================

def persa_encode(trits):
    """Codifica trits en base 33 (persa ciclo-33)."""
    return trits_to_base_chunks(trits, 33)


def persa_decode(digits, n_trits):
    """Decodifica base 33 a trits."""
    return base_chunks_to_trits(digits, 33, n_trits)


# =============================================================================
# BABILÓNICO SEXAGESIMAL (Base 60)
# =============================================================================

def babilonico_encode(trits):
    """Codifica trits en base 60 (babilónico sexagesimal)."""
    return trits_to_base_chunks(trits, 60)


def babilonico_decode(digits, n_trits):
    """Decodifica base 60 a trits."""
    return base_chunks_to_trits(digits, 60, n_trits)


# =============================================================================
# RESIDUAL MOD-33 (Persa)
# =============================================================================

def block_residue(block, modulus=33):
    """Calcula residuo mod-33 de un bloque de trits."""
    value = 0
    for t in block:
        value = value * 3 + (t + 1)
    return value % modulus


def compress_residual(trits, block_size=4):
    """Comprime trits usando residuos mod-33 con run-length."""
    compressed = []
    i = 0
    while i < len(trits):
        block = trits[i:i + block_size]
        if len(block) < block_size:
            block = list(block) + [0] * (block_size - len(block))
        residue = block_residue(block)
        
        # Run-length
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
        i += count * block_size
    
    return compressed


# =============================================================================
# QUIPU CHECKSUM — Paridad ternaria
# =============================================================================

def quipu_encode(block):
    """Genera checksum quipu para bloque de 5 trits."""
    p1 = -(sum(block)) % 3  # aditivo
    product = 1
    for t in block:
        product *= (t + 2)  # mapear {-1,0,1} a {1,2,3}
    p2 = -(product) % 3     # multiplicativo
    
    # A trits balanceados
    p1_bal = p1 if p1 <= 1 else p1 - 3
    p2_bal = p2 if p2 <= 1 else p2 - 3
    return p1_bal, p2_bal


def quipu_verify(block, p1, p2):
    """Verifica integridad de bloque."""
    p1_calc, p2_calc = quipu_encode(block)
    return p1_calc == p1 and p2_calc == p2


def quipu_checksum_full(data, block_size=5):
    """Calcula checksum completo."""
    checksum = []
    for i in range(0, len(data), block_size):
        block = data[i:i + block_size]
        if len(block) < block_size:
            block = list(block) + [0] * (block_size - len(block))
        p1, p2 = quipu_encode(block)
        checksum.extend([p1, p2])
    return checksum


# =============================================================================
# PIPELINES COMBINADOS
# =============================================================================

def pipeline_maya_residual(trits):
    """Maya + residual mod-33."""
    maya_digits = maya_encode(trits)
    compressed = compress_residual(trits)
    
    return {
        'maya_digits': maya_digits,
        'n_maya': len(maya_digits),
        'compressed': compressed,
        'n_compressed': len(compressed),
        'total_bytes': len(maya_digits) + len(compressed)
    }


def pipeline_babilonico_residual(trits):
    """Babilónico + residual mod-33."""
    babilonico_digits = babilonico_encode(trits)
    compressed = compress_residual(trits)
    
    return {
        'babilonico_digits': babilonico_digits,
        'n_babilonico': len(babilonico_digits),
        'compressed': compressed,
        'n_compressed': len(compressed),
        'total_bytes': len(babilonico_digits) + len(compressed)
    }


def pipeline_full_ancestral(trits):
    """Babilónico + residual + quipu (máxima compresión)."""
    babilonico_digits = babilonico_encode(trits)
    compressed = compress_residual(trits)
    checksum = quipu_checksum_full(trits)
    
    return {
        'babilonico_digits': babilonico_digits,
        'n_babilonico': len(babilonico_digits),
        'compressed': compressed,
        'n_compressed': len(compressed),
        'checksum': checksum,
        'n_checksum': len(checksum),
        'total_bytes': len(babilonico_digits) + len(compressed) + len(checksum)
    }


# =============================================================================
# MÉTRICAS
# =============================================================================

def compression_ratio(original_trits, compressed_bytes):
    """Calcula ratio de compresión (trits originales / bytes comprimidos)."""
    if compressed_bytes == 0:
        return float('inf')
    return original_trits / compressed_bytes


def verify_decode(original_trits, decoded_trits):
    """Verifica decodificación exacta."""
    return original_trits == decoded_trits


# =============================================================================
# SENSOR A TRIT
# =============================================================================

def sensor_to_trit(value, low=20.0, high=25.0):
    """Convierte valor de sensor a trit balanceado."""
    if value < low:
        return -1
    elif value > high:
        return 1
    else:
        return 0
