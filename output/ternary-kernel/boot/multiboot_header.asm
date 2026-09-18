; boot/multiboot_header.asm — Multiboot header para GRUB
; Debe estar en los primeros 8KB del binario

MBALIGN  equ 1 << 0            ; Alinear módulos en límite de página
MEMINFO  equ 1 << 1            ; Información de memoria
VIDMODE  equ 1 << 2            ; Solicitar información de video
FLAGS    equ MBALIGN | MEMINFO | VIDMODE
MAGIC    equ 0x1BADB002        ; Multiboot magic
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
    
    ; Multiboot header fields (vidmode)
    dd 0    ; header_addr
    dd 0    ; load_addr
    dd 0    ; load_end_addr
    dd 0    ; bss_end_addr
    dd 0    ; entry_addr
    
    ; Video mode request
    dd 0    ; mode_type (0 = linear graphics)
    dd 1024 ; width
    dd 768  ; height
    dd 32   ; bpp (32-bit color)
