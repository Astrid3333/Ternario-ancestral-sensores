/**
 * hopfield_ternary.c — Red de Hopfield Ternaria
 * 
 * Implementa una red de Hopfield con neuronas ternarias:
 * - Cada neurona tiene 3 estados: -1, 0, +1
 * - Almacenamiento de patrones con regla de aprendizaje ternaria
 * - Recuperación de patrones con dinámica de convergencia
 * - Capacidad de memoria: superior a la red binaria
 * 
 * Ventajas sobre binario:
 * - Mayor capacidad de almacenamiento
 * - Patrones con información de "intensidad" (0 = neutral)
 * - Convergencia más estable
 * - Ideal para sensores con 3 niveles
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* Ternary value */
typedef enum {
    T_NEG = -1,
    T_ZERO = 0,
    T_POS = 1
} ternary_t;

/* Ternary vector */
typedef struct {
    int size;
    ternary_t* data;
} ternary_vec_t;

/* Hopfield network */
typedef struct {
    int n_neurons;
    ternary_vec_t** weights;  /* Weight matrix (symmetric) */
    ternary_vec_t* state;    /* Current state */
    int n_patterns;          /* Stored patterns */
    ternary_vec_t** patterns;
    float temperature;       /* For stochastic dynamics */
    int max_iterations;
    float convergence_threshold;
} hopfield_ternary_t;

/* Create ternary vector */
ternary_vec_t* ternary_vec_create(int size) {
    ternary_vec_t* v = malloc(sizeof(ternary_vec_t));
    if (!v) return NULL;
    
    v->size = size;
    v->data = calloc(size, sizeof(ternary_t));
    
    return v;
}

/* Create Hopfield network */
hopfield_ternary_t* hopfield_create(int n_neurons) {
    hopfield_ternary_t* net = malloc(sizeof(hopfield_ternary_t));
    if (!net) return NULL;
    
    net->n_neurons = n_neurons;
    net->n_patterns = 0;
    net->patterns = NULL;
    net->temperature = 0.1;
    net->max_iterations = 100;
    net->convergence_threshold = 0.01;
    
    /* Initialize weight matrix */
    net->weights = malloc(n_neurons * sizeof(ternary_vec_t*));
    for (int i = 0; i < n_neurons; i++) {
        net->weights[i] = ternary_vec_create(n_neurons);
    }
    
    /* Initialize state */
    net->state = ternary_vec_create(n_neurons);
    
    return net;
}

/* Store pattern using ternary Hebbian rule */
void hopfield_store(hopfield_ternary_t* net, ternary_vec_t* pattern) {
    if (pattern->size != net->n_neurons) {
        fprintf(stderr, "Error: Pattern size mismatch\n");
        return;
    }
    
    /* Store pattern */
    net->n_patterns++;
    net->patterns = realloc(net->patterns, net->n_patterns * sizeof(ternary_vec_t*));
    net->patterns[net->n_patterns - 1] = ternary_vec_create(net->n_neurons);
    memcpy(net->patterns[net->n_patterns - 1]->data, pattern->data, 
           net->n_neurons * sizeof(ternary_t));
    
    /* Update weights using ternary Hebbian rule */
    for (int i = 0; i < net->n_neurons; i++) {
        for (int j = 0; j < net->n_neurons; j++) {
            if (i != j) {
                int delta = pattern->data[i] * pattern->data[j];
                
                /* Ternary weight update */
                if (delta > 0) {
                    net->weights[i]->data[j] = T_POS;
                } else if (delta < 0) {
                    net->weights[i]->data[j] = T_NEG;
                } else {
                    net->weights[i]->data[j] = T_ZERO;
                }
            }
        }
    }
    
    printf("Stored pattern %d (capacity: %.1f%%)\n", 
           net->n_patterns, 100.0 * net->n_patterns / net->n_neurons);
}

/* Update neuron state */
void hopfield_update_neuron(hopfield_ternary_t* net, int neuron) {
    int sum = 0;
    
    for (int j = 0; j < net->n_neurons; j++) {
        sum += net->weights[neuron]->data[j] * net->state->data[j];
    }
    
    /* Ternary activation with temperature */
    float noise = ((float)rand() / RAND_MAX - 0.5) * net->temperature;
    float threshold = sum + noise;
    
    if (threshold > 0.5) {
        net->state->data[neuron] = T_POS;
    } else if (threshold < -0.5) {
        net->state->data[neuron] = T_NEG;
    } else {
        net->state->data[neuron] = T_ZERO;
    }
}

/* Async update (random order) */
void hopfield_update_async(hopfield_ternary_t* net) {
    int* order = malloc(net->n_neurons * sizeof(int));
    for (int i = 0; i < net->n_neurons; i++) order[i] = i;
    
    /* Shuffle */
    for (int i = net->n_neurons - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = order[i];
        order[i] = order[j];
        order[j] = tmp;
    }
    
    for (int i = 0; i < net->n_neurons; i++) {
        hopfield_update_neuron(net, order[i]);
    }
    
    free(order);
}

/* Sync update */
void hopfield_update_sync(hopfield_ternary_t* net) {
    ternary_vec_t* new_state = ternary_vec_create(net->n_neurons);
    
    for (int i = 0; i < net->n_neurons; i++) {
        int sum = 0;
        for (int j = 0; j < net->n_neurons; j++) {
            sum += net->weights[i]->data[j] * net->state->data[j];
        }
        
        float noise = ((float)rand() / RAND_MAX - 0.5) * net->temperature;
        float threshold = sum + noise;
        
        if (threshold > 0.5) {
            new_state->data[i] = T_POS;
        } else if (threshold < -0.5) {
            new_state->data[i] = T_NEG;
        } else {
            new_state->data[i] = T_ZERO;
        }
    }
    
    /* Swap states */
    ternary_vec_t* temp = net->state;
    net->state = new_state;
    free(temp->data);
    free(temp);
}

/* Calculate overlap with pattern */
float hopfield_overlap(hopfield_ternary_t* net, ternary_vec_t* pattern) {
    int match = 0;
    int total = 0;
    
    for (int i = 0; i < net->n_neurons; i++) {
        if (net->state->data[i] == pattern->data[i]) {
            match++;
        }
        total++;
    }
    
    return (float)match / total;
}

/* Find best matching pattern */
int hopfield_find_pattern(hopfield_ternary_t* net) {
    int best = -1;
    float best_overlap = -1;
    
    for (int p = 0; p < net->n_patterns; p++) {
        float overlap = hopfield_overlap(net, net->patterns[p]);
        if (overlap > best_overlap) {
            best_overlap = overlap;
            best = p;
        }
    }
    
    return best;
}

/* Recover pattern from noisy input */
ternary_vec_t* hopfield_recover(hopfield_ternary_t* net, ternary_vec_t* input, 
                                int* iterations_used) {
    /* Set initial state */
    memcpy(net->state->data, input->data, net->n_neurons * sizeof(ternary_t));
    
    int iter;
    for (iter = 0; iter < net->max_iterations; iter++) {
        ternary_vec_t* old_state = ternary_vec_create(net->n_neurons);
        memcpy(old_state->data, net->state->data, net->n_neurons * sizeof(ternary_t));
        
        /* Update */
        hopfield_update_async(net);
        
        /* Check convergence */
        int changed = 0;
        for (int i = 0; i < net->n_neurons; i++) {
            if (net->state->data[i] != old_state->data[i]) {
                changed = 1;
                break;
            }
        }
        
        free(old_state->data);
        free(old_state);
        
        if (!changed) break;
    }
    
    if (iterations_used) *iterations_used = iter;
    
    /* Return copy of recovered state */
    ternary_vec_t* result = ternary_vec_create(net->n_neurons);
    memcpy(result->data, net->state->data, net->n_neurons * sizeof(ternary_t));
    
    return result;
}

/* Add noise to pattern */
ternary_vec_t* hopfield_add_noise(ternary_vec_t* pattern, float noise_level) {
    ternary_vec_t* noisy = ternary_vec_create(pattern->size);
    
    for (int i = 0; i < pattern->size; i++) {
        if ((float)rand() / RAND_MAX < noise_level) {
            /* Flip state */
            noisy->data[i] = -pattern->data[i];
        } else {
            noisy->data[i] = pattern->data[i];
        }
    }
    
    return noisy;
}

/* Export network state as JSON */
void hopfield_export_json(hopfield_ternary_t* net, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "{\n");
    fprintf(f, "  \"n_neurons\": %d,\n", net->n_neurons);
    fprintf(f, "  \"n_patterns\": %d,\n", net->n_patterns);
    fprintf(f, "  \"capacity\": %.2f%%,\n", 100.0 * net->n_patterns / net->n_neurons);
    fprintf(f, "  \"patterns\": [\n");
    
    for (int p = 0; p < net->n_patterns; p++) {
        fprintf(f, "    [");
        for (int i = 0; i < net->n_neurons; i++) {
            fprintf(f, "%d", net->patterns[p]->data[i]);
            if (i < net->n_neurons - 1) fprintf(f, ",");
        }
        fprintf(f, "]");
        if (p < net->n_patterns - 1) fprintf(f, ",");
        fprintf(f, "\n");
    }
    
    fprintf(f, "  ],\n");
    fprintf(f, "  \"weights\": [\n");
    
    for (int i = 0; i < net->n_neurons; i++) {
        fprintf(f, "    [");
        for (int j = 0; j < net->n_neurons; j++) {
            fprintf(f, "%d", net->weights[i]->data[j]);
            if (j < net->n_neurons - 1) fprintf(f, ",");
        }
        fprintf(f, "]");
        if (i < net->n_neurons - 1) fprintf(f, ",");
        fprintf(f, "\n");
    }
    
    fprintf(f, "  ]\n}\n");
    fclose(f);
}

/* Free memory */
void hopfield_destroy(hopfield_ternary_t* net) {
    if (net) {
        for (int i = 0; i < net->n_neurons; i++) {
            free(net->weights[i]->data);
            free(net->weights[i]);
        }
        free(net->weights);
        
        free(net->state->data);
        free(net->state);
        
        for (int p = 0; p < net->n_patterns; p++) {
            free(net->patterns[p]->data);
            free(net->patterns[p]);
        }
        free(net->patterns);
        
        free(net);
    }
}

void ternary_vec_free(ternary_vec_t* v) {
    if (v) {
        free(v->data);
        free(v);
    }
}

/* Example: Pattern storage and retrieval */
void example_pattern_recovery() {
    printf("=== Ternary Hopfield Pattern Recovery ===\n\n");
    
    int n_neurons = 100;
    hopfield_ternary_t* net = hopfield_create(n_neurons);
    
    /* Create patterns */
    int n_patterns = 5;
    ternary_vec_t** patterns = malloc(n_patterns * sizeof(ternary_vec_t*));
    
    for (int p = 0; p < n_patterns; p++) {
        patterns[p] = ternary_vec_create(n_neurons);
        for (int i = 0; i < n_neurons; i++) {
            float r = (float)rand() / RAND_MAX;
            if (r < 0.333) patterns[p]->data[i] = T_NEG;
            else if (r < 0.666) patterns[p]->data[i] = T_ZERO;
            else patterns[p]->data[i] = T_POS;
        }
    }
    
    /* Store patterns */
    printf("Storing patterns:\n");
    for (int p = 0; p < n_patterns; p++) {
        hopfield_store(net, patterns[p]);
    }
    
    /* Test recovery with noise */
    printf("\nTesting pattern recovery:\n");
    for (int p = 0; p < n_patterns; p++) {
        ternary_vec_t* noisy = hopfield_add_noise(patterns[p], 0.3);
        
        int iterations;
        ternary_vec_t* recovered = hopfield_recover(net, noisy, &iterations);
        
        float overlap_orig = hopfield_overlap(net, patterns[p]);
        float overlap_noisy = hopfield_overlap(net, noisy);
        
        printf("Pattern %d: original↔noisy=%.2f%%, original↔recovered=%.2f%%, iterations=%d\n",
               p, overlap_noisy * 100, overlap_orig * 100, iterations);
        
        ternary_vec_free(noisy);
        ternary_vec_free(recovered);
    }
    
    /* Export */
    hopfield_export_json(net, "hopfield_ternary.json");
    printf("\nNetwork exported to hopfield_ternary.json\n");
    
    /* Cleanup */
    for (int p = 0; p < n_patterns; p++) {
        ternary_vec_free(patterns[p]);
    }
    free(patterns);
    hopfield_destroy(net);
}

/* CLI interface */
int hopfield_main(int argc, char* argv[]) {
    printf("Ternary Hopfield Network\n");
    printf("========================\n\n");
    
    example_pattern_recovery();
    
    return 0;
}
