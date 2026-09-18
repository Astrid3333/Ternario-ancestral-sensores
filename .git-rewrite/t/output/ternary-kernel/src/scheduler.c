/**
 * scheduler.c — Planificador basado en Ciclos Mayas (v2 — corregido)
 * 
 * Fixes:
 * - select_next() no puede retornar PID 0 inválido
 * - sched_kill() libera memoria del proceso
 * - sched_get_info() retorna copia válida
 * - Nombre de hijos corregido (max 3, no 4)
 */

#include "../include/ternary.h"

// =============================================================================
// ESTRUCTURAS
// =============================================================================

typedef enum {
    PROC_DEAD = -1,
    PROC_SLEEPING = 0,
    PROC_ACTIVE = 1
} proc_state_t;

typedef struct {
    maya_pid_t pid;
    proc_state_t state;
    trit_t priority;
    uint8_t memory_block;
    uint16_t cpu_cycles;
    uint8_t quantum;
    uint8_t parent;
    uint8_t children[3];
    uint8_t n_children;
} __attribute__((packed)) process_t;

typedef struct {
    uint32_t global_tick;
    uint8_t current_pid;
    uint8_t tzolkin_day;
    uint8_t haab_day;
    uint8_t active_count;
    trit_t system_load;
} __attribute__((packed)) scheduler_state_t;

// =============================================================================
// VARIABLES GLOBALES
// =============================================================================

static process_t processes[MAX_PROCS];
static scheduler_state_t scheduler;

// =============================================================================
// FUNCIONES
// =============================================================================

void sched_init(void) {
    memset_t(processes, 0, sizeof(processes));
    
    for (uint8_t i = 0; i < MAX_PROCS; i++) {
        processes[i].pid = make_maya_pid(0);
        processes[i].state = PROC_DEAD;
        processes[i].priority = 0;
    }
    
    scheduler.global_tick = 0;
    scheduler.current_pid = 0;
    scheduler.tzolkin_day = 1;
    scheduler.haab_day = 1;
    scheduler.active_count = 0;
    scheduler.system_load = 0;
    
    processes[0].pid = make_maya_pid(0);
    processes[0].state = PROC_ACTIVE;
    processes[0].priority = 0;
    processes[0].memory_block = 0;
    scheduler.active_count = 1;
}

int8_t sched_create(uint8_t parent, trit_t priority) {
    for (uint8_t i = 1; i < MAX_PROCS; i++) {
        if (processes[i].state == PROC_DEAD) {
            processes[i].pid = make_maya_pid(scheduler.global_tick + i);
            processes[i].state = PROC_SLEEPING;
            processes[i].priority = priority;
            processes[i].memory_block = 0;
            processes[i].cpu_cycles = 0;
            processes[i].quantum = 0;
            processes[i].parent = parent;
            processes[i].n_children = 0;
            
            if (parent < MAX_PROCS && processes[parent].n_children < 3) {
                processes[parent].children[processes[parent].n_children] = i;
                processes[parent].n_children++;
            }
            
            scheduler.active_count++;
            return (int8_t)i;
        }
    }
    
    return -1;
}

int8_t sched_kill(uint8_t pid) {
    if (pid >= MAX_PROCS) return -1;
    if (processes[pid].state == PROC_DEAD) return -1;
    if (pid == 0) return -1;
    
    for (uint8_t i = 0; i < processes[pid].n_children; i++) {
        sched_kill(processes[pid].children[i]);
    }
    
    if (processes[pid].memory_block > 0) {
        mem_free(processes[pid].memory_block);
        processes[pid].memory_block = 0;
    }
    
    processes[pid].state = PROC_DEAD;
    scheduler.active_count--;
    
    return 0;
}

int8_t sched_sleep(uint8_t pid) {
    if (pid >= MAX_PROCS) return -1;
    if (processes[pid].state == PROC_DEAD) return -1;
    processes[pid].state = PROC_SLEEPING;
    return 0;
}

int8_t sched_wake(uint8_t pid) {
    if (pid >= MAX_PROCS) return -1;
    if (processes[pid].state == PROC_DEAD) return -1;
    processes[pid].state = PROC_ACTIVE;
    return 0;
}

static void update_maya_calendar(void) {
    scheduler.global_tick++;
    
    scheduler.tzolkin_day++;
    if (scheduler.tzolkin_day > TZOLKIN) {
        scheduler.tzolkin_day = 1;
    }
    
    scheduler.haab_day++;
    if (scheduler.haab_day > HAAB) {
        scheduler.haab_day = 1;
    }
}

static uint8_t select_next(void) {
    uint8_t start = scheduler.current_pid;
    trit_t best_priority = -2;
    uint8_t best_pid = 0;
    uint8_t found = 0;
    
    for (uint8_t i = 0; i < MAX_PROCS; i++) {
        uint8_t idx = (start + i + 1) % MAX_PROCS;
        
        if (processes[idx].state == PROC_ACTIVE) {
            trit_t cmp = trit_cmp(processes[idx].priority, best_priority);
            
            if (!found || cmp == 1 || (cmp == 0 && processes[idx].quantum < 3)) {
                best_priority = processes[idx].priority;
                best_pid = idx;
                found = 1;
            }
        }
    }
    
    return found ? best_pid : scheduler.current_pid;
}

static void update_system_load(void) {
    uint8_t active = scheduler.active_count;
    
    if (active <= 3) {
        scheduler.system_load = -1;
    } else if (active <= 10) {
        scheduler.system_load = 0;
    } else {
        scheduler.system_load = 1;
    }
}

uint8_t sched_tick(void) {
    update_maya_calendar();
    
    uint8_t next = select_next();
    
    if (next != scheduler.current_pid) {
        processes[scheduler.current_pid].quantum = 0;
        scheduler.current_pid = next;
    }
    
    processes[scheduler.current_pid].quantum++;
    processes[scheduler.current_pid].cpu_cycles++;
    
    if (processes[scheduler.current_pid].quantum >= 3) {
        processes[scheduler.current_pid].quantum = 0;
        return 1;
    }
    
    return 0;
}

uint8_t sched_get_current(void) {
    return scheduler.current_pid;
}

void sched_get_state(uint32_t* tick, uint8_t* tzolkin, uint8_t* haab, trit_t* load) {
    *tick = scheduler.global_tick;
    *tzolkin = scheduler.tzolkin_day;
    *haab = scheduler.haab_day;
    *load = scheduler.system_load;
}

int8_t sched_get_info(uint8_t pid, process_t* info) {
    if (pid >= MAX_PROCS) return -1;
    if (processes[pid].state == PROC_DEAD) return -1;
    
    memcpy_t(info, &processes[pid], sizeof(process_t));
    return 0;
}

int8_t sys_fork(void) {
    return sched_create(scheduler.current_pid,
                       processes[scheduler.current_pid].priority);
}

int8_t sys_exit(void) {
    return sched_kill(scheduler.current_pid);
}

int8_t sys_sleep(void) {
    return sched_sleep(scheduler.current_pid);
}

int8_t sys_wake(uint8_t pid) {
    return sched_wake(pid);
}
