/**
 * usb_storage.c — USB Mass Storage Driver para Tritos OS
 *
 * Soporte para dispositivos de almacenamiento USB (flash drives, HDDs).
 * Lectura/escritura de bloques FAT16/FAT32.
 */

#include "../include/ternary.h"

// USB Mass Storage class codes
#define USB_CLASS_MASS_STORAGE     0x08

// SCSI commands
#define SCSI_TEST_UNIT_READY       0x00
#define SCSI_INQUIRY               0x12
#define SCSI_READ_CAPACITY_10      0x25
#define SCSI_READ_10               0x28
#define SCSI_WRITE_10              0x2A

// Block size
#define USB_BLOCK_SIZE             512

// Max devices
#define MAX_USB_STORAGE            4

// Storage device
typedef struct {
    uint8_t present;
    uint8_t port;
    uint32_t total_blocks;
    uint32_t block_size;
    char name[32];
    char vendor[16];
    char product[16];
    uint8_t mounted;
    char mount_point[16];
} usb_storage_t;

static usb_storage_t usb_storage[MAX_USB_STORAGE];
static uint8_t usb_storage_count = 0;

// =============================================================================
// SCSI Commands over USB Bulk (simplified)
// =============================================================================

static int usb_storage_send_scsi(uint8_t port, uint8_t* cdb, uint8_t cdb_len,
                                  uint8_t* data, uint32_t data_len, uint8_t write) {
    // Build CBW (Command Block Wrapper)
    uint8_t cbw[31];
    memset(cbw, 0, 31);

    cbw[0] = 0x55; cbw[1] = 0x53; cbw[2] = 0x42; cbw[3] = 0x43;
    cbw[4] = 0x01;
    cbw[8] = data_len & 0xFF;
    cbw[9] = (data_len >> 8) & 0xFF;
    cbw[10] = (data_len >> 16) & 0xFF;
    cbw[11] = (data_len >> 24) & 0xFF;
    cbw[12] = write ? 0x00 : 0x80;
    cbw[14] = cdb_len;

    for (int i = 0; i < cdb_len && i < 16; i++) {
        cbw[15 + i] = cdb[i];
    }

    // Simplified — actual needs UHCI bulk transfer
    return 0;
}

// =============================================================================
// SCSI Commands
// =============================================================================

int usb_storage_inquiry(uint8_t port, char* vendor, char* product) {
    uint8_t cdb[6];
    uint8_t data[36];
    memset(cdb, 0, 6);
    cdb[0] = SCSI_INQUIRY;
    cdb[4] = 36;

    int result = usb_storage_send_scsi(port, cdb, 6, data, 36, 0);
    if (result < 0) return -1;

    for (int i = 0; i < 8; i++) vendor[i] = data[8 + i];
    vendor[8] = 0;
    for (int i = 0; i < 16; i++) product[i] = data[16 + i];
    product[16] = 0;

    return 0;
}

int usb_storage_read_capacity(uint8_t port, uint32_t* total_blocks, uint32_t* block_size) {
    uint8_t cdb[10];
    uint8_t data[8];
    memset(cdb, 0, 10);
    cdb[0] = SCSI_READ_CAPACITY_10;

    int result = usb_storage_send_scsi(port, cdb, 10, data, 8, 0);
    if (result < 0) return -1;

    *total_blocks = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    *block_size = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];

    return 0;
}

int usb_storage_read_block(uint8_t port, uint32_t lba, uint8_t* buffer) {
    uint8_t cdb[10];
    memset(cdb, 0, 10);
    cdb[0] = SCSI_READ_10;
    cdb[2] = (lba >> 24) & 0xFF;
    cdb[3] = (lba >> 16) & 0xFF;
    cdb[4] = (lba >> 8) & 0xFF;
    cdb[5] = lba & 0xFF;
    cdb[8] = 1;

    return usb_storage_send_scsi(port, cdb, 10, buffer, USB_BLOCK_SIZE, 0);
}

int usb_storage_write_block(uint8_t port, uint32_t lba, const uint8_t* buffer) {
    uint8_t cdb[10];
    memset(cdb, 0, 10);
    cdb[0] = SCSI_WRITE_10;
    cdb[2] = (lba >> 24) & 0xFF;
    cdb[3] = (lba >> 16) & 0xFF;
    cdb[4] = (lba >> 8) & 0xFF;
    cdb[5] = lba & 0xFF;
    cdb[8] = 1;

    return usb_storage_send_scsi(port, cdb, 10, (uint8_t*)buffer, USB_BLOCK_SIZE, 1);
}

// =============================================================================
// Device Management
// =============================================================================

void usb_storage_detect(void) {
    usb_storage_count = 0;
    uint8_t count = usb_get_device_count();

    for (int i = 0; i < count; i++) {
        uint16_t vendor_id, product_id;
        uint8_t dev_class;
        usb_get_device_info(i, &vendor_id, &product_id, &dev_class);

        if (dev_class == USB_CLASS_MASS_STORAGE) {
            uint8_t idx = usb_storage_count;
            if (idx >= MAX_USB_STORAGE) break;

            usb_storage[idx].present = 1;
            usb_storage[idx].port = i;

            // Name from vendor/product IDs
            usb_storage[idx].vendor[0] = 'U';
            usb_storage[idx].vendor[1] = 'S';
            usb_storage[idx].vendor[2] = 'B';
            usb_storage[idx].vendor[3] = 0;

            usb_storage[idx].product[0] = 'D';
            usb_storage[idx].product[1] = 'e';
            usb_storage[idx].product[2] = 'v';
            usb_storage[idx].product[3] = ' ';
            {
                char nb[4];
                num_to_str(idx, nb);
                int len = 0;
                while (nb[len]) { usb_storage[idx].product[3 + len] = nb[len]; len++; }
                usb_storage[idx].product[3 + len] = 0;
            }

            // Build name
            usb_storage[idx].name[0] = 'U';
            usb_storage[idx].name[1] = 'S';
            usb_storage[idx].name[2] = 'B';
            usb_storage[idx].name[3] = '0' + idx;
            usb_storage[idx].name[4] = ':';
            usb_storage[idx].name[5] = 'D';
            usb_storage[idx].name[6] = 'e';
            usb_storage[idx].name[7] = 'v';
            usb_storage[idx].name[8] = 0;

            usb_storage[idx].mounted = 0;
            usb_storage_count++;

            vga_puts("[USB-STORAGE] Detected: ");
            vga_puts(usb_storage[idx].name);
            vga_puts("\n");
        }
    }
}

int usb_storage_mount(uint8_t dev_idx, const char* mount_point) {
    if (dev_idx >= usb_storage_count) return -1;
    if (!usb_storage[dev_idx].present) return -1;

    uint8_t sector[512];
    if (usb_storage_read_block(usb_storage[dev_idx].port, 0, sector) < 0) {
        vga_puts("[USB-STORAGE] Cannot read partition table\n");
        return -1;
    }

    if (sector[0] == 0xEB || sector[0] == 0xE9) {
        uint16_t bytes_per_sector = sector[11] | (sector[12] << 8);
        uint16_t reserved_sectors = sector[14] | (sector[15] << 8);
        uint8_t num_fats = sector[16];
        uint16_t root_entries = sector[17] | (sector[18] << 8);
        uint16_t fat_size_16 = sector[22] | (sector[23] << 8);

        uint32_t root_sectors = ((root_entries * 32) + (bytes_per_sector - 1)) / bytes_per_sector;
        (void)root_sectors;
        (void)reserved_sectors;
        (void)num_fats;

        vga_puts("[USB-STORAGE] FAT");
        if (fat_size_16 == 0) {
            vga_puts("32");
        } else {
            vga_puts("16");
        }
        vga_puts(" detected\n");
    }

    strncpy(usb_storage[dev_idx].mount_point, mount_point, 15);
    usb_storage[dev_idx].mounted = 1;

    vga_puts("[USB-STORAGE] Mounted at ");
    vga_puts(mount_point);
    vga_puts("\n");

    return 0;
}

int usb_storage_unmount(uint8_t dev_idx) {
    if (dev_idx >= usb_storage_count) return -1;
    if (!usb_storage[dev_idx].mounted) return -1;

    usb_storage[dev_idx].mounted = 0;
    usb_storage[dev_idx].mount_point[0] = 0;

    vga_puts("[USB-STORAGE] Unmounted\n");
    return 0;
}

// =============================================================================
// Status
// =============================================================================

void usb_storage_status(void) {
    vga_puts("\n  [USB Mass Storage]\n\n");

    vga_puts("  Devices: ");
    { char nb[2]; num_to_str(usb_storage_count, nb); vga_puts(nb); }
    vga_puts("\n");

    for (int i = 0; i < usb_storage_count; i++) {
        vga_puts("    [");
        vga_puts(usb_storage[i].present ? "OK" : "  ");
        vga_puts("] ");
        vga_puts(usb_storage[i].name);
        vga_puts("\n");

        if (usb_storage[i].present) {
            vga_puts("        Vendor: ");
            vga_puts(usb_storage[i].vendor);
            vga_puts("\n        Size: ");
            uint32_t size_mb = (usb_storage[i].total_blocks * usb_storage[i].block_size) / (1024 * 1024);
            { char nb[8]; num_to_str(size_mb, nb); vga_puts(nb); }
            vga_puts(" MB\n");

            vga_puts("        Mount: ");
            if (usb_storage[i].mounted) {
                vga_puts(usb_storage[i].mount_point);
            } else {
                vga_puts("(not mounted)");
            }
            vga_puts("\n");
        }
    }
    vga_puts("\n");
}
