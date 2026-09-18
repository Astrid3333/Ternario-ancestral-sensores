"""
evaluate.py — Verificación de resultados: Octave vs Python
Compara los resultados de run_all.m con la réplica Python
"""
import json
import sys
import os
import math
import numpy as np

# Agregar directorio actual al path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from data import generate_sensor_data
from model import ternary_encode, residual_compressor, quipu_encode, quipu_decode, benchmark


def load_octave_results(path="results.json"):
    """Carga resultados de Octave"""
    with open(path, 'r') as f:
        return json.load(f)


def run_python_experiment():
    """Ejecuta el experimento completo en Python"""
    temp = generate_sensor_data()
    trits = ternary_encode(temp)

    n_cold = int(np.sum(trits == -1))
    n_normal = int(np.sum(trits == 0))
    n_hot = int(np.sum(trits == 1))

    res = residual_compressor(trits)

    n_blocks = len(trits) // 5
    data5 = trits[:n_blocks * 5]
    enc = quipu_encode(data5)
    dec = quipu_decode(enc['codeword'])

    bench = benchmark(temp)

    kB = 1.380649e-23
    T = 300.0

    return {
        'n_samples': len(temp),
        'temperature_range': [float(temp.min()), float(temp.max())],
        'ternary': {
            'n_trits': len(trits),
            'distribution': [n_cold, n_normal, n_hot],
        },
        'residual': {
            'n_blocks': res['n_blocks'],
            'cycles_detected': res['cycles_detected'],
            'compression_ratio': res['compression_ratio'],
            'unique_residues': len(np.unique(res['residues'])),
        },
        'quipu': {
            'n_blocks': enc['n_blocks'],
            'total_codeword': len(enc['codeword']),
            'valid': dec['valid'],
        },
        'benchmark': {
            'binary_bits': bench['binary_bits'],
            'ternary_bits': bench['ternary_bits'],
            'ternary_quipu_bits': bench['ternary_quipu_bits'],
            'delta_bits': bench['delta_bits'],
            'gzip_bits': bench['gzip_bits'],
            'energy_binary_J': bench['energy_binary'],
            'energy_ternary_J': bench['energy_ternary'],
            'energy_saving_pct': bench['energy_saving'] * 100,
        },
        'landauer': {
            'E_binary_J': kB * T * math.log(2),
            'E_ternary_J': kB * T * math.log(3),
            'efficiency_ratio': math.log(3) / math.log(2),
            'ln3_over_3': math.log(3) / 3,
            'ln2_over_2': math.log(2) / 2,
            'advantage_pct': (math.log(3) / 3 - math.log(2) / 2) / (math.log(2) / 2) * 100,
        },
    }


def compare_values(octave_val, python_val, name, tol=1e-3):
    """Compara un valor entre Octave y Python"""
    if isinstance(octave_val, bool):
        match = octave_val == python_val
    elif isinstance(octave_val, (int, float, np.floating)):
        match = abs(float(octave_val) - float(python_val)) < tol
    elif isinstance(octave_val, list):
        if isinstance(python_val, np.ndarray):
            python_val = python_val.tolist()
        match = len(octave_val) == len(python_val)
        if match:
            for i, (o, p) in enumerate(zip(octave_val, python_val)):
                if not compare_values(o, p, f"{name}[{i}]", tol=tol):
                    match = False
                    break
    else:
        match = str(octave_val) == str(python_val)

    status = "OK" if match else "FAIL"
    if not match:
        print(f"  {status}: {name} — Octave={octave_val}, Python={python_val}")
    return match


def main():
    print("=== EVALUATE: Octave vs Python ===\n")

    # Cargar resultados Octave
    octave_path = os.path.join(os.path.dirname(__file__), "results.json")
    if not os.path.exists(octave_path):
        # Intentar en directorio padre
        octave_path = "results.json"

    try:
        octave = load_octave_results(octave_path)
        print(f"[OK] Resultados Octave cargados desde {octave_path}")
    except FileNotFoundError:
        print(f"[WARN] No se encontró results.json, usando solo Python")
        octave = None

    # Ejecutar Python
    python = run_python_experiment()
    print(f"[OK] Experimento Python completado\n")

    if octave is None:
        print("Solo resultados Python disponibles:")
        print(json.dumps(python, indent=2))
        return

    # Comparaciones
    checks = []
    total = 0

    def check(name, o_val, p_val, tol=1e-3):
        nonlocal total
        total += 1
        ok = compare_values(o_val, p_val, name, tol=tol)
        checks.append((name, ok))
        return ok

    # Verificar que ambos son 'measured'
    check("mode", octave.get("mode"), "measured")

    # Datos
    check("n_samples", octave["n_samples"], python["n_samples"])
    check("temp_range_min", octave["temperature_range"][0], python["temperature_range"][0], tol=0.1)
    check("temp_range_max", octave["temperature_range"][1], python["temperature_range"][1], tol=0.1)

    # Ternario
    check("n_trits", octave["ternary"]["n_trits"], python["ternary"]["n_trits"])
    check("distribution", octave["ternary"]["distribution"], python["ternary"]["distribution"])

    # Residual
    check("n_blocks", octave["residual"]["n_blocks"], python["residual"]["n_blocks"])
    check("cycles_detected", octave["residual"]["cycles_detected"], python["residual"]["cycles_detected"])
    check("compression_ratio", octave["residual"]["compression_ratio"], python["residual"]["compression_ratio"])
    check("unique_residues", octave["residual"]["unique_residues"], python["residual"]["unique_residues"])

    # Quipu
    check("quipu_blocks", octave["quipu"]["n_blocks"], python["quipu"]["n_blocks"])
    check("quipu_codeword_len", octave["quipu"]["total_codeword"], python["quipu"]["total_codeword"])
    check("quipu_valid", octave["quipu"]["valid"], python["quipu"]["valid"])

    # Benchmark
    check("binary_bits", octave["benchmark"]["binary_bits"], python["benchmark"]["binary_bits"])
    check("ternary_bits", octave["benchmark"]["ternary_bits"], python["benchmark"]["ternary_bits"], tol=1.0)
    check("energy_saving", octave["benchmark"]["energy_saving_pct"], python["benchmark"]["energy_saving_pct"], tol=0.1)

    # Landauer
    check("ln3_over_3", octave["landauer"]["ln3_over_3"], python["landauer"]["ln3_over_3"])
    check("ln2_over_2", octave["landauer"]["ln2_over_2"], python["landauer"]["ln2_over_2"])
    check("advantage_pct", octave["landauer"]["advantage_pct"], python["landauer"]["advantage_pct"])

    # Resumen
    passed = sum(1 for _, ok in checks if ok)
    failed = sum(1 for _, ok in checks if not ok)

    print(f"\n{'='*50}")
    print(f"RESULTADO: {passed}/{total} checks OK, {failed} FAIL")

    if failed == 0:
        print("TODO MEDIDO OK — Resultados Octave y Python coinciden")
    else:
        print("ALGUNAS VERIFICACIONES FALLARON")
        for name, ok in checks:
            if not ok:
                print(f"  FAIL: {name}")

    # Guardar resultado
    result = {
        'status': 'OK' if failed == 0 else 'FAIL',
        'octave_checks': total,
        'passed': passed,
        'failed': failed,
        'python_results': python,
    }

    eval_path = os.path.join(os.path.dirname(__file__), "eval_results.json")
    with open(eval_path, 'w') as f:
        json.dump(result, f, indent=2)
    print(f"\nResultados guardados en {eval_path}")


if __name__ == "__main__":
    main()
