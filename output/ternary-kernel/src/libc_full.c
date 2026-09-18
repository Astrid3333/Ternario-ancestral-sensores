/**
 * libc_full.c — libc completa para user-space
 *
 * Funciones estándar de C para apps de usuario
 */

#include "../include/ternary.h"

// Freestanding va_args
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_end(ap) __builtin_va_end(ap)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_copy(dest, src) __builtin_va_copy(dest, src)

// =============================================================================
// Syscall wrappers (user-space)
// =============================================================================

void syscall_exit(int status) {
    asm volatile("int $0x80" : : "a"(0), "b"(status));
}

int syscall_read(int fd, void* buf, size_t count) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(1), "b"(fd), "c"(buf), "d"(count));
    return result;
}

int syscall_write(int fd, const void* buf, size_t count) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(2), "b"(fd), "c"(buf), "d"(count));
    return result;
}

int syscall_open(const char* path, int flags) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(3), "b"(path), "c"(flags));
    return result;
}

void syscall_close(int fd) {
    asm volatile("int $0x80" : : "a"(4), "b"(fd));
}

int syscall_getpid(void) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(8));
    return result;
}

void syscall_sleep(uint32_t ms) {
    asm volatile("int $0x80" : : "a"(9), "b"(ms));
}

uint32_t syscall_time(void) {
    uint32_t result;
    asm volatile("int $0x80" : "=a"(result) : "a"(10));
    return result;
}

// =============================================================================
// String functions
// =============================================================================

size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

char* strcpy(char* dst, const char* src) {
    char* d = dst;
    while ((*d++ = *src++));
    return dst;
}

char* strncpy(char* dst, const char* src, size_t n) {
    char* d = dst;
    while (n && (*d++ = *src++)) n--;
    while (n--) *d++ = 0;
    return dst;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && *s1 == *s2) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

char* strcat(char* dst, const char* src) {
    char* d = dst;
    while (*d) d++;
    while ((*d++ = *src++));
    return dst;
}

char* strchr(const char* s, int c) {
    while (*s) {
        if (*s == (char)c) return (char*)s;
        s++;
    }
    return (c == 0) ? (char*)s : 0;
}

char* strrchr(const char* s, int c) {
    const char* last = 0;
    while (*s) {
        if (*s == (char)c) last = s;
        s++;
    }
    return (c == 0) ? (char*)s : (char*)last;

}

char* strstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    size_t nlen = strlen(needle);
    while (*haystack) {
        if (strncmp(haystack, needle, nlen) == 0) return (char*)haystack;
        haystack++;
    }
    return 0;
}

// =============================================================================
// Memory functions
// =============================================================================

void* memset(void* s, int c, size_t n) {
    unsigned char* p = (unsigned char*)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

void* memcpy(void* dst, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;
    while (n--) *d++ = *s++;
    return dst;
}

void* memmove(void* dst, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;
    
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
    return dst;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;
    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++;
        p2++;
    }
    return 0;
}

void* memchr(const void* s, int c, size_t n) {
    const unsigned char* p = (const unsigned char*)s;
    while (n--) {
        if (*p == (unsigned char)c) return (void*)p;
        p++;
    }
    return 0;
}

// =============================================================================
// Stdio functions
// =============================================================================

// Internal put char to stdout
static void stdout_putchar(char c) {
    syscall_write(1, &c, 1);
}

static void stdout_puts(const char* s) {
    syscall_write(1, s, strlen(s));
}

// printf format parser
static int format_value(const char** fmt, va_list args) {
    const char* f = *fmt;
    int count = 0;
    
    f++; // skip '%'
    
    // Parse flags
    int pad_zero = 0;
    int pad_width = 0;
    int is_long = 0;
    
    if (*f == '0') { pad_zero = 1; f++; }
    while (*f >= '0' && *f <= '9') {
        pad_width = pad_width * 10 + (*f - '0');
        f++;
    }
    if (*f == 'l') { is_long = 1; f++; }
    
    switch (*f) {
        case 's': {
            const char* s = va_arg(args, const char*);
            if (!s) s = "(null)";
            int len = strlen(s);
            while (pad_width > len) { stdout_putchar(' '); pad_width--; }
            stdout_puts(s);
            count += len;
            break;
        }
        case 'd':
        case 'i': {
            long d;
            if (is_long) d = va_arg(args, long);
            else d = va_arg(args, int);
            char buf[24];
            if (d < 0) { stdout_putchar('-'); d = -d; count++; }
            int_to_str(d, buf);
            int len = strlen(buf);
            while (pad_width > len) { stdout_putchar(pad_zero ? '0' : ' '); pad_width--; }
            stdout_puts(buf);
            count += len;
            break;
        }
        case 'u': {
            unsigned long u;
            if (is_long) u = va_arg(args, unsigned long);
            else u = va_arg(args, unsigned int);
            char buf[24];
            num_to_str(u, buf);
            int len = strlen(buf);
            while (pad_width > len) { stdout_putchar(pad_zero ? '0' : ' '); pad_width--; }
            stdout_puts(buf);
            count += len;
            break;
        }
        case 'x': {
            unsigned long x;
            if (is_long) x = va_arg(args, unsigned long);
            else x = va_arg(args, unsigned int);
            char buf[12];
            num_to_hex(x, buf);
            int len = strlen(buf);
            while (pad_width > len) { stdout_putchar(pad_zero ? '0' : ' '); pad_width--; }
            stdout_puts(buf);
            count += len;
            break;
        }
        case 'p': {
            unsigned long p = (unsigned long)va_arg(args, void*);
            char buf[12];
            num_to_hex(p, buf);
            stdout_puts("0x");
            stdout_puts(buf);
            count += 10;
            break;
        }
        case 'c': {
            char c = (char)va_arg(args, int);
            stdout_putchar(c);
            count++;
            break;
        }
        case '%': {
            stdout_putchar('%');
            count++;
            break;
        }
        case 'n': {
            int* p = va_arg(args, int*);
            *p = count;
            break;
        }
        default:
            stdout_putchar('%');
            stdout_putchar(*f);
            count += 2;
    }
    
    f++;
    *fmt = f;
    return count;
}

int printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    int count = 0;
    const char* f = format;
    
    while (*f) {
        if (*f == '%') {
            count += format_value(&f, args);
        } else {
            stdout_putchar(*f);
            count++;
            f++;
        }
    }
    
    va_end(args);
    return count;
}

int puts(const char* s) {
    int count = 0;
    while (*s) {
        stdout_putchar(*s);
        s++;
        count++;
    }
    stdout_putchar('\n');
    return count + 1;
}

int putchar(int c) {
    stdout_putchar((char)c);
    return c;
}

int getchar(void) {
    char c;
    syscall_read(0, &c, 1);
    return c;
}

int sprintf(char* str, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    int count = 0;
    char* out = str;
    
    while (*format) {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 's': {
                    const char* s = va_arg(args, const char*);
                    while (*s) *out++ = *s++;
                    break;
                }
                case 'd': {
                    int d = va_arg(args, int);
                    char buf[12];
                    int_to_str(d, buf);
                    char* p = buf;
                    while (*p) *out++ = *p++;
                    break;
                }
                case 'x': {
                    unsigned int x = va_arg(args, unsigned int);
                    char buf[12];
                    num_to_hex(x, buf);
                    char* p = buf;
                    while (*p) *out++ = *p++;
                    break;
                }
                case 'c': {
                    *out++ = (char)va_arg(args, int);
                    break;
                }
                case '%': {
                    *out++ = '%';
                    break;
                }
            }
        } else {
            *out++ = *format;
        }
        format++;
    }
    
    *out = 0;
    va_end(args);
    return count;
}

int sscanf(const char* str, const char* format, ...) {
    // Simplified sscanf - only supports %d and %s
    va_list args;
    va_start(args, format);
    
    const char* s = str;
    int count = 0;
    
    while (*format && *s) {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 'd': {
                    int* p = va_arg(args, int*);
                    int val = 0;
                    while (*s >= '0' && *s <= '9') {
                        val = val * 10 + (*s - '0');
                        s++;
                    }
                    *p = val;
                    count++;
                    break;
                }
                case 's': {
                    char* p = va_arg(args, char*);
                    while (*s && *s != ' ') {
                        *p++ = *s++;
                    }
                    *p = 0;
                    count++;
                    break;
                }
            }
        } else {
            if (*format != *s) break;
            s++;
        }
        format++;
    }
    
    va_end(args);
    return count;
}

// =============================================================================
// Stdlib functions
// =============================================================================

int atoi(const char* s) {
    int result = 0;
    int sign = 1;
    
    while (*s == ' ') s++;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') s++;
    
    while (*s >= '0' && *s <= '9') {
        result = result * 10 + (*s - '0');
        s++;
    }
    
    return result * sign;
}

long atol(const char* s) {
    return (long)atoi(s);
}

char* itoa(int value, char* str, int base) {
    char* rc;
    char* ptr;
    char* low;
    
    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }
    
    rc = ptr = str;
    
    if (value < 0 && base == 10) {
        *ptr++ = '-';
        value = -value;
    }
    
    low = ptr;
    
    do {
        int rem = value % base;
        *ptr++ = (rem < 10) ? '0' + rem : 'a' + rem - 10;
        value /= base;
    } while (value);
    
    *ptr-- = '\0';
    
    while (low < ptr) {
        char tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }
    
    return rc;
}

// Simple memory allocation (kernel-managed)
static uint32_t heap_next = 0x2000000;
static uint32_t heap_max = 0x4000000;

void* malloc(size_t size) {
    size = (size + 3) & ~3;
    if (heap_next + size > heap_max) return 0;
    void* ptr = (void*)heap_next;
    heap_next += size;
    return ptr;
}

void free(void* ptr) {
    (void)ptr;
}

void* calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void* ptr = malloc(total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

void* realloc(void* ptr, size_t size) {
    if (!ptr) return malloc(size);
    void* new_ptr = malloc(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, size);
        free(ptr);
    }
    return new_ptr;
}

// =============================================================================
// Math functions
// =============================================================================

int abs(int x) {
    return x < 0 ? -x : x;
}

long labs(long x) {
    return x < 0 ? -x : x;
}

int pow(int base, int exp) {
    int result = 1;
    while (exp > 0) {
        result *= base;
        exp--;
    }
    return result;
}

int sqrt(int x) {
    if (x <= 0) return 0;
    int guess = x / 2;
    int new_guess = (guess + x / guess) / 2;
    while (new_guess < guess) {
        guess = new_guess;
        new_guess = (guess + x / guess) / 2;
    }
    return guess;
}

// =============================================================================
// ctype functions
// =============================================================================

int isdigit(int c) {
    return c >= '0' && c <= '9';
}

int isalpha(int c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

int isalnum(int c) {
    return isdigit(c) || isalpha(c);
}

int isspace(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

int isupper(int c) {
    return c >= 'A' && c <= 'Z';
}

int islower(int c) {
    return c >= 'a' && c <= 'z';
}

int toupper(int c) {
    if (islower(c)) return c - 32;
    return c;
}

int tolower(int c) {
    if (isupper(c)) return c + 32;
    return c;
}

// =============================================================================
// qsort (simple insertion sort)
// =============================================================================

void qsort(void* base, size_t nmemb, size_t size,
           int (*compar)(const void*, const void*)) {
    unsigned char* arr = (unsigned char*)base;
    
    for (size_t i = 1; i < nmemb; i++) {
        unsigned char key[size];
        memcpy(key, arr + i * size, size);
        size_t j = i;
        
        while (j > 0 && compar(arr + (j - 1) * size, key) > 0) {
            memcpy(arr + j * size, arr + (j - 1) * size, size);
            j--;
        }
        
        memcpy(arr + j * size, key, size);
    }
}

// =============================================================================
// String search
// =============================================================================

size_t strspn(const char* s, const char* accept) {
    size_t count = 0;
    while (*s) {
        const char* a = accept;
        while (*a) {
            if (*s == *a) { count++; s++; goto next; }
            a++;
        }
        break;
        next:;
    }
    return count;
}

size_t strcspn(const char* s, const char* reject) {
    size_t count = 0;
    while (*s) {
        const char* r = reject;
        while (*r) {
            if (*s == *r) return count;
            r++;
        }
        count++;
        s++;
    }
    return count;
}

char* strtok(char* str, const char* delim) {
    static char* last = 0;
    if (str) last = str;
    if (!last) return 0;
    
    // Skip leading delimiters
    last += strspn(last, delim);
    if (!*last) return 0;
    
    char* token = last;
    last += strcspn(last, delim);
    if (*last) *last++ = 0;
    
    return token;
}
