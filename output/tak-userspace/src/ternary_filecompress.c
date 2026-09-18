/**
 * ternary_filecompress.c — Compresor Ternario Ancestral v7
 *
 * Modos:
 *   0 (ternary)  : Delta → Balanced Ternary → Zlib/LZMA
 *   4 (crt)      : CRT(mod 3,5,7,11) → Delta por stream → Ternary → Zlib/LZMA
 *
 * Estrategia smart: prueba LZMA, zlib, y ternary; usa el mejor.
 *
 * Uso: ./tritos_compress -c [-m mode] in out
 *      ./tritos_compress -d in.trc5 out
 *      ./tritos_compress -i file.trc5
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>
#include <lzma.h>

#define MAGIC      "TRC5"
#define VERSION    7

/* Compression types */
#define COMP_ZLIB   0
#define COMP_LZMA   1

#pragma pack(push,1)
typedef struct {
    char     magic[4];
    uint8_t  version;
    uint8_t  mode;
    uint8_t  comp_type;    /* 0=zlib, 1=lzma */
    uint8_t  _pad;
    uint32_t original_size;
    uint32_t compressed_size;
    uint32_t checksum;
    uint32_t ternary_count;
} header_t;
#pragma pack(pop)

static uint32_t crc_table[256];
static void crc_init(void) {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int j = 0; j < 8; j++) c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
        crc_table[i] = c;
    }
}
static uint32_t do_crc32(const uint8_t *d, size_t len) {
    uint32_t c = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) c = crc_table[(c ^ d[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFF;
}

/* ── Delta (lossless via wrapping arithmetic) ── */
static void delta_enc(const uint8_t *in, int8_t *out, size_t n) {
    if (!n) return;
    out[0] = (int8_t)((uint8_t)(in[0]) - 128u);
    for (size_t i = 1; i < n; i++) {
        out[i] = (int8_t)((uint8_t)(in[i]) - (uint8_t)(in[i-1]));
    }
}
static void delta_dec(const int8_t *in, uint8_t *out, size_t n) {
    if (!n) return;
    out[0] = (uint8_t)((int8_t)(in[0]) + 128);
    for (size_t i = 1; i < n; i++) {
        out[i] = (uint8_t)((int8_t)out[i-1] + (int8_t)in[i]);
    }
}

/* ── Delta on small-range values (for CRT residues 0..max) ── */
static void delta_enc_u8(const uint8_t *in, int8_t *out, size_t n) {
    if (!n) return;
    out[0] = (int8_t)in[0];
    for (size_t i = 1; i < n; i++) {
        int d = (int)in[i] - (int)in[i-1];
        out[i] = (int8_t)(d < -128 ? -128 : d > 127 ? 127 : d);
    }
}
static void delta_dec_u8(const int8_t *in, uint8_t *out, size_t n) {
    if (!n) return;
    out[0] = (uint8_t)in[0];
    for (size_t i = 1; i < n; i++) {
        int v = (int)out[i-1] + (int)in[i];
        out[i] = (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v);
    }
}

/* ── LZMA helpers ── */
static uint8_t *lzma_compress(const uint8_t *in, size_t in_size,
                              size_t *out_size) {
    lzma_stream stream = LZMA_STREAM_INIT;
    lzma_ret ret = lzma_easy_encoder(&stream, 6, LZMA_CHECK_CRC32);
    if (ret != LZMA_OK) return NULL;

    size_t out_buf_size = in_size + in_size / 4 + 256;
    uint8_t *out = malloc(out_buf_size);
    if (!out) { lzma_end(&stream); return NULL; }

    stream.next_in = in;
    stream.avail_in = in_size;
    stream.next_out = out;
    stream.avail_out = out_buf_size;

    ret = lzma_code(&stream, LZMA_FINISH);
    lzma_end(&stream);

    if (ret != LZMA_OK && ret != LZMA_STREAM_END) { free(out); return NULL; }
    *out_size = stream.total_out;
    return out;
}

static uint8_t *lzma_decompress(const uint8_t *in, size_t in_size,
                                size_t out_size) {
    lzma_stream stream = LZMA_STREAM_INIT;
    lzma_ret ret = lzma_stream_decoder(&stream, UINT64_MAX, 0);
    if (ret != LZMA_OK) return NULL;

    uint8_t *out = malloc(out_size);
    if (!out) { lzma_end(&stream); return NULL; }

    stream.next_in = in;
    stream.avail_in = in_size;
    stream.next_out = out;
    stream.avail_out = out_size;

    ret = lzma_code(&stream, LZMA_FINISH);
    lzma_end(&stream);

    if (ret != LZMA_OK && ret != LZMA_STREAM_END) { free(out); return NULL; }
    return out;
}

/* ── Balanced Ternary ── */
static int trits_for_value(int v) {
    if (v < 0) v = -v;
    if (v <= 1) return 1;
    if (v <= 4) return 2;
    if (v <= 13) return 3;
    if (v <= 40) return 4;
    if (v <= 121) return 5;
    return 6;
}

static void value_to_trits(int val, int *t, int n) {
    for (int i = 0; i < n; i++) {
        int r = val % 3;
        if (r == 2) { t[i] = -1; val = val / 3 + 1; }
        else if (r == -2) { t[i] = 1; val = val / 3 - 1; }
        else { t[i] = r; val = val / 3; }
    }
}

static int trits_to_value(const int *t, int n) {
    int v = 0, p = 1;
    for (int i = 0; i < n; i++) { v += t[i] * p; p *= 3; }
    return v;
}

/* ── Bitstream writer ── */
typedef struct { uint8_t *buf; size_t byte; int bits; } bitwriter_t;

static void bw_init(bitwriter_t *w, uint8_t *buf) {
    w->buf = buf; w->byte = 0; w->bits = 0;
}

static void bw_write(bitwriter_t *w, int value, int nbits) {
    for (int i = 0; i < nbits; i++) {
        if ((value >> i) & 1) w->buf[w->byte] |= (1 << w->bits);
        w->bits++;
        if (w->bits >= 8) { w->byte++; w->bits = 0; }
    }
}

static size_t bw_done(bitwriter_t *w) {
    return w->byte + (w->bits > 0 ? 1 : 0);
}

/* ── Bitstream reader ── */
typedef struct { const uint8_t *buf; size_t byte; int bits; } bitreader_t;

static void br_init(bitreader_t *r, const uint8_t *buf) {
    r->buf = buf; r->byte = 0; r->bits = 0;
}

static int br_read(bitreader_t *r, int nbits) {
    int v = 0;
    for (int i = 0; i < nbits; i++) {
        if ((r->buf[r->byte] >> r->bits) & 1) v |= (1 << i);
        r->bits++;
        if (r->bits >= 8) { r->byte++; r->bits = 0; }
    }
    return v;
}

/* ── Ternary compress/decompress ── */
static uint8_t *ternary_compress(const int8_t *delta, size_t n,
                                  size_t *out_len, uint32_t *total_trits) {
    size_t buf_size = n * 2 + 1024;
    if (buf_size < 65536) buf_size = 65536;
    uint8_t *buf = calloc(1, buf_size);
    if (!buf) return NULL;
    bitwriter_t w;
    bw_init(&w, buf);
    uint32_t tc = 0;

    for (size_t i = 0; i < n; i++) {
        int val = (int)delta[i];
        int nt = trits_for_value(val);
        int trits[6] = {0};
        value_to_trits(val, trits, nt);

        bw_write(&w, nt - 1, 3);
        for (int t = 0; t < nt; t++) {
            bw_write(&w, trits[t] + 1, 2);
        }
        tc += nt;
    }

    *out_len = bw_done(&w);
    *total_trits = tc;
    return buf;
}

static int8_t *ternary_decompress(const uint8_t *buf, size_t out_n) {
    int8_t *out = malloc(out_n);
    if (!out) return NULL;
    bitreader_t r;
    br_init(&r, buf);

    for (size_t i = 0; i < out_n; i++) {
        int nt = br_read(&r, 3) + 1;
        int trits[6] = {0};
        for (int t = 0; t < nt; t++) {
            trits[t] = br_read(&r, 2) - 1;
        }
        out[i] = (int8_t)trits_to_value(trits, nt);
    }
    return out;
}

/* ══════════════════════════════════════════════════════════════
 * CRT — Teorema del Resto Chino (Ancestral China, ~300 AD)
 *
 * Modulos coprimos: 3, 5, 7, 11
 * Producto: 3×5×7×11 = 1155 > 256 (cubre todo byte)
 *
 * Reconstruccion: x = (r1·385 + r2·231 + r3·330 + r4·210) mod 1155
 *   M1=5·7·11=385,  385 mod 3 = 1,  inv1 = 1
 *   M2=3·7·11=231,  231 mod 5 = 1,  inv2 = 1
 *   M3=3·5·11=165,  165 mod 7 = 4,  inv3 = 2  (4·2=8≡1 mod7)
 *   M4=3·5·7=105,   105 mod 11 = 6, inv4 = 2  (6·2=12≡1 mod11)
 * ══════════════════════════════════════════════════════════════ */

#define CRT_N 4
static const uint8_t  crt_p[CRT_N]  = {3, 5, 7, 11};
static const uint32_t crt_Mi[CRT_N] = {385, 231, 330, 210};
static const uint32_t crt_M  = 1155;

static void crt_encode(const uint8_t *in, uint8_t **residues, size_t n) {
    for (int k = 0; k < CRT_N; k++) {
        residues[k] = malloc(n);
        if (!residues[k]) return;
        for (size_t i = 0; i < n; i++)
            residues[k][i] = in[i] % crt_p[k];
    }
}

static void crt_decode(uint8_t *out, uint8_t **residues, size_t n) {
    for (size_t i = 0; i < n; i++) {
        uint32_t x = 0;
        for (int k = 0; k < CRT_N; k++)
            x += (uint32_t)residues[k][i] * crt_Mi[k];
        out[i] = (uint8_t)(x % crt_M);
    }
}

/* ── Info ── */
static const char *mode_name(uint8_t m) {
    switch(m) {
        case 0: return "ternary";
        case 1: return "maya";
        case 2: return "persian";
        case 3: return "babylon";
        case 4: return "CRT-ternary";
        default: return "unknown";
    }
}

static void usage(void) {
    printf("Tritos Compress v7 — LZMA + Zlib + Ternary\n\n");
    printf("  ./tritos_compress -c [-m mode] in out\n");
    printf("  ./tritos_compress -d in.trc5 out\n");
    printf("  ./tritos_compress -i file.trc5\n\n");
    printf("Modes: ternary, crt\n");
    printf("Auto-selects best: LZMA, zlib, or ternary+zlib\n");
}

int main(int argc, char **argv) {
    crc_init();
    if (argc < 2) { usage(); return 1; }

    /* ── Info ── */
    if (strcmp(argv[1], "-i") == 0 && argc >= 3) {
        FILE *f = fopen(argv[2], "rb");
        if (!f) { perror("open"); return 1; }
        header_t h;
        if (fread(&h, sizeof(h), 1, f) != 1 ||
            (memcmp(h.magic, "TRC5", 4) && memcmp(h.magic, "TRC3", 4))) {
            printf("Not TRC5/TRC3\n"); fclose(f); return 1;
        }
        printf("File:       %s\n", argv[2]);
        printf("Mode:       %s (v%d)\n", mode_name(h.mode), h.version);
        printf("Original:   %u bytes\n", h.original_size);
        printf("Compressed: %u bytes\n", h.compressed_size);
        if (h.mode == 4) {
            uint32_t stream_sizes[CRT_N], stream_tc[CRT_N];
            if (fread(stream_sizes, 4, CRT_N, f) == CRT_N &&
                fread(stream_tc, 4, CRT_N, f) == CRT_N) {
                printf("CRT streams:\n");
                uint32_t total_tc = 0;
                for (int k = 0; k < CRT_N; k++) {
                    printf("  mod%-2d: %u bytes, %u trits\n",
                        crt_p[k], stream_sizes[k], stream_tc[k]);
                    total_tc += stream_tc[k];
                }
                printf("Trits total: %u (%.1f trits/byte)\n", total_tc,
                    h.original_size ? (float)total_tc / h.original_size : 0);
            }
        } else {
            printf("Trits:      %u (%.1f trits/byte)\n", h.ternary_count,
                h.original_size ? (float)h.ternary_count / h.original_size : 0);
        }
        if (h.compressed_size > 0)
            printf("Ratio:      %.2fx (%.1f%% saved)\n",
                (float)h.original_size / h.compressed_size,
                (1.0f - (float)h.compressed_size / h.original_size) * 100);
        printf("CRC32:      0x%08X\n", h.checksum);
        fclose(f);
        return 0;
    }

    /* ── Compress ── */
    if (strcmp(argv[1], "-c") == 0) {
        int ai = 2, mode = 0;
        if (ai < argc && strcmp(argv[ai], "-m") == 0) {
            ai++;
            if (ai < argc) {
                if (strcmp(argv[ai], "crt") == 0) mode = 4;
                else if (strcmp(argv[ai], "maya") == 0) mode = 1;
                else if (strcmp(argv[ai], "persian") == 0) mode = 2;
                else if (strcmp(argv[ai], "babylon") == 0) mode = 3;
            }
            ai++;
        }
        if (ai + 1 >= argc) { printf("Missing files\n"); return 1; }

        const char *inpath = argv[ai];
        int is_dir = 0;
        struct stat st;
        if (stat(inpath, &st) == 0 && S_ISDIR(st.st_mode)) is_dir = 1;

        uint8_t *data = NULL;
        long sz = 0;
        char tmptar[] = "/tmp/tritos_tar_XXXXXX";
        int tarfd = -1;

        if (is_dir) {
            /* Create temp tar from directory */
            tarfd = mkstemp(tmptar);
            if (tarfd < 0) { perror("mkstemp"); return 1; }
            close(tarfd);
            char cmd[1024];
            snprintf(cmd, sizeof(cmd), "tar cf %s -C '%s' . 2>/dev/null", tmptar, inpath);
            if (system(cmd) != 0) { unlink(tmptar); return 1; }

            FILE *ft = fopen(tmptar, "rb");
            if (!ft) { unlink(tmptar); return 1; }
            fseek(ft, 0, SEEK_END);
            sz = ftell(ft);
            fseek(ft, 0, SEEK_SET);
            data = malloc(sz);
            if (!data) { fclose(ft); unlink(tmptar); return 1; }
            if ((long)fread(data, 1, sz, ft) != sz) { free(data); fclose(ft); unlink(tmptar); return 1; }
            fclose(ft);
            unlink(tmptar);
            printf("Directory archived: %ld bytes (tar)\n", sz);
        } else {
            FILE *fin = fopen(inpath, "rb");
            if (!fin) { perror("open"); return 1; }
            fseek(fin, 0, SEEK_END);
            sz = ftell(fin);
            fseek(fin, 0, SEEK_SET);
            data = malloc(sz);
            if (!data) { fclose(fin); return 1; }
            if ((long)fread(data, 1, sz, fin) != sz) { free(data); fclose(fin); return 1; }
            fclose(fin);
        }

        uint32_t chk = do_crc32(data, sz);

        if (mode == 4) {
            /* ═══ CRT + Ternary ═══ */
            uint8_t *residues[CRT_N];
            crt_encode(data, residues, sz);

            size_t stream_tlen[CRT_N];
            uint32_t stream_tc[CRT_N];
            uint8_t *stream_tern[CRT_N];
            uint32_t total_tc = 0;

            for (int k = 0; k < CRT_N; k++) {
                int8_t *delta = malloc(sz);
                delta_enc_u8(residues[k], delta, sz);
                stream_tern[k] = ternary_compress(delta, sz, &stream_tlen[k], &stream_tc[k]);
                free(delta);
                total_tc += stream_tc[k];
            }

            /* Pack all ternary streams */
            size_t pack_total = 0;
            for (int k = 0; k < CRT_N; k++) pack_total += stream_tlen[k];
            uint8_t *packed = malloc(pack_total);
            size_t off = 0;
            for (int k = 0; k < CRT_N; k++) {
                memcpy(packed + off, stream_tern[k], stream_tlen[k]);
                off += stream_tlen[k];
            }

            uLongf zlen = compressBound(pack_total);
            uint8_t *zbuf = malloc(zlen);
            int rc = compress2(zbuf, &zlen, packed, pack_total, Z_DEFAULT_COMPRESSION);
            free(packed);
            if (rc != Z_OK) { printf("zlib error\n"); free(data); free(zbuf); return 1; }

            /* Header: standard (22) + CRT info (32) */
            size_t hdr_extra = CRT_N * 4 * 2;
            FILE *fout = fopen(argv[ai + 1], "wb");
            if (!fout) { free(data); free(zbuf); return 1; }
            header_t h;
            memcpy(h.magic, "TRC5", 4);
            h.version = VERSION;
            h.mode = 4;
            h.original_size = (uint32_t)sz;
            h.compressed_size = (uint32_t)(sizeof(h) + hdr_extra + zlen);
            h.checksum = chk;
            h.ternary_count = total_tc;
            fwrite(&h, sizeof(h), 1, fout);
            for (int k = 0; k < CRT_N; k++) {
                uint32_t tmp = (uint32_t)stream_tlen[k];
                fwrite(&tmp, 4, 1, fout);
            }
            for (int k = 0; k < CRT_N; k++)
                fwrite(&stream_tc[k], 4, 1, fout);
            fwrite(zbuf, 1, zlen, fout);
            fclose(fout);

            printf("Compressed: %ld -> %u bytes (%.2fx, %.1f%% saved)\n",
                sz, h.compressed_size,
                (float)sz / h.compressed_size,
                (1.0f - (float)h.compressed_size / sz) * 100);
            printf("CRT: %u trits total (%.1f trits/byte) [%s]\n", total_tc,
                (float)total_tc / sz, mode_name(mode));
            for (int k = 0; k < CRT_N; k++)
                printf("  mod%-2d: %u bytes, %u trits\n",
                    crt_p[k], (uint32_t)stream_tlen[k], stream_tc[k]);

            for (int k = 0; k < CRT_N; k++) { free(residues[k]); free(stream_tern[k]); }
            free(data); free(zbuf);
        } else {
            /* ═══ Smart Compress: LZMA vs zlib vs Ternary ═══ */
            size_t best_size = sz + 1;
            int best_comp = COMP_ZLIB;
            int best_is_ternary = 0;
            uint32_t best_tc = 0;

            /* Strategy 1: Pure zlib */
            uLongf zlen_raw = compressBound(sz);
            uint8_t *zbuf_raw = malloc(zlen_raw);
            int rc_raw = compress2(zbuf_raw, &zlen_raw, data, sz, Z_DEFAULT_COMPRESSION);
            if (rc_raw == Z_OK && zlen_raw < best_size) {
                best_size = zlen_raw;
                best_comp = COMP_ZLIB;
                best_is_ternary = 0;
                best_tc = 0;
            }

            /* Strategy 2: Pure LZMA */
            size_t lzma_len = 0;
            uint8_t *lzma_buf = lzma_compress(data, sz, &lzma_len);
            if (lzma_buf && lzma_len < best_size) {
                best_size = lzma_len;
                best_comp = COMP_LZMA;
                best_is_ternary = 0;
                best_tc = 0;
            }

            /* Strategy 3: Delta + Ternary + Zlib */
            int8_t *delta = malloc(sz);
            delta_enc(data, delta, sz);
            size_t tlen;
            uint32_t tc;
            uint8_t *tern = ternary_compress(delta, sz, &tlen, &tc);
            free(delta);

            uLongf zlen_tern = compressBound(tlen);
            uint8_t *zbuf_tern = malloc(zlen_tern);
            int rc_tern = compress2(zbuf_tern, &zlen_tern, tern, tlen, Z_DEFAULT_COMPRESSION);
            if (rc_tern == Z_OK && zlen_tern < best_size) {
                best_size = zlen_tern;
                best_comp = COMP_ZLIB;
                best_is_ternary = 1;
                best_tc = tc;
            }

            /* Write the best result */
            FILE *fout = fopen(argv[ai + 1], "wb");
            if (!fout) {
                free(data); free(zbuf_raw); free(lzma_buf);
                free(tern); free(zbuf_tern); return 1;
            }
            header_t h;
            memcpy(h.magic, "TRC5", 4);
            h.version = VERSION;
            h.mode = (uint8_t)mode;
            h.comp_type = (uint8_t)best_comp;
            h._pad = 0;
            h.original_size = (uint32_t)sz;
            h.compressed_size = (uint32_t)(sizeof(h) + best_size);
            h.checksum = chk;
            h.ternary_count = best_tc;
            fwrite(&h, sizeof(h), 1, fout);

            if (best_is_ternary) {
                fwrite(zbuf_tern, 1, best_size, fout);
            } else if (best_comp == COMP_LZMA) {
                fwrite(lzma_buf, 1, best_size, fout);
            } else {
                fwrite(zbuf_raw, 1, best_size, fout);
            }
            fclose(fout);

            printf("Compressed: %ld -> %u bytes (%.2fx, %.1f%% saved)\n",
                sz, h.compressed_size,
                (float)sz / h.compressed_size,
                (1.0f - (float)h.compressed_size / sz) * 100);
            if (best_is_ternary)
                printf("Trits: %u (%.1f trits/byte) [%s]\n", best_tc,
                    (float)best_tc / sz, mode_name(mode));
            else
                printf("Mode: %s [%s]\n",
                    best_comp == COMP_LZMA ? "LZMA" : "zlib", mode_name(mode));

            free(data); free(zbuf_raw); free(lzma_buf);
            free(tern); free(zbuf_tern);
        }
        return 0;
    }

    /* ── Decompress ── */
    if (strcmp(argv[1], "-d") == 0) {
        if (argc < 4) { printf("Usage: -d in.trc5 out\n"); return 1; }
        FILE *fin = fopen(argv[2], "rb");
        if (!fin) { perror("open"); return 1; }
        header_t h;
        if (fread(&h, sizeof(h), 1, fin) != 1 ||
            (memcmp(h.magic, "TRC5", 4) && memcmp(h.magic, "TRC3", 4))) {
            printf("Not TRC5/TRC3\n"); fclose(fin); return 1;
        }

        if (h.mode == 4) {
            /* ═══ CRT + Ternary decompress ═══ */
            uint32_t stream_tlen[CRT_N], stream_tc[CRT_N];
            if (fread(stream_tlen, 4, CRT_N, fin) != CRT_N) { fclose(fin); return 1; }
            if (fread(stream_tc, 4, CRT_N, fin) != CRT_N) { fclose(fin); return 1; }

            size_t hdr_extra = CRT_N * 4 * 2;
            size_t clen = h.compressed_size - sizeof(h) - hdr_extra;
            uint8_t *zbuf = malloc(clen);
            if (fread(zbuf, 1, clen, fin) != clen) { free(zbuf); fclose(fin); return 1; }
            fclose(fin);

            /* Unpack ternary streams */
            size_t pack_total = 0;
            for (int k = 0; k < CRT_N; k++) pack_total += stream_tlen[k];
            uint8_t *packed = malloc(pack_total);
            uLongf dlen = pack_total;
            int rc = uncompress(packed, &dlen, zbuf, clen);
            free(zbuf);
            if (rc != Z_OK) { printf("zlib error\n"); free(packed); return 1; }

            /* Ternary decode each stream → delta → residue */
            uint8_t *residues[CRT_N];
            size_t poff = 0;
            for (int k = 0; k < CRT_N; k++) {
                int8_t *delta = ternary_decompress(packed + poff, h.original_size);
                poff += stream_tlen[k];
                if (!delta) { free(packed); return 1; }

                residues[k] = malloc(h.original_size);
                delta_dec_u8(delta, residues[k], h.original_size);
                free(delta);
            }
            free(packed);

            /* CRT reconstruct */
            uint8_t *out = malloc(h.original_size);
            crt_decode(out, residues, h.original_size);
            for (int k = 0; k < CRT_N; k++) free(residues[k]);

            uint32_t chk = do_crc32(out, h.original_size);
            if (chk != h.checksum)
                printf("WARNING: checksum mismatch (got 0x%08X, expected 0x%08X)\n",
                    chk, h.checksum);
            else
                printf("Checksum OK\n");

            FILE *fout = fopen(argv[3], "wb");
            if (!fout) { free(out); return 1; }
            fwrite(out, 1, h.original_size, fout);
            fclose(fout);
            printf("Decompressed: %u bytes [CRT-ternary]\n", h.original_size);

            /* Check if output is a tar archive and extract */
            if (h.original_size > 262 && memcmp(out + 257, "ustar", 5) == 0) {
                printf("Detected tar archive, extracting...\n");
                char cmd[1024];
                snprintf(cmd, sizeof(cmd), "tar xf %s -C . 2>/dev/null", argv[3]);
                if (system(cmd) == 0) {
                    printf("Extracted to current directory\n");
                }
            }
            free(out);
        } else {
            /* ═══ Decompress: LZMA / zlib / Ternary ═══ */
            size_t clen = h.compressed_size - sizeof(h);
            uint8_t *cbuf = malloc(clen);
            if (fread(cbuf, 1, clen, fin) != clen) { free(cbuf); fclose(fin); return 1; }
            fclose(fin);

            uint8_t *out = malloc(h.original_size);
            if (!out) { free(cbuf); return 1; }

            if (h.comp_type == COMP_LZMA) {
                /* LZMA mode */
                uint8_t *tmp = lzma_decompress(cbuf, clen, h.original_size);
                free(cbuf);
                if (!tmp) { printf("LZMA error\n"); free(out); return 1; }
                memcpy(out, tmp, h.original_size);
                free(tmp);
            } else if (h.ternary_count == 0) {
                /* Pure zlib mode (no ternary encoding) */
                uLongf outlen = h.original_size;
                int rc = uncompress(out, &outlen, cbuf, clen);
                free(cbuf);
                if (rc != Z_OK) { printf("zlib error\n"); free(out); return 1; }
            } else {
                /* Ternary mode */
                uLongf tlen = h.original_size * 12;
                uint8_t *tern = malloc(tlen);
                int rc = uncompress(tern, &tlen, cbuf, clen);
                free(cbuf);
                if (rc != Z_OK) { printf("zlib error\n"); free(tern); free(out); return 1; }

                int8_t *delta = ternary_decompress(tern, h.original_size);
                free(tern);
                if (!delta) { free(out); return 1; }

                delta_dec(delta, out, h.original_size);
                free(delta);
            }

            uint32_t chk = do_crc32(out, h.original_size);
            if (chk != h.checksum)
                printf("WARNING: checksum mismatch\n");
            else
                printf("Checksum OK\n");

            FILE *fout = fopen(argv[3], "wb");
            if (!fout) { free(out); return 1; }
            fwrite(out, 1, h.original_size, fout);
            fclose(fout);
            printf("Decompressed: %u bytes [%s]\n", h.original_size, mode_name(h.mode));

            /* Check if output is a tar archive and extract */
            if (h.original_size > 262 && memcmp(out + 257, "ustar", 5) == 0) {
                printf("Detected tar archive, extracting...\n");
                char cmd[1024];
                snprintf(cmd, sizeof(cmd), "tar xf %s -C . 2>/dev/null && rm -f %s", argv[3], argv[3]);
                if (system(cmd) == 0) {
                    printf("Extracted to current directory\n");
                }
            }
            free(out);
        }
        return 0;
    }

    usage();
    return 1;
}
