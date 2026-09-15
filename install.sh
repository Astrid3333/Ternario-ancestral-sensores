#!/bin/bash
# =============================================================================
# TRITOS — Instalador Unificado
# Kernel Ternario Ancestral + Shell Científico
# =============================================================================
set -e

VERSION="1.0"
INSTALL_DIR="$HOME/.local/bin"
DATA_DIR="$HOME/.tritos"
BOOT_DIR="/boot/ternary"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

print_banner() {
    echo ""
    echo -e "${CYAN}  ╔══════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}  ║   TRITOS — Kernel Ternario Ancestral     ║${NC}"
    echo -e "${CYAN}  ║   v${VERSION} — Instalador Unificado           ║${NC}"
    echo -e "${CYAN}  ╚══════════════════════════════════════════╝${NC}"
    echo ""
}

ok() { echo -e "        ${GREEN}✓${NC} $1"; }
fail() { echo -e "        ${RED}✗${NC} $1"; }
warn() { echo -e "        ${YELLOW}!${NC} $1"; }
info() { echo -e "  ${BOLD}$1${NC}"; }

# =============================================================================
# 1. DETECCIÓN DEL SISTEMA
# =============================================================================
detect_system() {
    info "[1/6] Detectando sistema..."
    
    OS=$(uname -s)
    ARCH=$(uname -m)
    
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        DISTRO="$NAME"
        VERSION_ID="$VERSION_ID"
    else
        DISTRO="Unknown"
        VERSION_ID="?"
    fi
    
    ok "Sistema: $DISTRO $VERSION_ID ($ARCH)"
    
    if [ "$OS" != "Linux" ]; then
        fail "Este instalador es solo para Linux"
        exit 1
    fi
}

# =============================================================================
# 2. VERIFICAR/INSTALAR DEPENDENCIAS
# =============================================================================
install_deps() {
    info "[2/6] Verificando dependencias..."
    
    NEED_SUDO=0
    MISSING=""
    
    # gcc
    if command -v gcc &>/dev/null; then
        ok "gcc"
    else
        MISSING="$MISSING gcc"
        warn "gcc — se instalará"
    fi
    
    # make
    if command -v make &>/dev/null; then
        ok "make"
    else
        MISSING="$MISSING make"
        warn "make — se instalará"
    fi
    
    # nasm
    if command -v nasm &>/dev/null; then
        ok "nasm"
    else
        MISSING="$MISSING nasm"
        warn "nasm — se instalará"
    fi
    
    # gcc-multilib (para kernel 32-bit)
    if dpkg -l gcc-multilib &>/dev/null 2>&1; then
        ok "gcc-multilib"
    else
        MISSING="$MISSING gcc-multilib"
        warn "gcc-multilib — se instalará"
    fi
    
    # qemu
    if command -v qemu-system-i386 &>/dev/null; then
        ok "qemu-system-i386"
    else
        MISSING="$MISSING qemu-system-x86"
        warn "qemu-system-x86 — se instalará"
    fi
    
    # genisoimage
    if command -v genisoimage &>/dev/null; then
        ok "genisoimage"
    else
        MISSING="$MISSING genisoimage"
        warn "genisoimage — se instalará"
    fi
    
    # libz-dev (para audio)
    if dpkg -l libz-dev &>/dev/null 2>&1; then
        ok "libz-dev"
    else
        MISSING="$MISSING libz-dev"
        warn "libz-dev — se instalará"
    fi
    
    # libbz2-dev (para audio)
    if dpkg -l libbz2-dev &>/dev/null 2>&1; then
        ok "libbz2-dev"
    else
        MISSING="$MISSING libbz2-dev"
        warn "libbz2-dev — se instalará"
    fi
    
    # Instalar faltantes
    if [ -n "$MISSING" ]; then
        echo ""
        echo -e "  ${YELLOW}Instalando dependencias faltantes...${NC}"
        sudo apt update -qq
        sudo apt install -y -qq $MISSING
        ok "Dependencias instaladas"
    else
        ok "Todas las dependencias están instaladas"
    fi
}

# =============================================================================
# 3. COMPILAR TRITOS (USERSPACE)
# =============================================================================
compile_userspace() {
    info "[3/6] Compilando Tritos..."
    
    cd "$SCRIPT_DIR/output/tak-userspace"
    
    # Backup si existe
    if [ -f tritos ]; then
        cp tritos tritos.bak 2>/dev/null || true
    fi
    
    make clean 2>/dev/null || true
    make
    
    if [ -f tritos ]; then
        local size=$(du -h tritos | cut -f1)
        ok "tritos ($size)"
    else
        fail "Error compilando tritos"
        exit 1
    fi
    
    if [ -f tritos_science ]; then
        local size=$(du -h tritos_science | cut -f1)
        ok "tritos_science ($size)"
    fi
    
    cd "$SCRIPT_DIR"
}

# =============================================================================
# 4. COMPILAR KERNEL BARE-METAL
# =============================================================================
compile_kernel() {
    info "[4/6] Compilando kernel ternario..."
    
    cd "$SCRIPT_DIR/output/ternary-kernel"
    
    # Backup
    if [ -f ternary_kernel.bin ]; then
        cp ternary_kernel.bin ternary_kernel.bin.bak 2>/dev/null || true
    fi
    
    make clean 2>/dev/null || true
    make
    
    if [ -f ternary_os.iso ]; then
        local size=$(du -h ternary_os.iso | cut -f1)
        ok "ternary_os.iso ($size)"
    else
        fail "Error creando ISO"
        exit 1
    fi
    
    if [ -f disk.img ]; then
        ok "disk.img (bootable)"
    fi
    
    cd "$SCRIPT_DIR"
}

# =============================================================================
# 5. INSTALACIÓN
# =============================================================================
install_files() {
    info "[5/6] Instalando..."
    
    # Crear directorios
    mkdir -p "$INSTALL_DIR"
    mkdir -p "$DATA_DIR/quipu"
    mkdir -p "$DATA_DIR/config"
    mkdir -p "$DATA_DIR/bin"
    
    # Instalar userspace
    cp "$SCRIPT_DIR/output/tak-userspace/tritos" "$INSTALL_DIR/tritos"
    chmod +x "$INSTALL_DIR/tritos"
    ok "tritos → $INSTALL_DIR/tritos"
    
    if [ -f "$SCRIPT_DIR/output/tak-userspace/tritos_science" ]; then
        cp "$SCRIPT_DIR/output/tak-userspace/tritos_science" "$INSTALL_DIR/tritos_science"
        chmod +x "$INSTALL_DIR/tritos_science"
        ok "tritos_science → $INSTALL_DIR/tritos_science"
        
        # Copiar también a ~/.tritos/bin/ para que el shell lo encuentre
        cp "$SCRIPT_DIR/output/tak-userspace/tritos_science" "$DATA_DIR/bin/tritos_science"
        chmod +x "$DATA_DIR/bin/tritos_science"
        ok "tritos_science → $DATA_DIR/bin/tritos_science"
    fi
    
    # Verificar ~/.local/bin está en PATH
    if [[ ":$PATH:" != *":$INSTALL_DIR:"* ]]; then
        warn "$INSTALL_DIR no está en PATH"
        echo "  Agregá esto a tu ~/.bashrc:"
        echo "    export PATH=\"\$HOME/.local/bin:\$PATH\""
    fi
}

# =============================================================================
# 6. CONFIGURACIÓN
# =============================================================================
setup_config() {
    # Estado por defecto
    if [ ! -f "$DATA_DIR/state.json" ]; then
        cat > "$DATA_DIR/state.json" << 'EOF'
{
  "shell": {"prompt": "t–", "history_size": 1000},
  "iface": {"lang": "es", "style": "minimal", "theme": "ancestral"},
  "quipu": {"root": "~/.tritos/quipu"}
}
EOF
        ok "Estado inicial creado"
    fi
    
    # Idioma por defecto
    if [ ! -f "$DATA_DIR/config/lang" ]; then
        echo "es" > "$DATA_DIR/config/lang"
    fi
    
    # Historial vacío
    touch "$DATA_DIR/history"
}

# =============================================================================
# CONFIGURACIÓN GRUB (DUAL-BOOT)
# =============================================================================
setup_grub() {
    info "Configurando GRUB para dual-boot..."
    
    # Copiar ISO a /boot
    sudo mkdir -p "$BOOT_DIR"
    sudo cp "$SCRIPT_DIR/output/ternary-kernel/ternary_os.iso" "$BOOT_DIR/ternary_os.iso"
    sudo cp "$SCRIPT_DIR/output/ternary-kernel/disk.img" "$BOOT_DIR/ternary_disk.img"
    ok "Archivos copiados a $BOOT_DIR"
    
    # Crear entrada GRUB
    GRUB_ENTRY="/etc/grub.d/40_tritos"
    sudo tee "$GRUB_ENTRY" > /dev/null << 'GRUBEOF'
#!/bin/sh
exec tail -n +3 $0

menuentry "Ternary Ancestral Kernel" {
    set isofile="/boot/ternary/ternary_os.iso"
    loopback loop $isofile
    linux16 (loop)/boot/ternary.img
}

menuentry "Ternary Kernel (from disk image)" {
    set root='(hd0,msdos1)'
    linux16 /boot/ternary/ternary_disk.img
}
GRUBEOF
    sudo chmod 755 "$GRUB_ENTRY"
    ok "Entrada GRUB creada"
    
    # Actualizar GRUB
    sudo update-grub 2>/dev/null || sudo grub-mkconfig -o /boot/grub/grub.cfg 2>/dev/null || true
    ok "GRUB actualizado"
    
    echo ""
    echo -e "  ${GREEN}Dual-boot configurado.${NC}"
    echo -e "  Al reiniciar, elegí ${BOLD}\"Ternary Ancestral Kernel\"${NC}"
    echo -e "  en el menú de GRUB."
}

# =============================================================================
# USB BOOTEABLE
# =============================================================================
setup_usb() {
    local device="$1"
    
    if [ -z "$device" ]; then
        echo ""
        echo "  Discos disponibles:"
        lsblk -d -o NAME,SIZE,MODEL | grep -v "loop\|sr\|ram"
        echo ""
        read -p "  Dispositivo USB (ej: /dev/sdb): " device
    fi
    
    if [ ! -b "$device" ]; then
        fail "Dispositivo $device no encontrado"
        return 1
    fi
    
    local size=$(lsblk -bno SIZE "$device" 2>/dev/null | head -1)
    local size_gb=$((size / 1073741824))
    
    echo ""
    echo -e "  ${RED}⚠ ADVERTENCIA: Esto ELIMINARÁ todos los datos de $device ($size_gb GB)${NC}"
    read -p "  ¿Continuar? (escribe SI para confirmar): " confirm
    
    if [ "$confirm" != "SI" ]; then
        warn "Cancelado"
        return 1
    fi
    
    info "Escribiendo en $device..."
    sudo umount "${device}"* 2>/dev/null || true
    sudo dd if="$SCRIPT_DIR/output/ternary-kernel/disk.img" of="$device" bs=512 status=progress
    sync
    
    ok "USB booteable creado en $device"
    echo -e "  Podés bootear desde $device en cualquier PC"
}

# =============================================================================
# DESINSTALAR
# =============================================================================
uninstall() {
    info "Desinstalando Tritos..."
    
    # Binarios
    rm -f "$INSTALL_DIR/tritos"
    rm -f "$INSTALL_DIR/tritos_science"
    ok "Binarios eliminados de $INSTALL_DIR"
    
    # GRUB
    if [ -f /etc/grub.d/40_tritos ]; then
        sudo rm -f /etc/grub.d/40_tritos
        sudo rm -f "$BOOT_DIR/ternary_os.iso"
        sudo rm -f "$BOOT_DIR/ternary_disk.img"
        sudo update-grub 2>/dev/null || true
        ok "Entrada GRUB eliminada"
    fi
    
    # Datos (opcional)
    read -p "  ¿Eliminar datos de configuración (~/.tritos)? (s/n): " del_data
    if [ "$del_data" = "s" ]; then
        rm -rf "$DATA_DIR"
        ok "Datos eliminados"
    else
        ok "Datos conservados en $DATA_DIR"
    fi
    
    echo ""
    ok "Tritos desinstalado"
}

# =============================================================================
# VERIFICACIÓN
# =============================================================================
verify() {
    info "Verificando instalación..."
    
    if [ -x "$INSTALL_DIR/tritos" ]; then
        ok "tritos ejecutable"
        "$INSTALL_DIR/tritos" --help >/dev/null 2>&1 && ok "tritos funciona"
    else
        fail "tritos no encontrado en $INSTALL_DIR"
    fi
    
    if [ -x "$INSTALL_DIR/tritos_science" ]; then
        ok "tritos_science ejecutable"
    fi
    
    if [ -f "$BOOT_DIR/ternary_os.iso" ]; then
        ok "ISO booteable en $BOOT_DIR"
    fi
    
    if [ -f /etc/grub.d/40_tritos ]; then
        ok "Entrada GRUB configurada"
    fi
    
    echo ""
    echo -e "  ${GREEN}Verificación completada.${NC}"
}

# =============================================================================
# USO
# =============================================================================
usage() {
    echo "Uso: $0 [opciones]"
    echo ""
    echo "Opciones:"
    echo "  (sin args)    Instalación completa (userspace + kernel + GRUB)"
    echo "  --userspace   Solo instalar tritos (sin kernel)"
    echo "  --kernel      Solo compilar kernel"
    echo "  --usb [dev]   Crear USB booteable"
    echo "  --grub        Configurar GRUB dual-boot"
    echo "  --check       Verificar dependencias"
    echo "  --verify      Verificar instalación"
    echo "  --uninstall   Desinstalar Tritos"
    echo "  --help        Mostrar esta ayuda"
    echo ""
    echo "Ejemplos:"
    echo "  ./install.sh                 # Instalación completa"
    echo "  ./install.sh --userspace     # Solo tritos"
    echo "  ./install.sh --usb /dev/sdb  # USB booteable"
    echo "  ./install.sh --uninstall     # Desinstalar"
}

# =============================================================================
# MAIN
# =============================================================================
main() {
    print_banner
    
    case "${1:-}" in
        --help|-h)
            usage
            exit 0
            ;;
        --check)
            detect_system
            install_deps
            echo ""
            ok "Todas las dependencias están listas"
            ;;
        --userspace|--userspace-only)
            detect_system
            install_deps
            compile_userspace
            install_files
            setup_config
            verify
            ;;
        --kernel|--kernel-only)
            detect_system
            install_deps
            compile_kernel
            ;;
        --usb)
            setup_usb "$2"
            ;;
        --grub)
            setup_grub
            ;;
        --verify)
            verify
            ;;
        --uninstall)
            uninstall
            ;;
        *)
            # Instalación completa
            detect_system
            install_deps
            compile_userspace
            compile_kernel
            install_files
            setup_config
            
            echo ""
            echo -e "  ${BOLD}¿Cómo querés usar Tritos?${NC}"
            echo ""
            echo "    1) Dual-boot (aparece al reiniciar)  ${GREEN}← recomendado${NC}"
            echo "    2) USB booteable (llevar a otra PC)"
            echo "    3) Solo QEMU (probar sin instalar)"
            echo "    4) Nada, solo compilar"
            echo ""
            read -p "  > " choice
            
            case "$choice" in
                1) setup_grub ;;
                2) setup_usb ;;
                3)
                    echo ""
                    echo -e "  Para probar en QEMU:"
                    echo "    qemu-system-i386 -cdrom output/ternary-kernel/ternary_os.iso -m 4M"
                    ;;
                4)
                    echo ""
                    ok "Compilado. Binarios en $INSTALL_DIR"
                    ;;
                *)
                    warn "Opción no válida. Configurando GRUB..."
                    setup_grub
                    ;;
            esac
            
            verify
            
            echo ""
            echo -e "  ${CYAN}════════════════════════════════════════════${NC}"
            echo -e "  ${GREEN}Tritos instalado correctamente.${NC}"
            echo ""
            echo -e "  Para usar:"
            echo -e "    ${BOLD}tritos${NC}                    — shell ternario"
            echo -e "    ${BOLD}tritos -c \"science lga\"${NC}  — ejecutar comando"
            echo -e "    ${BOLD}tritos_science${NC}            — suite científica"
            echo ""
            if [ -f /etc/grub.d/40_tritos ]; then
                echo -e "  Para kernel bare-metal:"
                echo -e "    ${BOLD}Reiniciá${NC} y elegí \"Ternary Ancestral Kernel\" en GRUB"
            fi
            echo -e "  ${CYAN}════════════════════════════════════════════${NC}"
            echo ""
            ;;
    esac
}

main "$@"
