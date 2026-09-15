/**
 * crypto_ternary.c — Criptografía Ternaria
 * 
 * Implementa:
 * - RSA ternario con anillo T₃
 * - Diffie-Hellman ternario para key exchange
 * - Generador de números pseudo-random ternarios
 * - Métricas de seguridad para cifrado ternario
 * 
 * El anillo T₃ usa aritmética balanced ternary:
 * - Operaciones modulares ternarias
 * - Exponenciación modulando en ternario
 * - Primos ternarios para generación de claves
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* Ternary number */
typedef struct {
    int* trits;
    int n_trits;
    int sign;
} ternary_num_t;

/* Ternary arithmetic in balanced ternary */
typedef struct {
    int p;  /* Modulus */
    int g;  /* Generator */
    int q;  /* Order of group */
} ternary_group_t;

/* Create ternary number from integer */
ternary_num_t* ternary_num_from_int(int value, int precision) {
    ternary_num_t* num = malloc(sizeof(ternary_num_t));
    if (!num) return NULL;
    
    num->n_trits = precision;
    num->trits = calloc(precision, sizeof(int));
    num->sign = (value >= 0) ? 1 : -1;
    
    int v = abs(value);
    for (int i = 0; i < precision; i++) {
        int power = 1;
        for (int p = 0; p < i; p++) power *= 3;
        
        int digit = v / power;
        v = v % power;
        
        /* Convert to balanced ternary */
        if (digit == 2) {
            num->trits[i] = -1;
            v += power;
        } else {
            num->trits[i] = digit;
        }
    }
    
    return num;
}

/* Convert ternary to integer */
int ternary_num_to_int(ternary_num_t* num) {
    int value = 0;
    int power = 1;
    
    for (int i = 0; i < num->n_trits; i++) {
        value += num->trits[i] * power;
        power *= 3;
    }
    
    return value * num->sign;
}

/* Add two ternary numbers */
ternary_num_t* ternary_num_add(ternary_num_t* a, ternary_num_t* b) {
    int max_trits = (a->n_trits > b->n_trits) ? a->n_trits : b->n_trits;
    ternary_num_t* result = ternary_num_from_int(0, max_trits + 1);
    
    int carry = 0;
    for (int i = 0; i < max_trits; i++) {
        int ai = (i < a->n_trits) ? a->trits[i] : 0;
        int bi = (i < b->n_trits) ? b->trits[i] : 0;
        int sum = ai + bi + carry;
        
        if (sum > 1) {
            result->trits[i] = sum - 3;
            carry = 1;
        } else if (sum < -1) {
            result->trits[i] = sum + 3;
            carry = -1;
        } else {
            result->trits[i] = sum;
            carry = 0;
        }
    }
    
    return result;
}

/* Multiply ternary numbers */
ternary_num_t* ternary_num_mul(ternary_num_t* a, ternary_num_t* b) {
    int val_a = ternary_num_to_int(a);
    int val_b = ternary_num_to_int(b);
    return ternary_num_from_int(val_a * val_b, a->n_trits + b->n_trits);
}

/* Modular exponentiation in ternary (with overflow protection) */
int ternary_mod_pow(int base, int exp, int mod) {
    long long result = 1;
    long long b = base % mod;
    
    while (exp > 0) {
        if (exp % 2 == 1) {
            result = (result * b) % mod;
        }
        exp = exp >> 1;
        b = (b * b) % mod;
    }
    
    return (int)result;
}

/* Extended Euclidean algorithm */
int extended_gcd(int a, int b, int* x, int* y) {
    if (a == 0) {
        *x = 0;
        *y = 1;
        return b;
    }
    
    int x1, y1;
    int gcd = extended_gcd(b % a, a, &x1, &y1);
    
    *x = y1 - (b / a) * x1;
    *y = x1;
    
    return gcd;
}

/* Modular inverse */
int mod_inverse(int a, int m) {
    int x, y;
    int g = extended_gcd(a, m, &x, &y);
    
    if (g != 1) return -1;  /* No inverse */
    return (x % m + m) % m;
}

/* Calculate GCD */
int gcd(int a, int b) {
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    }
    return a;
}

/* Check if number is prime */
int is_prime(int n) {
    if (n <= 1) return 0;
    if (n <= 3) return 1;
    if (n % 2 == 0 || n % 3 == 0) return 0;
    
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return 0;
    }
    
    return 1;
}

/* Find next prime */
int next_prime(int n) {
    while (!is_prime(n)) n++;
    return n;
}

/* ============================================================
 * RSA Ternary
 * ============================================================ */

typedef struct {
    int p;          /* First prime */
    int q;          /* Second prime */
    int n;          /* Modulus: p * q */
    int phi;        /* Euler's totient: (p-1)(q-1) */
    int e;          /* Public exponent */
    int d;          /* Private exponent */
    int key_size;   /* Key size in trits */
} rsa_ternary_t;

/* Generate RSA ternary key pair */
rsa_ternary_t* rsa_ternary_keygen(int key_bits) {
    rsa_ternary_t* key = malloc(sizeof(rsa_ternary_t));
    if (!key) return NULL;
    
    /* Generate primes */
    int p_bits = key_bits / 2;
    int q_bits = key_bits - p_bits;
    
    key->p = next_prime(100 + rand() % (1 << p_bits));
    key->q = next_prime(100 + rand() % (1 << q_bits));
    
    /* Calculate modulus */
    key->n = key->p * key->q;
    
    /* Calculate Euler's totient */
    key->phi = (key->p - 1) * (key->q - 1);
    
    /* Choose public exponent (coprime to phi and smaller than phi) */
    key->e = 3;  /* Start with small prime */
    while (gcd(key->e, key->phi) != 1 || key->e >= key->phi) {
        key->e += 2;
        if (key->e >= key->phi) {
            /* Regenerate primes if e is too large */
            key->p = next_prime(100 + rand() % (1 << p_bits));
            key->q = next_prime(100 + rand() % (1 << q_bits));
            key->n = key->p * key->q;
            key->phi = (key->p - 1) * (key->q - 1);
            key->e = 3;
        }
    }
    
    /* Calculate private exponent */
    key->d = mod_inverse(key->e, key->phi);
    
    /* Key size in trits: log3(n) */
    key->key_size = (int)(log(key->n) / log(3)) + 1;
    
    return key;
}

/* Encrypt with RSA ternary */
int rsa_ternary_encrypt(rsa_ternary_t* key, int plaintext) {
    return ternary_mod_pow(plaintext, key->e, key->n);
}

/* Decrypt with RSA ternary */
int rsa_ternary_decrypt(rsa_ternary_t* key, int ciphertext) {
    return ternary_mod_pow(ciphertext, key->d, key->n);
}

/* Export RSA key as JSON */
void rsa_ternary_export_json(rsa_ternary_t* key, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "{\n");
    fprintf(f, "  \"type\": \"RSA_Ternary\",\n");
    fprintf(f, "  \"p\": %d,\n", key->p);
    fprintf(f, "  \"q\": %d,\n", key->q);
    fprintf(f, "  \"n\": %d,\n", key->n);
    fprintf(f, "  \"phi\": %d,\n", key->phi);
    fprintf(f, "  \"e\": %d,\n", key->e);
    fprintf(f, "  \"d\": %d,\n", key->d);
    fprintf(f, "  \"key_size_trits\": %d,\n", key->key_size);
    fprintf(f, "  \"security_bits\": %.1f\n", log2(key->n));
    fprintf(f, "}\n");
    
    fclose(f);
}

/* ============================================================
 * Diffie-Hellman Ternary
 * ============================================================ */

typedef struct {
    int p;          /* Prime modulus */
    int g;          /* Generator */
    int private_key;
    int public_key;
} dh_ternary_t;

/* Initialize DH parameters */
dh_ternary_t* dh_ternary_init(int prime_bits) {
    dh_ternary_t* dh = malloc(sizeof(dh_ternary_t));
    if (!dh) return NULL;
    
    /* Choose prime */
    dh->p = next_prime(100 + rand() % (1 << prime_bits));
    
    /* Choose generator (small, primitive root mod p) */
    dh->g = 2;
    while (ternary_mod_pow(dh->g, (dh->p - 1) / 2, dh->p) == 1) {
        dh->g++;
    }
    
    /* Generate private key */
    dh->private_key = 2 + rand() % (dh->p - 2);
    
    /* Calculate public key */
    dh->public_key = ternary_mod_pow(dh->g, dh->private_key, dh->p);
    
    return dh;
}

/* Calculate shared secret */
int dh_ternary_shared_secret(dh_ternary_t* local, dh_ternary_t* remote) {
    return ternary_mod_pow(remote->public_key, local->private_key, local->p);
}

/* Export DH parameters as JSON */
void dh_ternary_export_json(dh_ternary_t* dh, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "{\n");
    fprintf(f, "  \"type\": \"DiffieHellman_Ternary\",\n");
    fprintf(f, "  \"p\": %d,\n", dh->p);
    fprintf(f, "  \"g\": %d,\n", dh->g);
    fprintf(f, "  \"private_key\": %d,\n", dh->private_key);
    fprintf(f, "  \"public_key\": %d,\n", dh->public_key);
    fprintf(f, "  \"security_bits\": %.1f\n", log2(dh->p));
    fprintf(f, "}\n");
    
    fclose(f);
}

/* ============================================================
 * Ternary PRNG
 * ============================================================ */

typedef struct {
    int state[3];   /* 3-trit state */
    int counter;
} ternary_prng_t;

/* Initialize PRNG */
ternary_prng_t* ternary_prng_create(int seed) {
    ternary_prng_t* prng = malloc(sizeof(ternary_prng_t));
    if (!prng) return NULL;
    
    /* Seed the state */
    prng->state[0] = seed % 3;
    prng->state[1] = (seed / 3) % 3;
    prng->state[2] = (seed / 9) % 3;
    prng->counter = 0;
    
    return prng;
}

/* Generate next ternary value */
int ternary_prng_next(ternary_prng_t* prng) {
    /* LFSR-like update */
    int feedback = (prng->state[0] + prng->state[1] + prng->state[2]) % 3;
    
    prng->state[0] = prng->state[1];
    prng->state[1] = prng->state[2];
    prng->state[2] = feedback;
    
    prng->counter++;
    
    return prng->state[2] - 1;  /* Return -1, 0, or +1 */
}

/* Generate random integer in range */
int ternary_prng_range(ternary_prng_t* prng, int min, int max) {
    int range = max - min + 1;
    int value = 0;
    
    /* Generate enough trits for the range */
    int trits_needed = (int)(log(range) / log(3)) + 1;
    for (int i = 0; i < trits_needed; i++) {
        int trit = ternary_prng_next(prng);
        value = value * 3 + (trit + 1);
    }
    
    return min + (value % range);
}

/* Export PRNG state as JSON */
void ternary_prng_export_json(ternary_prng_t* prng, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "{\n");
    fprintf(f, "  \"type\": \"Ternary_PRNG\",\n");
    fprintf(f, "  \"state\": [%d, %d, %d],\n", 
            prng->state[0], prng->state[1], prng->state[2]);
    fprintf(f, "  \"counter\": %d\n", prng->counter);
    fprintf(f, "}\n");
    
    fclose(f);
}

/* Free functions */
void ternary_num_free(ternary_num_t* num) {
    if (num) {
        free(num->trits);
        free(num);
    }
}

void rsa_ternary_free(rsa_ternary_t* key) {
    free(key);
}

void dh_ternary_free(dh_ternary_t* dh) {
    free(dh);
}

void ternary_prng_free(ternary_prng_t* prng) {
    free(prng);
}

/* ============================================================
 * CLI Interface
 * ============================================================ */

int crypto_main(int argc, char* argv[]) {
    printf("Ternary Cryptography Suite\n");
    printf("==========================\n\n");
    
    srand(time(NULL));
    
    /* RSA Ternary Demo */
    printf("--- RSA Ternary ---\n");
    rsa_ternary_t* rsa_key = rsa_ternary_keygen(32);
    printf("Generated RSA key pair:\n");
    printf("  p = %d, q = %d\n", rsa_key->p, rsa_key->q);
    printf("  n = %d (modulus)\n", rsa_key->n);
    printf("  e = %d (public exponent)\n", rsa_key->e);
    printf("  d = %d (private exponent)\n", rsa_key->d);
    printf("  Key size: %d trits\n", rsa_key->key_size);
    printf("  Security: %.1f bits\n", log2(rsa_key->n));
    
    /* Test encryption/decryption */
    int message = 42;
    int encrypted = rsa_ternary_encrypt(rsa_key, message);
    int decrypted = rsa_ternary_decrypt(rsa_key, encrypted);
    printf("\n  Test: message=%d → encrypted=%d → decrypted=%d\n", 
           message, encrypted, decrypted);
    printf("  %s\n", message == decrypted ? "SUCCESS" : "FAILED");
    
    rsa_ternary_export_json(rsa_key, "rsa_ternary.json");
    printf("  Key exported to rsa_ternary.json\n\n");
    
    rsa_ternary_free(rsa_key);
    
    /* Diffie-Hellman Ternary Demo */
    printf("--- Diffie-Hellman Ternary ---\n");
    
    /* Generate common parameters */
    int dh_p = next_prime(100 + rand() % (1 << 16));
    int dh_g = 2;
    while (ternary_mod_pow(dh_g, (dh_p - 1) / 2, dh_p) == 1) {
        dh_g++;
    }
    printf("Common params: p=%d, g=%d\n", dh_p, dh_g);
    
    /* Alice */
    dh_ternary_t* alice = malloc(sizeof(dh_ternary_t));
    alice->p = dh_p;
    alice->g = dh_g;
    alice->private_key = 2 + rand() % (dh_p - 2);
    alice->public_key = ternary_mod_pow(dh_g, alice->private_key, dh_p);
    
    /* Bob */
    dh_ternary_t* bob = malloc(sizeof(dh_ternary_t));
    bob->p = dh_p;
    bob->g = dh_g;
    bob->private_key = 2 + rand() % (dh_p - 2);
    bob->public_key = ternary_mod_pow(dh_g, bob->private_key, dh_p);
    
    /* Exchange public keys */
    int secret_alice = dh_ternary_shared_secret(alice, bob);
    int secret_bob = dh_ternary_shared_secret(bob, alice);
    
    printf("Alice:\n");
    printf("  Private: %d, Public: %d\n", alice->private_key, alice->public_key);
    printf("Bob:\n");
    printf("  Private: %d, Public: %d\n", bob->private_key, bob->public_key);
    printf("Shared secrets:\n");
    printf("  Alice computed: %d\n", secret_alice);
    printf("  Bob computed: %d\n", secret_bob);
    printf("  %s\n", secret_alice == secret_bob ? "MATCH" : "MISMATCH");
    
    dh_ternary_export_json(alice, "dh_alice.json");
    dh_ternary_export_json(bob, "dh_bob.json");
    printf("  Keys exported to dh_alice.json, dh_bob.json\n\n");
    
    dh_ternary_free(alice);
    dh_ternary_free(bob);
    
    /* Ternary PRNG Demo */
    printf("--- Ternary PRNG ---\n");
    ternary_prng_t* prng = ternary_prng_create(time(NULL));
    
    printf("First 20 random trits: ");
    for (int i = 0; i < 20; i++) {
        printf("%+d ", ternary_prng_next(prng));
    }
    printf("\n");
    
    printf("Random numbers 0-100: ");
    for (int i = 0; i < 10; i++) {
        printf("%d ", ternary_prng_range(prng, 0, 100));
    }
    printf("\n");
    
    ternary_prng_export_json(prng, "prng_ternary.json");
    printf("  PRNG state exported to prng_ternary.json\n");
    
    ternary_prng_free(prng);
    
    return 0;
}
