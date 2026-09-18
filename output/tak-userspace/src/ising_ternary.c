/**
 * ising_ternary.c — Modelo de Ising Ternario
 * 
 * Implementa el modelo de Ising con 3 estados de espín:
 * -1: spin down (↓)
 *  0: spin neutral (○)
 * +1: spin up (↑)
 * 
 * Ventaja sobre binario: mayor riqueza de transiciones de fase,
 * punto triple natural, y comportamiento crítico más complejo.
 * 
 * Aplicaciones:
 * - Modelado de materiales con 3 estados
 * - Sistemas de sensor con 3 niveles
 * - Compresión de datos binarios con redundancia
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "science_types.h"

/* Global simulation instance for render access */
ising_simulation_t* g_ising_sim = NULL;

/* Initialize lattice */
ising_simulation_t* ising_create(int width, int height, float T, float J, float h) {
    ising_simulation_t* sim = malloc(sizeof(ising_simulation_t));
    if (!sim) return NULL;
    
    sim->width = width;
    sim->height = height;
    sim->temperature = T;
    sim->coupling_J = J;
    sim->magnetic_field = h;
    sim->step = 0;
    sim->accepted_moves = 0;
    sim->total_moves = 0;
    
    sim->cells = calloc(width * height, sizeof(ising_cell_t));
    if (!sim->cells) {
        free(sim);
        return NULL;
    }
    
    /* Initialize spins randomly */
    srand(time(NULL));
    for (int i = 0; i < width * height; i++) {
        sim->cells[i].spin = (rand() % 3) - 1;  /* -1, 0, +1 */
        sim->cells[i].energy = 0;
    }
    
    /* Set up neighbor indices (hexagonal lattice) */
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            /* East, West, North, South, NE, SW */
            sim->cells[idx].neighbors[0] = y * width + ((x + 1) % width);
            sim->cells[idx].neighbors[1] = y * width + ((x - 1 + width) % width);
            sim->cells[idx].neighbors[2] = ((y + 1) % height) * width + x;
            sim->cells[idx].neighbors[3] = ((y - 1 + height) % height) * width + x;
            sim->cells[idx].neighbors[4] = ((y + 1) % height) * width + ((x + 1) % width);
            sim->cells[idx].neighbors[5] = ((y - 1 + height) % height) * width + ((x - 1 + width) % width);
        }
    }
    
    return sim;
}

/* Calculate local energy */
static float ising_local_energy(ising_simulation_t* sim, int idx) {
    float E = 0;
    int spin = sim->cells[idx].spin;
    
    /* Interaction with neighbors */
    for (int n = 0; n < 6; n++) {
        int neighbor_spin = sim->cells[sim->cells[idx].neighbors[n]].spin;
        E -= sim->coupling_J * spin * neighbor_spin;
    }
    
    /* Magnetic field interaction */
    E -= sim->magnetic_field * spin;
    
    return E;
}

/* Calculate total energy */
static void ising_calc_energy(ising_simulation_t* sim) {
    sim->total_energy = 0;
    for (int i = 0; i < sim->width * sim->height; i++) {
        sim->cells[i].energy = ising_local_energy(sim, i);
        sim->total_energy += sim->cells[i].energy;
    }
    sim->total_energy /= 2;  /* Each pair counted twice */
}

/* Calculate magnetization */
static void ising_calc_magnetization(ising_simulation_t* sim) {
    float M = 0;
    for (int i = 0; i < sim->width * sim->height; i++) {
        M += sim->cells[i].spin;
    }
    sim->magnetization = M / (sim->width * sim->height);
}

/* Metropolis algorithm for single spin flip */
static int ising_metropolis(ising_simulation_t* sim, int idx) {
    int old_spin = sim->cells[idx].spin;
    int new_spin = (rand() % 3) - 1;  /* -1, 0, +1 */
    
    if (old_spin == new_spin) return 0;
    
    /* Calculate energy change */
    float old_energy = ising_local_energy(sim, idx);
    
    /* Temporarily change spin */
    sim->cells[idx].spin = new_spin;
    float new_energy = ising_local_energy(sim, idx);
    
    float delta_E = new_energy - old_energy;
    
    /* Metropolis criterion */
    if (delta_E <= 0 || (float)rand() / RAND_MAX < exp(-delta_E / sim->temperature)) {
        sim->accepted_moves++;
        return 1;
    } else {
        sim->cells[idx].spin = old_spin;  /* Reject */
        return 0;
    }
}

/* Run Monte Carlo step */
void ising_monte_carlo(ising_simulation_t* sim, int sweeps) {
    int N = sim->width * sim->height;
    
    for (int s = 0; s < sweeps; s++) {
        for (int i = 0; i < N; i++) {
            int idx = rand() % N;
            ising_metropolis(sim, idx);
            sim->total_moves++;
        }
        
        /* Calculate observables */
        ising_calc_energy(sim);
        ising_calc_magnetization(sim);
        
        /* Specific heat: C = (⟨E²⟩ - ⟨E⟩²) / (kT²) */
        sim->specific_heat = 0;  /* Would need averaging over steps */
        
        /* Susceptibility: χ = (⟨M²⟩ - ⟨M⟩²) / (kT) */
        sim->susceptibility = 0;  /* Would need averaging over steps */
        
        sim->step++;
    }
}

/* Export state as JSON */
void ising_export_json(ising_simulation_t* sim, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "{\n");
    fprintf(f, "  \"width\": %d,\n", sim->width);
    fprintf(f, "  \"height\": %d,\n", sim->height);
    fprintf(f, "  \"step\": %d,\n", sim->step);
    fprintf(f, "  \"temperature\": %.4f,\n", sim->temperature);
    fprintf(f, "  \"coupling_J\": %.4f,\n", sim->coupling_J);
    fprintf(f, "  \"magnetic_field\": %.4f,\n", sim->magnetic_field);
    fprintf(f, "  \"total_energy\": %.4f,\n", sim->total_energy);
    fprintf(f, "  \"magnetization\": %.4f,\n", sim->magnetization);
    fprintf(f, "  \"acceptance_rate\": %.4f,\n", 
            sim->total_moves > 0 ? (float)sim->accepted_moves / sim->total_moves : 0);
    fprintf(f, "  \"spins\": [\n");
    
    for (int y = 0; y < sim->height; y++) {
        fprintf(f, "    [");
        for (int x = 0; x < sim->width; x++) {
            fprintf(f, "%d", sim->cells[y * sim->width + x].spin);
            if (x < sim->width - 1) fprintf(f, ",");
        }
        fprintf(f, "]");
        if (y < sim->height - 1) fprintf(f, ",");
        fprintf(f, "\n");
    }
    
    fprintf(f, "  ]\n}\n");
    fclose(f);
}

/* ASCII visualization */
void ising_print_ascii(ising_simulation_t* sim) {
    printf("Step: %d | T: %.2f | E: %.2f | M: %.4f\n",
           sim->step, sim->temperature, sim->total_energy, sim->magnetization);
    
    for (int y = 0; y < sim->height; y++) {
        for (int x = 0; x < sim->width; x++) {
            int spin = sim->cells[y * sim->width + x].spin;
            switch (spin) {
                case -1: printf("v"); break;  /* Down */
                case  0: printf("."); break;  /* Neutral */
                case +1: printf("^"); break;  /* Up */
            }
        }
        printf("\n");
    }
}

/* Temperature scan */
void ising_temperature_scan(ising_simulation_t* sim, float T_min, float T_max, int T_steps, int mc_steps) {
    float T_step = (T_max - T_min) / T_steps;
    
    printf("Temperature scan: %.2f → %.2f (%d steps)\n", T_min, T_max, T_steps);
    printf("T\tE\tM\tAccept\n");
    
    for (int t = 0; t < T_steps; t++) {
        sim->temperature = T_min + t * T_step;
        
        /* Equilibrate */
        ising_monte_carlo(sim, mc_steps / 2);
        
        /* Measure */
        ising_monte_carlo(sim, mc_steps / 2);
        
        float accept = sim->total_moves > 0 ? 
                      (float)sim->accepted_moves / sim->total_moves : 0;
        
        printf("%.3f\t%.4f\t%.4f\t%.4f\n", 
               sim->temperature, sim->total_energy, sim->magnetization, accept);
    }
}

/* Free memory */
void ising_destroy(ising_simulation_t* sim) {
    if (sim) {
        free(sim->cells);
        free(sim);
    }
}

/* CLI interface */
int ising_main(int argc, char* argv[]) {
    int width = 32;
    int height = 32;
    int steps = 1000;
    float temperature = 2.0;
    float coupling = 1.0;
    float field = 0.0;
    char output_file[256] = "ising_output.json";
    int ascii_mode = 0;
    int scan_mode = 0;
    float T_min = 0.5;
    float T_max = 4.0;
    
    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) width = atoi(argv[++i]);
        else if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) height = atoi(argv[++i]);
        else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) steps = atoi(argv[++i]);
        else if (strcmp(argv[i], "-T") == 0 && i + 1 < argc) temperature = atof(argv[++i]);
        else if (strcmp(argv[i], "-J") == 0 && i + 1 < argc) coupling = atof(argv[++i]);
        else if (strcmp(argv[i], "-B") == 0 && i + 1 < argc) field = atof(argv[++i]);
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) strncpy(output_file, argv[++i], 255);
        else if (strcmp(argv[i], "-a") == 0) ascii_mode = 1;
        else if (strcmp(argv[i], "-scan") == 0) scan_mode = 1;
        else if (strcmp(argv[i], "-Tmin") == 0 && i + 1 < argc) T_min = atof(argv[++i]);
        else if (strcmp(argv[i], "-Tmax") == 0 && i + 1 < argc) T_max = atof(argv[++i]);
        else {
            printf("Usage: %s [-w width] [-h height] [-s steps] [-T temp] [-J coupling] [-B field] [-o output] [-a] [-scan]\n", argv[0]);
            return 1;
        }
    }
    
    printf("Ternary Ising Model\n");
    printf("Grid: %dx%d | T: %.2f | J: %.2f | h: %.2f\n", 
           width, height, temperature, coupling, field);
    
    /* Create simulation */
    ising_simulation_t* sim = ising_create(width, height, temperature, coupling, field);
    if (!sim) {
        fprintf(stderr, "Error: Failed to create simulation\n");
        return 1;
    }
    
    if (ascii_mode) {
        printf("\nInitial state:\n");
        ising_print_ascii(sim);
    }
    
    if (scan_mode) {
        /* Temperature scan */
        ising_temperature_scan(sim, T_min, T_max, 20, steps / 20);
    } else {
        /* Single temperature run */
        ising_monte_carlo(sim, steps);
    }
    
    if (ascii_mode) {
        printf("\nFinal state:\n");
        ising_print_ascii(sim);
    }
    
    /* Export results */
    ising_export_json(sim, output_file);
    printf("Results exported to %s\n", output_file);
    
    /* Store for render access (don't destroy) */
    if (g_ising_sim) ising_destroy(g_ising_sim);
    g_ising_sim = sim;
    
    return 0;
}
