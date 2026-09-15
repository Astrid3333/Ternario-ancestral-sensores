/**
 * sensor_environment_ternary.c — Sensores Ambientales Acoplados
 * 
 * Simula sensores ambientales con interacciones físicas:
 * - Temperatura ↔ Humedad (acoplamiento negativo)
 * - Temperatura ↔ Presión (ley de gas ideal)
 * - Humedad ↔ Lluvia (umbral de saturación)
 * - Viento ↔ Temperatura (convección)
 * - Radiación ↔ Temperatura (calentamiento)
 * 
 * Cada variable se muestrea y codifica en ternario:
 * -1: por debajo del óptimo
 *  0: en rango óptimo
 * +1: por encima del óptimo
 * 
 * Los acoplamientos se modelan con ecuaciones diferenciales simples
 * y la representación ternaria captura la dirección del cambio.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* Ternary environmental reading */
typedef struct {
    int ternary_value;   /* -1, 0, +1 */
    float float_value;
    float optimal_min;
    float optimal_max;
    char name[32];
    char unit[16];
} env_ternary_t;

/* Environmental sensor node with coupling */
typedef struct {
    int id;
    float latitude;
    float longitude;
    float altitude;
    
    /* Primary measurements */
    env_ternary_t temperature;
    env_ternary_t humidity;
    env_ternary_t pressure;
    env_ternary_t radiation;
    env_ternary_t wind_speed;
    env_ternary_t rainfall;
    env_ternary_t co2;
    env_ternary_t uv_index;
    
    /* Coupling coefficients */
    float coupling_temp_humidity;   /* Negative: temp↑ → humidity↓ */
    float coupling_temp_pressure;   /* Positive: temp↑ → pressure↑ */
    float coupling_humidity_rain;   /* Positive: humidity↑ → rain↑ */
    float coupling_wind_temp;       /* Negative: wind↑ → temp↓ */
    float coupling_rad_temp;        /* Positive: rad↑ → temp↑ */
    
    /* State derivatives (for Euler integration) */
    float d_temp;
    float d_humidity;
    float d_pressure;
    
    time_t timestamp;
} env_node_t;

/* Environmental network */
typedef struct {
    int n_nodes;
    env_node_t* nodes;
    float global_temperature;
    float global_humidity;
    float time_step;
    int step;
    float total_coupling_energy;
} env_network_t;

/* Convert float to ternary based on optimal range */
int float_to_ternary_optimal(float value, float min, float max) {
    if (value < min) return -1;
    if (value > max) return +1;
    return 0;
}

/* Initialize environmental network */
env_network_t* env_create(int n_nodes, float area_size) {
    env_network_t* net = malloc(sizeof(env_network_t));
    if (!net) return NULL;
    
    net->n_nodes = n_nodes;
    net->nodes = calloc(n_nodes, sizeof(env_node_t));
    net->global_temperature = 25.0;
    net->global_humidity = 60.0;
    net->time_step = 0.1;
    net->step = 0;
    net->total_coupling_energy = 0;
    
    if (!net->nodes) {
        free(net);
        return NULL;
    }
    
    srand(time(NULL));
    
    for (int i = 0; i < n_nodes; i++) {
        env_node_t* node = &net->nodes[i];
        
        node->id = i;
        node->latitude = (float)rand() / RAND_MAX * area_size;
        node->longitude = (float)rand() / RAND_MAX * area_size;
        node->altitude = (float)rand() / RAND_MAX * 500;
        node->timestamp = time(NULL);
        
        /* Initialize temperature */
        strcpy(node->temperature.name, "Temperature");
        strcpy(node->temperature.unit, "°C");
        node->temperature.optimal_min = 18.0;
        node->temperature.optimal_max = 28.0;
        node->temperature.float_value = 22.0 + ((float)rand() / RAND_MAX - 0.5) * 10;
        node->temperature.ternary_value = float_to_ternary_optimal(
            node->temperature.float_value,
            node->temperature.optimal_min,
            node->temperature.optimal_max
        );
        
        /* Initialize humidity */
        strcpy(node->humidity.name, "Humidity");
        strcpy(node->humidity.unit, "%");
        node->humidity.optimal_min = 40.0;
        node->humidity.optimal_max = 70.0;
        node->humidity.float_value = 55.0 + ((float)rand() / RAND_MAX - 0.5) * 30;
        node->humidity.ternary_value = float_to_ternary_optimal(
            node->humidity.float_value,
            node->humidity.optimal_min,
            node->humidity.optimal_max
        );
        
        /* Initialize pressure */
        strcpy(node->pressure.name, "Pressure");
        strcpy(node->pressure.unit, "hPa");
        node->pressure.optimal_min = 1010.0;
        node->pressure.optimal_max = 1025.0;
        node->pressure.float_value = 1013.0 + ((float)rand() / RAND_MAX - 0.5) * 20;
        node->pressure.ternary_value = float_to_ternary_optimal(
            node->pressure.float_value,
            node->pressure.optimal_min,
            node->pressure.optimal_max
        );
        
        /* Initialize radiation */
        strcpy(node->radiation.name, "Radiation");
        strcpy(node->radiation.unit, "W/m²");
        node->radiation.optimal_min = 300.0;
        node->radiation.optimal_max = 700.0;
        node->radiation.float_value = 500.0 + ((float)rand() / RAND_MAX - 0.5) * 400;
        node->radiation.ternary_value = float_to_ternary_optimal(
            node->radiation.float_value,
            node->radiation.optimal_min,
            node->radiation.optimal_max
        );
        
        /* Initialize wind speed */
        strcpy(node->wind_speed.name, "Wind");
        strcpy(node->wind_speed.unit, "m/s");
        node->wind_speed.optimal_min = 2.0;
        node->wind_speed.optimal_max = 8.0;
        node->wind_speed.float_value = 4.0 + ((float)rand() / RAND_MAX - 0.5) * 6;
        node->wind_speed.ternary_value = float_to_ternary_optimal(
            node->wind_speed.float_value,
            node->wind_speed.optimal_min,
            node->wind_speed.optimal_max
        );
        
        /* Initialize rainfall */
        strcpy(node->rainfall.name, "Rainfall");
        strcpy(node->rainfall.unit, "mm/h");
        node->rainfall.optimal_min = 0.0;
        node->rainfall.optimal_max = 5.0;
        node->rainfall.float_value = (float)rand() / RAND_MAX * 10;
        node->rainfall.ternary_value = float_to_ternary_optimal(
            node->rainfall.float_value,
            node->rainfall.optimal_min,
            node->rainfall.optimal_max
        );
        
        /* Initialize CO2 */
        strcpy(node->co2.name, "CO2");
        strcpy(node->co2.unit, "ppm");
        node->co2.optimal_min = 350.0;
        node->co2.optimal_max = 450.0;
        node->co2.float_value = 400.0 + ((float)rand() / RAND_MAX - 0.5) * 100;
        node->co2.ternary_value = float_to_ternary_optimal(
            node->co2.float_value,
            node->co2.optimal_min,
            node->co2.optimal_max
        );
        
        /* Initialize UV index */
        strcpy(node->uv_index.name, "UV");
        strcpy(node->uv_index.unit, "idx");
        node->uv_index.optimal_min = 2.0;
        node->uv_index.optimal_max = 6.0;
        node->uv_index.float_value = 4.0 + ((float)rand() / RAND_MAX - 0.5) * 5;
        node->uv_index.ternary_value = float_to_ternary_optimal(
            node->uv_index.float_value,
            node->uv_index.optimal_min,
            node->uv_index.optimal_max
        );
        
        /* Set coupling coefficients */
        node->coupling_temp_humidity = -0.3;   /* Temp↑ → Humidity↓ */
        node->coupling_temp_pressure = 0.2;    /* Temp↑ → Pressure↑ */
        node->coupling_humidity_rain = 0.4;    /* Humidity↑ → Rain↑ */
        node->coupling_wind_temp = -0.2;       /* Wind↑ → Temp↓ */
        node->coupling_rad_temp = 0.5;         /* Radiation↑ → Temp↑ */
    }
    
    return net;
}

/* Update coupled environment */
void env_update_coupled(env_network_t* net) {
    float dt = net->time_step;
    
    for (int i = 0; i < net->n_nodes; i++) {
        env_node_t* node = &net->nodes[i];
        
        /* Calculate derivatives with coupling */
        node->d_temp = node->coupling_rad_temp * (node->radiation.float_value - 500) / 500
                     + node->coupling_wind_temp * node->wind_speed.float_value / 10
                     + ((float)rand() / RAND_MAX - 0.5) * 0.1;  /* Noise */
        
        node->d_humidity = node->coupling_temp_humidity * (node->temperature.float_value - 25) / 10
                         + node->coupling_humidity_rain * node->rainfall.float_value / 10
                         + ((float)rand() / RAND_MAX - 0.5) * 0.1;
        
        node->d_pressure = node->coupling_temp_pressure * (node->temperature.float_value - 25) / 10
                         + ((float)rand() / RAND_MAX - 0.5) * 0.05;
        
        /* Euler integration */
        node->temperature.float_value += node->d_temp * dt;
        node->humidity.float_value += node->d_humidity * dt;
        node->pressure.float_value += node->d_pressure * dt;
        
        /* Add solar forcing (diurnal cycle) */
        float hour = fmod(time(NULL) / 3600.0, 24.0);
        float solar_forcing = sin((hour - 6) / 24.0 * 2 * M_PI);
        node->radiation.float_value = 500 + 400 * solar_forcing;
        
        /* Wind forcing (random gusts) */
        node->wind_speed.float_value += ((float)rand() / RAND_MAX - 0.5) * 0.5;
        node->wind_speed.float_value = fmax(0, fmin(20, node->wind_speed.float_value));
        
        /* Rainfall (threshold model) */
        if (node->humidity.float_value > 80) {
            node->rainfall.float_value += 0.1;
        } else {
            node->rainfall.float_value *= 0.95;  /* Decay */
        }
        node->rainfall.float_value = fmax(0, fmin(20, node->rainfall.float_value));
        
        /* Update ternary values */
        node->temperature.ternary_value = float_to_ternary_optimal(
            node->temperature.float_value,
            node->temperature.optimal_min,
            node->temperature.optimal_max
        );
        
        node->humidity.ternary_value = float_to_ternary_optimal(
            node->humidity.float_value,
            node->humidity.optimal_min,
            node->humidity.optimal_max
        );
        
        node->pressure.ternary_value = float_to_ternary_optimal(
            node->pressure.float_value,
            node->pressure.optimal_min,
            node->pressure.optimal_max
        );
        
        node->radiation.ternary_value = float_to_ternary_optimal(
            node->radiation.float_value,
            node->radiation.optimal_min,
            node->radiation.optimal_max
        );
        
        node->wind_speed.ternary_value = float_to_ternary_optimal(
            node->wind_speed.float_value,
            node->wind_speed.optimal_min,
            node->wind_speed.optimal_max
        );
        
        node->rainfall.ternary_value = float_to_ternary_optimal(
            node->rainfall.float_value,
            node->rainfall.optimal_min,
            node->rainfall.optimal_max
        );
        
        node->co2.ternary_value = float_to_ternary_optimal(
            node->co2.float_value,
            node->co2.optimal_min,
            node->co2.optimal_max
        );
        
        node->uv_index.ternary_value = float_to_ternary_optimal(
            node->uv_index.float_value,
            node->uv_index.optimal_min,
            node->uv_index.optimal_max
        );
        
        node->timestamp = time(NULL);
    }
    
    net->step++;
}

/* Export network state as JSON */
void env_export_json(env_network_t* net, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "{\n");
    fprintf(f, "  \"n_nodes\": %d,\n", net->n_nodes);
    fprintf(f, "  \"step\": %d,\n", net->step);
    fprintf(f, "  \"time_step\": %.2f,\n", net->time_step);
    fprintf(f, "  \"nodes\": [\n");
    
    for (int i = 0; i < net->n_nodes; i++) {
        env_node_t* node = &net->nodes[i];
        fprintf(f, "    {\n");
        fprintf(f, "      \"id\": %d,\n", node->id);
        fprintf(f, "      \"lat\": %.4f,\n", node->latitude);
        fprintf(f, "      \"lon\": %.4f,\n", node->longitude);
        fprintf(f, "      \"temp\": {\"val\": %.2f, \"t\": %d},\n",
                node->temperature.float_value, node->temperature.ternary_value);
        fprintf(f, "      \"humidity\": {\"val\": %.2f, \"t\": %d},\n",
                node->humidity.float_value, node->humidity.ternary_value);
        fprintf(f, "      \"pressure\": {\"val\": %.2f, \"t\": %d},\n",
                node->pressure.float_value, node->pressure.ternary_value);
        fprintf(f, "      \"radiation\": {\"val\": %.2f, \"t\": %d},\n",
                node->radiation.float_value, node->radiation.ternary_value);
        fprintf(f, "      \"wind\": {\"val\": %.2f, \"t\": %d},\n",
                node->wind_speed.float_value, node->wind_speed.ternary_value);
        fprintf(f, "      \"rain\": {\"val\": %.2f, \"t\": %d},\n",
                node->rainfall.float_value, node->rainfall.ternary_value);
        fprintf(f, "      \"co2\": {\"val\": %.2f, \"t\": %d},\n",
                node->co2.float_value, node->co2.ternary_value);
        fprintf(f, "      \"uv\": {\"val\": %.2f, \"t\": %d}\n",
                node->uv_index.float_value, node->uv_index.ternary_value);
        fprintf(f, "    }");
        if (i < net->n_nodes - 1) fprintf(f, ",");
        fprintf(f, "\n");
    }
    
    fprintf(f, "  ]\n}\n");
    fclose(f);
}

/* Print network status */
void env_print_status(env_network_t* net) {
    printf("Environmental Sensor Network Status\n");
    printf("===================================\n");
    printf("Step: %d | Nodes: %d | dt: %.2f\n", net->step, net->n_nodes, net->time_step);
    printf("\n");
    
    for (int i = 0; i < net->n_nodes; i++) {
        env_node_t* node = &net->nodes[i];
        printf("Node %2d [%.1f, %.1f]:\n", node->id, node->latitude, node->longitude);
        printf("  T=%+.1f°C (%+d) H=%+.1f%% (%+d) P=%+.1fhPa (%+d)\n",
               node->temperature.float_value, node->temperature.ternary_value,
               node->humidity.float_value, node->humidity.ternary_value,
               node->pressure.float_value, node->pressure.ternary_value);
        printf("  R=%+.0fW/m² (%+d) W=%+.1fm/s (%+d) Rain=%+.1fmm/h (%+d)\n",
               node->radiation.float_value, node->radiation.ternary_value,
               node->wind_speed.float_value, node->wind_speed.ternary_value,
               node->rainfall.float_value, node->rainfall.ternary_value);
    }
}

/* Free network */
void env_destroy(env_network_t* net) {
    if (net) {
        free(net->nodes);
        free(net);
    }
}

/* CLI interface */
int env_main(int argc, char* argv[]) {
    int n_nodes = 8;
    int steps = 100;
    float area = 100.0;
    char output_file[256] = "env_output.json";
    
    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) n_nodes = atoi(argv[++i]);
        else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) steps = atoi(argv[++i]);
        else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) area = atof(argv[++i]);
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) strncpy(output_file, argv[++i], 255);
        else {
            printf("Usage: %s [-n nodes] [-s steps] [-a area] [-o output]\n", argv[0]);
            return 1;
        }
    }
    
    printf("Coupled Environmental Sensor Network (Ternary)\n");
    printf("Nodes: %d | Steps: %d | Area: %.0f×%.0f\n", n_nodes, steps, area, area);
    
    /* Create network */
    env_network_t* net = env_create(n_nodes, area);
    if (!net) {
        fprintf(stderr, "Error: Failed to create network\n");
        return 1;
    }
    
    /* Run simulation */
    for (int s = 0; s < steps; s++) {
        env_update_coupled(net);
        
        if (s % 20 == 0) {
            printf("\nStep %d:\n", s);
            env_print_status(net);
        }
    }
    
    /* Export results */
    env_export_json(net, output_file);
    printf("\nResults exported to %s\n", output_file);
    
    env_destroy(net);
    return 0;
}
