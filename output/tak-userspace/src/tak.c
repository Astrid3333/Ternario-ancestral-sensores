/**
 * tak.c — Ternary Ancestral Kernel v3 (Userspace)
 *
 * Features added:
 * - Pipe support (cmd1 | cmd2)
 * - Command chaining (cmd1 ; cmd2 && cmd3)
 * - cd, alias, jobs/fg/bg, history with arrows
 * - I/O redirection (>, >>, <)
 * - Script mode (-f file.tak)
 * - Signal handling (Ctrl+C kills foreground, not shell)
 * - Foreground process tracking
 */

#include "ternary.h"
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

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
// GLOBALS
// =============================================================================

static char tak_home[512];
static char state_path[512];
static char cwd[1024];
static pid_t foreground_pid = -1;
static int shell_pgid;

// =============================================================================
// ALIASES
// =============================================================================

#define MAX_ALIASES 32

typedef struct {
    char name[32];
    char expansion[128];
} alias_t;

static alias_t aliases[MAX_ALIASES];
static int n_aliases = 0;

void alias_add(const char* name, const char* expansion) {
    for (int i = 0; i < n_aliases; i++) {
        if (strcmp(aliases[i].name, name) == 0) {
            strncpy(aliases[i].expansion, expansion, 127);
            return;
        }
    }
    if (n_aliases < MAX_ALIASES) {
        strncpy(aliases[n_aliases].name, name, 31);
        strncpy(aliases[n_aliases].expansion, expansion, 127);
        n_aliases++;
    }
}

const char* alias_lookup(const char* name) {
    for (int i = 0; i < n_aliases; i++) {
        if (strcmp(aliases[i].name, name) == 0) {
            return aliases[i].expansion;
        }
    }
    return NULL;
}

void alias_list(void) {
    for (int i = 0; i < n_aliases; i++) {
        printf("  %s=%s\n", aliases[i].name, aliases[i].expansion);
    }
}

// =============================================================================
// JOBS
// =============================================================================

#define MAX_JOBS 16

typedef struct {
    int id;
    pid_t pid;
    char cmd[128];
    int running; /* 1=running, 0=done */
} job_t;

static job_t jobs[MAX_JOBS];
static int n_jobs = 0;
static int next_job_id = 1;

int job_add(pid_t pid, const char* cmd) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].running) {
            jobs[i].id = next_job_id++;
            jobs[i].pid = pid;
            strncpy(jobs[i].cmd, cmd, 127);
            jobs[i].running = 1;
            n_jobs++;
            return jobs[i].id;
        }
    }
    return -1;
}

void job_reap(void) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < MAX_JOBS; i++) {
            if (jobs[i].running && jobs[i].pid == pid) {
                jobs[i].running = 0;
                n_jobs--;
                break;
            }
        }
        /* Also reap TAK processes */
        for (int i = 1; i < TAK_MAX_PROCS; i++) {
            tak_process_t* p = sched_get_process(i);
            if (p->state != PROC_DEAD && p->linux_pid == pid) {
                p->state = PROC_DEAD;
                p->linux_pid = -1;
                break;
            }
        }
        if (pid == foreground_pid) {
            foreground_pid = -1;
        }
    }
}

void job_list(void) {
    printf("\n");
    int found = 0;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].running) {
            printf("  [%d] %s " COLOR_GREEN "running" COLOR_RESET " (pid %d)\n",
                   jobs[i].id, jobs[i].cmd, jobs[i].pid);
            found = 1;
        }
    }
    if (!found) {
        printf("  " COLOR_GRAY "No active jobs" COLOR_RESET "\n");
    }
    printf("\n");
}

int job_fg(int id) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].running && jobs[i].id == id) {
            foreground_pid = jobs[i].pid;
            tcsetpgrp(STDIN_FILENO, jobs[i].pid);
            kill(jobs[i].pid, SIGCONT);
            waitpid(jobs[i].pid, NULL, 0);
            tcsetpgrp(STDIN_FILENO, shell_pgid);
            foreground_pid = -1;
            jobs[i].running = 0;
            n_jobs--;
            return 0;
        }
    }
    return -1;
}

int job_bg(int id) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].running && jobs[i].id == id) {
            kill(jobs[i].pid, SIGCONT);
            printf("  [%d] %d continued\n", jobs[i].id, jobs[i].pid);
            return 0;
        }
    }
    return -1;
}

// =============================================================================
// HISTORY
// =============================================================================

#define TAK_HISTORY 50

static char cmd_history[TAK_HISTORY][TAK_CMD_MAX];
static int history_idx = 0;
static int history_pos = 0;

void history_add(const char* cmd) {
    if (strlen(cmd) == 0) return;
    /* Don't add duplicates of last command */
    if (history_idx > 0 &&
        strcmp(cmd_history[(history_idx - 1) % TAK_HISTORY], cmd) == 0) {
        return;
    }
    strncpy(cmd_history[history_idx % TAK_HISTORY], cmd, TAK_CMD_MAX - 1);
    history_idx++;
    history_pos = history_idx;
}

void history_show(void) {
    int start = history_idx > 20 ? history_idx - 20 : 0;
    for (int i = start; i < history_idx; i++) {
        printf("  %4d  %s\n", i + 1, cmd_history[i % TAK_HISTORY]);
    }
}

const char* history_prev(void) {
    if (history_pos > 0) history_pos--;
    return cmd_history[history_pos % TAK_HISTORY];
}

const char* history_next(void) {
    if (history_pos < history_idx) history_pos++;
    if (history_pos >= history_idx) return "";
    return cmd_history[history_pos % TAK_HISTORY];
}

// =============================================================================
// TERMINAL / RAW MODE
// =============================================================================

static struct termios orig_termios;

void term_init(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
}

void term_raw_on(void) {
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void term_raw_off(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

// =============================================================================
// LINE READER with history arrows
// =============================================================================

int read_line(char* buf, int bufsize) {
    term_raw_on();
    int pos = 0;
    buf[0] = 0;

    while (1) {
        int ch = getchar();
        if (ch == EOF) { term_raw_off(); return -1; }

        if (ch == '\n') {
            term_raw_off();
            printf("\n");
            break;
        } else if (ch == 127 || ch == 8) {
            /* Backspace */
            if (pos > 0) {
                pos--;
                buf[pos] = 0;
                printf("\b \b");
                fflush(stdout);
            }
        } else if (ch == 27) {
            /* Escape sequence: ESC [ A/B/C/D */
            int ch2 = getchar();
            int ch3 = getchar();
            if (ch2 == '[') {
                if (ch3 == 'A') {
                    /* Up arrow: history prev */
                    const char* h = history_prev();
                    /* Clear current line */
                    while (pos > 0) { printf("\b \b"); pos--; }
                    strcpy(buf, h);
                    pos = strlen(buf);
                    printf("%s", buf);
                    fflush(stdout);
                } else if (ch3 == 'B') {
                    /* Down arrow: history next */
                    const char* h = history_next();
                    while (pos > 0) { printf("\b \b"); pos--; }
                    strcpy(buf, h);
                    pos = strlen(buf);
                    printf("%s", buf);
                    fflush(stdout);
                } else if (ch3 == 'C') {
                    /* Right arrow: skip */
                } else if (ch3 == 'D') {
                    /* Left arrow: skip */
                }
            }
        } else if (ch == 4) {
            /* Ctrl+D: EOF */
            if (pos == 0) {
                term_raw_off();
                return -1;
            }
        } else if (ch == 12) {
            /* Ctrl+L: clear */
            printf("\033[2J\033[H");
            show_prompt();
            printf("%s", buf);
            fflush(stdout);
        } else if (ch >= 32 && pos < bufsize - 1) {
            buf[pos++] = ch;
            buf[pos] = 0;
            putchar(ch);
            fflush(stdout);
        }
    }

    return pos;
}

// =============================================================================
// PERSISTENCE
// =============================================================================

void state_save(void) {
    FILE* f = fopen(state_path, "w");
    if (!f) return;

    maya_calendar_t* cal = sched_get_calendar();
    fprintf(f, "tick=%lu\n", (unsigned long)cal->global_tick);
    fprintf(f, "tzolkin=%u\n", cal->tzolkin_day);
    fprintf(f, "haab=%u\n", cal->haab_day);
    fprintf(f, "cwd=%s\n", cwd);

    for (int i = 0; i < n_aliases; i++) {
        fprintf(f, "alias=%s=%s\n", aliases[i].name, aliases[i].expansion);
    }
    fclose(f);
}

void state_load(void) {
    FILE* f = fopen(state_path, "r");
    if (!f) return;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        if (strncmp(line, "cwd=", 4) == 0) {
            strncpy(cwd, line + 4, sizeof(cwd) - 1);
        } else if (strncmp(line, "alias=", 6) == 0) {
            char* eq = strchr(line + 6, '=');
            if (eq) {
                *eq = 0;
                alias_add(line + 6, eq + 1);
            }
        }
    }
    fclose(f);
}

// =============================================================================
// BANNER
// =============================================================================

void show_banner(void) {
    printf("\n");
    printf(COLOR_CYAN COLOR_BOLD);
    printf("  ╔══════════════════════════════════════════════════╗\n");
    printf("  ║         TERNARY ANCESTRAL KERNEL v" TAK_VERSION "           ║\n");
    printf("  ║         Ultra-lite · Runs on Linux               ║\n");
    printf("  ╚══════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET "\n");

    printf("  " COLOR_GREEN "▸ Logic:" COLOR_RESET "     Ternary {-1, 0, +1}\n");
    printf("  " COLOR_GREEN "▸ Memory:" COLOR_RESET "    Base 60 (Babylonian)\n");
    printf("  " COLOR_GREEN "▸ Scheduler:" COLOR_RESET " Maya Tzolkin/Haab\n");
    printf("  " COLOR_GREEN "▸ Filesystem:" COLOR_RESET " Quipu (real directory)\n");
    printf("  " COLOR_GREEN "▸ Pipes:" COLOR_RESET "     cmd1 | cmd2\n");
    printf("  " COLOR_GREEN "▸ Chaining:" COLOR_RESET "   cmd1 ; cmd2 && cmd3\n");
    printf("  " COLOR_GREEN "▸ Redirection:" COLOR_RESET " > >> <\n");
    printf("  " COLOR_GREEN "▸ Jobs:" COLOR_RESET "       jobs, fg, Ctrl+Z\n");
    printf("\n");
    printf("  " COLOR_YELLOW "Type 'help' for commands." COLOR_RESET "\n\n");
}

// =============================================================================
// PROMPT
// =============================================================================

void show_prompt(void) {
    maya_calendar_t* cal = sched_get_calendar();

    /* Shorten cwd for display */
    const char* display_cwd = cwd;
    const char* home = getenv("HOME");
    if (home && strncmp(cwd, home, strlen(home)) == 0) {
        display_cwd = cwd + strlen(home);
        printf(COLOR_GREEN COLOR_BOLD "~" COLOR_RESET);
    }

    printf(COLOR_GREEN COLOR_BOLD "tak" COLOR_RESET);
    printf(COLOR_GRAY "@" COLOR_RESET);
    printf(COLOR_CYAN "mayan" COLOR_RESET);
    printf(COLOR_GRAY ":" COLOR_RESET);
    printf(COLOR_BLUE "%s" COLOR_RESET, display_cwd);
    printf(COLOR_GRAY ":" COLOR_RESET);
    printf(COLOR_BLUE "%u" COLOR_RESET, cal->tzolkin_day);
    printf(COLOR_GRAY "/" COLOR_RESET);
    printf(COLOR_YELLOW "%u" COLOR_RESET, cal->haab_day);
    printf(COLOR_BOLD "$ " COLOR_RESET);
    fflush(stdout);
}

// =============================================================================
// BUILTIN COMMANDS
// =============================================================================

void cmd_help(void) {
    printf("\n");
    printf("  " COLOR_BOLD "Ternary Commands:" COLOR_RESET "\n");
    printf("  ──────────────────────────────────────────────\n");
    printf("  " COLOR_CYAN "help" COLOR_RESET "              Show this help\n");
    printf("  " COLOR_CYAN "ps" COLOR_RESET "                List processes\n");
    printf("  " COLOR_CYAN "fork <cmd...>" COLOR_RESET "     Fork+exec Linux command\n");
    printf("  " COLOR_CYAN "kill <pid>" COLOR_RESET "       Kill TAK process\n");
    printf("  " COLOR_CYAN "cd <dir>" COLOR_RESET "         Change directory\n");
    printf("  " COLOR_CYAN "pwd" COLOR_RESET "              Print working directory\n");
    printf("  " COLOR_CYAN "ls [dir]" COLOR_RESET "         List directory\n");
    printf("  " COLOR_CYAN "mem" COLOR_RESET "              Memory status\n");
    printf("  " COLOR_CYAN "malloc <bytes>" COLOR_RESET "   Allocate block\n");
    printf("  " COLOR_CYAN "free <block>" COLOR_RESET "     Free block\n");
    printf("  " COLOR_CYAN "fs" COLOR_RESET "               List Quipu files\n");
    printf("  " COLOR_CYAN "touch <name> <data>" COLOR_RESET " Create file\n");
    printf("  " COLOR_CYAN "cat <name>" COLOR_RESET "       Read file\n");
    printf("  " COLOR_CYAN "rm <name>" COLOR_RESET "        Delete file\n");
    printf("  " COLOR_CYAN "mkdir <dir>" COLOR_RESET "      Create directory\n");
    printf("  " COLOR_CYAN "mv <src> <dst>" COLOR_RESET "  Move/rename file\n");
    printf("  " COLOR_CYAN "cp <src> <dst>" COLOR_RESET "  Copy file\n");
    printf("  " COLOR_CYAN "cal" COLOR_RESET "             Maya calendar\n");
    printf("  " COLOR_CYAN "trit <n>" COLOR_RESET "        Number in ternary\n");
    printf("  " COLOR_CYAN "b60 <n>" COLOR_RESET "         Number in Base 60\n");
    printf("  " COLOR_CYAN "alias <n>=<v>" COLOR_RESET "   Set alias\n");
    printf("  " COLOR_CYAN "unalias <n>" COLOR_RESET "     Remove alias\n");
    printf("  " COLOR_CYAN "history" COLOR_RESET "         Command history\n");
    printf("  " COLOR_CYAN "jobs" COLOR_RESET "            List background jobs\n");
    printf("  " COLOR_CYAN "fg <id>" COLOR_RESET "         Bring job to foreground\n");
    printf("  " COLOR_CYAN "bg <id>" COLOR_RESET "         Resume job in background\n");
    printf("  " COLOR_CYAN "whoami" COLOR_RESET "          Current user\n");
    printf("  " COLOR_CYAN "uname" COLOR_RESET "           System info\n");
    printf("  " COLOR_CYAN "uptime" COLOR_RESET "          System uptime\n");
    printf("  " COLOR_CYAN "clear" COLOR_RESET "           Clear screen\n");
    printf("  " COLOR_CYAN "neofetch" COLOR_RESET "        Ternary system info\n");
    printf("  " COLOR_CYAN "halt" COLOR_RESET "            Exit TAK\n");
    printf("\n");
    printf("  " COLOR_BOLD "Pipes & Chaining:" COLOR_RESET "\n");
    printf("  cmd1 | cmd2           Pipe output to cmd2\n");
    printf("  cmd1 ; cmd2           Run cmd1 then cmd2\n");
    printf("  cmd1 && cmd2          Run cmd2 only if cmd1 succeeds\n");
    printf("  cmd1 || cmd2          Run cmd2 only if cmd1 fails\n");
    printf("  cmd > file            Redirect stdout to file\n");
    printf("  cmd >> file           Append stdout to file\n");
    printf("  cmd < file            Redirect file to stdin\n");
    printf("  cmd &                 Run in background\n");
    printf("\n");
}

void cmd_cd(int argc, char** argv) {
    const char* dir;
    if (argc < 2) {
        dir = getenv("HOME");
        if (!dir) dir = "/";
    } else {
        dir = argv[1];
        if (strcmp(dir, "~") == 0) {
            dir = getenv("HOME");
        } else if (strcmp(dir, "-") == 0) {
            dir = cwd; /* TODO: save previous dir */
        }
    }

    if (chdir(dir) == 0) {
        getcwd(cwd, sizeof(cwd));
    } else {
        printf("  cd: %s: %s\n", dir, strerror(errno));
    }
}

void cmd_pwd(void) {
    printf("  %s\n", cwd);
}

int cmd_ls(int argc, char** argv) {
    const char* dir = argc > 1 ? argv[1] : ".";
    DIR* d = opendir(dir);
    if (!d) {
        fprintf(stderr, "  ls: %s: %s\n", dir, strerror(errno));
        return 1;
    }

    struct dirent* ent;
    int count = 0;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;

        struct stat st;
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);

        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            printf(COLOR_BLUE "%s/" COLOR_RESET "  ", ent->d_name);
        } else {
            printf("%s  ", ent->d_name);
        }
        count++;
        if (count % 5 == 0) printf("\n");
    }
    if (count % 5 != 0) printf("\n");
    closedir(d);
    return 0;
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
        if (p->state == PROC_ACTIVE)
            printf(COLOR_GREEN "ACTIVE  " COLOR_RESET);
        else
            printf(COLOR_YELLOW "SLEEP   " COLOR_RESET);

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
        printf("  " COLOR_GREEN "✓" COLOR_RESET " TAK PID %d (Linux %d): ",
               slot, p->linux_pid);
        for (int i = 1; i < argc; i++) printf("%s ", argv[i]);
        printf("\n");
    } else {
        printf("  " COLOR_RED "✗" COLOR_RESET " Failed: %s\n", strerror(errno));
    }
}

void cmd_kill_tak(int argc, char** argv) {
    if (argc < 2) {
        printf("  Usage: kill <tak_pid>\n");
        return;
    }
    int pid = atoi(argv[1]);
    if (pid <= 0 || pid >= TAK_MAX_PROCS) {
        printf("  Invalid PID (1-%d)\n", TAK_MAX_PROCS - 1);
        return;
    }
    if (sched_kill(pid) == 0)
        printf("  " COLOR_GREEN "✓" COLOR_RESET " Killed PID %d\n", pid);
    else
        printf("  " COLOR_RED "✗" COLOR_RESET " Could not kill PID %d\n", pid);
}

void cmd_mem(void) {
    int active = mem_get_active_count();
    uint32_t alloc = mem_get_total_allocated();

    printf("\n");
    printf("  " COLOR_BOLD "Memory — Base 60" COLOR_RESET "\n");
    printf("  ──────────────────────────────────────────────\n");
    printf("  Blocks: %d / %d\n", active, TAK_MAX_MEM);
    printf("  Alloc:  %u bytes\n", alloc);
    printf("  Size:   %d bytes/block\n\n", TAK_BLOCK_SIZE);

    printf("  ");
    for (int i = 0; i < TAK_MAX_MEM; i++) {
        mem_block_t* b = mem_get_block(i);
        if (b->flags)
            printf("%s█" COLOR_RESET, mem_color_ansi(b->color));
        else
            printf(COLOR_GRAY "·" COLOR_RESET);
        if ((i + 1) % 30 == 0) printf("\n  ");
    }
    printf("\n\n");
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
        printf("  " COLOR_GREEN "✓" COLOR_RESET " %u bytes → block %d (addr %d:%d)\n",
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
    if (mem_free(block) == 0)
        printf("  " COLOR_GREEN "✓" COLOR_RESET " Freed block %d\n", block);
    else
        printf("  " COLOR_RED "✗" COLOR_RESET " Could not free block %d\n", block);
}

void cmd_fs(void) {
    tak_file_t files[TAK_MAX_FILES];
    int n = fs_list(files, TAK_MAX_FILES);

    printf("\n  " COLOR_BOLD "Quipu — %s" COLOR_RESET "\n", fs_get_root());
    printf("  ──────────────────────────────────────────────\n");

    if (n == 0) {
        printf("  " COLOR_GRAY "(empty)" COLOR_RESET "\n");
    } else {
        for (int i = 0; i < n; i++) {
            printf("  " COLOR_CYAN "%-18s" COLOR_RESET, files[i].name);
            if (files[i].type == 1)
                printf(COLOR_BLUE "  <DIR>" COLOR_RESET);
            else
                printf("%u B", files[i].size);
            printf("\n");
        }
    }
    printf("\n");
}

void cmd_touch(int argc, char** argv) {
    if (argc < 3) { printf("  Usage: touch <name> <data>\n"); return; }
    if (fs_create(argv[1], argv[2], strlen(argv[2])) == 0)
        printf("  " COLOR_GREEN "✓" COLOR_RESET " Created '%s' (%lu bytes)\n",
               argv[1], (unsigned long)strlen(argv[2]));
    else
        printf("  " COLOR_RED "✗" COLOR_RESET " Failed\n");
}

void cmd_cat(int argc, char** argv) {
    if (argc < 2) { printf("  Usage: cat <name>\n"); return; }

    /* Try Quipu first */
    char buf[4096];
    int n = fs_read(argv[1], buf, sizeof(buf));
    if (n >= 0) {
        write(STDOUT_FILENO, buf, n);
        return;
    }

    /* Try real filesystem with raw write (no buffering issues) */
    int fd = open(argv[1], O_RDONLY);
    if (fd >= 0) {
        ssize_t r;
        while ((r = read(fd, buf, sizeof(buf))) > 0) {
            write(STDOUT_FILENO, buf, r);
        }
        close(fd);
        return;
    }

    fprintf(stderr, "cat: %s: %s\n", argv[1], strerror(errno));
}

void cmd_rm(int argc, char** argv) {
    if (argc < 2) { printf("  Usage: rm <name>\n"); return; }
    if (fs_delete(argv[1]) == 0)
        printf("  " COLOR_GREEN "✓" COLOR_RESET " Deleted '%s'\n", argv[1]);
    else
        printf("  " COLOR_RED "✗" COLOR_RESET " Could not delete\n");
}

int cmd_mkdir(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "  Usage: mkdir <dir>\n"); return 1; }
    if (mkdir(argv[1], 0755) == 0) return 0;
    fprintf(stderr, "  mkdir: %s: %s\n", argv[1], strerror(errno));
    return 1;
}

int cmd_mv(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "  Usage: mv <src> <dst>\n"); return 1; }
    if (rename(argv[1], argv[2]) == 0) return 0;
    fprintf(stderr, "  mv: %s -> %s: %s\n", argv[1], argv[2], strerror(errno));
    return 1;
}

int cmd_cp(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "  Usage: cp <src> <dst>\n"); return 1; }
    int src = open(argv[1], O_RDONLY);
    if (src < 0) { fprintf(stderr, "  cp: %s: %s\n", argv[1], strerror(errno)); return 1; }
    int dst = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst < 0) { fprintf(stderr, "  cp: %s: %s\n", argv[2], strerror(errno)); close(src); return 1; }
    char buf[4096];
    ssize_t r;
    while ((r = read(src, buf, sizeof(buf))) > 0) write(dst, buf, r);
    close(src); close(dst);
    return 0;
}

void cmd_cal(void) {
    maya_calendar_t* cal = sched_get_calendar();
    printf("\n  " COLOR_BOLD "Maya Calendar" COLOR_RESET "\n");
    printf("  ──────────────────────────────────────────────\n");
    printf("  Tick:     %lu\n", (unsigned long)cal->global_tick);
    printf("  Tzolkin:  " COLOR_CYAN "%u" COLOR_RESET " / 260\n", cal->tzolkin_day);
    printf("  Haab:     " COLOR_CYAN "%u" COLOR_RESET " / 365\n", cal->haab_day);
    printf("  Load:     ");
    if (cal->system_load == -1) printf(COLOR_GREEN "LOW" COLOR_RESET);
    else if (cal->system_load == 0) printf(COLOR_YELLOW "MEDIUM" COLOR_RESET);
    else printf(COLOR_RED "HIGH" COLOR_RESET);
    printf("\n\n");
}

void cmd_trit(int argc, char** argv) {
    if (argc < 2) { printf("  Usage: trit <number>\n"); return; }
    int n = atoi(argv[1]);
    printf("  %d → ", n);
    if (n == 0) {
        printf(COLOR_GREEN "0" COLOR_RESET);
    } else {
        if (n < 0) { printf(COLOR_RED "-" COLOR_RESET); n = -n; }
        char trits[32];
        int len = 0;
        while (n > 0) {
            int d = n % 3;
            trits[len++] = (d == 0) ? '0' : (d == 1) ? '+' : '-';
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
    if (argc < 2) { printf("  Usage: b60 <number>\n"); return; }
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
    char buf[256];
    FILE* f = fopen("/proc/version", "r");
    if (f) {
        if (fgets(buf, sizeof(buf), f)) {
            buf[strcspn(buf, "\n")] = 0;
            printf("  %s\n", buf);
        }
        fclose(f);
    }
}

void cmd_uptime(void) {
    FILE* f = fopen("/proc/uptime", "r");
    if (f) {
        double up = 0;
        if (fscanf(f, "%lf", &up) == 1) {
            printf("  Uptime: %dh %dm\n", (int)(up / 3600), ((int)up % 3600) / 60);
        }
        fclose(f);
    }
    printf("  TAK ticks: %lu\n", (unsigned long)sched_get_calendar()->global_tick);
}

void cmd_neofetch(void) {
    printf("\n");
    printf(COLOR_CYAN COLOR_BOLD);
    printf("        ╔═══╗         " COLOR_RESET COLOR_BOLD "ternary@mayan\n" COLOR_RESET);
    printf(COLOR_CYAN COLOR_BOLD);
    printf("       ╔╝   ╚╗        " COLOR_RESET "─────────────────\n");
    printf(COLOR_CYAN COLOR_BOLD "      ╔╝  " COLOR_RESET COLOR_YELLOW "▲" COLOR_CYAN COLOR_BOLD "  ╚╗       " COLOR_RESET COLOR_BOLD "OS:" COLOR_RESET "       TAK v" TAK_VERSION "\n");
    printf(COLOR_CYAN COLOR_BOLD "     ╔╝  " COLOR_RESET COLOR_YELLOW "▲ ▲" COLOR_CYAN COLOR_BOLD "  ╚╗      " COLOR_RESET COLOR_BOLD "Kernel:" COLOR_RESET "    Ternary Ancestral\n");
    printf(COLOR_CYAN COLOR_BOLD "    ╔╝  " COLOR_RESET COLOR_YELLOW "▲ ▲ ▲" COLOR_CYAN COLOR_BOLD "  ╚╗     " COLOR_RESET COLOR_BOLD "Substrate:" COLOR_RESET "   Linux (userspace)\n");
    printf(COLOR_CYAN COLOR_BOLD "    ╚╗  " COLOR_RESET COLOR_GREEN "- 0 +" COLOR_CYAN COLOR_BOLD "  ╔╝     " COLOR_RESET COLOR_BOLD "Shell:" COLOR_RESET "       tak-sh v3\n");
    printf(COLOR_CYAN COLOR_BOLD "     ╚╗       ╔╝      " COLOR_RESET COLOR_BOLD "Logic:" COLOR_RESET "        Ternary {-1,0,+1}\n");
    printf(COLOR_CYAN COLOR_BOLD "      ╚╗     ╔╝       " COLOR_RESET COLOR_BOLD "Memory:" COLOR_RESET "       Base 60 (60 blocks)\n");
    printf(COLOR_CYAN COLOR_BOLD "       ╚╗   ╔╝        " COLOR_RESET COLOR_BOLD "Scheduler:" COLOR_RESET "    Maya (260/365)\n");
    printf(COLOR_CYAN COLOR_BOLD "        ╚═══╝         " COLOR_RESET COLOR_BOLD "FS:" COLOR_RESET "           Quipu\n");

    maya_calendar_t* cal = sched_get_calendar();
    printf("\n");
    printf("  " COLOR_BOLD "Calendar:" COLOR_RESET "  Tzolkin %u/260, Haab %u/365\n",
           cal->tzolkin_day, cal->haab_day);
    printf("  " COLOR_BOLD "Processes:" COLOR_RESET " %d active\n", sched_active_count());
    printf("  " COLOR_BOLD "Memory:" COLOR_RESET "    %d blocks\n", mem_get_active_count());
    printf("  " COLOR_BOLD "Aliases:" COLOR_RESET "   %d\n", n_aliases);
    printf("  " COLOR_BOLD "Jobs:" COLOR_RESET "      %d active\n", n_jobs);
    printf("  " COLOR_BOLD "CWD:" COLOR_RESET "       %s\n", cwd);
    printf("\n");
}

void cmd_echo(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        if (i > 1) write(STDOUT_FILENO, " ", 1);
        write(STDOUT_FILENO, argv[i], strlen(argv[i]));
    }
    write(STDOUT_FILENO, "\n", 1);
}

// =============================================================================
// SEGMENT RUNNER — handles pipes, redirection, background for one segment
// =============================================================================

int run_segment(char* line) {
    while (*line == ' ') line++;
    if (*line == 0) return 0;

    /* Check for pipes */
    char* pipe_cmds[16];
    int n_pipes = 0;

    char* saveptr;
    char* pipe_tok = strtok_r(line, "|", &saveptr);
    while (pipe_tok && n_pipes < 16) {
        while (*pipe_tok == ' ') pipe_tok++;
        pipe_cmds[n_pipes++] = pipe_tok;
        pipe_tok = strtok_r(NULL, "|", &saveptr);
    }

    if (n_pipes > 1) {
        return run_piped(pipe_cmds, n_pipes);
    }

    /* Single command with possible redirection and background */
    char* cmd = line;

    /* Check for redirection */
    char* redir_out = NULL;
    char* redir_append = NULL;
    char* redir_in = NULL;

    char* r;
    if ((r = strstr(cmd, ">>"))) {
        *r = 0; r += 2;
        while (*r == ' ') r++;
        redir_append = r;
        char* end = r + strlen(r) - 1;
        while (end > r && *end == ' ') *end-- = 0;
    } else if ((r = strstr(cmd, ">"))) {
        *r = 0; r++;
        while (*r == ' ') r++;
        redir_out = r;
        char* end = r + strlen(r) - 1;
        while (end > r && *end == ' ') *end-- = 0;
    }
    if ((r = strstr(cmd, "<"))) {
        *r = 0; r++;
        while (*r == ' ') r++;
        redir_in = r;
        char* end = r + strlen(r) - 1;
        while (end > r && *end == ' ') *end-- = 0;
    }

    /* Apply redirections */
    int saved_stdout = -1, saved_stdin = -1;

    if (redir_out) {
        saved_stdout = dup(STDOUT_FILENO);
        int fd = open(redir_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) { dup2(fd, STDOUT_FILENO); close(fd); }
    } else if (redir_append) {
        saved_stdout = dup(STDOUT_FILENO);
        int fd = open(redir_append, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd >= 0) { dup2(fd, STDOUT_FILENO); close(fd); }
    }

    if (redir_in) {
        saved_stdin = dup(STDIN_FILENO);
        int fd = open(redir_in, O_RDONLY);
        if (fd >= 0) { dup2(fd, STDIN_FILENO); close(fd); }
    }

    int ret = run_single(cmd);

    /* Restore fds */
    if (saved_stdout >= 0) { dup2(saved_stdout, STDOUT_FILENO); close(saved_stdout); }
    if (saved_stdin >= 0) { dup2(saved_stdin, STDIN_FILENO); close(saved_stdin); }

    return ret;
}

// =============================================================================
// COMMAND DISPATCH
// =============================================================================

int run_single(char* line) {
    while (*line == ' ') line++;
    if (*line == 0) return 0;

    /* Tokenize */
    char* argv[64];
    int argc = 0;

    char* tok = strtok(line, " \t");
    while (tok && argc < 64) {
        argv[argc++] = tok;
        tok = strtok(NULL, " \t");
    }
    argv[argc] = NULL;

    if (argc == 0) return 0;

    /* Fork for ALL commands (builtins and external) so redirections work */
    int background = 0;
    if (argc > 1 && strcmp(argv[argc - 1], "&") == 0) {
        background = 1;
        argv[--argc] = NULL;
    }

    pid_t pid = fork();
    if (pid == 0) {
        /* Child: run builtin or exec */
        setpgid(0, 0);

        /* Check builtins */
        int is_builtin = 0;
        int builtin_rc = 0;
        if (strcmp(argv[0], "help") == 0 || strcmp(argv[0], "?") == 0) { cmd_help(); is_builtin = 1; }
        else if (strcmp(argv[0], "ps") == 0) { cmd_ps(); is_builtin = 1; }
        else if (strcmp(argv[0], "fork") == 0) { cmd_fork(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "kill") == 0) { cmd_kill_tak(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "cd") == 0) { cmd_cd(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "pwd") == 0) { cmd_pwd(); is_builtin = 1; }
        else if (strcmp(argv[0], "ls") == 0) { builtin_rc = cmd_ls(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "mem") == 0) { cmd_mem(); is_builtin = 1; }
        else if (strcmp(argv[0], "malloc") == 0) { cmd_malloc(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "free") == 0) { cmd_free_block(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "fs") == 0) { cmd_fs(); is_builtin = 1; }
        else if (strcmp(argv[0], "touch") == 0) { cmd_touch(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "cat") == 0) { cmd_cat(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "rm") == 0) { cmd_rm(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "mkdir") == 0) { builtin_rc = cmd_mkdir(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "mv") == 0) { builtin_rc = cmd_mv(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "cp") == 0) { builtin_rc = cmd_cp(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "cal") == 0) { cmd_cal(); is_builtin = 1; }
        else if (strcmp(argv[0], "trit") == 0) { cmd_trit(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "b60") == 0) { cmd_b60(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "whoami") == 0) { cmd_whoami(); is_builtin = 1; }
        else if (strcmp(argv[0], "uname") == 0) { cmd_uname(); is_builtin = 1; }
        else if (strcmp(argv[0], "uptime") == 0) { cmd_uptime(); is_builtin = 1; }
        else if (strcmp(argv[0], "echo") == 0) { cmd_echo(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "clear") == 0 || argv[0][0] == 12) { printf("\033[2J\033[H"); is_builtin = 1; }
        else if (strcmp(argv[0], "neofetch") == 0) { cmd_neofetch(); is_builtin = 1; }
        else if (strcmp(argv[0], "halt") == 0 || strcmp(argv[0], "exit") == 0) {
            _exit(0);
        }
        else if (strcmp(argv[0], "history") == 0) { history_show(); is_builtin = 1; }
        else if (strcmp(argv[0], "jobs") == 0) { job_list(); is_builtin = 1; }
        else if (strcmp(argv[0], "fg") == 0) {
            if (argc < 2) { fprintf(stderr, "  Usage: fg <job_id>\n"); builtin_rc = 1; }
            else if (job_fg(atoi(argv[1])) != 0) { fprintf(stderr, "  No such job: %s\n", argv[1]); builtin_rc = 1; }
            is_builtin = 1;
        }
        else if (strcmp(argv[0], "bg") == 0) {
            if (argc < 2) { fprintf(stderr, "  Usage: bg <job_id>\n"); builtin_rc = 1; }
            else if (job_bg(atoi(argv[1])) != 0) { fprintf(stderr, "  No such job: %s\n", argv[1]); builtin_rc = 1; }
            is_builtin = 1;
        }

        if (is_builtin) {
            fflush(stdout);
            _exit(builtin_rc);
        }

        /* External: PATH lookup + exec */
        char path_buf[1024];
        const char* path_env = getenv("PATH");
        if (path_env && argv[0][0] != '/') {
            char* path_copy = strdup(path_env);
            char* sv;
            char* d = strtok_r(path_copy, ":", &sv);
            int found = 0;
            while (d) {
                snprintf(path_buf, sizeof(path_buf), "%s/%s", d, argv[0]);
                if (access(path_buf, X_OK) == 0) { found = 1; break; }
                d = strtok_r(NULL, ":", &sv);
            }
            free(path_copy);
            if (!found) snprintf(path_buf, sizeof(path_buf), "%s", argv[0]);
        } else {
            snprintf(path_buf, sizeof(path_buf), "%s", argv[0]);
        }

        execvp(path_buf, argv);
        fprintf(stderr, "tak: %s: %s\n", argv[0], strerror(errno));
        _exit(127);
    } else if (pid > 0) {
        /* Parent */
        if (background) {
            setpgid(pid, pid);
            int jid = job_add(pid, argv[0]);
            printf("  [%d] %d\n", jid, pid);
            return 0;
        } else {
            foreground_pid = pid;
            setpgid(pid, shell_pgid);
            tcsetpgrp(STDIN_FILENO, pid);
            int status;
            waitpid(pid, &status, 0);
            tcsetpgrp(STDIN_FILENO, shell_pgid);
            foreground_pid = -1;
            if (WIFEXITED(status)) return WEXITSTATUS(status);
            if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
            return 1;
        }
    } else {
        printf("  fork: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}

// =============================================================================
// PIPE EXECUTION
// =============================================================================

int run_piped(char** cmds, int ncmds) {
    int prev_fd = -1;
    int fd[2];
    pid_t last_pid = -1;

    for (int i = 0; i < ncmds; i++) {
        if (pipe(fd) < 0) { perror("pipe"); return 1; }

        pid_t pid = fork();
        if (pid == 0) {
            /* Child */
            setpgid(0, 0);
            if (prev_fd != -1) {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }
            if (i < ncmds - 1) {
                dup2(fd[1], STDOUT_FILENO);
            }
            close(fd[0]);
            close(fd[1]);

            /* Tokenize and exec */
            char buf[TAK_CMD_MAX];
            strncpy(buf, cmds[i], TAK_CMD_MAX - 1);
            buf[TAK_CMD_MAX - 1] = 0;

            char* argv[64];
            int argc = 0;
            char* tok = strtok(buf, " \t");
            while (tok && argc < 64) { argv[argc++] = tok; tok = strtok(NULL, " \t"); }
            argv[argc] = NULL;
            if (argc == 0) _exit(0);

            /* Try builtin first */
            if (strcmp(argv[0], "trit") == 0 || strcmp(argv[0], "b60") == 0 ||
                strcmp(argv[0], "cal") == 0 || strcmp(argv[0], "mem") == 0 ||
                strcmp(argv[0], "whoami") == 0) {
                char orig[TAK_CMD_MAX];
                strncpy(orig, cmds[i], TAK_CMD_MAX - 1);
                run_single(orig);
                _exit(0);
            }

            /* External: PATH lookup + exec */
            char path_buf[1024];
            const char* path_env = getenv("PATH");
            if (path_env && argv[0][0] != '/') {
                char* pc = strdup(path_env);
                char* sv;
                char* d = strtok_r(pc, ":", &sv);
                int found = 0;
                while (d) {
                    snprintf(path_buf, sizeof(path_buf), "%s/%s", d, argv[0]);
                    if (access(path_buf, X_OK) == 0) { found = 1; break; }
                    d = strtok_r(NULL, ":", &sv);
                }
                free(pc);
                if (!found) snprintf(path_buf, sizeof(path_buf), "%s", argv[0]);
            } else {
                snprintf(path_buf, sizeof(path_buf), "%s", argv[0]);
            }
            execvp(path_buf, argv);
            fprintf(stderr, "tak: %s: %s\n", argv[0], strerror(errno));
            _exit(127);
        }

        /* Parent */
        if (prev_fd != -1) close(prev_fd);
        close(fd[1]);
        prev_fd = fd[0];
        last_pid = pid;
    }

    if (prev_fd != -1) close(prev_fd);
    /* Wait for all children, return last one's status */
    int status = 0;
    for (int i = 0; i < ncmds; i++) {
        int s;
        wait(&s);
        if (i == ncmds - 1) status = s;
    }
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return 1;
}

// =============================================================================
// LINE DISPATCHER — ; && || | > >> < &
// =============================================================================

/* Find operator at top level (not inside quotes, not part of another word) */
static char* find_op(const char* line, const char* op) {
    int len = strlen(op);
    for (int i = 0; line[i]; i++) {
        if (strncmp(line + i, op, len) == 0) {
            /* Make sure it's not part of &&& or ||| */
            if (len == 1 && (op[0] == '&' || op[0] == '|')) {
                if (line[i + 1] == op[0]) continue; /* skip && and || */
            }
            return (char*)(line + i);
        }
    }
    return NULL;
}

int parse_and_run(char* line) {
    while (*line == ' ') line++;
    if (*line == 0) return 0;

    history_add(line);

    /* Alias/Unalias handled first (before any splitting) */
    if (strcmp(line, "alias") == 0) { alias_list(); return 0; }
    if (strncmp(line, "alias ", 6) == 0) {
        char* arg = line + 6;
        char* eq = strchr(arg, '=');
        if (eq) {
            *eq = 0;
            alias_add(arg, eq + 1);
            printf("  alias %s=%s\n", arg, eq + 1);
        } else {
            const char* val = alias_lookup(arg);
            if (val) printf("  %s=%s\n", arg, val);
            else printf("  alias: %s: not found\n", arg);
        }
        return 0;
    }
    if (strncmp(line, "unalias ", 8) == 0) {
        char* name = line + 8;
        for (int i = 0; i < n_aliases; i++) {
            if (strcmp(aliases[i].name, name) == 0) {
                aliases[i] = aliases[--n_aliases];
                printf("  Removed alias '%s'\n", name);
                return 0;
            }
        }
        printf("  alias: %s: not found\n", name);
        return 0;
    }

    /* Alias expansion: if first word is an alias, expand it */
    char expanded[TAK_CMD_MAX];
    char* sp = strchr(line, ' ');
    int first_word_len = sp ? (int)(sp - line) : (int)strlen(line);
    char first_word[64];
    if (first_word_len < 63) {
        strncpy(first_word, line, first_word_len);
        first_word[first_word_len] = 0;
        const char* exp = alias_lookup(first_word);
        if (exp) {
            snprintf(expanded, TAK_CMD_MAX, "%s%s", exp, sp ? sp : "");
            line = expanded;
        }
    }

    /* 1. Split on ; */
    char* semi = find_op(line, ";");
    if (semi) {
        *semi = 0;
        parse_and_run(line);
        return parse_and_run(semi + 1);
    }

    /* 2. Split on && */
    char* andop = find_op(line, "&&");
    if (andop) {
        *andop = 0; andop += 2;
        int left = parse_and_run(line);
        if (left == 0) return parse_and_run(andop);
        return left;
    }

    /* 3. Split on || */
    char* orop = find_op(line, "||");
    if (orop) {
        *orop = 0; orop += 2;
        int left = parse_and_run(line);
        if (left != 0) return parse_and_run(orop);
        return left;
    }

    /* 4. Check for pipes */
    char* pipe_cmds[16];
    int n_pipes = 0;

    char* saveptr;
    char* pipe_tok = strtok_r(line, "|", &saveptr);
    while (pipe_tok && n_pipes < 16) {
        while (*pipe_tok == ' ') pipe_tok++;
        if (*pipe_tok) pipe_cmds[n_pipes++] = pipe_tok;
        pipe_tok = strtok_r(NULL, "|", &saveptr);
    }

    if (n_pipes > 1) {
        return run_piped(pipe_cmds, n_pipes);
    }

    /* 5. Single command: handle redirection + background */
    return run_segment(line);
}

// =============================================================================
// SIGNALS
// =============================================================================

void handle_sigint(int sig) {
    (void)sig;
    if (foreground_pid > 0) {
        kill(foreground_pid, SIGINT);
    } else {
        printf("\n");
        show_prompt();
    }
}

void handle_sigtstp(int sig) {
    (void)sig;
    if (foreground_pid > 0) {
        kill(foreground_pid, SIGTSTP);
        job_add(foreground_pid, "(stopped)");
        printf("\n  [" COLOR_YELLOW "stopped" COLOR_RESET "] %d\n", foreground_pid);
        foreground_pid = -1;
        tcsetpgrp(STDIN_FILENO, shell_pgid);
        show_prompt();
    }
}

void handle_sigchld(int sig) {
    (void)sig;
    job_reap();
}

// =============================================================================
// MAIN
// =============================================================================

int main(int argc, char* argv[]) {
    /* Home directory */
    const char* home = getenv("HOME");
    if (!home) home = "/tmp";
    snprintf(tak_home, sizeof(tak_home), "%s/%s", home, TAK_HOME);
    fs_set_home(tak_home);
    snprintf(state_path, sizeof(state_path), "%s/state", tak_home);
    getcwd(cwd, sizeof(cwd));

    /* Init */
    printf(COLOR_GRAY "  [init] %s/" COLOR_RESET "\n", tak_home);
    fs_init();
    mem_init();
    sched_init();
    state_load();

    /* Signals */
    shell_pgid = getpid();
    setpgid(0, shell_pgid);
    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);
    signal(SIGCHLD, handle_sigchld);

    /* -f script mode */
    if (argc >= 3 && strcmp(argv[1], "-f") == 0) {
        FILE* f = fopen(argv[2], "r");
        if (!f) {
            fprintf(stderr, "tak: %s: %s\n", argv[2], strerror(errno));
            return 1;
        }
        char line[TAK_CMD_MAX];
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = 0;
            if (line[0] == '#' || line[0] == 0) continue;
            printf(COLOR_GRAY "  $ " COLOR_RESET "%s\n", line);
            parse_and_run(line);
        }
        fclose(f);
        state_save();
        return 0;
    }

    /* -c single command */
    if (argc >= 3 && strcmp(argv[1], "-c") == 0) {
        parse_and_run(argv[2]);
        return 0;
    }

    /* Interactive mode */
    show_banner();

    char line[TAK_CMD_MAX];
    while (1) {
        sched_tick();
        job_reap();
        show_prompt();

        if (read_line(line, sizeof(line)) < 0) {
            printf("\n");
            break;
        }

        parse_and_run(line);
    }

    state_save();
    return 0;
}
