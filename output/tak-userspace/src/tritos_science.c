/**
 * tritos_science.c — Tritos Science Suite
 * 
 * Binario unificado que incluye todos los módulos científicos:
 * - Lattice Gas Automata ternario
 * - Ising Model ternario
 * - Dinámica Molecular ternaria
 * - Sensores agronómicos, sísmicos, ambientales
 * - Perceptrón y Hopfield ternarios
 * - Lógica ternaria
 * - Criptografía ternaria (RSA, DH, PRNG)
 * - Compresión de datos
 * - Visualización 3D (Blender)
 * - Audio ternario
 * - Compresión optimizada para sensores
 * - Render ternario para impresión 3D (STL)
 * 
 * Uso: ./tritos_science <comando> [opciones]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declarations of main functions */
extern int lga_main(int argc, char* argv[]);
extern int ising_main(int argc, char* argv[]);
extern int md_main(int argc, char* argv[]);
extern int agro_main(int argc, char* argv[]);
extern int seismic_main(int argc, char* argv[]);
extern int env_main(int argc, char* argv[]);
extern int perceptron_main(int argc, char* argv[]);
extern int hopfield_main(int argc, char* argv[]);
extern int logic_main(int argc, char* argv[]);
extern int crypto_main(int argc, char* argv[]);
extern int compression_main(int argc, char* argv[]);
extern int main(int argc, char* argv[]);  /* blender_ternary */
extern int audio_main(int argc, char* argv[]);
extern int compress_opt_main(int argc, char* argv[]);
extern int render_main(int argc, char* argv[]);

void print_help() {
    printf("Tritos Science Suite\n");
    printf("====================\n\n");
    printf("Available commands:\n\n");
    
    printf("Physics Simulations:\n");
    printf("  lga          - Lattice Gas Automata (ternary fluid dynamics)\n");
    printf("  ising        - Ising Model (ternary spin system)\n");
    printf("  md           - Molecular Dynamics (ternary coordinates)\n");
    printf("\n");
    
    printf("Sensor Networks:\n");
    printf("  agro         - Agronomic sensor network\n");
    printf("  seismic      - Seismic monitoring network\n");
    printf("  env          - Coupled environmental sensors\n");
    printf("\n");
    
    printf("Neural Networks:\n");
    printf("  perceptron   - Ternary perceptron\n");
    printf("  hopfield     - Ternary Hopfield network\n");
    printf("\n");
    
    printf("Logic & Classification:\n");
    printf("  logic        - Ternary logic and truth tables\n");
    printf("\n");
    
    printf("Cryptography:\n");
    printf("  crypto       - RSA, Diffie-Hellman, PRNG in ternary\n");
    printf("\n");
    
    printf("Data Compression:\n");
    printf("  compress     - Ternary compression vs gzip/bzip2\n");
    printf("  compress-opt - Optimized compression for sensor data\n");
    printf("\n");
    
    printf("Visualization & Audio:\n");
    printf("  blender      - Generate Blender 3D scripts\n");
    printf("  audio        - Audio ternary processing\n");
    printf("\n");
    
    printf("3D Printing (STL):\n");
    printf("  render lga    - LGA density field as heightmap STL\n");
    printf("  render ising  - Ising spin lattice as voxel STL\n");
    printf("  render md     - MD particles as sphere mesh STL\n");
    printf("  render sensor - Sensor network as heightmap STL\n");
    printf("\n");
    
    printf("Use '<command> -h' for help on each command.\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        return 0;
    }
    
    char* cmd = argv[1];
    
    /* Skip command name for subcommands */
    argv++;
    argc--;
    
    if (strcmp(cmd, "lga") == 0) {
        return lga_main(argc, argv);
    } else if (strcmp(cmd, "ising") == 0) {
        return ising_main(argc, argv);
    } else if (strcmp(cmd, "md") == 0) {
        return md_main(argc, argv);
    } else if (strcmp(cmd, "agro") == 0) {
        return agro_main(argc, argv);
    } else if (strcmp(cmd, "seismic") == 0) {
        return seismic_main(argc, argv);
    } else if (strcmp(cmd, "env") == 0) {
        return env_main(argc, argv);
    } else if (strcmp(cmd, "perceptron") == 0) {
        return perceptron_main(argc, argv);
    } else if (strcmp(cmd, "hopfield") == 0) {
        return hopfield_main(argc, argv);
    } else if (strcmp(cmd, "logic") == 0) {
        return logic_main(argc, argv);
    } else if (strcmp(cmd, "crypto") == 0) {
        return crypto_main(argc, argv);
    } else if (strcmp(cmd, "compress") == 0) {
        return compression_main(argc, argv);
    } else if (strcmp(cmd, "compress-opt") == 0) {
        return compress_opt_main(argc, argv);
    } else if (strcmp(cmd, "blender") == 0) {
        return blender_main(argc, argv);
    } else if (strcmp(cmd, "audio") == 0) {
        return audio_main(argc, argv);
    } else if (strcmp(cmd, "render") == 0) {
        return render_main(argc, argv);
    } else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "-h") == 0) {
        print_help();
        return 0;
    } else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        print_help();
        return 1;
    }
}
