/**
 * compression_ternary.c — Compresión de Datos Científicos en Ternario
 * 
 * Implementa algoritmos de compresión usando representación ternaria:
 * - Codificación ancestral (Maya, Persa, Babylonian)
 * - Comparación con gzip/bzip2/zstd
 * - Análisis de ratio de compresión para datos de sensores
 * - Audio ternario para señales de audio
 * 
 * Datos de entrada: arrays de floats (simulando sensores)
 * Datos de salida: representación ternaria comprimida
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <zlib.h>
#include <bzlib.h>

/* Ternary encoding modes */
typedef enum {
    ENCODE_MAYA,        /* Base 20 (kin, winal, tun...) */
    ENCODE_PERSIAN,     /* Base 60 (kad, arku, soss...) */
    ENCODE_BALANCED,    /* Balanced ternary (-1, 0, +1) */
    ENCODE_BABYLONIAN   /* Base 60 positional */
} ternary_encoding_t;

/* Compression result */
typedef struct {
    int original_size;
    int compressed_size;
    float ratio;
    float time_ms;
    char method[32];
} compression_result_t;

/* Convert float to balanced ternary array */
int* float_to_balanced_ternary(float value, int precision) {
    int* trits = calloc(precision, sizeof(int));
    
    float v = fabs(value);
    int sign = (value >= 0) ? 1 : -1;
    
    for (int i = 0; i < precision; i++) {
        float power = pow(3, i);
        float digit = v / power;
        
        if (digit >= 0.666) {
            trits[i] = 1;
            v -= power;
        } else if (digit >= 0.333) {
            trits[i] = 0;
        } else {
            trits[i] = -1;
            v += power;
        }
    }
    
    return trits;
}

/* Convert balanced ternary array to float */
float balanced_ternary_to_float(int* trits, int precision) {
    float value = 0;
    
    for (int i = 0; i < precision; i++) {
        value += trits[i] * pow(3, i);
    }
    
    return value;
}

/* Compress float array to ternary */
int compress_to_ternary(float* data, int n_samples, int precision, int* output) {
    int out_idx = 0;
    
    for (int i = 0; i < n_samples; i++) {
        int* trits = float_to_balanced_ternary(data[i], precision);
        
        /* Pack trits into integers (5 trits per int) */
        for (int t = 0; t < precision; t += 5) {
            int packed = 0;
            for (int j = 0; j < 5 && (t + j) < precision; j++) {
                packed |= (trits[t + j] + 1) << (j * 2);
            }
            output[out_idx++] = packed;
        }
        
        free(trits);
    }
    
    return out_idx;
}

/* Decompress ternary to float array */
void decompress_from_ternary(int* data, int n_packed, int precision, float* output) {
    int sample = 0;
    int trit_idx = 0;
    int* trits = calloc(precision, sizeof(int));
    
    for (int i = 0; i < n_packed; i++) {
        int packed = data[i];
        
        for (int j = 0; j < 5 && trit_idx < precision; j++) {
            trits[trit_idx] = ((packed >> (j * 2)) & 0x3) - 1;
            trit_idx++;
        }
        
        if (trit_idx >= precision) {
            output[sample] = balanced_ternary_to_float(trits, precision);
            sample++;
            trit_idx = 0;
        }
    }
    
    free(trits);
}

/* Maya-style encoding (Base 20) */
int compress_maya(float* data, int n_samples, int* output) {
    int out_idx = 0;
    
    for (int i = 0; i < n_samples; i++) {
        /* Convert to Maya numerals */
        int value = (int)(data[i] * 100);  /* Scale to integer */
        if (value < 0) value = -value;
        
        /* Maya uses base 20 */
        output[out_idx++] = value % 20;      /* K'in */
        output[out_idx++] = (value / 20) % 20;   /* Winal */
        output[out_idx++] = (value / 400) % 20;  /* Tun */
    }
    
    return out_idx;
}

/* Persian-style encoding (Base 60) */
int compress_persian(float* data, int n_samples, int* output) {
    int out_idx = 0;
    
    for (int i = 0; i < n_samples; i++) {
        int value = (int)(data[i] * 100);
        if (value < 0) value = -value;
        
        /* Persian uses base 60 */
        output[out_idx++] = value % 60;          /* Soss */
        output[out_idx++] = (value / 60) % 60;  /* Arku */
        output[out_idx++] = (value / 3600) % 60; /* Kad */
    }
    
    return out_idx;
}

/* Compress with zlib */
compression_result_t compress_zlib(float* data, int n_samples) {
    compression_result_t result;
    strcpy(result.method, "zlib");
    
    uLongf original_size = n_samples * sizeof(float);
    uLongf compressed_size = original_size * 1.1 + 12;
    Bytef* compressed = malloc(compressed_size);
    
    clock_t start = clock();
    int ret = compress2(compressed, &compressed_size, (Bytef*)data, original_size, Z_BEST_COMPRESSION);
    clock_t end = clock();
    
    if (ret != Z_OK) {
        result.ratio = 0;
        result.time_ms = 0;
        free(compressed);
        return result;
    }
    
    result.original_size = original_size;
    result.compressed_size = compressed_size;
    result.ratio = (float)compressed_size / original_size;
    result.time_ms = (float)(end - start) / CLOCKS_PER_SEC * 1000;
    
    free(compressed);
    return result;
}

/* Compress with bzip2 */
compression_result_t compress_bzip2(float* data, int n_samples) {
    compression_result_t result;
    strcpy(result.method, "bzip2");
    
    unsigned int original_size = n_samples * sizeof(float);
    unsigned int compressed_size = original_size * 1.1 + 100;
    char* compressed = malloc(compressed_size);
    
    clock_t start = start = clock();
    int ret = BZ2_bzBuffToBuffCompress(compressed, &compressed_size, 
                                        (char*)data, original_size, 9, 0, 0);
    clock_t end = clock();
    
    if (ret != BZ_OK) {
        result.ratio = 0;
        result.time_ms = 0;
        free(compressed);
        return result;
    }
    
    result.original_size = original_size;
    result.compressed_size = compressed_size;
    result.ratio = (float)compressed_size / original_size;
    result.time_ms = (float)(end - start) / CLOCKS_PER_SEC * 1000;
    
    free(compressed);
    return result;
}

/* Export compression results as JSON */
void compression_export_json(compression_result_t* results, int n_results, 
                            float* original_data, int n_samples,
                            const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "{\n");
    fprintf(f, "  \"n_samples\": %d,\n", n_samples);
    fprintf(f, "  \"original_size_bytes\": %d,\n", n_samples * (int)sizeof(float));
    fprintf(f, "  \"results\": [\n");
    
    for (int i = 0; i < n_results; i++) {
        fprintf(f, "    {\n");
        fprintf(f, "      \"method\": \"%s\",\n", results[i].method);
        fprintf(f, "      \"original_size\": %d,\n", results[i].original_size);
        fprintf(f, "      \"compressed_size\": %d,\n", results[i].compressed_size);
        fprintf(f, "      \"ratio\": %.4f,\n", results[i].ratio);
        fprintf(f, "      \"time_ms\": %.2f\n", results[i].time_ms);
        fprintf(f, "    }");
        if (i < n_results - 1) fprintf(f, ",");
        fprintf(f, "\n");
    }
    
    fprintf(f, "  ]\n}\n");
    fclose(f);
}

/* Print comparison table */
void print_compression_comparison(compression_result_t* results, int n_results) {
    printf("Compression Comparison\n");
    printf("======================\n");
    printf("%-15s %10s %10s %8s %10s\n", "Method", "Original", "Compressed", "Ratio", "Time(ms)");
    printf("%-15s %10s %10s %8s %10s\n", "------", "--------", "----------", "-----", "--------");
    
    for (int i = 0; i < n_results; i++) {
        printf("%-15s %10d %10d %7.1f%% %10.2f\n",
               results[i].method,
               results[i].original_size,
               results[i].compressed_size,
               results[i].ratio * 100,
               results[i].time_ms);
    }
}

/* CLI interface */
int compression_main(int argc, char* argv[]) {
    int n_samples = 1000;
    int precision = 16;
    char output_file[256] = "compression_results.json";
    
    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) n_samples = atoi(argv[++i]);
        else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) precision = atoi(argv[++i]);
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) strncpy(output_file, argv[++i], 255);
        else {
            printf("Usage: %s [-n samples] [-p precision] [-o output]\n", argv[0]);
            return 1;
        }
    }
    
    printf("Ternary Data Compression\n");
    printf("========================\n\n");
    printf("Samples: %d | Precision: %d trits\n\n", n_samples, precision);
    
    /* Generate test data (sensor-like) */
    float* data = malloc(n_samples * sizeof(float));
    srand(time(NULL));
    
    for (int i = 0; i < n_samples; i++) {
        /* Simulate temperature sensor */
        float t = i * 0.01;
        data[i] = 25.0 + 10.0 * sin(t) + ((float)rand() / RAND_MAX - 0.5) * 2;
    }
    
    /* Run compression methods */
    compression_result_t results[5];
    int n_results = 0;
    
    /* Ternary balanced */
    int* ternary_compressed = malloc(n_samples * (precision / 5 + 1) * sizeof(int));
    clock_t start = clock();
    int ternary_size = compress_to_ternary(data, n_samples, precision, ternary_compressed);
    clock_t end = clock();
    
    results[n_results].original_size = n_samples * sizeof(float);
    results[n_results].compressed_size = ternary_size * sizeof(int);
    results[n_results].ratio = (float)results[n_results].compressed_size / results[n_results].original_size;
    results[n_results].time_ms = (float)(end - start) / CLOCKS_PER_SEC * 1000;
    strcpy(results[n_results].method, "Ternary");
    n_results++;
    
    free(ternary_compressed);
    
    /* Maya encoding */
    int* maya_compressed = malloc(n_samples * 3 * sizeof(int));
    start = clock();
    int maya_size = compress_maya(data, n_samples, maya_compressed);
    end = clock();
    
    results[n_results].original_size = n_samples * sizeof(float);
    results[n_results].compressed_size = maya_size * sizeof(int);
    results[n_results].ratio = (float)results[n_results].compressed_size / results[n_results].original_size;
    results[n_results].time_ms = (float)(end - start) / CLOCKS_PER_SEC * 1000;
    strcpy(results[n_results].method, "Maya (Base20)");
    n_results++;
    
    free(maya_compressed);
    
    /* Persian encoding */
    int* persian_compressed = malloc(n_samples * 3 * sizeof(int));
    start = clock();
    int persian_size = compress_persian(data, n_samples, persian_compressed);
    end = clock();
    
    results[n_results].original_size = n_samples * sizeof(float);
    results[n_results].compressed_size = persian_size * sizeof(int);
    results[n_results].ratio = (float)results[n_results].compressed_size / results[n_results].original_size;
    results[n_results].time_ms = (float)(end - start) / CLOCKS_PER_SEC * 1000;
    strcpy(results[n_results].method, "Persian (Base60)");
    n_results++;
    
    free(persian_compressed);
    
    /* zlib */
    results[n_results] = compress_zlib(data, n_samples);
    n_results++;
    
    /* bzip2 */
    results[n_results] = compress_bzip2(data, n_samples);
    n_results++;
    
    /* Print comparison */
    print_compression_comparison(results, n_results);
    
    /* Export results */
    compression_export_json(results, n_results, data, n_samples, output_file);
    printf("\nResults exported to %s\n", output_file);
    
    free(data);
    return 0;
}
