; boot/start.asm — Entry point for multiboot kernel
; Sets up stack and passes multiboot args to kernel_main

global _start
extern kernel_main

section .text
_start:
    ; Set up a temporary stack
    mov esp, 0x90000

    ; EAX = multiboot magic (0x2BADB002)
    ; EBX = multiboot info address
    ; Push as arguments for kernel_main(magic, mboot_addr)
    push ebx        ; mboot_addr (2nd arg)
    push eax        ; magic (1st arg)

    call kernel_main

    ; If kernel_main returns, halt
    cli
.hang:
    hlt
    jmp .hang
