/**
 * usb.c — Driver USB básico para kernel ternario ancestral
 *
 * Detección de dispositivos USB (UHCI/OHCI)
 */

#include "../include/ternary.h"

// USB controller types
#define USB_NONE     0
#define USB_UHCI     1
#define USB_OHCI     2
#define USB_EHCI     3
#define USB_XHCI     4

// USB register bases (UHCI)
#define UHCI_USBCMD      0x00
#define USBSTS           0x02
#define USBINTR          0x04
#define FRNUM            0x06
#define FLBASEADD        0x08
#define SOFMOD           0x0C
#define PORTSC1          0x10
#define PORTSC2          0x12

// USB device states
#define USB_DEV_NONE     0
#define USB_DEV_ATTACHED 1
#define USB_DEV_POWERED  2
#define USB_DEV_DEFAULT  3
#define USB_DEV_ADDRESS  4
#define USB_DEV_CONFIG   5

// USB device descriptor
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} __attribute__((packed)) usb_dev_desc_t;

// USB device info
typedef struct {
    uint8_t  state;
    uint8_t  address;
    uint8_t  port;
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t  device_class;
    uint8_t  device_subclass;
    uint8_t  max_packet_size;
    char     name[32];
} usb_device_t;

// USB controller state
static uint8_t usb_controller_type = USB_NONE;
static uint16_t usb_io_base = 0;
static usb_device_t usb_devices[8];
static uint8_t usb_device_count = 0;

// Read USB register
static uint16_t usb_read_word(uint16_t reg) {
    return inw(usb_io_base + reg);
}

// Write USB register
static void usb_write_word(uint16_t reg, uint16_t val) {
    outw(usb_io_base + reg, val);
}

// Write byte
static void usb_write_byte(uint16_t reg, uint8_t val) {
    outb(usb_io_base + reg, val);
}

// Reset USB controller
static void usb_reset_controller(void) {
    // Set reset bit
    usb_write_word(UHCI_USBCMD, 0x0004);
    
    // Wait 10ms
    for (volatile int i = 0; i < 10000; i++) asm("nop");
    
    // Clear reset
    usb_write_word(UHCI_USBCMD, 0x0000);
}

// Check USB port status
static uint8_t usb_port_status(int port) {
    uint16_t reg = (port == 0) ? PORTSC1 : PORTSC2;
    uint16_t status = usb_read_word(reg);
    return (status & 0x01) ? 1 : 0; // Current Connect Status
}

// Reset USB port
static void usb_port_reset(int port) {
    uint16_t reg = (port == 0) ? PORTSC1 : PORTSC2;
    
    // Set reset
    usb_write_word(reg, 0x0200);
    
    // Wait 50ms
    for (volatile int i = 0; i < 50000; i++) asm("nop");
    
    // Clear reset
    usb_write_word(reg, 0x0000);
    
    // Wait 10ms
    for (volatile int i = 0; i < 10000; i++) asm("nop");
}

// Detect USB controller (PCI)
static void usb_detect_pci(void) {
    // Scan PCI bus for USB controllers
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            for (uint8_t function = 0; function < 8; function++) {
                uint32_t addr = 0x80000000 | (bus << 16) | (device << 11) | (function << 8);
                
                // Read vendor ID
                outl(0xCF8, addr);
                uint16_t vendor = inl(0xCFC) & 0xFFFF;
                
                if (vendor == 0xFFFF) continue;
                
                // Read class code
                outl(0xCF8, addr + 0x08);
                uint32_t class = inl(0xCFC);
                uint8_t base_class = (class >> 24) & 0xFF;
                uint8_t sub_class = (class >> 16) & 0xFF;
                
                // USB controller: class 0x0C, subclass 0x03
                if (base_class == 0x0C && sub_class == 0x03) {
                    // Read BAR4 (I/O base)
                    outl(0xCF8, addr + 0x20);
                    uint32_t bar4 = inl(0xCFC);
                    
                    if (bar4 & 0x01) { // I/O space
                        usb_io_base = bar4 & 0xFFFC;
                        usb_controller_type = USB_UHCI;
                        
                        vga_puts("[USB] Found UHCI controller at IO=0x");
                        { char nb[8]; num_to_hex(usb_io_base, nb); vga_puts(nb); }
                        vga_puts("\n");
                        return;
                    }
                }
            }
        }
    }
}

// Initialize USB
void usb_init(void) {
    vga_puts("[USB] Initializing USB...\n");
    
    // Clear device table
    for (int i = 0; i < 8; i++) {
        usb_devices[i].state = USB_DEV_NONE;
    }
    usb_device_count = 0;
    
    // Detect controller via PCI
    usb_detect_pci();
    
    if (usb_controller_type == USB_NONE) {
        vga_puts("[USB] No USB controller found\n");
        return;
    }
    
    // Reset controller
    usb_reset_controller();
    
    // Enable interrupts
    usb_write_word(USBINTR, 0x000F);
    
    // Check ports
    for (int port = 0; port < 2; port++) {
        if (usb_port_status(port)) {
            vga_puts("[USB] Device detected on port ");
            { char nb[2]; num_to_str(port, nb); vga_puts(nb); }
            vga_puts("\n");
            
            // Reset port
            usb_port_reset(port);
            
            // Add device
            if (usb_device_count < 8) {
                usb_device_t* dev = &usb_devices[usb_device_count];
                dev->state = USB_DEV_DEFAULT;
                dev->address = 0;
                dev->port = port;
                dev->vendor_id = 0;
                dev->product_id = 0;
                usb_device_count++;
            }
        }
    }
    
    vga_puts("[USB] USB initialized, ");
    { char nb[2]; num_to_str(usb_device_count, nb); vga_puts(nb); }
    vga_puts(" device(s) found\n");
}

// Get device count
uint8_t usb_get_device_count(void) {
    return usb_device_count;
}

// Get device info
void usb_get_device_info(uint8_t index, uint16_t* vendor, uint16_t* product, uint8_t* class) {
    if (index >= usb_device_count) return;
    
    usb_device_t* dev = &usb_devices[index];
    if (vendor) *vendor = dev->vendor_id;
    if (product) *product = dev->product_id;
    if (class) *class = dev->device_class;
}

// USB status
void usb_status(void) {
    vga_puts("\n  USB status:\n\n");
    
    vga_puts("  Controller: ");
    switch (usb_controller_type) {
        case USB_UHCI: vga_puts("UHCI"); break;
        case USB_OHCI: vga_puts("OHCI"); break;
        case USB_EHCI: vga_puts("EHCI"); break;
        case USB_XHCI: vga_puts("xHCI"); break;
        default: vga_puts("None"); break;
    }
    vga_puts("\n");
    
    if (usb_io_base) {
        vga_puts("  I/O base: 0x");
        { char nb[8]; num_to_hex(usb_io_base, nb); vga_puts(nb); }
        vga_puts("\n");
    }
    
    vga_puts("  Devices: ");
    { char nb[2]; num_to_str(usb_device_count, nb); vga_puts(nb); }
    vga_puts("\n");
    
    for (int i = 0; i < usb_device_count; i++) {
        vga_puts("    Port ");
        { char nb[2]; num_to_str(usb_devices[i].port, nb); vga_puts(nb); }
        vga_puts(": ");
        vga_puts(usb_devices[i].name);
        vga_puts("\n");
    }
}
