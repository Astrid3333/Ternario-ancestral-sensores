; ternary_boot.asm — Boot loader para kernel ternario ancestral
; Ensamblado con NASM: nasm -f bin boot.asm -o boot.bin

[bits 16]
[org 0x7C00]

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov [boot_drive], dl

    mov ah, 0x00
    mov al, 0x03
    int 0x10

    mov si, msg_boot
    call print_string

    ; Load kernel: read 16 sectors (8KB) from sector 2 to 0x1000:0x0000
    mov ax, 0x1000
    mov es, ax
    mov bx, 0x0000

    mov ah, 0x02
    mov al, 16          ; 16 sectors = 8KB
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    ; Second read: 16 more sectors to 0x1000:0x2000 (offset 8KB)
    mov bx, 0x2000

    mov ah, 0x02
    mov al, 16          ; 16 sectors = 8KB (total 16KB, more than enough)
    mov ch, 0
    mov cl, 18          ; sector 18 (after first 16+1 boot)
    mov dh, 0
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    jmp 0x1000:0x0000

disk_error:
    mov si, msg_error
    call print_string

halt:
    cli
    hlt
    jmp halt

print_string:
    push ax
    push bx
.loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp .loop
.done:
    pop bx
    pop ax
    ret

boot_drive: db 0
msg_boot:   db 'Ternary Ancestral Kernel v0.1', 13, 10
            db 'Loading kernel...', 13, 10, 0
msg_error:  db 'Disk read error!', 13, 10, 0

times 510-($-$$) db 0
dw 0xAA55
