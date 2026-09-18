/**
 * md_ternary.c — Dinámica Molecular con Coordenadas Ternarias
 * 
 * Implementa simulación de dinámica molecular donde las coordenadas
 * se almacenan en sistema ternario (balanced ternary).
 * 
 * Cada coordenada (x, y, z) se representa como:
 * x = Σ d_i × 3^i, donde d_i ∈ {-1, 0, +1}
 * 
 * Ventajas:
 * - Compresión natural de coordenadas
 * - Precisión adjustable (más trits = más precisión)
 * - Operaciones aritméticas directas en ternario
 * - Ideal para sistemas de sensores distribuidos
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "science_types.h"

/* Global simulation instance for render access */
md_simulation_t* g_md_sim = NULL;

/* Convert float to ternary coordinate */
ternary_coord_t float_to_ternary(float value, int precision) {
    ternary_coord_t tc;
    tc.n_trits = precision;
    tc.trits = malloc(precision * sizeof(int));
    tc.sign = (value >= 0) ? 1 : -1;
    
    float v = fabs(value);
    
    for (int i = 0; i < precision; i++) {
        float power = pow(3, i);
        float digit = v / power;
        
        if (digit >= 0.666) {
            tc.trits[i] = 1;
            v -= power;
        } else if (digit >= 0.333) {
            tc.trits[i] = 0;
        } else {
            tc.trits[i] = -1;
            v += power;
        }
    }
    
    return tc;
}

/* Convert ternary coordinate to float */
float ternary_to_float(ternary_coord_t tc) {
    float value = 0;
    
    for (int i = 0; i < tc.n_trits; i++) {
        value += tc.trits[i] * pow(3, i);
    }
    
    return value * tc.sign;
}

/* Add two ternary coordinates */
ternary_coord_t ternary_add(ternary_coord_t a, ternary_coord_t b) {
    int max_trits = (a.n_trits > b.n_trits) ? a.n_trits : b.n_trits;
    ternary_coord_t result;
    result.n_trits = max_trits;
    result.trits = malloc(max_trits * sizeof(int));
    result.sign = 1;
    
    int carry = 0;
    for (int i = 0; i < max_trits; i++) {
        int ai = (i < a.n_trits) ? a.trits[i] : 0;
        int bi = (i < b.n_trits) ? b.trits[i] : 0;
        int sum = ai + bi + carry;
        
        if (sum > 1) {
            result.trits[i] = sum - 3;
            carry = 1;
        } else if (sum < -1) {
            result.trits[i] = sum + 3;
            carry = -1;
        } else {
            result.trits[i] = sum;
            carry = 0;
        }
    }
    
    return result;
}

/* Multiply ternary by scalar */
ternary_coord_t ternary_scale(ternary_coord_t a, float scale) {
    float value = ternary_to_float(a) * scale;
    return float_to_ternary(value, a.n_trits);
}

/* Calculate Lennard-Jones potential */
float lj_potential(float r, float epsilon, float sigma) {
    if (r < 0.1) r = 0.1;  /* Avoid division by zero */
    float sr6 = pow(sigma / r, 6);
    return 4 * epsilon * (sr6 * sr6 - sr6);
}

/* Calculate Lennard-Jones force */
float lj_force(float r, float epsilon, float sigma) {
    if (r < 0.1) r = 0.1;
    float sr6 = pow(sigma / r, 6);
    return 24 * epsilon * (2 * sr6 * sr6 - sr6) / r;
}

/* Initialize simulation */
md_simulation_t* md_create(int n_particles, float Lx, float Ly, float Lz, float T) {
    md_simulation_t* sim = malloc(sizeof(md_simulation_t));
    if (!sim) return NULL;
    
    sim->n_particles = n_particles;
    sim->Lx = Lx;
    sim->Ly = Ly;
    sim->Lz = Lz;
    sim->temperature = T;
    sim->dt = 0.005;
    sim->step = 0;
    
    sim->particles = calloc(n_particles, sizeof(md_particle_t));
    if (!sim->particles) {
        free(sim);
        return NULL;
    }
    
    /* Initialize particles with random positions and velocities */
    srand(time(NULL));
    for (int i = 0; i < n_particles; i++) {
        float x = (float)rand() / RAND_MAX * Lx;
        float y = (float)rand() / RAND_MAX * Ly;
        float z = (float)rand() / RAND_MAX * Lz;
        
        sim->particles[i].x = float_to_ternary(x, 16);
        sim->particles[i].y = float_to_ternary(y, 16);
        sim->particles[i].z = float_to_ternary(z, 16);
        
        /* Random velocities (Maxwell-Boltzmann) */
        float vx = ((float)rand() / RAND_MAX - 0.5) * sqrt(T);
        float vy = ((float)rand() / RAND_MAX - 0.5) * sqrt(T);
        float vz = ((float)rand() / RAND_MAX - 0.5) * sqrt(T);
        
        sim->particles[i].vx = float_to_ternary(vx, 16);
        sim->particles[i].vy = float_to_ternary(vy, 16);
        sim->particles[i].vz = float_to_ternary(vz, 16);
        
        sim->particles[i].mass = 1.0;
        sim->particles[i].charge = 0.0;
        sim->particles[i].type = 0;
    }
    
    return sim;
}

/* Calculate forces between particles */
void md_calc_forces(md_simulation_t* sim) {
    float epsilon = 1.0;
    float sigma = 1.0;
    float cutoff = 2.5 * sigma;
    
    /* Reset forces */
    for (int i = 0; i < sim->n_particles; i++) {
        sim->particles[i].fx = float_to_ternary(0, 16);
        sim->particles[i].fy = float_to_ternary(0, 16);
        sim->particles[i].fz = float_to_ternary(0, 16);
    }
    
    /* Calculate pairwise forces */
    for (int i = 0; i < sim->n_particles; i++) {
        for (int j = i + 1; j < sim->n_particles; j++) {
            float xi = ternary_to_float(sim->particles[i].x);
            float yi = ternary_to_float(sim->particles[i].y);
            float zi = ternary_to_float(sim->particles[i].z);
            
            float xj = ternary_to_float(sim->particles[j].x);
            float yj = ternary_to_float(sim->particles[j].y);
            float zj = ternary_to_float(sim->particles[j].z);
            
            /* Minimum image convention */
            float dx = xi - xj;
            float dy = yi - yj;
            float dz = zi - zj;
            
            dx -= sim->Lx * round(dx / sim->Lx);
            dy -= sim->Ly * round(dy / sim->Ly);
            dz -= sim->Lz * round(dz / sim->Lz);
            
            float r = sqrt(dx*dx + dy*dy + dz*dz);
            
            if (r < cutoff && r > 0.1) {
                float f = lj_force(r, epsilon, sigma);
                float fx = f * dx / r;
                float fy = f * dy / r;
                float fz = f * dz / r;
                
                /* Add forces (in ternary) */
                ternary_coord_t dfx = float_to_ternary(fx, 16);
                ternary_coord_t dfy = float_to_ternary(fy, 16);
                ternary_coord_t dfz = float_to_ternary(fz, 16);
                
                sim->particles[i].fx = ternary_add(sim->particles[i].fx, dfx);
                sim->particles[i].fy = ternary_add(sim->particles[i].fy, dfy);
                sim->particles[i].fz = ternary_add(sim->particles[i].fz, dfz);
                
                sim->particles[j].fx = ternary_add(sim->particles[j].fx, ternary_scale(dfx, -1));
                sim->particles[j].fy = ternary_add(sim->particles[j].fy, ternary_scale(dfy, -1));
                sim->particles[j].fz = ternary_add(sim->particles[j].fz, ternary_scale(dfz, -1));
            }
        }
    }
}

/* Velocity Verlet integration */
void md_step(md_simulation_t* sim) {
    float dt = sim->dt;
    float dt2 = dt * dt / 2;
    
    /* Update positions */
    for (int i = 0; i < sim->n_particles; i++) {
        float x = ternary_to_float(sim->particles[i].x);
        float y = ternary_to_float(sim->particles[i].y);
        float z = ternary_to_float(sim->particles[i].z);
        
        float vx = ternary_to_float(sim->particles[i].vx);
        float vy = ternary_to_float(sim->particles[i].vy);
        float vz = ternary_to_float(sim->particles[i].vz);
        
        float fx = ternary_to_float(sim->particles[i].fx);
        float fy = ternary_to_float(sim->particles[i].fy);
        float fz = ternary_to_float(sim->particles[i].fz);
        
        x += vx * dt + fx * dt2;
        y += vy * dt + fy * dt2;
        z += vz * dt + fz * dt2;
        
        /* Periodic boundary conditions */
        x = fmod(x + sim->Lx, sim->Lx);
        y = fmod(y + sim->Ly, sim->Ly);
        z = fmod(z + sim->Lz, sim->Lz);
        
        sim->particles[i].x = float_to_ternary(x, 16);
        sim->particles[i].y = float_to_ternary(y, 16);
        sim->particles[i].z = float_to_ternary(z, 16);
    }
    
    /* Calculate new forces */
    md_calc_forces(sim);
    
    /* Update velocities */
    for (int i = 0; i < sim->n_particles; i++) {
        float vx = ternary_to_float(sim->particles[i].vx);
        float vy = ternary_to_float(sim->particles[i].vy);
        float vz = ternary_to_float(sim->particles[i].vz);
        
        float fx = ternary_to_float(sim->particles[i].fx);
        float fy = ternary_to_float(sim->particles[i].fy);
        float fz = ternary_to_float(sim->particles[i].fz);
        
        float fx_old = ternary_to_float(sim->particles[i].fx);
        float fy_old = ternary_to_float(sim->particles[i].fy);
        float fz_old = ternary_to_float(sim->particles[i].fz);
        
        vx += (fx + fx_old) * dt / 2;
        vy += (fy + fy_old) * dt / 2;
        vz += (fz + fz_old) * dt / 2;
        
        sim->particles[i].vx = float_to_ternary(vx, 16);
        sim->particles[i].vy = float_to_ternary(vy, 16);
        sim->particles[i].vz = float_to_ternary(vz, 16);
    }
    
    sim->step++;
}

/* Calculate kinetic energy */
void md_calc_energies(md_simulation_t* sim) {
    sim->kinetic_energy = 0;
    
    for (int i = 0; i < sim->n_particles; i++) {
        float vx = ternary_to_float(sim->particles[i].vx);
        float vy = ternary_to_float(sim->particles[i].vy);
        float vz = ternary_to_float(sim->particles[i].vz);
        
        sim->kinetic_energy += 0.5 * sim->particles[i].mass * (vx*vx + vy*vy + vz*vz);
    }
    
    sim->total_energy = sim->kinetic_energy + sim->potential_energy;
}

/* Export state as JSON */
void md_export_json(md_simulation_t* sim, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "{\n");
    fprintf(f, "  \"n_particles\": %d,\n", sim->n_particles);
    fprintf(f, "  \"box\": [%.4f, %.4f, %.4f],\n", sim->Lx, sim->Ly, sim->Lz);
    fprintf(f, "  \"temperature\": %.4f,\n", sim->temperature);
    fprintf(f, "  \"step\": %d,\n", sim->step);
    fprintf(f, "  \"kinetic_energy\": %.4f,\n", sim->kinetic_energy);
    fprintf(f, "  \"potential_energy\": %.4f,\n", sim->potential_energy);
    fprintf(f, "  \"total_energy\": %.4f,\n", sim->total_energy);
    fprintf(f, "  \"particles\": [\n");
    
    for (int i = 0; i < sim->n_particles; i++) {
        fprintf(f, "    {\n");
        fprintf(f, "      \"pos\": [%.4f, %.4f, %.4f],\n",
                ternary_to_float(sim->particles[i].x),
                ternary_to_float(sim->particles[i].y),
                ternary_to_float(sim->particles[i].z));
        fprintf(f, "      \"vel\": [%.4f, %.4f, %.4f],\n",
                ternary_to_float(sim->particles[i].vx),
                ternary_to_float(sim->particles[i].vy),
                ternary_to_float(sim->particles[i].vz));
        fprintf(f, "      \"mass\": %.4f\n", sim->particles[i].mass);
        fprintf(f, "    }");
        if (i < sim->n_particles - 1) fprintf(f, ",");
        fprintf(f, "\n");
    }
    
    fprintf(f, "  ]\n}\n");
    fclose(f);
}

/* Free memory */
void md_destroy(md_simulation_t* sim) {
    if (sim) {
        for (int i = 0; i < sim->n_particles; i++) {
            free(sim->particles[i].x.trits);
            free(sim->particles[i].y.trits);
            free(sim->particles[i].z.trits);
            free(sim->particles[i].vx.trits);
            free(sim->particles[i].vy.trits);
            free(sim->particles[i].vz.trits);
            free(sim->particles[i].fx.trits);
            free(sim->particles[i].fy.trits);
            free(sim->particles[i].fz.trits);
        }
        free(sim->particles);
        free(sim);
    }
}

/* CLI interface */
int md_main(int argc, char* argv[]) {
    int n_particles = 64;
    float L = 10.0;
    float T = 1.0;
    int steps = 1000;
    char output_file[256] = "md_output.json";
    
    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) n_particles = atoi(argv[++i]);
        else if (strcmp(argv[i], "-L") == 0 && i + 1 < argc) L = atof(argv[++i]);
        else if (strcmp(argv[i], "-T") == 0 && i + 1 < argc) T = atof(argv[++i]);
        else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) steps = atoi(argv[++i]);
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) strncpy(output_file, argv[++i], 255);
        else {
            printf("Usage: %s [-n particles] [-L box_size] [-T temperature] [-s steps] [-o output]\n", argv[0]);
            return 1;
        }
    }
    
    printf("Ternary Molecular Dynamics\n");
    printf("Particles: %d | Box: %.1f | T: %.2f | Steps: %d\n", 
           n_particles, L, T, steps);
    
    /* Create simulation */
    md_simulation_t* sim = md_create(n_particles, L, L, L, T);
    if (!sim) {
        fprintf(stderr, "Error: Failed to create simulation\n");
        return 1;
    }
    
    /* Run simulation */
    for (int s = 0; s < steps; s++) {
        md_step(sim);
        md_calc_energies(sim);
        
        if (s % 100 == 0) {
            printf("Step %d: KE=%.4f PE=%.4f E=%.4f\n",
                   sim->step, sim->kinetic_energy, sim->potential_energy, sim->total_energy);
        }
    }
    
    /* Export results */
    md_export_json(sim, output_file);
    printf("Results exported to %s\n", output_file);
    
    /* Store for render access (don't destroy) */
    if (g_md_sim) md_destroy(g_md_sim);
    g_md_sim = sim;
    
    return 0;
}
