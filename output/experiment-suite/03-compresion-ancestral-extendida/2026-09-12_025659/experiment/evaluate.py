"""evaluate.py — Verifica resultados de compresión ancestral extendida."""

import json
import os
import sys


def evaluate(results_path):
    """Verifica que los resultados sean correctos."""
    with open(results_path) as f:
        r = json.load(f)
    
    # Verificar campos requeridos
    required = {'task', 'dataset', 'metrics', 'seeds', 'provenance', 'summary', 'quality'}
    missing = required - set(r.keys())
    assert not missing, f"Campos faltantes: {missing}"
    
    # Verificar calidad: TODAS las codificaciones deben ser exactas
    q = r['quality']
    assert q['all_exact'], "TODAS las codificaciones deben ser exactas (0 errores)"
    
    # Verificar que babilónico tiene mejor ratio que maya
    cr_maya = r['summary']['maya_vigesimal']['compression_ratio']['mean']
    cr_bab = r['summary']['babilonico_sexagesimal']['compression_ratio']['mean']
    assert cr_bab > cr_maya, f"Babilónico ({cr_bab}) debe ser mejor que Maya ({cr_maya})"
    
    # Verificar que full ancestral supera 3x
    cr_full = r['summary']['full_ancestral']['compression_ratio']['mean']
    assert cr_full > 3.0, f"Full ancestral debe superar 3x, got {cr_full}"
    
    # Verificar que maya supera 2x
    assert cr_maya > 2.0, f"Maya debe superar 2x, got {cr_maya}"
    
    print("evaluate: TODO MEDIDO OK")
    print(f"  Maya vigesimal:      {cr_maya:.3f}x")
    print(f"  Babilónico sexag.:   {cr_bab:.3f}x")
    print(f"  Full ancestral:      {cr_full:.3f}x")
    print(f"  Calidad:             {'✓ TODAS EXACTAS' if q['all_exact'] else '✗ FALLO'}")
    
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
