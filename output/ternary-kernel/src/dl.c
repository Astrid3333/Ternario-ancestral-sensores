/**
 * dl.c — Gestor de descargas para kernel ternario ancestral
 *
 * Cola de descargas, reanudación, verificación checksum
 */

#include "../include/ternary.h"

#define DL_MAX_DOWNLOADS 8
#define DL_MAX_URL       128
#define DL_MAX_FILE      64

typedef struct {
    uint8_t  in_use;
    char     url[DL_MAX_URL];
    char     filename[DL_MAX_FILE];
    uint32_t total_size;
    uint32_t downloaded;
    uint8_t  status;     // 0=pending, 1=active, 2=done, 3=error, 4=paused
    uint32_t checksum;   // CRC32
    uint32_t offset;     // Resume offset
} download_t;

static download_t downloads[DL_MAX_DOWNLOADS];
static uint8_t n_downloads = 0;

// CRC32 calculation
static uint32_t crc32(const uint8_t* data, uint32_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}

// Initialize download manager
void dl_init(void) {
    vga_puts("[DL] Download manager initialized\n");
    for (int i = 0; i < DL_MAX_DOWNLOADS; i++) {
        downloads[i].in_use = 0;
    }
}

// Add download to queue
int8_t dl_add(const char* url, const char* filename) {
    // Find free slot
    int8_t slot = -1;
    for (int i = 0; i < DL_MAX_DOWNLOADS; i++) {
        if (!downloads[i].in_use) {
            slot = i;
            break;
        }
    }
    
    if (slot < 0) {
        vga_puts("[DL] Download queue full\n");
        return -1;
    }
    
    strcpy_t(downloads[slot].url, url);
    strcpy_t(downloads[slot].filename, filename);
    downloads[slot].total_size = 0;
    downloads[slot].downloaded = 0;
    downloads[slot].status = 0; // pending
    downloads[slot].checksum = 0;
    downloads[slot].offset = 0;
    downloads[slot].in_use = 1;
    n_downloads++;
    
    vga_puts("[DL] Added: ");
    vga_puts(filename);
    vga_puts("\n");
    
    return slot;
}

// Start download
int8_t dl_start(int8_t slot) {
    if (slot < 0 || slot >= DL_MAX_DOWNLOADS || !downloads[slot].in_use) {
        vga_puts("[DL] Invalid download\n");
        return -1;
    }
    
    downloads[slot].status = 1; // active
    vga_puts("[DL] Starting download: ");
    vga_puts(downloads[slot].filename);
    vga_puts("\n");
    
    // Simulate HTTP download
    uint8_t* buf = (uint8_t*)(0x200000 + slot * 0x100000); // 1MB per download
    uint32_t received = 0;
    
    if (downloads[slot].url[4] == 's') {
        received = https_get(downloads[slot].url, buf, 0x100000);
    } else {
        received = http_get(downloads[slot].url, buf, 0x100000);
    }
    
    if (received > 0) {
        downloads[slot].total_size = received;
        downloads[slot].downloaded = received;
        downloads[slot].checksum = crc32(buf, received);
        downloads[slot].status = 2; // done
        
        vga_set_color(0x0A, 0);
        vga_puts("[DL] Complete: ");
        vga_puts(downloads[slot].filename);
        vga_puts(" (");
        { char nb[8]; num_to_str(received, nb); vga_puts(nb); }
        vga_puts(" bytes)\n");
        vga_set_color(0x07, 0);
    } else {
        downloads[slot].status = 3; // error
        vga_puts("[DL] Failed: ");
        vga_puts(downloads[slot].filename);
        vga_puts("\n");
    }
    
    return 0;
}

// Pause download
void dl_pause(int8_t slot) {
    if (slot < 0 || slot >= DL_MAX_DOWNLOADS || !downloads[slot].in_use) return;
    
    if (downloads[slot].status == 1) {
        downloads[slot].status = 4; // paused
        vga_puts("[DL] Paused: ");
        vga_puts(downloads[slot].filename);
        vga_puts("\n");
    }
}

// Resume download
void dl_resume(int8_t slot) {
    if (slot < 0 || slot >= DL_MAX_DOWNLOADS || !downloads[slot].in_use) return;
    
    if (downloads[slot].status == 4) {
        downloads[slot].status = 1; // active
        vga_puts("[DL] Resuming: ");
        vga_puts(downloads[slot].filename);
        vga_puts("\n");
    }
}

// Remove download
void dl_remove(int8_t slot) {
    if (slot < 0 || slot >= DL_MAX_DOWNLOADS || !downloads[slot].in_use) return;
    
    downloads[slot].in_use = 0;
    n_downloads--;
    vga_puts("[DL] Removed: ");
    vga_puts(downloads[slot].filename);
    vga_puts("\n");
}

// List downloads
void dl_list(void) {
    vga_puts("\n  Downloads:\n\n");
    
    uint8_t count = 0;
    for (int i = 0; i < DL_MAX_DOWNLOADS; i++) {
        if (downloads[i].in_use) {
            vga_puts("  [");
            { char nb[2]; num_to_str(i, nb); vga_puts(nb); }
            vga_puts("] ");
            vga_puts(downloads[i].filename);
            vga_puts(" - ");
            
            switch (downloads[i].status) {
                case 0: vga_puts("pending"); break;
                case 1: 
                    vga_set_color(0x0B, 0);
                    vga_puts("active");
                    vga_set_color(0x07, 0);
                    break;
                case 2: 
                    vga_set_color(0x0A, 0);
                    vga_puts("done");
                    vga_set_color(0x07, 0);
                    break;
                case 3: 
                    vga_set_color(0x0C, 0);
                    vga_puts("error");
                    vga_set_color(0x07, 0);
                    break;
                case 4: 
                    vga_set_color(0x0E, 0);
                    vga_puts("paused");
                    vga_set_color(0x07, 0);
                    break;
            }
            
            if (downloads[i].status == 1 || downloads[i].status == 2) {
                vga_puts(" (");
                { char nb[8]; num_to_str(downloads[i].downloaded, nb); vga_puts(nb); }
                vga_puts("/");
                { char nb[8]; num_to_str(downloads[i].total_size, nb); vga_puts(nb); }
                vga_puts(" bytes)");
            }
            
            vga_puts("\n");
            count++;
        }
    }
    
    if (count == 0) {
        vga_puts("  No downloads\n");
    }
}

// Verify checksum
uint8_t dl_verify(int8_t slot) {
    if (slot < 0 || slot >= DL_MAX_DOWNLOADS || !downloads[slot].in_use) return 0;
    
    if (downloads[slot].status != 2) {
        vga_puts("[DL] Download not complete\n");
        return 0;
    }
    
    uint8_t* buf = (uint8_t*)(0x200000 + slot * 0x100000);
    uint32_t crc = crc32(buf, downloads[slot].downloaded);
    
    if (crc == downloads[slot].checksum) {
        vga_puts("[DL] Checksum OK\n");
        return 1;
    } else {
        vga_puts("[DL] Checksum mismatch\n");
        return 0;
    }
}

// Show status
void dl_status(void) {
    vga_puts("\n  Download manager status:\n\n");
    
    uint8_t active = 0, done = 0, pending = 0;
    for (int i = 0; i < DL_MAX_DOWNLOADS; i++) {
        if (downloads[i].in_use) {
            if (downloads[i].status == 1) active++;
            else if (downloads[i].status == 2) done++;
            else if (downloads[i].status == 0) pending++;
        }
    }
    
    vga_puts("  Active: ");
    { char nb[4]; num_to_str(active, nb); vga_puts(nb); }
    vga_puts("\n");
    
    vga_puts("  Completed: ");
    { char nb[4]; num_to_str(done, nb); vga_puts(nb); }
    vga_puts("\n");
    
    vga_puts("  Pending: ");
    { char nb[4]; num_to_str(pending, nb); vga_puts(nb); }
    vga_puts("\n");
}
