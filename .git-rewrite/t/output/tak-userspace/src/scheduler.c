/**
 * scheduler.c — Maya-cycle scheduler wrapping Linux processes
 *
 * Each TAK "process" maps to a Linux process (fork/exec or thread).
 * Scheduling is done by Linux; we track state with Maya calendar IDs.
 */

#include "ternary.h"
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

// =============================================================================
// STATE
// =============================================================================

static tak_process_t processes[TAK_MAX_PROCS];
static maya_calendar_t calendar;
static uint32_t next_pid_counter = 0;

// =============================================================================
// INIT
// =============================================================================

void sched_init(void) {
    memset(processes, 0, sizeof(processes));
    for (int i = 0; i < TAK_MAX_PROCS; i++) {
        processes[i].state = PROC_DEAD;
        processes[i].linux_pid = -1;
    }

    calendar.global_tick = 0;
    calendar.tzolkin_day = 1;
    calendar.haab_day = 1;
    calendar.system_load = 0;

    /* PID 0 = init process (the shell itself) */
    processes[0].pid = make_maya_pid(0);
    processes[0].state = PROC_ACTIVE;
    processes[0].linux_pid = getpid();
    strncpy(processes[0].name, "init", 31);
    processes[0].name[4] = 0;
    processes[0].priority = 0;
}

// =============================================================================
// CREATE / KILL
// =============================================================================

int sched_create(const char* name, trit_t priority) {
    for (int i = 1; i < TAK_MAX_PROCS; i++) {
        if (processes[i].state == PROC_DEAD) {
            processes[i].pid = make_maya_pid(next_pid_counter++);
            processes[i].state = PROC_SLEEPING;
            processes[i].priority = priority;
            processes[i].linux_pid = -1;
            processes[i].cpu_cycles = 0;
            processes[i].memory_bytes = 0;
            strncpy(processes[i].name, name, 31);
            processes[i].name[31] = 0;
            return i;
        }
    }
    return -1;
}

int sched_kill(int tak_pid) {
    if (tak_pid < 0 || tak_pid >= TAK_MAX_PROCS) return -1;
    if (processes[tak_pid].state == PROC_DEAD) return -1;
    if (tak_pid == 0) return -1; /* can't kill init */

    /* Kill the Linux process if alive */
    if (processes[tak_pid].linux_pid > 0) {
        kill(processes[tak_pid].linux_pid, SIGTERM);
        usleep(50000);
        if (kill(processes[tak_pid].linux_pid, 0) == 0) {
            kill(processes[tak_pid].linux_pid, SIGKILL);
        }
        waitpid(processes[tak_pid].linux_pid, NULL, 0);
    }

    processes[tak_pid].state = PROC_DEAD;
    processes[tak_pid].linux_pid = -1;
    return 0;
}

// =============================================================================
// LAUNCH — fork+exec a Linux command, register as TAK process
// =============================================================================

int sched_exec(const char* name, const char* path, char* const argv[]) {
    int slot = sched_create(name, 0);
    if (slot < 0) return -1;

    pid_t pid = fork();
    if (pid == 0) {
        /* Child: become the new process group leader */
        setpgid(0, 0);
        execvp(path, argv);
        fprintf(stderr, "tak: exec '%s' failed: %s\n", path, strerror(errno));
        _exit(127);
    } else if (pid > 0) {
        processes[slot].linux_pid = pid;
        processes[slot].state = PROC_ACTIVE;
        return slot;
    } else {
        processes[slot].state = PROC_DEAD;
        return -1;
    }
}

// =============================================================================
// WAIT — reap children, update TAK process table
// =============================================================================

void sched_reap(void) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 1; i < TAK_MAX_PROCS; i++) {
            if (processes[i].linux_pid == pid) {
                processes[i].state = PROC_DEAD;
                processes[i].linux_pid = -1;
                break;
            }
        }
    }
}

// =============================================================================
// TICK — advance Maya calendar
// =============================================================================

void sched_tick(void) {
    calendar.global_tick++;

    calendar.tzolkin_day++;
    if (calendar.tzolkin_day > TZOLKIN_DAYS) calendar.tzolkin_day = 1;

    calendar.haab_day++;
    if (calendar.haab_day > HAAB_DAYS) calendar.haab_day = 1;

    /* Update system load (ternary) */
    int active = 0;
    for (int i = 1; i < TAK_MAX_PROCS; i++) {
        if (processes[i].state == PROC_ACTIVE) active++;
    }
    if (active <= 3) calendar.system_load = -1;
    else if (active <= 10) calendar.system_load = 0;
    else calendar.system_load = 1;

    /* Reap zombies */
    sched_reap();
}

// =============================================================================
// QUERY
// =============================================================================

int sched_get_current(void) {
    return 0; /* always init */
}

maya_calendar_t* sched_get_calendar(void) {
    return &calendar;
}

tak_process_t* sched_get_process(int i) {
    if (i < 0 || i >= TAK_MAX_PROCS) return NULL;
    return &processes[i];
}

int sched_active_count(void) {
    int c = 0;
    for (int i = 1; i < TAK_MAX_PROCS; i++) {
        if (processes[i].state == PROC_ACTIVE) c++;
    }
    return c;
}
