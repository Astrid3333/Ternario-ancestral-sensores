/**
 * syscalls.c — Llamadas al sistema para kernel ternario ancestral
 *
 * Interfaz int 0x80 para apps de usuario
 */

#include "../include/ternary.h"

// Syscall numbers
#define SYS_EXIT        0
#define SYS_READ        1
#define SYS_WRITE       2
#define SYS_OPEN        3
#define SYS_CLOSE       4
#define SYS_FORK        5
#define SYS_EXEC        6
#define SYS_WAIT        7
#define SYS_GETPID      8
#define SYS_SLEEP       9
#define SYS_TIME        10
#define SYS_MMAP        11
#define SYS_MUNMAP      12
#define SYS_BRK         13
#define SYS_CHDIR       14
#define SYS_GETCWD      15
#define SYS_MKDIR       16
#define SYS_RMDIR       17
#define SYS_UNLINK      18
#define SYS_RENAME      19
#define SYS_STAT        20
#define SYS_IOCTL       21
#define SYS_DUP         22
#define SYS_DUP2        23
#define SYS_PIPE        24
#define SYS_SOCKET      25
#define SYS_CONNECT     26
#define SYS_BIND        27
#define SYS_LISTEN      28
#define SYS_ACCEPT      29
#define SYS_SEND        30
#define SYS_RECV        31
#define SYS_MAX_SYSCALL 32

// Syscall handler
static uint32_t syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    switch (syscall_num) {
        case SYS_EXIT: {
            process_exit(arg1);
            return 0;
        }
        
        case SYS_READ: {
            // Read from file descriptor
            // arg1 = fd, arg2 = buffer, arg3 = count
            int8_t fd = (int8_t)arg1;
            uint8_t* buf = (uint8_t*)arg2;
            uint32_t count = arg3;
            
            // For now, only support stdin (fd 0)
            if (fd == 0) {
                uint32_t i = 0;
                while (i < count) {
                    if (inb(0x64) & 1) {
                        uint8_t sc = inb(0x60);
                        if (sc & 0x80) continue;
                        if (sc == 0x1C) { buf[i++] = '\n'; break; }
                        if (sc < 58 && sc != 0) {
                            static const char map[] = "??1234567890-=??qwertyuiop[]??asdfghjkl;'`?\\zxcvbnm,./?*? ?";
                            char ch = map[sc];
                            if (ch != '?') buf[i++] = ch;
                        }
                    }
                }
                return i;
            }
            
            return -1;
        }
        
        case SYS_WRITE: {
            // Write to file descriptor
            // arg1 = fd, arg2 = buffer, arg3 = count
            int8_t fd = (int8_t)arg1;
            const uint8_t* buf = (const uint8_t*)arg2;
            uint32_t count = arg3;
            
            // For now, only support stdout (fd 1) and stderr (fd 2)
            if (fd == 1 || fd == 2) {
                for (uint32_t i = 0; i < count; i++) {
                    vga_putc(buf[i]);
                    serial_putc(buf[i]);
                }
                return count;
            }
            
            return -1;
        }
        
        case SYS_OPEN: {
            // Open file
            const char* filename = (const char*)arg1;
            uint32_t flags = arg2;
            
            return fs_open(filename, flags);
        }
        
        case SYS_CLOSE: {
            // Close file
            int8_t fd = (int8_t)arg1;
            fs_close(fd);
            return 0;
        }
        
        case SYS_FORK: {
            // Fork process
            proc_t* parent = process_current();
            
            // Create child process
            int8_t child_pid = process_create("child", parent->eip, parent->flags);
            if (child_pid < 0) return -1;
            
            proc_t* child = process_get(child_pid);
            
            // Copy registers
            child->eax = 0;  // Child gets 0
            child->ebx = parent->ebx;
            child->ecx = parent->ecx;
            child->edx = parent->edx;
            child->esi = parent->esi;
            child->edi = parent->edi;
            child->ebp = parent->ebp;
            child->esp = parent->esp;
            child->eip = parent->eip;
            child->eflags = parent->eflags;
            
            // Copy page directory (simplified)
            child->page_directory = parent->page_directory;
            
            // Parent gets child PID
            return child_pid;
        }
        
        case SYS_EXEC: {
            // Execute program
            const char* filename = (const char*)arg1;
            
            // Try to open and load ELF
            int8_t fd = fs_open(filename, 0);
            if (fd < 0) return -1;
            
            // Read file into memory
            uint8_t buf[32768]; // 32KB max
            int32_t bytes_read = fs_read(fd, buf, sizeof(buf));
            fs_close(fd);
            
            if (bytes_read <= 0) return -1;
            
            // Try to load as ELF
            uint32_t entry_point;
            if (elf_load(buf, bytes_read, &entry_point) == 0) {
                proc_t* proc = process_current();
                proc->eip = entry_point;
                return 0;
            }
            
            return -1;
        }
        
        case SYS_WAIT: {
            // Wait for child
            int32_t status;
            return process_wait(arg1, &status);
        }
        
        case SYS_GETPID: {
            // Get process ID
            return sched_get_current();
        }
        
        case SYS_SLEEP: {
            // Sleep for milliseconds
            uint32_t ms = arg1;
            // Simple delay loop
            for (volatile uint32_t i = 0; i < ms * 10000; i++) {
                asm volatile("nop");
            }
            return 0;
        }
        
        case SYS_TIME: {
            // Get time (simplified)
            return 0;
        }
        
        case SYS_MMAP: {
            // Map memory
            uint32_t addr = arg1;
            uint32_t size = arg2;
            
            // Simple allocation from fixed address
            static uint32_t next_free = 0x1000000;
            uint32_t result = next_free;
            next_free += size;
            
            // Map pages (using paging module flags)
            for (uint32_t i = 0; i < size; i += 0x1000) {
                paging_map_page(result + i, next_free + i, 0x03);
            }
            
            return result;
        }
        
        case SYS_BRK: {
            // Set break (heap end)
            uint32_t addr = arg1;
            // TODO: Implement proper heap
            return addr;
        }
        
        case SYS_CHDIR: {
            // Change directory
            const char* path = (const char*)arg1;
            // TODO: Implement
            return 0;
        }
        
        case SYS_GETCWD: {
            // Get current working directory
            char* buf = (char*)arg1;
            strcpy_t(buf, "/");
            return 0;
        }
        
        case SYS_IOCTL: {
            // I/O control
            // TODO: Implement
            return 0;
        }
        
        default:
            return -1;
    }
}

// Syscall dispatcher (called from interrupt handler)
void syscall_dispatch(uint32_t* regs) {
    uint32_t syscall_num = regs[0]; // eax
    uint32_t arg1 = regs[1];        // ebx
    uint32_t arg2 = regs[2];        // ecx
    uint32_t arg3 = regs[3];        // edx
    
    uint32_t result = syscall_handler(syscall_num, arg1, arg2, arg3);
    
    regs[0] = result; // Return value in eax
}

// Initialize syscalls
void syscalls_init(void) {
    vga_puts("[SYSCALL] Initializing syscall interface...\n");
    
    // Install syscall interrupt handler (int 0x80)
    // This would be done in IDT setup
    // idt_set_gate(0x80, (uint32_t)syscall_interrupt_handler, 0x08, 0xEE);
    
    vga_puts("[SYSCALL] Syscall interface ready\n");
    vga_puts("[SYSCALL] ");
    { char nb[8]; num_to_str(SYS_MAX_SYSCALL, nb); vga_puts(nb); }
    vga_puts(" syscalls available\n");
}

// Syscall stubs for user programs (in libc)
// These would be in user-space libc
#if 0
// Example syscall stubs
int exit(int status) {
    asm volatile("int $0x80" : : "a"(SYS_EXIT), "b"(status));
}

int read(int fd, void* buf, size_t count) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_READ), "b"(fd), "c"(buf), "d"(count));
    return result;
}

int write(int fd, const void* buf, size_t count) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_WRITE), "b"(fd), "c"(buf), "d"(count));
    return result;
}

int open(const char* pathname, int flags) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_OPEN), "b"(pathname), "c"(flags));
    return result;
}

int close(int fd) {
    asm volatile("int $0x80" : : "a"(SYS_CLOSE), "b"(fd));
}

pid_t fork(void) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_FORK));
    return result;
}

int exec(const char* pathname, char* const argv[]) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_EXEC), "b"(pathname));
    return result;
}

pid_t getpid(void) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_GETPID));
    return result;
}

unsigned int sleep(unsigned int seconds) {
    asm volatile("int $0x80" : : "a"(SYS_SLEEP), "b"(seconds * 1000));
}
#endif
