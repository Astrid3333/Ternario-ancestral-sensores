; boot/multiboot_header.asm — Multiboot header para GRUB
; Debe estar en los primeros 8KB del binario

MBALIGN  equ 1 << 0            ; Alinear módulos en límite de página
MEMINFO  equ 1 << 1            ; Información de memoria
FLAGS    equ MBALIGN | MEMINFO ; Sin VIDMODE (evita crash en GRUB)
MAGIC    equ 0x1BADB002        ; Multiboot magic
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
    
    ; Multiboot header fields (sin vidmode)
    dd 0    ; header_addr
    dd 0    ; load_addr
    dd 0    ; load_end_addr
    dd 0    ; bss_end_addr
    dd 0    ; entry_addr
