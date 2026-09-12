# Experimento: Centro de datos nodal ternario ancestral

Simula un nodo de centro de datos con codificación ternaria nativa y compara con binario estándar.

## Ejecución

```bash
cd output/experiment-suite/02-nodo-ternario-datacenter/2026-09-12_021819
python -m experiment.train
python -m experiment.evaluate
```

## Archivos

| Archivo | Descripción |
|---------|-------------|
| `experiment/model.py` | Codec ternario, compresión residual, quipu checksum |
| `experiment/data.py` | Generación de datos sintéticos |
| `experiment/train.py` | Ejecuta experimento completo |
| `experiment/evaluate.py` | Verifica resultados |
| `experiment/config.yaml` | Parámetros del run |
| `results.json` | Resultados medidos |
| `figures/` | Figuras publication-grade |
| `experiment_design.md` | Diseño experimental |
| `data_contract.md` | Contrato de datos |
| `experiment_report.md` | Informe estructurado |

## Resultados esperados

- Compresión ternaria: ~8.3x (91.7% ahorro)
- Ahorro energético: ~86.9% vs binario
- Detección de ciclos: 100% (período 144)
- Quipu checksum: 100% errores detectados
