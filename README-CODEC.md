# Tritos Codec — Codec Ternario para IoT

> Codec para datos con 3 estados naturales (sensores IoT). No es un compresor de propósito general.

## Resumen

Tritos Codec es un formato de codificación diseñado para datos que naturalmente tienen 3 estados:
- Sensores de temperatura (bajo/normal/alto)
- Sensores de humedad (seco/óptimo/saturado)
- Estados de máquina (apagado/encendido/error)

**No es un compresor universal.** Para datos binarios puros, expande ~4x. Para datos ternarios, ahorra ~33%.

## Formato TRC5

```
┌─────────┬─────────────┬──────────────┐
│ Header  │ Trits Pack  │ Checksum     │
│ 4 bytes │ N bytes     │ 2 bytes      │
└─────────┴─────────────┴──────────────┘
```

### Header (4 bytes)
- `TRC5` — Identificador
- Versión: 1 byte
- Cantidad de trits: 3 bytes

### Empaquetado de Trits

5 trits caben en 1 byte (3^5 = 243 < 256):

```
Byte: [t4 t3 t2 t1 t0]

Valor = t4×81 + t3×27 + t2×9 + t1×3 + t0×1
```

Ejemplo:
```
Trits: +1  0  -1  +1  0
Valor: 1×81 + 0×27 + (-1)×9 + 1×3 + 0×1 = 75
Byte:  0x4B
```

## Métricas Honestas

| Tipo de datos | Ratio real | Nota |
|---|---|---|
| Datos ternarios puros | 0.6x (ahorro 33%) | 5 trits/byte vs 5 bytes |
| Mezcla ternario/binario | 1.0-1.5x | Depende de proporción |
| Datos binarios puros | 4.0x (expande) | **No usar** |

### Ejemplo real: Sensor IoT

```python
# Sensor de temperatura: 0=bajo, 1=normal, 2=alto
datos = [0, 1, 1, 2, 0, 1, 1, 1, 0, 2, 1, 0]  # 12 trits

# Codificación ternaria: 12 trits / 5 = 3 bytes (redondeo)
# Sin codec: 12 bytes
# Con codec: 3 bytes
# Ahorro: 75%
```

## Cuándo usar

| Caso de uso | ¿Usar Tritos Codec? |
|---|---|
| Sensores IoT de 3 estados | ✓ Sí |
| Datos binarios (texto, imágenes) | ✗ No |
| Mezcla de datos | ⚠ Depende |
| Compresión general | ✗ No |

## Instalación

```bash
# Usar el codec
python3 tritos_compress.py encode input.ternary output.trc5
python3 tritos_compress.py decode output.trc5 output.ternary
```

## API Python

```python
from tritos_compress import TritosCodec

codec = TritosCodec()

# Codificar datos ternarios
datos = [0, 1, 2, 1, 0, 2, 1, 1, 0]  # Lista de 0, 1, 2
codificado = codec.encode(datos)

# Decodificar
decodificado = codec.decode(codificado)
```

## Limitaciones

1. **No comprime datos binarios** — expande ~4x
2. **Requiere datos de 3 estados** — no funciona con datos de 2 estados
3. **Formato simple** — sin Huffman, sin LZ77

## Futuro

- [ ] Empaquetado 5/byte eficiente
- [ ] Soporte para carpetas (uca.xml)
- [ ] Benchmark con sensores reales
- [ ] Integración con Arduino

## Comparación con otros codecs

| Codec | Tipo | Datos ternarios | Datos binarios |
|---|---|---|---|
| **Tritos Codec** | Ternario | 0.6x | 4.0x |
| gzip | Binario | 1.2x | 0.3x |
| LZ4 | Binario | 1.5x | 0.5x |
| Base64 | Encoding | 1.3x | 1.3x |

**Tritos Codec es mejor que gzip SOLO para datos ternarios puros.**

## Licencia

MIT

## Autor

Astrid3333 — https://github.com/Astrid3333
