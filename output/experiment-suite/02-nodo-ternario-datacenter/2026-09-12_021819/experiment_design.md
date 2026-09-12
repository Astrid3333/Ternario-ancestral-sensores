# Diseño experimental: Centro de datos nodal ternario ancestral

## 1. Pregunta de investigación e hipótesis

**Pregunta:** ¿Puede una arquitectura de centro de datos nodal con codificación ternaria nativa reducir el consumo de energía y los requerimientos de almacenamiento en comparación con un centro de datos binario estándar, manteniendo la integridad de datos y la capacidad de detección de patrones?

**Hipótesis H1 (almacenamiento):** Un nodo ternario almacena datos de sensores de 3 estados con 33% menos bits que un nodo binario equivalente, manteniendo la misma información útil.

**Hipótesis H2 (energía):** Un nodo ternario consume 86.9% menos energía por operación de borrado de símbolo que un nodo binario, según el límite de Landauer (E = kT·ln(B), B=3 vs B=2).

**Hipótesis H3 (transmisión):** Un nodo ternario con compresión residual mod-33 reduce el tráfico de red en 91.7% comparado con transmisión binaria estándar, para datos de sensores de temperatura con ciclo diario.

**Hipótesis H4 (integridad):** El checksum quipu (paridad ternaria) detecta el 100% de errores de 1 trit en bloques de 5 trits, con tasa de sobrecarga k/n = 0.714, comparable a Hamming binario (7/12 = 0.583).

**Hipótesis H5 (detección de patrones):** Los residuos mod-33 del calendario persa detectan ciclos diarios en datos de sensores con 100% de precisión, sin algoritmos de aprendizaje automático.

## 2. Datasets

### 2.1 Dataset principal: datos de sensores sintéticos

- **Nombre:** SensorDataTernary-v1
- **Fuente:** Generado sintéticamente en sesión (ver `data_contract.md`)
- **Ruta de acceso:** `experiment/sensor_data.csv`
- **Tamaño:** 1006 muestras de temperatura (7 días, cada 10 minutos)
- **Estructura:** `timestamp, temperature, humidity, light` (simulados con ciclo diario ±5°C, ruido gaussiano σ=0.5°C)
- **Por qué este dataset:** Permite control total sobre las propiedades estadísticas (ciclos, ruido, rango) y reproducción exacta en Octave y Python. Los datos reales de sensores IoT tienen estructura similar (ciclos diarios, estacionalidad).
- **Pre-procesamiento:** Conversión a trits balanceados con umbrales configurables (<20°C → -1, 20-25°C → 0, >25°C → +1)
- **Limitaciones:** Datos sintéticos; no capturan fallos de hardware, interferencia RF, o patrones de carga reales de una red de datos.

### 2.2 Dataset de validación: datos de sensores reales (OpenWeatherMap)

- **Nombre:** OpenWeatherHourly-v1
- **Fuente:** API pública OpenWeatherMap (datos horarios de temperatura de Santiago, Chile)
- **Ruta de acceso:** `experiment/weather_data.csv`
- **Tamaño:** 720 muestras (30 días, horarias)
- **Por qué este dataset:** Valida que los resultados sintéticos se generalizan a datos reales con ciclos diarios y variabilidad estacional.
- **Limitaciones:** Datos de una sola ciudad; no representan condiciones extremas o tropicales.

## 3. Baselines

### 3.1 Baselines inferiores (lower-bound)

| Baseline | Descripción | Por qué |
|----------|-------------|---------|
| Raw binary | Datos sin compresión, 8 bits/muestra | Referencia de peor caso |
| Run-length binary | Compresión run-length estándar en binario | Comparación con método clásico simple |

### 3.2 Mismo paradigma

| Baseline | Descripción | Por qué |
|----------|-------------|---------|
| gzip | Compresión estándar DEFLATE | Estándar de la industria |
| ternary raw | Ternario sin compresión residual | Aislamiento del efecto de la codificación |
| ternary + gzip | Datos ternarios comprimidos con gzip | Híbrido para ver si gzip supera al residual |

### 3.3 Paradigma adyacente

| Baseline | Descripción | Por qué |
|----------|-------------|---------|
| LZ4 | Compresión rápida, baja latencia | Usado en sistemas de streaming |
| ZSTD | Compresión moderna, ajustable | Usado en Meta/Netflix para datos masivos |

### 3.4 Presupuesto por baseline

Cada baseline recibe los mismos 1006 puntos de entrada. No hay búsqueda de hiperparámetros para compresión (determinista). Para detección de ciclos, se comparan algoritmos de O(n²) vs O(n log n).

## 4. Métricas

| Métrica | Unidades | Dirección | Por qué |
|---------|----------|-----------|---------|
| Ratio de compresión | bits_originales / bits_comprimidos | Mayor mejor | Eficiencia de almacenamiento |
| Ahorro energético | % vs binario estándar | Mayor mejor | Sostenibilidad del centro de datos |
| Precisión de detección de ciclos | % de ciclos correctos | Mayor mejor | Capacidad analítica del nodo |
| Tasa de detección de errores | % de errores detectados | Mayor mejor | Integridad de datos |
| Latencia de codificación | μs por muestra | Menor mejor | Viabilidad en tiempo real |
| Uso de memoria | bytes por 1000 muestras | Menor mejor | Restricción de hardware IoT |

## 5. Protocolo

- **Semillas:** 5 semillas para generación de datos sintéticos (reproducibilidad)
- **Hardware:** Intel Celeron J4025, 2 cores, 11 GB RAM (laptop estándar)
- **Framework:** GNU Octave (código principal) + Python 3.12 (réplica)
- **Verificación:** Cada resultado se verifica en ambos motores (Octave + Python)
- **Cross-validation:** No aplica (determinista, no hay aprendizaje automático)

## 6. Ablaciones

| Ablación | Qué elimina | Efecto esperado | Por qué prueba la hipótesis |
|----------|-------------|-----------------|---------------------------|
| A1: Sin compresión residual | Solo ternario raw | Compresión baja (~33%) | Demuestra que la compresión residual es esencial para H3 |
| A2: Sin checksum quipu | Sin verificación de errores | Ahorro de 28.6% en sobrecarga | Demuestra el costo de la integridad (H4) |
| A3: Binario puro | Sin ternario | Referencia base | Demuestra la ventaja del ternario (H1, H2) |
| A4: Sin detección de ciclos | Solo compresión | Pierde capacidad analítica | Demuestra el valor de los residuos mod-33 (H5) |
| A5: Ternario con Run-Length | Compresión Run-Length en vez de residual | Compresión diferente | Compara estrategias de compresión |

## 7. Riesgos y limitaciones

- **Datos sintéticos:** Los resultados se validan con OpenWeatherMap real, pero no cubren condiciones de red real (latencia, pérdida de paquetes, colisiones).
- **Hardware limitado:** El experimento corre en laptop; no mide el consumo real de un nodo Raspberry Pi o ESP32.
- **Escalabilidad:** El experimento simula 1 nodo con 1006 muestras; no modela 1000 nodos en red con tráfico concurrente.
- **Compresión ternaria:** Solo supera a binario cuando el dato tiene ≤3 estados naturalmente; para datos continuos (temperatura con decimales), la ventaja disminuye.
- **Energía Landauer:** Es un límite termodinámico teórico; la implementación real gasta más por overhead de control.

## 8. Presupuesto de cómputo

| Item | Costo por config | Total |
|------|-----------------|-------|
| Generación de datos (5 semillas) | 1 min × 5 | 5 min |
| Codificación + compresión (7 métodos) | 2 min × 7 | 14 min |
| Detección de ciclos | 5 min | 5 min |
| Figuras (6 figuras) | 3 min × 6 | 18 min |
| **Total** | | **~42 min** |

Todo el experimento corre en < 1 hora en laptop sin GPU.
