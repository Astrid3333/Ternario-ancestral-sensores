#!/bin/bash
# =============================================================================
# TRITOS — Creador de USB Booteable
# =============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DISK_IMG="$SCRIPT_DIR/output/ternary-kernel/disk.img"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

ok() { echo -e "${GREEN}✓${NC} $1"; }
fail() { echo -e "${RED}✗${NC} $1"; exit 1; }
warn() { echo -e "${YELLOW}!${NC} $1"; }
info() { echo -e "${BOLD}$1${NC}"; }

usage() {
    echo "Uso: $0 [dispositivo]"
    echo ""
    echo "Ejemplos:"
    echo "  ./create-usb.sh           # Lista dispositivos y pregunta"
    echo "  ./create-usb.sh /dev/sdb  # Escribe directo en sdb"
    echo ""
    echo "⚠ Esto ELIMINA todos los datos del USB"
}

list_devices() {
    echo ""
    echo -e "  ${BOLD}Dispositivos de bloque:${NC}"
    echo ""
    lsblk -d -o NAME,SIZE,TYPE,MODEL | grep -v "loop\|sr\|ram"
    echo ""
}

confirm_write() {
    local device="$1"
    local size_bytes=$(lsblk -bno SIZE "$device" 2>/dev/null | head -1)
    local size_gb=$((size_bytes / 1073741824))
    
    echo ""
    echo -e "  ${RED}╔══════════════════════════════════════════╗${NC}"
    echo -e "  ${RED}║  ⚠ ADVERTENCIA: OPERACIÓN PELIGROSA     ║${NC}"
    echo -e "  ${RED}╚══════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "  Dispositivo: ${BOLD}$device${NC}"
    echo -e "  Tamaño:      ${BOLD}$size_gb GB${NC}"
    echo -e "  Contenido:   ${BOLD}SE ELIMINARÁ TODO${NC}"
    echo ""
    echo -e "  ${YELLOW}Esto no se puede deshacer.${NC}"
    echo ""
    read -p "  ¿Continuar? (escribe SI para confirmar): " confirm
    
    if [ "$confirm" != "SI" ]; then
        echo ""
        warn "Operación cancelada"
        exit 0
    fi
}

write_usb() {
    local device="$1"
    
    info "Preparando escritura..."
    
    # Desmontar particiones
    sudo umount "${device}"* 2>/dev/null || true
    
    info "Escribiendo disk.img en $device..."
    echo ""
    
    sudo dd if="$DISK_IMG" of="$device" bs=512 status=progress oflag=sync
    
    echo ""
    sync
    
    ok "USB booteable creado en $device"
}

verify_usb() {
    local device="$1"
    
    info "Verificando escritura..."
    
    # Leer primer sector y verificar firma
    local signature=$(sudo dd if="$device" bs=1 skip=510 count=2 2>/dev/null | xxd -p)
    
    if [ "$signature" = "55aa" ]; then
        ok "Firma de arranque verificada (0xAA55)"
    else
        warn "Firma de arranque no detectada (puede ser normal)"
    fi
    
    echo ""
    echo -e "  ${GREEN}USB listo para bootear.${NC}"
    echo ""
    echo -e "  Para usar:"
    echo -e "    1. Conectar el USB a la PC destino"
    echo -e "    2. Reiniciar y elegir bootear desde USB"
    echo -e "    3. Aparecerá: ${BOLD}ternary@mayan$${NC}"
}

# =============================================================================
# MAIN
# =============================================================================
main() {
    echo ""
    echo -e "${CYAN}  ╔══════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}  ║   TRITOS — USB Booteable                 ║${NC}"
    echo -e "${CYAN}  ╚══════════════════════════════════════════╝${NC}"
    echo ""
    
    # Verificar que existe disk.img
    if [ ! -f "$DISK_IMG" ]; then
        fail "disk.img no encontrado en $DISK_IMG"
        echo -  "Ejecutá primero: ./build-kernel.sh"
    fi
    
    local size=$(du -h "$DISK_IMG" | cut -f1)
    ok "disk.img ($size)"
    
    # Determinar dispositivo
    local device="${1:-}"
    
    if [ -z "$device" ]; then
        list_devices
        read -p "  Dispositivo USB (ej: /dev/sdb): " device
    fi
    
    if [ -z "$device" ]; then
        fail "No se especificó dispositivo"
    fi
    
    if [ ! -b "$device" ]; then
        fail "Dispositivo $device no encontrado"
    fi
    
    # Confirmar
    confirm_write "$device"
    
    # Escribir
    write_usb "$device"
    
    # Verificar
    verify_usb "$device"
}

main "$@"
