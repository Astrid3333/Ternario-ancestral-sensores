/**
 * science_types.h — Common type definitions for Tritos Science Suite
 * 
 * This header MUST be included by all science modules and render_ternary.c
 * to ensure consistent struct layouts across compilation units.
 */

#ifndef SCIENCE_TYPES_H
#define SCIENCE_TYPES_H

#include <stdint.h>

/* ============================================================
 * LGA (Lattice Gas Automata)
 * ============================================================ */

typedef struct {
    int state;          /* -1, 0, +1 */
    int next_state;
    int density;
    float velocity_x, velocity_y;
} lga_cell_t;

typedef struct {
    int width, height;
    lga_cell_t* cells;
    lga_cell_t* buffer;  /* Double buffering */
    int step;
    float total_energy;
    float total_momentum;
    float viscosity;
    float density_avg;
} lga_simulation_t;

/* ============================================================
 * Ising Model
 * ============================================================ */

typedef struct {
    int spin;           /* -1, 0, +1 */
    int neighbors[6];
    float energy;
} ising_cell_t;

typedef struct {
    int width, height;
    ising_cell_t* cells;
    float temperature;
    float coupling_J;
    float magnetic_field;
    float total_energy;
    float magnetization;
    float specific_heat;
    float susceptibility;
    int step;
    long accepted_moves;
    long total_moves;
} ising_simulation_t;

/* ============================================================
 * Molecular Dynamics (Ternary Coordinates)
 * ============================================================ */

typedef struct {
    int* trits;         /* Array of trits: -1, 0, +1 */
    int n_trits;        /* Number of trits */
    int sign;           /* +1 or -1 */
} ternary_coord_t;

/* Ternary sensor value */
typedef struct {
    int value;          /* -1, 0, +1 */
    float raw;          /* Original float value */
    float min_threshold;
    float max_threshold;
    char unit[16];
    char name[32];
} ternary_sensor_t;

typedef struct {
    ternary_coord_t x, y, z;
    ternary_coord_t vx, vy, vz;
    ternary_coord_t fx, fy, fz;
    float mass;
    float charge;
    int type;
} md_particle_t;

typedef struct {
    float Lx, Ly, Lz;
    int n_particles;
    md_particle_t* particles;
    int step;
    float temperature;
    float dt;
    float kinetic_energy;
    float potential_energy;
    float total_energy;
    float pressure;
} md_simulation_t;

/* ============================================================
 * Agronomic Sensor Network
 * ============================================================ */

typedef struct {
    int id;
    float latitude, longitude, altitude;
    ternary_sensor_t temperature;
    ternary_sensor_t humidity;
    ternary_sensor_t solar;
    ternary_sensor_t ph;
    ternary_sensor_t wind_speed;
    int wind_direction;
    time_t timestamp;
    int battery_level;
    int signal_strength;
} agro_node_t;

typedef struct {
    int n_nodes;
    agro_node_t* nodes;
    float network_energy;
    float total_compressed;
    float compression_ratio;
    int total_transmissions;
    int compressed_bytes;
    int original_bytes;
} agro_network_t;

/* ============================================================
 * Seismic Sensor Network
 * ============================================================ */

typedef struct {
    int id;
    float latitude, longitude, depth;
    float magnitude;
    float p_wave_arrival;
    float s_wave_arrival;
} seismic_node_t;

typedef struct {
    int n_nodes;
    seismic_node_t* nodes;
} seismic_network_t;

/* ============================================================
 * Environmental Sensor Network
 * ============================================================ */

typedef struct {
    int id;
    float latitude, longitude, altitude;
    float temperature;
    float humidity;
    float radiation;
    float wind_speed;
    float wind_direction;
} env_node_t;

typedef struct {
    int n_nodes;
    env_node_t* nodes;
} env_network_t;

#endif /* SCIENCE_TYPES_H */