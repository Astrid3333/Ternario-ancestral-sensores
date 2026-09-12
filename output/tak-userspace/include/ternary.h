/**
 * ternary.h — Core ternary definitions for userspace kernel
 * 
 * This kernel runs ON TOP of Linux.
 * Linux provides: processes, memory, filesystem, I/O, network
 * We provide: ternary logic, base-60 addressing, Maya scheduling, Quipu FS
 */

#ifndef TERNARY_H
#define TERNARY_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <termios.h>
#include <pwd.h>
#include <sys/types.h>

// =============================================================================
// CONSTANTS
// =============================================================================

#define TAK_VERSION     "2.0"
#define TAK_NAME        "Ternary Ancestral Kernel"
#define TAK_HOME        ".tak"
#define TAK_MAX_PROCS   33
#define TAK_MAX_FILES   33
#define TAK_MAX_MEM     60
#define TAK_BLOCK_SIZE  60
#define TAK_CMD_MAX     256
#define TAK_HISTORY     50

#define TZOLKIN_DAYS    260
#define HAAB_DAYS       365
#define BASE_60         60

// Colors
#define COLOR_RESET     "\033[0m"
#define COLOR_BOLD      "\033[1m"
#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_MAGENTA   "\033[35m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_WHITE     "\033[37m"
#define COLOR_GRAY      "\033[90m"
#define COLOR_BG_RED    "\033[41m"
#define COLOR_BG_GREEN  "\033[42m"
#define COLOR_BG_YELLOW "\033[43m"

// =============================================================================
// TYPES
// =============================================================================

typedef int8_t trit_t;

typedef struct {
    uint16_t tzolkin;
    uint8_t haab;
} maya_pid_t;

typedef struct {
    uint8_t high;
    uint8_t low;
} babilonian_addr_t;

typedef enum {
    PROC_DEAD = -1,
    PROC_SLEEPING = 0,
    PROC_ACTIVE = 1
} proc_state_t;

typedef struct {
    maya_pid_t pid;
    proc_state_t state;
    trit_t priority;
    pid_t linux_pid;
    char name[32];
    uint64_t cpu_cycles;
    uint64_t memory_bytes;
} tak_process_t;

typedef struct {
    char name[32];
    uint8_t type;       // 0=file, 1=dir, 2=exec
    uint32_t size;
    char path[256];
} tak_file_t;

typedef struct {
    void* ptr;
    uint32_t size;
    uint8_t owner;
    uint8_t color;
    uint8_t flags;
    char label[16];
} mem_block_t;

typedef struct {
    maya_pid_t pid;
    uint32_t tzolkin_day;
    uint32_t haab_day;
    uint64_t global_tick;
    trit_t system_load;
} maya_calendar_t;

// =============================================================================
// TERNARY OPERATIONS
// =============================================================================

static inline trit_t trit_add(trit_t a, trit_t b) {
    int r = a + b;
    if (r > 1) return 1;
    if (r < -1) return -1;
    return (trit_t)r;
}

static inline trit_t trit_mul(trit_t a, trit_t b) {
    if (a == 0 || b == 0) return 0;
    return (a == b) ? 1 : -1;
}

static inline trit_t trit_neg(trit_t a) { return -a; }

static inline trit_t trit_cmp(trit_t a, trit_t b) {
    if (a > b) return 1;
    if (a < b) return -1;
    return 0;
}

// =============================================================================
// BABILONIAN ADDRESSING
// =============================================================================

static inline babilonian_addr_t linear_to_b60(uint32_t addr) {
    babilonian_addr_t r;
    r.high = addr / BASE_60;
    r.low = addr % BASE_60;
    return r;
}

static inline uint32_t b60_to_linear(babilonian_addr_t a) {
    return a.high * BASE_60 + a.low;
}

// =============================================================================
// MAYA CALENDAR
// =============================================================================

static inline maya_pid_t make_maya_pid(uint32_t counter) {
    maya_pid_t p;
    p.tzolkin = (counter % TZOLKIN_DAYS) + 1;
    p.haab = (counter % HAAB_DAYS) + 1;
    return p;
}

#endif // TERNARY_H
