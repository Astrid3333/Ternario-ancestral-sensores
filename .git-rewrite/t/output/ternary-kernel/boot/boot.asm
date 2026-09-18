; ternary_boot.asm — Boot loader para kernel ternario ancestral
; Ensamblado con NASM: nasm -f bin boot.asm -o boot.bin

[bits 16]           ; Modo real de 16 bits
[org 0x7C00]        ; Dirección de carga del bootloader

; =============================================================================
; BOOT LOADER — 512 bytes (sector completo)
; =============================================================================

start:
    ; Configurar segmentos
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00      ; Stack justo debajo del bootloader
    
    ; Guardar disco de arranque
    mov [boot_drive], dl
    
    ; Limpiar pantalla
    mov ah, 0x00
    mov al, 0x03        ; Modo texto 80x25
    int 0x10
    
    ; Mostrar mensaje de arranque
    mov si, msg_boot
    call print_string
    
    ; Cargar kernel desde disco
    ; Leer 10 sectores (5KB) desde sector 1 hasta 0x1000:0x0000
    mov ax, 0x1000
    mov es, ax
    mov bx, 0x0000      ; ES:BX = buffer de carga
    
    mov ah, 0x02        ; Función: leer sector
    mov al, 10          ; Número de sectores a leer
    mov ch, 0           ; Cilindro 0
    mov cl, 2           ; Sector 2 (después del bootloader)
    mov dh, 0           ; Cabeza 0
    mov dl, [boot_drive]
    int 0x13
    
    jc disk_error       ; Si hay error, saltar
    
    ; Salto al kernel cargado en 0x1000:0x0000
    jmp 0x1000:0x0000

disk_error:
    mov si, msg_error
    call print_string
    jmp halt

halt:
    cli
    hlt
    jmp halt

; =============================================================================
; Funciones auxiliares
; =============================================================================

print_string:
    push ax
    push bx
.loop:
    lodsb               ; Cargar byte de [SI] a AL
    or al, al           ; ¿Fin de cadena?
    jz .done
    mov ah, 0x0E        ; Función de impresión
    mov bx, 0x0007      ; Atributo: blanco
    int 0x10
    jmp .loop
.done:
    pop bx
    pop ax
    ret

; =============================================================================
; Datos
; =============================================================================

boot_drive: db 0
msg_boot:   db 'Ternary Ancestral Kernel v0.1', 13, 10
            db 'Loading kernel...', 13, 10, 0
msg_error:  db 'Disk read error!', 13, 10, 0

; Padding y firma del bootloader
times 510-($-$$) db 0
dw 0xAA55           ; Firma de arranque
