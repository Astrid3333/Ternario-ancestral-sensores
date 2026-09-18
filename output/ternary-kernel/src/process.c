/**
 * process.c — Gestión de procesos para kernel ternario ancestral
 *
 * PCB, fork, exec, wait, exit, context switch
 */

#include "../include/ternary.h"

// =============================================================================
// Process Control Block (PCB)
// =============================================================================

// Process states
#define PROC_UNUSED    0
#define PROC_READY     1
#define PROC_RUNNING   2
#define PROC_BLOCKED   3
#define PROC_ZOMBIE    4
#define PROC_WAITING   5

// Process flags
#define PROC_FLAG_KERNEL  0x01
#define PROC_FLAG_USER    0x02

// Maximum processes (from ternary.h)
// MAX_PROCESSES already defined

// Kernel stack size (4KB)
#define KERNEL_STACK_SIZE 4096

// Process table
// =============================================================================

static proc_t processes[MAX_PROCESSES];
static uint8_t process_count = 0;
static uint8_t current_pid = 0;
static uint8_t next_pid = 1;

// Get current process
proc_t* process_current(void) {
    return &processes[current_pid];
}

// Get process by PID
proc_t* process_get(uint8_t pid) {
    if (pid >= MAX_PROCESSES) return 0;
    if (processes[pid].state == PROC_UNUSED) return 0;
    return &processes[pid];
}

// =============================================================================
// Process creation
// =============================================================================

// Allocate a new PID
static uint8_t alloc_pid(void) {
    for (int i = 1; i < MAX_PROCESSES; i++) {
        if (processes[i].state == PROC_UNUSED) {
            return i;
        }
    }
    return 0; // No free slots
}

// Create a new process
int8_t process_create(const char* name, uint32_t entry_point, uint8_t flags) {
    uint8_t pid = alloc_pid();
    if (pid == 0) return -1; // No free slots
    
    proc_t* proc = &processes[pid];
    memset(proc, 0, sizeof(process_t));
    
    proc->pid = pid;
    proc->ppid = current_pid;
    proc->state = PROC_READY;
    proc->flags = flags;
    proc->priority = 16; // Default priority
    proc->ticks = 0;
    
    // Set name
    strncpy(proc->name, name, 31);
    
    // Set instruction pointer
    proc->eip = entry_point;
    
    // Set stack pointer (at end of user stack)
    proc->esp = 0x800000; // 8MB — user stack top
    
    // Set up page directory
    // For now, use kernel's page directory
    proc->page_directory = 0;
    
    // Initialize file descriptors
    for (int i = 0; i < 16; i++) {
        proc->fd[i] = -1;
    }
    
    process_count++;
    
    vga_puts("[PROC] Created process '");
    vga_puts(name);
    vga_puts("' with PID ");
    { char nb[4]; num_to_str(pid, nb); vga_puts(nb); }
    vga_puts("\n");
    
    return pid;
}

// =============================================================================
// Process termination
// =============================================================================

// Exit current process
void process_exit(int32_t status) {
    proc_t* proc = process_current();
    
    vga_puts("[PROC] Process '");
    vga_puts(proc->name);
    vga_puts("' exiting with status ");
    { char nb[8]; num_to_str(status, nb); vga_puts(nb); }
    vga_puts("\n");
    
    // Close file descriptors
    for (int i = 0; i < 16; i++) {
        if (proc->fd[i] >= 0) {
            fs_close(proc->fd[i]);
            proc->fd[i] = -1;
        }
    }
    
    // Mark as zombie (parent can wait for it)
    proc->state = PROC_ZOMBIE;
    proc->exit_status = status;
    
    // If no parent waiting, clean up immediately
    if (proc->ppid == 0 || processes[proc->ppid].state == PROC_UNUSED) {
        proc->state = PROC_UNUSED;
        process_count--;
    }
    
    // Switch to next process if this was current
    if (proc->pid == current_pid) {
        process_schedule();
    }
}

// =============================================================================
// Process waiting
// =============================================================================

// Wait for child process
int8_t process_wait(uint8_t child_pid, int32_t* status) {
    proc_t* proc = process_current();
    
    // Find child
    if (child_pid == 0) {
        // Wait for any child
        for (int i = 1; i < MAX_PROCESSES; i++) {
            if (processes[i].ppid == proc->pid &&
                processes[i].state == PROC_ZOMBIE) {
                // Found zombie child
                if (status) *status = processes[i].exit_status;
                uint8_t ret_pid = processes[i].pid;
                processes[i].state = PROC_UNUSED;
                process_count--;
                return ret_pid;
            }
        }
    } else {
        // Wait for specific child
        if (child_pid < MAX_PROCESSES &&
            processes[child_pid].ppid == proc->pid) {
            if (processes[child_pid].state == PROC_ZOMBIE) {
                if (status) *status = processes[child_pid].exit_status;
                processes[child_pid].state = PROC_UNUSED;
                process_count--;
                return child_pid;
            }
            // Child still running — block
            proc->state = PROC_WAITING;
            process_schedule();
            // When we wake up, check again
            if (processes[child_pid].state == PROC_ZOMBIE) {
                if (status) *status = processes[child_pid].exit_status;
                processes[child_pid].state = PROC_UNUSED;
                process_count--;
                return child_pid;
            }
        }
    }
    
    return -1; // No child found
}

// =============================================================================
// Process scheduling
// =============================================================================

// Schedule next process (round-robin)
void process_schedule(void) {
    uint8_t start = current_pid;
    
    // Find next ready process
    for (int i = 0; i < MAX_PROCESSES; i++) {
        uint8_t pid = (start + i + 1) % MAX_PROCESSES;
        if (pid == 0) pid = 1; // Skip PID 0 (idle)
        
        if (processes[pid].state == PROC_READY ||
            processes[pid].state == PROC_RUNNING) {
            if (pid != current_pid) {
                process_switch(pid);
            }
            return;
        }
    }
    
    // No ready process — run idle (PID 0)
    if (current_pid != 0) {
        process_switch(0);
    }
}

// Switch to process
void process_switch(uint8_t new_pid) {
    if (new_pid >= MAX_PROCESSES) return;
    if (processes[new_pid].state == PROC_UNUSED) return;
    
    proc_t* old_proc = process_current();
    proc_t* new_proc = &processes[new_pid];
    
    // Save old process state
    if (old_proc->state == PROC_RUNNING) {
        old_proc->state = PROC_READY;
    }
    
    // Restore new process state
    new_proc->state = PROC_RUNNING;
    current_pid = new_pid;
    
    // Switch page directory if needed
    if (new_proc->page_directory && new_proc->page_directory != old_proc->page_directory) {
        asm volatile("mov %0, %%cr3" : : "r"(new_proc->page_directory));
    }
    
    // Context switch (would save/restore registers here)
    // For now, just update the instruction pointer
}

// =============================================================================
// Process init
// =============================================================================

// Idle process (PID 0)
static void idle_process(void) {
    while (1) {
        asm volatile("hlt");
    }
}

// Initialize process system
void process_init(void) {
    vga_puts("[PROC] Initializing process management...\n");
    
    // Clear process table
    for (int i = 0; i < MAX_PROCESSES; i++) {
        processes[i].state = PROC_UNUSED;
        processes[i].pid = 0;
    }
    
    // Create idle process (PID 0)
    proc_t* idle = &processes[0];
    idle->pid = 0;
    idle->ppid = 0;
    idle->state = PROC_RUNNING;
    idle->flags = PROC_FLAG_KERNEL;
    idle->priority = 31; // Lowest priority
    idle->eip = (uint32_t)idle_process;
    idle->esp = 0x700000; // Idle stack
    strncpy(idle->name, "idle", 4);
    
    current_pid = 0;
    process_count = 1;
    
    vga_puts("[PROC] Process management ready\n");
    vga_puts("[PROC] ");
    { char nb[4]; num_to_str(MAX_PROCESSES, nb); vga_puts(nb); }
    vga_puts(" process slots available\n");
}

// =============================================================================
// Process info
// =============================================================================

// List all processes
void process_list(void) {
    vga_puts("\n  PID  NAME                 STATE      CPU\n");
    vga_puts("  ---  ----                 -----      ---\n");
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].state != PROC_UNUSED) {
            vga_puts("  ");
            { char nb[4]; num_to_str(i, nb); vga_puts(nb); }
            vga_puts("   ");
            vga_puts(processes[i].name);
            
            // Pad name
            int len = strlen(processes[i].name);
            while (len < 20) { vga_putc(' '); len++; }
            
            // State
            switch (processes[i].state) {
                case PROC_READY:   vga_puts("READY"); break;
                case PROC_RUNNING: vga_puts("RUNNING"); break;
                case PROC_BLOCKED: vga_puts("BLOCKED"); break;
                case PROC_ZOMBIE:  vga_puts("ZOMBIE"); break;
                case PROC_WAITING: vga_puts("WAITING"); break;
                default:           vga_puts("UNKNOWN"); break;
            }
            
            // CPU ticks
            vga_puts("    ");
            { char nb[8]; num_to_str(processes[i].ticks, nb); vga_puts(nb); }
            vga_puts("\n");
        }
    }
    
    vga_puts("\n  Total: ");
    { char nb[4]; num_to_str(process_count, nb); vga_puts(nb); }
    vga_puts(" processes\n");
}

// Get process count
uint8_t process_get_count(void) {
    return process_count;
}

// Get current PID
uint8_t process_get_pid(void) {
    return current_pid;
}
