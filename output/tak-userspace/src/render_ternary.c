/**
 * render_ternary.c — Ternary Render Engine for 3D Printing
 * 
 * Genera archivos STL binarios desde los módulos científicos ternarios:
 * - LGA: heightmap desde campo de densidad
 * - Ising: voxels desde lattice de spins
 * - MD: esferas desde posiciones moleculares
 * - Sensor (agro): heightmap interpolado desde red de sensores
 * 
 * Uso: ./tritos_science render <módulo> [opciones] -o output.stl
 * 
 * Cada comando de render acepta parámetros de simulación y la ejecuta
 * antes de generar el STL (no depende de estado global entre procesos).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include "science_types.h"

/* ============================================================
 * External function declarations (from other modules)
 * ============================================================ */

extern lga_simulation_t* lga_create(int width, int height, float initial_density);
extern void lga_run(lga_simulation_t* sim, int steps);
extern void lga_destroy(lga_simulation_t* sim);

extern ising_simulation_t* ising_create(int width, int height, float T, float J, float h);
extern void ising_monte_carlo(ising_simulation_t* sim, int steps);
extern void ising_destroy(ising_simulation_t* sim);

extern md_simulation_t* md_create(int n_particles, float Lx, float Ly, float Lz, float T);
extern void md_step(md_simulation_t* sim);
extern void md_calc_energies(md_simulation_t* sim);
extern void md_destroy(md_simulation_t* sim);

extern agro_network_t* agro_create(int n_nodes, float area);
extern void agro_simulate(agro_network_t* net);
extern void agro_destroy(agro_network_t* net);

/* ============================================================
 * STL writer functions
 * ============================================================ */

void stl_write_header(FILE* f, const char* description);
long stl_write_count_placeholder(FILE* f);
void stl_update_count(FILE* f, long pos, uint32_t count);
void stl_write_triangle(FILE* f, float v1[3], float v2[3], float v3[3]);
void stl_write_cube(FILE* f, float x, float y, float z, float size);
void stl_write_sphere(FILE* f, float x, float y, float z, float radius, int segments);
void stl_write_heightmap(const char* filename, float** heights, int width, int height,
                          float x_scale, float y_scale, float z_scale,
                          float base_height, const char* description);

/* ============================================================
 * Utility functions
 * ============================================================ */

float ternary_to_float_render(int* trits, int n_trits, int sign) {
    float val = 0;
    for (int i = 0; i < n_trits; i++) {
        val += trits[i] * powf(3.0f, i);
    }
    return sign * val;
}

/* ============================================================
 * LGA Render: Heightmap from density field
 * ============================================================ */

int render_lga(int argc, char* argv[]) {
    char* output = "lga_terrain.stl";
    int width = 32, height = 16, steps = 100;
    float density = 0.3f;
    float x_scale = 1.0f, y_scale = 1.0f, z_scale = 0.1f;
    float base_height = 0.0f;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) output = argv[++i];
        else if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) width = atoi(argv[++i]);
        else if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) height = atoi(argv[++i]);
        else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) steps = atoi(argv[++i]);
        else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) density = atof(argv[++i]);
        else if (strcmp(argv[i], "-xs") == 0 && i + 1 < argc) x_scale = atof(argv[++i]);
        else if (strcmp(argv[i], "-ys") == 0 && i + 1 < argc) y_scale = atof(argv[++i]);
        else if (strcmp(argv[i], "-zs") == 0 && i + 1 < argc) z_scale = atof(argv[++i]);
        else if (strcmp(argv[i], "-base") == 0 && i + 1 < argc) base_height = atof(argv[++i]);
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("LGA Render to STL\n");
            printf("Usage: render lga [-o file.stl] [-w width] [-h height] [-s steps] [-d density]\n");
            printf("                  [-xs scale] [-ys scale] [-zs scale] [-base height]\n");
            return 0;
        }
    }
    
    printf("Running LGA simulation (%dx%d, %d steps)...\n", width, height, steps);
    lga_simulation_t* sim = lga_create(width, height, density);
    if (!sim) {
        fprintf(stderr, "Error: Failed to create LGA simulation\n");
        return 1;
    }
    
    lga_run(sim, steps);
    
    int w = sim->width;
    int h = sim->height;
    
    /* Allocate height array */
    float** heights = malloc(h * sizeof(float*));
    for (int y = 0; y < h; y++) {
        heights[y] = malloc(w * sizeof(float));
        for (int x = 0; x < w; x++) {
            int idx = y * w + x;
            float dens = sim->cells[idx].density;
            heights[y][x] = dens;
        }
    }
    
    char desc[80];
    snprintf(desc, sizeof(desc), "LGA Density Field %dx%d step %d", w, h, sim->step);
    
    stl_write_heightmap(output, heights, w, h, x_scale, y_scale, z_scale, base_height, desc);
    
    for (int y = 0; y < h; y++) free(heights[y]);
    free(heights);
    lga_destroy(sim);
    
    printf("LGA STL escrito: %s (%dx%d cells)\n", output, w, h);
    return 0;
}

/* ============================================================
 * Ising Render: Voxel cubes from spin lattice
 * ============================================================ */

int render_ising(int argc, char* argv[]) {
    char* output = "ising_lattice.stl";
    int width = 32, height = 32, steps = 1000;
    float temperature = 2.0f, coupling = 1.0f, field = 0.0f;
    float cube_size = 1.0f;
    int use_energy = 0;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) output = argv[++i];
        else if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) width = atoi(argv[++i]);
        else if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) height = atoi(argv[++i]);
        else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) steps = atoi(argv[++i]);
        else if (strcmp(argv[i], "-T") == 0 && i + 1 < argc) temperature = atof(argv[++i]);
        else if (strcmp(argv[i], "-J") == 0 && i + 1 < argc) coupling = atof(argv[++i]);
        else if (strcmp(argv[i], "-B") == 0 && i + 1 < argc) field = atof(argv[++i]);
        else if (strcmp(argv[i], "-size") == 0 && i + 1 < argc) cube_size = atof(argv[++i]);
        else if (strcmp(argv[i], "-energy") == 0) use_energy = 1;
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Ising Render to STL\n");
            printf("Usage: render ising [-o file.stl] [-w width] [-h height] [-s steps] [-T temp] [-J coupling] [-B field] [-size scale] [-energy]\n");
            return 0;
        }
    }
    
    printf("Running Ising simulation (%dx%d, %d steps, T=%.2f)...\n", width, height, steps, temperature);
    ising_simulation_t* sim = ising_create(width, height, temperature, coupling, field);
    if (!sim) {
        fprintf(stderr, "Error: Failed to create Ising simulation\n");
        return 1;
    }
    
    ising_monte_carlo(sim, steps);
    
    int w = sim->width;
    int h = sim->height;
    
    FILE* f = fopen(output, "wb");
    if (!f) {
        fprintf(stderr, "Error: No se puede crear %s\n", output);
        ising_destroy(sim);
        return 1;
    }
    
    char desc[80];
    snprintf(desc, sizeof(desc), "Ising Spin Lattice %dx%d T=%.2f", w, h, sim->temperature);
    stl_write_header(f, desc);
    long count_pos = stl_write_count_placeholder(f);
    uint32_t tri_count = 0;
    
    float spacing = cube_size * 1.1f;
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = y * w + x;
            int spin = sim->cells[idx].spin;
            
            if (spin == 0 && !use_energy) continue;
            
            float cx = x * spacing;
            float cy = y * spacing;
            float cz = use_energy ? sim->cells[idx].energy * 10.0f : spin * 0.5f;
            
            stl_write_cube(f, cx, cy, cz, cube_size);
            tri_count += 12;
        }
    }
    
    stl_update_count(f, count_pos, tri_count);
    fclose(f);
    ising_destroy(sim);
    
    printf("Ising STL escrito: %s (%d cubos, %d triángulos)\n", output, tri_count/12, tri_count);
    return 0;
}

/* ============================================================
 * MD Render: Spheres from particle positions
 * ============================================================ */

int render_md(int argc, char* argv[]) {
    char* output = "md_particles.stl";
    int n_particles = 64;
    float L = 10.0f;
    float T = 1.0f;
    int steps = 1000;
    float radius = 0.5f;
    int segments = 8;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) output = argv[++i];
        else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) n_particles = atoi(argv[++i]);
        else if (strcmp(argv[i], "-L") == 0 && i + 1 < argc) L = atof(argv[++i]);
        else if (strcmp(argv[i], "-T") == 0 && i + 1 < argc) T = atof(argv[++i]);
        else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) steps = atoi(argv[++i]);
        else if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) radius = atof(argv[++i]);
        else if (strcmp(argv[i], "-seg") == 0 && i + 1 < argc) segments = atoi(argv[++i]);
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("MD Render to STL\n");
            printf("Usage: render md [-o file.stl] [-n particles] [-L box_size] [-T temp] [-s steps] [-r radius] [-seg segments]\n");
            return 0;
        }
    }
    
    printf("Running MD simulation (%d particles, %d steps)...\n", n_particles, steps);
    md_simulation_t* sim = md_create(n_particles, L, L, L, T);
    if (!sim) {
        fprintf(stderr, "Error: Failed to create MD simulation\n");
        return 1;
    };
    
    for (int s = 0; s < steps; s++) {
        md_step(sim);
        md_calc_energies(sim);
    }
    
    printf("Simulation completed, writing STL...\n");
    fflush(stdout);
    
    FILE* f = fopen(output, "wb");
    if (!f) {
        fprintf(stderr, "Error: No se puede crear %s\n", output);
        md_destroy(sim);
        return 1;
    }
    
    char desc[80];
    snprintf(desc, sizeof(desc), "MD Particles %d step %d", sim->n_particles, sim->step);
    stl_write_header(f, desc);
    long count_pos = stl_write_count_placeholder(f);
    uint32_t tri_count = 0;
    
    for (int i = 0; i < sim->n_particles; i++) {
        md_particle_t* p = &sim->particles[i];
        
        /* Validate ternary coordinates before conversion */
        if (!p->x.trits || !p->y.trits || !p->z.trits) {
            fprintf(stderr, "Warning: Particle %d has null trits, skipping\n", i);
            continue;
        }
        if (p->x.n_trits <= 0 || p->y.n_trits <= 0 || p->z.n_trits <= 0) {
            fprintf(stderr, "Warning: Particle %d has invalid n_trits, skipping\n", i);
            continue;
        }
        
        float x = ternary_to_float_render(p->x.trits, p->x.n_trits, p->x.sign);
        float y = ternary_to_float_render(p->y.trits, p->y.n_trits, p->y.sign);
        float z = ternary_to_float_render(p->z.trits, p->z.n_trits, p->z.sign);
        
        stl_write_sphere(f, x, y, z, radius, segments);
        tri_count += 2 * segments * segments;
    }
    
    stl_update_count(f, count_pos, tri_count);
    fclose(f);
    md_destroy(sim);
    
    printf("MD STL escrito: %s (%d esferas, ~%d triángulos)\n", output, sim->n_particles, tri_count);
    return 0;
}

/* ============================================================
 * Sensor (Agro) Render: Interpolated heightmap from scattered sensors
 * ============================================================ */

float interpolate_idw(agro_node_t* nodes, int n_nodes, float tx, float ty, float power) {
    float num = 0, den = 0;
    
    for (int i = 0; i < n_nodes; i++) {
        float dx = nodes[i].longitude - tx;
        float dy = nodes[i].latitude - ty;
        float dist = sqrtf(dx*dx + dy*dy);
        
        if (dist < 1e-6) return nodes[i].temperature.raw;
        
        float weight = 1.0f / powf(dist, power);
        num += weight * nodes[i].temperature.raw;
        den += weight;
    }
    
    return (den > 0) ? num / den : 0;
}

int render_sensor(int argc, char* argv[]) {
    char* output = "sensor_map.stl";
    int n_nodes = 16;
    float area = 100.0f;
    int steps = 10;
    int grid_w = 32, grid_h = 32;
    float x_scale = 1.0f, y_scale = 1.0f, z_scale = 0.1f;
    float base_height = 0.0f;
    int sensor_channel = 0;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) output = argv[++i];
        else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) n_nodes = atoi(argv[++i]);
        else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) area = atof(argv[++i]);
        else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) steps = atoi(argv[++i]);
        else if (strcmp(argv[i], "-gw") == 0 && i + 1 < argc) grid_w = atoi(argv[++i]);
        else if (strcmp(argv[i], "-gh") == 0 && i + 1 < argc) grid_h = atoi(argv[++i]);
        else if (strcmp(argv[i], "-xs") == 0 && i + 1 < argc) x_scale = atof(argv[++i]);
        else if (strcmp(argv[i], "-ys") == 0 && i + 1 < argc) y_scale = atof(argv[++i]);
        else if (strcmp(argv[i], "-zs") == 0 && i + 1 < argc) z_scale = atof(argv[++i]);
        else if (strcmp(argv[i], "-base") == 0 && i + 1 < argc) base_height = atof(argv[++i]);
        else if (strcmp(argv[i], "-ch") == 0 && i + 1 < argc) sensor_channel = atoi(argv[++i]);
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Sensor Render to STL\n");
            printf("Usage: render sensor [-o file] [-n nodes] [-a area] [-s steps] [-gw w] [-gh h] [-xs -ys -zs scale] [-base h] [-ch 0-4]\n");
            printf("  Canales: 0=temp, 1=humidity, 2=solar, 3=pH, 4=wind\n");
            return 0;
        }
    }
    
    printf("Running Agro sensor simulation (%d nodes, %d steps)...\n", n_nodes, steps);
    agro_network_t* net = agro_create(n_nodes, area);
    if (!net) {
        fprintf(stderr, "Error: Failed to create sensor network\n");
        return 1;
    }
    
    for (int s = 0; s < steps; s++) {
        agro_simulate(net);
    }
    
    /* Find bounds */
    float min_x = 1e9, max_x = -1e9, min_y = 1e9, max_y = -1e9;
    for (int i = 0; i < net->n_nodes; i++) {
        agro_node_t* n = &net->nodes[i];
        if (n->longitude < min_x) min_x = n->longitude;
        if (n->longitude > max_x) max_x = n->longitude;
        if (n->latitude < min_y) min_y = n->latitude;
        if (n->latitude > max_y) max_y = n->latitude;
    }
    
    /* Build height grid via IDW interpolation */
    float** heights = malloc(grid_h * sizeof(float*));
    for (int y = 0; y < grid_h; y++) {
        heights[y] = malloc(grid_w * sizeof(float));
        for (int x = 0; x < grid_w; x++) {
            float tx = min_x + (max_x - min_x) * x / (grid_w - 1);
            float ty = min_y + (max_y - min_y) * y / (grid_h - 1);
            
            float val = interpolate_idw(net->nodes, net->n_nodes, tx, ty, 2.0f);
            heights[y][x] = val;
        }
    }
    
    char desc[80];
    const char* ch_names[] = {"Temperature", "Humidity", "Solar", "pH", "Wind"};
    snprintf(desc, sizeof(desc), "Sensor %s Map %dx%d", ch_names[sensor_channel], grid_w, grid_h);
    
    stl_write_heightmap(output, heights, grid_w, grid_h, x_scale, y_scale, z_scale, base_height, desc);
    
    for (int y = 0; y < grid_h; y++) free(heights[y]);
    free(heights);
    agro_destroy(net);
    
    printf("Sensor STL escrito: %s (%dx%d grid)\n", output, grid_w, grid_h);
    return 0;
}

/* ============================================================
 * Main dispatcher
 * ============================================================ */

void render_help() {
    printf("Tritos Ternary Render Engine — 3D Printing STL Generator\n");
    printf("========================================================\n\n");
    printf("Commands:\n\n");
    printf("  render lga      - LGA density field as heightmap terrain\n");
    printf("  render ising    - Ising spin lattice as voxel cubes\n");
    printf("  render md       - MD particles as sphere meshes\n");
    printf("  render sensor   - Sensor network as interpolated heightmap\n\n");
    printf("Usage: tritos_science render <command> [options] -o output.stl\n\n");
    printf("Each subcommand runs its simulation internally, then exports STL.\n");
    printf("Use '<command> -h' for specific options.\n");
}

int render_main(int argc, char* argv[]) {
    if (argc < 2) {
        render_help();
        return 0;
    }
    
    char* cmd = argv[1];
    argv++;
    argc--;
    
    if (strcmp(cmd, "lga") == 0) {
        return render_lga(argc, argv);
    } else if (strcmp(cmd, "ising") == 0) {
        return render_ising(argc, argv);
    } else if (strcmp(cmd, "md") == 0) {
        return render_md(argc, argv);
    } else if (strcmp(cmd, "sensor") == 0) {
        return render_sensor(argc, argv);
    } else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "-h") == 0) {
        render_help();
        return 0;
    } else {
        fprintf(stderr, "Unknown render command: %s\n", cmd);
        render_help();
        return 1;
    }
}