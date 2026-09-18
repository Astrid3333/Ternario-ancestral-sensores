/**
 * tutorials.c — Tutoriales interactivos de matemática ternaria
 *
 * Lecciones guiadas con ejemplos y ejercicios
 */

#include "../include/ternary.h"

// =============================================================================
// TUTORIAL DATA
// =============================================================================

typedef struct {
    const char* title;
    const char* content;
    const char* example;
    const char* exercise;
} tutorial_t;

static const tutorial_t tutorials[] = {
    {
        "Introducción al Ternario",
        "El sistema ternario usa 3 dígitos:\n"
        "  - (negativo uno)\n"
        "  0 (cero)\n"
        "  + (positivo uno)\n\n"
        "Ejemplos:\n"
        "  + = 1\n"
        "  +0 = 3\n"
        "  ++ = 4\n"
        "  +- = 2\n"
        "  +0- = 8",
        "math conv 5",
        "Convierte 7 a ternario"
    },
    {
        "Operaciones Básicas",
        "Suma ternaria (balanceada):\n"
        "  + + + = +0 (1+1=2)\n"
        "  + + - = 0 (1+(-1)=0)\n"
        "  - + - = +0- (8)\n\n"
        "Resta:\n"
        "  Invertir el segundo operando\n"
        "  y sumar",
        "math add 5 3",
        "Suma 4 + 2 en ternario"
    },
    {
        "Conversión de Bases",
        "Decimal → Ternario:\n"
        "  Dividir por 3 sucesivamente\n"
        "  Los residuos dan los trits\n\n"
        "Ternario → Decimal:\n"
        "  Sumar potencias de 3\n"
        "  (ti^0 + ti^1 + ti^2 + ...)",
        "math conv 255",
        "Convierte 100 a ternario"
    },
    {
        "Lógica Ternaria (Kleene)",
        "En lógica ternaria:\n"
        "  0 = falso\n"
        "  1 = desconocido\n"
        "  2 = verdadero\n\n"
        "AND:\n"
        "  0 AND 0 = 0\n"
        "  0 AND 1 = 0\n"
        "  1 AND 2 = 1\n"
        "  2 AND 2 = 2\n\n"
        "OR:\n"
        "  0 OR 0 = 0\n"
        "  0 OR 1 = 1\n"
        "  1 OR 2 = 2\n"
        "  2 OR 2 = 2",
        "logic",
        "Calcula 1 AND 2"
    },
    {
        "Aritmética Base 60 (Maya)",
        "Los Mayas usaban base 20\n"
        "Los Babilónicos usaban base 60\n\n"
        "Base 60:\n"
        "  60^0 = 1\n"
        "  60^1 = 60\n"
        "  60^2 = 3600\n\n"
        "Ejemplo:\n"
        "  120 = 2 × 60 + 0\n"
        "  = 2.0 en base 60",
        "math maya 120",
        "Convierte 3661 a base 60"
    },
    {
        "Límite de Landauer",
        "El principio de Landauer establece\n"
        "el mínimo de energía para borrar\n"
        "un bit de información:\n\n"
        "  E = kT × ln(2)\n\n"
        "Para base 3:\n"
        "  E = kT × ln(3)\n\n"
        "k = 1.38 × 10^-23 J/K\n"
        "T = 300K (ambiente)",
        "math landauer 1000",
        "Calcula energía para 1 trit"
    }
};

#define NUM_TUTORIALS (sizeof(tutorials) / sizeof(tutorial_t))

// =============================================================================
// TUTORIAL INTERFACE
// =============================================================================

static int current_tutorial = -1;

void tutorials_show(void) {
    vga_puts("\n  [Tutoriales Ternarios]\n\n");
    for (int i = 0; i < NUM_TUTORIALS; i++) {
        vga_puts("  [");
        char nb[4];
        num_to_str(i + 1, nb);
        vga_puts(nb);
        vga_puts("] ");
        vga_puts(tutorials[i].title);
        vga_puts("\n");
    }
    vga_puts("\n  Usa: tutorial <número> para ver\n");
    vga_puts("  Usa: tutorial next/prev para navegar\n\n");
}

void tutorials_show_one(int index) {
    if (index < 0 || index >= NUM_TUTORIALS) {
        vga_puts("  Tutorial no válido\n");
        return;
    }
    
    current_tutorial = index;
    const tutorial_t* t = &tutorials[index];
    
    vga_puts("\n  ════════════════════════════════════════\n");
    vga_puts("  Tutorial ");
    { char nb[4]; num_to_str(index + 1, nb); vga_puts(nb); }
    vga_puts(": ");
    vga_puts(t->title);
    vga_puts("\n  ════════════════════════════════════════\n\n");
    
    vga_puts(t->content);
    
    vga_puts("\n  ────────────────────────────────────────\n");
    vga_puts("  Ejemplo: ");
    vga_puts(t->example);
    vga_puts("\n");
    vga_puts("  Ejercicio: ");
    vga_puts(t->exercise);
    vga_puts("\n\n");
}

void tutorials_next(void) {
    if (current_tutorial < NUM_TUTORIALS - 1) {
        tutorials_show_one(current_tutorial + 1);
    } else {
        vga_puts("  Ya estás en el último tutorial\n");
    }
}

void tutorials_prev(void) {
    if (current_tutorial > 0) {
        tutorials_show_one(current_tutorial - 1);
    } else {
        vga_puts("  Ya estás en el primer tutorial\n");
    }
}

// Command: tutorial
void cmd_tutorial(const char* args) {
    if (args[0] == 0 || strcmp_t(args, "list") == 0) {
        tutorials_show();
    } else if (strcmp_t(args, "next") == 0) {
        tutorials_next();
    } else if (strcmp_t(args, "prev") == 0) {
        tutorials_prev();
    } else {
        // Parse number
        int n = 0;
        for (int i = 0; args[i] >= '0' && args[i] <= '9'; i++) {
            n = n * 10 + (args[i] - '0');
        }
        if (n > 0 && n <= NUM_TUTORIALS) {
            tutorials_show_one(n - 1);
        } else {
            vga_puts("  Uso: tutorial [número|list|next|prev]\n");
        }
    }
}
