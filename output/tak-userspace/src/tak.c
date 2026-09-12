/**
 * tak.c — Ternary Ancestral Kernel (Userspace, runs on Linux)
 *
 * Ultra-lite kernel that uses Linux as substrate.
 * Provides: ternary logic, Maya scheduling, Quipu FS, Base-60 memory
 * Requires: any Linux, gcc, standard libc
 *
 * Build: gcc -O2 -o tak tak.c -lm
 * Run:   ./tak
 */

#include "ternary.h"

/* Subsystem APIs */
extern void sched_init(void);
extern void sched_tick(void);
extern int  sched_create(const char* name, trit_t priority);
extern int  sched_kill(int tak_pid);
extern int  sched_exec(const char* name, const char* path, char* const argv[]);
extern void sched_reap(void);
extern maya_calendar_t* sched_get_calendar(void);
extern tak_process_t* sched_get_process(int i);
extern int  sched_active_count(void);

extern void fs_init(void);
extern void fs_set_home(const char* home);
extern int  fs_create(const char* name, const char* data, uint32_t size);
extern int  fs_read(const char* name, char* buf, uint32_t bufsize);
extern int  fs_delete(const char* name);
extern int  fs_exists(const char* name);
extern int  fs_list(tak_file_t* files, int max);
extern uint32_t fs_size(const char* name);
extern const char* fs_get_root(void);

extern void mem_init(void);
extern int  mem_alloc(int owner, uint8_t color, uint32_t size, const char* label);
extern int  mem_free(int block);
extern int  mem_get_active_count(void);
extern uint32_t mem_get_total_allocated(void);
extern mem_block_t* mem_get_block(int i);
extern const char* mem_color_name(uint8_t color);
extern const char* mem_color_ansi(uint8_t color);

// =============================================================================
// TERMINAL
// =============================================================================

static struct termios orig_termios;
static int raw_mode = 0;

void term_init(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
}

void term_raw_on(void) {
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    raw_mode = 1;
}

void term_raw_off(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    raw_mode = 0;
}

void term_get_size(int* w, int* h) {
    /* fallback: 80x24 */
    *w = 80;
    *h = 24;
}

// =============================================================================
// PERSISTENCE — save/restore state to ~/.tak/state
// =============================================================================

static char state_path[512];

void state_save(void) {
    FILE* f = fopen(state_path, "w");
    if (!f) return;

    maya_calendar_t* cal = sched_get_calendar();
    fprintf(f, "tick=%lu\n", (unsigned long)cal->global_tick);
    fprintf(f, "tzolkin=%u\n", cal->tzolkin_day);
    fprintf(f, "haab=%u\n", cal->haab_day);

    for (int i = 1; i < TAK_MAX_PROCS; i++) {
        tak_process_t* p = sched_get_process(i);
        if (p->state != PROC_DEAD) {
            fprintf(f, "proc=%d,%s,%d,%d\n", i, p->name, p->priority, p->linux_pid);
        }
    }
    fclose(f);
}

void state_load(void) {
    FILE* f = fopen(state_path, "r");
    if (!f) return;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        if (strncmp(line, "tick=", 5) == 0) {
            /* restore tick */
        }
    }
    fclose(f);
}

// =============================================================================
// PREAMBLE — show banner
// =============================================================================

void show_banner(void) {
    printf("\n");
    printf(COLOR_CYAN COLOR_BOLD);
    printf("  ╔══════════════════════════════════════════════╗\n");
    printf("  ║       TERNARY ANCESTRAL KERNEL v" TAK_VERSION "         ║\n");
    printf("  ║       Ultra-lite · Runs on Linux             ║\n");
    printf("  ╚══════════════════════════════════════════════╝\n");
    printf(COLOR_RESET "\n");

    printf("  " COLOR_GREEN "▸ Substrates:" COLOR_RESET " Linux kernel, glibc, gcc\n");
    printf("  " COLOR_GREEN "▸ Logic:" COLOR_RESET "     Ternary {-1, 0, +1}\n");
    printf("  " COLOR_GREEN "▸ Memory:" COLOR_RESET "    Base 60 (Babylonian), %d blocks\n", TAK_MAX_MEM);
    printf("  " COLOR_GREEN "▸ Scheduler:" COLOR_RESET " Maya Tzolkin (260) + Haab (365)\n");
    printf("  " COLOR_GREEN "▸ Filesystem:" COLOR_RESET " Quipu (real directory)\n");
    printf("  " COLOR_GREEN "▸ Processes:" COLOR_RESET "  %d max (Linux fork/exec)\n", TAK_MAX_PROCS);
    printf("  " COLOR_GREEN "▸ Memory model:" COLOR_RESET " malloc + ternary tracking\n");
    printf("\n");
    printf("  " COLOR_YELLOW "Type 'help' for commands." COLOR_RESET "\n\n");
}

// =============================================================================
// SHELL
// =============================================================================

static char cmd_history[TAK_HISTORY][TAK_CMD_MAX];
static int history_idx = 0;
static int history_pos = 0;

void history_add(const char* cmd) {
    if (strlen(cmd) == 0) return;
    strncpy(cmd_history[history_idx % TAK_HISTORY], cmd, TAK_CMD_MAX - 1);
    history_idx++;
    history_pos = history_idx;
}

const char* history_prev(void) {
    if (history_pos > 0) history_pos--;
    return cmd_history[history_pos % TAK_HISTORY];
}

const char* history_next(void) {
    if (history_pos < history_idx) history_pos++;
    return cmd_history[history_pos % TAK_HISTORY];
}

// --- Command implementations ---

void cmd_help(void) {
    printf("\n");
    printf("  " COLOR_BOLD "Ternary Commands:" COLOR_RESET "\n");
    printf("  ─────────────────────────────────────────\n");
    printf("  " COLOR_CYAN "help" COLOR_RESET "            Show this help\n");
    printf("  " COLOR_CYAN "ps" COLOR_RESET "              List processes (TAK + Linux)\n");
    printf("  " COLOR_CYAN "fork <cmd...>" COLOR_RESET "   Fork+exec a Linux command\n");
    printf("  " COLOR_CYAN "kill <pid>" COLOR_RESET "     Kill a TAK process\n");
    printf("  " COLOR_CYAN "mem" COLOR_RESET "            Memory status (Base 60)\n");
    printf("  " COLOR_CYAN "malloc <bytes>" COLOR_RESET " Allocate ternary block\n");
    printf("  " COLOR_CYAN "free <block>" COLOR_RESET "   Free a ternary block\n");
    printf("  " COLOR_CYAN "fs" COLOR_RESET "             List Quipu files\n");
    printf("  " COLOR_CYAN "touch <name> <data>" COLOR_RESET " Create Quipu file\n");
    printf("  " COLOR_CYAN "cat <name>" COLOR_RESET "     Read Quipu file\n");
    printf("  " COLOR_CYAN "rm <name>" COLOR_RESET "      Delete Quipu file\n");
    printf("  " COLOR_CYAN "cal" COLOR_RESET "           Maya calendar\n");
    printf("  " COLOR_CYAN "trit <n>" COLOR_RESET "      Number in ternary\n");
    printf("  " COLOR_CYAN "b60 <n>" COLOR_RESET "       Number in Base 60\n");
    printf("  " COLOR_CYAN "whoami" COLOR_RESET "        Current user\n");
    printf("  " COLOR_CYAN "uname" COLOR_RESET "         System info\n");
    printf("  " COLOR_CYAN "uptime" COLOR_RESET "        System uptime\n");
    printf("  " COLOR_CYAN "clear" COLOR_RESET "         Clear screen\n");
    printf("  " COLOR_CYAN "neofetch" COLOR_RESET "      Ternary system info\n");
    printf("  " COLOR_CYAN "halt" COLOR_RESET "          Exit TAK\n");
    printf("\n");
}

void cmd_ps(void) {
    printf("\n");
    printf("  " COLOR_BOLD "TAK Processes:" COLOR_RESET "\n");
    printf("  ──────────────────────────────────────────────\n");
    printf("  " COLOR_GRAY "PID   STATE     PRI  LINUX_PID  NAME" COLOR_RESET "\n");

    for (int i = 0; i < TAK_MAX_PROCS; i++) {
        tak_process_t* p = sched_get_process(i);
        if (p->state == PROC_DEAD) continue;

        printf("  %-6d", i);

        if (p->state == PROC_ACTIVE) {
            printf(COLOR_GREEN "ACTIVE  " COLOR_RESET);
        } else {
            printf(COLOR_YELLOW "SLEEP   " COLOR_RESET);
        }

        printf("  ");
        if (p->priority == -1) printf(COLOR_RED "-" COLOR_RESET);
        else if (p->priority == 0) printf("0");
        else printf(COLOR_GREEN "+" COLOR_RESET);

        printf("   %-10d", p->linux_pid);
        printf(COLOR_CYAN "%s" COLOR_RESET, p->name);
        printf("\n");
    }

    printf("\n  " COLOR_GRAY "Active: %d/%d" COLOR_RESET "\n",
           sched_active_count(), TAK_MAX_PROCS);
}

void cmd_fork(int argc, char** argv) {
    if (argc < 2) {
        printf("  Usage: fork <command> [args...]\n");
        return;
    }

    char name[32];
    snprintf(name, 32, "%.28s", argv[1]);

    int slot = sched_exec(name, argv[1], &argv[1]);
    if (slot >= 0) {
        tak_process_t* p = sched_get_process(slot);
        printf("  " COLOR_GREEN "✓" COLOR_RESET " Created TAK PID %d (Linux PID %d): ",
               slot, p->linux_pid);
        for (int i = 1; i < argc; i++) printf("%s ", argv[i]);
        printf("\n");
    } else {
        printf("  " COLOR_RED "✗" COLOR_RESET " Failed to fork: %s\n", strerror(errno));
    }
}

void cmd_kill_tak(int argc, char** argv) {
    if (argc < 2) {
        printf("  Usage: kill <tak_pid>\n");
        return;
    }
    int pid = atoi(argv[1]);
    if (pid <= 0 || pid >= TAK_MAX_PROCS) {
        printf("  Invalid TAK PID (1-%d)\n", TAK_MAX_PROCS - 1);
        return;
    }
    if (sched_kill(pid) == 0) {
        printf("  " COLOR_GREEN "✓" COLOR_RESET " Killed TAK PID %d\n", pid);
    } else {
        printf("  " COLOR_RED "✗" COLOR_RESET " Could not kill PID %d\n", pid);
    }
}

void cmd_mem(void) {
    int active = mem_get_active_count();
    uint32_t alloc = mem_get_total_allocated();

    printf("\n");
    printf("  " COLOR_BOLD "Memory — Base 60 (Babylonian)" COLOR_RESET "\n");
    printf("  ──────────────────────────────────────────────\n");
    printf("  Active blocks:  %d / %d\n", active, TAK_MAX_MEM);
    printf("  Total alloc'd:  %u bytes\n", alloc);
    printf("  Block size:     %d bytes\n\n", TAK_BLOCK_SIZE);

    printf("  Block map:\n  ");
    for (int i = 0; i < TAK_MAX_MEM; i++) {
        mem_block_t* b = mem_get_block(i);
        if (b->flags) {
            printf("%s█" COLOR_RESET, mem_color_ansi(b->color));
        } else {
            printf(COLOR_GRAY "·" COLOR_RESET);
        }
        if ((i + 1) % 30 == 0) printf("\n  ");
    }
    printf("\n\n");

    printf("  " COLOR_GRAY "Colors:" COLOR_RESET " ");
    printf(COLOR_RED "█" COLOR_RESET " kernel  ");
    printf(COLOR_GREEN "█" COLOR_RESET " process  ");
    printf(COLOR_BLUE "█" COLOR_RESET " data  ");
    printf(COLOR_YELLOW "█" COLOR_RESET " code  ");
    printf(COLOR_MAGENTA "█" COLOR_RESET " stack\n\n");
}

void cmd_malloc(int argc, char** argv) {
    if (argc < 2) {
        printf("  Usage: malloc <bytes> [label]\n");
        return;
    }
    uint32_t size = (uint32_t)atoi(argv[1]);
    const char* label = argc > 2 ? argv[2] : "user";

    int slot = mem_alloc(1, 3, size, label);
    if (slot >= 0) {
        babilonian_addr_t addr = linear_to_b60(slot * TAK_BLOCK_SIZE);
        printf("  " COLOR_GREEN "✓" COLOR_RESET " Allocated %u bytes → block %d (addr %d:%d)\n",
               size, slot, addr.high, addr.low);
    } else {
        printf("  " COLOR_RED "✗" COLOR_RESET " No free blocks\n");
    }
}

void cmd_free_block(int argc, char** argv) {
    if (argc < 2) {
        printf("  Usage: free <block>\n");
        return;
    }
    int block = atoi(argv[1]);
    if (mem_free(block) == 0) {
        printf("  " COLOR_GREEN "✓" COLOR_RESET " Freed block %d\n", block);
    } else {
        printf("  " COLOR_RED "✗" COLOR_RESET " Could not free block %d\n", block);
    }
}

void cmd_fs(void) {
    tak_file_t files[TAK_MAX_FILES];
    int n = fs_list(files, TAK_MAX_FILES);

    printf("\n");
    printf("  " COLOR_BOLD "Quipu Filesystem — %s" COLOR_RESET "\n", fs_get_root());
    printf("  ──────────────────────────────────────────────\n");

    if (n == 0) {
        printf("  " COLOR_GRAY "(empty)" COLOR_RESET "\n");
    } else {
        printf("  " COLOR_GRAY "NAME              SIZE" COLOR_RESET "\n");
        for (int i = 0; i < n; i++) {
            printf("  " COLOR_CYAN "%-18s" COLOR_RESET, files[i].name);
            if (files[i].type == 1) {
                printf(COLOR_BLUE "  <DIR>" COLOR_RESET);
            } else {
                printf("%u B", files[i].size);
            }
            printf("\n");
        }
    }
    printf("\n");
}

void cmd_touch(int argc, char** argv) {
    if (argc < 3) {
        printf("  Usage: touch <name> <data>\n");
        return;
    }
    if (fs_create(argv[1], argv[2], strlen(argv[2])) == 0) {
        printf("  " COLOR_GREEN "✓" COLOR_RESET " Created '%s' (%lu bytes)\n",
               argv[1], (unsigned long)strlen(argv[2]));
    } else {
        printf("  " COLOR_RED "✗" COLOR_RESET " Failed to create '%s'\n", argv[1]);
    }
}

void cmd_cat(int argc, char** argv) {
    if (argc < 2) {
        printf("  Usage: cat <name>\n");
        return;
    }
    char buf[4096];
    int n = fs_read(argv[1], buf, sizeof(buf));
    if (n >= 0) {
        printf("  " COLOR_GREEN "%s" COLOR_RESET "\n", buf);
    } else {
        printf("  " COLOR_RED "✗" COLOR_RESET " File not found: '%s'\n", argv[1]);
    }
}

void cmd_rm(int argc, char** argv) {
    if (argc < 2) {
        printf("  Usage: rm <name>\n");
        return;
    }
    if (fs_delete(argv[1]) == 0) {
        printf("  " COLOR_GREEN "✓" COLOR_RESET " Deleted '%s'\n", argv[1]);
    } else {
        printf("  " COLOR_RED "✗" COLOR_RESET " Could not delete '%s'\n", argv[1]);
    }
}

void cmd_cal(void) {
    maya_calendar_t* cal = sched_get_calendar();

    printf("\n");
    printf("  " COLOR_BOLD "Maya Calendar" COLOR_RESET "\n");
    printf("  ──────────────────────────────────────────────\n");
    printf("  Global tick:   %lu\n", (unsigned long)cal->global_tick);
    printf("  Tzolkin day:   " COLOR_CYAN "%u" COLOR_RESET " / 260\n", cal->tzolkin_day);
    printf("  Haab day:      " COLOR_CYAN "%u" COLOR_RESET " / 365\n", cal->haab_day);

    printf("  System load:   ");
    if (cal->system_load == -1) printf(COLOR_GREEN "LOW" COLOR_RESET);
    else if (cal->system_load == 0) printf(COLOR_YELLOW "MEDIUM" COLOR_RESET);
    else printf(COLOR_RED "HIGH" COLOR_RESET);
    printf("\n\n");
}

void cmd_trit(int argc, char** argv) {
    if (argc < 2) {
        printf("  Usage: trit <number>\n");
        return;
    }
    int n = atoi(argv[1]);
    printf("  %d → ", n);

    if (n == 0) {
        printf(COLOR_GREEN "0" COLOR_RESET);
    } else {
        if (n < 0) {
            printf(COLOR_RED "-" COLOR_RESET);
            n = -n;
        }
        char trits[32];
        int len = 0;
        while (n > 0) {
            int d = n % 3;
            if (d == 0) trits[len++] = '0';
            else if (d == 1) trits[len++] = '+';
            else trits[len++] = '-';
            n /= 3;
        }
        for (int i = len - 1; i >= 0; i--) {
            if (trits[i] == '+') printf(COLOR_GREEN "+" COLOR_RESET);
            else if (trits[i] == '-') printf(COLOR_RED "-" COLOR_RESET);
            else printf("0");
        }
    }
    printf("\n");
}

void cmd_b60(int argc, char** argv) {
    if (argc < 2) {
        printf("  Usage: b60 <number>\n");
        return;
    }
    uint32_t n = (uint32_t)atoi(argv[1]);
    babilonian_addr_t addr = linear_to_b60(n);

    printf("  %u → " COLOR_CYAN "%d:%d" COLOR_RESET " (Base 60)\n",
           n, addr.high, addr.low);
}

void cmd_whoami(void) {
    struct passwd* pw = getpwuid(getuid());
    printf("  %s\n", pw ? pw->pw_name : "unknown");
}

void cmd_uname(void) {
    printf("  Ternary Ancestral Kernel v" TAK_VERSION "\n");
    printf("  Runs on Linux (userspace)\n");

    /* Get Linux kernel version */
    char buf[256];
    FILE* f = fopen("/proc/version", "r");
    if (f) {
        if (fgets(buf, sizeof(buf), f) == NULL) buf[0] = 0;
        buf[strcspn(buf, "\n")] = 0;
        printf("  Substrate: %s\n", buf);
        fclose(f);
    }
}

void cmd_uptime(void) {
    FILE* f = fopen("/proc/uptime", "r");
    if (f) {
        double up = 0;
        if (fscanf(f, "%lf", &up) != 1) up = 0;
        fclose(f);
        int h = (int)(up / 3600);
        int m = ((int)up % 3600) / 60;
        printf("  Uptime: %dh %dm\n", h, m);
    }
    printf("  TAK ticks: %lu\n", (unsigned long)sched_get_calendar()->global_tick);
}

void cmd_neofetch(void) {
    printf("\n");
    printf(COLOR_CYAN COLOR_BOLD);
    printf("        ╔═══╗         " COLOR_RESET COLOR_BOLD "ternary@mayan\n" COLOR_RESET);
    printf(COLOR_CYAN COLOR_BOLD);
    printf("       ╔╝   ╚╗        " COLOR_RESET "─────────────────\n");
    printf(COLOR_CYAN COLOR_BOLD);
    printf("      ╔╝  " COLOR_RESET COLOR_YELLOW "▲" COLOR_CYAN COLOR_BOLD "  ╚╗       " COLOR_RESET COLOR_BOLD "OS:" COLOR_RESET "       TAK v" TAK_VERSION "\n");
    printf(COLOR_CYAN COLOR_BOLD);
    printf("     ╔╝  " COLOR_RESET COLOR_YELLOW "▲ ▲" COLOR_CYAN COLOR_BOLD "  ╚╗      " COLOR_RESET COLOR_BOLD "Kernel:" COLOR_RESET "    Ternary Ancestral\n");
    printf(COLOR_CYAN COLOR_BOLD);
    printf("    ╔╝  " COLOR_RESET COLOR_YELLOW "▲ ▲ ▲" COLOR_CYAN COLOR_BOLD "  ╚╗     " COLOR_RESET COLOR_BOLD "Substrate:" COLOR_RESET "   Linux (userspace)\n");
    printf(COLOR_CYAN COLOR_BOLD);
    printf("    ╚╗  " COLOR_RESET COLOR_GREEN "- 0 +" COLOR_CYAN COLOR_BOLD "  ╔╝     " COLOR_RESET COLOR_BOLD "Shell:" COLOR_RESET "       tak-sh\n");
    printf(COLOR_CYAN COLOR_BOLD);
    printf("     ╚╗       ╔╝      " COLOR_RESET COLOR_BOLD "Logic:" COLOR_RESET "        Ternary {-1,0,+1}\n");
    printf(COLOR_CYAN COLOR_BOLD);
    printf("      ╚╗     ╔╝       " COLOR_RESET COLOR_BOLD "Memory:" COLOR_RESET "       Base 60 (60 blocks)\n");
    printf(COLOR_CYAN COLOR_BOLD);
    printf("       ╚╗   ╔╝        " COLOR_RESET COLOR_BOLD "Scheduler:" COLOR_RESET "    Maya (260/365)\n");
    printf(COLOR_CYAN COLOR_BOLD);
    printf("        ╚═══╝         " COLOR_RESET COLOR_BOLD "FS:" COLOR_RESET "           Quipu\n");

    maya_calendar_t* cal = sched_get_calendar();
    printf("\n");
    printf("  " COLOR_BOLD "Calendar:" COLOR_RESET " Tzolkin %u/260, Haab %u/365\n",
           cal->tzolkin_day, cal->haab_day);
    printf("  " COLOR_BOLD "Processes:" COLOR_RESET " %d active\n", sched_active_count());
    printf("  " COLOR_BOLD "Memory:" COLOR_RESET "    %d blocks used\n", mem_get_active_count());
    printf("  " COLOR_BOLD "Files:" COLOR_RESET "     Quipu @ %s\n", fs_get_root());
    printf("\n");
}

// =============================================================================
// PROMPT
// =============================================================================

void show_prompt(void) {
    maya_calendar_t* cal = sched_get_calendar();

    printf(COLOR_GREEN COLOR_BOLD "tak" COLOR_RESET);
    printf(COLOR_GRAY "@" COLOR_RESET);
    printf(COLOR_CYAN "mayan" COLOR_RESET);
    printf(COLOR_GRAY ":" COLOR_RESET);
    printf(COLOR_BLUE "%u" COLOR_RESET, cal->tzolkin_day);
    printf(COLOR_GRAY "/" COLOR_RESET);
    printf(COLOR_YELLOW "%u" COLOR_RESET, cal->haab_day);
    printf(COLOR_BOLD "$ " COLOR_RESET);
    fflush(stdout);
}

// =============================================================================
// LINE READER (with raw mode support)
// =============================================================================

int read_line(char* buf, int bufsize) {
    int pos = 0;
    buf[0] = 0;

    while (1) {
        int ch = getchar();
        if (ch == EOF) break;

        if (ch == '\n') {
            printf("\n");
            break;
        } else if (ch == 127 || ch == '\b') {
            if (pos > 0) {
                pos--;
                printf("\b \b");
                fflush(stdout);
            }
        } else if (ch == 27) {
            /* Escape sequence */
            int ch2 = getchar();
            int ch3 = getchar();
            (void)ch2;
            (void)ch3;
        } else if (ch >= 32 && pos < bufsize - 1) {
            buf[pos++] = ch;
            putchar(ch);
            fflush(stdout);
        }
    }

    buf[pos] = 0;
    return pos;
}

// =============================================================================
// PARSE COMMAND
// =============================================================================

void parse_and_run(char* line) {
    /* Skip whitespace */
    while (*line == ' ') line++;
    if (*line == 0) return;

    /* Add to history */
    history_add(line);

    /* Tokenize */
    char* argv[64];
    int argc = 0;
    char* token = strtok(line, " \t");
    while (token && argc < 64) {
        argv[argc++] = token;
        token = strtok(NULL, " \t");
    }
    argv[argc] = NULL;

    if (argc == 0) return;

    /* Dispatch */
    if (strcmp(argv[0], "help") == 0 || strcmp(argv[0], "?") == 0) {
        cmd_help();
    } else if (strcmp(argv[0], "ps") == 0) {
        cmd_ps();
    } else if (strcmp(argv[0], "fork") == 0) {
        cmd_fork(argc, argv);
    } else if (strcmp(argv[0], "kill") == 0) {
        cmd_kill_tak(argc, argv);
    } else if (strcmp(argv[0], "mem") == 0) {
        cmd_mem();
    } else if (strcmp(argv[0], "malloc") == 0) {
        cmd_malloc(argc, argv);
    } else if (strcmp(argv[0], "free") == 0) {
        cmd_free_block(argc, argv);
    } else if (strcmp(argv[0], "fs") == 0) {
        cmd_fs();
    } else if (strcmp(argv[0], "touch") == 0) {
        cmd_touch(argc, argv);
    } else if (strcmp(argv[0], "cat") == 0) {
        cmd_cat(argc, argv);
    } else if (strcmp(argv[0], "rm") == 0) {
        cmd_rm(argc, argv);
    } else if (strcmp(argv[0], "cal") == 0) {
        cmd_cal();
    } else if (strcmp(argv[0], "trit") == 0) {
        cmd_trit(argc, argv);
    } else if (strcmp(argv[0], "b60") == 0) {
        cmd_b60(argc, argv);
    } else if (strcmp(argv[0], "whoami") == 0) {
        cmd_whoami();
    } else if (strcmp(argv[0], "uname") == 0) {
        cmd_uname();
    } else if (strcmp(argv[0], "uptime") == 0) {
        cmd_uptime();
    } else if (strcmp(argv[0], "neofetch") == 0) {
        cmd_neofetch();
    } else if (strcmp(argv[0], "clear") == 0) {
        printf("\033[2J\033[H");
    } else if (strcmp(argv[0], "halt") == 0) {
        printf("\n  Shutting down TAK...\n");
        state_save();
        printf("  " COLOR_GREEN "✓" COLOR_RESET " State saved to %s\n", state_path);
        printf("  " COLOR_GREEN "✓" COLOR_RESET " Goodbye.\n\n");
        exit(0);
    } else if (strcmp(argv[0], "exit") == 0) {
        printf("\n  Goodbye.\n\n");
        exit(0);
    } else {
        printf(COLOR_RED "  Unknown command: %s" COLOR_RESET "\n", argv[0]);
        printf("  Type 'help' for available commands.\n");
    }
}

// =============================================================================
// SIGNAL HANDLERS
// =============================================================================

void handle_sigint(int sig) {
    (void)sig;
    printf("\n");
    show_prompt();
}

void handle_sigtstp(int sig) {
    (void)sig;
    /* ignore Ctrl+Z */
}

// =============================================================================
// MAIN
// =============================================================================

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    /* Determine home directory */
    const char* home = getenv("HOME");
    if (!home) home = "/tmp";

    char tak_dir[512];
    snprintf(tak_dir, sizeof(tak_dir), "%s/%s", home, TAK_HOME);
    fs_set_home(tak_dir);
    snprintf(state_path, sizeof(state_path), "%s/state", tak_dir);

    /* Initialize */
    printf(COLOR_GRAY "  [init] Setting up %s/..." COLOR_RESET "\n", tak_dir);
    fs_init();
    mem_init();
    sched_init();
    state_load();

    /* Signal handlers */
    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);

    /* Check for -c flag (execute command) */
    if (argc >= 3 && strcmp(argv[1], "-c") == 0) {
        parse_and_run(argv[2]);
        return 0;
    }

    /* Interactive mode */
    show_banner();

    char line[TAK_CMD_MAX];
    while (1) {
        sched_tick();
        show_prompt();

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        line[strcspn(line, "\n")] = 0;
        parse_and_run(line);
    }

    state_save();
    return 0;
}
