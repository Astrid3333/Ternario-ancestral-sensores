#!/bin/bash
# build-tritos-distro.sh — Generador de Distro Tritos OS
# Basada en Debian minimal + Blender + octave-mcp + GUI Tritos
#
# Requisitos: sudo, debootstrap, squashfs-tools, xorriso
# Uso: sudo ./build-tritos-distro.sh

set -e

WORKDIR="/tmp/tritos-distro-build"
ISO_NAME="tritos-os-1.0-amd64.iso"
ISO_DIR="${WORKDIR}/iso"
ROOTFS="${WORKDIR}/rootfs"

echo "============================================"
echo "  TRITOS OS — Generador de Distro"
echo "  Basada en Debian 12 (Bookworm) minimal"
echo "============================================"
echo ""

# Check root
if [ "$EUID" -ne 0 ]; then
    echo "Error: ejecuta con sudo"
    echo "  sudo ./build-tritos-distro.sh"
    exit 1
fi

# Install dependencies
echo "[1/8] Instalando dependencias..."
apt-get update -qq
apt-get install -y -qq debootstrap squashfs-tools xorriso \
    grub-pc-bin grub-efi-amd64-bin mtools genisoimage

# Clean previous build
rm -rf "${WORKDIR}"
mkdir -p "${ROOTFS}" "${ISO_DIR}"

# Bootstrap Debian minimal
echo "[2/8] Descargando Debian minimal (~200MB)..."
debootstrap --variant=minbase bookworm "${ROOTFS}" \
    http://deb.debian.org/debian

# Configure rootfs
echo "[3/8] Configurando rootfs..."

# fstab
cat > "${ROOTFS}/etc/fstab" << 'EOF'
# <file system> <mount point>   <type>  <options>       <dump>  <pass>
/dev/sda1       /               ext4    errors=remount-ro 0       1
/dev/sda2       /home           ext4    defaults        0       2
EOF

# hostname
echo "tritos" > "${ROOTFS}/etc/hostname"
echo "127.0.0.1 localhost tritos" > "${ROOTFS}/etc/hosts"

# Locale
echo "es_AR.UTF-8 UTF-8" >> "${ROOTFS}/etc/locale.gen"
chroot "${ROOTFS}" locale-gen 2>/dev/null || true

# Network
cat > "${ROOTFS}/etc/network/interfaces" << 'EOF'
auto lo
iface lo inet loopback

auto eth0
iface eth0 inet dhcp
EOF

# APT sources
cat > "${ROOTFS}/etc/apt/sources.list" << 'EOF'
deb http://deb.debian.org/debian bookworm main contrib non-free non-free-firmware
deb http://deb.debian.org/debian bookworm-updates main contrib non-free non-free-firmware
deb http://security.debian.org/debian-security bookworm-security main contrib non-free non-free-firmware
EOF

# Install base packages
echo "[4/8] Instalando paquetes base..."
chroot "${ROOTFS}" apt-get update -qq
chroot "${ROOTFS}" apt-get install -y -qq \
    linux-image-amd64 \
    live-boot \
    systemd-sysv \
    network-manager \
    wget \
    curl \
    git \
    python3 \
    python3-pip \
    python3-gi \
    python3-gi-cairo \
    gir1.2-gtk-3.0 \
    gnome-terminal \
    thunar \
    openbox \
    tint2 \
    nitrogen \
    lightdm \
    sudo \
    nano \
    vim \
    htop \
    tmux

# Install scientific tools
echo "[5/8] Instalando herramientas científicas..."
chroot "${ROOTFS}" apt-get install -y -qq \
    blender \
    octave \
    freecad \
    gimp \
    inkscape \
    qgis \
    paraview \
    numpy \
    scipy \
    sympy

# Install MCP
echo "[6/8] Instalando octave-mcp..."
chroot "${ROOTFS}" pip3 install octave-mcp 2>/dev/null || true

# Install Tritos
echo "[7/8] Instalando Tritos OS..."
TRITOS_DIR=$(dirname "$(realpath "$0")")

# Copy Tritos files
mkdir -p "${ROOTFS}/opt/tritos"
cp -r "${TRITOS_DIR}/tritos_gui.py" "${ROOTFS}/opt/tritos/"
cp -r "${TRITOS_DIR}/tritos_lab.py" "${ROOTFS}/opt/tritos/"
cp -r "${TRITOS_DIR}/tritos_encyclopedia.py" "${ROOTFS}/opt/tritos/"
cp -r "${TRITOS_DIR}/tritos_browser.py" "${ROOTFS}/opt/tritos/"
cp -r "${TRITOS_DIR}/tritos_mcp_client.py" "${ROOTFS}/opt/tritos/"
cp -r "${TRITOS_DIR}/tritos_codec.py" "${ROOTFS}/opt/tritos/"
cp -r "${TRITOS_DIR}/tritos_math_panel.py" "${ROOTFS}/opt/tritos/"
cp -r "${TRITOS_DIR}/tritos_lab/" "${ROOTFS}/opt/tritos/"
chmod +x "${ROOTFS}/opt/tritos/"*.py

# Desktop entry
cat > "${ROOTFS}/usr/share/applications/tritos.desktop" << 'EOF'
[Desktop Entry]
Name=Tritos OS
Comment=Ternary Ancestral Operating System
Exec=python3 /opt/tritos/tritos_gui.py
Icon=utilities-terminal
Terminal=false
Type=Application
Categories=Education;Science;Math;
EOF

# Autostart for openbox
mkdir -p "${ROOTFS}/etc/xdg/openbox"
cat > "${ROOTFS}/etc/xdg/openbox/autostart" << 'EOF'
# Tritos OS autostart
tint2 &
nitrogen --restore &
python3 /opt/tritos/tritos_gui.py &
EOF

# User setup
chroot "${ROOTFS}" useradd -m -s /bin/bash tritos 2>/dev/null || true
echo "tritos:tritos" | chroot "${ROOTFS}" chpasswd
echo "root:tritos" | chroot "${ROOTFS}" chpasswd
echo "tritos ALL=(ALL) NOPASSWD:ALL" > "${ROOTFS}/etc/sudoers.d/tritos"

# Create ISO
echo "[8/8] Generando ISO..."
mkdir -p "${ISO_DIR}/boot/grub"

# Copy kernel
cp "${ROOTFS}/boot/vmlinuz"* "${ISO_DIR}/boot/vmlinuz" 2>/dev/null || true
cp "${ROOTFS}/boot/initrd"* "${ISO_DIR}/boot/initrd.img" 2>/dev/null || true

# GRUB config
cat > "${ISO_DIR}/boot/grub/grub.cfg" << 'EOF'
set timeout=10
set default=0

menuentry "Tritos OS" {
    linux /boot/vmlinuz boot=live quiet splash
    initrd /boot/initrd.img
}

menuentry "Tritos OS (safe mode)" {
    linux /boot/vmlinuz boot=live nomodeset
    initrd /boot/initrd.img
}
EOF

# Create squashfs
mksquashfs "${ROOTFS}" "${ISO_DIR}/filesystem.squashfs" -comp xz -b 1M

# Create initrd for live boot
cat > "${ISO_DIR}/boot/grub/loopback.cfg" << 'EOF'
loopback loop /filesystem.squashfs
set root=(loop)
linux /boot/vmlinuz boot=live
initrd /boot/initrd.img
EOF

# Generate ISO
genisoimage -o "${WORKDIR}/${ISO_NAME}" \
    -b boot/grub/grub.cfg \
    -no-emul-boot \
    -boot-load-size 4 \
    -boot-info-table \
    -eltorito-alt-boot \
    -e filesystem.squashfs \
    -no-emul-boot \
    -R -T \
    "${ISO_DIR}"

# Make USB-bootable
isohybrid --uefi "${WORKDIR}/${ISO_NAME}" 2>/dev/null || true

echo ""
echo "============================================"
echo "  TRITOS OS — Build Completo"
echo "============================================"
echo ""
echo "  ISO: ${WORKDIR}/${ISO_NAME}"
echo "  Tamaño: $(du -h "${WORKDIR}/${ISO_NAME}" | cut -f1)"
echo ""
echo "  Para crear USB:"
echo "    sudo dd if=${WORKDIR}/${ISO_NAME} of=/dev/sdX bs=4M status=progress"
echo ""
echo "  Para probar en QEMU:"
echo "    qemu-system-x86_64 -m 4G -cdrom ${WORKDIR}/${ISO_NAME}"
echo ""
