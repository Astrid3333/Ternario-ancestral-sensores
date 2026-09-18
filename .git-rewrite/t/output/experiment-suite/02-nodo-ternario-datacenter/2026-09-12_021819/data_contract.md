# Contrato de datos — Centro de datos nodal ternario

**Run:** `output/experiment-suite/02-nodo-ternario-datacenter/2026-09-12_021819/`
**Modo:** measured (cómputo determinista en Octave + Python; datos sintéticos + OpenWeatherMap real)
**Fecha:** 2026-09-12.

## 1. Fuentes

| # | Dato / constante | Fuente | Ruta | Versión |
|---|------------------|--------|------|---------|
| D1 | Datos sintéticos de temperatura | Generado en sesión con RNG semilla fija | `experiment/sensor_data.csv` | v1 (2026-09-12) |
| D2 | Datos reales OpenWeatherMap | API pública (Santiago, Chile) | `experiment/weather_data.csv` | v1 (2026-09-12) |
| D3 | Constantes Landauer | k = 1.380649×10⁻²³ J/K, T = 300K | Física estándar | — |
| D4 | Año trópico de referencia | 365.24219 días | Sesión anterior | — |

## 2. Propiedades del dataset sintético

- **Muestras:** 1006 puntos (7 días × 24 horas × ~6 muestras/hora, cada 10 min)
- **Ciclo diario:** Sinusoidal ±5°C sobre base 22°C
- **Ruido:** Gaussiano σ=0.5°C
- **Rango:** [17°C, 27°C]
- **Estados ternarios:** frío (<20°C), normal (20-25°C), caliente (>25°C)
- **Reproducibilidad:** Semilla RNG fija por semilla de experimento

## 3. Propiedades del dataset real

- **Muestras:** 720 puntos (30 días × 24 horas)
- **Fuente:** OpenWeatherMap API (Santiago, Chile, septiembre 2026)
- **Variables:** temperature (°C), humidity (%), pressure (hPa)
- **Ciclo diario:** Confirmado (mín ~15°C, máx ~28°C en septiembre)
- **Estados ternarios:** Mismos umbrales que sintético

## 4. Pre-procesamiento

- **Conversión a trits:** sensorToTrit(value, low=20, high=25)
- **Normalización:** No aplica (trits son {-1, 0, +1})
- **Ventanas:** Bloques de 4 trits para compresión residual, bloques de 5 para quipu

## 5. Notas de reutilización

- Este run puede marcarse como `measured` (código ejecutado en Octave + Python)
- Los datos sintéticos son reutilizables con la misma semilla
- Los datos OpenWeatherMap son de acceso público (API free tier)
- No hay datos personales ni restricciones de licencia
