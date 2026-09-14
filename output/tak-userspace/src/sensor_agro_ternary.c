/**
 * sensor_agro_ternary.c — Red de Sensores Agronómicos Ternarios
 * 
 * Simula una red de sensores para agricultura de precisión:
 * - Temperatura del aire
 * - Humedad del suelo
 - Radiación solar
 * - pH del suelo
 * - Viento (velocidad y dirección)
 * 
 * Cada valor se almacena en ternario balanced:
 * -1: bajo/optimo_bajo
 *  0: optimal
 * +1: alto/optimo_alto
 * 
 * Compresión natural: 3 estados por sensor = 1.58 bits (vs 8 bits binario)
 * Ahorro: ~80% en transmisión de datos
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "science_types.h"

/* Global simulation instance for render access */
agro_network_t* g_agro_net = NULL;

/* Convert float to ternary based on thresholds */
int float_to_ternary_sensor(float value, float min_thresh, float max_thresh) {
    if (value < min_thresh) return -1;
    if (value > max_thresh) return +1;
    return 0;
}

/* Create sensor network */
agro_network_t* agro_create(int n_nodes, float area_size) {
    agro_network_t* net = malloc(sizeof(agro_network_t));
    if (!net) return NULL;
    
    net->n_nodes = n_nodes;
    net->nodes = calloc(n_nodes, sizeof(agro_node_t));
    net->network_energy = 100.0;
    net->total_transmissions = 0;
    net->compressed_bytes = 0;
    net->original_bytes = 0;
    
    if (!net->nodes) {
        free(net);
        return NULL;
    }
    
    srand(time(NULL));
    
    for (int i = 0; i < n_nodes; i++) {
        net->nodes[i].id = i;
        net->nodes[i].latitude = (float)rand() / RAND_MAX * area_size;
        net->nodes[i].longitude = (float)rand() / RAND_MAX * area_size;
        net->nodes[i].altitude = (float)rand() / RAND_MAX * 100;
        net->nodes[i].battery_level = 80 + rand() % 20;
        net->nodes[i].signal_strength = 70 + rand() % 30;
        net->nodes[i].timestamp = time(NULL);
        
        /* Initialize sensor thresholds */
        strcpy(net->nodes[i].temperature.name, "Temperature");
        strcpy(net->nodes[i].temperature.unit, "°C");
        net->nodes[i].temperature.min_threshold = 15.0;
        net->nodes[i].temperature.max_threshold = 30.0;
        
        strcpy(net->nodes[i].humidity.name, "Humidity");
        strcpy(net->nodes[i].humidity.unit, "%");
        net->nodes[i].humidity.min_threshold = 40.0;
        net->nodes[i].humidity.max_threshold = 70.0;
        
        strcpy(net->nodes[i].solar.name, "Solar");
        strcpy(net->nodes[i].solar.unit, "W/m²");
        net->nodes[i].solar.min_threshold = 300.0;
        net->nodes[i].solar.max_threshold = 700.0;
        
        strcpy(net->nodes[i].ph.name, "pH");
        strcpy(net->nodes[i].ph.unit, "");
        net->nodes[i].ph.min_threshold = 6.0;
        net->nodes[i].ph.max_threshold = 7.5;
        
        strcpy(net->nodes[i].wind_speed.name, "Wind");
        strcpy(net->nodes[i].wind_speed.unit, "m/s");
        net->nodes[i].wind_speed.min_threshold = 2.0;
        net->nodes[i].wind_speed.max_threshold = 8.0;
    }
    
    return net;
}

/* Simulate sensor readings */
void agro_simulate(agro_network_t* net) {
    for (int i = 0; i < net->n_nodes; i++) {
        agro_node_t* node = &net->nodes[i];
        
        /* Temperature: 10-40°C with daily cycle */
        float hour_factor = sin(time(NULL) % 86400 / 86400.0 * 2 * M_PI);
        node->temperature.raw = 25.0 + 10.0 * hour_factor + ((float)rand() / RAND_MAX - 0.5) * 5;
        node->temperature.value = float_to_ternary_sensor(
            node->temperature.raw,
            node->temperature.min_threshold,
            node->temperature.max_threshold
        );
        
        /* Humidity: 20-90% */
        node->humidity.raw = 60.0 + 30.0 * sin(time(NULL) % 3600 / 3600.0 * M_PI) 
                           + ((float)rand() / RAND_MAX - 0.5) * 20;
        node->humidity.raw = fmax(20, fmin(90, node->humidity.raw));
        node->humidity.value = float_to_ternary_sensor(
            node->humidity.raw,
            node->humidity.min_threshold,
            node->humidity.max_threshold
        );
        
        /* Solar: 0-1000 W/m² */
        node->solar.raw = 500.0 + 400.0 * hour_factor + ((float)rand() / RAND_MAX - 0.5) * 100;
        node->solar.raw = fmax(0, fmin(1000, node->solar.raw));
        node->solar.value = float_to_ternary_sensor(
            node->solar.raw,
            node->solar.min_threshold,
            node->solar.max_threshold
        );
        
        /* pH: 4.5-8.5 */
        node->ph.raw = 6.5 + ((float)rand() / RAND_MAX - 0.5) * 2;
        node->ph.value = float_to_ternary_sensor(
            node->ph.raw,
            node->ph.min_threshold,
            node->ph.max_threshold
        );
        
        /* Wind: 0-20 m/s */
        node->wind_speed.raw = 5.0 + ((float)rand() / RAND_MAX - 0.5) * 10;
        node->wind_speed.raw = fmax(0, fmin(20, node->wind_speed.raw));
        node->wind_speed.value = float_to_ternary_sensor(
            node->wind_speed.raw,
            node->wind_speed.min_threshold,
            node->wind_speed.max_threshold
        );
        
        node->wind_direction = rand() % 12;
        node->timestamp = time(NULL);
        node->battery_level = fmax(0, node->battery_level - (rand() % 3));
    }
}

/* Compress network data to ternary */
int agro_compress_ternary(agro_network_t* net, int* output) {
    int idx = 0;
    
    for (int i = 0; i < net->n_nodes; i++) {
        agro_node_t* node = &net->nodes[i];
        
        /* Pack 5 sensors per node into ternary words */
        /* Each sensor is 2 trits (can represent -1, 0, +1) */
        /* 5 sensors = 10 trits = 2 ternary words (5 trits each) */
        
        int word1 = 0;
        word1 |= (node->temperature.value + 1) << 0;
        word1 |= (node->humidity.value + 1) << 2;
        word1 |= (node->solar.value + 1) << 4;
        word1 |= (node->ph.value + 1) << 6;
        word1 |= (node->wind_speed.value + 1) << 8;
        
        int word2 = node->wind_direction;
        
        output[idx++] = word1;
        output[idx++] = word2;
        
        net->compressed_bytes += 8;  /* 2 words × 4 bytes */
        net->original_bytes += 20;   /* 5 sensors × 4 bytes */
    }
    
    net->total_transmissions++;
    return idx;
}

/* Decompress ternary data */
void agro_decompress_ternary(agro_network_t* net, int* input, int n_words) {
    int idx = 0;
    
    for (int i = 0; i < net->n_nodes && idx < n_words; i++) {
        agro_node_t* node = &net->nodes[i];
        
        int word1 = input[idx++];
        int word2 = input[idx++];
        
        node->temperature.value = ((word1 >> 0) & 0x3) - 1;
        node->humidity.value = ((word1 >> 2) & 0x3) - 1;
        node->solar.value = ((word1 >> 4) & 0x3) - 1;
        node->ph.value = ((word1 >> 6) & 0x3) - 1;
        node->wind_speed.value = ((word1 >> 8) & 0x3) - 1;
        
        node->wind_direction = word2 & 0xF;
    }
}

/* Export network state as JSON */
void agro_export_json(agro_network_t* net, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "{\n");
    fprintf(f, "  \"n_nodes\": %d,\n", net->n_nodes);
    fprintf(f, "  \"network_energy\": %.2f,\n", net->network_energy);
    fprintf(f, "  \"total_transmissions\": %d,\n", net->total_transmissions);
    fprintf(f, "  \"compression_ratio\": %.2f,\n",
            net->original_bytes > 0 ? (float)net->compressed_bytes / net->original_bytes : 0);
    fprintf(f, "  \"nodes\": [\n");
    
    for (int i = 0; i < net->n_nodes; i++) {
        agro_node_t* node = &net->nodes[i];
        fprintf(f, "    {\n");
        fprintf(f, "      \"id\": %d,\n", node->id);
        fprintf(f, "      \"lat\": %.4f,\n", node->latitude);
        fprintf(f, "      \"lon\": %.4f,\n", node->longitude);
        fprintf(f, "      \"temp\": {\"raw\": %.2f, \"ternary\": %d},\n",
                node->temperature.raw, node->temperature.value);
        fprintf(f, "      \"humidity\": {\"raw\": %.2f, \"ternary\": %d},\n",
                node->humidity.raw, node->humidity.value);
        fprintf(f, "      \"solar\": {\"raw\": %.2f, \"ternary\": %d},\n",
                node->solar.raw, node->solar.value);
        fprintf(f, "      \"ph\": {\"raw\": %.2f, \"ternary\": %d},\n",
                node->ph.raw, node->ph.value);
        fprintf(f, "      \"wind\": {\"raw\": %.2f, \"ternary\": %d, \"dir\": %d},\n",
                node->wind_speed.raw, node->wind_speed.value, node->wind_direction);
        fprintf(f, "      \"battery\": %d,\n", node->battery_level);
        fprintf(f, "      \"signal\": %d\n", node->signal_strength);
        fprintf(f, "    }");
        if (i < net->n_nodes - 1) fprintf(f, ",");
        fprintf(f, "\n");
    }
    
    fprintf(f, "  ]\n}\n");
    fclose(f);
}

/* Print network status */
void agro_print_status(agro_network_t* net) {
    printf("Agronomic Sensor Network Status\n");
    printf("================================\n");
    printf("Nodes: %d\n", net->n_nodes);
    printf("Energy: %.1f%%\n", net->network_energy);
    printf("Transmissions: %d\n", net->total_transmissions);
    printf("Compression: %.1f%% (%d → %d bytes)\n",
           net->original_bytes > 0 ? 100.0 * net->compressed_bytes / net->original_bytes : 0,
           net->original_bytes, net->compressed_bytes);
    printf("\n");
    
    for (int i = 0; i < net->n_nodes; i++) {
        agro_node_t* node = &net->nodes[i];
        printf("Node %2d [%.1f, %.1f]: T=%+.1f°C H=%+.1f%% S=%+.0fW/m² pH=%+.1f W=%+.1fm/s Dir=%d Battery=%d%%\n",
               node->id, node->latitude, node->longitude,
               node->temperature.raw, node->humidity.raw,
               node->solar.raw, node->ph.raw,
               node->wind_speed.raw, node->wind_direction,
               node->battery_level);
    }
}

/* Free network */
void agro_destroy(agro_network_t* net) {
    if (net) {
        free(net->nodes);
        free(net);
    }
}

/* CLI interface */
int agro_main(int argc, char* argv[]) {
    int n_nodes = 16;
    float area = 100.0;
    int steps = 10;
    char output_file[256] = "agro_output.json";
    
    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) n_nodes = atoi(argv[++i]);
        else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) area = atof(argv[++i]);
        else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) steps = atoi(argv[++i]);
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) strncpy(output_file, argv[++i], 255);
        else {
            printf("Usage: %s [-n nodes] [-a area] [-s steps] [-o output]\n", argv[0]);
            return 1;
        }
    }
    
    printf("Agronomic Sensor Network (Ternary)\n");
    printf("Nodes: %d | Area: %.0f×%.0f m | Steps: %d\n", n_nodes, area, area, steps);
    
    /* Create network */
    agro_network_t* net = agro_create(n_nodes, area);
    if (!net) {
        fprintf(stderr, "Error: Failed to create network\n");
        return 1;
    }
    
    /* Run simulation */
    for (int s = 0; s < steps; s++) {
        agro_simulate(net);
        
        int* compressed = malloc(n_nodes * 2 * sizeof(int));
        agro_compress_ternary(net, compressed);
        
        if (s == 0 || s == steps - 1) {
            printf("\nStep %d:\n", s);
            agro_print_status(net);
        }
        
        free(compressed);
    }
    
    /* Export results */
    agro_export_json(net, output_file);
    printf("\nResults exported to %s\n", output_file);
    
    /* Store for render access (don't destroy) */
    if (g_agro_net) agro_destroy(g_agro_net);
    g_agro_net = net;
    
    return 0;
}
