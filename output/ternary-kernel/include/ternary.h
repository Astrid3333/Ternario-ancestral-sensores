/**
 * ternary.h — Definiciones core del kernel ternario ancestral (v2)
 * 
 * Corregido:
 * - Agregados outb/inb como inline assembly
 * - Fix return types para(mem_alloc, mem_get_base)
 * - Agregadas validaciones
 * - Agregadas funciones de libc ternaria
 */

#ifndef TERNARY_H
#define TERNARY_H

// Freestanding types (no stdlib)
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;
typedef signed long long int64_t;
typedef unsigned long long uint64_t;

// =============================================================================
// I/O PORTS — Inline assembly para x86
// =============================================================================

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outl(uint16_t port, uint32_t val) {
    asm volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

// =============================================================================
// TIPOS TERNARIOS
// =============================================================================

typedef int8_t trit_t;
typedef uint8_t trit_packed_t;

typedef struct {
    uint8_t high;
    uint8_t low;
} __attribute__((packed)) babilonian_addr_t;

typedef struct {
    uint16_t tzolkin;
    uint8_t haab;
} __attribute__((packed)) maya_pid_t;

// =============================================================================
// CONSTANTES
// =============================================================================

#define TRIT_NEG    (-1)
#define TRIT_ZERO   (0)
#define TRIT_POS    (1)

#define BASE_60     60
#define TZOLKIN     260
#define HAAB        365
#define MAX_PROCS   33
#define MEM_BLOCKS  60
#define BLOCK_SIZE  60

#define COLOR_FREE      0
#define COLOR_KERNEL    1
#define COLOR_PROCESS   2
#define COLOR_STACK     3
#define COLOR_CODE      4
#define COLOR_DATA      5

// =============================================================================
// OPERACIONES TERNARIAS
// =============================================================================

static inline trit_t trit_add(trit_t a, trit_t b) {
    trit_t sum = a + b;
    if (sum > 1) return 1;
    if (sum < -1) return -1;
    return sum;
}

static inline trit_t trit_mul(trit_t a, trit_t b) {
    if (a == 0 || b == 0) return 0;
    return (a == b) ? 1 : -1;
}

static inline trit_t trit_neg(trit_t a) {
    return -a;
}

static inline char trit_to_char(trit_t t) {
    if (t == -1) return '-';
    if (t == 0) return '0';
    return '+';
}

static inline trit_t trit_cmp(trit_t a, trit_t b) {
    if (a > b) return 1;
    if (a < b) return -1;
    return 0;
}

// =============================================================================
// EMPAQUETADO DE TRITS
// =============================================================================

static inline trit_packed_t pack_trits(trit_t t1, trit_t t2, trit_t t3) {
    uint8_t v1 = (uint8_t)(t1 + 1);
    uint8_t v2 = (uint8_t)(t2 + 1);
    uint8_t v3 = (uint8_t)(t3 + 1);
    return v1 * 9 + v2 * 3 + v3;
}

static inline void unpack_trits(trit_packed_t packed, trit_t* t1, trit_t* t2, trit_t* t3) {
    *t1 = (trit_t)((packed / 9) % 3) - 1;
    *t2 = (trit_t)((packed / 3) % 3) - 1;
    *t3 = (trit_t)(packed % 3) - 1;
}

// =============================================================================
// DIRECCIONES BABILÓNICAS
// =============================================================================

static inline babilonian_addr_t linear_to_babilonian(uint16_t addr) {
    babilonian_addr_t result;
    result.high = addr / BASE_60;
    result.low = addr % BASE_60;
    return result;
}

static inline uint16_t babilonian_to_linear(babilonian_addr_t addr) {
    return addr.high * BASE_60 + addr.low;
}

// =============================================================================
// IDs MAYAS
// =============================================================================

static inline maya_pid_t make_maya_pid(uint32_t counter) {
    maya_pid_t pid;
    pid.tzolkin = (counter % TZOLKIN) + 1;
    pid.haab = (counter % HAAB) + 1;
    return pid;
}

static inline trit_t compare_maya_pid(maya_pid_t a, maya_pid_t b) {
    if (a.tzolkin != b.tzolkin) {
        return (a.tzolkin > b.tzolkin) ? 1 : -1;
    }
    if (a.haab != b.haab) {
        return (a.haab > b.haab) ? 1 : -1;
    }
    return 0;
}

// =============================================================================
// TIPOS DE PROCESOS Y SCHEDULER
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

// =============================================================================
// LIBC TERNARIA MÍNIMA
// =============================================================================

static inline uint16_t strlen_t(const char* str) {
    uint16_t len = 0;
    while (str[len]) len++;
    return len;
}

static inline int8_t strcmp_t(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return (*a > *b) ? 1 : -1;
        a++;
        b++;
    }
    if (*a) return 1;
    if (*b) return -1;
    return 0;
}

static inline void strcpy_t(char* dst, const char* src) {
    while (*src) {
        *dst++ = *src++;
    }
    *dst = 0;
}

static inline void memset_t(void* ptr, uint8_t val, uint16_t size) {
    uint8_t* p = (uint8_t*)ptr;
    for (uint16_t i = 0; i < size; i++) {
        p[i] = val;
    }
}

static inline void memcpy_t(void* dst, const void* src, uint16_t size) {
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;
    for (uint16_t i = 0; i < size; i++) {
        d[i] = s[i];
    }
}

static inline int8_t strncmp_t(const char* a, const char* b, uint8_t n) {
    for (uint8_t i = 0; i < n; i++) {
        if (a[i] != b[i]) return (a[i] > b[i]) ? 1 : -1;
        if (a[i] == 0) return 0;
    }
    return 0;
}

static inline void num_to_hex(uint32_t n, char* buf) {
    const char hex[] = "0123456789ABCDEF";
    buf[0] = hex[(n >> 28) & 0xF];
    buf[1] = hex[(n >> 24) & 0xF];
    buf[2] = hex[(n >> 20) & 0xF];
    buf[3] = hex[(n >> 16) & 0xF];
    buf[4] = hex[(n >> 12) & 0xF];
    buf[5] = hex[(n >> 8) & 0xF];
    buf[6] = hex[(n >> 4) & 0xF];
    buf[7] = hex[n & 0xF];
    buf[8] = 0;
}

static inline void num_to_str(uint32_t n, char* buf) {
    if (n == 0) { buf[0] = '0'; buf[1] = 0; return; }
    char tmp[12];
    int i = 0;
    while (n > 0) {
        tmp[i++] = '0' + (n % 10);
        n /= 10;
    }
    for (int j = 0; j < i; j++) {
        buf[j] = tmp[i - 1 - j];
    }
    buf[i] = 0;
}

static inline void int_to_str(int32_t n, char* buf) {
    if (n < 0) { buf[0] = '-'; num_to_str(-n, buf + 1); }
    else { num_to_str(n, buf); }
}

static inline int8_t memcmp_t(const void* a, const void* b, uint16_t n) {
    const uint8_t* pa = (const uint8_t*)a;
    const uint8_t* pb = (const uint8_t*)b;
    for (uint16_t i = 0; i < n; i++) {
        if (pa[i] != pb[i]) return (pa[i] > pb[i]) ? 1 : -1;
    }
    return 0;
}

static inline void strcat_t(char* dst, const char* src) {
    while (*dst) dst++;
    while (*src) *dst++ = *src++;
    *dst = 0;
}

static inline int8_t is_digit(char c) {
    return c >= '0' && c <= '9';
}

static inline int16_t parse_num(const char* str) {
    int16_t num = 0;
    int8_t neg = 0;
    if (*str == '-') { neg = 1; str++; }
    while (*str >= '0' && *str <= '9') {
        num = num * 10 + (*str - '0');
        str++;
    }
    return neg ? -num : num;
}

// =============================================================================
// DECLARACIONES DE FUNCIONES (scheduler.c, memory.c)
// =============================================================================

void mem_init(void);
void mem_get_status(uint8_t* used, uint8_t* free_count, uint8_t* locked);
uint8_t mem_get_color(uint8_t block);

void sched_init(void);
int8_t sched_create(uint8_t parent, trit_t priority);
int8_t sched_kill(uint8_t pid);
int8_t sched_sleep(uint8_t pid);
int8_t sched_wake(uint8_t pid);
uint8_t sched_tick(void);
uint8_t sched_get_current(void);
void sched_get_state(uint32_t* tick, uint8_t* tzolkin, uint8_t* haab, trit_t* load);
int8_t sched_get_info(uint8_t pid, process_t* info);
int8_t sys_fork(void);

// Filesystem
void fs_init(void);
void fs_list_files(void);
int8_t fs_create(const char* filename);
int32_t fs_write(int8_t fd, const uint8_t* buf, uint32_t len);
int8_t fs_delete(const char* filename);
int32_t fs_get_size(const char* filename);
int8_t fs_open(const char* filename, uint8_t mode);
int32_t fs_read(int8_t fd, uint8_t* buf, uint32_t len);
void fs_close(int8_t fd);

// =============================================================================
// DECLARACIONES DE FUNCIONES DE RED
// =============================================================================

// VGA (defined in kernel.c)
extern void vga_puts(const char* str);
extern void vga_putc(char c);
extern void vga_set_color(uint8_t fg, uint8_t bg);
extern void vga_puts_at(int x, int y, const char* str, uint8_t color);
extern void vga_draw_window(int x, int y, int w, int h, const char* title, uint8_t border_color, uint8_t title_color);

// PCI
typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t  revision_id;
    uint8_t  prog_if;
    uint8_t  subclass;
    uint8_t  class_code;
    uint8_t  cache_line_size;
    uint8_t  latency_timer;
    uint8_t  header_type;
    uint8_t  bist;
    uint32_t bar[6];
    uint32_t cardbus_cis;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;
    uint32_t expansion_rom;
    uint8_t  capabilities_ptr;
    uint8_t  reserved1[3];
    uint32_t reserved2;
    uint8_t  interrupt_line;
    uint8_t  interrupt_pin;
    uint8_t  min_gnt;
    uint8_t  max_lat;
} __attribute__((packed)) pci_device_t;

void pci_init(void);
int8_t pci_find_device(uint16_t vendor_id, uint16_t device_id, pci_device_t* dev);
uint32_t pci_read_bar0(uint16_t bus, uint16_t device, uint16_t function);
uint8_t pci_read_irq(uint16_t bus, uint16_t device, uint16_t function);

// Network
void net_init(void);
void net_poll(void);
void net_receive(uint8_t* packet, uint16_t len);
void net_send_packet(uint8_t* dst_mac, uint16_t ether_type, uint8_t* data, uint16_t len);
void arp_resolve(uint32_t ip);
void ip_send(uint32_t dst_ip, uint8_t protocol, uint8_t* data, uint16_t len);
void icmp_echo_request(uint32_t dst_ip);
void icmp_handle(uint8_t* packet, uint16_t len, uint32_t src_ip);
void udp_send(uint32_t dst_ip, uint16_t dst_port, uint16_t src_port, uint8_t* data, uint16_t len);
void udp_handle(uint8_t* packet, uint16_t len, uint32_t src_ip);
uint32_t dns_resolve(const char* hostname);
void dns_set_server(uint32_t server);

// TCP
int8_t tcp_connect(uint32_t dst_ip, uint16_t dst_port);
int8_t tcp_listen(uint16_t port);
int16_t tcp_recv(int8_t sock_idx, uint8_t* buf, uint16_t max_len);
int16_t tcp_send(int8_t sock_idx, uint8_t* data, uint16_t len);
void tcp_close(int8_t sock_idx);

// DHCP
void dhcp_start(void);
void dhcp_handle(uint8_t* packet, uint16_t len, uint32_t src_ip);

// HTTP
uint32_t http_get(const char* url, uint8_t* buf, uint32_t buf_size);
void cmd_wget(const char* url);
void cmd_curl(const char* url);

// TLS/HTTPS
int8_t tls_connect(uint32_t ip, uint16_t port);
int32_t tls_send(const uint8_t* data, uint16_t len);
int32_t tls_recv(uint8_t* buf, uint16_t len);
void tls_close(void);
uint32_t https_get(const char* url, uint8_t* buf, uint32_t buf_size);

// Package Manager
void pkg_init(void);
void pkg_search(const char* query);
void pkg_list(void);
int8_t pkg_install(const char* name);
int8_t pkg_remove(const char* name);
void pkg_update(void);
void pkg_upgrade(void);
void pkg_info(const char* name);
void pkg_add_repo(const char* name, const char* url);
void pkg_list_repos(void);

// Firewall
void fw_init(void);
void fw_enable(void);
void fw_disable(void);
int8_t fw_add_rule(uint32_t ip, uint32_t mask, uint16_t port, uint8_t protocol, uint8_t action);
void fw_block_ip(uint32_t ip);
void fw_unblock_ip(uint32_t ip);
uint8_t fw_check(uint32_t src_ip, uint16_t port, uint8_t protocol);
void fw_list_rules(void);
void fw_list_blocked(void);
void fw_status(void);

// Download Manager
void dl_init(void);
int8_t dl_add(const char* url, const char* filename);
int8_t dl_start(int8_t slot);
void dl_pause(int8_t slot);
void dl_resume(int8_t slot);
void dl_remove(int8_t slot);
void dl_list(void);
uint8_t dl_verify(int8_t slot);
void dl_status(void);

// Mail
void mail_init(void);
int8_t mail_add_account(const char* email, const char* smtp_host, const char* pop3_host, uint16_t smtp_port, uint16_t pop3_port);
int8_t mail_send(const char* to, const char* subject, const char* body);
int8_t mail_receive(void);
void mail_list(void);
void mail_read(uint8_t index);
void mail_delete(uint8_t index);
void mail_accounts(void);
void mail_status(void);

// Chat
void chat_init(void);
void chat_join(const char* room);
void chat_send(const char* message);
void chat_history(void);
void chat_users(void);
void chat_rooms(void);
void chat_leave(void);
void chat_status(void);
void chat_pm(const char* user, const char* message);

// VPN
void vpn_init(void);
int8_t vpn_connect(const char* server, uint16_t port);
void vpn_disconnect(int8_t idx);
void vpn_status(void);

// DNS over HTTPS
void doh_init(void);
void doh_enable(void);
void doh_disable(void);
uint32_t doh_resolve(const char* hostname);
void doh_status(void);

// Domain Blocking
void block_init(void);
void block_enable(void);
void block_disable(void);
int8_t block_add(const char* domain);
void block_remove(const char* domain);
uint8_t block_check(const char* domain);
void block_list(void);
void block_status(void);

// Parental Control
void parental_init(void);
void parental_enable(void);
void parental_disable(void);
void parental_set_schedule(uint8_t day, uint8_t start, uint8_t end);
int8_t parental_add_filter(const char* keyword);
uint8_t parental_check_time(void);
uint8_t parental_check_content(const char* content);
void parental_list_filters(void);
void parental_status(void);

// AI Client
void ai_client_init(void);
void ai_enable(void);
void ai_disable(void);
int8_t ai_query(const char* prompt, char* response);
void ai_history_show(void);
void ai_status(void);

// FTP
int8_t ftp_connect(const char* host, uint16_t port);
int8_t ftp_get(const char* host, const char* path, const char* filename);
void ftp_list(const char* host);

// Sockets API
void sockets_init(void);
int8_t sock_create(uint8_t type);
int8_t sock_connect(int8_t fd, uint32_t ip, uint16_t port);
int8_t sock_listen(int8_t fd, uint16_t port);
int32_t sock_send(int8_t fd, const uint8_t* data, uint16_t len);
int32_t sock_recv(int8_t fd, uint8_t* buf, uint16_t len);
void sock_close(int8_t fd);
void sock_info(int8_t fd);
void sock_list(void);

// GDT
void gdt_init(void);

// Paging
void paging_init(void);
void paging_map_page(uint32_t virtual, uint32_t physical, uint32_t flags);
void paging_unmap_page(uint32_t virtual);
uint32_t paging_get_physical(uint32_t virtual);
void page_fault_handler(uint32_t error_code);
uint32_t paging_get_fault_count(void);
void paging_status(void);

// Syscalls
void syscalls_init(void);
void syscall_dispatch(uint32_t* regs);

// ELF loader
int8_t elf_load(const uint8_t* data, uint32_t size, uint32_t* entry_point);
int8_t elf_execute(const uint8_t* data, uint32_t size);

// Framebuffer
void framebuffer_init(uint32_t addr, uint32_t width, uint32_t height, uint32_t pitch, uint8_t bpp);
uint8_t fb_is_active(void);
void fb_console_init(uint32_t width, uint32_t height);
void fb_console_putc(char c, uint8_t color);
void fb_set_pixel(uint32_t x, uint32_t y, uint32_t color);
uint32_t fb_get_pixel(uint32_t x, uint32_t y);
void fb_fill(uint32_t color);
void fb_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void fb_draw_line(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t color);
void fb_draw_circle(uint32_t cx, uint32_t cy, uint32_t r, uint32_t color);
void fb_draw_char(uint32_t x, uint32_t y, char c, uint32_t color, uint32_t bg);
void fb_draw_string(uint32_t x, uint32_t y, const char* str, uint32_t color, uint32_t bg);
void fb_get_info(uint32_t* width, uint32_t* height);
uint32_t fb_get_ternary_color(uint8_t index);
void fb_draw_trit_blocks(uint32_t x, uint32_t y, uint8_t trit0, uint8_t trit1, uint8_t trit2, uint32_t size);
void fb_draw_ternary_grid(uint32_t x, uint32_t y, uint32_t w, uint32_t h);

// Ternary color palette (27 colors, base-3)
#define TRIT_OTH    0xE85D04
#define TRIT_HAAB   0x0077B6
#define TRIT_TZOLKIN 0x2D6A4F
#define TRIT_000    0x0B0B1A
#define TRIT_001    0x003566
#define TRIT_002    0x0077B6
#define TRIT_010    0x0D4A0D
#define TRIT_011    0x2D6A4F
#define TRIT_012    0x40916C
#define TRIT_020    0x9B2226
#define TRIT_021    0xBB3E03
#define TRIT_022    0xE85D04
#define TRIT_100    0x3A0CA3
#define TRIT_101    0x7209B7
#define TRIT_102    0x560BAD
#define TRIT_110    0x38B000
#define TRIT_111    0x70E000
#define TRIT_112    0xCCD700
#define TRIT_120    0xF77F00
#define TRIT_121    0xFCBF49
#define TRIT_122    0xFFD166
#define TRIT_200    0xE63946
#define TRIT_201    0xD62828
#define TRIT_202    0xC1121F
#define TRIT_210    0xF4845F
#define TRIT_211    0xF7B267
#define TRIT_212    0xF7D794
#define TRIT_220    0xFFFFFF
#define TRIT_221    0xE0E0E0
#define TRIT_222    0xCCCCCC

// Mouse
void mouse_init(void);
void mouse_get_position(int32_t* x, int32_t* y);
uint8_t mouse_get_buttons(void);
uint8_t mouse_left_button(void);
uint8_t mouse_right_button(void);
uint8_t mouse_middle_button(void);
void mouse_draw_cursor(void);
void mouse_status(void);

// Window Manager
void wm_init(void);
int8_t wm_create_window(const char* title, uint8_t col, uint8_t row, uint8_t w, uint8_t h, uint8_t bg_attr);
void wm_close_window(int8_t id);
void wm_minimize_window(int8_t id);
void wm_restore_window(int8_t id);
void wm_maximize_window(int8_t id);
void wm_focus_window(int8_t id);
void wm_move_window(int8_t id, uint8_t col, uint8_t row);
void wm_resize_window(int8_t id, uint8_t w, uint8_t h);
void wm_handle_click(uint8_t x, uint8_t y);
void wm_handle_drag(int32_t mx, int32_t my);
void wm_handle_release(void);
uint8_t wm_get_window_count(void);
int8_t wm_get_focused(void);
void wm_status(void);

// AC97 Sound
void ac97_init(void);
void ac97_set_volume(uint8_t left, uint8_t right);
void ac97_set_pcm_volume(uint8_t left, uint8_t right);
void ac97_set_rate(uint32_t rate);
void ac97_play_pcm(int16_t* buffer, uint32_t samples);
void ac97_beep(uint32_t freq, uint32_t duration_ms);
void ac97_status(void);

// USB
void usb_init(void);
uint8_t usb_get_device_count(void);
void usb_get_device_info(uint8_t index, uint16_t* vendor, uint16_t* product, uint8_t* class);
void usb_status(void);

// USB Mass Storage
void usb_storage_detect(void);
int usb_storage_mount(uint8_t dev_idx, const char* mount_point);
int usb_storage_unmount(uint8_t dev_idx);
int usb_storage_read_block(uint8_t port, uint32_t lba, uint8_t* buffer);
int usb_storage_write_block(uint8_t port, uint32_t lba, const uint8_t* buffer);
void usb_storage_status(void);

// Dynamic ELF
int elf_load_dynamic(const uint8_t* data, uint32_t size, uint32_t* entry_point);
int dyn_add_symbol(const char* name, uint32_t addr, uint32_t size, uint8_t type, uint8_t bind);
uint32_t dyn_find_symbol(const char* name);
void dyn_list_symbols(void);
int dyn_load_library(const char* name, const uint8_t* data, uint32_t size);
void dyn_unload_library(const char* name);
void dyn_list_libraries(void);

// Multi-User
void user_init(void);
int user_create(const char* name, const char* pass, uint8_t gid);
int user_delete(const char* name);
int user_login(const char* name, const char* pass);
void user_logout(void);
int user_get_current(void);
const char* user_get_name(uint8_t uid);
int group_create(const char* name);
int group_add_member(const char* group_name, const char* user_name);
void user_status(void);

// Educational module
void edu_status(void);

// Ternary language
int tri_run(const char* source);
void editor_init(void);
void editor_insert_char(char c);
void editor_insert_newline(void);
void editor_backspace(void);
void editor_move_cursor(int dx, int dy);
void editor_save(void);
void editor_load(const char* filename);
void editor_display(void);
void editor_run(void);
void tri_editor(const char* filename);
int tri_editor_cmd(const char* cmd);
void cmd_tri(const char* args);
uint8_t tri_editor_is_active(void);

// Phase 3: Education
void history_add(const char* cmd);
void history_show(void);
void history_save(void);
void history_load(void);
void cmd_hist(const char* args);
void cmd_bench(const char* args);
void cmd_dual(const char* args);
uint8_t dual_get_mode(void);
void cmd_savings(const char* args);
void phase3_status(void);

// Phase 4: Advanced
void vm_init(void);
int vm_execute(const uint8_t* code, uint32_t size);
int vm_compile(const char* source, uint8_t* bytecode, int* size);
void cmd_vm(const char* args);
void cmd_logic(const char* args);
void cmd_compile(const char* args);
void cmd_ai_tri(const char* args);
void phase4_status(void);

// Applications
void cmd_calc(const char* args);
void clock_init(void);
void clock_tick(void);
void clock_get_time(uint8_t* hour, uint8_t* min, uint8_t* sec);
void cmd_clock(const char* args);
void cmd_convert(const char* args);
void cmd_tetris(const char* args);
void apps_status(void);

// Fonts & Widgets
#define WIDGET_NONE     0
#define WIDGET_BUTTON   1
#define WIDGET_LABEL    2
#define WIDGET_SCROLLBAR 3
#define WIDGET_PANEL    4

typedef struct {
    uint8_t id;
    uint8_t type;
    int32_t x, y, w, h;
    char text[64];
    uint32_t bg_color, fg_color;
    uint8_t visible, enabled, focused;
    int32_t value, min_val, max_val;
    void (*on_click)(uint8_t id);
    void (*on_change)(uint8_t id, int32_t value);
} widget_t;

void widgets_init(void);
uint8_t widget_create_button(int32_t x, int32_t y, int32_t w, int32_t h, const char* text, uint32_t bg, uint32_t fg);
uint8_t widget_create_label(int32_t x, int32_t y, const char* text, uint32_t fg);
uint8_t widget_create_scrollbar(int32_t x, int32_t y, int32_t h, int32_t min, int32_t max, int32_t initial);
uint8_t widget_create_panel(int32_t x, int32_t y, int32_t w, int32_t h, const char* title, uint32_t bg);
void widgets_draw(void);
void widgets_click(int32_t x, int32_t y);
widget_t* widgets_get(uint8_t id);
void cmd_snake(const char* args);
void cmd_pong(const char* args);

// Ternary Operations
typedef int8_t trit;  // -1, 0, 1

void trit_to_str(trit t, char* out);
int int_to_ternary(int n, trit* trits, int max_len);
int ternary_to_int(const trit* trits, int len);
void trits_to_ternary_str(const trit* trits, int len, char* str);
int ternary_str_to_trits(const char* str, trit* trits, int max_len);
int ternary_add(const trit* a, const trit* b, trit* result, int max_len);
void ternary_mul(const trit* a, int a_len, const trit* b, int b_len,
                 trit* result, int max_len);
void ternary_sub(const trit* a, const trit* b, trit* result, int max_len);

// Base conversion
int int_to_base60(int n, int* digits, int max_digits);
void maya_digit_to_str(int digit, char* str);

void cmd_trinary(const char* args);
void cmd_tcalc(const char* args);

// Ternary Compiler
int tric_compile(const char* source, uint8_t* output, int* size);
void cmd_tric(const char* args);

// Math Shell
void cmd_math(const char* args);

// Tutorials
void cmd_tutorial(const char* args);

// Lab — Ancestral Math Lab
void cmd_formula(const char* args);
void cmd_lab(const char* args);
void cmd_codigo(const char* args);
void cmd_ancestro(const char* args);
void cmd_patron(const char* args);
void cmd_lab_experimento(const char* args);
void cmd_lab_tutorial(const char* args);
void lab_status(void);

// User shell
void shell_user_main(void);

// Process management
typedef struct {
    uint8_t pid;
    uint8_t ppid;
    uint8_t state;
    uint8_t flags;
    uint8_t priority;
    uint32_t ticks;
    uint32_t wait_ticks;
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip, eflags;
    uint32_t* page_directory;
    uint32_t kernel_stack[1024];
    uint32_t code_start, code_end;
    uint32_t data_start, data_end;
    uint32_t heap_start, heap_end;
    uint32_t stack_start;
    int8_t fd[16];
    int32_t exit_status;
    char name[32];
} pcb_t;

// Process states
#define PROC_UNUSED    0
#define PROC_READY     1
#define PROC_RUNNING   2
#define PROC_BLOCKED   3
#define PROC_ZOMBIE    4
#define PROC_WAITING   5

#define PROC_FLAG_KERNEL  0x01
#define PROC_FLAG_USER    0x02
#define MAX_PROCESSES 32

// Process Control Block (new)
typedef struct {
    uint8_t pid;
    uint8_t ppid;
    uint8_t state;
    uint8_t flags;
    uint8_t priority;
    uint32_t ticks;
    uint32_t wait_ticks;
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip, eflags;
    uint32_t* page_directory;
    uint32_t kernel_stack[1024];
    uint32_t code_start, code_end;
    uint32_t data_start, data_end;
    uint32_t heap_start, heap_end;
    uint32_t stack_start;
    int8_t fd[16];
    int32_t exit_status;
    char name[32];
} proc_t;

void process_init(void);
int8_t process_create(const char* name, uint32_t entry_point, uint8_t flags);
void process_exit(int32_t status);
int8_t process_wait(uint8_t child_pid, int32_t* status);
void process_schedule(void);
void process_switch(uint8_t new_pid);
proc_t* process_current(void);
proc_t* process_get(uint8_t pid);
void process_list(void);
uint8_t process_get_count(void);
uint8_t process_get_pid(void);
void context_switch(uint32_t* old_regs, uint32_t* new_regs);
void save_state(uint32_t* state);
void load_state(uint32_t* state);

// Syscall wrappers
void syscall_exit(int status);
int syscall_read(int fd, void* buf, size_t count);
int syscall_write(int fd, const void* buf, size_t count);
int syscall_open(const char* path, int flags);
void syscall_close(int fd);
int syscall_getpid(void);
void syscall_sleep(uint32_t ms);
uint32_t syscall_time(void);

// libc (full)
size_t strlen(const char* s);
char* strcpy(char* dst, const char* src);
char* strncpy(char* dst, const char* src, size_t n);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
char* strcat(char* dst, const char* src);
char* strchr(const char* s, int c);
char* strrchr(const char* s, int c);
void* memset(void* s, int c, size_t n);
void* memcpy(void* dst, const void* src, size_t n);
void* memmove(void* dst, const void* src, size_t n);
int memcmp(const void* s1, const void* s2, size_t n);
int printf(const char* format, ...);
int puts(const char* s);
int putchar(int c);
int getchar(void);
int atoi(const char* s);
char* itoa(int value, char* str, int base);
void* malloc(size_t size);
void free(void* ptr);
void* realloc(void* ptr, size_t size);
int abs(int x);
int pow(int base, int exp);
int sqrt(int x);

// MACros de red
#define ETH_ARP      0x0806
#define ETH_IP       0x0800
#define ARP_REQUEST  1
#define ARP_REPLY    2
#define IP_PROTOCOL_ICMP  1
#define IP_PROTOCOL_UDP   17
#define IP_PROTOCOL_TCP   6
#define DNS_PORT    53
#define DHCP_PORT   67
#define DHCP_CLIENT_PORT 68

#endif // TERNARY_H
