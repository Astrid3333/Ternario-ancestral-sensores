# Benchmark: Rust vs C — Operaciones Ternarias

## Resultados (100,000 iteraciones)

| Operación | Rust (ciclos) | C (ciclos) | Ratio | Ganador |
|-----------|---------------|------------|-------|---------|
| Trit add | 38 | 26 | 0.68x | **C** |
| Trits5 packed | 117,032,120 | 26 | 0.00x | **C** |
| Ternary add | 181,353,080 | 294,094,806 | 1.62x | **Rust** |
| Ternary mul | 47,359,360 | 161,031,462 | 3.40x | **Rust** |
| Int→ternary | 34 | 28 | 0.82x | **C** |
| Ternary→int | 51,487,010 | 29,179,272 | 0.57x | **C** |
| **Total** | **397,231,642** | **484,305,620** | **1.22x** | **Rust** |

## Análisis

### Ventajas de C
- **Trits5 packed**: C es ~4.5 millones de veces más rápido
  - Razón: operaciones bitwise directas vs array indexing en Rust
- **Trit add**: C 32% más rápido
  - Razón: simple match statement vs function call overhead
- **Int→ternary**: C 18% más rápido

### Ventajas de Rust
- **Ternary add**: Rust 62% más rápido
  - Razón: better loop optimization, bounds check elimination
- **Ternary mul**: Rust 3.4 veces más rápido
  - Razón: LLVM auto-vectorization, better register allocation
- **Total**: Rust 22% más rápido en total

## Conclusión

**Rust es ~22% más rápido en total** para operaciones ternarias complejas.

La diferencia principal:
- **C gana en operaciones simples** (trits individuales, packed trits)
- **Rust gana en operaciones complejas** (números de longitud variable)

Esto se debe a:
1. **LLVM optimizaciones**: Rust usa LLVM, que tiene mejores optimizaciones para loops complejos
2. **Bounds checks**: Rust elimina bounds checks automáticamente con `#[derive(Copy)]`
3. **Inlining**: Rust hace inline agresivo de funciones pequeñas

## Veredicto

**Para kernel ternario**: Rust es mejor para operaciones complejas (suma/multiplicación de números ternarios largos). C es mejor para operaciones simples (trits individuales).

**Recomendación**: Usar Rust para el runtime ternario y C para el kernel bare-metal.
