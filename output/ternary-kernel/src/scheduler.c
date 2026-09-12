/**
 * scheduler.c — Planificador basado en Ciclos Mayas
 * 
 * Procesos planificados en ciclos de:
 * - Tzolkin: 260 días (ciclo sagrado)
 * - Haab: 365 días (año solar)
 * 
 * Cada "día" = quantum de CPU
 * Los procesos se planifican según su "fecha" en el calendario Maya
 */

#include "../include/ternary.h"

// =============================================================================
// ESTRUCTURAS
// =============================================================================

// Estados del proceso (ternario)
typedef enum {
    PROC_DEAD = -1,     // Muerto
    PROC_SLEEPING = 0,  // Dormido
    PROC_ACTIVE = 1     // Activo
} proc_state_t;

// Proceso
typedef struct {
    maya_pid_t pid;         // ID maya
    proc_state_t state;     // Estado ternario
    trit_t priority;        // Prioridad: -1=baja, 0=media, +1=alta
    uint8_t memory_block;   // Bloque de memoria asignado
    uint16_t cpu_cycles;    // Ciclos de CPU consumidos
    uint8_t quantum;        // Quantum actual (0-2)
    uint8_t parent;         // PID del padre
    uint8_t children[4];    // PIDs de hijos (máximo 4)
    uint8_t n_children;     // Número de hijos
} __attribute__((packed)) process_t;

// Estado del scheduler
typedef struct {
    uint32_t global_tick;           // Tick global del sistema
    uint8_t current_pid;            // Índice del proceso actual
    uint8_t tzolkin_day;            // Día actual en Tzolkin (1-260)
    uint8_t haab_day;               // Día actual en Haab (1-365)
    uint8_t active_count;           // Procesos activos
    trit_t system_load;             // Carga del sistema: -1=baja, 0=media, +1=alta
} __attribute__((packed)) scheduler_state_t;

// =============================================================================
// VARIABLES GLOBALES
// =============================================================================

static process_t processes[MAX_PROCS];
static scheduler_state_t scheduler;

// =============================================================================
// FUNCIONES
// =============================================================================

// Inicializar scheduler
void sched_init() {
    // Limpiar procesos
    for (uint8_t i = 0; i < MAX_PROCS; i++) {
        processes[i].pid = make_maya_pid(0);
        processes[i].state = PROC_DEAD;
        processes[i].priority = 0;
        processes[i].memory_block = 0;
        processes[i].cpu_cycles = 0;
        processes[i].quantum = 0;
        processes[i].parent = 0;
        processes[i].n_children = 0;
        
        for (uint8_t j = 0; j < 4; j++) {
            processes[i].children[j] = 0;
        }
    }
    
    // Inicializar scheduler
    scheduler.global_tick = 0;
    scheduler.current_pid = 0;
    scheduler.tzolkin_day = 1;
    scheduler.haab_day = 1;
    scheduler.active_count = 0;
    scheduler.system_load = 0;
    
    // Crear proceso init (PID 0)
    processes[0].pid = make_maya_pid(0);
    processes[0].state = PROC_ACTIVE;
    processes[0].priority = 0;  // prioridad media
    processes[0].memory_block = 0;
    scheduler.active_count = 1;
}

// Crear proceso
int8_t sched_create(uint8_t parent, trit_t priority) {
    // Buscar slot libre
    for (uint8_t i = 1; i < MAX_PROCS; i++) {
        if (processes[i].state == PROC_DEAD) {
            // Crear nuevo proceso con ID maya
            processes[i].pid = make_maya_pid(scheduler.global_tick + i);
            processes[i].state = PROC_SLEEPING;
            processes[i].priority = priority;
            processes[i].memory_block = 0;  // Se asignará después
            processes[i].cpu_cycles = 0;
            processes[i].quantum = 0;
            processes[i].parent = parent;
            processes[i].n_children = 0;
            
            // Agregar a hijos del padre
            if (parent < MAX_PROCS && processes[parent].n_children < 4) {
                processes[parent].children[processes[parent].n_children] = i;
                processes[parent].n_children++;
            }
            
            scheduler.active_count++;
            return i;
        }
    }
    
    return -1;  // Sin espacio
}

// Matar proceso
int8_t sched_kill(uint8_t pid) {
    if (pid >= MAX_PROCS) return -1;
    if (processes[pid].state == PROC_DEAD) return -1;
    if (pid == 0) return -1;  // No se puede matar init
    
    // Liberar hijos recursivamente
    for (uint8_t i = 0; i < processes[pid].n_children; i++) {
        sched_kill(processes[pid].children[i]);
    }
    
    // Marcar como muerto
    processes[pid].state = PROC_DEAD;
    processes[pid].memory_block = 0;
    scheduler.active_count--;
    
    return 0;
}

// Dormir proceso
int8_t sched_sleep(uint8_t pid) {
    if (pid >= MAX_PROCS) return -1;
    if (processes[pid].state == PROC_DEAD) return -1;
    
    processes[pid].state = PROC_SLEEPING;
    return 0;
}

// Despertar proceso
int8_t sched_wake(uint8_t pid) {
    if (pid >= MAX_PROCS) return -1;
    if (processes[pid].state == PROC_DEAD) return -1;
    
    processes[pid].state = PROC_ACTIVE;
    return 0;
}

// Actualizar calendario Maya
static void update_maya_calendar() {
    scheduler.global_tick++;
    
    // Avanzar Tzolkin (1-260)
    scheduler.tzolkin_day++;
    if (scheduler.tzolkin_day > TZOLKIN) {
        scheduler.tzolkin_day = 1;
    }
    
    // Avanzar Haab (1-365)
    scheduler.haab_day++;
    if (scheduler.haab_day > HAAB) {
        scheduler.haab_day = 1;
    }
}

// Seleccionar siguiente proceso (round-robin ternario)
static uint8_t select_next() {
    uint8_t start = scheduler.current_pid;
    trit_t best_priority = -2;  // Menor que cualquier prioridad válida
    uint8_t best_pid = 0;
    
    // Buscar proceso con mayor prioridad
    for (uint8_t i = 0; i < MAX_PROCS; i++) {
        uint8_t idx = (start + i + 1) % MAX_PROCS;
        
        if (processes[idx].state == PROC_ACTIVE) {
            // Comparar prioridad (ternario)
            trit_t cmp = trit_cmp(processes[idx].priority, best_priority);
            
            if (cmp == 1 || (cmp == 0 && processes[idx].quantum < 3)) {
                best_priority = processes[idx].priority;
                best_pid = idx;
            }
        }
    }
    
    return best_pid;
}

// Actualizar carga del sistema
static void update_system_load() {
    uint8_t active = scheduler.active_count;
    
    if (active <= 3) {
        scheduler.system_load = -1;  // baja
    } else if (active <= 10) {
        scheduler.system_load = 0;   // media
    } else {
        scheduler.system_load = 1;   // alta
    }
}

// Tick del scheduler
uint8_t sched_tick() {
    // Actualizar calendario
    update_maya_calendar();
    
    // Seleccionar siguiente proceso
    uint8_t next = select_next();
    
    if (next != scheduler.current_pid) {
        // Cambio de contexto
        processes[scheduler.current_pid].quantum = 0;
        scheduler.current_pid = next;
    }
    
    // Ejecutar quantum actual
    processes[scheduler.current_pid].quantum++;
    processes[scheduler.current_pid].cpu_cycles++;
    
    // Si quantum completado (3 ciclos), cambiar proceso
    if (processes[scheduler.current_pid].quantum >= 3) {
        processes[scheduler.current_pid].quantum = 0;
        return 1;  // Context switch
    }
    
    return 0;  // Misma tarea
}

// Obtener proceso actual
uint8_t sched_get_current() {
    return scheduler.current_pid;
}

// Obtener estado del scheduler
void sched_get_state(uint32_t* tick, uint8_t* tzolkin, uint8_t* haab, trit_t* load) {
    *tick = scheduler.global_tick;
    *tzolkin = scheduler.tzolkin_day;
    *haab = scheduler.haab_day;
    *load = scheduler.system_load;
}

// Obtener info de un proceso
int8_t sched_get_info(uint8_t pid, process_t* info) {
    if (pid >= MAX_PROCS) return -1;
    
    *info = processes[pid];
    return 0;
}

// Syscall: fork
int8_t sys_fork() {
    return sched_create(scheduler.current_pid, 
                       processes[scheduler.current_pid].priority);
}

// Syscall: exit
int8_t sys_exit() {
    return sched_kill(scheduler.current_pid);
}

// Syscall: sleep
int8_t sys_sleep() {
    return sched_sleep(scheduler.current_pid);
}

// Syscall: wake
int8_t sys_wake(uint8_t pid) {
    return sched_wake(pid);
}
