/**
 * ai_client.c — Cliente de IA para kernel ternario ancestral
 *
 * Conectar a APIs de IA para consultas
 */

#include "../include/ternary.h"

#define AI_MAX_PROMPT  512
#define AI_MAX_RESPONSE 2048
#define AI_MAX_HISTORY  8

typedef struct {
    char     prompt[AI_MAX_PROMPT];
    char     response[AI_MAX_RESPONSE];
    uint8_t  in_use;
} ai_message_t;

static ai_message_t ai_history[AI_MAX_HISTORY];
static uint8_t n_history = 0;
static uint8_t ai_enabled = 0;

void ai_client_init(void) {
    vga_puts("[AI] AI client initialized\n");
    ai_enabled = 0;
}

void ai_enable(void) {
    ai_enabled = 1;
    vga_puts("[AI] AI client enabled\n");
}

void ai_disable(void) {
    ai_enabled = 0;
    vga_puts("[AI] AI client disabled\n");
}

// Query AI (simulated)
int8_t ai_query(const char* prompt, char* response) {
    if (!ai_enabled) {
        strcpy_t(response, "AI client is disabled");
        return -1;
    }
    
    vga_puts("[AI] Querying: ");
    vga_puts(prompt);
    vga_puts("\n");
    
    // Simulate AI response
    strcpy_t(response, "[AI Response] This is a simulated response from the Ternary AI engine. ");
    strcat_t(response, "The query was: ");
    strcat_t(response, prompt);
    
    // Store in history
    if (n_history < AI_MAX_HISTORY) {
        strcpy_t(ai_history[n_history].prompt, prompt);
        strcpy_t(ai_history[n_history].response, response);
        ai_history[n_history].in_use = 1;
        n_history++;
    }
    
    return 0;
}

// Show AI history
void ai_history_show(void) {
    vga_puts("\n  AI conversation history:\n\n");
    
    for (int i = 0; i < AI_MAX_HISTORY; i++) {
        if (ai_history[i].in_use) {
            vga_puts("  You: ");
            vga_puts(ai_history[i].prompt);
            vga_puts("\n  AI:  ");
            vga_puts(ai_history[i].response);
            vga_puts("\n\n");
        }
    }
}

// Show AI status
void ai_status(void) {
    vga_puts("\n  AI client: ");
    if (ai_enabled) {
        vga_set_color(0x0A, 0);
        vga_puts("ENABLED");
    } else {
        vga_set_color(0x0C, 0);
        vga_puts("DISABLED");
    }
    vga_set_color(0x07, 0);
    vga_puts("\n");
    
    vga_puts("  History entries: ");
    { char nb[4]; num_to_str(n_history, nb); vga_puts(nb); }
    vga_puts("\n");
}
