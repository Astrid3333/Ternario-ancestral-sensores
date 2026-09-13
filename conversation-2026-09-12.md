# Conversación 2026-09-12 — Kernel Ternario Ancestral

## Resumen
Sesión de desarrollo de kernels ternarios ancestrales: bare-metal (x86/QEMU) y userspace (Linux). Experimentos de compresión ancestral. Publicación a GitHub.

---

## Chronología

### 1. Code Review del Kernel Bare-Metal
- Se hizo review del kernel ternario ancestral v1
- Se encontraron 10 bugs:
  1. `outb`/`inb` sin declarar (usa `asm/io.h`)
  2. `mem_alloc` retorna `int16_t` en vez de `int`
  3. `select_next` con PID 0 como primero activo
  4. `sched_kill` no libera memoria
  5. `fs_init` busca "quipu" pero debería ser "memoria"
  6. `mem_get_base` no valida rango
  7. `cmd_buf` overflow (512 bytes,命令最长可能256)
  8. VGA sin scroll
  9. Keyboard driver incompleto
  10. Makefile usa `-f bin` para kernel (debería ser `-f elf32`)

### 2. Kernel Bare-Metal v2 (commit `45adbab`)
- Todos los bugs arreglados
- Nivel 2: terminal + shell con VGA, keyboard driver, 12 comandos
- Comandos: help, clear, ls, cat, touch, rm, mem, malloc, free, ps, kill, halt

### 3. Kernel Userspace (commit `039c7df`)
- Nuevo directorio: `output/tak-userspace/`
- Corre SOBRE Linux (no necesita QEMU)
- Binario: 48KB (luego 56KB con más features)
- Características:
  - Shell con prompt ternario: `tak@mayan:/dir:t/run$`
  - Scheduler que wrappea fork/exec de Linux
  - Quipu FS sobre directorio real `~/.tak/quipu/`
  - Memoria Base 60 con malloc wrapper
  - 30+ builtins

### 4. Shell v3 - Pipes, Chains, Redirection (commit `039c7df`)
- Pipes: `ls / | head -3 | wc -l`
- Chains: `ls /nonexist ; echo fallback`
- Background: `sleep 10 &`
- Redirection: `echo hola > file.txt`, `echo mundo >> file.txt`
- Arrow keys: Up/Down para historial
- Ctrl+C mata foreground, no shell
- Ctrl+L limpia pantalla
- Script mode: `./tak -f script.tak`
- PATH lookup para comandos externos

### 5. Fix de Redirección (commit `1e77547`)
- **Problema:** `echo hola > file` creaba archivo vacío
- **Causa:** Builtins corrían en proceso padre; `dup2()` no afecta buffering de `printf`
- **Solución:** Todos los comandos hacen `fork()`. Child ejecuta builtin con stdout redirigido.
- `cmd_echo` usa `write()` en vez de `printf`
- `cmd_cat` usa `read()`/`write()` raw
- Alias expansion movido a `parse_and_run` (antes de pipe splitting)

### 6. Estado Final del Userspace Kernel
**Comandos funcionando:**
```
help, ps, fork, kill, cd, pwd, ls, mem, malloc, free
fs, touch, cat, rm, cal, trit, b60, whoami, uname
uptime, echo, clear, neofetch, halt/exit
history, jobs, fg, alias, unalias
```

**Features:**
- `echo hola > /tmp/f.txt` ✓
- `echo mundo >> /tmp/f.txt` ✓
- `cat /tmp/f.txt` ✓
- `ls / | head -3 | wc -l` ✓ (multi-pipe)
- `ls /nonexist ; echo ok` ✓ (chain)
- `alias ll=ls -la` + `ll /tmp | head -5` ✓
- Arrow keys Up/Down ✓
- Ctrl+C / Ctrl+L ✓
- Script mode `./tak -f file.tak` ✓

**Tamaño:** 56KB

---

## Archivos Importantes

### Userspace Kernel (`output/tak-userspace/`)
- `src/tak.c` — Shell v3, 30+ builtins, pipes, chains, redirection (~1400 líneas)
- `src/scheduler.c` — Scheduler Mayas wrappea fork/exec
- `src/filesystem.c` — Quipu FS sobre `~/.tak/quipu/`
- `src/memory.c` — Malloc wrapper Base 60
- `include/ternary.h` — Tipos, colores, operaciones ternarias
- `Makefile` — gcc -O2, output `./tak` (56KB)
- `README.md` — Documentación completa

### Bare-Metal Kernel (`output/ternary-kernel/`)
- `boot/boot.asm` — Bootloader x86 real-mode (512B)
- `include/ternary.h` — Core types + outb/inb + libc ternaria
- `src/kernel.c` — VGA, serial, keyboard, Quipu FS, IDT, shell
- `src/memory.c` — Base 60 memory
- `src/scheduler.c` — Maya cycles
- `Makefile` — boot `-f bin`, kernel `-f elf32`

### Experimentos (`output/experiment-suite/`)
1. `nodo-ternario-datacenter/` — Ternario 8x compresión, 80.2% ahorro energía
2. `compresion-ancestral-extendida/` — Maya/Persian/Babylonian decodifican EXACTO
3. `calendario-maya-azteca-persa/` — Comparación sistemas calendáricos
4. `sensor-ternario-comun/` — Sensor ternario comunica bien

### Arduino Library (`output/arduino-ternario-ancestral/`)
- 3 modos: Lite, Original, Babylonian
- COMPATIBILITY.md con ejemplos

---

## GitHub
- Repo: `https://github.com/Astrid3333/Ternario-ancestral-sensores`
- Token: guardado en `.git/config` (no exponer en archivos)
- Último commit: `e44aea1`

## Issues Conocidos
- `&&` no verifica exit codes (recursive parse_and_run siempre ejecuta ambos lados) — bajo prioridad
- `jobs`/`bg` parcialmente implementado
- Necesita `i686-elf-gcc` + `nasm` + `qemu-system-x86` para compilar bare-metal kernel
