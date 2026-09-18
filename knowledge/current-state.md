# Current State — Tritos OS

## Last updated: 2026-09-17

## Goal
Build Tritos, a complete ternary ancestral bare-metal operating system.

## Kernel Version: v4.3

## Status
- **Kernel**: 148KB ELF, boots in QEMU
- **All core OS components implemented**: GDT, IDT, Paging, Syscalls, Memory, Scheduler, Processes
- **Network**: TCP/IP stack with RTL8139, HTTP/HTTPS, DNS, DHCP
- **Filesystem**: FAT16 (read only)
- **Userspace**: Shell in Ring 3, complete libc, window manager
- **Process management**: fork, exec, wait, exit, context switch
- **Missing**: FAT16 write, USB, sound, more apps, TrueType fonts

## Recent Work (2026-09-17)
- Implemented GDT (6 segments), paging (4MB), syscalls (32), ELF loader
- Implemented complete libc, user shell (Ring 3), window manager
- Fixed IDT/GDT init order (IDT must come first)
- Fixed serial output mirroring for debug

## Key Files
- `output/ternary-kernel/` — bare-metal kernel
- `output/tak-userspace/` — Linux userspace
- `tritos_gui.py` — GTK3 GUI
- `tritos_codec.py` — ternary codec
