/**
 * audio_ternary.c — Audio Ternario
 * 
 * Implementa compresión y procesamiento de audio en ternario:
 * - Conversión de samples a balanced ternary
 * - Compresión de audio (16-bit → ternary)
 * - Generación de tonos ternarios
 * - Análisis espectral ternario
 * 
 * Formato: WAV (PCM 16-bit mono/stereo)
 * Compresión: cada sample se convierte a N trits
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* WAV header */
typedef struct {
    char riff[4];
    int file_size;
    char wave[4];
    char fmt[4];
    int fmt_size;
    short audio_format;
    short num_channels;
    int sample_rate;
    int byte_rate;
    short block_align;
    short bits_per_sample;
    char data[4];
    int data_size;
} wav_header_t;

/* Convert sample to balanced ternary */
int* sample_to_ternary(short sample, int precision) {
    int* trits = calloc(precision, sizeof(int));
    
    /* Normalize to [-1, 1] */
    float value = sample / 32768.0;
    
    /* Convert to balanced ternary using correct algorithm */
    float v = fabs(value);
    int sign = (value >= 0) ? 1 : -1;
    
    /* First, convert to standard ternary (0, 1, 2) */
    int* temp = calloc(precision, sizeof(int));
    for (int i = 0; i < precision; i++) {
        float power = pow(3, i);
        float digit = v * 32768.0 / power;
        temp[i] = (int)(digit) % 3;
        v -= temp[i] * power / 32768.0;
    }
    
    /* Convert standard ternary to balanced ternary (-1, 0, 1) */
    int carry = 0;
    for (int i = 0; i < precision; i++) {
        int val = temp[i] + carry;
        if (val == 0) { trits[i] = 0; carry = 0; }
        else if (val == 1) { trits[i] = 1; carry = 0; }
        else if (val == 2) { trits[i] = -1; carry = 1; }
        else if (val == 3) { trits[i] = 0; carry = 1; }
        else { trits[i] = 0; carry = 0; }
    }
    
    /* Apply sign */
    if (sign < 0) {
        for (int i = 0; i < precision; i++) {
            trits[i] = -trits[i];
        }
    }
    
    free(temp);
    return trits;
}

/* Convert balanced ternary to sample */
short ternary_to_sample(int* trits, int precision) {
    float value = 0;
    
    for (int i = 0; i < precision; i++) {
        value += trits[i] * pow(3, i);
    }
    
    /* Scale back to [-32768, 32767] */
    float max_val = pow(3, precision) - 1;
    value = value / (max_val / 2.0) * 32768.0;
    
    /* Clamp */
    if (value > 32767) value = 32767;
    if (value < -32768) value = -32768;
    
    return (short)value;
}

/* Compress audio to ternary */
int compress_audio_ternary(short* samples, int n_samples, int precision, int* output) {
    int out_idx = 0;
    
    for (int i = 0; i < n_samples; i++) {
        int* trits = sample_to_ternary(samples[i], precision);
        
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

/* Decompress ternary to audio */
void decompress_audio_ternary(int* data, int n_packed, int precision, short* output) {
    int sample_idx = 0;
    int trit_idx = 0;
    int* trits = calloc(precision, sizeof(int));
    
    for (int i = 0; i < n_packed; i++) {
        int packed = data[i];
        
        for (int j = 0; j < 5 && trit_idx < precision; j++) {
            trits[trit_idx] = ((packed >> (j * 2)) & 0x3) - 1;
            trit_idx++;
        }
        
        if (trit_idx >= precision) {
            output[sample_idx] = ternary_to_sample(trits, precision);
            sample_idx++;
            trit_idx = 0;
        }
    }
    
    free(trits);
}

/* Generate ternary tone */
void generate_ternary_tone(float frequency, float duration, int sample_rate, 
                          int precision, short* output, int* n_samples) {
    *n_samples = (int)(duration * sample_rate);
    
    for (int i = 0; i < *n_samples; i++) {
        float t = (float)i / sample_rate;
        float value = sin(2 * M_PI * frequency * t);
        
        /* Convert to ternary and back (quantization) */
        int* trits = sample_to_ternary((short)(value * 32767), precision);
        output[i] = ternary_to_sample(trits, precision);
        free(trits);
    }
}

/* Analyze audio spectrum in ternary */
void analyze_audio_ternary(short* samples, int n_samples, int sample_rate, 
                          int* spectrum, int n_bins) {
    /* DFT block size = 512 for decent frequency resolution */
    int block_size = 512;
    if (block_size > n_samples) block_size = n_samples;
    
    for (int k = 0; k < n_bins; k++) {
        float real = 0, imag = 0;
        
        for (int n = 0; n < block_size; n++) {
            float angle = 2 * M_PI * k * n / block_size;
            real += samples[n] * cos(angle);
            imag -= samples[n] * sin(angle);
        }
        
        float magnitude = sqrt(real * real + imag * imag) / block_size;
        
        /* Convert magnitude to ternary */
        if (magnitude < 500) spectrum[k] = -1;
        else if (magnitude < 3000) spectrum[k] = 0;
        else spectrum[k] = 1;
    }
}

/* Read WAV file */
short* read_wav(const char* filename, int* n_samples, int* sample_rate, int* channels) {
    FILE* f = fopen(filename, "rb");
    if (!f) return NULL;
    
    /* Read header manually to avoid struct padding */
    char riff[4];
    fread(riff, 1, 4, f);
    int file_size;
    fread(&file_size, 4, 1, f);
    char wave[4];
    fread(wave, 1, 4, f);
    char fmt[4];
    fread(fmt, 1, 4, f);
    int fmt_size;
    fread(&fmt_size, 4, 1, f);
    short audio_format;
    fread(&audio_format, 2, 1, f);
    short num_channels;
    fread(&num_channels, 2, 1, f);
    int sample_rate_val;
    fread(&sample_rate_val, 4, 1, f);
    int byte_rate;
    fread(&byte_rate, 4, 1, f);
    short block_align;
    fread(&block_align, 2, 1, f);
    short bits_per_sample;
    fread(&bits_per_sample, 2, 1, f);
    char data[4];
    fread(data, 1, 4, f);
    int data_size;
    fread(&data_size, 4, 1, f);
    
    *sample_rate = sample_rate_val;
    *channels = num_channels;
    *n_samples = data_size / (bits_per_sample / 8);
    
    short* samples = malloc(*n_samples * sizeof(short));
    fread(samples, sizeof(short), *n_samples, f);
    
    fclose(f);
    return samples;
}

/* Write WAV file */
int write_wav(const char* filename, short* samples, int n_samples, 
              int sample_rate, int channels) {
    FILE* f = fopen(filename, "wb");
    if (!f) return -1;
    
    int byte_rate = sample_rate * channels * sizeof(short);
    int block_align = channels * sizeof(short);
    int data_size = n_samples * sizeof(short);
    
    /* Write header manually to avoid struct padding */
    fwrite("RIFF", 1, 4, f);
    int file_size = 36 + data_size;
    fwrite(&file_size, 4, 1, f);
    fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f);
    int fmt_size = 16;
    fwrite(&fmt_size, 4, 1, f);
    short audio_format = 1;
    fwrite(&audio_format, 2, 1, f);
    short num_channels = channels;
    fwrite(&num_channels, 2, 1, f);
    fwrite(&sample_rate, 4, 1, f);
    fwrite(&byte_rate, 4, 1, f);
    fwrite(&block_align, 2, 1, f);
    short bits_per_sample = 16;
    fwrite(&bits_per_sample, 2, 1, f);
    fwrite("data", 1, 4, f);
    fwrite(&data_size, 4, 1, f);
    
    fwrite(samples, sizeof(short), n_samples, f);
    
    fclose(f);
    return 0;
}

/* Print help */
void audio_help() {
    printf("Tritos Science — Audio Ternary\n");
    printf("==============================\n\n");
    printf("Commands:\n");
    printf("  compress <input.wav> <output.bin> [-p precision]\n");
    printf("  decompress <input.bin> <output.wav> [-p precision]\n");
    printf("  tone <freq> <duration> <output.wav> [-p precision]\n");
    printf("  analyze <input.wav>\n");
    printf("\nOptions:\n");
    printf("  -p <int>    Ternary precision (default: 16)\n");
}

int audio_main(int argc, char* argv[]) {
    if (argc < 2) {
        audio_help();
        return 0;
    }
    
    char* cmd = argv[1];
    int precision = 16;
    
    /* Parse precision */
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            precision = atoi(argv[++i]);
        }
    }
    
    if (strcmp(cmd, "compress") == 0 && argc >= 4) {
        int n_samples, sample_rate, channels;
        short* samples = read_wav(argv[2], &n_samples, &sample_rate, &channels);
        if (!samples) {
            fprintf(stderr, "Error: Could not read %s\n", argv[2]);
            return 1;
        }
        
        int* compressed = malloc(n_samples * (precision / 5 + 1) * sizeof(int));
        int n_compressed = compress_audio_ternary(samples, n_samples, precision, compressed);
        
        FILE* f = fopen(argv[3], "wb");
        fwrite(compressed, sizeof(int), n_compressed, f);
        fclose(f);
        
        printf("Compressed %d samples → %d integers (%.1f%%)\n",
               n_samples, n_compressed,
               100.0 * n_compressed * sizeof(int) / (n_samples * sizeof(short)));
        
        free(samples);
        free(compressed);
        
    } else if (strcmp(cmd, "decompress") == 0 && argc >= 4) {
        FILE* f = fopen(argv[2], "rb");
        fseek(f, 0, SEEK_END);
        int n_packed = ftell(f) / sizeof(int);
        rewind(f);
        
        int* compressed = malloc(n_packed * sizeof(int));
        fread(compressed, sizeof(int), n_packed, f);
        fclose(f);
        
        int n_samples = n_packed * 5 / precision;
        short* samples = malloc(n_samples * sizeof(short));
        decompress_audio_ternary(compressed, n_packed, precision, samples);
        
        write_wav(argv[3], samples, n_samples, 44100, 1);
        printf("Decompressed %d integers → %d samples\n", n_packed, n_samples);
        
        free(compressed);
        free(samples);
        
    } else if (strcmp(cmd, "tone") == 0 && argc >= 4) {
        float freq = atof(argv[2]);
        float duration = atof(argv[3]);
        
        int n_samples = (int)(duration * 44100);
        short* samples = malloc(n_samples * sizeof(short));
        generate_ternary_tone(freq, duration, 44100, precision, samples, &n_samples);
        
        char output[256];
        snprintf(output, sizeof(output), "%s", argv[4]);
        write_wav(output, samples, n_samples, 44100, 1);
        printf("Generated %.1fs tone at %.0f Hz → %s\n", duration, freq, output);
        
        free(samples);
        
    } else if (strcmp(cmd, "analyze") == 0 && argc >= 3) {
        int n_samples, sample_rate, channels;
        short* samples = read_wav(argv[2], &n_samples, &sample_rate, &channels);
        if (!samples) {
            fprintf(stderr, "Error: Could not read %s\n", argv[2]);
            return 1;
        }
        
        int n_bins = 64;
        int* spectrum = malloc(n_bins * sizeof(int));
        analyze_audio_ternary(samples, n_samples, sample_rate, spectrum, n_bins);
        
        printf("Audio Analysis (ternary spectrum):\n");
        printf("Samples: %d | Rate: %d Hz | Channels: %d\n", n_samples, sample_rate, channels);
        printf("Spectrum: ");
        for (int i = 0; i < n_bins; i++) {
            printf("%+d", spectrum[i]);
        }
        printf("\n");
        
        free(samples);
        free(spectrum);
        
    } else {
        audio_help();
        return 1;
    }
    
    return 0;
}
