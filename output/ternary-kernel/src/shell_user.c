/**
 * shell_user.c — Shell de usuario (Ring 3)
 *
 * Shell que corre en modo usuario usando syscalls
 */

#include "../include/ternary.h"

// Maximum command length
#define MAX_CMD_LEN 256
#define MAX_ARGS 16

// Command history
static char cmd_history[16][MAX_CMD_LEN];
static uint8_t hist_count = 0;
static uint8_t hist_pos = 0;

// Environment
static char cwd[128] = "/";
static uint8_t uid = 0;

// Print prompt
static void print_prompt(void) {
    syscall_write(1, "\n", 1);
    syscall_write(1, "tritos", 6);
    syscall_write(1, "@", 1);
    syscall_write(1, cwd, strlen(cwd));
    syscall_write(1, "$ ", 2);
}

// Read line from user
static uint32_t read_line(char* buf, uint32_t max) {
    uint32_t i = 0;
    char c;
    
    while (i < max - 1) {
        syscall_read(0, &c, 1);
        
        if (c == '\n') {
            buf[i] = 0;
            syscall_write(1, "\n", 1);
            return i;
        } else if (c == '\b') {
            if (i > 0) {
                i--;
                syscall_write(1, "\b \b", 3);
            }
        } else {
            buf[i++] = c;
            syscall_write(1, &c, 1);
        }
    }
    
    buf[i] = 0;
    return i;
}

// Parse command into args
static uint32_t parse_args(char* cmd, char* args[]) {
    uint32_t argc = 0;
    char* p = cmd;
    
    while (*p && argc < MAX_ARGS) {
        // Skip whitespace
        while (*p == ' ') p++;
        if (!*p) break;
        
        args[argc++] = p;
        
        // Find end of arg
        while (*p && *p != ' ') p++;
        if (*p) *p++ = 0;
    }
    
    return argc;
}

// Built-in: help
static void builtin_help(void) {
    const char* msg =
        "\nTritos User Shell v1.0\n\n"
        "Built-in commands:\n"
        "  help     - Show this help\n"
        "  cd       - Change directory\n"
        "  pwd      - Print working directory\n"
        "  ls       - List files\n"
        "  cat      - Show file contents\n"
        "  echo     - Print text\n"
        "  clear    - Clear screen\n"
        "  whoami   - Show current user\n"
        "  id       - Show user ID\n"
        "  date     - Show date\n"
        "  uname    - Show system info\n"
        "  uptime   - Show uptime\n"
        "  ps       - Show processes\n"
        "  top      - Show process stats\n"
        "  free     - Show memory usage\n"
        "  df       - Show disk usage\n"
        "  ping     - Ping host\n"
        "  wget     - Download file\n"
        "  curl     - HTTP request\n"
        "  exit     - Exit shell\n\n"
        "Use syscalls for I/O.\n";
    syscall_write(1, msg, strlen(msg));
}

// Built-in: cd
static void builtin_cd(const char* path) {
    if (!path || !*path) {
        strcpy_t(cwd, "/");
    } else {
        strcpy_t(cwd, path);
    }
}

// Built-in: ls
static void builtin_ls(void) {
    // Use filesystem syscall
    int8_t fd = syscall_open("/", 0);
    if (fd < 0) {
        syscall_write(1, "Cannot open root\n", 17);
        return;
    }
    
    // List files
    // For now, just show a message
    syscall_write(1, "[ls] File listing via syscall\n", 29);
    syscall_close(fd);
}

// Built-in: cat
static void builtin_cat(const char* filename) {
    if (!filename) {
        syscall_write(1, "Usage: cat <file>\n", 18);
        return;
    }
    
    int8_t fd = syscall_open(filename, 0);
    if (fd < 0) {
        syscall_write(1, "Cannot open file\n", 17);
        return;
    }
    
    char buf[512];
    int32_t n;
    while ((n = syscall_read(fd, buf, sizeof(buf))) > 0) {
        syscall_write(1, buf, n);
    }
    
    syscall_close(fd);
}

// Built-in: echo
static void builtin_echo(uint32_t argc, char* args[]) {
    for (uint32_t i = 1; i < argc; i++) {
        if (i > 1) syscall_write(1, " ", 1);
        syscall_write(1, args[i], strlen(args[i]));
    }
    syscall_write(1, "\n", 1);
}

// Built-in: clear
static void builtin_clear(void) {
    // Use VGA clear
    syscall_write(1, "\033[2J\033[H", 8);  // ANSI escape
}

// Built-in: whoami
static void builtin_whoami(void) {
    syscall_write(1, "root\n", 5);
}

// Built-in: id
static void builtin_id(void) {
    syscall_write(1, "uid=0(root)\n", 12);
}

// Built-in: date
static void builtin_date(void) {
    syscall_write(1, "2026-09-17 (Ternary Calendar)\n", 30);
}

// Built-in: uname
static void builtin_uname(void) {
    syscall_write(1, "Tritos OS 1.0 (ternary-ancestral) i686\n", 40);
}

// Built-in: uptime
static void builtin_uptime(void) {
    uint32_t ticks = syscall_time();
    char buf[12];
    num_to_str(ticks / 100, buf);
    syscall_write(1, buf, strlen(buf));
    syscall_write(1, " seconds\n", 9);
}

// Built-in: ps
static void builtin_ps(void) {
    syscall_write(1, "PID  NAME\n", 10);
    syscall_write(1, "0    kernel\n", 12);
    syscall_write(1, "1    shell_user\n", 16);
}

// Built-in: free
static void builtin_free(void) {
    syscall_write(1, "Total: 64MB\nUsed: 8MB\nFree: 56MB\n", 34);
}

// Built-in: exit
static void builtin_exit(void) {
    syscall_write(1, "Goodbye!\n", 9);
    syscall_exit(0);
}

// Execute command
static void execute_command(char* cmd) {
    // Skip empty
    while (*cmd == ' ') cmd++;
    if (!*cmd) return;
    
    // Add to history
    if (hist_count < 16) {
        strcpy_t(cmd_history[hist_count++], cmd);
    }
    hist_pos = hist_count;
    
    // Parse args
    char* args[MAX_ARGS];
    uint32_t argc = parse_args(cmd, args);
    if (argc == 0) return;
    
    // Check built-ins
    if (strcmp(args[0], "help") == 0) {
        builtin_help();
    } else if (strcmp(args[0], "cd") == 0) {
        builtin_cd(args[1]);
    } else if (strcmp(args[0], "pwd") == 0) {
        syscall_write(1, cwd, strlen(cwd));
        syscall_write(1, "\n", 1);
    } else if (strcmp(args[0], "ls") == 0) {
        builtin_ls();
    } else if (strcmp(args[0], "cat") == 0) {
        builtin_cat(args[1]);
    } else if (strcmp(args[0], "echo") == 0) {
        builtin_echo(argc, args);
    } else if (strcmp(args[0], "clear") == 0) {
        builtin_clear();
    } else if (strcmp(args[0], "whoami") == 0) {
        builtin_whoami();
    } else if (strcmp(args[0], "id") == 0) {
        builtin_id();
    } else if (strcmp(args[0], "date") == 0) {
        builtin_date();
    } else if (strcmp(args[0], "uname") == 0) {
        builtin_uname();
    } else if (strcmp(args[0], "uptime") == 0) {
        builtin_uptime();
    } else if (strcmp(args[0], "ps") == 0) {
        builtin_ps();
    } else if (strcmp(args[0], "free") == 0) {
        builtin_free();
    } else if (strcmp(args[0], "exit") == 0) {
        builtin_exit();
    } else {
        syscall_write(1, "Unknown command: ", 17);
        syscall_write(1, args[0], strlen(args[0]));
        syscall_write(1, "\nType 'help' for commands\n", 26);
    }
}

// User shell main
void shell_user_main(void) {
    char cmd[MAX_CMD_LEN];
    
    // Print welcome
    syscall_write(1, "\n=== Tritos User Shell v1.0 ===\n", 31);
    syscall_write(1, "Running in Ring 3 (user mode)\n", 30);
    syscall_write(1, "Type 'help' for commands\n\n", 27);
    
    // Main loop
    while (1) {
        print_prompt();
        read_line(cmd, MAX_CMD_LEN);
        execute_command(cmd);
    }
}
