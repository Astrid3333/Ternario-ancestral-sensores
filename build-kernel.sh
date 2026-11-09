#!/bin/bash
# =============================================================================
# TRITOS — Wrapper de build del kernel bare-metal
# -----------------------------------------------------------------------------
# El build canónico está en output/ternary-kernel/Makefile: compila todas las
# TUs del kernel, linka con el linker.ld multiboot versionado y genera
#   output/releases/ternary_os.iso   (GRUB multiboot)
#   output/releases/disk.img
#
# Este script solo orquesta el Makefile. NO duplica los flags de compilacion,
# NO genera linker.ld (esta versionado y es el unico compatible con multiboot)
# y NO usa linux16 para una imagen que arranca por multiboot.
# =============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
KERNEL_DIR="$SCRIPT_DIR/output/ternary-kernel"
RELEASE_DIR="$SCRIPT_DIR/output/releases"

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
    echo "  (sin args)    Compilar kernel + crear ISO y disk.img"
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

    command -v ld &>/dev/null || fail "ld no encontrado"
    command -v objcopy &>/dev/null || fail "objcopy no encontrado"
    ok "ld + objcopy"

    # El Makefile usa grub-mkrescue y cae a genisoimage si no esta.
    if command -v grub-mkrescue &>/dev/null; then
        ok "grub-mkrescue"
    elif command -v genisoimage &>/dev/null; then
        ok "genisoimage (fallback)"
    else
        fail "ni grub-mkrescue ni genisoimage encontrados. Instalá: sudo apt install grub-common genisoimage"
    fi

    command -v make &>/dev/null || fail "make no encontrado"
    ok "make"
}

build_kernel() {
    info "Compilando kernel (Makefile canónico)..."
    make -C "$KERNEL_DIR" all
    ok "build completo"
}

show_info() {
    cd "$KERNEL_DIR" || fail "no existe $KERNEL_DIR"

    echo ""
    echo -e "  ${CYAN}════════════════════════════════════════════${NC}"
    echo -e "  ${BOLD}TERNARY ANCESTRAL KERNEL${NC}"
    echo -e "  ${CYAN}════════════════════════════════════════════${NC}"
    echo ""

    if [ -f boot/boot.bin ]; then
        echo -e "  Boot loader:  $(wc -c < boot/boot.bin) bytes"
    fi
    if [ -f ternary_kernel.elf ]; then
        echo -e "  Kernel ELF:   $(wc -c < ternary_kernel.elf) bytes"
    fi
    if [ -f ternary_kernel.bin ]; then
        echo -e "  Kernel bin:   $(wc -c < ternary_kernel.bin) bytes"
    fi
    if [ -f "$RELEASE_DIR/ternary_os.iso" ]; then
        echo -e "  ISO:          $(du -h "$RELEASE_DIR/ternary_os.iso" | cut -f1)"
    fi
    if [ -f "$RELEASE_DIR/disk.img" ]; then
        echo -e "  Disk image:   $(du -h "$RELEASE_DIR/disk.img" | cut -f1)"
    fi

    echo ""
    echo -e "  ${BOLD}Arranque:${NC}"
    echo -e "    GRUB multiboot (ver iso/boot/grub/grub.cfg)"
    echo -e "    Kernel en 0x100000, entrada _start"
    echo ""
    echo -e "  ${BOLD}Para probar:${NC}"
    echo -e "    qemu-system-i386 -cdrom output/releases/ternary_os.iso -m 8M -serial stdio"
    echo -e "  ${CYAN}════════════════════════════════════════════${NC}"
}

clean() {
    info "Limpiando..."
    # El clean del Makefile preserva los archivos versionados (linker.ld, grub.cfg).
    make -C "$KERNEL_DIR" clean
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
            build_kernel
            show_info
            command -v qemu-system-i386 &>/dev/null || fail "qemu-system-i386 no encontrado"
            echo ""
            info "Abriendo en QEMU..."
            qemu-system-i386 -cdrom "$RELEASE_DIR/ternary_os.iso" -m 8M -serial stdio
            ;;
        --disk)
            check_deps
            build_kernel
            show_info
            ;;
        *)
            check_deps
            build_kernel
            show_info
            ;;
    esac
}

main "$@"
