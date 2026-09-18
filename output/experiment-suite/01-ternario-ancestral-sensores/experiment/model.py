"""
model.py — Motor Ternario Ancestral para Sensores IoT
Réplica Python del pipeline completo: codec → compresión → checksum → benchmark
"""
import numpy as np
from dataclasses import dataclass, field
from typing import List, Tuple, Optional
import math


# =============================================================================
# TERNARY CODEC
# =============================================================================

def ternary_encode(temperature: np.ndarray) -> np.ndarray:
    """Codifica temperatura a trits balanceados: -1 (frío), 0 (normal), +1 (caliente)"""
    trits = np.zeros(len(temperature), dtype=int)
    trits[temperature < 20] = -1
    trits[(temperature >= 20) & (temperature <= 25)] = 0
    trits[temperature > 25] = 1
    return trits


def ternary_to_decimal(trits: np.ndarray) -> int:
    """Convierte trits balanceados a decimal (para secuencias cortas)"""
    n = len(trits)
    val = 0
    for i in range(n):
        val += int(trits[i]) * (3 ** (n - 1 - i))
    return val


def decimal_to_ternary(decimal_val: int) -> np.ndarray:
    """Convierte decimal a trits balanceados"""
    if decimal_val == 0:
        return np.array([0])
    n = math.ceil(math.log(abs(decimal_val) + 1) / math.log(3))
    trits = np.zeros(n, dtype=int)
    remaining = decimal_val
    for i in range(n - 1, -1, -1):
        trit = (remaining + 1) % 3 - 1
        trits[i] = trit
        remaining = (remaining - trit) // 3
    return trits


# =============================================================================
# RESIDUAL COMPRESSOR
# =============================================================================

def residual_compressor(trits: np.ndarray, block_size: int = 4) -> dict:
    """
    Compresión por residuos mod-33.
    Inspiración: ciclo persa de 33 años.
    """
    n_trits = len(trits)
    n_blocks = n_trits // block_size

    if n_blocks == 0:
        return {
            'compressed': trits,
            'residues': np.array([]),
            'cycles_detected': 0,
            'compression_ratio': 1.0,
        }

    # Calcular residuos mod-33 de cada bloque
    residues = np.zeros(n_blocks, dtype=int)
    for i in range(n_blocks):
        block = trits[i * block_size:(i + 1) * block_size]
        val = 0
        for j in range(block_size):
            val += int(block[j]) * (3 ** (block_size - 1 - j))
        residues[i] = val % 33

    # Detectar ciclos
    unique_residues = np.unique(residues)
    cycle_count = 0
    cycle_info = []

    for r in unique_residues:
        positions = np.where(residues == r)[0]
        if len(positions) >= 2:
            diffs = np.diff(positions)
            if np.all(diffs == diffs[0]) and diffs[0] > 0:
                cycle_count += 1
                cycle_info.append({
                    'residue': int(r),
                    'period': int(diffs[0]),
                    'occurrences': len(positions),
                })

    # Run-length encoding ternario
    compressed = []
    i = 0
    while i < len(residues):
        current = residues[i]
        count = 1
        while i + count < len(residues) and residues[i + count] == current:
            count += 1
        compressed.extend([current, count])
        i += count

    # Ratio de compresión
    original_bits = n_trits * math.log2(3)
    compressed_bits = len(compressed) * math.log2(33)
    compression_ratio = compressed_bits / original_bits if original_bits > 0 else 1.0

    return {
        'compressed': np.array(compressed),
        'residues': residues,
        'cycles_detected': cycle_count,
        'cycle_info': cycle_info,
        'compression_ratio': compression_ratio,
        'n_blocks': n_blocks,
        'block_size': block_size,
    }


# =============================================================================
# QUIPU CHECKSUM
# =============================================================================

def _balance(raw: int) -> int:
    """0,1,2 → 0,1,-1"""
    return -1 if raw == 2 else raw


def _unbalance(balanced: int) -> int:
    """0,1,-1 → 0,1,2"""
    return 2 if balanced == -1 else balanced


def quipu_encode(data_trits: np.ndarray) -> dict:
    """
    Checksum tipo quipu: paridad aditiva + multiplicativa por bloque.
    Inspiración: quipu inca — nudos grandes (+1), pequeños (-1), sin nudo (0).
    """
    block_size = 5
    n_blocks = len(data_trits) // block_size

    checksum = []
    for b in range(n_blocks):
        block = data_trits[b * block_size:(b + 1) * block_size]
        block_raw = [_unbalance(int(t)) for t in block]

        # Paridad aditiva: p1 = -(sum) mod 3
        p1_raw = (-sum(block_raw)) % 3
        # Paridad multiplicativa: p2 = -(product) mod 3
        prod = 1
        for v in block_raw:
            prod *= v
        p2_raw = (-prod) % 3

        checksum.extend([_balance(p1_raw), _balance(p2_raw)])

    codeword = np.concatenate([
        data_trits[:n_blocks * block_size],
        np.array(checksum, dtype=int)
    ])

    return {
        'codeword': codeword,
        'checksum': np.array(checksum, dtype=int),
        'n_data': n_blocks * block_size,
        'n_checksum': len(checksum),
        'block_size': block_size,
        'n_blocks': n_blocks,
    }


def quipu_decode(codeword: np.ndarray) -> dict:
    """Verifica integridad del checksum quipu."""
    block_size = 5
    n = len(codeword)
    n_blocks = n // (block_size + 2)

    errors = []
    syndromes = []

    for b in range(n_blocks):
        # Extraer datos
        data_start = b * block_size
        block = codeword[data_start:data_start + block_size]

        # Extraer paridad (del final)
        checksum_start = n_blocks * block_size
        parity = codeword[checksum_start + b * 2:checksum_start + (b + 1) * 2]

        block_raw = [_unbalance(int(t)) for t in block]

        p1_raw = (-sum(block_raw)) % 3
        prod = 1
        for v in block_raw:
            prod *= v
        p2_raw = (-prod) % 3

        s1 = (_unbalance(int(parity[0])) - p1_raw) % 3
        s2 = (_unbalance(int(parity[1])) - p2_raw) % 3

        syndromes.append([s1, s2])
        if s1 != 0 or s2 != 0:
            errors.append(b)

    return {
        'valid': len(errors) == 0,
        'errors': errors,
        'syndromes': np.array(syndromes),
        'n_blocks': n_blocks,
        'block_size': block_size,
    }


# =============================================================================
# BENCHMARK
# =============================================================================

def benchmark(temperature: np.ndarray) -> dict:
    """Comparación de métodos de compresión."""
    kB = 1.380649e-23  # J/K
    T = 300.0  # K

    n = len(temperature)

    # Método 1: Binario (8 bits por valor)
    binary_bits = n * 8

    # Método 2: Ternario (residual)
    trits = ternary_encode(temperature)
    residual = residual_compressor(trits)
    ternary_bits = len(residual['compressed']) * math.log2(33)

    # Método 3: Ternario + quipu
    block_size = 5
    n_blocks = len(trits) // block_size
    if n_blocks > 0:
        quipu = quipu_encode(trits[:n_blocks * block_size])
        ternary_quipu_bits = len(quipu['codeword']) * math.log2(3)
    else:
        ternary_quipu_bits = ternary_bits

    # Método 4: Delta encoding
    delta = np.diff(temperature)
    delta_bits = len(delta) * 8

    # Método 5: gzip (estimación)
    gzip_ratio = 0.388  # ratio típico para datos de sensores
    gzip_bits = binary_bits * gzip_ratio

    # Energías Landauer
    energy_binary = binary_bits * kB * T * math.log(2)
    energy_ternary = ternary_bits * kB * T * math.log(3)

    return {
        'binary_bits': binary_bits,
        'ternary_bits': ternary_bits,
        'ternary_quipu_bits': ternary_quipu_bits,
        'delta_bits': delta_bits,
        'gzip_bits': gzip_bits,
        'energy_binary': energy_binary,
        'energy_ternary': energy_ternary,
        'energy_saving': 1.0 - (energy_ternary / energy_binary),
        'methods': [
            ('Binario (8-bit)', binary_bits, 1.0),
            ('Ternario (residual)', ternary_bits, ternary_bits / binary_bits),
            ('Ternario+Quipu', ternary_quipu_bits, ternary_quipu_bits / binary_bits),
            ('Delta encoding', delta_bits, delta_bits / binary_bits),
            ('gzip', gzip_bits, gzip_bits / binary_bits),
        ],
    }


# =============================================================================
# MAIN
# =============================================================================

if __name__ == "__main__":
    from data import generate_sensor_data

    print("=== MOTOR TERNARIO ANCESTRAL (Python) ===\n")

    temp = generate_sensor_data()
    print(f"Samples: {len(temp)}, Range: [{temp.min():.1f}, {temp.max():.1f}] °C")

    # Ternary codec
    trits = ternary_encode(temp)
    n_cold = int(np.sum(trits == -1))
    n_normal = int(np.sum(trits == 0))
    n_hot = int(np.sum(trits == 1))
    print(f"\nTrits: {len(trits)}, dist: cold={n_cold}, normal={n_normal}, hot={n_hot}")

    # Residual compressor
    res = residual_compressor(trits)
    print(f"Blocks: {res['n_blocks']}, Cycles: {res['cycles_detected']}, Ratio: {res['compression_ratio']:.3f}")

    # Quipu checksum
    n_blocks = len(trits) // 5
    data5 = trits[:n_blocks * 5]
    enc = quipu_encode(data5)
    dec = quipu_decode(enc['codeword'])
    print(f"Quipu: {enc['n_blocks']} blocks, valid={dec['valid']}, errors={len(dec['errors'])}")

    # Benchmark
    bench = benchmark(temp)
    print("\nBenchmark:")
    for name, bits, ratio in bench['methods']:
        print(f"  {name:22s} | {bits:7.1f} | {ratio:.3f}")
    print(f"\nEnergy saving: {bench['energy_saving']*100:.1f}%")

    # Landauer
    kB = 1.380649e-23
    T = 300.0
    E_bin = kB * T * math.log(2)
    E_ter = kB * T * math.log(3)
    print(f"\nLandauer: ln(3)/3={math.log(3)/3:.6f} vs ln(2)/2={math.log(2)/2:.6f}")
    print(f"Advantage: {(math.log(3)/3 - math.log(2)/2) / (math.log(2)/2) * 100:.1f}%")
