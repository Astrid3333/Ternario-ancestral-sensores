# Ternary Ancestral Kernel v2 — Terminal + Shell

Sistema operativo experimental ultra-liviano basado en matemáticas ancestrales.

## Nivel 2: Terminal + Shell

| Feature | Estado |
|---------|--------|
| VGA 80x25 con scroll | ✓ |
| Keyboard driver (scancode set 1) | ✓ |
| Serial logging (9600 baud) | ✓ |
| Shell interactivo con 12 comandos | ✓ |
| Prompt con colores (ternary@mayan$) | ✓ |
| Editor de línea (backspace) | ✓ |
| Conversor ternario (`trit <n>`) | ✓ |
| Conversor babilónico (`b60 <n>`) | ✓ |

## Especificaciones

- **Tamaño total:** ~6KB de código
- **Memoria:** 3.5KB (60 bloques × 60 bytes)
- **Procesos:** Máximo 33 (mod-33)
- **Archivos:** Máximo 33
- **Arquitectura:** x86 (i386, modo real)

## Comandos del Shell

```
help         Mostrar ayuda
ps           Listar procesos activos
mem          Estado de memoria (Base 60)
fs           Listar archivos (Quipu)
cal          Calendario Maya (Tzolkin/Haab)
fork         Crear nuevo proceso
kill <pid>   Matar proceso
echo <msg>   Imprimir mensaje
clear        Limpiar pantalla
trit <n>     Convertir número a ternario
b60 <n>      Convertir a dirección babilónica
halt         Apagar sistema
```

## Compilación

### Requisitos
```bash
# Ubuntu/Debian
sudo apt install nasm gcc-i686-linux-gnu qemu-system-x86

# Arch Linux
sudo pacman -S nasm i686-elf-gcc qemu-system-x86
```

### Build
```bash
make all     # Crear ISO
make run     # Ejecutar en QEMU
make debug   # Ejecutar con GDB
make info    # Mostrar información
make clean   # Limpiar
```

## Arquitectura

```
┌─────────────────────────────────────────────┐
│            KERNEL TERNARIO v2               │
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
│                    │                        │
│  ┌─────────────────▼─────────────────┐    │
│  │        Terminal + Shell           │    │
│  │   VGA + Keyboard + Serial I/O    │    │
│  └───────────────────────────────────┘    │
└─────────────────────────────────────────────┘
```

## Memoria

60 bloques de 60 bytes cada uno:

| Bloque | Color | Uso |
|--------|-------|-----|
| 0-4 | Kernel | Código del kernel |
| 5-10 | Process | Procesos activos |
| 11-20 | Stack | Pilas de procesos |
| 21-50 | Code | Código de usuario |
| 51-59 | Free | Libre |

## Bugs corregidos (v2)

1. `outb`/`inb` no declaradas → agregadas en `ternary.h`
2. `cmd_buf` overflow potencial → buffer aumentado a 64
3. `mem_alloc()` retorno incorrecto → cambiado a `int16_t`
4. `select_next()` podía retornar PID 0 inválido → agregado flag `found`
5. `sched_kill()` no liberaba memoria → ahora libera el bloque
6. `fs_init()` nombre "/" mal inicializado → fix con `strcpy_t`
7. `mem_get_base()` sin validación → agregada
8. VGA sin scroll → implementado
9. Keyboard incompleto → scancode set 1 completo

## Futuro

- [ ] Preemptive multitasking
- [ ] Memoria virtual en base 60
- [ ] Drivers de disco
- [ ] Syscalls completas
- [ ] Modo protegido (32 bits)
- [ ] Network stack ternario

## Licencia

MIT
