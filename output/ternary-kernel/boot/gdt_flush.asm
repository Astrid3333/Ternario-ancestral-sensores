; gdt_flush.asm — Load GDT and reload segment registers
; For Ternary Ancestral Kernel

global gdt_flush
gdt_flush:
    mov eax, [esp + 4]    ; Get GDT pointer
    lgdt [eax]            ; Load GDT
    
    ; Reload segment registers
    mov ax, 0x10          ; Kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Reload CS with far jump
    jmp 0x08:.reload_cs
.reload_cs:
    ret
