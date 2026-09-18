/**
 * fs.c — Sistema de archivos FAT16 para kernel ternario ancestral
 *
 * Soporte básico: leer/escribir archivos, listar directorios
 */

#include "../include/ternary.h"

// FAT16 structures
typedef struct {
    uint8_t  jump[3];
    char     oem[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint16_t root_entries;
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t fat_size_sectors;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    // Extended Boot Record
    uint8_t  drive_number;
    uint8_t  reserved;
    uint8_t  boot_signature;
    uint32_t volume_id;
    char     volume_label[11];
    char     fs_type[8];
} __attribute__((packed)) fat16_bpb_t;

typedef struct {
    char     name[8];
    char     ext[3];
    uint8_t  attr;
    uint8_t  reserved;
    uint8_t  create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t access_date;
    uint16_t first_cluster_high;
    uint16_t modify_time;
    uint16_t modify_date;
    uint16_t first_cluster;
    uint32_t file_size;
} __attribute__((packed)) fat16_dir_entry_t;

// File system state
static fat16_bpb_t bpb;
static uint16_t* fat_table = 0;
static uint8_t* root_dir = 0;
static uint32_t data_area_start = 0;
static uint8_t fs_initialized = 0;

// File handle
#define MAX_FILES 8
typedef struct {
    uint8_t  in_use;
    char     name[13];
    uint32_t size;
    uint16_t first_cluster;
    uint32_t offset;
    uint8_t  mode; // 0=read, 1=write
} file_handle_t;

static file_handle_t file_handles[MAX_FILES];

// Read a sector from disk (using ATA PIO)
static void ata_read_sector(uint32_t lba, uint8_t* buf) {
    // Wait for disk
    while (inb(0x1F7) & 0x80);
    
    outb(0x1F2, 1); // Sector count
    outb(0x1F3, lba & 0xFF);
    outb(0x1F4, (lba >> 8) & 0xFF);
    outb(0x1F5, (lba >> 16) & 0xFF);
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F7, 0x20); // Read sectors
    
    // Wait for data
    while (!(inb(0x1F7) & 0x08));
    
    // Read 256 words (512 bytes)
    uint16_t* p = (uint16_t*)buf;
    for (int i = 0; i < 256; i++) {
        p[i] = inw(0x1F0);
    }
}

// Write a sector to disk (using ATA PIO)
static void ata_write_sector(uint32_t lba, uint8_t* buf) {
    while (inb(0x1F7) & 0x80);
    
    outb(0x1F2, 1);
    outb(0x1F3, lba & 0xFF);
    outb(0x1F4, (lba >> 8) & 0xFF);
    outb(0x1F5, (lba >> 16) & 0xFF);
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F7, 0x30); // Write sectors
    
    while (!(inb(0x1F7) & 0x08));
    
    uint16_t* p = (uint16_t*)buf;
    for (int i = 0; i < 256; i++) {
        outw(0x1F0, p[i]);
    }
    
    // Flush
    outb(0x1F7, 0xE7);
}

// Get cluster chain from FAT
static uint16_t fat_get_next_cluster(uint16_t cluster) {
    if (!fat_table) return 0;
    uint16_t next = fat_table[cluster];
    if (next >= 0xFFF8) return 0; // End of chain
    return next;
}

// Read a cluster
static void fat_read_cluster(uint16_t cluster, uint8_t* buf) {
    uint32_t lba = data_area_start + (uint32_t)(cluster - 2) * bpb.sectors_per_cluster;
    for (int i = 0; i < bpb.sectors_per_cluster; i++) {
        ata_read_sector(lba + i, buf + i * 512);
    }
}

// Write a cluster
static void fat_write_cluster(uint16_t cluster, uint8_t* buf) {
    uint32_t lba = data_area_start + (uint32_t)(cluster - 2) * bpb.sectors_per_cluster;
    for (int i = 0; i < bpb.sectors_per_cluster; i++) {
        ata_write_sector(lba + i, buf + i * 512);
    }
}

// Initialize filesystem
void fs_init(void) {
    vga_puts("[FS] Initializing FAT16...\n");
    
    // Read boot sector
    uint8_t sector[512];
    ata_read_sector(0, sector);
    
    // Parse BPB
    memcpy_t(&bpb, sector, sizeof(fat16_bpb_t));
    
    // Validate
    if (bpb.bytes_per_sector != 512) {
        vga_puts("[FS] Not a valid FAT16 filesystem\n");
        return;
    }
    
    // Calculate positions
    uint32_t root_dir_start = bpb.reserved_sectors + bpb.fat_size_sectors * bpb.num_fats;
    uint32_t root_dir_sectors = (bpb.root_entries * 32 + 511) / 512;
    data_area_start = root_dir_start + root_dir_sectors;
    
    // Read FAT table
    uint32_t fat_size = bpb.fat_size_sectors * 512;
    fat_table = (uint16_t*)(sector); // Reuse sector buffer for now
    
    // Read root directory
    uint8_t root_buf[512];
    ata_read_sector(root_dir_start, root_buf);
    root_dir = root_buf;
    
    vga_puts("[FS] FAT16 initialized\n");
    vga_puts("[FS] Sectors/cluster: ");
    { char nb[8]; num_to_str(bpb.sectors_per_cluster, nb); vga_puts(nb); }
    vga_puts("\n");
    
    fs_initialized = 1;
}

// List files in root directory
void fs_list_files(void) {
    if (!fs_initialized) {
        vga_puts("  Filesystem not initialized\n");
        return;
    }
    
    vga_puts("\n  Files:\n");
    vga_puts("  ---\n");
    
    uint8_t sector[512];
    uint32_t root_start = bpb.reserved_sectors + bpb.fat_size_sectors * bpb.num_fats;
    
    for (int s = 0; s < 14; s++) { // Read up to 14 sectors of root dir
        ata_read_sector(root_start + s, sector);
        
        fat16_dir_entry_t* entries = (fat16_dir_entry_t*)sector;
        for (int i = 0; i < 16; i++) {
            if (entries[i].name[0] == 0) goto done;
            if (entries[i].name[0] == 0xE5) continue;
            if (entries[i].attr == 0x0F) continue; // Long filename
            
            // Print filename
            vga_puts("  ");
            for (int j = 0; j < 8; j++) {
                if (entries[i].name[j] != ' ') vga_putc(entries[i].name[j]);
            }
            if (entries[i].ext[0] != ' ') {
                vga_putc('.');
                for (int j = 0; j < 3; j++) {
                    if (entries[i].ext[j] != ' ') vga_putc(entries[i].ext[j]);
                }
            }
            
            // Print size
            vga_puts("  ");
            char nb[8];
            num_to_str(entries[i].file_size, nb);
            vga_puts(nb);
            vga_puts(" bytes\n");
        }
    }
done:
    vga_puts("\n");
}

// Open a file
int8_t fs_open(const char* filename, uint8_t mode) {
    if (!fs_initialized) return -1;
    
    // Find free handle
    int8_t fd = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (!file_handles[i].in_use) {
            fd = i;
            break;
        }
    }
    if (fd < 0) return -1;
    
    // Parse filename (NAME.EXT)
    char name[8] = {' ',' ',' ',' ',' ',' ',' ',' '};
    char ext[3] = {' ',' ',' '};
    
    const char* dot = 0;
    for (const char* p = filename; *p; p++) {
        if (*p == '.') dot = p;
    }
    
    if (dot) {
        int nlen = dot - filename;
        if (nlen > 8) nlen = 8;
        for (int i = 0; i < nlen; i++) name[i] = filename[i];
        dot++;
        int elen = strlen_t(dot);
        if (elen > 3) elen = 3;
        for (int i = 0; i < elen; i++) ext[i] = dot[i];
    } else {
        int nlen = strlen_t(filename);
        if (nlen > 8) nlen = 8;
        for (int i = 0; i < nlen; i++) name[i] = filename[i];
    }
    
    // Search root directory
    uint8_t sector[512];
    uint32_t root_start = bpb.reserved_sectors + bpb.fat_size_sectors * bpb.num_fats;
    
    for (int s = 0; s < 14; s++) {
        ata_read_sector(root_start + s, sector);
        
        fat16_dir_entry_t* entries = (fat16_dir_entry_t*)sector;
        for (int i = 0; i < 16; i++) {
            if (entries[i].name[0] == 0) return -1;
            if (entries[i].name[0] == 0xE5) continue;
            if (entries[i].attr == 0x0F) continue;
            
            // Compare name and ext
            if (memcmp_t(entries[i].name, name, 8) == 0 &&
                memcmp_t(entries[i].ext, ext, 3) == 0) {
                
                file_handles[fd].in_use = 1;
                file_handles[fd].first_cluster = entries[i].first_cluster;
                file_handles[fd].size = entries[i].file_size;
                file_handles[fd].offset = 0;
                file_handles[fd].mode = mode;
                
                // Copy filename
                for (int j = 0; j < 8; j++) file_handles[fd].name[j] = entries[i].name[j];
                file_handles[fd].name[8] = '.';
                for (int j = 0; j < 3; j++) file_handles[fd].name[9 + j] = entries[i].ext[j];
                file_handles[fd].name[12] = 0;
                
                return fd;
            }
        }
    }
    
    return -1;
}

// Read from file
int32_t fs_read(int8_t fd, uint8_t* buf, uint32_t len) {
    if (fd < 0 || fd >= MAX_FILES || !file_handles[fd].in_use) return -1;
    
    file_handle_t* fh = &file_handles[fd];
    
    if (fh->offset >= fh->size) return 0;
    
    uint32_t remaining = fh->size - fh->offset;
    if (len > remaining) len = remaining;
    
    uint16_t cluster = fh->first_cluster;
    uint32_t cluster_size = bpb.sectors_per_cluster * 512;
    uint32_t bytes_read = 0;
    
    // Skip to current cluster
    uint32_t skip = fh->offset / cluster_size;
    for (uint32_t i = 0; i < skip; i++) {
        cluster = fat_get_next_cluster(cluster);
        if (cluster == 0) return bytes_read;
    }
    
    // Read data
    uint8_t cluster_buf[512 * 8]; // Max cluster size
    uint32_t offset_in_cluster = fh->offset % cluster_size;
    
    while (len > 0 && cluster != 0) {
        fat_read_cluster(cluster, cluster_buf);
        
        uint32_t copy = cluster_size - offset_in_cluster;
        if (copy > len) copy = len;
        
        memcpy_t(buf + bytes_read, cluster_buf + offset_in_cluster, copy);
        bytes_read += copy;
        len -= copy;
        fh->offset += copy;
        offset_in_cluster = 0;
        
        cluster = fat_get_next_cluster(cluster);
    }
    
    return bytes_read;
}

// Close file
void fs_close(int8_t fd) {
    if (fd >= 0 && fd < MAX_FILES) {
        file_handles[fd].in_use = 0;
    }
}

// =============================================================================
// FAT16 WRITE SUPPORT
// =============================================================================

// Find free cluster in FAT
static uint16_t fat_find_free_cluster(void) {
    // Search FAT for free cluster (value 0)
    uint32_t root_start = bpb.reserved_sectors + bpb.fat_size_sectors * bpb.num_fats;
    uint8_t fat_sector[512];
    
    // Skip first 2 reserved entries
    for (uint16_t c = 2; c < 4096; c++) {
        uint32_t byte_offset = c * 2;
        uint32_t sector = root_start + (byte_offset / 512);
        uint32_t offset_in_sector = byte_offset % 512;
        
        // Read FAT sector if needed
        static uint32_t last_fat_sector = 0xFFFFFFFF;
        if (sector != last_fat_sector) {
            ata_read_sector(sector, fat_sector);
            last_fat_sector = sector;
        }
        
        uint16_t* entry = (uint16_t*)(fat_sector + offset_in_sector);
        if (*entry == 0) {
            return c;
        }
    }
    
    return 0; // No free cluster
}

// Set FAT entry
static void fat_set_entry(uint16_t cluster, uint16_t value) {
    uint32_t root_start = bpb.reserved_sectors + bpb.fat_size_sectors * bpb.num_fats;
    uint8_t fat_sector[512];
    
    uint32_t byte_offset = cluster * 2;
    uint32_t sector = root_start + (byte_offset / 512);
    uint32_t offset_in_sector = byte_offset % 512;
    
    ata_read_sector(sector, fat_sector);
    uint16_t* entry = (uint16_t*)(fat_sector + offset_in_sector);
    *entry = value;
    ata_write_sector(sector, fat_sector);
}

// Allocate chain of clusters
static uint16_t fat_alloc_chain(uint32_t num_clusters) {
    uint16_t first = fat_find_free_cluster();
    if (first == 0) return 0;
    
    uint16_t prev = first;
    for (uint32_t i = 1; i < num_clusters; i++) {
        uint16_t next = fat_find_free_cluster();
        if (next == 0) {
            // Free what we allocated
            uint16_t c = first;
            while (c != 0 && c < 0xFFF8) {
                uint16_t n = fat_get_next_cluster(c);
                fat_set_entry(c, 0);
                c = n;
            }
            return 0;
        }
        fat_set_entry(prev, next);
        fat_set_entry(next, 0xFFF8); // End marker
        prev = next;
    }
    
    return first;
}

// Free cluster chain
static void fat_free_chain(uint16_t first_cluster) {
    uint16_t c = first_cluster;
    while (c != 0 && c < 0xFFF8) {
        uint16_t n = fat_get_next_cluster(c);
        fat_set_entry(c, 0);
        c = n;
    }
}

// Find free directory entry slot
static int32_t fat_find_free_dir_entry(uint32_t* sector_out, uint32_t* offset_out) {
    uint8_t sector[512];
    uint32_t root_start = bpb.reserved_sectors + bpb.fat_size_sectors * bpb.num_fats;
    
    for (int s = 0; s < 14; s++) {
        ata_read_sector(root_start + s, sector);
        
        fat16_dir_entry_t* entries = (fat16_dir_entry_t*)sector;
        for (int i = 0; i < 16; i++) {
            if (entries[i].name[0] == 0 || entries[i].name[0] == 0xE5) {
                if (sector_out) *sector_out = root_start + s;
                if (offset_out) *offset_out = i;
                return 0;
            }
        }
    }
    
    return -1; // No free slot
}

// Update directory entry on disk
static void fat_update_dir_entry(const char* filename, uint16_t first_cluster, uint32_t size) {
    char name[8] = {' ',' ',' ',' ',' ',' ',' ',' '};
    char ext[3] = {' ',' ',' '};
    
    const char* dot = 0;
    for (const char* p = filename; *p; p++) {
        if (*p == '.') dot = p;
    }
    
    if (dot) {
        int nlen = dot - filename;
        if (nlen > 8) nlen = 8;
        for (int i = 0; i < nlen; i++) name[i] = filename[i];
        dot++;
        int elen = strlen_t(dot);
        if (elen > 3) elen = 3;
        for (int i = 0; i < elen; i++) ext[i] = dot[i];
    } else {
        int nlen = strlen_t(filename);
        if (nlen > 8) nlen = 8;
        for (int i = 0; i < nlen; i++) name[i] = filename[i];
    }
    
    uint8_t sector[512];
    uint32_t root_start = bpb.reserved_sectors + bpb.fat_size_sectors * bpb.num_fats;
    
    for (int s = 0; s < 14; s++) {
        ata_read_sector(root_start + s, sector);
        
        fat16_dir_entry_t* entries = (fat16_dir_entry_t*)sector;
        for (int i = 0; i < 16; i++) {
            if (entries[i].name[0] == 0 || entries[i].name[0] == 0xE5) {
                // Found slot — write entry
                memcpy_t(entries[i].name, name, 8);
                memcpy_t(entries[i].ext, ext, 3);
                entries[i].attr = 0x20; // Archive
                entries[i].first_cluster = first_cluster;
                entries[i].file_size = size;
                entries[i].create_time = 0;
                entries[i].create_date = 0;
                entries[i].modify_time = 0;
                entries[i].modify_date = 0;
                
                ata_write_sector(root_start + s, sector);
                return;
            }
        }
    }
}

// Create a new file
int8_t fs_create(const char* filename) {
    if (!fs_initialized) return -1;
    
    // Allocate one cluster for the file
    uint16_t cluster = fat_alloc_chain(1);
    if (cluster == 0) return -1;
    
    // Find free directory entry
    if (fat_find_free_dir_entry(0, 0) < 0) {
        fat_free_chain(cluster);
        return -1;
    }
    
    // Create directory entry
    fat_update_dir_entry(filename, cluster, 0);
    
    return 0;
}

// Write to file
int32_t fs_write(int8_t fd, const uint8_t* buf, uint32_t len) {
    if (fd < 0 || fd >= MAX_FILES || !file_handles[fd].in_use) return -1;
    
    file_handle_t* fh = &file_handles[fd];
    if (fh->mode != 1) return -1; // Not in write mode
    
    uint32_t cluster_size = bpb.sectors_per_cluster * 512;
    uint32_t bytes_written = 0;
    uint16_t cluster = fh->first_cluster;
    
    // Skip to current cluster
    uint32_t skip = fh->offset / cluster_size;
    for (uint32_t i = 0; i < skip; i++) {
        uint16_t next = fat_get_next_cluster(cluster);
        if (next == 0) {
            // Need more clusters
            uint16_t new_cluster = fat_find_free_cluster();
            if (new_cluster == 0) return bytes_written;
            fat_set_entry(cluster, new_cluster);
            fat_set_entry(new_cluster, 0xFFF8);
            cluster = new_cluster;
        } else {
            cluster = next;
        }
    }
    
    // Write data
    uint8_t cluster_buf[512 * 8];
    uint32_t offset_in_cluster = fh->offset % cluster_size;
    
    while (len > 0) {
        // Allocate new cluster if needed
        if (cluster == 0 || cluster >= 0xFFF8) {
            uint16_t new_cluster = fat_find_free_cluster();
            if (new_cluster == 0) break;
            
            // Link to chain
            uint16_t prev = fh->first_cluster;
            while (fat_get_next_cluster(prev) != 0 && fat_get_next_cluster(prev) < 0xFFF8) {
                prev = fat_get_next_cluster(prev);
            }
            fat_set_entry(prev, new_cluster);
            fat_set_entry(new_cluster, 0xFFF8);
            cluster = new_cluster;
        }
        
        // Read existing cluster data (for partial writes)
        fat_read_cluster(cluster, cluster_buf);
        
        uint32_t copy = cluster_size - offset_in_cluster;
        if (copy > len) copy = len;
        
        memcpy_t(cluster_buf + offset_in_cluster, buf + bytes_written, copy);
        fat_write_cluster(cluster, cluster_buf);
        
        bytes_written += copy;
        len -= copy;
        fh->offset += copy;
        offset_in_cluster = 0;
        
        cluster = fat_get_next_cluster(cluster);
    }
    
    // Update file size
    if (fh->offset > fh->size) {
        fh->size = fh->offset;
        // Update directory entry
        fat_update_dir_entry(fh->name, fh->first_cluster, fh->size);
    }
    
    return bytes_written;
}

// Delete a file
int8_t fs_delete(const char* filename) {
    if (!fs_initialized) return -1;
    
    char name[8] = {' ',' ',' ',' ',' ',' ',' ',' '};
    char ext[3] = {' ',' ',' '};
    
    const char* dot = 0;
    for (const char* p = filename; *p; p++) {
        if (*p == '.') dot = p;
    }
    
    if (dot) {
        int nlen = dot - filename;
        if (nlen > 8) nlen = 8;
        for (int i = 0; i < nlen; i++) name[i] = filename[i];
        dot++;
        int elen = strlen_t(dot);
        if (elen > 3) elen = 3;
        for (int i = 0; i < elen; i++) ext[i] = dot[i];
    } else {
        int nlen = strlen_t(filename);
        if (nlen > 8) nlen = 8;
        for (int i = 0; i < nlen; i++) name[i] = filename[i];
    }
    
    uint8_t sector[512];
    uint32_t root_start = bpb.reserved_sectors + bpb.fat_size_sectors * bpb.num_fats;
    
    for (int s = 0; s < 14; s++) {
        ata_read_sector(root_start + s, sector);
        
        fat16_dir_entry_t* entries = (fat16_dir_entry_t*)sector;
        for (int i = 0; i < 16; i++) {
            if (entries[i].name[0] == 0) return -1;
            if (entries[i].name[0] == 0xE5) continue;
            if (entries[i].attr == 0x0F) continue;
            
            if (memcmp_t(entries[i].name, name, 8) == 0 &&
                memcmp_t(entries[i].ext, ext, 3) == 0) {
                
                // Free cluster chain
                fat_free_chain(entries[i].first_cluster);
                
                // Mark directory entry as deleted
                entries[i].name[0] = 0xE5;
                ata_write_sector(root_start + s, sector);
                
                return 0;
            }
        }
    }
    
    return -1;
}

// Get file size
int32_t fs_get_size(const char* filename) {
    if (!fs_initialized) return -1;
    
    char name[8] = {' ',' ',' ',' ',' ',' ',' ',' '};
    char ext[3] = {' ',' ',' '};
    
    const char* dot = 0;
    for (const char* p = filename; *p; p++) {
        if (*p == '.') dot = p;
    }
    
    if (dot) {
        int nlen = dot - filename;
        if (nlen > 8) nlen = 8;
        for (int i = 0; i < nlen; i++) name[i] = filename[i];
        dot++;
        int elen = strlen_t(dot);
        if (elen > 3) elen = 3;
        for (int i = 0; i < elen; i++) ext[i] = dot[i];
    } else {
        int nlen = strlen_t(filename);
        if (nlen > 8) nlen = 8;
        for (int i = 0; i < nlen; i++) name[i] = filename[i];
    }
    
    uint8_t sector[512];
    uint32_t root_start = bpb.reserved_sectors + bpb.fat_size_sectors * bpb.num_fats;
    
    for (int s = 0; s < 14; s++) {
        ata_read_sector(root_start + s, sector);
        
        fat16_dir_entry_t* entries = (fat16_dir_entry_t*)sector;
        for (int i = 0; i < 16; i++) {
            if (entries[i].name[0] == 0) return -1;
            if (entries[i].name[0] == 0xE5) continue;
            if (entries[i].attr == 0x0F) continue;
            
            if (memcmp_t(entries[i].name, name, 8) == 0 &&
                memcmp_t(entries[i].ext, ext, 3) == 0) {
                return entries[i].file_size;
            }
        }
    }
    
    return -1;
}
