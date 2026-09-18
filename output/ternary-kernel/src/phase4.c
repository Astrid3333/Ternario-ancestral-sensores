/**
 * phase4.c — Fase 4: VM Ternaria, Lógica, Compilador, IA
 */

#include "../include/ternary.h"

// =============================================================================
// VM TERNARIA — Máquina virtual de bytecode ternario
// =============================================================================

#define VM_STACK_SIZE 256
#define VM_MEM_SIZE   1024
#define VM_MAX_CODE   4096

// Opcodes ternarios (cada instrucción = 1 trit = 3 valores posibles por byte)
#define VM_NOP      0x00
#define VM_HALT     0x01
#define VM_PUSH     0x02  // Push literal
#define VM_POP      0x03
#define VM_DUP      0x04
#define VM_ADD      0x10
#define VM_SUB      0x11
#define VM_MUL      0x12
#define VM_DIV      0x13
#define VM_MOD      0x14
#define VM_AND      0x20
#define VM_OR       0x21
#define VM_XOR      0x22
#define VM_NOT      0x23
#define VM_SHL      0x24
#define VM_SHR      0x25
#define VM_EQ       0x30
#define VM_NEQ      0x31
#define VM_LT       0x32
#define VM_GT       0x33
#define VM_JMP      0x40  // Jump unconditional
#define VM_JZ       0x41  // Jump if zero
#define VM_JNZ      0x42  // Jump if not zero
#define VM_CALL     0x50
#define VM_RET      0x51
#define VM_LOAD     0x60  // Load from memory
#define VM_STORE    0x61  // Store to memory
#define VM_PRINT    0x70  // Print top of stack
#define VM_PRINTT   0x71  // Print as ternary

// VM state
typedef struct {
    uint8_t code[VM_MAX_CODE];
    uint32_t code_size;
    int32_t stack[VM_STACK_SIZE];
    uint32_t sp;           // Stack pointer
    uint32_t pc;           // Program counter
    int32_t memory[VM_MEM_SIZE];
    uint8_t running;
    char output[2048];
    int output_len;
} vm_t;

static vm_t vm;

// VM Initialize
void vm_init(void) {
    vm.code_size = 0;
    vm.sp = 0;
    vm.pc = 0;
    vm.running = 0;
    vm.output_len = 0;
    vm.output[0] = 0;
    
    for (int i = 0; i < VM_STACK_SIZE; i++) vm.stack[i] = 0;
    for (int i = 0; i < VM_MEM_SIZE; i++) vm.memory[i] = 0;
}

// VM Print helper
static void vm_print(const char* str) {
    while (*str && vm.output_len < 2045) {
        vm.output[vm.output_len++] = *str++;
    }
    vm.output[vm.output_len] = 0;
}

// VM Stack operations
static int32_t vm_pop(void) {
    if (vm.sp == 0) return 0;
    return vm.stack[--vm.sp];
}

static void vm_push(int32_t val) {
    if (vm.sp < VM_STACK_SIZE) {
        vm.stack[vm.sp++] = val;
    }
}

// VM Execute bytecode
int vm_execute(const uint8_t* code, uint32_t size) {
    vm_init();
    vm.code_size = size < VM_MAX_CODE ? size : VM_MAX_CODE;
    for (uint32_t i = 0; i < vm.code_size; i++) {
        vm.code[i] = code[i];
    }
    
    vm.running = 1;
    
    while (vm.running && vm.pc < vm.code_size) {
        uint8_t opcode = vm.code[vm.pc++];
        
        switch (opcode) {
            case VM_NOP:
                break;
                
            case VM_HALT:
                vm.running = 0;
                break;
                
            case VM_PUSH: {
                int32_t val = 0;
                // Read 4-byte little-endian value
                for (int i = 0; i < 4 && vm.pc < vm.code_size; i++) {
                    val |= ((int32_t)vm.code[vm.pc++]) << (i * 8);
                }
                vm_push(val);
                break;
            }
                
            case VM_POP:
                vm_pop();
                break;
                
            case VM_DUP: {
                int32_t val = vm_pop();
                vm_push(val);
                vm_push(val);
                break;
            }
                
            case VM_ADD: {
                int32_t b = vm_pop();
                int32_t a = vm_pop();
                vm_push(a + b);
                break;
            }
                
            case VM_SUB: {
                int32_t b = vm_pop();
                int32_t a = vm_pop();
                vm_push(a - b);
                break;
            }
                
            case VM_MUL: {
                int32_t b = vm_pop();
                int32_t a = vm_pop();
                vm_push(a * b);
                break;
            }
                
            case VM_DIV: {
                int32_t b = vm_pop();
                int32_t a = vm_pop();
                vm_push(b != 0 ? a / b : 0);
                break;
            }
                
            case VM_MOD: {
                int32_t b = vm_pop();
                int32_t a = vm_pop();
                vm_push(b != 0 ? a % b : 0);
                break;
            }
                
            case VM_AND: {
                int32_t b = vm_pop();
                int32_t a = vm_pop();
                vm_push(a & b);
                break;
            }
                
            case VM_OR: {
                int32_t b = vm_pop();
                int32_t a = vm_pop();
                vm_push(a | b);
                break;
            }
                
            case VM_XOR: {
                int32_t b = vm_pop();
                int32_t a = vm_pop();
                vm_push(a ^ b);
                break;
            }
                
            case VM_NOT: {
                int32_t a = vm_pop();
                vm_push(!a);
                break;
            }
                
            case VM_EQ: {
                int32_t b = vm_pop();
                int32_t a = vm_pop();
                vm_push(a == b ? 1 : 0);
                break;
            }
                
            case VM_NEQ: {
                int32_t b = vm_pop();
                int32_t a = vm_pop();
                vm_push(a != b ? 1 : 0);
                break;
            }
                
            case VM_LT: {
                int32_t b = vm_pop();
                int32_t a = vm_pop();
                vm_push(a < b ? 1 : 0);
                break;
            }
                
            case VM_GT: {
                int32_t b = vm_pop();
                int32_t a = vm_pop();
                vm_push(a > b ? 1 : 0);
                break;
            }
                
            case VM_JMP: {
                int32_t addr = 0;
                for (int i = 0; i < 4 && vm.pc < vm.code_size; i++) {
                    addr |= ((int32_t)vm.code[vm.pc++]) << (i * 8);
                }
                vm.pc = addr;
                break;
            }
                
            case VM_JZ: {
                int32_t addr = 0;
                for (int i = 0; i < 4 && vm.pc < vm.code_size; i++) {
                    addr |= ((int32_t)vm.code[vm.pc++]) << (i * 8);
                }
                int32_t val = vm_pop();
                if (val == 0) vm.pc = addr;
                break;
            }
                
            case VM_JNZ: {
                int32_t addr = 0;
                for (int i = 0; i < 4 && vm.pc < vm.code_size; i++) {
                    addr |= ((int32_t)vm.code[vm.pc++]) << (i * 8);
                }
                int32_t val = vm_pop();
                if (val != 0) vm.pc = addr;
                break;
            }
                
            case VM_LOAD: {
                int32_t addr = vm_pop();
                if (addr >= 0 && addr < VM_MEM_SIZE) {
                    vm_push(vm.memory[addr]);
                } else {
                    vm_push(0);
                }
                break;
            }
                
            case VM_STORE: {
                int32_t val = vm_pop();
                int32_t addr = vm_pop();
                if (addr >= 0 && addr < VM_MEM_SIZE) {
                    vm.memory[addr] = val;
                }
                break;
            }
                
            case VM_PRINT: {
                int32_t val = vm_pop();
                char buf[16];
                { char nb[8]; num_to_str(val, nb); vm_print(nb); }
                vm_print("\n");
                break;
            }
                
            case VM_PRINTT: {
                int32_t val = vm_pop();
                char buf[32];
                // Convert to ternary
                if (val == 0) {
                    vm_print("0");
                } else {
                    int num = val < 0 ? -val : val;
                    int len = 0;
                    char temp[32];
                    while (num > 0) {
                        int rem = num % 3;
                        num = num / 3;
                        if (rem == 2) { temp[len++] = '-'; num++; }
                        else { temp[len++] = '0' + rem; }
                    }
                    if (val < 0) vm_print("-");
                    for (int i = len - 1; i >= 0; i--) {
                        char s[2] = {temp[i], 0};
                        vm_print(s);
                    }
                }
                vm_print("\n");
                break;
            }
                
            default:
                vm_print("  Unknown opcode: ");
                { char nb[4]; num_to_str(opcode, nb); vm_print(nb); }
                vm_print("\n");
                vm.running = 0;
                break;
        }
    }
    
    // Print output
    vga_puts(vm.output);
    
    return 0;
}

// Compile ternary source to bytecode (simplified)
int vm_compile(const char* source, uint8_t* bytecode, int* size) {
    int pos = 0;
    const char* p = source;
    
    while (*p && pos < VM_MAX_CODE - 5) {
        // Skip whitespace
        while (*p == ' ' || *p == '\n' || *p == '\r') p++;
        
        if (*p == 0) break;
        
        // PUSH <number>
        if (strncmp_t(p, "push", 4) == 0) {
            p += 4;
            while (*p == ' ') p++;
            
            int val = 0;
            int neg = 0;
            if (*p == '-') { neg = 1; p++; }
            while (*p >= '0' && *p <= '9') {
                val = val * 10 + (*p - '0');
                p++;
            }
            if (neg) val = -val;
            
            bytecode[pos++] = VM_PUSH;
            bytecode[pos++] = val & 0xFF;
            bytecode[pos++] = (val >> 8) & 0xFF;
            bytecode[pos++] = (val >> 16) & 0xFF;
            bytecode[pos++] = (val >> 24) & 0xFF;
            continue;
        }
        
        // ADD
        if (strncmp_t(p, "add", 3) == 0) { bytecode[pos++] = VM_ADD; p += 3; continue; }
        // SUB
        if (strncmp_t(p, "sub", 3) == 0) { bytecode[pos++] = VM_SUB; p += 3; continue; }
        // MUL
        if (strncmp_t(p, "mul", 3) == 0) { bytecode[pos++] = VM_MUL; p += 3; continue; }
        // DIV
        if (strncmp_t(p, "div", 3) == 0) { bytecode[pos++] = VM_DIV; p += 3; continue; }
        // POP
        if (strncmp_t(p, "pop", 3) == 0) { bytecode[pos++] = VM_POP; p += 3; continue; }
        // DUP
        if (strncmp_t(p, "dup", 3) == 0) { bytecode[pos++] = VM_DUP; p += 3; continue; }
        // PRINT
        if (strncmp_t(p, "print", 5) == 0) { bytecode[pos++] = VM_PRINT; p += 5; continue; }
        // PRINTT
        if (strncmp_t(p, "printt", 6) == 0) { bytecode[pos++] = VM_PRINTT; p += 6; continue; }
        // HALT
        if (strncmp_t(p, "halt", 4) == 0) { bytecode[pos++] = VM_HALT; p += 4; continue; }
        
        // Skip unknown
        while (*p && *p != '\n') p++;
    }
    
    bytecode[pos++] = VM_HALT;
    *size = pos;
    return 0;
}

// Command: vm
void cmd_vm(const char* args) {
    if (args[0] == 0) {
        vga_puts("\n  [Ternary VM v0.1]\n\n");
        vga_puts("  Commands:\n");
        vga_puts("    vm run <file.tbc>  - Run bytecode file\n");
        vga_puts("    vm asm <file.tri>  - Assemble source\n");
        vga_puts("    vm demo            - Run demo program\n\n");
        vga_puts("  Bytecode instructions:\n");
        vga_puts("    push <n>  Push number\n");
        vga_puts("    add/sub/mul/div  Arithmetic\n");
        vga_puts("    print     Print as decimal\n");
        vga_puts("    printt    Print as ternary\n");
        vga_puts("    halt      Stop execution\n\n");
        return;
    }
    
    if (strcmp_t(args, "demo") == 0) {
        vga_puts("\n  [VM Demo: 5 + 3 = 8]\n\n");
        
        uint8_t code[] = {
            VM_PUSH, 5, 0, 0, 0,
            VM_PUSH, 3, 0, 0, 0,
            VM_ADD,
            VM_PRINTT,
            VM_PRINT,
            VM_HALT
        };
        
        vm_execute(code, sizeof(code));
        return;
    }
    
    if (strncmp_t(args, "asm", 3) == 0) {
        const char* filename = args + 4;
        
        int fd = fs_open(filename, 0);
        if (fd < 0) {
            vga_puts("  Error: file not found\n");
            return;
        }
        
        int32_t size = fs_get_size(filename);
        char buf[4096];
        int32_t read = fs_read(fd, (uint8_t*)buf, size < 4095 ? size : 4095);
        fs_close(fd);
        
        if (read <= 0) return;
        buf[read] = 0;
        
        uint8_t bytecode[VM_MAX_CODE];
        int bytecode_size = 0;
        
        vm_compile(buf, bytecode, &bytecode_size);
        
        vga_puts("  Compiled ");
        { char nb[8]; num_to_str(bytecode_size, nb); vga_puts(nb); }
        vga_puts(" bytes of bytecode\n");
        
        vm_execute(bytecode, bytecode_size);
        return;
    }
}

// =============================================================================
// LÓGICA TERNARIA — Kleene y Łukasiewicz
// =============================================================================

// Ternary logic values: 0=false, 1=unknown, 2=true
#define T_FALSE  0
#define T_UNKNOWN 1
#define T_TRUE   2

// Convert ternary value to string
static const char* ternary_val_str(int val) {
    switch (val) {
        case 0: return "False (0)";
        case 1: return "Unknown (1)";
        case 2: return "True (2)";
        default: return "?";
    }
}

// Kleene logic operations
static int kleene_and(int a, int b) {
    if (a == 0 || b == 0) return 0;
    if (a == 2 && b == 2) return 2;
    return 1; // Unknown
}

static int kleene_or(int a, int b) {
    if (a == 2 || b == 2) return 2;
    if (a == 0 && b == 0) return 0;
    return 1; // Unknown
}

static int kleene_not(int a) {
    return 2 - a; // Flip: 0→2, 1→1, 2→0
}

// Łukasiewicz logic (fuzzy)
static int luk_and(int a, int b) {
    int sum = a + b;
    if (sum <= 2) return 0;
    return sum - 2;
}

static int luk_or(int a, int b) {
    int sum = a + b;
    if (sum >= 2) return 2;
    return sum;
}

static int luk_implies(int a, int b) {
    if (a <= b) return 2;
    return 2 - a + b;
}

// Command: logic
void cmd_logic(const char* args) {
    vga_puts("\n  [Ternary Logic]\n\n");
    
    vga_puts("  Values: 0=False, 1=Unknown, 2=True\n\n");
    
    // Kleene truth tables
    vga_puts("  === Kleene Logic ===\n\n");
    vga_puts("  AND truth table:\n");
    vga_puts("    A B | A∧B\n");
    vga_puts("    ----|----\n");
    vga_puts("    0 0 | "); { char nb[2]; num_to_str(kleene_and(0,0), nb); vga_puts(nb); vga_puts("\n"); }
    vga_puts("    0 1 | "); { char nb[2]; num_to_str(kleene_and(0,1), nb); vga_puts(nb); vga_puts("\n"); }
    vga_puts("    0 2 | "); { char nb[2]; num_to_str(kleene_and(0,2), nb); vga_puts(nb); vga_puts("\n"); }
    vga_puts("    1 0 | "); { char nb[2]; num_to_str(kleene_and(1,0), nb); vga_puts(nb); vga_puts("\n"); }
    vga_puts("    1 1 | "); { char nb[2]; num_to_str(kleene_and(1,1), nb); vga_puts(nb); vga_puts("\n"); }
    vga_puts("    1 2 | "); { char nb[2]; num_to_str(kleene_and(1,2), nb); vga_puts(nb); vga_puts("\n"); }
    vga_puts("    2 0 | "); { char nb[2]; num_to_str(kleene_and(2,0), nb); vga_puts(nb); vga_puts("\n"); }
    vga_puts("    2 1 | "); { char nb[2]; num_to_str(kleene_and(2,1), nb); vga_puts(nb); vga_puts("\n"); }
    vga_puts("    2 2 | "); { char nb[2]; num_to_str(kleene_and(2,2), nb); vga_puts(nb); vga_puts("\n"); }
    
    vga_puts("\n  OR truth table:\n");
    vga_puts("    A B | A∨B\n");
    vga_puts("    ----|----\n");
    vga_puts("    0 0 | "); { char nb[2]; num_to_str(kleene_or(0,0), nb); vga_puts(nb); vga_puts("\n"); }
    vga_puts("    0 1 | "); { char nb[2]; num_to_str(kleene_or(0,1), nb); vga_puts(nb); vga_puts("\n"); }
    vga_puts("    0 2 | "); { char nb[2]; num_to_str(kleene_or(0,2), nb); vga_puts(nb); vga_puts("\n"); }
    vga_puts("    1 0 | "); { char nb[2]; num_to_str(kleene_or(1,0), nb); vga_puts(nb); vga_puts("\n"); }
    vga_puts("    1 1 | "); { char nb[2]; num_to_str(kleene_or(1,1), nb); vga_puts(nb); vga_puts("\n"); }
    vga_puts("    1 2 | "); { char nb[2]; num_to_str(kleene_or(1,2), nb); vga_puts(nb); vga_puts("\n"); }
    vga_puts("    2 0 | "); { char nb[2]; num_to_str(kleene_or(2,0), nb); vga_puts(nb); vga_puts("\n"); }
    vga_puts("    2 1 | "); { char nb[2]; num_to_str(kleene_or(2,1), nb); vga_puts(nb); vga_puts("\n"); }
    vga_puts("    2 2 | "); { char nb[2]; num_to_str(kleene_or(2,2), nb); vga_puts(nb); vga_puts("\n"); }
    
    vga_puts("\n  NOT operation:\n");
    vga_puts("    ¬0 = "); { char nb[2]; num_to_str(kleene_not(0), nb); vga_puts(nb); vga_puts(" (True)\n"); }
    vga_puts("    ¬1 = "); { char nb[2]; num_to_str(kleene_not(1), nb); vga_puts(nb); vga_puts(" (Unknown)\n"); }
    vga_puts("    ¬2 = "); { char nb[2]; num_to_str(kleene_not(2), nb); vga_puts(nb); vga_puts(" (False)\n"); }
    
    // Łukasiewicz
    vga_puts("\n  === Łukasiewicz Logic (Fuzzy) ===\n\n");
    vga_puts("  A→B (implication):\n");
    vga_puts("    A B | A→B\n");
    vga_puts("    ----|----\n");
    for (int a = 0; a <= 2; a++) {
        for (int b = 0; b <= 2; b++) {
            vga_puts("    ");
            { char nb[2]; num_to_str(a, nb); vga_puts(nb); }
            vga_puts(" ");
            { char nb[2]; num_to_str(b, nb); vga_puts(nb); }
            vga_puts(" | ");
            { char nb[2]; num_to_str(luk_implies(a, b), nb); vga_puts(nb); }
            vga_puts("\n");
        }
    }
    
    vga_puts("\n  Key differences from binary:\n");
    vga_puts("    - Unknown (1) is a valid truth value\n");
    vga_puts("    - A ∧ ¬A can be Unknown (not always false)\n");
    vga_puts("    - Useful for partial information\n\n");
}

// =============================================================================
// COMPILADOR TRI → BYTECODE
// =============================================================================

// Command: compile
void cmd_compile(const char* args) {
    if (args[0] == 0) {
        vga_puts("\n  [Ternary Compiler]\n\n");
        vga_puts("  Usage:\n");
        vga_puts("    compile <file.tri>     - Compile to bytecode\n");
        vga_puts("    compile run <file.tri> - Compile and run\n\n");
        return;
    }
    
    if (strncmp_t(args, "run", 3) == 0) {
        const char* filename = args + 4;
        
        int fd = fs_open(filename, 0);
        if (fd < 0) {
            vga_puts("  Error: file not found\n");
            return;
        }
        
        int32_t size = fs_get_size(filename);
        char buf[4096];
        int32_t read = fs_read(fd, (uint8_t*)buf, size < 4095 ? size : 4095);
        fs_close(fd);
        
        if (read <= 0) return;
        buf[read] = 0;
        
        uint8_t bytecode[VM_MAX_CODE];
        int bytecode_size = 0;
        
        vm_compile(buf, bytecode, &bytecode_size);
        
        vga_puts("  Compiled ");
        { char nb[8]; num_to_str(bytecode_size, nb); vga_puts(nb); }
        vga_puts(" bytes\n");
        
        vm_execute(bytecode, bytecode_size);
        return;
    }
    
    vga_puts("  Usage: compile <file.tri>\n");
}

// =============================================================================
// ASISTENTE IA TERNARIO
// =============================================================================

// Command: ai
void cmd_ai_tri(const char* args) {
    if (args[0] == 0) {
        vga_puts("\n  [Ternary AI Assistant]\n\n");
        vga_puts("  I can help you with ternary programming!\n\n");
        vga_puts("  Topics:\n");
        vga_puts("    ai help     - Show this help\n");
        vga_puts("    ai convert  - Convert numbers\n");
        vga_puts("    ai logic    - Ternary logic\n");
        vga_puts("    ai program  - Programming tips\n");
        vga_puts("    ai learn    - Learning path\n\n");
        return;
    }
    
    if (strcmp_t(args, "help") == 0 || strcmp_t(args, "") == 0) {
        vga_puts("\n  Ternary AI: I can help you learn!\n\n");
        vga_puts("  Try these commands:\n");
        vga_puts("    conv 255    - Convert a number\n");
        vga_puts("    logic       - Learn ternary logic\n");
        vga_puts("    tutorial 1  - Start learning\n");
        vga_puts("    vm demo     - See VM in action\n");
        vga_puts("    bench       - Compare binary vs ternary\n\n");
        return;
    }
    
    if (strcmp_t(args, "learn") == 0) {
        vga_puts("\n  [Ternary Learning Path]\n\n");
        vga_puts("  1. Start with: tutorial 1\n");
        vga_puts("     Learn what ternary is\n\n");
        vga_puts("  2. Practice: conv <numbers>\n");
        vga_puts("     Convert between bases\n\n");
        vga_puts("  3. Code: tri examples\n");
        vga_puts("     Write ternary programs\n\n");
        vga_puts("  4. Logic: logic\n");
        vga_puts("     Understand ternary logic\n\n");
        vga_puts("  5. Advanced: vm demo\n");
        vga_puts("     See bytecode execution\n\n");
        vga_puts("  6. Compare: bench\n");
        vga_puts("     See efficiency gains\n\n");
        return;
    }
    
    if (strcmp_t(args, "program") == 0) {
        vga_puts("\n  [Ternary Programming Tips]\n\n");
        vga_puts("  Variables:\n");
        vga_puts("    var x = 10;       // Decimal\n");
        vga_puts("    var y = +0-;      // Ternary notation\n\n");
        vga_puts("  Print:\n");
        vga_puts("    print(x);         // Decimal output\n");
        vga_puts("    printT(x);        // Ternary output\n\n");
        vga_puts("  Loops:\n");
        vga_puts("    for (var i = 0; i < 10; i++) { ... }\n");
        vga_puts("    while (x > 0) { ... }\n\n");
        vga_puts("  Example program:\n");
        vga_puts("    var sum = 0;\n");
        vga_puts("    for (var i = 1; i <= 10; i++) {\n");
        vga_puts("      sum = sum + i;\n");
        vga_puts("    }\n");
        vga_puts("    print(sum);  // 55\n\n");
        return;
    }
    
    vga_puts("  Unknown topic. Try: ai help\n");
}

// =============================================================================
// STATUS
// =============================================================================

void phase4_status(void) {
    vga_puts("\n  [Phase 4: Advanced Ternary]\n\n");
    vga_puts("  Commands:\n");
    vga_puts("    vm <cmd>      Ternary virtual machine\n");
    vga_puts("    logic         Ternary logic (Kleene/Łukasiewicz)\n");
    vga_puts("    compile <f>   Compile .tri to bytecode\n");
    vga_puts("    ai <topic>    AI assistant\n\n");
}
