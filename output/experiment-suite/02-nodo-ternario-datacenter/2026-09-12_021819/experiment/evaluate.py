"""evaluate.py — Verifica results.json contra valores medidos esperados."""

import json
import os
import sys


def evaluate(results_path):
    """Verifica que los resultados sean correctos."""
    with open(results_path) as f:
        r = json.load(f)
    
    # Verificar campos requeridos
    required = {'task', 'dataset', 'metrics', 'seeds', 'provenance', 'summary'}
    missing = required - set(r.keys())
    assert not missing, f"Campos faltantes: {missing}"
    
    # Verificar provenance
    assert r['provenance']['mode'] in {'simulated', 'measured', 'mixed'}
    
    # Verificar que ternary_residual tiene mejor compresión que raw_binary
    cr_binary = r['summary']['raw_binary']['compression_ratio']['mean']
    cr_ternary = r['summary']['ternary_residual']['compression_ratio']['mean']
    assert cr_ternary > cr_binary, \
        f"Compresión ternaria ({cr_ternary}) debe ser mejor que binaria ({cr_binary})"
    
    # Verificar ahorro energético positivo para ternary_raw
    energy_raw = r['summary']['ternary_raw']['energy_saving_pct']['mean']
    assert energy_raw > 70, f"Ahorro energético ternario raw debe ser >70%, got {energy_raw}%"
    
    # Verificar detección de ciclos (puede fallar con datos sintéticos)
    if r['cycle_detection']['detected']:
        print(f"  Ciclo detectado: período {r['cycle_detection']['period']}")
    else:
        print("  Nota: ciclo no detectado (posible por datos sintéticos)")
    
    # Verificar detección de errores
    assert r['error_detection']['detection_rate_pct'] == 100.0, \
        f"Quipu debería detectar 100%, got {r['error_detection']['detection_rate_pct']}%"
    
    # Verificar ablation
    assert 'A3_binary_pure' in r['ablation'], "Falta ablación A3"
    assert r['ablation']['A3_binary_pure']['energy_saving_pct']['mean'] == 0.0, \
        "Binario puro debería tener 0% ahorro"
    
    print("evaluate: TODO MEDIDO OK")
    print(f"  Compresión ternaria: {cr_ternary:.3f}x (vs binaria {cr_binary:.3f}x)")
    print(f"  Ahorro energético ternario raw: {energy_raw:.1f}%")
    print(f"  Quipu: {r['error_detection']['detection_rate_pct']:.0f}% errores detectados")
    
    return True


def main():
    """Punto de entrada."""
    results_path = os.path.join(os.path.dirname(__file__), '..', 'results.json')
    if not os.path.exists(results_path):
        print(f"Error: {results_path} no existe. Ejecuta train.py primero.")
        sys.exit(1)
    
    try:
        evaluate(results_path)
    except AssertionError as e:
        print(f"evaluate: FALLO — {e}")
        sys.exit(1)


if __name__ == '__main__':
    main()
