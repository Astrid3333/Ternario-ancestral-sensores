/**
 * perceptron_ternary.c — Perceptrón Ternario
 * 
 * Implementa un perceptrón con pesos en {-1, 0, +1}:
 * - Entradas: vectores ternarios
 * - Pesos: matrices ternarias
 * - Activación: función de umbral ternaria
 * - Entrenamiento: regla de aprendizaje ternaria
 * 
 * Ventajas sobre binario:
 * - Mayor capacidad de representación
 * - Memoria más eficiente (1.58 bits/peso vs 1 bit)
 * - Computación natural en sistemas ternarios
 * 
 * Aplicaciones:
 * - Clasificación de patrones
 * - Reconocimiento de formas
 * - Decisión en sistemas de sensores
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

/* Ternary matrix */
typedef struct {
    int rows;
    int cols;
    ternary_t* data;
} ternary_matrix_t;

/* Ternary vector */
typedef struct {
    int size;
    ternary_t* data;
} ternary_vector_t;

/* Perceptron */
typedef struct {
    int n_inputs;
    int n_outputs;
    ternary_matrix_t* weights;
    ternary_vector_t* bias;
    float learning_rate;
    int epochs;
    float accuracy;
} ternary_perceptron_t;

/* Create ternary matrix */
ternary_matrix_t* ternary_matrix_create(int rows, int cols) {
    ternary_matrix_t* m = malloc(sizeof(ternary_matrix_t));
    if (!m) return NULL;
    
    m->rows = rows;
    m->cols = cols;
    m->data = calloc(rows * cols, sizeof(ternary_t));
    
    return m;
}

/* Create ternary vector */
ternary_vector_t* ternary_vector_create(int size) {
    ternary_vector_t* v = malloc(sizeof(ternary_vector_t));
    if (!v) return NULL;
    
    v->size = size;
    v->data = calloc(size, sizeof(ternary_t));
    
    return v;
}

/* Randomly initialize ternary values */
void ternary_randomize(ternary_matrix_t* m, float density) {
    for (int i = 0; i < m->rows * m->cols; i++) {
        float r = (float)rand() / RAND_MAX;
        if (r < density / 2) {
            m->data[i] = T_NEG;
        } else if (r < density) {
            m->data[i] = T_POS;
        } else {
            m->data[i] = T_ZERO;
        }
    }
}

/* Ternary activation function */
ternary_t ternary_activation(int sum) {
    if (sum > 0) return T_POS;
    if (sum < 0) return T_NEG;
    return T_ZERO;
}

/* Ternary dot product */
int ternary_dot(ternary_vector_t* a, ternary_vector_t* b) {
    int sum = 0;
    int size = (a->size < b->size) ? a->size : b->size;
    
    for (int i = 0; i < size; i++) {
        sum += a->data[i] * b->data[i];
    }
    
    return sum;
}

/* Matrix-vector multiplication */
ternary_vector_t* ternary_matvec(ternary_matrix_t* m, ternary_vector_t* v) {
    ternary_vector_t* result = ternary_vector_create(m->rows);
    
    for (int i = 0; i < m->rows; i++) {
        int sum = 0;
        for (int j = 0; j < m->cols && j < v->size; j++) {
            sum += m->data[i * m->cols + j] * v->data[j];
        }
        result->data[i] = ternary_activation(sum);
    }
    
    return result;
}

/* Create perceptron */
ternary_perceptron_t* perceptron_create(int n_inputs, int n_outputs, float lr) {
    ternary_perceptron_t* p = malloc(sizeof(ternary_perceptron_t));
    if (!p) return NULL;
    
    p->n_inputs = n_inputs;
    p->n_outputs = n_outputs;
    p->learning_rate = lr;
    p->epochs = 0;
    p->accuracy = 0;
    
    p->weights = ternary_matrix_create(n_outputs, n_inputs);
    p->bias = ternary_vector_create(n_outputs);
    
    /* Random initialization */
    ternary_randomize(p->weights, 0.333);
    for (int i = 0; i < n_outputs; i++) {
        p->bias->data[i] = (rand() % 3) - 1;
    }
    
    return p;
}

/* Forward pass */
ternary_vector_t* perceptron_forward(ternary_perceptron_t* p, ternary_vector_t* input) {
    return ternary_matvec(p->weights, input);
}

/* Ternary learning rule */
void perceptron_train(ternary_perceptron_t* p, ternary_vector_t* input, ternary_vector_t* target) {
    ternary_vector_t* output = perceptron_forward(p, input);
    
    for (int i = 0; i < p->n_outputs; i++) {
        ternary_t error = target->data[i] - output->data[i];
        
        if (error != T_ZERO) {
            /* Update weights */
            for (int j = 0; j < p->n_inputs; j++) {
                ternary_t delta = T_ZERO;
                
                if (error == T_POS && input->data[j] == T_POS) delta = T_POS;
                else if (error == T_POS && input->data[j] == T_NEG) delta = T_NEG;
                else if (error == T_NEG && input->data[j] == T_POS) delta = T_NEG;
                else if (error == T_NEG && input->data[j] == T_NEG) delta = T_POS;
                
                /* Apply learning rate (ternary scaling) */
                if (rand() / (float)RAND_MAX < p->learning_rate) {
                    p->weights->data[i * p->n_inputs + j] = delta;
                }
            }
            
            /* Update bias */
            if (rand() / (float)RAND_MAX < p->learning_rate) {
                p->bias->data[i] = error;
            }
        }
    }
    
    free(output->data);
    free(output);
}

/* Calculate accuracy */
float perceptron_accuracy(ternary_perceptron_t* p, ternary_vector_t** inputs, 
                         ternary_vector_t** targets, int n_samples) {
    int correct = 0;
    
    for (int s = 0; s < n_samples; s++) {
        ternary_vector_t* output = perceptron_forward(p, inputs[s]);
        
        int match = 1;
        for (int i = 0; i < p->n_outputs; i++) {
            if (output->data[i] != targets[s]->data[i]) {
                match = 0;
                break;
            }
        }
        
        if (match) correct++;
        
        free(output->data);
        free(output);
    }
    
    return (float)correct / n_samples;
}

/* Train on dataset */
void perceptron_train_dataset(ternary_perceptron_t* p, ternary_vector_t** inputs,
                             ternary_vector_t** targets, int n_samples, int epochs) {
    printf("Training ternary perceptron...\n");
    printf("Inputs: %d | Outputs: %d | Samples: %d | Epochs: %d\n",
           p->n_inputs, p->n_outputs, n_samples, epochs);
    
    for (int e = 0; e < epochs; e++) {
        /* Shuffle samples */
        int* order = malloc(n_samples * sizeof(int));
        for (int i = 0; i < n_samples; i++) order[i] = i;
        for (int i = n_samples - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            int tmp = order[i];
            order[i] = order[j];
            order[j] = tmp;
        }
        
        /* Train on each sample */
        for (int s = 0; s < n_samples; s++) {
            perceptron_train(p, inputs[order[s]], targets[order[s]]);
        }
        
        free(order);
        
        /* Calculate accuracy */
        p->accuracy = perceptron_accuracy(p, inputs, targets, n_samples);
        p->epochs = e + 1;
        
        if ((e + 1) % 10 == 0 || e == 0) {
            printf("Epoch %3d: accuracy=%.2f%%\n", e + 1, p->accuracy * 100);
        }
    }
}

/* Export weights as JSON */
void perceptron_export_json(ternary_perceptron_t* p, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "{\n");
    fprintf(f, "  \"n_inputs\": %d,\n", p->n_inputs);
    fprintf(f, "  \"n_outputs\": %d,\n", p->n_outputs);
    fprintf(f, "  \"learning_rate\": %.4f,\n", p->learning_rate);
    fprintf(f, "  \"epochs\": %d,\n", p->epochs);
    fprintf(f, "  \"accuracy\": %.4f,\n", p->accuracy);
    fprintf(f, "  \"weights\": [\n");
    
    for (int i = 0; i < p->n_outputs; i++) {
        fprintf(f, "    [");
        for (int j = 0; j < p->n_inputs; j++) {
            fprintf(f, "%d", p->weights->data[i * p->n_inputs + j]);
            if (j < p->n_inputs - 1) fprintf(f, ",");
        }
        fprintf(f, "]");
        if (i < p->n_outputs - 1) fprintf(f, ",");
        fprintf(f, "\n");
    }
    
    fprintf(f, "  ],\n");
    fprintf(f, "  \"bias\": [");
    for (int i = 0; i < p->n_outputs; i++) {
        fprintf(f, "%d", p->bias->data[i]);
        if (i < p->n_outputs - 1) fprintf(f, ",");
    }
    fprintf(f, "]\n}\n");
    
    fclose(f);
}

/* Free memory */
void perceptron_destroy(ternary_perceptron_t* p) {
    if (p) {
        free(p->weights->data);
        free(p->weights);
        free(p->bias->data);
        free(p->bias);
        free(p);
    }
}

void ternary_vector_free(ternary_vector_t* v) {
    if (v) {
        free(v->data);
        free(v);
    }
}

/* Example: XOR ternary */
void example_xor() {
    printf("=== Ternary XOR Example ===\n\n");
    
    /* Create perceptron */
    ternary_perceptron_t* p = perceptron_create(2, 1, 0.5);
    
    /* Create training data */
    int n_samples = 4;
    ternary_vector_t** inputs = malloc(n_samples * sizeof(ternary_vector_t*));
    ternary_vector_t** targets = malloc(n_samples * sizeof(ternary_vector_t*));
    
    /* Input combinations */
    ternary_t input_data[4][2] = {
        {T_NEG, T_NEG}, {T_NEG, T_POS}, {T_POS, T_NEG}, {T_POS, T_POS}
    };
    
    /* XOR targets */
    ternary_t target_data[4] = {T_ZERO, T_POS, T_POS, T_ZERO};
    
    for (int i = 0; i < n_samples; i++) {
        inputs[i] = ternary_vector_create(2);
        inputs[i]->data[0] = input_data[i][0];
        inputs[i]->data[1] = input_data[i][1];
        
        targets[i] = ternary_vector_create(1);
        targets[i]->data[0] = target_data[i];
    }
    
    /* Train */
    perceptron_train_dataset(p, inputs, targets, n_samples, 100);
    
    /* Test */
    printf("\nTest results:\n");
    for (int i = 0; i < n_samples; i++) {
        ternary_vector_t* output = perceptron_forward(p, inputs[i]);
        printf("  [%+d, %+d] → %+d (expected %+d)\n",
               inputs[i]->data[0], inputs[i]->data[1],
               output->data[0], targets[i]->data[0]);
        ternary_vector_free(output);
    }
    
    /* Export */
    perceptron_export_json(p, "perceptron_xor.json");
    printf("\nWeights exported to perceptron_xor.json\n");
    
    /* Cleanup */
    for (int i = 0; i < n_samples; i++) {
        ternary_vector_free(inputs[i]);
        ternary_vector_free(targets[i]);
    }
    free(inputs);
    free(targets);
    perceptron_destroy(p);
}

/* CLI interface */
int perceptron_main(int argc, char* argv[]) {
    printf("Ternary Perceptron\n");
    printf("==================\n\n");
    
    /* Run XOR example */
    example_xor();
    
    return 0;
}
