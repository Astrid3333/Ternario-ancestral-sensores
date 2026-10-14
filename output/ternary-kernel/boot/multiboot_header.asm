; boot/multiboot_header.asm — Multiboot header para GRUB

MBALIGN  equ 1 << 0
MEMINFO  equ 1 << 1
VIDEO    equ 1 << 2
FLAGS    equ MBALIGN | MEMINFO | VIDEO
MAGIC    equ 0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
    dd 0, 0, 0, 0, 0
    dd 0          ; mode_type: 0 = grafico lineal
    dd 1024       ; ancho
    dd 768        ; alto
    dd 32         ; bits por pixel
