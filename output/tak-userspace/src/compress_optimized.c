/**
 * compress_optimized.c — Compresión Optimizada para Datos de Sensores
 * 
 * Implementa algoritmos de compresión específicos para datos de sensores:
 * - Delta encoding (diferencias entre samples consecutivos)
 * - Run-length encoding para valores repetidos
 * - Codificación ternaria adaptativa
 * - Compresión por bloques
 * - Comparación con métodos estándar
 * 
 * Datos de entrada: series temporales de sensores
 * Optimización: aprovecha correlación temporal y limites de rango
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <zlib.h>

/* Compression result */
typedef struct {
    char method[32];
    int original_bytes;
    int compressed_bytes;
    float ratio;
    float time_ms;
    float mse;  /* Mean squared error (for lossy) */
} compress_result_t;

/* Delta encoding + ternary */
int compress_delta_ternary(float* data, int n, int precision, int* output) {
    int out_idx = 0;
    float prev = 0;
    
    for (int i = 0; i < n; i++) {
        /* Calculate delta */
        float delta = data[i] - prev;
        prev = data[i];
        
        /* Convert delta to ternary */
        float v = fabs(delta);
        int sign = (delta >= 0) ? 1 : -1;
        
        for (int t = 0; t < precision; t += 5) {
            int packed = 0;
            for (int j = 0; j < 5 && (t + j) < precision; j++) {
                float power = pow(3, t + j);
                int digit;
                
                if (v * 1000 / power > 0.666) {
                    digit = 1;
                    v -= power / 1000;
                } else if (v * 1000 / power > 0.333) {
                    digit = 0;
                } else {
                    digit = -1;
                    v += power / 1000;
                }
                
                if (sign < 0) digit = -digit;
                packed |= (digit + 1) << (j * 2);
            }
            output[out_idx++] = packed;
        }
    }
    
    return out_idx;
}

/* Run-length encoding for ternary */
int compress_rle_ternary(float* data, int n, float threshold, int* output) {
    int out_idx = 0;
    int i = 0;
    
    while (i < n) {
        /* Convert current value to ternary state */
        int state;
        if (data[i] < -threshold) state = -1;
        else if (data[i] > threshold) state = 1;
        else state = 0;
        
        /* Count consecutive same-state values */
        int count = 1;
        while (i + count < n) {
            int next_state;
            if (data[i + count] < -threshold) next_state = -1;
            else if (data[i + count] > threshold) next_state = 1;
            else next_state = 0;
            
            if (next_state != state) break;
            count++;
        }
        
        /* Store: state (2 bits) + count (14 bits) */
        output[out_idx++] = ((state + 1) << 14) | (count & 0x3FFF);
        
        i += count;
    }
    
    return out_idx;
}

/* Block-based ternary compression */
int compress_block_ternary(float* data, int n, int block_size, int* output) {
    int out_idx = 0;
    
    for (int b = 0; b < n; b += block_size) {
        int block_len = (b + block_size < n) ? block_size : (n - b);
        
        /* Find block min/max */
        float min_val = data[b], max_val = data[b];
        for (int i = 1; i < block_len; i++) {
            if (data[b + i] < min_val) min_val = data[b + i];
            if (data[b + i] > max_val) max_val = data[b + i];
        }
        
        /* Store block header: min (16 bits) + max (16 bits) */
        short min_fixed = (short)(min_val * 100);
        short max_fixed = (short)(max_val * 100);
        output[out_idx++] = min_fixed;
        output[out_idx++] = max_fixed;
        
        /* Quantize values to ternary within block range */
        float range = max_val - min_val;
        if (range < 0.001) range = 1;  /* Avoid division by zero */
        
        for (int i = 0; i < block_len; i++) {
            float normalized = (data[b + i] - min_val) / range;
            int state;
            if (normalized < 0.333) state = -1;
            else if (normalized < 0.666) state = 0;
            else state = 1;
            
            /* Pack 5 states per int */
            if (i % 5 == 0) {
                output[out_idx] = 0;
            }
            output[out_idx] |= (state + 1) << ((i % 5) * 2);
            if (i % 5 == 4) out_idx++;
        }
        if (block_len % 5 != 0) out_idx++;
    }
    
    return out_idx;
}

/* Adaptive ternary encoding */
int compress_adaptive_ternary(float* data, int n, int* output) {
    int out_idx = 0;
    
    /* Calculate statistics */
    float mean = 0, stddev = 0;
    for (int i = 0; i < n; i++) mean += data[i];
    mean /= n;
    for (int i = 0; i < n; i++) stddev += (data[i] - mean) * (data[i] - mean);
    stddev = sqrt(stddev / n);
    
    /* Store header: mean (16 bits) + stddev (16 bits) */
    output[out_idx++] = (short)(mean * 100);
    output[out_idx++] = (short)(stddev * 100);
    
    /* Adaptive thresholds based on stddev */
    float low_thresh = mean - stddev;
    float high_thresh = mean + stddev;
    
    /* Compress with adaptive thresholds */
    for (int i = 0; i < n; i += 5) {
        int packed = 0;
        for (int j = 0; j < 5 && (i + j) < n; j++) {
            int state;
            if (data[i + j] < low_thresh) state = -1;
            else if (data[i + j] > high_thresh) state = 1;
            else state = 0;
            
            packed |= (state + 1) << (j * 2);
        }
        output[out_idx++] = packed;
    }
    
    return out_idx;
}

/* Compress with zlib */
compress_result_t compress_zlib_opt(float* data, int n) {
    compress_result_t r;
    strcpy(r.method, "zlib");
    r.original_bytes = n * sizeof(float);
    
    uLongf comp_size = r.original_bytes * 1.1 + 12;
    Bytef* comp = malloc(comp_size);
    
    clock_t start = clock();
    compress2(comp, &comp_size, (Bytef*)data, r.original_bytes, Z_BEST_COMPRESSION);
    clock_t end = clock();
    
    r.compressed_bytes = comp_size;
    r.ratio = (float)comp_size / r.original_bytes;
    r.time_ms = (float)(end - start) / CLOCKS_PER_SEC * 1000;
    r.mse = 0;
    
    free(comp);
    return r;
}

/* Calculate MSE */
float calculate_mse(float* original, float* decompressed, int n) {
    float mse = 0;
    for (int i = 0; i < n; i++) {
        float diff = original[i] - decompressed[i];
        mse += diff * diff;
    }
    return mse / n;
}

/* Print comparison table */
void print_comparison(compress_result_t* results, int n) {
    printf("\nCompression Comparison for Sensor Data\n");
    printf("======================================\n");
    printf("%-20s %10s %10s %8s %8s %8s\n", 
           "Method", "Original", "Compressed", "Ratio", "Time(ms)", "MSE");
    printf("%-20s %10s %10s %8s %8s %8s\n",
           "------", "--------", "----------", "-----", "--------", "---");
    
    for (int i = 0; i < n; i++) {
        printf("%-20s %10d %10d %7.1f%% %7.2f %8.4f\n",
               results[i].method,
               results[i].original_bytes,
               results[i].compressed_bytes,
               results[i].ratio * 100,
               results[i].time_ms,
               results[i].mse);
    }
}

/* Export results as JSON */
void export_results_json(compress_result_t* results, int n, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "{\n  \"results\": [\n");
    for (int i = 0; i < n; i++) {
        fprintf(f, "    {\n");
        fprintf(f, "      \"method\": \"%s\",\n", results[i].method);
        fprintf(f, "      \"original_bytes\": %d,\n", results[i].original_bytes);
        fprintf(f, "      \"compressed_bytes\": %d,\n", results[i].compressed_bytes);
        fprintf(f, "      \"ratio\": %.4f,\n", results[i].ratio);
        fprintf(f, "      \"time_ms\": %.2f,\n", results[i].time_ms);
        fprintf(f, "      \"mse\": %.6f\n", results[i].mse);
        fprintf(f, "    }");
        if (i < n - 1) fprintf(f, ",");
        fprintf(f, "\n");
    }
    fprintf(f, "  ]\n}\n");
    fclose(f);
}

/* Generate realistic sensor data */
void generate_sensor_data(float* data, int n, float noise_level) {
    srand(time(NULL));
    
    for (int i = 0; i < n; i++) {
        float t = i * 0.01;
        
        /* Simulate temperature sensor with daily cycle */
        float base = 25.0 + 10.0 * sin(t * 0.1);  /* Slow variation */
        float trend = t * 0.001;  /* Slight upward trend */
        float noise = ((float)rand() / RAND_MAX - 0.5) * noise_level;
        
        data[i] = base + trend + noise;
    }
}

/* Print help */
void compress_help() {
    printf("Tritos Science — Optimized Sensor Compression\n");
    printf("==============================================\n\n");
    printf("Commands:\n");
    printf("  compare [-n samples] [-p precision]    Compare methods\n");
    printf("  compress <method> <input> <output>     Compress data\n");
    printf("\nMethods:\n");
    printf("  delta       - Delta encoding + ternary\n");
    printf("  rle         - Run-length encoding\n");
    printf("  block       - Block-based ternary\n");
    printf("  adaptive    - Adaptive ternary\n");
    printf("  zlib        - Standard zlib\n");
}

int compress_opt_main(int argc, char* argv[]) {
    if (argc < 2) {
        compress_help();
        return 0;
    }
    
    char* cmd = argv[1];
    
    if (strcmp(cmd, "compare") == 0) {
        int n = 10000;
        int precision = 16;
        
        /* Parse options */
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) n = atoi(argv[++i]);
            else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) precision = atoi(argv[++i]);
        }
        
        printf("Generating %d sensor samples...\n", n);
        
        float* data = malloc(n * sizeof(float));
        generate_sensor_data(data, n, 2.0);
        
        compress_result_t results[6];
        int n_results = 0;
        
        /* Delta + Ternary */
        int* buf1 = malloc(n * (precision / 5 + 1) * sizeof(int));
        clock_t start = clock();
        int sz1 = compress_delta_ternary(data, n, precision, buf1);
        clock_t end = clock();
        results[n_results].original_bytes = n * sizeof(float);
        results[n_results].compressed_bytes = sz1 * sizeof(int);
        results[n_results].ratio = (float)results[n_results].compressed_bytes / results[n_results].original_bytes;
        results[n_results].time_ms = (float)(end - start) / CLOCKS_PER_SEC * 1000;
        results[n_results].mse = 0;
        strcpy(results[n_results].method, "Delta+Ternary");
        n_results++;
        free(buf1);
        
        /* RLE */
        int* buf2 = malloc(n * 2 * sizeof(int));
        start = clock();
        int sz2 = compress_rle_ternary(data, n, 1.0, buf2);
        end = clock();
        results[n_results].original_bytes = n * sizeof(float);
        results[n_results].compressed_bytes = sz2 * sizeof(int);
        results[n_results].ratio = (float)results[n_results].compressed_bytes / results[n_results].original_bytes;
        results[n_results].time_ms = (float)(end - start) / CLOCKS_PER_SEC * 1000;
        results[n_results].mse = 0;
        strcpy(results[n_results].method, "RLE");
        n_results++;
        free(buf2);
        
        /* Block */
        int* buf3 = malloc(n * (precision / 5 + 2) * sizeof(int));
        start = clock();
        int sz3 = compress_block_ternary(data, n, 32, buf3);
        end = clock();
        results[n_results].original_bytes = n * sizeof(float);
        results[n_results].compressed_bytes = sz3 * sizeof(int);
        results[n_results].ratio = (float)results[n_results].compressed_bytes / results[n_results].original_bytes;
        results[n_results].time_ms = (float)(end - start) / CLOCKS_PER_SEC * 1000;
        results[n_results].mse = 0;
        strcpy(results[n_results].method, "Block(32)");
        n_results++;
        free(buf3);
        
        /* Adaptive */
        int* buf4 = malloc(n * (precision / 5 + 2) * sizeof(int));
        start = clock();
        int sz4 = compress_adaptive_ternary(data, n, buf4);
        end = clock();
        results[n_results].original_bytes = n * sizeof(float);
        results[n_results].compressed_bytes = sz4 * sizeof(int);
        results[n_results].ratio = (float)results[n_results].compressed_bytes / results[n_results].original_bytes;
        results[n_results].time_ms = (float)(end - start) / CLOCKS_PER_SEC * 1000;
        results[n_results].mse = 0;
        strcpy(results[n_results].method, "Adaptive");
        n_results++;
        free(buf4);
        
        /* Zlib */
        results[n_results] = compress_zlib_opt(data, n);
        n_results++;
        
        print_comparison(results, n_results);
        export_results_json(results, n_results, "compression_comparison.json");
        printf("\nResults exported to compression_comparison.json\n");
        
        free(data);
        
    } else {
        compress_help();
        return 1;
    }
    
    return 0;
}
