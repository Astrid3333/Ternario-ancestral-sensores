# Diseño experimental: Compresión ancestral extendida en trits

## 1. Pregunta e hipótesis

**Pregunta:** ¿Pueden los sistemas numéricos ancestrales de mayor radix (maya vigesimal, persa ciclo-33, babilónico sexagesimal) comprimir datos ternarios con mejor ratio que la codificación decimal estándar, manteniendo calidad perfecta (0 errores)?

**H1:** Un bloque de 6 trits (729 estados) se codifica en 2 símbolos mayas (400 estados) con compresión 3x y decodificación exacta.

**H2:** Un bloque de 10 trits (59049 estados) se codifica en 3 símbolos babilónicos (216000 estados) con compresión 3.3x y decodificación exacta.

**H3:** La compresión ancestral combinada (babilónico + residuo persa + quipu) supera 5x de ratio total, manteniendo detección de errores al 100%.

## 2. Métodos comparados

| Método | Base | BlocKEE (trits) | Símbolos | Ratio | Decodificación |
|--------|------|-----------------|----------|-------|----------------|
| Ternario raw | 3 | 1 | 1 | 1x | exacta |
| Decimal (quipu) | 10 | 2 | 1 | 2x | exacta |
| Maya vigesimal | 20 | 3 | 1 | 3x | exacta |
| Persa ciclo-33 | 33 | 3 | 1 | 3.3x | exacta |
| Babilónico sexagesimal | 60 | 4 | 1 | 4x | exacta |
| Maya + residual | 20 | 3 | +residuo | 3.5x | exacta |
| Babilónico + residual | 60 | 4 | +residuo | 5x | exacta |
| Full ancestral | 60 | 4 | +residuo+quipu | 4.2x | exacta |

## 3. Dataset

- 1006 muestras de temperatura sintética (7 días, ciclo diario)
- Estados ternarios: frío(-1), normal(0), caliente(+1)
- Validación: 720 muestras OpenWeatherMap Santiago

## 4. Métricas

| Métrica | Dirección | Por qué |
|---------|-----------|---------|
| Ratio de compresión | Mayor mejor | Eficiencia de almacenamiento |
| Calidad de decodificación | 100% exacta | Integridad de datos |
| Latencia de codificación | Menor mejor | Viabilidad en tiempo real |
| Uso de memoria | Menor mejor | Restricción IoT |
| Complejidad algorítmica | O(n) preferible | Escalabilidad |

## 5. Protocolo

- 5 semillas para reproducibilidad
- Cada método se verifica con decodificación exacta
- Código en Python puro (sin dependencias externas)
- Figuras publication-grade (PDF + SVG)
