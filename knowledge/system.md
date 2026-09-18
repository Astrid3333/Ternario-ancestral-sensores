# System Knowledge

## Project: Tritos OS
- **Type**: Bare-metal ternary ancestral operating system
- **Language**: C + x86 assembly (NASM)
- **Target**: i386 (x86) via QEMU
- **Binary**: `./tritos` (userspace), `ternary_kernel.elf` (bare-metal)
- **GitHub**: https://github.com/Astrid3333/Ternario-ancestral-sensores.git
- **Token**: (stored in .openscience/ only, NOT in repo)

## Build Commands
- **Bare-metal kernel**: `cd output/ternary-kernel && make`
- **Userspace**: `./build.sh` or `gcc -m32 ...`
- **QEMU test**: `qemu-system-i386 -cdrom ternary_os.iso -m 8M -serial file:/tmp/tritos.log -display none`

## Key Conventions
- **Kernel compilation**: `gcc -m32 -ffreestanding -fno-pie -no-pie`
- **Assembly**: NASM with `-f elf32`
- **Memory**: `-m 8M` required (was 4M, TLS/BSS increase caused overflow)
- **Serial debug**: Enable `serial_input_enabled = 1` from start
- **IDT/GDT order**: IDT must be installed BEFORE GDT

## User Preferences
- **Language**: Spanish (Argentine dialect)
- **Style**: Terse responses
- **sudo password**: `1212`
- **Desktop environment**: Xfce (Thunar file manager)
