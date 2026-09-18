/**
 * ac97.c — Driver AC97 para kernel ternario ancestral
 *
 * Audio básico via AC97 codec
 */

#include "../include/ternary.h"

// AC97 registers
#define AC97_RESET          0x00
#define AC97_MASTER_VOL     0x02
#define AC97_AUX_VOL        0x04
#define AC97_MASTER_MIC     0x06
#define AC97_LINE_OUT_VOL   0x06
#define AC97_CD_VOL         0x08
#define AC97_VIDEO_VOL      0x0A
#define AC97_PHONE_VOL      0x0C
#define AC97_LINE_IN_VOL    0x0E
#define AC97_MIC_VOL        0x0E
#define AC97_PCM_OUT_VOL    0x18
#define AC97_RECORD_SELECT  0x1A
#define AC97_RECORD_GAIN    0x1C
#define AC97_MIC_GAIN       0x1E
#define AC97_ADC_RATE       0x2C
#define AC97_DAC_RATE       0x2C
#define AC97_PLAYBACK_FMT   0x2E
#define AC97_FRONT_DAC_RATE 0x2C
#define AC97_SSR            0x2E
#define AC97_STATUS         0x2E
#define AC97_NOCACHE        0x2A
#define AC97_POWERDOWN      0x26

// AC97 DMA registers (BM + offset)
#define AC97_BDBAR          0x00  // Buffer descriptor base
#define AC97_CIV            0x04  // Current index
#define AC97_LVI            0x05  // Last valid index
#define AC97_SR             0x06  // Status register
#define AC97_PICB           0x08  // Position in current buffer
#define AC97_PCR            0x0A  // Playback control

// Buffer descriptor
typedef struct {
    uint32_t buffer_addr;
    uint16_t control;
    uint16_t length;
} __attribute__((packed)) ac97_buf_desc_t;

// AC97 state
static uint16_t ac97_nam = 0;     // NAM (native audio mix) base
static uint16_t ac97_nabm = 0;    // NABM (native audio bus master) base
static ac97_buf_desc_t* buf_desc = 0;
static int16_t* audio_buf = 0;
static uint8_t ac97_initialized = 0;
static uint32_t current_sample_rate = 44100;

// Write to NAM register
static void ac97_write(uint16_t reg, uint16_t val) {
    outw(ac97_nam + reg, val);
}

// Read from NAM register
static uint16_t ac97_read(uint16_t reg) {
    return inw(ac97_nam + reg);
}

// Write to NABM register
static void ac97_bm_write(uint16_t reg, uint8_t val) {
    outb(ac97_nabm + reg, val);
}

// Read from NABM register
static uint8_t ac97_bm_read(uint16_t reg) {
    return inb(ac97_nabm + reg);
}

// Reset AC97 codec
static void ac97_reset(void) {
    ac97_write(AC97_RESET, 0xFFFF);
    for (volatile int i = 0; i < 10000; i++) asm("nop");
}

// Set master volume (0-31 per channel)
void ac97_set_volume(uint8_t left, uint8_t right) {
    uint16_t vol = (left & 0x1F) | ((right & 0x1F) << 8);
    ac97_write(AC97_MASTER_VOL, vol);
}

// Set PCM volume
void ac97_set_pcm_volume(uint8_t left, uint8_t right) {
    uint16_t vol = (left & 0x1F) | ((right & 0x1F) << 8);
    ac97_write(AC97_PCM_OUT_VOL, vol);
}

// Set sample rate
void ac97_set_rate(uint32_t rate) {
    current_sample_rate = rate;
    ac97_write(AC97_FRONT_DAC_RATE, rate);
    ac97_write(AC97_ADC_RATE, rate);
}

// Play PCM buffer
void ac97_play_pcm(int16_t* buffer, uint32_t samples) {
    if (!ac97_initialized || !buf_desc) return;
    
    // Set buffer descriptor
    ac97_bm_write(AC97_BDBAR, 0);
    outl(ac97_nabm + AC97_BDBAR, (uint32_t)buf_desc);
    
    // Setup buffer descriptor
    buf_desc[0].buffer_addr = (uint32_t)buffer;
    buf_desc[0].control = 0x0000; // No interrupt
    buf_desc[0].length = samples * 2; // Bytes
    
    // Clear status
    ac97_bm_write(AC97_SR, 0x1F);
    
    // Set last valid index
    ac97_bm_write(AC97_LVI, 0);
    
    // Start playback
    ac97_bm_write(AC97_PCR, 0x01); // PAUSE = 0, RUN = 1
    
    // Wait for completion
    while (!(ac97_bm_read(AC97_SR) & 0x04)) {
        asm("nop");
    }
    
    // Stop
    ac97_bm_write(AC97_PCR, 0x00);
}

// Play a simple beep
void ac97_beep(uint32_t freq, uint32_t duration_ms) {
    uint32_t samples = current_sample_rate * duration_ms / 1000;
    int16_t* buf = (int16_t*)malloc(samples * 2);
    if (!buf) return;
    
    // Generate sine wave
    for (uint32_t i = 0; i < samples; i++) {
        uint32_t t = i * freq / current_sample_rate;
        int16_t val = 0;
        if ((t % 2) == 0) val = 8000;
        else val = -8000;
        buf[i] = val;
    }
    
    ac97_play_pcm(buf, samples);
    free(buf);
}

// Detect AC97 via PCI
static void ac97_detect_pci(void) {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            for (uint8_t function = 0; function < 8; function++) {
                uint32_t addr = 0x80000000 | (bus << 16) | (device << 11) | (function << 8);
                
                outl(0xCF8, addr);
                uint16_t vendor = inl(0xCFC) & 0xFFFF;
                if (vendor == 0xFFFF) continue;
                
                outl(0xCF8, addr + 0x08);
                uint32_t class = inl(0xCFC);
                uint8_t base_class = (class >> 24) & 0xFF;
                uint8_t sub_class = (class >> 16) & 0xFF;
                
                // Audio device: class 0x04, subclass 0x01
                if (base_class == 0x04 && sub_class == 0x01) {
                    // Read BAR0 (NAM) and BAR1 (NABM)
                    outl(0xCF8, addr + 0x10);
                    uint32_t bar0 = inl(0xCFC);
                    
                    outl(0xCF8, addr + 0x14);
                    uint32_t bar1 = inl(0xCFC);
                    
                    if (bar0 & 0x01) ac97_nam = bar0 & 0xFFFC;
                    if (bar1 & 0x01) ac97_nabm = bar1 & 0xFFFC;
                    
                    if (ac97_nam && ac97_nabm) {
                        vga_puts("[SOUND] Found AC97 at IO=0x");
                        { char nb[8]; num_to_hex(ac97_nam, nb); vga_puts(nb); }
                        vga_puts("\n");
                        return;
                    }
                }
            }
        }
    }
}

// Initialize AC97
void ac97_init(void) {
    vga_puts("[SOUND] Initializing AC97...\n");
    
    // Detect via PCI
    ac97_detect_pci();
    
    if (!ac97_nam || !ac97_nabm) {
        vga_puts("[SOUND] No AC97 codec found\n");
        return;
    }
    
    // Reset codec
    ac97_reset();
    
    // Set default volumes
    ac97_set_volume(20, 20);
    ac97_set_pcm_volume(20, 20);
    
    // Set sample rate
    ac97_set_rate(44100);
    
    // Allocate buffer descriptor (aligned)
    buf_desc = (ac97_buf_desc_t*)malloc(sizeof(ac97_buf_desc_t));
    audio_buf = (int16_t*)malloc(4096);
    
    if (!buf_desc || !audio_buf) {
        vga_puts("[SOUND] Failed to allocate buffers\n");
        return;
    }
    
    ac97_initialized = 1;
    
    vga_puts("[SOUND] AC97 initialized\n");
    vga_puts("[SOUND] Sample rate: ");
    { char nb[8]; num_to_str(current_sample_rate, nb); vga_puts(nb); }
    vga_puts(" Hz\n");
}

// AC97 status
void ac97_status(void) {
    vga_puts("\n  Sound status:\n\n");
    
    vga_puts("  Codec: ");
    if (ac97_initialized) {
        vga_puts("AC97\n");
        vga_puts("  NAM base: 0x");
        { char nb[8]; num_to_hex(ac97_nam, nb); vga_puts(nb); }
        vga_puts("\n");
        vga_puts("  NABM base: 0x");
        { char nb[8]; num_to_hex(ac97_nabm, nb); vga_puts(nb); }
        vga_puts("\n");
        vga_puts("  Sample rate: ");
        { char nb[8]; num_to_str(current_sample_rate, nb); vga_puts(nb); }
        vga_puts(" Hz\n");
        vga_puts("  Status: Ready\n");
    } else {
        vga_puts("Not found\n");
    }
}
