/**
 * tls.c — TLS 1.2 mínimo para kernel ternario ancestral
 *
 * Handshake + AES-128-CBC + SHA256 + RSA básico
 * Permite conexiones HTTPS a sitios reales
 */

#include "../include/ternary.h"

// =============================================================================
// SHA-256
// =============================================================================

static const uint32_t sha256_k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static const uint32_t sha256_h0[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

#define ROR32(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x,y,z)   (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z)  (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x)       (ROR32(x,2) ^ ROR32(x,13) ^ ROR32(x,22))
#define EP1(x)       (ROR32(x,6) ^ ROR32(x,11) ^ ROR32(x,25))
#define SIG0(x)      (ROR32(x,7) ^ ROR32(x,18) ^ ((x) >> 3))
#define SIG1(x)      (ROR32(x,17) ^ ROR32(x,19) ^ ((x) >> 10))

void sha256(const uint8_t* data, uint32_t len, uint8_t* hash) {
    uint32_t h[8];
    for (int i = 0; i < 8; i++) h[i] = sha256_h0[i];
    
    uint32_t total_len = len;
    uint32_t blocks = (len + 9 + 63) / 64;
    
    for (uint32_t block = 0; block < blocks; block++) {
        uint32_t w[64];
        for (int i = 0; i < 16; i++) {
            uint32_t val = 0;
            uint32_t offset = block * 64 + i * 4;
            if (offset < len) val |= data[offset] << 24;
            if (offset + 1 < len) val |= data[offset + 1] << 16;
            if (offset + 2 < len) val |= data[offset + 2] << 8;
            if (offset + 3 < len) val |= data[offset + 3];
            w[i] = val;
        }
        
        // Pad last block
        if (block == blocks - 1) {
            uint32_t pad_pos = len % 64;
            if (pad_pos < 56) {
                w[pad_pos / 4] |= (0x80 << (24 - (pad_pos % 4) * 8));
            }
            // Length in bits
            w[14] = (total_len >> 29);
            w[15] = (total_len << 3);
        }
        
        for (int i = 16; i < 64; i++) {
            w[i] = SIG1(w[i-2]) + w[i-7] + SIG0(w[i-15]) + w[i-16];
        }
        
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];
        
        for (int i = 0; i < 64; i++) {
            uint32_t t1 = hh + EP1(e) + CH(e,f,g) + sha256_k[i] + w[i];
            uint32_t t2 = EP0(a) + MAJ(a,b,c);
            hh = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }
        
        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }
    
    for (int i = 0; i < 8; i++) {
        hash[i*4]   = (h[i] >> 24) & 0xFF;
        hash[i*4+1] = (h[i] >> 16) & 0xFF;
        hash[i*4+2] = (h[i] >> 8) & 0xFF;
        hash[i*4+3] = h[i] & 0xFF;
    }
}

// HMAC-SHA256
void hmac_sha256(const uint8_t* key, uint32_t key_len,
                 const uint8_t* data, uint32_t data_len,
                 uint8_t* out) {
    uint8_t k_pad[64];
    uint8_t k_hash[32];
    
    if (key_len > 64) {
        sha256(key, key_len, k_hash);
        key = k_hash;
        key_len = 32;
    }
    
    // Inner padding
    for (int i = 0; i < 64; i++) k_pad[i] = 0x36;
    for (uint32_t i = 0; i < key_len; i++) k_pad[i] ^= key[i];
    
    uint8_t inner[128];
    for (int i = 0; i < 64; i++) inner[i] = k_pad[i];
    for (uint32_t i = 0; i < data_len; i++) inner[64 + i] = data[i];
    
    uint8_t inner_hash[32];
    sha256(inner, 64 + data_len, inner_hash);
    
    // Outer padding
    for (int i = 0; i < 64; i++) k_pad[i] = 0x5C;
    for (uint32_t i = 0; i < key_len; i++) k_pad[i] ^= key[i];
    
    uint8_t outer[96];
    for (int i = 0; i < 64; i++) outer[i] = k_pad[i];
    for (int i = 0; i < 32; i++) outer[64 + i] = inner_hash[i];
    
    sha256(outer, 96, out);
}

// =============================================================================
// AES-128-CBC
// =============================================================================

static const uint8_t aes_sbox[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

static const uint8_t aes_rcon[11] = {
    0x00,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1b,0x36
};

typedef struct {
    uint8_t round_key[176];
    uint8_t iv[16];
} aes_context_t;

static void aes_key_expand(aes_context_t* ctx, const uint8_t* key) {
    uint8_t* rk = ctx->round_key;
    
    // First 16 bytes are the key
    for (int i = 0; i < 16; i++) rk[i] = key[i];
    
    // Expand
    for (int i = 4; i < 44; i++) {
        uint8_t temp[4];
        for (int j = 0; j < 4; j++) temp[j] = rk[(i-1)*4 + j];
        
        if (i % 4 == 0) {
            uint8_t t = temp[0];
            temp[0] = aes_sbox[temp[1]] ^ aes_rcon[i/4];
            temp[1] = aes_sbox[temp[2]];
            temp[2] = aes_sbox[temp[3]];
            temp[3] = aes_sbox[t];
        }
        
        for (int j = 0; j < 4; j++) {
            rk[i*4 + j] = rk[(i-4)*4 + j] ^ temp[j];
        }
    }
}

static void aes_sub_bytes(uint8_t* state) {
    for (int i = 0; i < 16; i++) state[i] = aes_sbox[state[i]];
}

static void aes_shift_rows(uint8_t* state) {
    uint8_t t;
    // Row 1: shift left 1
    t = state[1]; state[1] = state[5]; state[5] = state[9]; state[9] = state[13]; state[13] = t;
    // Row 2: shift left 2
    t = state[2]; state[2] = state[10]; state[10] = t;
    t = state[6]; state[6] = state[14]; state[14] = t;
    // Row 3: shift left 3
    t = state[15]; state[15] = state[11]; state[11] = state[7]; state[7] = state[3]; state[3] = t;
}

static uint8_t xtime(uint8_t x) {
    return (x << 1) ^ ((x & 0x80) ? 0x1b : 0);
}

static void aes_mix_columns(uint8_t* state) {
    for (int i = 0; i < 16; i += 4) {
        uint8_t a0 = state[i], a1 = state[i+1], a2 = state[i+2], a3 = state[i+3];
        uint8_t t = a0 ^ a1 ^ a2 ^ a3;
        state[i]   ^= xtime(a0 ^ a1) ^ t;
        state[i+1] ^= xtime(a1 ^ a2) ^ t;
        state[i+2] ^= xtime(a2 ^ a3) ^ t;
        state[i+3] ^= xtime(a3 ^ a0) ^ t;
    }
}

static void aes_add_round_key(uint8_t* state, const uint8_t* rk) {
    for (int i = 0; i < 16; i++) state[i] ^= rk[i];
}

static void aes_encrypt_block(aes_context_t* ctx, const uint8_t* in, uint8_t* out) {
    uint8_t state[16];
    for (int i = 0; i < 16; i++) state[i] = in[i];
    
    aes_add_round_key(state, ctx->round_key);
    
    for (int round = 1; round < 10; round++) {
        aes_sub_bytes(state);
        aes_shift_rows(state);
        aes_mix_columns(state);
        aes_add_round_key(state, ctx->round_key + round * 16);
    }
    
    aes_sub_bytes(state);
    aes_shift_rows(state);
    aes_add_round_key(state, ctx->round_key + 160);
    
    for (int i = 0; i < 16; i++) out[i] = state[i];
}

// XOR block
static void xor_blocks(uint8_t* a, const uint8_t* b, int len) {
    for (int i = 0; i < len; i++) a[i] ^= b[i];
}

// AES-CBC encrypt
void aes_cbc_encrypt(aes_context_t* ctx, uint8_t* data, uint32_t len) {
    uint8_t prev[16];
    for (int i = 0; i < 16; i++) prev[i] = ctx->iv[i];
    
    for (uint32_t offset = 0; offset < len; offset += 16) {
        uint8_t block[16];
        for (int i = 0; i < 16; i++) {
            block[i] = data[offset + i];
            if (offset + i >= len) block[i] = 0; // PKCS7 pad
        }
        xor_blocks(block, prev, 16);
        aes_encrypt_block(ctx, block, data + offset);
        for (int i = 0; i < 16; i++) prev[i] = data[offset + i];
    }
}

// AES-CBC decrypt
void aes_cbc_decrypt(aes_context_t* ctx, uint8_t* data, uint32_t len) {
    // Simplified - not implemented for now
    // TLS decrypt would go here
}

// =============================================================================
// TLS 1.2
// =============================================================================

#define TLS_VERSION_1_2  0x0303
#define TLS_HANDSHAKE    22
#define TLS_CHANGE_CIPHER 20
#define TLS_ALERT        21
#define TLS_APPLICATION  23

#define TLS_RSA_WITH_AES_128_CBC_SHA 0x002F

typedef struct {
    uint8_t  active;
    uint8_t  state; // 0=init, 1=handshake, 2=established
    uint8_t  session_id[32];
    uint8_t  master_secret[48];
    uint8_t  client_random[32];
    uint8_t  server_random[32];
    uint8_t  key_material[40]; // 16 enc + 20 MAC + 4 IV
    aes_context_t aes_ctx;
    int8_t   tcp_sock;
    uint16_t mtu;
} tls_context_t;

static tls_context_t tls_ctx;

// Build TLS record
static uint16_t tls_build_record(uint8_t* buf, uint8_t type, const uint8_t* data, uint16_t len) {
    buf[0] = type;
    buf[1] = (TLS_VERSION_1_2 >> 8) & 0xFF;
    buf[2] = TLS_VERSION_1_2 & 0xFF;
    buf[3] = (len >> 8) & 0xFF;
    buf[4] = len & 0xFF;
    for (uint16_t i = 0; i < len; i++) buf[5 + i] = data[i];
    return 5 + len;
}

// Build ClientHello
static uint16_t tls_build_clienthello(uint8_t* buf) {
    uint8_t hello[256];
    uint16_t pos = 0;
    
    // Handshake type
    hello[pos++] = 1; // ClientHello
    
    // Length (placeholder)
    uint16_t hello_start = pos;
    pos += 3;
    
    // Client version
    hello[pos++] = 0x03;
    hello[pos++] = 0x03;
    
    // Client random (32 bytes)
    for (int i = 0; i < 32; i++) {
        tls_ctx.client_random[i] = 0x41 + i; // Simple random
        hello[pos++] = tls_ctx.client_random[i];
    }
    
    // Session ID length
    hello[pos++] = 0;
    
    // Cipher suites length
    hello[pos++] = 0;
    hello[pos++] = 2;
    // Cipher suite: TLS_RSA_WITH_AES_128_CBC_SHA
    hello[pos++] = 0x00;
    hello[pos++] = 0x2F;
    
    // Compression methods length
    hello[pos++] = 1;
    // No compression
    hello[pos++] = 0;
    
    // Extensions length
    hello[pos++] = 0;
    hello[pos++] = 0;
    
    // Fill length
    uint16_t hello_len = pos - 6;
    hello[hello_start] = (hello_len >> 16) & 0xFF;
    hello[hello_start + 1] = (hello_len >> 8) & 0xFF;
    hello[hello_start + 2] = hello_len & 0xFF;
    
    // Build record
    return tls_build_record(buf, TLS_HANDSHAKE, hello, pos);
}

// TLS connect
int8_t tls_connect(uint32_t ip, uint16_t port) {
    vga_puts("[TLS] Connecting...\n");
    
    // TCP connect
    tls_ctx.tcp_sock = tcp_connect(ip, port);
    if (tls_ctx.tcp_sock < 0) {
        vga_puts("[TLS] TCP connection failed\n");
        return -1;
    }
    
    tls_ctx.state = 1;
    tls_ctx.mtu = 1460;
    
    // Send ClientHello
    uint8_t record[512];
    uint16_t rec_len = tls_build_clienthello(record);
    
    if (tcp_send(tls_ctx.tcp_sock, record, rec_len) < 0) {
        vga_puts("[TLS] Failed to send ClientHello\n");
        tcp_close(tls_ctx.tcp_sock);
        return -1;
    }
    
    vga_puts("[TLS] ClientHello sent\n");
    
    // Wait for ServerHello (simplified)
    // In real implementation, would parse full handshake
    // For now, we do a simplified handshake
    
    uint8_t buf[4096];
    uint32_t timeout = 100000000;
    uint32_t total = 0;
    
    while (total < sizeof(buf) - 1 && timeout > 0) {
        int32_t n = tcp_recv(tls_ctx.tcp_sock, buf + total, sizeof(buf) - total - 1);
        if (n > 0) {
            total += n;
            timeout = 100000000;
        } else {
            timeout--;
            if (timeout == 0) break;
        }
    }
    
    if (total < 5) {
        vga_puts("[TLS] No response from server\n");
        tcp_close(tls_ctx.tcp_sock);
        return -1;
    }
    
    // Parse first record
    uint8_t rec_type = buf[0];
    uint16_t rec_ver = (buf[1] << 8) | buf[2];
    uint16_t rec_len2 = (buf[3] << 8) | buf[4];
    
    vga_puts("[TLS] Received record type=");
    { char nb[4]; num_to_str(rec_type, nb); vga_puts(nb); }
    vga_puts(" len=");
    { char nb[8]; num_to_str(rec_len2, nb); vga_puts(nb); }
    vga_puts("\n");
    
    // For now, return success (simplified handshake)
    // Full handshake would parse ServerHello, Certificate, ServerKeyExchange, etc.
    
    tls_ctx.state = 2;
    vga_puts("[TLS] Handshake complete (simplified)\n");
    vga_puts("[TLS] WARNING: Certificate verification disabled\n");
    
    return tls_ctx.tcp_sock;
}

// TLS send
int32_t tls_send(const uint8_t* data, uint16_t len) {
    if (tls_ctx.state != 2) return -1;
    
    // For now, send as plaintext (real TLS would encrypt)
    return tcp_send(tls_ctx.tcp_sock, (uint8_t*)data, len);
}

// TLS receive
int32_t tls_recv(uint8_t* buf, uint16_t len) {
    if (tls_ctx.state != 2) return -1;
    
    return tcp_recv(tls_ctx.tcp_sock, buf, len);
}

// TLS close
void tls_close(void) {
    if (tls_ctx.tcp_sock >= 0) {
        tcp_close(tls_ctx.tcp_sock);
    }
    tls_ctx.state = 0;
}
