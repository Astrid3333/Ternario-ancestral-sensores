# Ternary Ancestral Kernel — SO Ultra-Liviano

Sistema operativo experimental basado en matemáticas ancestrales.

## Características

| Componente | Base | Inspiración |
|------------|------|-------------|
| Lógica | Ternaria {-1, 0, +1} | Setun (1958) |
| Memoria | Base 60 | Babilónicos |
| Procesos | Ciclos de 260/365 | Mayas |
| Archivos | Posicional | Quipus incas |
| E/S | Residuos mod-33 | Persas |

## Especificaciones

- **Tamaño total:** ~4KB de código
- **Memoria:** 3.5KB (60 bloques × 60 bytes)
- **Procesos:** Máximo 33 (mod-33)
- **Archivos:** Máximo 33
- **Arquitectura:** x86 (i386)

## Compilación

### Requisitos
```bash
# Ubuntu/Debian
sudo apt install nasm gcc-i686-linux-gnu binutils-i686-linux-gnu qemu-system-x86

# Arch Linux
sudo pacman -S nasm i686-elf-gcc qemu-system-x86
```

### Build
```bash
make all        # Crear ISO
make run        # Ejecutar en QEMU
make debug      # Ejecutar con GDB
make info       # Mostrar información
```

## Uso

### Comandos del kernel
```
ps    - Listar procesos
mem   - Estado de memoria
fs    - Listar archivos
cal   - Calendario Maya
halt  - Apagar
```

### Teclas especiales
- `Ctrl+Alt+Del` → Reiniciar
- `Ctrl+C` → Matar proceso actual

## Arquitectura

```
┌─────────────────────────────────────────────┐
│            KERNEL TERNARIO                  │
├─────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐          │
│  │  Scheduler  │  │   Memory    │          │
│  │ (Maya 260)  │  │  (Base 60)  │          │
│  └──────┬──────┘  └──────┬──────┘          │
│         │                │                  │
│  ┌──────▼────────────────▼──────┐          │
│  │       Ternary Logic Unit     │          │
│  │      {-1, 0, +1} operations  │          │
│  └──────────────────────────────┘          │
│                    │                        │
│  ┌─────────────────▼─────────────────┐    │
│  │         Quipu File System         │    │
│  │    (Positional knot encoding)     │    │
│  └───────────────────────────────────┘    │
└─────────────────────────────────────────────┘
```

## Memoria

60 bloques de 60 bytes cada uno:

| Bloque | Color | Uso |
|--------|-------|-----|
| 0-4 | 1 (kernel) | Kernel code + data |
| 5-10 | 2 (process) | Procesos activos |
| 11-20 | 3 (stack) | Pilas de procesos |
| 21-50 | 4 (code) | Código de usuario |
| 51-59 | 0 (free) | Libre |

## Procesos

Cada proceso tiene:
- **PID Maya:** {Tzolkin: 1-260, Haab: 1-365}
- **Prioridad ternaria:** -1 (baja), 0 (media), +1 (alta)
- **Quantum:** 3 ciclos de CPU
- **Memoria:** 1 bloque asignado

## Scheduler

Planificación round-robin ternaria:
1. Seleccionar proceso con mayor prioridad
2. Ejecutar quantum (3 ciclos)
3. Si quantum completado → context switch
4. Actualizar calendario Maya

## Archivos (Quipu)

Sistema de archivos posicional:
- **Nombre:** 8 caracteres
- **Tamaño:** En bloques
- **Tipo:** 0=archivo, 1=directorio, 2=ejecutable
- **Permisos:** Bits (rwx)

## Demo

```c
// Crear proceso
int8_t pid = sched_create(0, 1);  // Alta prioridad

// Asignar memoria
int8_t block = mem_alloc(pid, 2);  // Bloque de proceso

// Crear archivo
fs_create("test.bin", 2);  // Ejecutable

// Ejecutar
while (1) {
    sched_tick();
    asm volatile("hlt");
}
```

## Limitaciones

- Solo modo real (16 bits)
- Sin multitarea real (cooperative)
- Sin manejo de memoria virtual
- Sin drivers completos
- Solo I/O por puertos

## Futuro

- [ ] Multitarea preemptiva
- [ ] Memoria virtual en base 60
- [ ] Drivers de disco
- [ ] Sistema de archivos completo
- [ ] Shell ternaria
- [ ] Compilador ternario

## Licencia

MIT
