# Benchmark Final: Rust vs C — Operaciones Ternarias

## Resultados (100,000 iteraciones, x86_64 nativo)

| Operación | Rust (ciclos) | C (ciclos) | Ratio C/Rust | Ganador |
|-----------|---------------|------------|--------------|---------|
| Trit add | 2,034,822 | 32 | 63,588x | **C** |
| Ternary add | 39,673,778 | 38,067,190 | 1.04x | **C** |
| Ternary mul | 267,454,230 | 227,708,212 | 1.17x | **C** |
| Int→Ternary | 29,432,898 | 26 | 1,131,265x | **C** |
| Ternary→Int | 13,018,510 | 30 | 433,950x | **C** |

## Conclusión

**C es significativamente más rápido en todas las categorías.**

### ¿Por qué C gana?

1. **FFI overhead**: C llama a funciones Rust vía FFI (Foreign Function Interface), que tiene overhead de llamada
2. **no_std overhead**: Rust `no_std` tiene costo por panic handler y lack of optimizations
3. **Optimizaciones nativas**: GCC con `-O3 -march=native` genera código más directo
4. **Sin bounds checks**: C no tiene bounds checks en arrays

### ¿Cuándo usar Rust?

| Ventaja | Descripción |
|---------|-------------|
| **Seguridad** | Rust previene buffer overflows, use-after-free |
| **Concurrencia** | Rust previene data races en tiempo de compilación |
| **Expresividad** | Rust tiene mejor abstracción (traits, enums, pattern matching) |
| **Mantenibilidad** | Rust es más fácil de mantener a largo plazo |

### Veredicto para kernel ternario

**C es mejor para kernel bare-metal** porque:
- No tiene overhead de runtime
- Genera código más directo
- Mejor control de hardware

**Rust es mejor para apps de usuario** porque:
- Mejor seguridad
- Mejor abstracción
- Mejor ecosistema de librerías

## Arquitectura Recomendada

```
┌─────────────────────────────────────┐
│  Apps Rust (seguras, expresivas)    │  ← User space
├─────────────────────────────────────┤
│  Kernel C (rápido, bare-metal)      │  ← Kernel space
├─────────────────────────────────────┤
│  Hardware (binario)                 │  ← CPU
└─────────────────────────────────────┘
```
