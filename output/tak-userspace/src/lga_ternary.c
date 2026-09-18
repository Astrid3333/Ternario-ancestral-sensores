/**
 * lga_ternary.c — Lattice Gas Automata en Sistema Ternario
 * 
 * Implementa un fluido donde cada nodo tiene 3 estados:
 * -1: vacío (sin partícula)
 *  0: partícula lenta (masa baja)
 * +1: partícula rápida (masa alta)
 * 
 * Propagación en lattice hexagonal 2D con colisiones ternarias.
 * Ventaja sobre binario: mayor riqueza dinámica, viscosidad ajustable,
 * y compresión natural de datos de sensores.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* Ternary direction vectors (hexagonal lattice) */
typedef struct {
    int dx;
    int dy;
    int state;  /* -1, 0, +1 */
} trit_dir_t;

/* Directions: 6 neighbors in hexagonal lattice */
static const trit_dir_t DIRECTIONS[6] = {
    { 1,  0, +1},   /* East */
    {-1,  0, -1},   /* West */
    { 0,  1,  0},   /* North */
    { 0, -1,  0},   /* South */
    { 1,  1, +1},   /* NE */
    {-1, -1, -1}    /* SW */
};

/* Ternary collision rules */
typedef struct {
    int input[3];   /* 3 input states */
    int output[3];  /* 3 output states */
    int probability; /* collision probability (0-100) */
} ternary_collision_t;

/* Collision table - conservative ternary rules */
static const ternary_collision_t COLLISIONS[] = {
    /* Empty collisions - particles slow down */
    {{-1, -1, +1}, {-1,  0,  0}, 70},  /* Fast + empty + empty → slow */
    {{+1, -1, -1}, { 0, -1, -1}, 70},  /* Fast + empty + empty → slow */
    
    /* Head-on collisions - scatter */
    {{+1, -1,  0}, { 0,  0, +1}, 50},  /* Fast + empty + slow → scatter */
    {{+1,  0, -1}, { 0, +1, -1}, 50},  /* Fast + slow + empty → scatter */
    
    /* Three-way collisions - complex */
    {{+1,  0,  0}, { 0, +1, +1}, 30},  /* Fast + slow + slow → two fast */
    {{-1, +1, +1}, {-1, -1, +1}, 40},  /* Empty + fast + fast → slow down */
    {{ 0,  0,  0}, { 0,  0,  0}, 100}, /* All empty → stay empty */
    {{+1, +1, +1}, {+1, +1, +1}, 100}, /* All fast → stay fast */
};

#define NUM_COLLISIONS (sizeof(COLLISIONS) / sizeof(COLLISIONS[0]))

#include "science_types.h"

/* Global simulation instance for render access */
lga_simulation_t* g_lga_sim = NULL;

/* Initialize simulation */
lga_simulation_t* lga_create(int width, int height, float initial_density) {
    lga_simulation_t* sim = malloc(sizeof(lga_simulation_t));
    if (!sim) return NULL;
    
    sim->width = width;
    sim->height = height;
    sim->step = 0;
    sim->viscosity = 0.5;
    sim->total_energy = 0;
    sim->total_momentum = 0;
    
    sim->cells = calloc(width * height, sizeof(lga_cell_t));
    sim->buffer = calloc(width * height, sizeof(lga_cell_t));
    
    if (!sim->cells || !sim->buffer) {
        free(sim->cells);
        free(sim->buffer);
        free(sim);
        return NULL;
    }
    
    /* Initialize with random ternary states */
    srand(time(NULL));
    for (int i = 0; i < width * height; i++) {
        float r = (float)rand() / RAND_MAX;
        if (r < (1.0 - initial_density) / 2) {
            sim->cells[i].state = -1;  /* Empty */
        } else if (r < 0.5 + initial_density / 2) {
            sim->cells[i].state = 0;   /* Slow */
        } else {
            sim->cells[i].state = +1;  /* Fast */
        }
        sim->cells[i].next_state = sim->cells[i].state;
        sim->cells[i].density = 0;
        sim->cells[i].velocity_x = 0;
        sim->cells[i].velocity_y = 0;
    }
    
    return sim;
}

/* Get cell at position (with periodic boundary) */
static lga_cell_t* lga_get_cell(lga_simulation_t* sim, int x, int y) {
    x = (x % sim->width + sim->width) % sim->width;
    y = (y % sim->height + sim->height) % sim->height;
    return &sim->cells[y * sim->width + x];
}

/* Apply ternary collision rules */
static void lga_collide(lga_cell_t* cells, int x, int y, int width, int height) {
    int idx = y * width + x;
    int input[3];
    int output[3];
    
    /* Collect 3 local states */
    input[0] = cells[idx].state;
    input[1] = cells[(y * width + (x + 1) % width)].state;
    input[2] = cells[(((y + 1) % height) * width + x)].state;
    
    /* Find matching collision rule */
    for (int c = 0; c < NUM_COLLISIONS; c++) {
        int match = 1;
        for (int k = 0; k < 3; k++) {
            if (COLLISIONS[c].input[k] != input[k]) {
                match = 0;
                break;
            }
        }
        
        if (match && (rand() % 100) < COLLISIONS[c].probability) {
            /* Apply collision */
            cells[idx].state = COLLISIONS[c].output[0];
            cells[(y * width + (x + 1) % width)].state = COLLISIONS[c].output[1];
            cells[(((y + 1) % height) * width + x)].state = COLLISIONS[c].output[2];
            break;
        }
    }
}

/* Propagate particles to neighbors */
static void lga_propagate(lga_simulation_t* sim) {
    for (int y = 0; y < sim->height; y++) {
        for (int x = 0; x < sim->width; x++) {
            int src_idx = y * sim->width + x;
            
            /* Copy current state to buffer */
            sim->buffer[src_idx].state = sim->cells[src_idx].state;
            sim->buffer[src_idx].density = 0;
            
            /* Propagate to each direction based on state */
            for (int d = 0; d < 6; d++) {
                int nx = x + DIRECTIONS[d].dx;
                int ny = y + DIRECTIONS[d].dy;
                nx = (nx % sim->width + sim->width) % sim->width;
                ny = (ny % sim->height + sim->height) % sim->height;
                
                int dst_idx = ny * sim->width + nx;
                
                /* Propagation rule: state affects which direction to move */
                if (sim->cells[src_idx].state == DIRECTIONS[d].state) {
                    sim->buffer[dst_idx].density += abs(sim->cells[src_idx].state);
                }
            }
        }
    }
    
    /* Swap buffers */
    lga_cell_t* temp = sim->cells;
    sim->cells = sim->buffer;
    sim->buffer = temp;
}

/* Calculate macroscopic quantities */
static void lga_calc_macroscopic(lga_simulation_t* sim) {
    sim->total_energy = 0;
    sim->total_momentum = 0;
    sim->density_avg = 0;
    
    for (int i = 0; i < sim->width * sim->height; i++) {
        float state_f = (float)sim->cells[i].state;
        sim->total_energy += state_f * state_f;
        sim->total_momentum += state_f;
        sim->density_avg += sim->cells[i].density;
    }
    
    sim->density_avg /= (sim->width * sim->height);
}

/* Run simulation for N steps */
void lga_run(lga_simulation_t* sim, int steps) {
    for (int s = 0; s < steps; s++) {
        /* Collision phase */
        for (int y = 0; y < sim->height; y++) {
            for (int x = 0; x < sim->width; x++) {
                lga_collide(sim->cells, x, y, sim->width, sim->height);
            }
        }
        
        /* Propagation phase */
        lga_propagate(sim);
        
        /* Calculate macroscopic quantities */
        lga_calc_macroscopic(sim);
        
        sim->step++;
    }
}

/* Export simulation state as JSON */
void lga_export_json(lga_simulation_t* sim, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "{\n");
    fprintf(f, "  \"width\": %d,\n", sim->width);
    fprintf(f, "  \"height\": %d,\n", sim->height);
    fprintf(f, "  \"step\": %d,\n", sim->step);
    fprintf(f, "  \"total_energy\": %.4f,\n", sim->total_energy);
    fprintf(f, "  \"total_momentum\": %.4f,\n", sim->total_momentum);
    fprintf(f, "  \"density_avg\": %.4f,\n", sim->density_avg);
    fprintf(f, "  \"viscosity\": %.4f,\n", sim->viscosity);
    fprintf(f, "  \"grid\": [\n");
    
    for (int y = 0; y < sim->height; y++) {
        fprintf(f, "    [");
        for (int x = 0; x < sim->width; x++) {
            int idx = y * sim->width + x;
            fprintf(f, "%d", sim->cells[idx].state);
            if (x < sim->width - 1) fprintf(f, ",");
        }
        fprintf(f, "]");
        if (y < sim->height - 1) fprintf(f, ",");
        fprintf(f, "\n");
    }
    
    fprintf(f, "  ]\n}\n");
    fclose(f);
}

/* Export as ASCII art */
void lga_print_ascii(lga_simulation_t* sim) {
    printf("Step: %d | Energy: %.2f | Momentum: %.2f | Density: %.4f\n",
           sim->step, sim->total_energy, sim->total_momentum, sim->density_avg);
    
    for (int y = 0; y < sim->height; y++) {
        for (int x = 0; x < sim->width; x++) {
            int idx = y * sim->width + x;
            switch (sim->cells[idx].state) {
                case -1: printf("."); break;  /* Empty */
                case  0: printf("o"); break;  /* Slow */
                case +1: printf("O"); break;  /* Fast */
                default: printf("?"); break;
            }
        }
        printf("\n");
    }
}

/* Free simulation */
void lga_destroy(lga_simulation_t* sim) {
    if (sim) {
        free(sim->cells);
        free(sim->buffer);
        free(sim);
    }
}

/* CLI interface */
int lga_main(int argc, char* argv[]) {
    int width = 32;
    int height = 16;
    int steps = 100;
    float density = 0.3;
    char output_file[256] = "lga_output.json";
    int ascii_mode = 0;
    
    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) width = atoi(argv[++i]);
        else if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) height = atoi(argv[++i]);
        else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) steps = atoi(argv[++i]);
        else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) density = atof(argv[++i]);
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) strncpy(output_file, argv[++i], 255);
        else if (strcmp(argv[i], "-a") == 0) ascii_mode = 1;
        else {
            printf("Usage: %s [-w width] [-h height] [-s steps] [-d density] [-o output] [-a]\n", argv[0]);
            return 1;
        }
    }
    
    printf("Ternary Lattice Gas Automata\n");
    printf("Grid: %dx%d | Steps: %d | Density: %.2f\n", width, height, steps, density);
    
    /* Create and run simulation */
    lga_simulation_t* sim = lga_create(width, height, density);
    if (!sim) {
        fprintf(stderr, "Error: Failed to create simulation\n");
        return 1;
    }
    
    if (ascii_mode) {
        lga_print_ascii(sim);
        printf("\nRunning %d steps...\n", steps);
    }
    
    lga_run(sim, steps);
    
    if (ascii_mode) {
        printf("\nFinal state:\n");
        lga_print_ascii(sim);
    }
    
    /* Export results */
    lga_export_json(sim, output_file);
    printf("Results exported to %s\n", output_file);
    
    /* Store for render access (don't destroy) */
    if (g_lga_sim) lga_destroy(g_lga_sim);
    g_lga_sim = sim;
    
    return 0;
}
