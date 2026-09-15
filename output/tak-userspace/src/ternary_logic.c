/**
 * ternary_logic.c — Tablas de Verdad Ternarias y Clasificación
 * 
 * Implementa sistema lógico completo ternario:
 * - 3 valores: -1 (Falso), 0 (Indeterminado), +1 (Verdadero)
 * - Operadores: AND, OR, NOT, IMPLIES, IFF, XOR
 * - Tablas de verdad para 2 y 3 entradas
 * - Clasificación de patrones basada en lógica ternaria
 * 
 * Aplicaciones:
 * - Sistemas de decisión difusa
 * - Control de sensores con 3 niveles
 * - Razonamiento bajo incertidumbre
 * - Compresión de reglas de decisión
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Ternary logic values */
typedef enum {
    T_FALSE = -1,
    T_UNDEF = 0,
    T_TRUE = 1
} ternary_logic_t;

/* Logic operations */
ternary_logic_t ternary_not(ternary_logic_t a) {
    return -a;
}

ternary_logic_t ternary_and(ternary_logic_t a, ternary_logic_t b) {
    if (a == T_FALSE || b == T_FALSE) return T_FALSE;
    if (a == T_TRUE && b == T_TRUE) return T_TRUE;
    return T_UNDEF;
}

ternary_logic_t ternary_or(ternary_logic_t a, ternary_logic_t b) {
    if (a == T_TRUE || b == T_TRUE) return T_TRUE;
    if (a == T_FALSE && b == T_FALSE) return T_FALSE;
    return T_UNDEF;
}

ternary_logic_t ternary_xor(ternary_logic_t a, ternary_logic_t b) {
    if (a == T_UNDEF || b == T_UNDEF) return T_UNDEF;
    if (a == b) return T_FALSE;
    return T_TRUE;
}

ternary_logic_t ternary_implies(ternary_logic_t a, ternary_logic_t b) {
    if (a == T_FALSE) return T_TRUE;
    if (a == T_TRUE && b == T_TRUE) return T_TRUE;
    if (a == T_TRUE && b == T_FALSE) return T_FALSE;
    return T_UNDEF;
}

ternary_logic_t ternary_iff(ternary_logic_t a, ternary_logic_t b) {
    if (a == b) return T_TRUE;
    if (a == T_UNDEF || b == T_UNDEF) return T_UNDEF;
    return T_FALSE;
}

/* Print ternary value */
const char* ternary_str(ternary_logic_t v) {
    switch (v) {
        case T_FALSE: return "-1";
        case T_UNDEF: return " 0";
        case T_TRUE: return "+1";
        default: return "?";
    }
}

/* Print 2-input truth table */
void print_truth_table_2() {
    ternary_logic_t inputs[3] = {T_FALSE, T_UNDEF, T_TRUE};
    
    printf("=== 2-Input Ternary Truth Tables ===\n\n");
    
    /* AND */
    printf("AND:\n");
    printf("  A\\B |  -1   0  +1\n");
    printf("  ----+------------\n");
    for (int i = 0; i < 3; i++) {
        printf("  %s |", ternary_str(inputs[i]));
        for (int j = 0; j < 3; j++) {
            printf("  %s", ternary_str(ternary_and(inputs[i], inputs[j])));
        }
        printf("\n");
    }
    
    /* OR */
    printf("\nOR:\n");
    printf("  A\\B |  -1   0  +1\n");
    printf("  ----+------------\n");
    for (int i = 0; i < 3; i++) {
        printf("  %s |", ternary_str(inputs[i]));
        for (int j = 0; j < 3; j++) {
            printf("  %s", ternary_str(ternary_or(inputs[i], inputs[j])));
        }
        printf("\n");
    }
    
    /* XOR */
    printf("\nXOR:\n");
    printf("  A\\B |  -1   0  +1\n");
    printf("  ----+------------\n");
    for (int i = 0; i < 3; i++) {
        printf("  %s |", ternary_str(inputs[i]));
        for (int j = 0; j < 3; j++) {
            printf("  %s", ternary_str(ternary_xor(inputs[i], inputs[j])));
        }
        printf("\n");
    }
    
    /* IMPLIES */
    printf("\nIMPLIES (A→B):\n");
    printf("  A\\B |  -1   0  +1\n");
    printf("  ----+------------\n");
    for (int i = 0; i < 3; i++) {
        printf("  %s |", ternary_str(inputs[i]));
        for (int j = 0; j < 3; j++) {
            printf("  %s", ternary_str(ternary_implies(inputs[i], inputs[j])));
        }
        printf("\n");
    }
    
    /* IFF */
    printf("\nIFF (A↔B):\n");
    printf("  A\\B |  -1   0  +1\n");
    printf("  ----+------------\n");
    for (int i = 0; i < 3; i++) {
        printf("  %s |", ternary_str(inputs[i]));
        for (int j = 0; j < 3; j++) {
            printf("  %s", ternary_str(ternary_iff(inputs[i], inputs[j])));
        }
        printf("\n");
    }
}

/* Classifier using ternary logic rules */
typedef struct {
    char name[32];
    int n_rules;
    struct {
        int input_indices[4];
        ternary_logic_t input_values[4];
        ternary_logic_t output;
        char description[64];
    } rules[20];
} ternary_classifier_t;

/* Create classifier */
ternary_classifier_t* classifier_create(const char* name) {
    ternary_classifier_t* c = malloc(sizeof(ternary_classifier_t));
    if (!c) return NULL;
    
    strncpy(c->name, name, 31);
    c->n_rules = 0;
    
    return c;
}

/* Add rule to classifier */
void classifier_add_rule(ternary_classifier_t* c, int* inputs, ternary_logic_t* values,
                         ternary_logic_t output, const char* desc) {
    if (c->n_rules >= 20) return;
    
    int idx = c->n_rules;
    for (int i = 0; i < 4; i++) {
        c->rules[idx].input_indices[i] = inputs[i];
        c->rules[idx].input_values[i] = values[i];
    }
    c->rules[idx].output = output;
    strncpy(c->rules[idx].description, desc, 63);
    
    c->n_rules++;
}

/* Classify input pattern */
ternary_logic_t classifier_classify(ternary_classifier_t* c, ternary_logic_t* inputs, 
                                    int n_inputs, int* matched_rule) {
    for (int r = 0; r < c->n_rules; r++) {
        int match = 1;
        
        for (int i = 0; i < 4 && c->rules[r].input_indices[i] >= 0; i++) {
            int idx = c->rules[r].input_indices[i];
            if (idx < n_inputs && inputs[idx] != c->rules[r].input_values[i]) {
                match = 0;
                break;
            }
        }
        
        if (match) {
            if (matched_rule) *matched_rule = r;
            return c->rules[r].output;
        }
    }
    
    if (matched_rule) *matched_rule = -1;
    return T_UNDEF;
}

/* Export classifier as JSON */
void classifier_export_json(ternary_classifier_t* c, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "{\n");
    fprintf(f, "  \"name\": \"%s\",\n", c->name);
    fprintf(f, "  \"n_rules\": %d,\n", c->n_rules);
    fprintf(f, "  \"rules\": [\n");
    
    for (int r = 0; r < c->n_rules; r++) {
        fprintf(f, "    {\n");
        fprintf(f, "      \"inputs\": [");
        for (int i = 0; i < 4; i++) {
            if (c->rules[r].input_indices[i] >= 0) {
                fprintf(f, "%d", c->rules[r].input_indices[i]);
                if (i < 3 && c->rules[r].input_indices[i + 1] >= 0) fprintf(f, ",");
            }
        }
        fprintf(f, "],\n");
        
        fprintf(f, "      \"values\": [");
        for (int i = 0; i < 4; i++) {
            if (c->rules[r].input_indices[i] >= 0) {
                fprintf(f, "%d", c->rules[r].input_values[i]);
                if (i < 3 && c->rules[r].input_indices[i + 1] >= 0) fprintf(f, ",");
            }
        }
        fprintf(f, "],\n");
        
        fprintf(f, "      \"output\": %d,\n", c->rules[r].output);
        fprintf(f, "      \"desc\": \"%s\"\n", c->rules[r].description);
        fprintf(f, "    }");
        if (r < c->n_rules - 1) fprintf(f, ",");
        fprintf(f, "\n");
    }
    
    fprintf(f, "  ]\n}\n");
    fclose(f);
}

/* Free classifier */
void classifier_destroy(ternary_classifier_t* c) {
    free(c);
}

/* Example: Sensor classification */
void example_sensor_classifier() {
    printf("=== Ternary Sensor Classifier ===\n\n");
    
    /* Create classifier for environmental sensors */
    ternary_classifier_t* c = classifier_create("environment");
    
    /* Rules based on ternary logic */
    /* Inputs: [0]=temperature, [1]=humidity, [2]=radiation */
    
    int inputs1[4] = {0, -1, -1, -1};
    ternary_logic_t values1[4] = {T_TRUE, T_UNDEF, T_UNDEF, T_UNDEF};
    classifier_add_rule(c, inputs1, values1, T_TRUE, "High temperature → alert");
    
    int inputs2[4] = {1, -1, -1, -1};
    ternary_logic_t values2[4] = {T_TRUE, T_UNDEF, T_UNDEF, T_UNDEF};
    classifier_add_rule(c, inputs2, values2, T_TRUE, "High humidity → rain likely");
    
    int inputs3[4] = {0, 1, -1, -1};
    ternary_logic_t values3[4] = {T_FALSE, T_FALSE, T_UNDEF, T_UNDEF};
    classifier_add_rule(c, inputs3, values3, T_TRUE, "Low temp + low humidity → frost warning");
    
    int inputs4[4] = {0, 1, 2, -1};
    ternary_logic_t values4[4] = {T_TRUE, T_TRUE, T_TRUE, T_UNDEF};
    classifier_add_rule(c, inputs4, values4, T_FALSE, "All high → overexposure");
    
    int inputs5[4] = {0, 1, 2, -1};
    ternary_logic_t values5[4] = {T_UNDEF, T_UNDEF, T_UNDEF, T_UNDEF};
    classifier_add_rule(c, inputs5, values5, T_UNDEF, "All neutral → normal conditions");
    
    /* Test patterns */
    printf("Testing classifier:\n\n");
    
    ternary_logic_t test_patterns[][3] = {
        {T_TRUE, T_FALSE, T_TRUE},   /* High temp, low humidity, high radiation */
        {T_FALSE, T_TRUE, T_FALSE},  /* Low temp, high humidity, low radiation */
        {T_UNDEF, T_UNDEF, T_UNDEF}, /* All neutral */
        {T_TRUE, T_TRUE, T_TRUE},    /* All high */
    };
    
    const char* pattern_desc[] = {
        "Hot & dry & sunny",
        "Cold & humid & cloudy",
        "Normal conditions",
        "All extreme"
    };
    
    for (int p = 0; p < 4; p++) {
        int rule_idx;
        ternary_logic_t result = classifier_classify(c, test_patterns[p], 3, &rule_idx);
        
        printf("Pattern %d: [%s, %s, %s] → %s",
               p,
               ternary_str(test_patterns[p][0]),
               ternary_str(test_patterns[p][1]),
               ternary_str(test_patterns[p][2]),
               ternary_str(result));
        
        if (rule_idx >= 0) {
            printf(" (rule: %s)", c->rules[rule_idx].description);
        } else {
            printf(" (no rule matched)");
        }
        printf("\n");
    }
    
    /* Export */
    classifier_export_json(c, "classifier_sensor.json");
    printf("\nClassifier exported to classifier_sensor.json\n");
    
    classifier_destroy(c);
}

/* Export all truth tables as JSON */
void export_truth_tables_json(const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) return;
    
    ternary_logic_t inputs[3] = {T_FALSE, T_UNDEF, T_TRUE};
    
    fprintf(f, "{\n");
    fprintf(f, "  \"operators\": {\n");
    
    /* AND */
    fprintf(f, "    \"AND\": [\n");
    for (int i = 0; i < 3; i++) {
        fprintf(f, "      [");
        for (int j = 0; j < 3; j++) {
            fprintf(f, "%d", ternary_and(inputs[i], inputs[j]));
            if (j < 2) fprintf(f, ",");
        }
        fprintf(f, "]");
        if (i < 2) fprintf(f, ",");
        fprintf(f, "\n");
    }
    fprintf(f, "    ],\n");
    
    /* OR */
    fprintf(f, "    \"OR\": [\n");
    for (int i = 0; i < 3; i++) {
        fprintf(f, "      [");
        for (int j = 0; j < 3; j++) {
            fprintf(f, "%d", ternary_or(inputs[i], inputs[j]));
            if (j < 2) fprintf(f, ",");
        }
        fprintf(f, "]");
        if (i < 2) fprintf(f, ",");
        fprintf(f, "\n");
    }
    fprintf(f, "    ],\n");
    
    /* XOR */
    fprintf(f, "    \"XOR\": [\n");
    for (int i = 0; i < 3; i++) {
        fprintf(f, "      [");
        for (int j = 0; j < 3; j++) {
            fprintf(f, "%d", ternary_xor(inputs[i], inputs[j]));
            if (j < 2) fprintf(f, ",");
        }
        fprintf(f, "]");
        if (i < 2) fprintf(f, ",");
        fprintf(f, "\n");
    }
    fprintf(f, "    ],\n");
    
    /* IMPLIES */
    fprintf(f, "    \"IMPLIES\": [\n");
    for (int i = 0; i < 3; i++) {
        fprintf(f, "      [");
        for (int j = 0; j < 3; j++) {
            fprintf(f, "%d", ternary_implies(inputs[i], inputs[j]));
            if (j < 2) fprintf(f, ",");
        }
        fprintf(f, "]");
        if (i < 2) fprintf(f, ",");
        fprintf(f, "\n");
    }
    fprintf(f, "    ],\n");
    
    /* IFF */
    fprintf(f, "    \"IFF\": [\n");
    for (int i = 0; i < 3; i++) {
        fprintf(f, "      [");
        for (int j = 0; j < 3; j++) {
            fprintf(f, "%d", ternary_iff(inputs[i], inputs[j]));
            if (j < 2) fprintf(f, ",");
        }
        fprintf(f, "]");
        if (i < 2) fprintf(f, ",");
        fprintf(f, "\n");
    }
    fprintf(f, "    ]\n");
    
    fprintf(f, "  }\n}\n");
    fclose(f);
}

/* CLI interface */
int logic_main(int argc, char* argv[]) {
    printf("Ternary Logic System\n");
    printf("====================\n\n");
    
    /* Print truth tables */
    print_truth_table_2();
    
    /* Export truth tables */
    export_truth_tables_json("truth_tables_ternary.json");
    printf("\nTruth tables exported to truth_tables_ternary.json\n\n");
    
    /* Run classifier example */
    example_sensor_classifier();
    
    return 0;
}
