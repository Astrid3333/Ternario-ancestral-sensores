/**
 * libc.c — libc mínima para kernel ternario ancestral
 *
 * Funciones básicas de C para apps de usuario
 */

#include "../include/ternary.h"

// Freestanding va_args
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_end(ap) __builtin_va_end(ap)
#define va_arg(ap, type) __builtin_va_arg(ap, type)

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

// =============================================================================
// Stdio functions (freestanding kernel - write to VGA/serial)
// =============================================================================

// Internal put char to screen+serial
static void kernel_putchar(char c) {
    extern void vga_putc(char c);
    extern void serial_putc(char c);
    vga_putc(c);
    serial_putc(c);
}

// Internal put string
static void kernel_puts(const char* s) {
    while (*s) { kernel_putchar(*s); s++; }
}

int printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    int count = 0;
    while (*format) {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 's': {
                    const char* s = va_arg(args, const char*);
                    kernel_puts(s);
                    count += strlen(s);
                    break;
                }
                case 'd': {
                    int d = va_arg(args, int);
                    char buf[12];
                    int_to_str(d, buf);
                    kernel_puts(buf);
                    count += strlen(buf);
                    break;
                }
                case 'x': {
                    unsigned int x = va_arg(args, unsigned int);
                    char buf[12];
                    num_to_hex(x, buf);
                    kernel_puts(buf);
                    count += strlen(buf);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    kernel_putchar(c);
                    count++;
                    break;
                }
                default:
                    kernel_putchar('%');
                    kernel_putchar(*format);
                    count += 2;
            }
        } else {
            kernel_putchar(*format);
            count++;
        }
        format++;
    }
    
    va_end(args);
    return count;
}

int puts(const char* s) {
    int count = 0;
    while (*s) {
        kernel_putchar(*s);
        s++;
        count++;
    }
    kernel_putchar('\n');
    return count + 1;
}

int putchar(int c) {
    kernel_putchar((char)c);
    return c;
}

int getchar(void) {
    // Stub - would need keyboard driver integration
    return -1;
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

// Simple memory allocation
static uint32_t heap_start = 0x2000000;  // 32MB
static uint32_t heap_end = 0x2000000;
static uint32_t heap_max = 0x4000000;    // 64MB

void* malloc(size_t size) {
    // Align to 4 bytes
    size = (size + 3) & ~3;
    
    if (heap_end + size > heap_max) {
        return 0; // Out of memory
    }
    
    void* ptr = (void*)heap_end;
    heap_end += size;
    
    return ptr;
}

void free(void* ptr) {
    // Simple free - does nothing (no free list)
    // Real implementation would need free list
    (void)ptr;
}

void* realloc(void* ptr, size_t size) {
    if (!ptr) return malloc(size);
    
    // Simple realloc - allocate new, copy, free old
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
