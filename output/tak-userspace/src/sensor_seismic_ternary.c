/**
 * sensor_seismic_ternary.c — Monitoreo Sísmico Virtual Ternario
 * 
 * Simula una red de sensores sísmicos con representación ternaria:
 * - Ondas P (primarias): compresión
 * - Ondas S (secundarias): cizalla
 * - Ondas superficie: Rayleigh y Love
 * 
 * Cada componente de onda se almacena en ternario:
 * -1: compresión negativa
 *  0: equilibrio
 * +1: compresión positiva
 * 
 * Ventaja: compresión natural de señales sísmicas (alta redundancia)
 * Aplicación: detección temprana de terremotos en IoT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* Ternary signal sample */
typedef struct {
    int p_wave;         /* Primary wave: -1, 0, +1 */
    int s_wave;         /* Secondary wave: -1, 0, +1 */
    int surface_wave;   /* Surface wave: -1, 0, +1 */
    float amplitude;
    float frequency;
    time_t timestamp;
} seismic_sample_t;

/* Seismic sensor node */
typedef struct {
    int id;
    float latitude;
    float longitude;
    float depth;
    seismic_sample_t* samples;
    int n_samples;
    int max_samples;
    float noise_level;
    float sensitivity;
} seismic_node_t;

/* Seismic event */
typedef struct {
    float magnitude;
    float depth;
    float latitude;
    float longitude;
    time_t origin_time;
    float p_velocity;
    float s_velocity;
    int intensity;
} seismic_event_t;

/* Seismic network */
typedef struct {
    int n_nodes;
    seismic_node_t* nodes;
    int n_events;
    seismic_event_t* events;
    float detection_threshold;
    int total_samples;
    int compressed_samples;
} seismic_network_t;

/* Initialize seismic network */
seismic_network_t* seismic_create(int n_nodes, float area_size) {
    seismic_network_t* net = malloc(sizeof(seismic_network_t));
    if (!net) return NULL;
    
    net->n_nodes = n_nodes;
    net->nodes = calloc(n_nodes, sizeof(seismic_node_t));
    net->n_events = 0;
    net->events = NULL;
    net->detection_threshold = 0.5;
    net->total_samples = 0;
    net->compressed_samples = 0;
    
    if (!net->nodes) {
        free(net);
        return NULL;
    }
    
    srand(time(NULL));
    
    for (int i = 0; i < n_nodes; i++) {
        net->nodes[i].id = i;
        net->nodes[i].latitude = (float)rand() / RAND_MAX * area_size;
        net->nodes[i].longitude = (float)rand() / RAND_MAX * area_size;
        net->nodes[i].depth = 0.0;  /* Surface sensor */
        net->nodes[i].max_samples = 1000;
        net->nodes[i].n_samples = 0;
        net->nodes[i].samples = calloc(net->nodes[i].max_samples, sizeof(seismic_sample_t));
        net->nodes[i].noise_level = 0.01 + (float)rand() / RAND_MAX * 0.02;
        net->nodes[i].sensitivity = 0.8 + (float)rand() / RAND_MAX * 0.4;
    }
    
    return net;
}

/* Generate seismic event */
void seismic_generate_event(seismic_network_t* net, float magnitude, float depth) {
    seismic_event_t event;
    event.magnitude = magnitude;
    event.depth = depth;
    event.latitude = (float)rand() / RAND_MAX * 100;
    event.longitude = (float)rand() / RAND_MAX * 100;
    event.origin_time = time(NULL);
    event.p_velocity = 6.0;  /* km/s */
    event.s_velocity = 3.5;  /* km/s */
    event.intensity = (int)(magnitude * 2);
    
    /* Add event */
    net->n_events++;
    net->events = realloc(net->events, net->n_events * sizeof(seismic_event_t));
    net->events[net->n_events - 1] = event;
    
    /* Generate waves at each node */
    for (int i = 0; i < net->n_nodes; i++) {
        seismic_node_t* node = &net->nodes[i];
        
        /* Calculate distance from event */
        float dx = node->latitude - event.latitude;
        float dy = node->longitude - event.longitude;
        float distance = sqrt(dx*dx + dy*dy);
        
        /* Time delays */
        float p_time = distance / event.p_velocity;
        float s_time = distance / event.s_velocity;
        float surface_time = distance / 2.0;  /* Slower surface waves */
        
        /* Generate samples */
        int n_samples = 100;
        for (int s = 0; s < n_samples && node->n_samples < node->max_samples; s++) {
            float t = s * 0.01;  /* 10ms intervals */
            seismic_sample_t* sample = &node->samples[node->n_samples];
            
            /* P-wave (high frequency, low amplitude) */
            float p_amp = magnitude * exp(-distance / 100) * exp(-fabs(t - p_time) / 0.1);
            sample->p_wave = (p_amp > 0.1) ? ((p_amp > 0) ? 1 : -1) : 0;
            
            /* S-wave (medium frequency, medium amplitude) */
            float s_amp = magnitude * 1.5 * exp(-distance / 80) * exp(-fabs(t - s_time) / 0.2);
            sample->s_wave = (s_amp > 0.1) ? ((s_amp > 0) ? 1 : -1) : 0;
            
            /* Surface wave (low frequency, high amplitude) */
            float surf_amp = magnitude * 2.0 * exp(-distance / 60) * exp(-fabs(t - surface_time) / 0.5);
            sample->surface_wave = (surf_amp > 0.1) ? ((surf_amp > 0) ? 1 : -1) : 0;
            
            sample->amplitude = sqrt(p_amp*p_amp + s_amp*s_amp + surf_amp*surf_amp);
            sample->frequency = 10.0 + p_amp * 5;
            sample->timestamp = time(NULL);
            
            node->n_samples++;
            net->total_samples++;
        }
    }
}

/* Generate background noise */
void seismic_generate_noise(seismic_network_t* net) {
    for (int i = 0; i < net->n_nodes; i++) {
        seismic_node_t* node = &net->nodes[i];
        
        if (node->n_samples < node->max_samples) {
            seismic_sample_t* sample = &node->samples[node->n_samples];
            
            float noise = ((float)rand() / RAND_MAX - 0.5) * node->noise_level;
            sample->p_wave = (noise > 0.05) ? 1 : ((noise < -0.05) ? -1 : 0);
            sample->s_wave = 0;
            sample->surface_wave = 0;
            sample->amplitude = fabs(noise);
            sample->frequency = 1.0;
            sample->timestamp = time(NULL);
            
            node->n_samples++;
            net->total_samples++;
        }
    }
}

/* Compress seismic data to ternary */
int seismic_compress_ternary(seismic_network_t* net, int* output) {
    int idx = 0;
    
    for (int i = 0; i < net->n_nodes; i++) {
        seismic_node_t* node = &net->nodes[i];
        
        /* Pack 3 waves per sample into ternary words */
        for (int s = 0; s < node->n_samples; s++) {
            seismic_sample_t* sample = &node->samples[s];
            
            /* 3 trits per sample (P, S, Surface) */
            int word = 0;
            word |= (sample->p_wave + 1) << 0;
            word |= (sample->s_wave + 1) << 2;
            word |= (sample->surface_wave + 1) << 4;
            
            output[idx++] = word;
            net->compressed_samples++;
        }
    }
    
    return idx;
}

/* Export network state as JSON */
void seismic_export_json(seismic_network_t* net, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "{\n");
    fprintf(f, "  \"n_nodes\": %d,\n", net->n_nodes);
    fprintf(f, "  \"n_events\": %d,\n", net->n_events);
    fprintf(f, "  \"total_samples\": %d,\n", net->total_samples);
    fprintf(f, "  \"compressed_samples\": %d,\n", net->compressed_samples);
    fprintf(f, "  \"compression_ratio\": %.2f,\n",
            net->total_samples > 0 ? (float)net->compressed_samples / net->total_samples : 0);
    fprintf(f, "  \"events\": [\n");
    
    for (int i = 0; i < net->n_events; i++) {
        seismic_event_t* event = &net->events[i];
        fprintf(f, "    {\n");
        fprintf(f, "      \"magnitude\": %.2f,\n", event->magnitude);
        fprintf(f, "      \"depth\": %.2f,\n", event->depth);
        fprintf(f, "      \"lat\": %.4f,\n", event->latitude);
        fprintf(f, "      \"lon\": %.4f,\n", event->longitude);
        fprintf(f, "      \"intensity\": %d\n", event->intensity);
        fprintf(f, "    }");
        if (i < net->n_events - 1) fprintf(f, ",");
        fprintf(f, "\n");
    }
    
    fprintf(f, "  ],\n");
    fprintf(f, "  \"nodes\": [\n");
    
    for (int i = 0; i < net->n_nodes; i++) {
        seismic_node_t* node = &net->nodes[i];
        fprintf(f, "    {\n");
        fprintf(f, "      \"id\": %d,\n", node->id);
        fprintf(f, "      \"lat\": %.4f,\n", node->latitude);
        fprintf(f, "      \"lon\": %.4f,\n", node->longitude);
        fprintf(f, "      \"samples\": %d,\n", node->n_samples);
        fprintf(f, "      \"noise\": %.4f\n", node->noise_level);
        fprintf(f, "    }");
        if (i < net->n_nodes - 1) fprintf(f, ",");
        fprintf(f, "\n");
    }
    
    fprintf(f, "  ]\n}\n");
    fclose(f);
}

/* Print network status */
void seismic_print_status(seismic_network_t* net) {
    printf("Seismic Network Status\n");
    printf("======================\n");
    printf("Nodes: %d\n", net->n_nodes);
    printf("Events: %d\n", net->n_events);
    printf("Total samples: %d\n", net->total_samples);
    printf("Compressed: %d (%.1f%%)\n", net->compressed_samples,
           net->total_samples > 0 ? 100.0 * net->compressed_samples / net->total_samples : 0);
    printf("\n");
    
    for (int i = 0; i < net->n_nodes; i++) {
        seismic_node_t* node = &net->nodes[i];
        printf("Node %2d [%.1f, %.1f]: %d samples, noise=%.3f\n",
               node->id, node->latitude, node->longitude,
               node->n_samples, node->noise_level);
    }
    
    if (net->n_events > 0) {
        printf("\nRecent Events:\n");
        for (int i = 0; i < net->n_events; i++) {
            seismic_event_t* event = &net->events[i];
            printf("  M%.1f at [%.1f, %.1f] depth=%.1fkm\n",
                   event->magnitude, event->latitude, event->longitude, event->depth);
        }
    }
}

/* Free network */
void seismic_destroy(seismic_network_t* net) {
    if (net) {
        for (int i = 0; i < net->n_nodes; i++) {
            free(net->nodes[i].samples);
        }
        free(net->nodes);
        free(net->events);
        free(net);
    }
}

/* CLI interface */
int seismic_main(int argc, char* argv[]) {
    int n_nodes = 8;
    int n_events = 3;
    float area = 100.0;
    char output_file[256] = "seismic_output.json";
    
    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) n_nodes = atoi(argv[++i]);
        else if (strcmp(argv[i], "-e") == 0 && i + 1 < argc) n_events = atoi(argv[++i]);
        else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) area = atof(argv[++i]);
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) strncpy(output_file, argv[++i], 255);
        else {
            printf("Usage: %s [-n nodes] [-e events] [-a area] [-o output]\n", argv[0]);
            return 1;
        }
    }
    
    printf("Seismic Monitoring Network (Ternary)\n");
    printf("Nodes: %d | Events: %d | Area: %.0f×%.0f km\n", n_nodes, n_events, area, area);
    
    /* Create network */
    seismic_network_t* net = seismic_create(n_nodes, area);
    if (!net) {
        fprintf(stderr, "Error: Failed to create network\n");
        return 1;
    }
    
    /* Generate seismic events */
    for (int e = 0; e < n_events; e++) {
        float magnitude = 3.0 + (float)rand() / RAND_MAX * 4.0;
        float depth = 5.0 + (float)rand() / RAND_MAX * 30.0;
        seismic_generate_event(net, magnitude, depth);
        printf("Generated event: M%.1f at depth %.1f km\n", magnitude, depth);
    }
    
    /* Add background noise */
    for (int i = 0; i < 100; i++) {
        seismic_generate_noise(net);
    }
    
    /* Print status */
    printf("\n");
    seismic_print_status(net);
    
    /* Export results */
    seismic_export_json(net, output_file);
    printf("\nResults exported to %s\n", output_file);
    
    seismic_destroy(net);
    return 0;
}
