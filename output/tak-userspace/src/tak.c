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

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#include "ternary.h"
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/time.h>
#include <termios.h>
#include <utmp.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include <sys/select.h>
#include <dirent.h>

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
    printf("  " COLOR_CYAN "grep [-inv] <pat> [f]" COLOR_RESET " Search pattern\n");
    printf("  " COLOR_CYAN "sort [-rn] [file]" COLOR_RESET " Sort lines\n");
    printf("  " COLOR_CYAN "wc [-lwc] [file]" COLOR_RESET " Word/line/char count\n");
    printf("  " COLOR_CYAN "head [-n N] [file]" COLOR_RESET " First N lines\n");
    printf("  " COLOR_CYAN "tail [-n N] [file]" COLOR_RESET " Last N lines\n");
    printf("  " COLOR_CYAN "find <dir> [-name p]" COLOR_RESET " Find files\n");
    printf("  " COLOR_CYAN "chmod <mode> <file>" COLOR_RESET " Change permissions\n");
    printf("  " COLOR_CYAN "du [dir]" COLOR_RESET "          Disk usage\n");
    printf("  " COLOR_CYAN "df [path]" COLOR_RESET "         Filesystem space\n");
    printf("  " COLOR_CYAN "env" COLOR_RESET "              List environment\n");
    printf("  " COLOR_CYAN "export VAR=val" COLOR_RESET "  Set environment\n");
    printf("  " COLOR_CYAN "tee [-a] <file>" COLOR_RESET " Write stdin to file+stdout\n");
    printf("  " COLOR_CYAN "date [-u]" COLOR_RESET "        Show date/time\n");
    printf("  " COLOR_CYAN "sleep <n>" COLOR_RESET "       Sleep N seconds\n");
    printf("  " COLOR_CYAN "which <cmd>" COLOR_RESET "     Find command path\n");
    printf("  " COLOR_CYAN "diff <f1> <f2>" COLOR_RESET " Compare two files\n");
    printf("  " COLOR_CYAN "xargs <cmd>" COLOR_RESET "    Build cmd from stdin\n");
    printf("  " COLOR_CYAN "cut -d'\\t' -f1,2" COLOR_RESET " Cut fields\n");
    printf("  " COLOR_CYAN "uniq [-c] [file]" COLOR_RESET " Unique lines\n");
    printf("  " COLOR_CYAN "tr <from> <to>" COLOR_RESET " Translate chars\n");
    printf("  " COLOR_CYAN "ln [-s] <t> <l>" COLOR_RESET " Create link\n");
    printf("  " COLOR_CYAN "stat <file>" COLOR_RESET "     File info\n");
    printf("  " COLOR_CYAN "time <cmd>" COLOR_RESET "      Measure time\n");
    printf("  " COLOR_CYAN "gcc <file.c>" COLOR_RESET "   Compile C code\n");
    printf("  " COLOR_CYAN "curl <url>" COLOR_RESET "     HTTP request\n");
    printf("  " COLOR_CYAN "who" COLOR_RESET "            Logged in users\n");
    printf("  " COLOR_CYAN "su [user]" COLOR_RESET "      Switch user\n");
    printf("  " COLOR_CYAN "passwd [user]" COLOR_RESET "  Change password\n");
    printf("  " COLOR_CYAN "top" COLOR_RESET "            Process monitor\n");
    printf("  " COLOR_CYAN "tar [-xvf] <arch>" COLOR_RESET " Archive\n");
    printf("  " COLOR_CYAN "gzip <file>" COLOR_RESET "    Compress\n");
    printf("  " COLOR_CYAN "ping <host>" COLOR_RESET "    Test network\n");
    printf("  " COLOR_CYAN "traceroute <h>" COLOR_RESET " Route trace\n");
    printf("  " COLOR_CYAN "nslookup <h>" COLOR_RESET "  DNS lookup\n");
    printf("  " COLOR_CYAN "ifconfig" COLOR_RESET "       Network config\n");
    printf("  " COLOR_CYAN "netstat" COLOR_RESET "        Network stats\n");
    printf("  " COLOR_CYAN "chown <u> <f>" COLOR_RESET " Change owner\n");
    printf("  " COLOR_CYAN "adduser <u>" COLOR_RESET "   Add user\n");
    printf("  " COLOR_CYAN "groups [u]" COLOR_RESET "    Show groups\n");
    printf("  " COLOR_CYAN "dmesg" COLOR_RESET "          Kernel messages\n");
    printf("  " COLOR_CYAN "mount" COLOR_RESET "          Mount points\n");
    printf("  " COLOR_CYAN "systemctl" COLOR_RESET "      System services\n");
    printf("  " COLOR_CYAN "journalctl" COLOR_RESET "     System logs\n");
    printf("  " COLOR_CYAN "lscpu" COLOR_RESET "          CPU info\n");
    printf("  " COLOR_CYAN "lsusb" COLOR_RESET "          USB devices\n");
    printf("  " COLOR_CYAN "locate <p>" COLOR_RESET "    Find files\n");
    printf("  " COLOR_CYAN "nano <file>" COLOR_RESET "    Text editor\n");
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

int cmd_grep(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "  Usage: grep <pattern> [file]\n"); return 1; }
    const char* pattern = argv[1];
    int ignore_case = 0;
    int invert = 0;
    int line_numbers = 0;
    /* Parse flags */
    int ai = 1;
    while (ai < argc && argv[ai][0] == '-') {
        for (int j = 1; argv[ai][j]; j++) {
            if (argv[ai][j] == 'i') ignore_case = 1;
            else if (argv[ai][j] == 'v') invert = 1;
            else if (argv[ai][j] == 'n') line_numbers = 1;
        }
        ai++;
    }
    if (ai >= argc) { fprintf(stderr, "  Usage: grep [-inv] <pattern> [file]\n"); return 1; }
    pattern = argv[ai];
    ai++;

    /* Read from stdin or file */
    FILE* fp = stdin;
    char* line = NULL;
    size_t len = 0;
    ssize_t nread;
    int line_num = 0;
    int matches = 0;

    if (ai < argc) {
        fp = fopen(argv[ai], "r");
        if (!fp) { fprintf(stderr, "  grep: %s: %s\n", argv[ai], strerror(errno)); return 1; }
    }

    while ((nread = getline(&line, &len, fp)) != -1) {
        line_num++;
        /* Remove trailing newline */
        if (nread > 0 && line[nread - 1] == '\n') line[nread - 1] = 0;
        int found = 0;
        if (ignore_case) {
            char* lower_line = strdup(line);
            char* lower_pat = strdup(pattern);
            for (int i = 0; lower_line[i]; i++) lower_line[i] = tolower(lower_line[i]);
            for (int i = 0; lower_pat[i]; i++) lower_pat[i] = tolower(lower_pat[i]);
            found = strstr(lower_line, lower_pat) != NULL;
            free(lower_line); free(lower_pat);
        } else {
            found = strstr(line, pattern) != NULL;
        }
        if (found != invert) {
            matches++;
            if (line_numbers) printf("%d:", line_num);
            printf("%s\n", line);
        }
    }

    free(line);
    if (fp != stdin) fclose(fp);
    return matches == 0 ? 1 : 0;
}

int cmd_sort(int argc, char** argv) {
    int reverse = 0;
    int numeric = 0;
    int ai = 1;
    while (ai < argc && argv[ai][0] == '-') {
        for (int j = 1; argv[ai][j]; j++) {
            if (argv[ai][j] == 'r') reverse = 1;
            else if (argv[ai][j] == 'n') numeric = 1;
        }
        ai++;
    }

    char* lines[4096];
    int n = 0;
    char buf[4096];
    FILE* fp = stdin;
    if (ai < argc) {
        fp = fopen(argv[ai], "r");
        if (!fp) { fprintf(stderr, "  sort: %s: %s\n", argv[ai], strerror(errno)); return 1; }
    }
    while (fgets(buf, sizeof(buf), fp) && n < 4096) {
        lines[n] = strdup(buf);
        n++;
    }
    if (fp != stdin) fclose(fp);

    /* Sort using strcmp or atoi */
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            int cmp;
            if (numeric) cmp = atoi(lines[i]) - atoi(lines[j]);
            else cmp = strcmp(lines[i], lines[j]);
            if (reverse ? cmp < 0 : cmp > 0) {
                char* tmp = lines[i]; lines[i] = lines[j]; lines[j] = tmp;
            }
        }
    }

    for (int i = 0; i < n; i++) {
        printf("%s", lines[i]);
        free(lines[i]);
    }
    return 0;
}

int cmd_wc(int argc, char** argv) {
    int show_lines = 1, show_words = 1, show_chars = 1;
    int ai = 1;
    while (ai < argc && argv[ai][0] == '-') {
        show_lines = show_words = show_chars = 0;
        for (int j = 1; argv[ai][j]; j++) {
            if (argv[ai][j] == 'l') show_lines = 1;
            else if (argv[ai][j] == 'w') show_words = 1;
            else if (argv[ai][j] == 'c') show_chars = 1;
        }
        ai++;
    }
    if (!show_lines && !show_words && !show_chars) show_lines = show_words = show_chars = 1;

    FILE* fp = stdin;
    if (ai < argc) {
        fp = fopen(argv[ai], "r");
        if (!fp) { fprintf(stderr, "  wc: %s: %s\n", argv[ai], strerror(errno)); return 1; }
    }

    int lines = 0, words = 0, chars = 0;
    char buf[4096];
    int in_word = 0;
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        for (size_t i = 0; i < n; i++) {
            chars++;
            if (buf[i] == '\n') lines++;
            if (buf[i] == ' ' || buf[i] == '\n' || buf[i] == '\t') in_word = 0;
            else if (!in_word) { in_word = 1; words++; }
        }
    }
    if (fp != stdin) fclose(fp);

    if (show_lines) printf("%d ", lines);
    if (show_words) printf("%d ", words);
    if (show_chars) printf("%d ", chars);
    if (ai < argc) printf("%s", argv[ai]);
    printf("\n");
    return 0;
}

int cmd_head(int argc, char** argv) {
    int n = 10;
    int ai = 1;
    if (ai < argc && strcmp(argv[ai], "-n") == 0) { ai++; if (ai < argc) n = atoi(argv[ai++]); }
    if (ai < argc && argv[ai][0] == '-' && argv[ai][1] >= '0' && argv[ai][1] <= '9') { n = atoi(argv[ai] + 1); ai++; }

    FILE* fp = stdin;
    if (ai < argc) {
        fp = fopen(argv[ai], "r");
        if (!fp) { fprintf(stderr, "  head: %s: %s\n", argv[ai], strerror(errno)); return 1; }
    }

    char buf[4096];
    int count = 0;
    while (count < n && fgets(buf, sizeof(buf), fp)) {
        printf("%s", buf);
        count++;
    }
    if (fp != stdin) fclose(fp);
    return 0;
}

int cmd_tail(int argc, char** argv) {
    int n = 10;
    int ai = 1;
    if (ai < argc && strcmp(argv[ai], "-n") == 0) { ai++; if (ai < argc) n = atoi(argv[ai++]); }
    if (ai < argc && argv[ai][0] == '-' && argv[ai][1] >= '0' && argv[ai][1] <= '9') { n = atoi(argv[ai] + 1); ai++; }

    FILE* fp = stdin;
    if (ai < argc) {
        fp = fopen(argv[ai], "r");
        if (!fp) { fprintf(stderr, "  tail: %s: %s\n", argv[ai], strerror(errno)); return 1; }
    }

    char* ring[4096];
    int ring_size = 0;
    char buf[4096];
    while (fgets(buf, sizeof(buf), fp)) {
        if (ring_size < 4096) {
            ring[ring_size++] = strdup(buf);
        } else {
            free(ring[0]);
            for (int i = 0; i < 4095; i++) ring[i] = ring[i + 1];
            ring[4095] = strdup(buf);
        }
    }
    if (fp != stdin) fclose(fp);

    int start = ring_size > n ? ring_size - n : 0;
    for (int i = start; i < ring_size; i++) {
        printf("%s", ring[i]);
        free(ring[i]);
    }
    return 0;
}

int cmd_find(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "  Usage: find <dir> [-name pattern]\n"); return 1; }
    const char* root = argv[1];
    const char* name_pattern = NULL;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-name") == 0 && i + 1 < argc) name_pattern = argv[++i];
    }

    char stack[256][1024];
    int sp = 0;
    strncpy(stack[sp], root, 1023); sp++;

    while (sp > 0) {
        sp--;
        char dir[1024];
        strncpy(dir, stack[sp], 1023); dir[1023] = 0;

        DIR* d = opendir(dir);
        if (!d) continue;

        struct dirent* ent;
        while ((ent = readdir(d)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;

            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);

            struct stat st;
            if (lstat(path, &st) < 0) continue;

            int match = 1;
            if (name_pattern) {
                match = 0;
                /* Simple wildcard match: *pat*, pat*, *pat, exact */
                if (name_pattern[0] == '*' && name_pattern[strlen(name_pattern)-1] == '*') {
                    match = strstr(ent->d_name, name_pattern + 1) != NULL;
                } else if (name_pattern[0] == '*') {
                    match = strcmp(ent->d_name + strlen(ent->d_name) - strlen(name_pattern + 1), name_pattern + 1) == 0;
                } else if (name_pattern[strlen(name_pattern)-1] == '*') {
                    match = strncmp(ent->d_name, name_pattern, strlen(name_pattern) - 1) == 0;
                } else {
                    match = strcmp(ent->d_name, name_pattern) == 0;
                }
            }

            if (match) printf("%s\n", path);

            if (S_ISDIR(st.st_mode) && sp < 256) {
                strncpy(stack[sp], path, 1023);
                sp++;
            }
        }
        closedir(d);
    }
    return 0;
}

int cmd_chmod(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "  Usage: chmod <mode> <file>\n"); return 1; }
    int mode = (int)strtol(argv[1], NULL, 8);
    if (chmod(argv[2], mode) == 0) return 0;
    fprintf(stderr, "  chmod: %s: %s\n", argv[2], strerror(errno));
    return 1;
}

int cmd_du(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : ".";
    struct stat st;
    if (stat(path, &st) < 0) { fprintf(stderr, "  du: %s: %s\n", path, strerror(errno)); return 1; }

    if (S_ISDIR(st.st_mode)) {
        DIR* d = opendir(path);
        if (!d) { fprintf(stderr, "  du: %s: %s\n", path, strerror(errno)); return 1; }
        long total = 0;
        struct dirent* ent;
        while ((ent = readdir(d)) != NULL) {
            if (ent->d_name[0] == '.') continue;
            char sub[1024];
            snprintf(sub, sizeof(sub), "%s/%s", path, ent->d_name);
            struct stat ss;
            if (stat(sub, &ss) == 0) total += ss.st_size;
        }
        closedir(d);
        printf("%ld\t%s\n", total / 1024, path);
    } else {
        printf("%ld\t%s\n", (long)st.st_size / 1024, path);
    }
    return 0;
}

int cmd_df(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : ".";
    struct statfs sf;
    if (statfs(path, &sf) < 0) { fprintf(stderr, "  df: %s: %s\n", path, strerror(errno)); return 1; }
    long total = (sf.f_blocks * sf.f_bsize) / 1024;
    long used = ((sf.f_blocks - sf.f_bfree) * sf.f_bsize) / 1024;
    long avail = (sf.f_bavail * sf.f_bsize) / 1024;
    printf("Filesystem     1K-blocks    Used Available Use%% Mounted on\n");
    printf("tak-fs         %9ld %7ld %9ld  --  %s\n", total, used, avail, path);
    return 0;
}

int cmd_env(int argc, char** argv) {
    extern char** environ;
    for (int i = 0; environ[i]; i++) printf("%s\n", environ[i]);
    return 0;
}

int cmd_export(int argc, char** argv) {
    if (argc < 2) { cmd_env(0, NULL); return 0; }
    for (int i = 1; i < argc; i++) {
        char* eq = strchr(argv[i], '=');
        if (eq) {
            *eq = 0;
            setenv(argv[i], eq + 1, 1);
        } else {
            /* export VAR → mark for export (no-op in our simple shell) */
        }
    }
    return 0;
}

int cmd_tee(int argc, char** argv) {
    int append = 0;
    int ai = 1;
    if (ai < argc && strcmp(argv[ai], "-a") == 0) { append = 1; ai++; }
    if (ai >= argc) { fprintf(stderr, "  Usage: tee [-a] <file>\n"); return 1; }

    int fd = open(argv[ai], O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC), 0644);
    if (fd < 0) { fprintf(stderr, "  tee: %s: %s\n", argv[ai], strerror(errno)); return 1; }

    char buf[4096];
    ssize_t n;
    while ((n = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        write(STDOUT_FILENO, buf, n);
        write(fd, buf, n);
    }
    close(fd);
    return 0;
}

int cmd_date(int argc, char** argv) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char buf[256];
    if (argc > 1 && strcmp(argv[1], "-u") == 0) t = gmtime(&now);
    strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Y", t);
    printf("%s\n", buf);
    return 0;
}

int cmd_sleep(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "  Usage: sleep <seconds>\n"); return 1; }
    unsigned int secs = (unsigned int)atoi(argv[1]);
    sleep(secs);
    return 0;
}

int cmd_which(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "  Usage: which <command>\n"); return 1; }
    const char* path_env = getenv("PATH");
    if (!path_env) { fprintf(stderr, "  which: PATH not set\n"); return 1; }
    char* path_copy = strdup(path_env);
    char* sv;
    char* d = strtok_r(path_copy, ":", &sv);
    while (d) {
        char full[1024];
        snprintf(full, sizeof(full), "%s/%s", d, argv[1]);
        if (access(full, X_OK) == 0) { printf("%s\n", full); free(path_copy); return 0; }
        d = strtok_r(NULL, ":", &sv);
    }
    free(path_copy);
    fprintf(stderr, "  which: %s not found\n", argv[1]);
    return 1;
}

int cmd_diff(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "  Usage: diff <file1> <file2>\n"); return 1; }
    FILE* f1 = fopen(argv[1], "r");
    FILE* f2 = fopen(argv[2], "r");
    if (!f1) { fprintf(stderr, "  diff: %s: %s\n", argv[1], strerror(errno)); return 1; }
    if (!f2) { fprintf(stderr, "  diff: %s: %s\n", argv[2], strerror(errno)); fclose(f1); return 1; }

    char line1[4096], line2[4096];
    int line_num = 0;
    int diffs = 0;
    while (1) {
        char* r1 = fgets(line1, sizeof(line1), f1);
        char* r2 = fgets(line2, sizeof(line2), f2);
        line_num++;
        if (!r1 && !r2) break;
        if (!r1 || !r2 || strcmp(line1, line2) != 0) {
            diffs++;
            if (r1) printf("%d: %s", line_num, line1);
            if (r2) printf("%d: %s", line_num, line2);
            if (!r1) printf("%d: end of %s\n", line_num, argv[1]);
            if (!r2) printf("%d: end of %s\n", line_num, argv[2]);
        }
    }
    fclose(f1); fclose(f2);
    return diffs == 0 ? 0 : 1;
}

int cmd_xargs(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "  Usage: xargs <cmd> [args...]\n"); return 1; }
    char line[4096];
    while (fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;
        /* Build command: argv[1] ... original_args ... line */
        char* cmd_argv[256];
        int n = 0;
        for (int i = 1; i < argc && n < 250; i++) cmd_argv[n++] = argv[i];
        /* Split line on spaces */
        char* tok = strtok(line, " \t");
        while (tok && n < 250) { cmd_argv[n++] = tok; tok = strtok(NULL, " \t"); }
        cmd_argv[n] = NULL;
        if (n == 0) continue;
        pid_t pid = fork();
        if (pid == 0) { execvp(cmd_argv[0], cmd_argv); _exit(127); }
        else { int s; waitpid(pid, &s, 0); }
    }
    return 0;
}

int cmd_cut(int argc, char** argv) {
    char delimiter = '\t';
    int f1 = -1, f2 = -1;
    int ai = 1;
    while (ai < argc && argv[ai][0] == '-') {
        if (strcmp(argv[ai], "-d") == 0 && ai + 1 < argc) { delimiter = argv[++ai][0]; ai++; }
        else if (strncmp(argv[ai], "-f", 2) == 0) {
            char* p = argv[ai] + 2;
            if (*p == 0 && ai + 1 < argc) p = argv[++ai];
            f1 = atoi(p);
            char* comma = strchr(p, ',');
            if (comma) f2 = atoi(comma + 1);
            ai++;
        } else ai++;
    }
    if (f1 < 1) { fprintf(stderr, "  Usage: cut -d'\\t' -f1,2 [file]\n"); return 1; }

    FILE* fp = stdin;
    if (ai < argc) { fp = fopen(argv[ai], "r"); if (!fp) { fprintf(stderr, "  cut: %s: %s\n", argv[ai], strerror(errno)); return 1; } }

    char line[4096];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        int field = 1;
        char* p = line;
        int printing = 0;
        while (*p) {
            if (*p == delimiter) {
                field++;
                if (printing) printf("%c", delimiter);
                printing = 0;
            } else {
                if (field == f1 || (f2 > 0 && field >= f1 && field <= f2)) {
                    printf("%c", *p);
                    printing = 1;
                }
            }
            p++;
        }
        printf("\n");
    }
    if (fp != stdin) fclose(fp);
    return 0;
}

int cmd_uniq(int argc, char** argv) {
    int count = 0;
    int ai = 1;
    if (ai < argc && strcmp(argv[ai], "-c") == 0) { count = 1; ai++; }

    FILE* fp = stdin;
    if (ai < argc) { fp = fopen(argv[ai], "r"); if (!fp) { fprintf(stderr, "  uniq: %s: %s\n", argv[ai], strerror(errno)); return 1; } }

    char prev[4096] = "";
    int n = 0;
    char line[4096];
    while (fgets(line, sizeof(line), fp)) {
        if (strcmp(line, prev) == 0) {
            n++;
        } else {
            if (prev[0] && n > 0) {
                if (count) printf("%d %s", n, prev);
                else printf("%s", prev);
            }
            strcpy(prev, line);
            n = 1;
        }
    }
    if (prev[0]) {
        if (count) printf("%d %s", n, prev);
        else printf("%s", prev);
    }
    if (fp != stdin) fclose(fp);
    return 0;
}

int cmd_tr(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "  Usage: tr <from> <to>\n"); return 1; }
    const char* from = argv[1];
    const char* to = argv[2];
    char map[256];
    for (int i = 0; i < 256; i++) map[i] = i;
    for (int i = 0; from[i] && to[i]; i++) map[(unsigned char)from[i]] = to[i];

    char buf[4096];
    ssize_t n;
    while ((n = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        for (ssize_t i = 0; i < n; i++) buf[i] = map[(unsigned char)buf[i]];
        write(STDOUT_FILENO, buf, n);
    }
    return 0;
}

int cmd_ln(int argc, char** argv) {
    int sym = 0;
    int ai = 1;
    if (ai < argc && strcmp(argv[ai], "-s") == 0) { sym = 1; ai++; }
    if (ai + 1 >= argc) { fprintf(stderr, "  Usage: ln [-s] <target> <link>\n"); return 1; }
    int rc;
    if (sym) rc = symlink(argv[ai], argv[ai + 1]);
    else rc = link(argv[ai], argv[ai + 1]);
    if (rc < 0) { fprintf(stderr, "  ln: %s\n", strerror(errno)); return 1; }
    return 0;
}

int cmd_stat(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "  Usage: stat <file>\n"); return 1; }
    struct stat st;
    if (lstat(argv[1], &st) < 0) { fprintf(stderr, "  stat: %s: %s\n", argv[1], strerror(errno)); return 1; }
    printf("  File: %s\n", argv[1]);
    printf("  Size: %ld\t", (long)st.st_size);
    if (S_ISREG(st.st_mode)) printf("regular file\n");
    else if (S_ISDIR(st.st_mode)) printf("directory\n");
    else if (S_ISLNK(st.st_mode)) printf("symbolic link\n");
    else if (S_ISCHR(st.st_mode)) printf("character device\n");
    else if (S_ISBLK(st.st_mode)) printf("block device\n");
    else if (S_ISFIFO(st.st_mode)) printf("FIFO\n");
    else if (S_ISSOCK(st.st_mode)) printf("socket\n");
    printf("  Mode: %04o\n", st.st_mode & 07777);
    printf("  Uid:  %d\tGid: %d\n", st.st_uid, st.st_gid);
    return 0;
}

int cmd_time(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "  Usage: time <cmd> [args...]\n"); return 1; }
    struct timespec t1, t2;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    pid_t pid = fork();
    if (pid == 0) {
        execvp(argv[1], argv + 1);
        fprintf(stderr, "  time: %s: %s\n", argv[1], strerror(errno));
        _exit(127);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        clock_gettime(CLOCK_MONOTONIC, &t2);
        double elapsed = (t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec) / 1e9;
        printf("\nreal\t%.3fs\n", elapsed);
    }
    return 0;
}

int cmd_gcc(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "  Usage: gcc <file.c> [-o output]\n"); return 1; }
    /* Auto-append -lm if no -l flags */
    int has_l = 0;
    for (int i = 1; i < argc; i++) if (strncmp(argv[i], "-l", 2) == 0) has_l = 1;

    char* cmd_argv[128];
    int n = 0;
    cmd_argv[n++] = "gcc";
    cmd_argv[n++] = "-Wall";
    cmd_argv[n++] = "-O2";
    for (int i = 1; i < argc && n < 120; i++) cmd_argv[n++] = argv[i];
    if (!has_l) { cmd_argv[n++] = "-lm"; }
    cmd_argv[n] = NULL;

    struct timespec t1, t2;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    pid_t pid = fork();
    if (pid == 0) {
        execvp("gcc", cmd_argv);
        fprintf(stderr, "  gcc: %s\n", strerror(errno));
        _exit(127);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        clock_gettime(CLOCK_MONOTONIC, &t2);
        double elapsed = (t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec) / 1e9;
        if (WEXITSTATUS(status) == 0)
            printf("  " COLOR_GREEN "✓" COLOR_RESET " Compiled in %.3fs\n", elapsed);
        else
            printf("  " COLOR_RED "✗" COLOR_RESET " Compilation failed\n");
        return WEXITSTATUS(status);
    }
    return 1;
}

int cmd_curl(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "  Usage: curl <url> [-o file]\n"); return 1; }
    /* Check if output file is specified */
    int pipe_mode = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            /* Pass through to real curl */
            char* cmd_argv[128];
            int n = 0;
            cmd_argv[n++] = "curl";
            for (int j = 1; j < argc && n < 120; j++) cmd_argv[n++] = argv[j];
            cmd_argv[n] = NULL;
            pid_t pid = fork();
            if (pid == 0) { execvp("curl", cmd_argv); _exit(127); }
            else { int s; waitpid(pid, &s, 0); return WEXITSTATUS(s); }
        }
        if (strcmp(argv[i], "-s") == 0) pipe_mode = 1;
    }
    /* Default: curl and show output */
    char* cmd_argv[128];
    int n = 0;
    cmd_argv[n++] = "curl";
    cmd_argv[n++] = "-s";
    for (int i = 1; i < argc && n < 120; i++) cmd_argv[n++] = argv[i];
    cmd_argv[n] = NULL;

    int pipefd[2];
    pipe(pipefd);
    pid_t pid = fork();
    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        execvp("curl", cmd_argv);
        _exit(127);
    } else {
        close(pipefd[1]);
        char buf[4096];
        ssize_t r;
        while ((r = read(pipefd[0], buf, sizeof(buf))) > 0) write(STDOUT_FILENO, buf, r);
        close(pipefd[0]);
        int s; waitpid(pid, &s, 0);
        return WEXITSTATUS(s);
    }
}

int cmd_who(void) {
    FILE* fp = fopen("/var/run/utmp", "r");
    if (!fp) { fprintf(stderr, "  who: cannot open utmp\n"); return 1; }
    struct utmp entry;
    while (fread(&entry, sizeof(entry), 1, fp) == 1) {
        if (entry.ut_type == USER_PROCESS) {
            printf("  %-10s %s\n", entry.ut_user, entry.ut_line);
        }
    }
    fclose(fp);
    return 0;
}

int cmd_su(int argc, char** argv) {
    const char* user = argc > 1 ? argv[1] : "root";
    char prompt[256];
    snprintf(prompt, sizeof(prompt), "  Password for %s: ", user);
    printf("%s", prompt);
    fflush(stdout);

    /* Read password (no echo) */
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    char pass[256];
    fgets(pass, sizeof(pass), stdin);
    pass[strcspn(pass, "\n")] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    printf("\n");

    /* Try su */
    pid_t pid = fork();
    if (pid == 0) {
        char* cmd_argv[] = {"su", "-", user, NULL};
        setenv("TERM", "xterm", 1);
        execvp("su", cmd_argv);
        _exit(127);
    } else {
        int s; waitpid(pid, &s, 0);
        return WEXITSTATUS(s);
    }
}

int cmd_passwd(int argc, char** argv) {
    const char* user = argc > 1 ? argv[1] : getenv("USER");
    if (!user) user = "root";
    pid_t pid = fork();
    if (pid == 0) {
        char* cmd_argv[] = {"passwd", (char*)user, NULL};
        execvp("passwd", cmd_argv);
        _exit(127);
    } else {
        int s; waitpid(pid, &s, 0);
        return WEXITSTATUS(s);
    }
}

int cmd_kill_tak_full(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "  Usage: kill <pid> [-signal]\n"); return 1; }
    int sig = SIGTERM;
    int pid = atoi(argv[1]);
    if (pid <= 0) { fprintf(stderr, "  kill: invalid pid\n"); return 1; }
    if (argc > 2 && strcmp(argv[2], "-9") == 0) sig = SIGKILL;
    if (argc > 2 && strcmp(argv[2], "-TERM") == 0) sig = SIGTERM;
    if (kill(pid, sig) < 0) { fprintf(stderr, "  kill: %s\n", strerror(errno)); return 1; }
    printf("  Sent signal %d to pid %d\n", sig, pid);
    return 0;
}

/* ═══════════════════════════════════════════════════════
   SYSTEM MONITORING
   ═══════════════════════════════════════════════════════ */

int cmd_top(void) {
    int rows, cols;
    tui_get_size(&rows, &cols);
    tui_hide_cursor();
    tui_clear();

    int running = 1;
    while (running) {
        /* Header */
        tui_cursor(1, 1);
        printf(COLOR_BG_MAGENTA COLOR_BOLD COLOR_WHITE " TOP " COLOR_RESET);
        printf(COLOR_BG_BLUE COLOR_WHITE " Ternary Ancestral Kernel — Process Monitor " COLOR_RESET "                ");

        /* System info */
        struct sysinfo si;
        sysinfo(&si);
        unsigned long mem_used = (si.totalram - si.freeram) * si.mem_unit / 1024 / 1024;
        unsigned long mem_total = si.totalram * si.mem_unit / 1024 / 1024;
        tui_cursor(3, 1);
        printf(COLOR_BOLD " Tasks: " COLOR_GREEN "%ld" COLOR_RESET COLOR_BOLD "  Mem: %lu/%luMB  Load: %ld.%ld %ld.%ld %ld.%ld  Uptime: %ldh%ldm" COLOR_RESET,
               si.procs, mem_used, mem_total,
               si.loads[0]/65536, (si.loads[0]/6553)%10,
               si.loads[1]/65536, (si.loads[1]/6553)%10,
               si.loads[2]/65536, (si.loads[2]/6553)%10,
               si.uptime/3600, (si.uptime/60)%60);

        /* Table header */
        tui_cursor(5, 1);
        printf(COLOR_BOLD COLOR_CYAN "  PID   USER     %%CPU  %%MEM    RSS    COMMAND" COLOR_RESET);

        /* Read /proc for processes */
        DIR* d = opendir("/proc");
        if (d) {
            struct dirent* ent;
            int line = 6;
            int shown = 0;
            int max_lines = rows - 8;

            while ((ent = readdir(d)) && line < 6 + max_lines) {
                if (!isdigit(ent->d_name[0])) continue;
                int pid = atoi(ent->d_name);
                if (pid <= 0) continue;

                char path[256], line_buf[1024];
                snprintf(path, sizeof(path), "/proc/%d/stat", pid);
                FILE* f = fopen(path, "r");
                if (!f) continue;

                if (fgets(line_buf, sizeof(line_buf), f)) {
                    /* Parse stat: pid (comm) state ppid ... */
                    char* p = strrchr(line_buf, ')');
                    if (p) {
                        char state = ' ';
                        sscanf(p + 2, "%c", &state);

                        /* Get command name */
                        char* start = strchr(line_buf, '(');
                        char comm[64] = "";
                        if (start && p) {
                            int len = p - start - 1;
                            if (len > 63) len = 63;
                            strncpy(comm, start + 1, len);
                            comm[len] = 0;
                        }

                        /* Get RSS from /proc/pid/statm */
                        char statm_path[256], statm_buf[256];
                        snprintf(statm_path, sizeof(statm_path), "/proc/%d/statm", pid);
                        FILE* fm = fopen(statm_path, "r");
                        long rss = 0;
                        if (fm) {
                            long pages;
                            if (fscanf(fm, "%*ld %ld", &pages) == 1)
                                rss = pages * 4 / 1024; /* KB to MB approx */
                            fclose(fm);
                        }

                        /* Get username */
                        struct stat st;
                        char uid_path[256];
                        snprintf(uid_path, sizeof(uid_path), "/proc/%d", pid);
                        stat(uid_path, &st);
                        struct passwd* pw = getpwuid(st.st_uid);
                        char user[32];
                        snprintf(user, sizeof(user), "%s", pw ? pw->pw_name : "?");

                        /* State color */
                        const char* state_color = COLOR_GREEN;
                        if (state == 'R') state_color = COLOR_GREEN;
                        else if (state == 'S') state_color = COLOR_BLUE;
                        else if (state == 'Z') state_color = COLOR_RED;
                        else if (state == 'T') state_color = COLOR_YELLOW;

                        tui_cursor(line, 1);
                        if (shown == 0) /* highlight first */
                            printf(COLOR_BG_CYAN COLOR_BOLD " %5d  %-8s  ?   ?  %5ld  %s" COLOR_RESET, pid, user, rss, comm);
                        else
                            printf(" %5d  %-8s  ?   ?  %5ld  %s" COLOR_RESET, pid, user, rss, comm);
                        line++;
                        shown++;
                    }
                }
                fclose(f);
            }
            closedir(d);
        }

        /* Bottom bar */
        tui_cursor(rows - 1, 1);
        printf(COLOR_BG_BLUE COLOR_WHITE " Q: Quit  R: Refresh  P: Sort by PID  M: Sort by MEM " COLOR_RESET "        ");

        /* Non-blocking input */
        system("/bin/stty raw -echo 2>/dev/null");
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        struct timeval tv = {0, 100000}; /* 100ms refresh */
        int ready = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);

        if (ready > 0) {
            int ch = getchar();
            if (ch == 'q' || ch == 'Q') running = 0;
        }
        system("/bin/stty cooked echo 2>/dev/null");
        tui_clear();
    }
    tui_show_cursor();
    tui_clear();
    return 0;
}

/* ═══════════════════════════════════════════════════════
   ARCHIVE & COMPRESSION
   ═══════════════════════════════════════════════════════ */

int cmd_tar(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "  Usage: tar [-xvf] [-cvf] <archive> [files...]\n");
        return 1;
    }
    int extract = 0, create = 0, verbose = 0;
    char* archive = NULL;
    char* files[256];
    int nfiles = 0;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j]; j++) {
                if (argv[i][j] == 'x') extract = 1;
                if (argv[i][j] == 'c') create = 1;
                if (argv[i][j] == 'v') verbose = 1;
                if (argv[i][j] == 'f') { /* next arg is archive */ }
            }
        } else if (!archive) {
            archive = argv[i];
        } else {
            files[nfiles++] = argv[i];
        }
    }
    if (!archive) { fprintf(stderr, "  tar: no archive specified\n"); return 1; }

    /* Use system tar */
    char cmd[4096];
    if (create) {
        snprintf(cmd, sizeof(cmd), "tar -cf %s", archive);
        for (int i = 0; i < nfiles; i++) { strcat(cmd, " "); strcat(cmd, files[i]); }
    } else if (extract) {
        snprintf(cmd, sizeof(cmd), "tar -xf %s", archive);
    } else {
        snprintf(cmd, sizeof(cmd), "tar -tf %s", archive);
    }
    system(cmd);
    return 0;
}

int cmd_gzip(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "  Usage: gzip <file>\n"); return 1; }
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "gzip %s", argv[1]);
    system(cmd);
    return 0;
}

int cmd_gunzip(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "  Usage: gunzip <file.gz>\n"); return 1; }
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "gunzip %s", argv[1]);
    system(cmd);
    return 0;
}

/* ═══════════════════════════════════════════════════════
   NETWORK TOOLS
   ═══════════════════════════════════════════════════════ */

int cmd_ping(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "  Usage: ping <host> [-c count]\n"); return 1; }
    int count = 4;
    const char* host = argv[1];
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) count = atoi(argv[++i]);
    }
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "ping -c %d %s", count, host);
    system(cmd);
    return 0;
}

int cmd_traceroute(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "  Usage: traceroute <host>\n"); return 1; }
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "traceroute %s", argv[1]);
    system(cmd);
    return 0;
}

int cmd_nslookup(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "  Usage: nslookup <host>\n"); return 1; }
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "nslookup %s", argv[1]);
    system(cmd);
    return 0;
}

int cmd_ifconfig(void) {
    system("ip addr show 2>/dev/null || ifconfig 2>/dev/null || echo '  No network tools'");
    return 0;
}

int cmd_netstat(void) {
    system("ss -tuln 2>/dev/null || netstat -tuln 2>/dev/null || echo '  No netstat'");
    return 0;
}

/* ═══════════════════════════════════════════════════════
   USER MANAGEMENT
   ═══════════════════════════════════════════════════════ */

int cmd_chown(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "  Usage: chown <user> <file>\n"); return 1; }
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "chown %s %s", argv[1], argv[2]);
    system(cmd);
    return 0;
}

int cmd_adduser(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "  Usage: adduser <username>\n"); return 1; }
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "sudo adduser %s", argv[1]);
    system(cmd);
    return 0;
}

int cmd_groups(int argc, char** argv) {
    const char* user = argc > 1 ? argv[1] : getenv("USER");
    if (!user) user = "root";
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "groups %s", user);
    system(cmd);
    return 0;
}

/* ═══════════════════════════════════════════════════════
   SYSTEM TOOLS
   ═══════════════════════════════════════════════════════ */

int cmd_dmesg(void) {
    system("dmesg 2>/dev/null | tail -20");
    return 0;
}

int cmd_mount(void) {
    system("mount 2>/dev/null | grep -E '^/' | head -20");
    return 0;
}

int cmd_systemctl(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "  Usage: systemctl <command> [service]\n"); return 1; }
    char cmd[512] = "sudo systemctl";
    for (int i = 1; i < argc; i++) { strcat(cmd, " "); strcat(cmd, argv[i]); }
    system(cmd);
    return 0;
}

int cmd_journalctl(int argc, char** argv) {
    char cmd[512] = "journalctl";
    for (int i = 1; i < argc; i++) { strcat(cmd, " "); strcat(cmd, argv[i]); }
    if (argc < 2) strcat(cmd, " -n 20");
    system(cmd);
    return 0;
}

int cmd_lscpu(void) {
    system("lscpu 2>/dev/null || cat /proc/cpuinfo | head -20");
    return 0;
}

int cmd_lsusb(void) {
    system("lsusb 2>/dev/null || echo '  lsusb not available'");
    return 0;
}

int cmd_locate(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "  Usage: locate <pattern>\n"); return 1; }
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "locate %s 2>/dev/null || find / -name '*%s*' 2>/dev/null | head -20", argv[1], argv[1]);
    system(cmd);
    return 0;
}

/* ═══════════════════════════════════════════════════════
   NANO TEXT EDITOR (simple TUI)
   ═══════════════════════════════════════════════════════ */

int cmd_nano(int argc, char** argv) {
    if (argc < 2) {
        /* Run system nano if available */
        system("nano 2>/dev/null || echo '  nano not installed, using vi'");
        system("vi 2>/dev/null || echo '  No editor available'");
        return 0;
    }
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "nano %s", argv[1]);
    system(cmd);
    return 0;
}

/* TUI DESKTOP */
void tui_get_size(int* rows, int* cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        *rows = ws.ws_row;
        *cols = ws.ws_col;
    } else {
        *rows = 24;
        *cols = 80;
    }
}

void tui_cursor(int r, int c) { printf("\033[%d;%dH", r, c); }
void tui_clear(void) { printf("\033[2J\033[H"); }
void tui_hide_cursor(void) { printf("\033[?25l"); }
void tui_show_cursor(void) { printf("\033[?25h"); }

void tui_hline(int r, int c1, int c2, const char* color) {
    tui_cursor(r, c1);
    printf("%s", color);
    for (int i = c1; i <= c2; i++) printf("─");
    printf(COLOR_RESET);
}

void tui_box(int r, int c, int w, int h, const char* title, const char* color) {
    /* Top */
    tui_cursor(r, c);
    printf("%s┌", color);
    if (title) { printf("─ %s ", title); int tlen = strlen(title) + 4; for (int i = tlen; i < w - 1; i++) printf("─"); }
    else { for (int i = 1; i < w - 1; i++) printf("─"); }
    printf("┐" COLOR_RESET);
    /* Sides */
    for (int i = 1; i < h - 1; i++) {
        tui_cursor(r + i, c);
        printf("%s│", color);
        for (int j = 1; j < w - 1; j++) printf(" ");
        printf("│" COLOR_RESET);
    }
    /* Bottom */
    tui_cursor(r + h - 1, c);
    printf("%s└", color);
    for (int i = 1; i < w - 1; i++) printf("─");
    printf("┘" COLOR_RESET);
}

/* Panel: top bar (clock, memory, disk, Maya tick) */
void tui_panel(int cols) {
    tui_cursor(1, 1);
    printf(COLOR_BG_MAGENTA COLOR_BOLD COLOR_WHITE " TAK " COLOR_RESET);
    printf(COLOR_BG_BLUE COLOR_WHITE);

    /* Maya calendar */
    maya_calendar_t* cal = sched_get_calendar();
    char buf[256];
    int pos = 7;
    snprintf(buf, sizeof(buf), " Tzol:%u ", cal->tzolkin_day);
    tui_cursor(1, pos); printf("%s", buf); pos += strlen(buf);

    snprintf(buf, sizeof(buf), " Haab:%u ", cal->haab_day);
    tui_cursor(1, pos); printf("%s", buf); pos += strlen(buf);

    snprintf(buf, sizeof(buf), " Tick:%lu ", (unsigned long)cal->global_tick);
    tui_cursor(1, pos); printf("%s", buf); pos += strlen(buf);

    /* System info */
    struct statfs sf;
    struct sysinfo si;
    sysinfo(&si);
    statfs("/", &sf);

    unsigned long mem_used = (si.totalram - si.freeram) * si.mem_unit / 1024 / 1024;
    unsigned long mem_total = si.totalram * si.mem_unit / 1024 / 1024;
    unsigned long disk_used = (sf.f_blocks - sf.f_bfree) * sf.f_bsize / 1024 / 1024;
    unsigned long disk_total = sf.f_blocks * sf.f_bsize / 1024 / 1024;

    snprintf(buf, sizeof(buf), " MEM:%lu/%luMB ", mem_used, mem_total);
    tui_cursor(1, pos); printf("%s", buf); pos += strlen(buf);

    snprintf(buf, sizeof(buf), " DISK:%lu/%luGB ", disk_used / 1024, disk_total / 1024);
    tui_cursor(1, pos); printf("%s", buf); pos += strlen(buf);

    /* Time right-aligned */
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char timebuf[32];
    strftime(timebuf, sizeof(timebuf), "%H:%M", t);
    int tlen = strlen(timebuf) + 4;
    if (pos + tlen < cols) {
        tui_cursor(1, cols - tlen - 1);
        printf(" %s ", timebuf);
    }

    printf(COLOR_RESET);
}

/* App menu (like Mint menu) */
typedef struct {
    const char* name;
    const char* cmd;
    const char* desc;
    const char* icon; /* unicode/ascii art */
} app_entry_t;

static app_entry_t apps[] = {
    {"Terminal",   "echo",       "Open a shell",           "$"},
    {"File Manager", "ls",       "Browse files",           "📁"},
    {"Text Editor", "cat",       "View/edit files",        "📝"},
    {"Calculator", "bc",         "Calculate expressions",  "="},
    {"System Info","neofetch",   "System information",     "ℹ"},
    {"Processes",  "ps",         "Running processes",      "⚙"},
    {"Network",    "curl",       "HTTP requests",          "🌐"},
    {"Calendar",   "cal",        "Maya calendar",          "📅"},
    {"Compile",    "gcc",        "Compile C code",         "🔨"},
    {"Sensors",    "cat sensors.txt","Ternary sensor data","📡"},
    {"Settings",   "echo",       "System settings",        "🔧"},
    {"About",      "echo",       "About TAK OS",           "ℹ"},
    {NULL, NULL, NULL, NULL}
};

int cmd_menu(void) {
    int rows, cols;
    tui_get_size(&rows, &cols);
    tui_hide_cursor();
    tui_clear();

    int menu_w = 36;
    int menu_h = 18;
    int menu_r = (rows - menu_h) / 2;
    int menu_c = (cols - menu_w) / 2;

    /* Panel */
    tui_panel(cols);

    /* Title */
    tui_cursor(menu_r, menu_c + (menu_w - 16) / 2);
    printf(COLOR_BOLD COLOR_CYAN "  TAK  Menu" COLOR_RESET);

    /* Menu box */
    tui_box(menu_r + 1, menu_c, menu_w, menu_h, "Applications", COLOR_CYAN);

    /* Items */
    int sel = 0;
    int running = 1;
    while (running) {
        for (int i = 0; apps[i].name; i++) {
            tui_cursor(menu_r + 3 + i, menu_c + 2);
            if (i == sel) printf(COLOR_BG_CYAN COLOR_BOLD COLOR_WHITE " %-32s " COLOR_RESET, apps[i].name);
            else printf("  %s  %-28s " COLOR_RESET, apps[i].icon, apps[i].name);
        }

        tui_cursor(rows, 1);
        printf(COLOR_BG_BLUE COLOR_WHITE " ↑↓ Select  Enter: Run  Q/Esc: Exit " COLOR_RESET "                    ");

        /* Wait for input */
        system("/bin/stty raw -echo 2>/dev/null");
        int ch = getchar();
        system("/bin/stty cooked echo 2>/dev/null");

        if (ch == 'q' || ch == 'Q' || ch == 27) running = 0;
        else if (ch == 127 || ch == 8) { /* backspace = up */ if (sel > 0) sel--; }
        else if (ch == 'A' || ch == 'k') { if (sel > 0) sel--; }
        else if (ch == 'B' || ch == 'j') { if (apps[sel + 1].name) sel++; }
        else if (ch == '\n' || ch == '\r') {
            /* Run command */
            tui_show_cursor();
            tui_cursor(rows - 2, 1);
            printf(COLOR_RESET "  Running: %s...                          \n", apps[sel].cmd);
            /* Return to shell to run */
            printf("\033[0m");
            system("/bin/stty cooked echo 2>/dev/null");
            return 0;
        }
    }
    tui_show_cursor();
    tui_clear();
    return 0;
}

/* File browser TUI */
int cmd_browse(void) {
    int rows, cols;
    tui_get_size(&rows, &cols);
    tui_hide_cursor();
    tui_clear();

    char cwd[4096];
    getcwd(cwd, sizeof(cwd));

    int panel_w = cols - 4;
    int list_h = rows - 6;
    int sel = 0;
    int scroll = 0;
    int running = 1;

    while (running) {
        /* Panel */
        tui_panel(cols);

        /* Path bar */
        tui_cursor(3, 1);
        printf(COLOR_BG_BLACK COLOR_CYAN " 📁 %-*s " COLOR_RESET, cols - 2, cwd);

        /* List files */
        DIR* d = opendir(cwd);
        if (!d) { tui_cursor(5, 3); printf("Cannot open directory"); break; }

        struct dirent* ent;
        char entries[1024][256];
        int n = 0;

        /* Add .. */
        snprintf(entries[0], 256, "..");
        n = 1;

        while ((ent = readdir(d)) && n < 1023) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
            snprintf(entries[n], 256, "%s%s", ent->d_name, ent->d_type == DT_DIR ? "/" : "");
            n++;
        }
        closedir(d);

        /* Sort */
        for (int i = 0; i < n - 1; i++)
            for (int j = i + 1; j < n; j++)
                if (strcmp(entries[i], entries[j]) > 0) {
                    char tmp[256]; strcpy(tmp, entries[i]);
                    strcpy(entries[i], entries[j]); strcpy(entries[j], tmp);
                }

        /* Display */
        if (sel < scroll) scroll = sel;
        if (sel >= scroll + list_h) scroll = sel - list_h + 1;

        for (int i = 0; i < list_h && scroll + i < n; i++) {
            int idx = scroll + i;
            tui_cursor(5 + i, 3);
            int is_dir = entries[idx][strlen(entries[idx]) - 1] == '/';
            if (idx == sel) {
                if (is_dir) printf(COLOR_BG_CYAN COLOR_BOLD " 📁 %-40s " COLOR_RESET, entries[idx]);
                else printf(COLOR_BG_CYAN COLOR_BOLD " 📄 %-40s " COLOR_RESET, entries[idx]);
            } else {
                if (is_dir) printf(COLOR_CYAN " 📁 %-40s " COLOR_RESET, entries[idx]);
                else printf(" 📄 %-40s ", entries[idx]);
            }
        }

        /* Bottom bar */
        tui_cursor(rows - 1, 1);
        printf(COLOR_BG_BLUE COLOR_WHITE " Enter: Open  Backspace: Up  Q: Exit " COLOR_RESET "                              ");

        /* Input */
        system("/bin/stty raw -echo 2>/dev/null");
        int ch = getchar();
        system("/bin/stty cooked echo 2>/dev/null");

        if (ch == 'q' || ch == 'Q') running = 0;
        else if (ch == 127 || ch == 8) {
            /* Go up */
            char* slash = strrchr(cwd, '/');
            if (slash && slash != cwd) { *slash = 0; sel = 0; scroll = 0; }
        }
        else if (ch == 'A' || ch == 'k') { if (sel > 0) sel--; }
        else if (ch == 'B' || ch == 'j') { if (sel < n - 1) sel++; }
        else if (ch == '\n' || ch == '\r') {
            /* Enter directory or open file */
            char* name = entries[sel];
            int is_dir = name[strlen(name) - 1] == '/';
            if (is_dir) {
                if (strcmp(name, "..") == 0) {
                    char* slash = strrchr(cwd, '/');
                    if (slash && slash != cwd) *slash = 0;
                } else {
                    snprintf(cwd + strlen(cwd), sizeof(cwd) - strlen(cwd), "/%s", name);
                    /* Remove trailing / */
                    cwd[strlen(cwd) - 1] = 0;
                }
                sel = 0; scroll = 0;
            } else {
                /* Open file in less */
                tui_show_cursor();
                tui_clear();
                char cmd[4096];
                snprintf(cmd, sizeof(cmd), "less %s/%s", cwd, name);
                system("/bin/stty cooked echo 2>/dev/null");
                system(cmd);
                tui_hide_cursor();
                tui_clear();
            }
        }
    }
    tui_show_cursor();
    tui_clear();
    return 0;
}

/* Full desktop environment */
int cmd_desktop(void) {
    int rows, cols;
    tui_get_size(&rows, &cols);
    tui_hide_cursor();
    tui_clear();

    int running = 1;
    int sel = 0;
    const char* panel_apps[] = {"Menu", "Files", "Terminal", "SysInfo", "Sensors", "Exit"};
    int n_apps = 6;

    while (running) {
        /* Top panel */
        tui_panel(cols);

        /* Taskbar */
        tui_cursor(3, 1);
        printf(COLOR_BG_BLACK);
        for (int i = 0; i < n_apps; i++) {
            if (i == sel) printf(COLOR_BG_CYAN COLOR_BOLD " %s " COLOR_RESET COLOR_BG_BLACK, panel_apps[i]);
            else printf(" %s ", panel_apps[i]);
        }
        printf(COLOR_RESET);

        /* Desktop area */
        tui_cursor(6, cols / 2 - 10);
        printf(COLOR_BOLD COLOR_CYAN "┌──────────────────────────┐");
        tui_cursor(7, cols / 2 - 10);
        printf(COLOR_CYAN "│" COLOR_RESET COLOR_BOLD "      TAK Desktop         " COLOR_CYAN "│");
        tui_cursor(8, cols / 2 - 10);
        printf(COLOR_CYAN "│" COLOR_RESET "   Ternary Ancestral OS   " COLOR_CYAN "│");
        tui_cursor(9, cols / 2 - 10);
        printf(COLOR_CYAN "│" COLOR_RESET "   58 commands available   " COLOR_CYAN "│");
        tui_cursor(10, cols / 2 - 10);
        printf(COLOR_CYAN "│" COLOR_RESET "   Maya · Persia · Inca   " COLOR_CYAN "│");
        tui_cursor(11, cols / 2 - 10);
        printf("└──────────────────────────┘" COLOR_RESET);

        /* Status */
        maya_calendar_t* cal = sched_get_calendar();
        struct sysinfo si;
        sysinfo(&si);
        unsigned long mem_used = (si.totalram - si.freeram) * si.mem_unit / 1024 / 1024;
        unsigned long mem_total = si.totalram * si.mem_unit / 1024 / 1024;

        tui_cursor(rows - 3, 3);
        printf(COLOR_GRAY " Memory: %lu/%luMB  Load: %ld.%ld  Uptime: %ldh%ldm" COLOR_RESET,
               mem_used, mem_total, si.loads[0] / 65536, (si.loads[0] / 6553) % 10,
               si.uptime / 3600, (si.uptime / 60) % 60);

        tui_cursor(rows - 2, 3);
        printf(COLOR_GRAY " Maya Tick: %lu  Tzolkin: %u/260  Haab: %u/365" COLOR_RESET,
               (unsigned long)cal->global_tick, cal->tzolkin_day, cal->haab_day);

        /* Bottom bar */
        tui_cursor(rows, 1);
        printf(COLOR_BG_BLUE COLOR_WHITE " ←→ Select  Enter: Run  Q: Exit " COLOR_RESET "                                   ");

        /* Input */
        system("/bin/stty raw -echo 2>/dev/null");
        int ch = getchar();
        system("/bin/stty cooked echo 2>/dev/null");

        if (ch == 'q' || ch == 'Q') running = 0;
        else if (ch == 'A' || ch == 'k') { if (sel > 0) sel--; }
        else if (ch == 'B' || ch == 'j') { if (sel < n_apps - 1) sel++; }
        else if (ch == 'C' || ch == 'l') { if (sel < n_apps - 1) sel++; }
        else if (ch == 'D' || ch == 'h') { if (sel > 0) sel--; }
        else if (ch == '\n' || ch == '\r') {
            tui_show_cursor();
            system("/bin/stty cooked echo 2>/dev/null");
            tui_clear();

            if (sel == 0) cmd_menu();
            else if (sel == 1) cmd_browse();
            else if (sel == 2) {
                printf("\n  Type 'exit' to return to desktop\n\n");
                char line[256];
                while (1) {
                    printf(COLOR_CYAN "tak>" COLOR_RESET " ");
                    if (!fgets(line, sizeof(line), stdin)) break;
                    line[strcspn(line, "\n")] = 0;
                    if (strcmp(line, "exit") == 0) break;
                    char cmd[280];
                    snprintf(cmd, sizeof(cmd), "%s", line);
                    system(cmd);
                }
            }
            else if (sel == 3) { system("neofetch 2>/dev/null || echo '  TAK OS v1.0'"); printf("\n  Press Enter..."); getchar(); }
            else if (sel == 4) { printf("\n  Ternary sensor data would appear here\n  Press Enter..."); getchar(); }
            else if (sel == 5) running = 0;

            tui_hide_cursor();
            tui_clear();
        }
    }
    tui_show_cursor();
    tui_clear();
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
    int interpret_escapes = 0;
    int start = 1;
    if (argc > 1 && strcmp(argv[1], "-e") == 0) { interpret_escapes = 1; start = 2; }
    for (int i = start; i < argc; i++) {
        if (i > start) write(STDOUT_FILENO, " ", 1);
        if (interpret_escapes) {
            for (int j = 0; argv[i][j]; j++) {
                if (argv[i][j] == '\\' && argv[i][j+1]) {
                    j++;
                    if (argv[i][j] == 'n') write(STDOUT_FILENO, "\n", 1);
                    else if (argv[i][j] == 't') write(STDOUT_FILENO, "\t", 1);
                    else if (argv[i][j] == '\\') write(STDOUT_FILENO, "\\", 1);
                    else { write(STDOUT_FILENO, "\\", 1); write(STDOUT_FILENO, &argv[i][j], 1); }
                } else {
                    write(STDOUT_FILENO, &argv[i][j], 1);
                }
            }
        } else {
            write(STDOUT_FILENO, argv[i], strlen(argv[i]));
        }
    }
    write(STDOUT_FILENO, "\n", 1);
}

// =============================================================================
// SEGMENT RUNNER — handles pipes, redirection, background for one segment
// =============================================================================

int run_segment(char* line) {
    while (*line == ' ') line++;
    if (*line == 0) return 0;

    /* Expand $VAR before pipe splitting (respects quotes) */
    {
        char exp[4096];
        int ei = 0;
        char q = 0;
        for (int i = 0; line[i] && ei < 4095; i++) {
            if (q) {
                if (line[i] == q) { q = 0; exp[ei++] = line[i]; continue; }
            } else {
                if (line[i] == '\'' || line[i] == '"') { q = line[i]; exp[ei++] = line[i]; continue; }
                if (line[i] == '$' && line[i+1] && line[i+1] != ' ' && line[i+1] != '=' && line[i+1] != '|') {
                    i++;
                    char varname[256];
                    int vi = 0;
                    while (line[i] && line[i] != ' ' && line[i] != '"' && line[i] != '\'' &&
                           line[i] != '|' && line[i] != ';' && line[i] != '&' && line[i] != '$' && vi < 255) {
                        varname[vi++] = line[i++];
                    }
                    varname[vi] = 0;
                    i--; /* loop will increment */
                    const char* val = getenv(varname);
                    if (val) {
                        for (int k = 0; val[k] && ei < 4095; k++) exp[ei++] = val[k];
                    }
                    continue;
                }
            }
            exp[ei++] = line[i];
        }
        exp[ei] = 0;
        strcpy(line, exp);
    }

    /* Check for pipes (respecting quotes) */
    char* pipe_cmds[16];
    int n_pipes = 0;

    {
        char* p = line;
        char quote = 0;
        pipe_cmds[0] = p;
        n_pipes = 1;
        while (*p) {
            if (quote) {
                if (*p == quote) quote = 0;
            } else {
                if (*p == '\'' || *p == '"') quote = *p;
                else if (*p == '|') {
                    *p = 0;
                    p++;
                    while (*p == ' ') p++;
                    pipe_cmds[n_pipes++] = p;
                    if (n_pipes >= 16) break;
                    continue;
                }
            }
            p++;
        }
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

    /* Tokenize with quote support */
    char* argv[64];
    int argc = 0;

    char* p = line;
    while (*p && argc < 64) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;

        char quote = 0;
        if (*p == '\'' || *p == '"') { quote = *p; p++; }

        argv[argc++] = p;
        while (*p) {
            if (quote) {
                if (*p == quote) { quote = 0; *p = ' '; p++; break; }
            } else {
                if (*p == ' ' || *p == '\t') break;
                if (*p == '\'' || *p == '"') { quote = *p; *p = ' '; p++; continue; }
            }
            p++;
        }
        if (*p) { *p = 0; p++; }
    }
    argv[argc] = NULL;

    if (argc == 0) return 0;

    /* Expand $VAR in arguments (not in single quotes) */
    for (int i = 0; i < argc; i++) {
        char* arg = argv[i];
        if (!strchr(arg, '$')) continue;
        char exp[4096];
        int ei = 0;
        for (int j = 0; arg[j] && ei < 4095; j++) {
            if (arg[j] == '$' && arg[j+1] && arg[j+1] != ' ') {
                j++;
                char varname[256];
                int vi = 0;
                while (arg[j] && arg[j] != ' ' && arg[j] != '"' && arg[j] != '\'' && vi < 255) {
                    varname[vi++] = arg[j++];
                }
                varname[vi] = 0;
                j--; /* loop will increment */
                const char* val = getenv(varname);
                if (val) {
                    for (int k = 0; val[k] && ei < 4095; k++) exp[ei++] = val[k];
                }
            } else {
                exp[ei++] = arg[j];
            }
        }
        exp[ei] = 0;
        strcpy(arg, exp);
    }

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
        else if (strcmp(argv[0], "grep") == 0) { builtin_rc = cmd_grep(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "sort") == 0) { builtin_rc = cmd_sort(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "wc") == 0) { builtin_rc = cmd_wc(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "head") == 0) { builtin_rc = cmd_head(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "tail") == 0) { builtin_rc = cmd_tail(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "find") == 0) { builtin_rc = cmd_find(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "chmod") == 0) { builtin_rc = cmd_chmod(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "du") == 0) { builtin_rc = cmd_du(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "df") == 0) { builtin_rc = cmd_df(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "env") == 0) { builtin_rc = cmd_env(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "tee") == 0) { builtin_rc = cmd_tee(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "date") == 0) { builtin_rc = cmd_date(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "sleep") == 0) { builtin_rc = cmd_sleep(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "which") == 0) { builtin_rc = cmd_which(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "diff") == 0) { builtin_rc = cmd_diff(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "xargs") == 0) { builtin_rc = cmd_xargs(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "cut") == 0) { builtin_rc = cmd_cut(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "uniq") == 0) { builtin_rc = cmd_uniq(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "tr") == 0) { builtin_rc = cmd_tr(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "ln") == 0) { builtin_rc = cmd_ln(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "stat") == 0) { builtin_rc = cmd_stat(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "time") == 0) { builtin_rc = cmd_time(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "gcc") == 0) { builtin_rc = cmd_gcc(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "cc") == 0) { builtin_rc = cmd_gcc(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "curl") == 0) { builtin_rc = cmd_curl(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "wget") == 0) { builtin_rc = cmd_curl(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "who") == 0) { builtin_rc = cmd_who(); is_builtin = 1; }
        else if (strcmp(argv[0], "su") == 0) { builtin_rc = cmd_su(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "passwd") == 0) { builtin_rc = cmd_passwd(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "top") == 0) { builtin_rc = cmd_top(); is_builtin = 1; }
        else if (strcmp(argv[0], "tar") == 0) { builtin_rc = cmd_tar(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "gzip") == 0) { builtin_rc = cmd_gzip(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "gunzip") == 0) { builtin_rc = cmd_gunzip(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "ping") == 0) { builtin_rc = cmd_ping(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "traceroute") == 0) { builtin_rc = cmd_traceroute(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "nslookup") == 0) { builtin_rc = cmd_nslookup(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "ifconfig") == 0 || strcmp(argv[0], "ip") == 0) { builtin_rc = cmd_ifconfig(); is_builtin = 1; }
        else if (strcmp(argv[0], "netstat") == 0) { builtin_rc = cmd_netstat(); is_builtin = 1; }
        else if (strcmp(argv[0], "chown") == 0) { builtin_rc = cmd_chown(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "adduser") == 0) { builtin_rc = cmd_adduser(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "groups") == 0) { builtin_rc = cmd_groups(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "dmesg") == 0) { builtin_rc = cmd_dmesg(); is_builtin = 1; }
        else if (strcmp(argv[0], "mount") == 0) { builtin_rc = cmd_mount(); is_builtin = 1; }
        else if (strcmp(argv[0], "systemctl") == 0) { builtin_rc = cmd_systemctl(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "journalctl") == 0) { builtin_rc = cmd_journalctl(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "lscpu") == 0) { builtin_rc = cmd_lscpu(); is_builtin = 1; }
        else if (strcmp(argv[0], "lsusb") == 0) { builtin_rc = cmd_lsusb(); is_builtin = 1; }
        else if (strcmp(argv[0], "locate") == 0) { builtin_rc = cmd_locate(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "nano") == 0) { builtin_rc = cmd_nano(argc, argv); is_builtin = 1; }
        else if (strcmp(argv[0], "desktop") == 0) { builtin_rc = cmd_desktop(); is_builtin = 1; }
        else if (strcmp(argv[0], "menu") == 0) { builtin_rc = cmd_menu(); is_builtin = 1; }
        else if (strcmp(argv[0], "browse") == 0) { builtin_rc = cmd_browse(); is_builtin = 1; }
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
    char quote = 0;
    for (int i = 0; line[i]; i++) {
        if (quote) {
            if (line[i] == quote) quote = 0;
            continue;
        }
        if (line[i] == '\'' || line[i] == '"') { quote = line[i]; continue; }
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

    /* Alias/Unalias/Export handled first (before any splitting, in parent) */
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
    /* Export: set env var, then continue processing rest of line */
    if (strncmp(line, "export ", 7) == 0) {
        char* arg = line + 7;
        /* Handle multiple exports: export A=1 B=2 */
        while (*arg) {
            while (*arg == ' ') arg++;
            char* eq = strchr(arg, '=');
            if (!eq) break;
            char* val_start = eq + 1;
            char* val_end = val_start;
            if (*val_end == '"' || *val_end == '\'') {
                char q = *val_end;
                val_end++;
                while (*val_end && *val_end != q) val_end++;
                if (*val_end == q) val_end++;
            } else {
                while (*val_end && *val_end != ' ' && *val_end != ';' &&
                       *val_end != '&' && *val_end != '|') val_end++;
            }
            *eq = 0;
            char save = *val_end;
            *val_end = 0;
            setenv(arg, val_start, 1);
            *val_end = save;
            arg = val_end;
        }
        /* Find next command after export */
        while (*arg == ' ') arg++;
        if (*arg == ';') { arg++; while (*arg == ' ') arg++; }
        if (*arg == 0) return 0;
        return parse_and_run(arg);
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
    if (argc < 3 || (strcmp(argv[1], "-c") != 0 && strcmp(argv[1], "-f") != 0))
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
