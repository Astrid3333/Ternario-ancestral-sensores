; context_switch.asm — Context switch between processes
; For Ternary Ancestral Kernel

global context_switch
context_switch:
    ; Save current process registers
    ; eax = pointer to old process registers
    ; edx = pointer to new process registers
    
    ; Save old process
    mov [eax + 0],  ebx
    mov [eax + 4],  ecx
    mov [eax + 8],  edx
    mov [eax + 12], esi
    mov [eax + 16], edi
    mov [eax + 20], ebp
    mov [eax + 24], esp
    
    ; Save EIP (return address)
    mov ebx, [esp]
    mov [eax + 28], ebx
    
    ; Save EFLAGS
    pushfd
    pop ebx
    mov [eax + 32], ebx
    
    ; Load new process registers
    mov ebx, [edx + 0]
    mov ecx, [edx + 4]
    mov esi, [edx + 12]
    mov edi, [edx + 16]
    mov ebp, [edx + 20]
    mov esp, [edx + 24]
    
    ; Load EFLAGS
    mov ebx, [edx + 32]
    push ebx
    popfd
    
    ; Jump to new process EIP
    mov eax, [edx + 28]
    push eax
    ret

; Save current CPU state
global save_state
save_state:
    ; eax = pointer to state structure
    mov [eax + 0],  ebx
    mov [eax + 4],  ecx
    mov [eax + 8],  edx
    mov [eax + 12], esi
    mov [eax + 16], edi
    mov [eax + 20], ebp
    
    ; Save return address as EIP
    mov ebx, [esp]
    mov [eax + 28], ebx
    
    ; Save stack pointer
    mov [eax + 24], esp
    
    ; Save EFLAGS
    pushfd
    pop ebx
    mov [eax + 32], ebx
    
    ret

; Load CPU state
global load_state
load_state:
    ; eax = pointer to state structure
    mov ebx, [eax + 0]
    mov ecx, [eax + 4]
    mov edx, [eax + 8]
    mov esi, [eax + 12]
    mov edi, [eax + 16]
    mov ebp, [eax + 20]
    mov esp, [eax + 24]
    
    ; Load EFLAGS
    mov ebx, [eax + 32]
    push ebx
    popfd
    
    ; Jump to saved EIP
    mov eax, [eax + 28]
    push eax
    ret

; Fork: duplicate process
global process_fork_asm
process_fork_asm:
    ; eax = parent PCB pointer
    ; edx = child PCB pointer
    
    ; Copy registers from parent to child
    mov ecx, [eax + 0]   ; ebx
    mov [edx + 0], ecx
    mov ecx, [eax + 4]   ; ecx
    mov [edx + 4], ecx
    mov ecx, [eax + 8]   ; edx
    mov [edx + 8], ecx
    mov ecx, [eax + 12]  ; esi
    mov [edx + 12], ecx
    mov ecx, [eax + 16]  ; edi
    mov [edx + 16], ecx
    mov ecx, [eax + 20]  ; ebp
    mov [edx + 20], ecx
    mov ecx, [eax + 24]  ; esp
    mov [edx + 24], ecx
    mov ecx, [eax + 28]  ; eip
    mov [edx + 28], ecx
    mov ecx, [eax + 32]  ; eflags
    mov [edx + 32], ecx
    
    ; Child gets return value of 0
    mov dword [edx + 0], 0   ; ebx = 0 in child
    
    ; Parent gets child PID in eax
    movzx ecx, byte [edx + 100]  ; child PID (offset in PCB)
    mov [eax + 0], ecx           ; eax = child PID in parent
    
    ret
