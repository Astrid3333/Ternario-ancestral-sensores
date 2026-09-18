#!/bin/bash
# =============================================================================
# TRITOS — Compilador de Kernel Bare-Metal
# =============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
KERNEL_DIR="$SCRIPT_DIR/output/ternary-kernel"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

ok() { echo -e "${GREEN}✓${NC} $1"; }
fail() { echo -e "${RED}✗${NC} $1"; exit 1; }
info() { echo -e "${BOLD}$1${NC}"; }

usage() {
    echo "Uso: $0 [opciones]"
    echo ""
    echo "Opciones:"
    echo "  (sin args)    Compilar kernel + crear ISO"
    echo "  --run         Compilar + abrir en QEMU"
    echo "  --disk        Compilar + crear disk.img"
    echo "  --clean       Limpiar archivos generados"
    echo "  --info        Mostrar info del kernel"
    echo ""
    echo "Ejemplo:"
    echo "  ./build-kernel.sh --run    # Compilar y probar en QEMU"
}

check_deps() {
    info "Verificando dependencias..."
    
    command -v nasm &>/dev/null || fail "nasm no encontrado. Instalá: sudo apt install nasm"
    ok "nasm"
    
    command -v gcc &>/dev/null || fail "gcc no encontrado"
    ok "gcc"
    
    # Verificar soporte 32-bit
    echo 'int main(){return 0;}' | gcc -m32 -x c - -o /dev/null 2>/dev/null || fail "gcc-multilib no instalado. Instalá: sudo apt install gcc-multilib"
    ok "gcc-multilib"
    
    command -v genisoimage &>/dev/null || fail "genisoimage no encontrado. Instalá: sudo apt install genisoimage"
    ok "genisoimage"
    
    command -v qemu-system-i386 &>/dev/null || fail "qemu-system-i386 no encontrado"
    ok "qemu-system-i386"
}

compile_boot() {
    info "Ensamblando bootloader..."
    cd "$KERNEL_DIR"
    
    nasm -f bin boot/boot.asm -o boot/boot.bin
    local size=$(wc -c < boot/boot.bin)
    ok "boot.bin ($size bytes)"
}

compile_kernel() {
    info "Compilando kernel..."
    cd "$KERNEL_DIR"
    
    # Compilar cada archivo (sin cross-compiler, usando gcc estándar con -m32)
    local CFLAGS="-m32 -ffreestanding -O2 -Wall -Wextra -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -fno-pie -no-pie"
    
    gcc $CFLAGS -Iinclude -c src/kernel.c -o kernel.o
    ok "kernel.c → kernel.o"
    
    gcc $CFLAGS -Iinclude -c src/memory.c -o memory.o
    ok "memory.c → memory.o"
    
    gcc $CFLAGS -Iinclude -c src/scheduler.c -o scheduler.o
    ok "scheduler.c → scheduler.o"
}

create_linker() {
    info "Generando linker script..."
    cd "$KERNEL_DIR"
    
    cat > linker.ld << 'EOF'
OUTPUT_FORMAT("binary")
ENTRY(kernel_main)
SECTIONS
{
  . = 0x10000;
  .text : { *(.text) }
  .rodata : { *(.rodata) }
  .data : { *(.data) }
  .bss : { *(.bss) }
}
EOF
    ok "linker.ld"
}

link_kernel() {
    info "Linkeando kernel..."
    cd "$KERNEL_DIR"
    
    ld -m elf_i386 -T linker.ld -no-pie -nostdlib kernel.o memory.o scheduler.o -o ternary_kernel.bin
    local size=$(wc -c < ternary_kernel.bin)
    ok "ternary_kernel.bin ($size bytes)"
}

create_disk() {
    info "Creando disco de arranque..."
    cd "$KERNEL_DIR"
    
    # Disco de 20 sectores (10KB)
    dd if=/dev/zero of=disk.img bs=512 count=20 2>/dev/null
    dd if=boot/boot.bin of=disk.img bs=512 count=1 conv=notrunc 2>/dev/null
    dd if=ternary_kernel.bin of=disk.img bs=512 seek=1 conv=notrunc 2>/dev/null
    
    ok "disk.img (10KB, booteable)"
}

create_iso() {
    info "Creando ISO booteable..."
    cd "$KERNEL_DIR"
    
    mkdir -p iso/boot/grub
    
    # Configuración GRUB
    cat > iso/boot/grub/grub.cfg << 'EOF'
set timeout=3
set default=0

menuentry "Ternary Ancestral Kernel v2" {
    set root='(hd0,msdos1)'
    linux16 /boot/ternary.img
}

menuentry "Ternary Kernel (safe mode)" {
    set root='(hd0,msdos1)'
    linux16 /boot/ternary.img
}
EOF
    
    # Copiar kernel al ISO
    cp disk.img iso/boot/ternary.img
    
    # Crear ISO
    genisoimage -R -b boot/grub/grub.cfg -no-emul-boot \
        -o ternary_os.iso iso/ 2>/dev/null
    
    local size=$(du -h ternary_os.iso | cut -f1)
    ok "ternary_os.iso ($size)"
}

show_info() {
    cd "$KERNEL_DIR"
    
    echo ""
    echo -e "  ${CYAN}════════════════════════════════════════════${NC}"
    echo -e "  ${BOLD}TERNARY ANCESTRAL KERNEL v2${NC}"
    echo -e "  ${CYAN}════════════════════════════════════════════${NC}"
    echo ""
    
    if [ -f boot/boot.bin ]; then
        echo -e "  Boot loader:  $(wc -c < boot/boot.bin) bytes"
    fi
    if [ -f ternary_kernel.bin ]; then
        echo -e "  Kernel:       $(wc -c < ternary_kernel.bin) bytes"
    fi
    if [ -f ternary_os.iso ]; then
        echo -e "  ISO:          $(du -h ternary_os.iso | cut -f1)"
    fi
    if [ -f disk.img ]; then
        echo -e "  Disk image:   $(du -h disk.img | cut -f1)"
    fi
    
    echo ""
    echo -e "  ${BOLD}Features:${NC}"
    echo -e "    - VGA con scroll y colores (80x25)"
    echo -e "    - Keyboard driver (scancode set 1)"
    echo -e "    - Serial logging (9600 baud)"
    echo -e "    - Ternary logic {-1, 0, +1}"
    echo -e "    - Memory: Base 60 (Babylonian)"
    echo -e "    - Scheduler: Maya cycles"
    echo -e "    - Filesystem: Quipu"
    echo ""
    echo -e "  ${BOLD}Shell commands:${NC}"
    echo -e "    help, ps, mem, fs, cal, fork, kill, echo, clear, trit, b60, halt"
    echo ""
    echo -e "  ${BOLD}Para probar:${NC}"
    echo -e "    qemu-system-i386 -cdrom ternary_os.iso -m 4M"
    echo -e "  ${CYAN}════════════════════════════════════════════${NC}"
}

clean() {
    info "Limpiando..."
    cd "$KERNEL_DIR"
    rm -f boot/boot.bin kernel.o memory.o scheduler.o linker.ld
    rm -f ternary_kernel.bin disk.img ternary_os.iso
    rm -rf iso/
    ok "Archivos limpiados"
}

# =============================================================================
# MAIN
# =============================================================================
main() {
    echo ""
    echo -e "${CYAN}  ╔══════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}  ║   TRITOS — Compilador de Kernel          ║${NC}"
    echo -e "${CYAN}  ╚══════════════════════════════════════════╝${NC}"
    echo ""
    
    case "${1:-}" in
        --help|-h)
            usage
            exit 0
            ;;
        --clean)
            clean
            exit 0
            ;;
        --info)
            show_info
            exit 0
            ;;
        --run)
            check_deps
            compile_boot
            compile_kernel
            create_linker
            link_kernel
            create_disk
            create_iso
            show_info
            echo ""
            info "Abriendo en QEMU..."
            qemu-system-i386 -cdrom "$KERNEL_DIR/ternary_os.iso" -m 4M -serial stdio
            ;;
        --disk)
            check_deps
            compile_boot
            compile_kernel
            create_linker
            link_kernel
            create_disk
            show_info
            ;;
        *)
            check_deps
            compile_boot
            compile_kernel
            create_linker
            link_kernel
            create_disk
            create_iso
            show_info
            ;;
    esac
}

main "$@"
