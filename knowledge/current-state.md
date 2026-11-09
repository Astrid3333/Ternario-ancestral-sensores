# Current State — Tritos OS

## Last updated: 2026-11-08

## Goal
Build Tritos, a complete ternary ancestral bare-metal operating system.

## Kernel Version: v4.5 (GUI)

## Status
- **Kernel**: ~360KB ELF, boots in QEMU via GRUB multiboot (verificado 2026-11-08)
- **All core OS components implemented**: GDT, IDT, Paging, Syscalls, Memory, Scheduler, Processes
- **Network**: TCP/IP stack with RTL8139, HTTP/HTTPS, DNS, DHCP
- **Filesystem**: FAT16 (read only)
- **Userspace**: Shell in Ring 3, complete libc, window manager
- **Process management**: fork, exec, wait, exit, context switch
- **Missing**: FAT16 write, USB, sound, more apps, TrueType fonts
- **WIP del usuario sin commitear**: GUI del kernel sobre framebuffer (`window.c` +198,
  `kernel.c` llama a `gui_run()`/`gui_text_buffer()`, `ternary.h` con prototipos `gui_*`)

## Recent Work (2026-11-08) — 12 bugs del análisis vs MCP corregidos (commit `544f633`)
- **Buffer overflows**: `num_to_str`/`int_to_str` no acotaban el destino; 41 buffers `nb[2]`
  desbordaban con valores >2 dígitos. Se agregaron `num_to_str_bounded`/`int_to_str_bounded`
  en `include/ternary.h` y se migraron 355 llamadas en 24 archivos.
- **USB**: `vendor[16]`/`product[16]` en `usb_storage.c` desbordaban y corrompían `mounted`;
  ahora 17 bytes + `memset` del sector.
- **FAT16**: el marcador 0xE5 se comparaba con `char` con signo (0xFFFFFFE5) → nunca coincidía;
  ahora se castea a `uint8_t`.
- **Prototipos**: 5 declaraciones implícitas (`mem_alloc`, `mem_free`, `serial_putc`,
  `serial_puts`, `wm_redraw`) eliminadas con prototipos en `ternary.h`.
- **Guardas de rango inoperantes**: `global_symbol_count` (elf_dynamic) y los días
  tzolkin/haab (scheduler) eran `uint8_t` → `>MAX_SYMBOLS`/`>TZOLKIN` nunca se cumplían;
  ahora `uint16_t` (y `sched_get_state` usa `uint16_t*`).
- **`getchar`**: leía en `char` sin inicializar y sin validar el retorno de `syscall_read`.
- **Casts de puntero**: `net.c` usaba `(uint32_t)(uint64_t)ptr`; ahora `(uintptr_t)`.
- **Build**: `build-kernel.sh` compilaba 3 de 45 TUs, generaba un `linker.ld` incompatible
  (rompía el boot multiboot) y usaba `linux16` para una imagen multiboot → ahora delega en
  el `Makefile` canónico. El `Makefile` crea `releases/`, deja de reescribir el `grub.cfg`
  versionado y `clean` ya no borra archivos versionados.
- **Herramientas Python**: `tritos_gui.py` (import `sys`, ruta `disk.img`),
  `tritos_lab/tritos_godot.py` (SyntaxError) y `tritos_lab/__init__.py` (el paquete tapa
  el módulo suelto).
- **Experimento**: `run_all.m` mezclaba dos modelos de Landauer; ahora se reporta por
  modelo y por unidad (bit vs símbolo).

## Key Files
- `output/ternary-kernel/` — bare-metal kernel (`Makefile` = build canónico)
- `output/tak-userspace/` — Linux userspace
- `build-kernel.sh` — wrapper del build (delega en el Makefile)
- `tritos_gui.py` — GTK3 GUI
- `tritos_codec.py` — ternary codec
- `output/experiment-suite/01-ternario-ancestral-sensores/experiment/run_all.m` — experimento IoT

## Verificación (2026-11-08)
- `gcc -m32 -Wall -Wextra` sobre las 45 TUs del kernel: 0 errores, avisos 89 → 68
  (los que quedan son `unused-*` y `sign-compare`, no bugs).
- `make all` genera `output/releases/ternary_os.iso` y `disk.img`; el kernel bootea en QEMU.
- `make all` en `output/tak-userspace` compila los 6 binarios.
- Los 2 experimentos Octave (`01-ternario-ancestral-sensores` y `04-calendario-maya-azteca-persa`)
  corren y terminan con "TODO MEDIDO OK".
