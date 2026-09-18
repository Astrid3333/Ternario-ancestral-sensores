/**
 * parental.c — Control parental para kernel ternario ancestral
 *
 * Horarios de acceso, filtros de contenido
 */

#include "../include/ternary.h"

#define PARENTAL_MAX_FILTERS 16
#define PARENTAL_MAX_HOURS   24

typedef struct {
    uint8_t  hour_start;
    uint8_t  hour_end;
    uint8_t  days[7]; // 0=Sun, 6=Sat
    uint8_t  active;
} parental_schedule_t;

typedef struct {
    char     keyword[32];
    uint8_t  active;
} parental_filter_t;

static parental_schedule_t schedules[7];
static parental_filter_t filters[PARENTAL_MAX_FILTERS];
static uint8_t n_filters = 0;
static uint8_t parental_enabled = 0;

void parental_init(void) {
    vga_puts("[PARENTAL] Parental control initialized\n");
    
    // Default: allow all hours
    for (int i = 0; i < 7; i++) {
        schedules[i].hour_start = 0;
        schedules[i].hour_end = 23;
        for (int j = 0; j < 7; j++) schedules[i].days[j] = 1;
        schedules[i].active = 1;
    }
    
    parental_enabled = 0;
}

void parental_enable(void) {
    parental_enabled = 1;
    vga_puts("[PARENTAL] Parental control enabled\n");
}

void parental_disable(void) {
    parental_enabled = 0;
    vga_puts("[PARENTAL] Parental control disabled\n");
}

void parental_set_schedule(uint8_t day, uint8_t start, uint8_t end) {
    if (day > 6) return;
    
    schedules[day].hour_start = start;
    schedules[day].hour_end = end;
    schedules[day].active = 1;
    
    vga_puts("[PARENTAL] Schedule set: day ");
    { char nb[2]; num_to_str(day, nb); vga_puts(nb); }
    vga_puts(" ");
    { char nb[4]; num_to_str(start, nb); vga_puts(nb); }
    vga_puts(":00 - ");
    { char nb[4]; num_to_str(end, nb); vga_puts(nb); }
    vga_puts(":00\n");
}

int8_t parental_add_filter(const char* keyword) {
    if (n_filters >= PARENTAL_MAX_FILTERS) {
        vga_puts("[PARENTAL] Too many filters\n");
        return -1;
    }
    
    strcpy_t(filters[n_filters].keyword, keyword);
    filters[n_filters].active = 1;
    n_filters++;
    
    vga_puts("[PARENTAL] Filter added: ");
    vga_puts(keyword);
    vga_puts("\n");
    
    return 0;
}

uint8_t parental_check_time(void) {
    if (!parental_enabled) return 1;
    
    // Simulate current time (would use RTC in real kernel)
    uint8_t hour = 12; // Noon
    uint8_t day = 3;   // Wednesday
    
    if (day <= 6 && schedules[day].active) {
        if (hour >= schedules[day].hour_start && hour <= schedules[day].hour_end) {
            return 1; // Allowed
        }
    }
    
    return 0; // Blocked
}

uint8_t parental_check_content(const char* content) {
    if (!parental_enabled) return 1;
    
    for (int i = 0; i < n_filters; i++) {
        if (filters[i].active) {
            // Check if content contains keyword
            const char* c = content;
            while (*c) {
                const char* f = filters[i].keyword;
                const char* start = c;
                uint8_t match = 1;
                
                while (*f && *c) {
                    if (*f != *c) { match = 0; break; }
                    f++; c++;
                }
                
                if (match && *f == 0) {
                    return 0; // Blocked
                }
                
                c = start + 1;
            }
        }
    }
    
    return 1; // Allowed
}

void parental_list_filters(void) {
    vga_puts("\n  Content filters:\n\n");
    
    for (int i = 0; i < n_filters; i++) {
        if (filters[i].active) {
            vga_puts("  ");
            vga_set_color(0x0C, 0);
            vga_puts("x");
            vga_set_color(0x07, 0);
            vga_puts(" ");
            vga_puts(filters[i].keyword);
            vga_puts("\n");
        }
    }
}

void parental_status(void) {
    vga_puts("\n  Parental control: ");
    if (parental_enabled) {
        vga_set_color(0x0A, 0);
        vga_puts("ENABLED");
    } else {
        vga_set_color(0x0C, 0);
        vga_puts("DISABLED");
    }
    vga_set_color(0x07, 0);
    vga_puts("\n");
    
    vga_puts("  Filters: ");
    { char nb[4]; num_to_str(n_filters, nb); vga_puts(nb); }
    vga_puts("\n");
}
