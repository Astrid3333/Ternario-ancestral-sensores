/**
 * net.c — Network Stack para kernel ternario ancestral
 *
 * RTL8139 driver + Ethernet + ARP + IP + ICMP + UDP
 */

#include "../include/ternary.h"

// PCI config space ports
#define PCI_CONFIG_ADDR  0xCF8
#define PCI_CONFIG_DATA  0xCFC

// RTL8139 registers
#define RTL8139_VENDOR_ID  0x10EC
#define RTL8139_DEVICE_ID  0x8139

// RTL8139 IO registers (offsets from BAR0)
#define RTL8139_IDR0       0x00  // MAC address (6 bytes)
#define RTL8139_MAR0       0x08  // Multicast filter (8 bytes)
#define RTL8139_TSD0       0x10  // Tx status (4 bytes)
#define RTL8139_TSD1       0x14
#define RTL8139_TSD2       0x18
#define RTL8139_TSD3       0x1C
#define RTL8139_TSAD0      0x20  // Tx start address
#define RTL8139_TSAD1      0x24
#define RTL8139_TSAD2      0x28
#define RTL8139_TSAD3      0x2C
#define RTL8139_RBSTART    0x30  // Rx buffer start address
#define RTL8139_ERBCR      0x34  // Rx buffer length
#define RTL8139_ERCSR      0x36  // Rx buffer read/write ptr
#define RTL8139_ISR        0x3E  // Interrupt status
#define RTL8139_IMR        0x3C  // Interrupt mask
#define RTL8139_RCR        0x44  // Rx config
#define RTL8139_TCR        0x40  // Tx config
#define RTL8139_CONFIG1    0x52  // Power state

// RTL8139 bits
#define RTL8139_ISR_ROK    0x01  // Rx OK
#define RTL8139_ISR_TOK    0x02  // Tx OK
#define RTL8139_ISR_RER    0x04  // Rx error
#define RTL8139_ISR_TER    0x08  // Tx error
#define RTL8139_RCR_AAM    0x80  // Accept all multicast
#define RTL8139_RCR_APM    0x20  // Accept physical match
#define RTL8139_RCR_AB     0x08  // Accept broadcast
#define RTL8139_RCR_WRAP   0x8000
#define RTL8139_TCR_CLR    0x06  // DMA burst length
#define RTL8139_CMD_RXEPT  0x08  // Rx config (WRAP)
#define RTL8139_CMD_TXREQ  0x04  // Tx request

// Network state
static uint16_t rtl8139_iobase = 0;
static uint8_t  rtl8139_irq = 0;
static uint8_t  my_mac[6] = {0};
static uint8_t  rx_buffer[8192 + 16] __attribute__((aligned(16)));
static uint16_t rx_offset = 0;
static uint32_t my_ip = 0;
static uint32_t my_gateway = 0;
static uint32_t my_subnet = 0;
static uint8_t  rx_packet[1518] __attribute__((aligned(16)));

// ARP cache (simplified)
typedef struct {
    uint32_t ip;
    uint8_t  mac[6];
    uint8_t  valid;
    uint8_t  ttl;  // ticks before expiry
} arp_entry_t;

#define ARP_CACHE_SIZE 8
static arp_entry_t arp_cache[ARP_CACHE_SIZE];

// Forward declarations
static void rtl8139_init(void);
static void rtl8139_reset(void);
static void rtl8139_send_raw(uint8_t* data, uint16_t len);
static void rtl8139_receive_packet(void);
static uint16_t checksum16(uint16_t* data, uint16_t len);
static void tcp_handle(uint8_t* packet, uint16_t len, uint32_t src_ip);
void dhcp_handle(uint8_t* packet, uint16_t len, uint32_t src_ip);

// =============================================================================
// PCI Configuration
// =============================================================================

static uint32_t pci_config_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset) {
    uint32_t addr = (1 << 31)                    // Enable bit
                  | ((uint32_t)bus << 16)
                  | ((uint32_t)(dev & 0x1F) << 11)
                  | ((uint32_t)(func & 0x07) << 8)
                  | (offset & 0xFC);
    outl(PCI_CONFIG_ADDR, addr);
    return inl(PCI_CONFIG_DATA);
}

static void pci_config_write(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t val) {
    uint32_t addr = (1 << 31)
                  | ((uint32_t)bus << 16)
                  | ((uint32_t)(dev & 0x1F) << 11)
                  | ((uint32_t)(func & 0x07) << 8)
                  | (offset & 0xFC);
    outl(PCI_CONFIG_ADDR, addr);
    outl(PCI_CONFIG_DATA, val);
}

int8_t pci_find_device(uint16_t vendor_id, uint16_t device_id, pci_device_t* dev) {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t d = 0; d < 32; d++) {
            uint32_t reg0 = pci_config_read(bus, d, 0, 0);
            uint16_t vid = reg0 & 0xFFFF;
            if (vid == 0xFFFF) continue;

            uint16_t did = (reg0 >> 16) & 0xFFFF;
            if (vid == vendor_id && did == device_id) {
                // Read full header
                dev->vendor_id = vid;
                dev->device_id = did;

                uint32_t reg2 = pci_config_read(bus, d, 0, 0x08);
                dev->revision_id = reg2 & 0xFF;
                dev->prog_if = (reg2 >> 8) & 0xFF;
                dev->subclass = (reg2 >> 16) & 0xFF;
                dev->class_code = (reg2 >> 24) & 0xFF;

                uint32_t reg4 = pci_config_read(bus, d, 0, 0x0C);
                dev->header_type = (reg4 >> 16) & 0xFF;

                // Read BARs
                for (uint8_t i = 0; i < 6; i++) {
                    uint32_t bar = pci_config_read(bus, d, 0, 0x10 + i * 4);
                    dev->bar[i] = bar;
                }

                // Read IRQ
                uint32_t reg15 = pci_config_read(bus, d, 0, 0x3C);
                dev->interrupt_line = reg15 & 0xFF;
                dev->interrupt_pin = (reg15 >> 8) & 0xFF;

                return 0; // Found
            }
        }
    }
    return -1; // Not found
}

// =============================================================================
// RTL8139 Driver
// =============================================================================

static uint8_t csum(uint8_t* buf, int len) {
    uint32_t sum = 0;
    for (int i = 0; i < len; i += 2) {
        sum += (buf[i] << 8) | (i + 1 < len ? buf[i + 1] : 0);
    }
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return ~(sum & 0xFFFF) & 0xFF;
}

static void rtl8139_init(void) {
    pci_device_t pci;
    if (pci_find_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID, &pci) < 0) {
        vga_puts("[NET] RTL8139 not found\n");
        return;
    }

    // Get I/O base from BAR0
    rtl8139_iobase = pci.bar[0] & 0xFFFE; // Bit 0 is I/O indicator
    rtl8139_irq = pci.interrupt_line;

    vga_puts("[NET] RTL8139 found at IO=0x");
    { char nb[8]; num_to_str(rtl8139_iobase, nb); vga_puts(nb); }
    vga_puts(" IRQ=");
    { char nb[8]; num_to_str(rtl8139_irq, nb); vga_puts(nb); }
    vga_puts("\n");

    // Enable bus mastering
    uint32_t cmd = pci_config_read(0, 0, 0, 0x04);
    cmd |= 0x04; // Bus Master Enable
    pci_config_write(0, 0, 0, 0x04, cmd);

    // Power on
    outb(rtl8139_iobase + RTL8139_CONFIG1, 0x00);

    // Software reset
    rtl8139_reset();

    // Read MAC address
    for (int i = 0; i < 6; i++) {
        my_mac[i] = inb(rtl8139_iobase + RTL8139_IDR0 + i);
    }

    vga_puts("[NET] MAC: ");
    for (int i = 0; i < 6; i++) {
        if (i > 0) vga_puts(":");
        uint8_t hi = (my_mac[i] >> 4) & 0x0F;
        uint8_t lo = my_mac[i] & 0x0F;
        vga_putc(hi < 10 ? '0' + hi : 'A' + hi - 10);
        vga_putc(lo < 10 ? '0' + lo : 'A' + lo - 10);
    }
    vga_puts("\n");

    // Set Rx buffer start address
    uint32_t rx_phys = (uint32_t)(uint64_t)rx_buffer;
    outl(rtl8139_iobase + RTL8139_RBSTART, rx_phys);

    // Configure Rx: accept broadcast + physical match
    outw(rtl8139_iobase + RTL8139_RCR,
         RTL8139_RCR_APM | RTL8139_RCR_AB | RTL8139_RCR_WRAP);

    // Configure Tx: IFG + DMA burst = 1024
    outl(rtl8139_iobase + RTL8139_TCR, 0x03000600);

    // Enable Rx/Tx
    outb(rtl8139_iobase + 0x37, 0x0C);

    // Enable interrupts: Rx OK, Tx OK
    outw(rtl8139_iobase + RTL8139_IMR,
         RTL8139_ISR_ROK | RTL8139_ISR_TOK);

    rx_offset = 0;

    vga_puts("[NET] RTL8139 initialized\n");
}

static void rtl8139_reset(void) {
    outb(rtl8139_iobase + 0x37, 0x10); // Reset
    while (inb(rtl8139_iobase + 0x37) & 0x10) {} // Wait
}

static void rtl8139_send_raw(uint8_t* data, uint16_t len) {
    // Use Tx buffer 0
    // Copy data to tx buffer (we use a static buffer)
    static uint8_t tx_buffer[1518];
    memcpy_t(tx_buffer, data, len);

    // Set Tx start address
    uint32_t tx_phys = (uint32_t)(uint64_t)tx_buffer;
    outl(rtl8139_iobase + RTL8139_TSAD0, tx_phys);

    // Set Tx status (length + OWN bit)
    outl(rtl8139_iobase + RTL8139_TSD0, len);

    // Wait for Tx to complete (polling)
    uint32_t timeout = 100000;
    while (!(inl(rtl8139_iobase + RTL8139_TSD0) & (1 << 15)) && timeout > 0) {
        timeout--;
    }
}

static void rtl8139_receive_packet(void) {
    uint16_t status = inw(rtl8139_iobase + RTL8139_ISR);
    if (!(status & RTL8139_ISR_ROK)) return;

    // Clear interrupt
    outw(rtl8139_iobase + RTL8139_ISR, RTL8139_ISR_ROK);

    // Read packet from Rx buffer
    uint16_t rx_status = *(uint16_t*)(rx_buffer + rx_offset);
    uint16_t rx_len = *(uint16_t*)(rx_buffer + rx_offset + 2);

    if (rx_len > 1518) rx_len = 1518;

    memcpy_t(rx_packet, rx_buffer + rx_offset + 4, rx_len - 4);

    // Advance offset (with alignment)
    rx_offset = (rx_offset + rx_len + 4 + 3) & ~3;
    if (rx_offset >= 8192) rx_offset -= 8192;

    // Process the packet
    net_receive(rx_packet, rx_len - 4);
}

// =============================================================================
// Network Protocol Handlers
// =============================================================================

static uint16_t checksum16(uint16_t* data, uint16_t len) {
    uint32_t sum = 0;
    while (len > 1) {
        sum += *data++;
        len -= 2;
    }
    if (len) sum += *data;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return ~(sum & 0xFFFF);
}

// ARP
static void arp_send_request(uint32_t target_ip) {
    uint8_t packet[42];
    memset_t(packet, 0, 42);

    // Ethernet header
    for (int i = 0; i < 6; i++) packet[i] = 0xFF; // Broadcast
    packet[6] = my_mac[0]; packet[7] = my_mac[1]; packet[8] = my_mac[2];
    packet[9] = my_mac[3]; packet[10] = my_mac[4]; packet[11] = my_mac[5];
    packet[12] = 0x08; packet[13] = 0x06; // ARP

    // ARP header
    packet[14] = 0x00; packet[15] = 0x01; // Ethernet
    packet[16] = 0x08; packet[17] = 0x00; // IPv4
    packet[18] = 6;  // Hardware size
    packet[19] = 4;  // Protocol size
    packet[20] = 0x00; packet[21] = ARP_REQUEST;

    // Sender MAC/IP
    packet[22] = my_mac[0]; packet[23] = my_mac[1]; packet[24] = my_mac[2];
    packet[25] = my_mac[3]; packet[26] = my_mac[4]; packet[27] = my_mac[5];
    packet[28] = my_ip & 0xFF;
    packet[29] = (my_ip >> 8) & 0xFF;
    packet[30] = (my_ip >> 16) & 0xFF;
    packet[31] = (my_ip >> 24) & 0xFF;

    // Target MAC (unknown)
    for (int i = 32; i < 38; i++) packet[i] = 0x00;

    // Target IP
    packet[38] = target_ip & 0xFF;
    packet[39] = (target_ip >> 8) & 0xFF;
    packet[40] = (target_ip >> 16) & 0xFF;
    packet[41] = (target_ip >> 24) & 0xFF;

    rtl8139_send_raw(packet, 42);
}

static void arp_handle(uint8_t* packet, uint16_t len) {
    if (len < 28) return;

    uint16_t opcode = (packet[6] << 8) | packet[7];
    uint32_t sender_ip = packet[14] | (packet[15] << 8) |
                         (packet[16] << 16) | (packet[17] << 24);

    // Store in ARP cache
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (!arp_cache[i].valid || arp_cache[i].ip == sender_ip) {
            arp_cache[i].ip = sender_ip;
            memcpy_t(arp_cache[i].mac, packet + 8, 6);
            arp_cache[i].valid = 1;
            arp_cache[i].ttl = 30;
            break;
        }
    }

    if (opcode == ARP_REQUEST) {
        uint32_t target_ip = packet[24] | (packet[25] << 8) |
                             (packet[26] << 16) | (packet[27] << 24);
        if (target_ip == my_ip) {
            // Send ARP reply
            uint8_t reply[42];
            memset_t(reply, 0, 42);

            // Ethernet header
            memcpy_t(reply, packet + 8, 6); // Target MAC = sender MAC
            reply[6] = my_mac[0]; reply[7] = my_mac[1]; reply[8] = my_mac[2];
            reply[9] = my_mac[3]; reply[10] = my_mac[4]; reply[11] = my_mac[5];
            reply[12] = 0x08; reply[13] = 0x06;

            // ARP header
            reply[14] = 0x00; reply[15] = 0x01;
            reply[16] = 0x08; reply[17] = 0x00;
            reply[18] = 6; reply[19] = 4;
            reply[20] = 0x00; reply[21] = ARP_REPLY;

            // Sender = me
            reply[22] = my_mac[0]; reply[23] = my_mac[1]; reply[24] = my_mac[2];
            reply[25] = my_mac[3]; reply[26] = my_mac[4]; reply[27] = my_mac[5];
            reply[28] = my_ip & 0xFF;
            reply[29] = (my_ip >> 8) & 0xFF;
            reply[30] = (my_ip >> 16) & 0xFF;
            reply[31] = (my_ip >> 24) & 0xFF;

            // Target
            memcpy_t(reply + 32, packet + 8, 6);
            reply[38] = target_ip & 0xFF;
            reply[39] = (target_ip >> 8) & 0xFF;
            reply[40] = (target_ip >> 16) & 0xFF;
            reply[41] = (target_ip >> 24) & 0xFF;

            rtl8139_send_raw(reply, 42);
        }
    }
}

// IP
static void ip_handle(uint8_t* packet, uint16_t len) {
    if (len < 20) return;

    uint8_t version = (packet[0] >> 4) & 0x0F;
    if (version != 4) return;

    uint8_t ihl = (packet[0] & 0x0F) * 4;
    uint16_t total_len = (packet[2] << 8) | packet[3];
    uint8_t protocol = packet[9];
    uint32_t src_ip = packet[12] | (packet[13] << 8) |
                      (packet[14] << 16) | (packet[15] << 24);
    uint32_t dst_ip = packet[16] | (packet[17] << 8) |
                      (packet[18] << 16) | (packet[19] << 24);

    // Check if for us
    if (dst_ip != my_ip) return;

    switch (protocol) {
        case IP_PROTOCOL_ICMP:
            icmp_handle(packet + ihl, total_len - ihl, src_ip);
            break;
        case IP_PROTOCOL_UDP:
            udp_handle(packet + ihl, total_len - ihl, src_ip);
            break;
        case IP_PROTOCOL_TCP:
            tcp_handle(packet + ihl, total_len - ihl, src_ip);
            break;
    }
}

// ICMP
static void icmp_echo_reply(uint32_t src_ip, uint8_t* data, uint16_t len) {
    if (len < 8) return;

    // Build ICMP echo reply (type 0)
    uint8_t packet[64];
    memset_t(packet, 0, 64);
    packet[0] = 0; // Type: Echo Reply
    packet[1] = 0; // Code: 0

    // Copy identifier and sequence from request
    packet[4] = data[4]; packet[5] = data[5];
    packet[6] = data[6]; packet[7] = data[7];

    // Copy data
    uint16_t data_len = len - 8;
    if (data_len > 56) data_len = 56;
    memcpy_t(packet + 8, data + 8, data_len);

    // Checksum
    packet[2] = 0; packet[3] = 0;
    packet[2] = checksum16((uint16_t*)packet, 8 + data_len);

    ip_send(src_ip, IP_PROTOCOL_ICMP, packet, 8 + data_len);
}

void icmp_handle(uint8_t* packet, uint16_t len, uint32_t src_ip) {
    if (len < 8) return;

    uint8_t type = packet[0];
    uint8_t code = packet[1];

    if (type == 8 && code == 0) {
        // Echo Request -> Send Reply
        icmp_echo_reply(src_ip, packet, len);
    }
}

// UDP
void udp_send(uint32_t dst_ip, uint16_t dst_port, uint16_t src_port, uint8_t* data, uint16_t len) {
    uint8_t packet[1500];
    memset_t(packet, 0, sizeof(packet));

    uint16_t total_len = 20 + 8 + len;

    // IP header
    packet[0] = 0x45; // Version 4, IHL 5
    packet[1] = 0x00; // DSCP
    packet[2] = (total_len >> 8) & 0xFF;
    packet[3] = total_len & 0xFF;
    packet[4] = 0x00; packet[5] = 0x00; // Identification
    packet[6] = 0x40; packet[7] = 0x00; // Don't fragment
    packet[8] = 0x40; // TTL = 64
    packet[9] = IP_PROTOCOL_UDP;
    packet[10] = 0; packet[11] = 0; // Checksum (filled later)
    packet[12] = my_ip & 0xFF;
    packet[13] = (my_ip >> 8) & 0xFF;
    packet[14] = (my_ip >> 16) & 0xFF;
    packet[15] = (my_ip >> 24) & 0xFF;
    packet[16] = dst_ip & 0xFF;
    packet[17] = (dst_ip >> 8) & 0xFF;
    packet[18] = (dst_ip >> 16) & 0xFF;
    packet[19] = (dst_ip >> 24) & 0xFF;

    // IP checksum
    packet[10] = 0; packet[11] = 0;
    uint16_t ip_csum = checksum16((uint16_t*)packet, 20);
    packet[10] = (ip_csum >> 8) & 0xFF;
    packet[11] = ip_csum & 0xFF;

    // UDP header
    packet[20] = (src_port >> 8) & 0xFF;
    packet[21] = src_port & 0xFF;
    packet[22] = (dst_port >> 8) & 0xFF;
    packet[23] = dst_port & 0xFF;
    uint16_t udp_len = 8 + len;
    packet[24] = (udp_len >> 8) & 0xFF;
    packet[25] = udp_len & 0xFF;
    packet[26] = 0; packet[27] = 0; // Checksum (optional for UDP)

    // Data
    memcpy_t(packet + 28, data, len);

    ip_send(dst_ip, IP_PROTOCOL_UDP, packet + 20, 8 + len);
}

void udp_handle(uint8_t* packet, uint16_t len, uint32_t src_ip) {
    if (len < 8) return;

    uint16_t src_port = (packet[0] << 8) | packet[1];
    uint16_t dst_port = (packet[2] << 8) | packet[3];
    uint16_t udp_len = (packet[4] << 8) | packet[5];

    // DHCP response
    if (dst_port == DHCP_CLIENT_PORT) {
        dhcp_handle(packet + 8, udp_len - 8, src_ip);
        return;
    }

    // Echo UDP data back
    if (dst_port == 1234) {
        uint8_t* data = packet + 8;
        uint16_t data_len = udp_len - 8;
        udp_send(src_ip, src_port, dst_port, data, data_len);
    }
}

// IP send
void ip_send(uint32_t dst_ip, uint8_t protocol, uint8_t* data, uint16_t len) {
    uint8_t packet[1500];
    memset_t(packet, 0, sizeof(packet));

    uint16_t total_len = 20 + len;

    // IP header
    packet[0] = 0x45;
    packet[1] = 0x00;
    packet[2] = (total_len >> 8) & 0xFF;
    packet[3] = total_len & 0xFF;
    packet[4] = 0x00; packet[5] = 0x00;
    packet[6] = 0x40; packet[7] = 0x00;
    packet[8] = 0x40; // TTL = 64
    packet[9] = protocol;
    packet[10] = 0; packet[11] = 0;
    packet[12] = my_ip & 0xFF;
    packet[13] = (my_ip >> 8) & 0xFF;
    packet[14] = (my_ip >> 16) & 0xFF;
    packet[15] = (my_ip >> 24) & 0xFF;
    packet[16] = dst_ip & 0xFF;
    packet[17] = (dst_ip >> 8) & 0xFF;
    packet[18] = (dst_ip >> 16) & 0xFF;
    packet[19] = (dst_ip >> 24) & 0xFF;

    // IP checksum
    uint16_t ip_csum = checksum16((uint16_t*)packet, 20);
    packet[10] = (ip_csum >> 8) & 0xFF;
    packet[11] = ip_csum & 0xFF;

    memcpy_t(packet + 20, data, len);

    // Resolve MAC via ARP
    uint8_t dst_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Broadcast
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip == dst_ip) {
            memcpy_t(dst_mac, arp_cache[i].mac, 6);
            break;
        }
    }

    // Build Ethernet frame
    uint8_t frame[1518];
    memcpy_t(frame, dst_mac, 6);
    memcpy_t(frame + 6, my_mac, 6);
    frame[12] = (ETH_IP >> 8) & 0xFF;
    frame[13] = ETH_IP & 0xFF;
    memcpy_t(frame + 14, packet, total_len);

    rtl8139_send_raw(frame, 14 + total_len);
}

// ICMP echo request
void icmp_echo_request(uint32_t dst_ip) {
    uint8_t packet[64];
    memset_t(packet, 0, 64);
    packet[0] = 8;  // Type: Echo Request
    packet[1] = 0;  // Code: 0

    // Identifier and sequence
    packet[4] = 0xBE; packet[5] = 0xEF; // Magic
    packet[6] = 0x00; packet[7] = 0x01; // Seq 1

    // Data
    for (int i = 8; i < 64; i++) packet[i] = i;

    // Checksum
    packet[2] = 0; packet[3] = 0;
    uint16_t csum = checksum16((uint16_t*)packet, 64);
    packet[2] = (csum >> 8) & 0xFF;
    packet[3] = csum & 0xFF;

    ip_send(dst_ip, IP_PROTOCOL_ICMP, packet, 64);
}

// ARP resolve
void arp_resolve(uint32_t ip) {
    arp_send_request(ip);
}

// =============================================================================
// Public API
// =============================================================================

void net_init(void) {
    vga_puts("[NET] Initializing network stack...\n");

    // Initialize ARP cache
    memset_t(arp_cache, 0, sizeof(arp_cache));

    // Default network config (will be overridden by DHCP later)
    my_ip = (10 << 24) | (0 << 16) | (2 << 8) | 15;  // 10.0.2.15 (QEMU default)
    my_gateway = (10 << 24) | (0 << 16) | (2 << 8) | 2; // 10.0.2.2
    my_subnet = (255 << 24) | (255 << 16) | (255 << 8) | 0; // 255.255.255.0

    rtl8139_init();

    vga_puts("[NET] IP: ");
    { char nb[8]; num_to_str((my_ip >> 24) & 0xFF, nb); vga_puts(nb); }
    vga_puts(".");
    { char nb[8]; num_to_str((my_ip >> 16) & 0xFF, nb); vga_puts(nb); }
    vga_puts(".");
    { char nb[8]; num_to_str((my_ip >> 8) & 0xFF, nb); vga_puts(nb); }
    vga_puts(".");
    { char nb[8]; num_to_str(my_ip & 0xFF, nb); vga_puts(nb); }
    vga_puts("\n");

    vga_puts("[NET] Gateway: ");
    { char nb[8]; num_to_str((my_gateway >> 24) & 0xFF, nb); vga_puts(nb); }
    vga_puts(".");
    { char nb[8]; num_to_str((my_gateway >> 16) & 0xFF, nb); vga_puts(nb); }
    vga_puts(".");
    { char nb[8]; num_to_str((my_gateway >> 8) & 0xFF, nb); vga_puts(nb); }
    vga_puts(".");
    { char nb[8]; num_to_str(my_gateway & 0xFF, nb); vga_puts(nb); }
    vga_puts("\n");
}

void net_poll(void) {
    if (!rtl8139_iobase) return;
    rtl8139_receive_packet();
}

void net_receive(uint8_t* packet, uint16_t len) {
    if (len < 14) return;

    uint16_t ether_type = (packet[12] << 8) | packet[13];

    switch (ether_type) {
        case ETH_ARP:
            arp_handle(packet + 14, len - 14);
            break;
        case ETH_IP:
            ip_handle(packet + 14, len - 14);
            break;
    }
}

void net_send_packet(uint8_t* dst_mac, uint16_t ether_type, uint8_t* data, uint16_t len) {
    uint8_t frame[1518];
    memcpy_t(frame, dst_mac, 6);
    memcpy_t(frame + 6, my_mac, 6);
    frame[12] = (ether_type >> 8) & 0xFF;
    frame[13] = ether_type & 0xFF;
    memcpy_t(frame + 14, data, len);
    rtl8139_send_raw(frame, 14 + len);
}

// =============================================================================
// TCP — Transmission Control Protocol
// =============================================================================

#define TCP_FIN  0x01
#define TCP_SYN  0x02
#define TCP_RST  0x04
#define TCP_PSH  0x08
#define TCP_ACK  0x10

#define TCP_STATE_CLOSED     0
#define TCP_STATE_LISTEN     1
#define TCP_STATE_SYN_SENT   2
#define TCP_STATE_SYN_RCVD   3
#define TCP_STATE_ESTABLISHED 4
#define TCP_STATE_FIN_WAIT1  5
#define TCP_STATE_FIN_WAIT2  6
#define TCP_STATE_CLOSING    7
#define TCP_STATE_TIME_WAIT  8
#define TCP_STATE_CLOSE_WAIT 9
#define TCP_STATE_LAST_ACK   10

#define TCP_MAX_SOCKETS 8
#define TCP_BUF_SIZE    4096

typedef struct {
    uint8_t  state;
    uint32_t local_ip;
    uint16_t local_port;
    uint32_t remote_ip;
    uint16_t remote_port;
    uint32_t seq;
    uint32_t ack;
    uint16_t window;
    uint8_t  rx_buf[TCP_BUF_SIZE];
    uint16_t rx_len;
    uint8_t  tx_buf[TCP_BUF_SIZE];
    uint16_t tx_len;
    uint8_t  in_use;
} tcp_socket_t;

static tcp_socket_t tcp_sockets[TCP_MAX_SOCKETS];
static uint16_t tcp_port_counter = 49152;

static uint32_t tcp_seq_num = 0;

static uint16_t tcp_checksum(uint32_t src_ip, uint32_t dst_ip, uint8_t* tcp_packet, uint16_t tcp_len) {
    uint32_t sum = 0;
    // Pseudo header
    sum += (src_ip >> 16) & 0xFFFF;
    sum += src_ip & 0xFFFF;
    sum += (dst_ip >> 16) & 0xFFFF;
    sum += dst_ip & 0xFFFF;
    sum += IP_PROTOCOL_TCP;
    sum += tcp_len;
    // TCP header + data
    uint16_t* p = (uint16_t*)tcp_packet;
    for (uint16_t i = 0; i < tcp_len / 2; i++) sum += p[i];
    if (tcp_len & 1) sum += tcp_packet[tcp_len - 1] << 8;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum & 0xFFFF;
}

static void tcp_send_segment(tcp_socket_t* sock, uint8_t flags, uint8_t* data, uint16_t len) {
    uint8_t packet[1500];
    memset_t(packet, 0, sizeof(packet));

    uint16_t ihl = 20;
    uint16_t total_len = ihl + len;

    // TCP header
    packet[0] = (sock->local_port >> 8) & 0xFF;
    packet[1] = sock->local_port & 0xFF;
    packet[2] = (sock->remote_port >> 8) & 0xFF;
    packet[3] = sock->remote_port & 0xFF;
    packet[4] = (sock->seq >> 24) & 0xFF;
    packet[5] = (sock->seq >> 16) & 0xFF;
    packet[6] = (sock->seq >> 8) & 0xFF;
    packet[7] = sock->seq & 0xFF;
    packet[8] = (sock->ack >> 24) & 0xFF;
    packet[9] = (sock->ack >> 16) & 0xFF;
    packet[10] = (sock->ack >> 8) & 0xFF;
    packet[11] = sock->ack & 0xFF;
    packet[12] = 0x50; // Data offset (5 * 4 = 20)
    packet[13] = flags;
    packet[14] = (sock->window >> 8) & 0xFF;
    packet[15] = sock->window & 0xFF;
    // Checksum placeholder
    packet[16] = 0; packet[17] = 0;
    packet[18] = 0; packet[19] = 0;

    // Copy data
    if (len > 0 && data) {
        memcpy_t(packet + ihl, data, len);
    }

    // Compute checksum
    uint16_t csum = tcp_checksum(sock->local_ip, sock->remote_ip, packet, total_len);
    packet[16] = (csum >> 8) & 0xFF;
    packet[17] = csum & 0xFF;

    ip_send(sock->remote_ip, IP_PROTOCOL_TCP, packet, total_len);

    if (flags & (TCP_SYN | TCP_FIN)) sock->seq++;
    if (len > 0) sock->seq += len;
}

static void tcp_handle(uint8_t* packet, uint16_t len, uint32_t src_ip) {
    if (len < 20) return;

    uint16_t src_port = (packet[0] << 8) | packet[1];
    uint16_t dst_port = (packet[2] << 8) | packet[3];
    uint32_t seq_num = ((uint32_t)packet[4] << 24) | ((uint32_t)packet[5] << 16) |
                       ((uint32_t)packet[6] << 8) | packet[7];
    uint32_t ack_num = ((uint32_t)packet[8] << 24) | ((uint32_t)packet[9] << 16) |
                       ((uint32_t)packet[10] << 8) | packet[11];
    uint8_t flags = packet[13];
    uint16_t window = (packet[14] << 8) | packet[15];

    // Find socket
    tcp_socket_t* sock = 0;
    for (int i = 0; i < TCP_MAX_SOCKETS; i++) {
        if (tcp_sockets[i].in_use &&
            tcp_sockets[i].local_port == dst_port &&
            tcp_sockets[i].remote_port == src_port &&
            tcp_sockets[i].remote_ip == src_ip) {
            sock = &tcp_sockets[i];
            break;
        }
    }

    // Also check for listening sockets (SYN)
    if (!sock && (flags & TCP_SYN) && !(flags & TCP_ACK)) {
        for (int i = 0; i < TCP_MAX_SOCKETS; i++) {
            if (tcp_sockets[i].in_use &&
                tcp_sockets[i].state == TCP_STATE_LISTEN &&
                tcp_sockets[i].local_port == dst_port) {
                sock = &tcp_sockets[i];
                // Assign remote
                sock->remote_ip = src_ip;
                sock->remote_port = src_port;
                break;
            }
        }
    }

    if (!sock) {
        // No socket found — send RST
        tcp_socket_t tmp;
        memset_t(&tmp, 0, sizeof(tmp));
        tmp.local_ip = my_ip;
        tmp.local_port = dst_port;
        tmp.remote_ip = src_ip;
        tmp.remote_port = src_port;
        tmp.seq = 0;
        tmp.ack = seq_num + 1;
        tcp_send_segment(&tmp, TCP_RST | TCP_ACK, 0, 0);
        return;
    }

    uint16_t hdr_len = (packet[12] >> 4) * 4;
    uint16_t data_len = len - hdr_len;
    uint8_t* data = packet + hdr_len;

    switch (sock->state) {
        case TCP_STATE_LISTEN:
            if (flags & TCP_SYN) {
                sock->ack = seq_num + 1;
                sock->seq = tcp_seq_num++;
                sock->state = TCP_STATE_SYN_RCVD;
                sock->window = TCP_BUF_SIZE;
                tcp_send_segment(sock, TCP_SYN | TCP_ACK, 0, 0);
            }
            break;

        case TCP_STATE_SYN_SENT:
            if ((flags & (TCP_SYN | TCP_ACK)) == (TCP_SYN | TCP_ACK)) {
                sock->ack = seq_num + 1;
                sock->seq = tcp_seq_num++;
                sock->state = TCP_STATE_ESTABLISHED;
                sock->window = TCP_BUF_SIZE;
                tcp_send_segment(sock, TCP_ACK, 0, 0);
            } else if (flags & TCP_SYN) {
                sock->ack = seq_num + 1;
                sock->state = TCP_STATE_SYN_RCVD;
                tcp_send_segment(sock, TCP_SYN | TCP_ACK, 0, 0);
            }
            break;

        case TCP_STATE_SYN_RCVD:
            if (flags & TCP_ACK) {
                sock->state = TCP_STATE_ESTABLISHED;
            }
            break;

        case TCP_STATE_ESTABLISHED:
            if (flags & TCP_FIN) {
                sock->ack = seq_num + 1;
                sock->state = TCP_STATE_CLOSE_WAIT;
                tcp_send_segment(sock, TCP_ACK, 0, 0);
            }
            if (data_len > 0) {
                // Copy data to receive buffer
                uint16_t copy = data_len;
                if (copy > TCP_BUF_SIZE - sock->rx_len) copy = TCP_BUF_SIZE - sock->rx_len;
                if (copy > 0) {
                    memcpy_t(sock->rx_buf + sock->rx_len, data, copy);
                    sock->rx_len += copy;
                }
                sock->ack += data_len;
                tcp_send_segment(sock, TCP_ACK, 0, 0);
            }
            break;

        case TCP_STATE_CLOSE_WAIT:
            // We initiated close
            break;

        case TCP_STATE_FIN_WAIT1:
            if (flags & TCP_ACK) {
                sock->state = TCP_STATE_FIN_WAIT2;
            }
            if (flags & TCP_FIN) {
                sock->ack = seq_num + 1;
                sock->state = TCP_STATE_TIME_WAIT;
                tcp_send_segment(sock, TCP_ACK, 0, 0);
            }
            break;

        case TCP_STATE_FIN_WAIT2:
            if (flags & TCP_FIN) {
                sock->ack = seq_num + 1;
                sock->state = TCP_STATE_TIME_WAIT;
                tcp_send_segment(sock, TCP_ACK, 0, 0);
            }
            break;

        case TCP_STATE_TIME_WAIT:
            // Wait then close
            sock->state = TCP_STATE_CLOSED;
            sock->in_use = 0;
            break;

        case TCP_STATE_LAST_ACK:
            if (flags & TCP_ACK) {
                sock->state = TCP_STATE_CLOSED;
                sock->in_use = 0;
            }
            break;
    }
}

int8_t tcp_connect(uint32_t dst_ip, uint16_t dst_port) {
    // Find free socket
    int8_t idx = -1;
    for (int i = 0; i < TCP_MAX_SOCKETS; i++) {
        if (!tcp_sockets[i].in_use) {
            idx = i;
            break;
        }
    }
    if (idx < 0) return -1;

    tcp_socket_t* sock = &tcp_sockets[idx];
    memset_t(sock, 0, sizeof(tcp_socket_t));
    sock->in_use = 1;
    sock->state = TCP_STATE_SYN_SENT;
    sock->local_ip = my_ip;
    sock->local_port = tcp_port_counter++;
    sock->remote_ip = dst_ip;
    sock->remote_port = dst_port;
    sock->seq = tcp_seq_num++;
    sock->window = TCP_BUF_SIZE;

    // Send SYN
    tcp_send_segment(sock, TCP_SYN, 0, 0);
    return idx;
}

int8_t tcp_listen(uint16_t port) {
    int8_t idx = -1;
    for (int i = 0; i < TCP_MAX_SOCKETS; i++) {
        if (!tcp_sockets[i].in_use) {
            idx = i;
            break;
        }
    }
    if (idx < 0) return -1;

    tcp_socket_t* sock = &tcp_sockets[idx];
    memset_t(sock, 0, sizeof(tcp_socket_t));
    sock->in_use = 1;
    sock->state = TCP_STATE_LISTEN;
    sock->local_ip = my_ip;
    sock->local_port = port;
    return idx;
}

int16_t tcp_recv(int8_t sock_idx, uint8_t* buf, uint16_t max_len) {
    if (sock_idx < 0 || sock_idx >= TCP_MAX_SOCKETS) return -1;
    tcp_socket_t* sock = &tcp_sockets[sock_idx];
    if (!sock->in_use) return -1;

    if (sock->rx_len == 0) return 0;

    uint16_t copy = sock->rx_len;
    if (copy > max_len) copy = max_len;
    memcpy_t(buf, sock->rx_buf, copy);

    // Shift remaining data
    if (copy < sock->rx_len) {
        memcpy_t(sock->rx_buf, sock->rx_buf + copy, sock->rx_len - copy);
    }
    sock->rx_len -= copy;
    return copy;
}

int16_t tcp_send(int8_t sock_idx, uint8_t* data, uint16_t len) {
    if (sock_idx < 0 || sock_idx >= TCP_MAX_SOCKETS) return -1;
    tcp_socket_t* sock = &tcp_sockets[sock_idx];
    if (!sock->in_use || sock->state != TCP_STATE_ESTABLISHED) return -1;

    // Send in segments
    uint16_t sent = 0;
    while (sent < len) {
        uint16_t chunk = len - sent;
        if (chunk > 1400) chunk = 1400;
        tcp_send_segment(sock, TCP_ACK | TCP_PSH, data + sent, chunk);
        sent += chunk;
    }
    return sent;
}

void tcp_close(int8_t sock_idx) {
    if (sock_idx < 0 || sock_idx >= TCP_MAX_SOCKETS) return;
    tcp_socket_t* sock = &tcp_sockets[sock_idx];
    if (!sock->in_use) return;

    if (sock->state == TCP_STATE_ESTABLISHED) {
        sock->state = TCP_STATE_FIN_WAIT1;
        tcp_send_segment(sock, TCP_FIN | TCP_ACK, 0, 0);
    } else if (sock->state == TCP_STATE_CLOSE_WAIT) {
        sock->state = TCP_STATE_LAST_ACK;
        tcp_send_segment(sock, TCP_FIN | TCP_ACK, 0, 0);
    } else {
        sock->state = TCP_STATE_CLOSED;
        sock->in_use = 0;
    }
}

// =============================================================================
// DNS — Domain Name System
// =============================================================================

typedef struct {
    char name[64];
    uint32_t ip;
    uint8_t valid;
} dns_cache_entry_t;

#define DNS_CACHE_SIZE 16
static dns_cache_entry_t dns_cache[DNS_CACHE_SIZE];
static uint32_t dns_server = 0;

static uint16_t dns_build_name(const char* name, uint8_t* buf) {
    uint16_t len = 0;
    while (*name) {
        const char* dot = name;
        while (*dot && *dot != '.') dot++;
        uint8_t seg_len = dot - name;
        buf[len++] = seg_len;
        for (uint8_t i = 0; i < seg_len; i++) {
            buf[len++] = name[i];
        }
        name = dot;
        if (*name == '.') name++;
    }
    buf[len++] = 0; // Root
    return len;
}

uint32_t dns_resolve(const char* hostname) {
    // Check cache
    for (int i = 0; i < DNS_CACHE_SIZE; i++) {
        if (dns_cache[i].valid && strcmp_t(dns_cache[i].name, hostname) == 0) {
            return dns_cache[i].ip;
        }
    }

    if (!dns_server) return 0;

    // Build DNS query
    uint8_t packet[512];
    memset_t(packet, 0, sizeof(packet));

    // Header
    packet[0] = 0x12; packet[1] = 0x34; // Transaction ID
    packet[2] = 0x01; packet[3] = 0x00; // Flags: standard query
    packet[4] = 0x00; packet[5] = 0x01; // Questions: 1
    packet[6] = 0x00; packet[7] = 0x00; // Answer RRs: 0
    packet[8] = 0x00; packet[9] = 0x00; // Authority RRs: 0
    packet[10] = 0x00; packet[11] = 0x00; // Additional RRs: 0

    // Question
    uint16_t qname_len = dns_build_name(hostname, packet + 12);
    uint16_t qend = 12 + qname_len;
    packet[qend] = 0x00; packet[qend + 1] = 0x01; // Type A
    packet[qend + 2] = 0x00; packet[qend + 3] = 0x01; // Class IN
    uint16_t total = qend + 4;

    // Send via UDP to DNS server on port 53
    uint16_t src_port = 5353;
    udp_send(dns_server, DNS_PORT, src_port, packet, total);

    // Wait for response (poll up to 50 times)
    for (int retry = 0; retry < 50; retry++) {
        net_poll();
        // Check if we got a UDP response on src_port
        // Simple approach: check all sockets
        for (int s = 0; s < 8; s++) {
            // Check if there's pending UDP data (handled in udp_handle)
        }
    }

    // Parse response (simplified — look for A record in UDP buffer)
    // For now, return 0 on timeout
    return 0;
}

void dns_set_server(uint32_t server) {
    dns_server = server;
}

// =============================================================================
// DHCP — Dynamic Host Configuration Protocol
// =============================================================================

#define DHCP_DISCOVER  1
#define DHCP_OFFER     2
#define DHCP_REQUEST   3
#define DHCP_ACK       5
#define DHCP_NAK       6

typedef struct {
    uint8_t  op;
    uint8_t  htype;
    uint8_t  hlen;
    uint8_t  hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    uint8_t  chaddr[16];
    uint8_t  sname[64];
    uint8_t  file[128];
    uint8_t  magic[4]; // 0x63 0x82 0x53 0x63
} __attribute__((packed)) dhcp_header_t;

static uint32_t dhcp_xid = 0xDEADBEEF;

static void dhcp_send_discover(void) {
    uint8_t packet[548];
    memset_t(packet, 0, sizeof(packet));

    dhcp_header_t* hdr = (dhcp_header_t*)packet;
    hdr->op = 1; // Boot request
    hdr->htype = 1; // Ethernet
    hdr->hlen = 6;
    hdr->xid = dhcp_xid;
    hdr->flags = 0x8000; // Broadcast
    memcpy_t(hdr->chaddr, my_mac, 6);
    hdr->magic[0] = 0x63; hdr->magic[1] = 0x82;
    hdr->magic[2] = 0x53; hdr->magic[3] = 0x63;

    // DHCP options: Message Type = Discover
    uint8_t* opt = packet + sizeof(dhcp_header_t);
    *opt++ = 0x35; *opt++ = 1; *opt++ = DHCP_DISCOVER; // Message Type
    *opt++ = 0x37; *opt++ = 4; // Parameter Request List
    *opt++ = 0x01; // Subnet Mask
    *opt++ = 0x03; // Router
    *opt++ = 0x06; // DNS Server
    *opt++ = 0x33; // Lease Time
    *opt++ = 0xFF; // End

    // Send to 255.255.255.255:67 from 0.0.0.0:68
    uint8_t saved_ip[4];
    saved_ip[0] = my_ip & 0xFF;
    saved_ip[1] = (my_ip >> 8) & 0xFF;
    saved_ip[2] = (my_ip >> 16) & 0xFF;
    saved_ip[3] = (my_ip >> 24) & 0xFF;

    my_ip = 0; // Use 0.0.0.0 for DHCP
    udp_send(0xFFFFFFFF, DHCP_PORT, DHCP_CLIENT_PORT, packet, sizeof(dhcp_header_t) + 16);
    my_ip = saved_ip[0] | (saved_ip[1] << 8) | (saved_ip[2] << 16) | (saved_ip[3] << 24);
}

static void dhcp_parse_options(uint8_t* options, uint16_t len, uint32_t* ip, uint32_t* mask, uint32_t* gw, uint32_t* dns) {
    uint16_t i = 0;
    while (i < len) {
        uint8_t type = options[i++];
        if (type == 0xFF) break;
        if (type == 0) continue;
        uint8_t olen = options[i++];
        if (type == 0x35) { // Message Type
            // Already handled
        } else if (type == 0x01 && olen == 4) { // Subnet Mask
            *mask = options[i] | (options[i+1] << 8) | (options[i+2] << 16) | (options[i+3] << 24);
        } else if (type == 0x03 && olen >= 4) { // Router
            *gw = options[i] | (options[i+1] << 8) | (options[i+2] << 16) | (options[i+3] << 24);
        } else if (type == 0x06 && olen >= 4) { // DNS Server
            *dns = options[i] | (options[i+1] << 8) | (options[i+2] << 16) | (options[i+3] << 24);
        }
        i += olen;
    }
}

void dhcp_handle(uint8_t* packet, uint16_t len, uint32_t src_ip) {
    if (len < sizeof(dhcp_header_t)) return;

    dhcp_header_t* hdr = (dhcp_header_t*)packet;
    if (hdr->xid != dhcp_xid) return;
    if (hdr->magic[0] != 0x63 || hdr->magic[1] != 0x82) return;

    uint8_t msg_type = 0;
    uint8_t* options = packet + sizeof(dhcp_header_t);
    uint16_t opt_len = len - sizeof(dhcp_header_t);
    uint32_t mask = 0, gw = 0, dns = 0;

    // Parse message type first
    for (uint16_t i = 0; i < opt_len; i++) {
        if (options[i] == 0x35 && i + 2 < opt_len) {
            msg_type = options[i + 2];
            break;
        }
    }

    if (msg_type == DHCP_OFFER) {
        // Send DHCP Request
        memset_t(packet, 0, sizeof(dhcp_header_t) + 16);
        hdr->op = 1;
        hdr->htype = 1;
        hdr->hlen = 6;
        hdr->xid = dhcp_xid;
        hdr->flags = 0x8000;
        hdr->yiaddr = hdr->yiaddr; // Requested IP
        memcpy_t(hdr->chaddr, my_mac, 6);
        hdr->magic[0] = 0x63; hdr->magic[1] = 0x82;
        hdr->magic[2] = 0x53; hdr->magic[3] = 0x63;

        uint8_t* opt = packet + sizeof(dhcp_header_t);
        *opt++ = 0x35; *opt++ = 1; *opt++ = DHCP_REQUEST;
        *opt++ = 0x32; *opt++ = 4; // Requested IP
        *opt++ = hdr->yiaddr & 0xFF;
        *opt++ = (hdr->yiaddr >> 8) & 0xFF;
        *opt++ = (hdr->yiaddr >> 16) & 0xFF;
        *opt++ = (hdr->yiaddr >> 24) & 0xFF;
        *opt++ = 0xFF;

        uint8_t saved_ip[4];
        saved_ip[0] = my_ip & 0xFF;
        saved_ip[1] = (my_ip >> 8) & 0xFF;
        saved_ip[2] = (my_ip >> 16) & 0xFF;
        saved_ip[3] = (my_ip >> 24) & 0xFF;
        my_ip = 0;
        udp_send(0xFFFFFFFF, DHCP_PORT, DHCP_CLIENT_PORT, packet, sizeof(dhcp_header_t) + 16);
        my_ip = saved_ip[0] | (saved_ip[1] << 8) | (saved_ip[2] << 16) | (saved_ip[3] << 24);
    } else if (msg_type == DHCP_ACK) {
        // Apply configuration
        my_ip = hdr->yiaddr;
        dhcp_parse_options(options, opt_len, &my_ip, &mask, &gw, &dns);
        if (mask) my_subnet = mask;
        if (gw) my_gateway = gw;
        if (dns) dns_server = dns;

        vga_puts("[DHCP] IP assigned: ");
        { char nb[8]; num_to_str((my_ip >> 24) & 0xFF, nb); vga_puts(nb); vga_puts("."); }
        { char nb[8]; num_to_str((my_ip >> 16) & 0xFF, nb); vga_puts(nb); vga_puts("."); }
        { char nb[8]; num_to_str((my_ip >> 8) & 0xFF, nb); vga_puts(nb); vga_puts("."); }
        { char nb[8]; num_to_str(my_ip & 0xFF, nb); vga_puts(nb); vga_puts("\n"); }
    } else if (msg_type == DHCP_NAK) {
        vga_puts("[DHCP] Request rejected\n");
    }
}

void dhcp_start(void) {
    vga_puts("[DHCP] Sending discover...\n");
    dhcp_xid = 0xDEADBEEF;
    dhcp_send_discover();
}

// =============================================================================
// HTTP Client
// =============================================================================

typedef struct {
    uint16_t status;
    uint32_t content_length;
    uint8_t  chunked;
    uint8_t  headers_end;
    char     content_type[64];
} http_response_t;

// Build HTTP GET request
static uint16_t http_build_get(char* buf, const char* host, const char* path) {
    uint16_t pos = 0;
    
    // Method + path
    const char* get = "GET ";
    for (int i = 0; get[i]; i++) buf[pos++] = get[i];
    for (int i = 0; path[i]; i++) buf[pos++] = path[i];
    const char* proto = " HTTP/1.0\r\n";
    for (int i = 0; proto[i]; i++) buf[pos++] = proto[i];
    
    // Host header
    const char* host_hdr = "Host: ";
    for (int i = 0; host_hdr[i]; i++) buf[pos++] = host_hdr[i];
    for (int i = 0; host[i]; i++) buf[pos++] = host[i];
    const char* crlf = "\r\n";
    for (int i = 0; crlf[i]; i++) buf[pos++] = crlf[i];
    
    // Connection close
    const char* conn = "Connection: close\r\n\r\n";
    for (int i = 0; conn[i]; i++) buf[pos++] = conn[i];
    
    return pos;
}

// Parse HTTP response headers, return offset to body
static uint16_t http_parse_headers(const uint8_t* data, uint16_t len, http_response_t* resp) {
    resp->status = 0;
    resp->content_length = 0;
    resp->chunked = 0;
    resp->headers_end = 0;
    resp->content_type[0] = 0;
    
    // Find status line
    uint16_t i = 0;
    while (i < len - 1 && data[i] != ' ') i++;
    i++; // skip space
    
    // Parse status code (3 digits)
    if (i + 2 < len) {
        resp->status = (data[i] - '0') * 100 + (data[i+1] - '0') * 10 + (data[i+2] - '0');
    }
    
    // Find end of headers
    while (i < len - 3) {
        if (data[i] == '\r' && data[i+1] == '\n' && data[i+2] == '\r' && data[i+3] == '\n') {
            resp->headers_end = i + 4;
            break;
        }
        
        // Parse Content-Length
        if (data[i] == 'C' && data[i+1] == 'o' && data[i+2] == 'n') {
            const char* cl = "Content-Length: ";
            uint8_t match = 1;
            for (int j = 0; cl[j]; j++) {
                if (data[i+j] != cl[j]) { match = 0; break; }
            }
            if (match) {
                i += 16;
                while (i < len && data[i] >= '0' && data[i] <= '9') {
                    resp->content_length = resp->content_length * 10 + (data[i] - '0');
                    i++;
                }
            }
        }
        
        // Parse Transfer-Encoding: chunked
        if (data[i] == 'T' && data[i+1] == 'r') {
            const char* te = "Transfer-Encoding: chunked";
            uint8_t match = 1;
            for (int j = 0; te[j]; j++) {
                if (data[i+j] != te[j]) { match = 0; break; }
            }
            if (match) resp->chunked = 1;
        }
        
        // Parse Content-Type
        if (data[i] == 'C' && data[i+1] == 'o' && data[i+2] == 'n') {
            const char* ct = "Content-Type: ";
            uint8_t match = 1;
            for (int j = 0; ct[j]; j++) {
                if (data[i+j] != ct[j]) { match = 0; break; }
            }
            if (match) {
                i += 14;
                int ci = 0;
                while (i < len && data[i] != '\r' && ci < 63) {
                    resp->content_type[ci++] = data[i++];
                }
                resp->content_type[ci] = 0;
            }
        }
        
        i++;
    }
    
    return resp->headers_end;
}

// Decode chunked transfer encoding
static uint16_t http_decode_chunked(const uint8_t* input, uint16_t input_len, uint8_t* output) {
    uint16_t out_pos = 0;
    uint16_t pos = 0;
    
    while (pos < input_len) {
        // Read chunk size (hex)
        uint32_t chunk_size = 0;
        while (pos < input_len && input[pos] != '\r') {
            uint8_t c = input[pos];
            if (c >= '0' && c <= '9') chunk_size = chunk_size * 16 + (c - '0');
            else if (c >= 'a' && c <= 'f') chunk_size = chunk_size * 16 + (c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') chunk_size = chunk_size * 16 + (c - 'A' + 10);
            pos++;
        }
        pos += 2; // skip \r\n
        
        if (chunk_size == 0) break;
        
        // Copy chunk data
        for (uint32_t i = 0; i < chunk_size && pos < input_len; i++) {
            output[out_pos++] = input[pos++];
        }
        pos += 2; // skip trailing \r\n
    }
    
    return out_pos;
}

// URL parse helper
static void http_parse_url(const char* url, char* host, char* path, uint16_t* port) {
    host[0] = 0;
    path[0] = '/';
    path[1] = 0;
    *port = 80;
    
    const char* p = url;
    
    // Skip http://
    if (p[0] == 'h' && p[1] == 't' && p[2] == 't' && p[3] == 'p') {
        p += 4;
        if (p[0] == 's') { p++; *port = 443; }
        if (p[0] == ':') p++;
        if (p[0] == '/' && p[1] == '/') p += 2;
    }
    
    // Extract host
    int hi = 0;
    while (*p && *p != ':' && *p != '/' && hi < 63) {
        host[hi++] = *p++;
    }
    host[hi] = 0;
    
    // Extract port
    if (*p == ':') {
        p++;
        *port = 0;
        while (*p >= '0' && *p <= '9') {
            *port = *port * 10 + (*p - '0');
            p++;
        }
    }
    
    // Extract path
    if (*p == '/') {
        int pi = 0;
        while (*p && pi < 127) {
            path[pi++] = *p++;
        }
        path[pi] = 0;
    }
}

// Download URL over HTTP (returns bytes received)
uint32_t http_get(const char* url, uint8_t* buf, uint32_t buf_size) {
    char host[64];
    char path[128];
    uint16_t port;
    
    http_parse_url(url, host, path, &port);
    
    vga_puts("[HTTP] Connecting to ");
    vga_puts(host);
    { char nb[8]; num_to_str(port, nb); vga_puts(":"); vga_puts(nb); }
    vga_puts("...\n");
    
    // Resolve DNS if needed
    uint32_t ip = dns_resolve(host);
    if (ip == 0) {
        // Try gateway
        ip = my_gateway;
    }
    
    // TCP connect
    int8_t sock = tcp_connect(ip, port);
    if (sock < 0) {
        vga_puts("[HTTP] Connection failed\n");
        return 0;
    }
    
    // Build and send request
    char request[512];
    uint16_t req_len = http_build_get(request, host, path);
    
    if (tcp_send(sock, (uint8_t*)request, req_len) < 0) {
        vga_puts("[HTTP] Send failed\n");
        tcp_close(sock);
        return 0;
    }
    
    vga_puts("[HTTP] Request sent, waiting for response...\n");
    
    // Wait for response
    uint32_t total = 0;
    uint32_t timeout = 50000000;
    
    while (total < buf_size - 1 && timeout > 0) {
        int32_t n = tcp_recv(sock, buf + total, buf_size - total - 1);
        if (n > 0) {
            total += n;
            timeout = 50000000; // Reset timeout
        } else {
            timeout--;
            if (timeout == 0) break;
        }
        
        // Check if connection closed
        if (tcp_sockets[sock].state == TCP_STATE_CLOSED) break;
    }
    
    buf[total] = 0;
    tcp_close(sock);
    
    // Parse headers
    http_response_t resp;
    uint16_t body_offset = http_parse_headers(buf, total, &resp);
    
    vga_puts("[HTTP] Status: ");
    { char nb[8]; num_to_str(resp.status, nb); vga_puts(nb); }
    vga_puts("\n");
    
    if (resp.content_type[0]) {
        vga_puts("[HTTP] Content-Type: ");
        vga_puts(resp.content_type);
        vga_puts("\n");
    }
    
    vga_puts("[HTTP] Body: ");
    { char nb[8]; num_to_str(total - body_offset, nb); vga_puts(nb); vga_puts(" bytes\n"); }
    
    // Move body to start of buffer
    if (body_offset > 0 && body_offset < total) {
        uint32_t body_len = total - body_offset;
        for (uint32_t i = 0; i < body_len; i++) {
            buf[i] = buf[body_offset + i];
        }
        buf[body_len] = 0;
        return body_len;
    }
    
    return total;
}

// =============================================================================
// HTTPS Client (over TLS)
// =============================================================================

uint32_t https_get(const char* url, uint8_t* buf, uint32_t buf_size) {
    char host[64];
    char path[128];
    uint16_t port;
    
    // Parse URL
    const char* p = url;
    if (p[0] == 'h' && p[1] == 't' && p[2] == 't' && p[3] == 'p' && p[4] == 's') {
        p += 8; // skip https://
    } else if (p[0] == 'h' && p[1] == 't' && p[2] == 't' && p[3] == 'p') {
        p += 7; // skip http://
        return http_get(url, buf, buf_size);
    }
    
    port = 443;
    
    // Extract host
    int hi = 0;
    while (*p && *p != ':' && *p != '/' && hi < 63) {
        host[hi++] = *p++;
    }
    host[hi] = 0;
    
    // Extract port
    if (*p == ':') {
        p++;
        port = 0;
        while (*p >= '0' && *p <= '9') {
            port = port * 10 + (*p - '0');
            p++;
        }
    }
    
    // Extract path
    char* pathp = path;
    if (*p == '/') {
        while (*p) *pathp++ = *p++;
    } else {
        *pathp++ = '/';
    }
    *pathp = 0;
    
    vga_puts("[HTTPS] Connecting to ");
    vga_puts(host);
    { char nb[8]; num_to_str(port, nb); vga_puts(":"); vga_puts(nb); }
    vga_puts("...\n");
    
    // Resolve DNS
    uint32_t ip = dns_resolve(host);
    if (ip == 0) {
        ip = my_gateway;
    }
    
    // TLS connect
    int8_t sock = tls_connect(ip, port);
    if (sock < 0) {
        vga_puts("[HTTPS] TLS connection failed\n");
        return 0;
    }
    
    // Build HTTP request
    char request[512];
    uint16_t req_pos = 0;
    
    const char* method = "GET ";
    for (int i = 0; method[i]; i++) request[req_pos++] = method[i];
    for (int i = 0; path[i]; i++) request[req_pos++] = path[i];
    const char* proto = " HTTP/1.0\r\nHost: ";
    for (int i = 0; proto[i]; i++) request[req_pos++] = proto[i];
    for (int i = 0; host[i]; i++) request[req_pos++] = host[i];
    const char* hdrs = "\r\nConnection: close\r\n\r\n";
    for (int i = 0; hdrs[i]; i++) request[req_pos++] = hdrs[i];
    
    // Send over TLS
    if (tls_send((uint8_t*)request, req_pos) < 0) {
        vga_puts("[HTTPS] Send failed\n");
        tls_close();
        return 0;
    }
    
    vga_puts("[HTTPS] Request sent, waiting for response...\n");
    
    // Receive response
    uint32_t total = 0;
    uint32_t timeout = 100000000;
    
    while (total < buf_size - 1 && timeout > 0) {
        int32_t n = tls_recv(buf + total, buf_size - total - 1);
        if (n > 0) {
            total += n;
            timeout = 100000000;
        } else {
            timeout--;
            if (timeout == 0) break;
        }
    }
    
    buf[total] = 0;
    tls_close();
    
    // Parse HTTP response
    http_response_t resp;
    uint16_t body_offset = http_parse_headers(buf, total, &resp);
    
    vga_puts("[HTTPS] Status: ");
    { char nb[8]; num_to_str(resp.status, nb); vga_puts(nb); }
    vga_puts("\n");
    
    vga_puts("[HTTPS] Body: ");
    { char nb[8]; num_to_str(total - body_offset, nb); vga_puts(nb); vga_puts(" bytes\n"); }
    
    // Move body to start
    if (body_offset > 0 && body_offset < total) {
        uint32_t body_len = total - body_offset;
        for (uint32_t i = 0; i < body_len; i++) {
            buf[i] = buf[body_offset + i];
        }
        buf[body_len] = 0;
        return body_len;
    }
    
    return total;
}

// wget - download file
void cmd_wget(const char* url) {
    if (!url || !*url) {
        vga_puts("  Usage: wget <url>\n");
        return;
    }
    
    // Skip leading space
    while (*url == ' ') url++;
    
    uint8_t* buf = (uint8_t*)0x200000; // Use high memory for buffer
    uint32_t received = 0;
    
    // Check if HTTPS
    if (url[0] == 'h' && url[1] == 't' && url[2] == 't' && url[3] == 'p' && url[4] == 's') {
        received = https_get(url, buf, 0x100000);
    } else {
        received = http_get(url, buf, 0x100000);
    }
    
    if (received > 0) {
        vga_puts("\n[WGET] Downloaded ");
        { char nb[8]; num_to_str(received, nb); vga_puts(nb); }
        vga_puts(" bytes\n");
        
        // Show first 512 bytes as preview
        vga_puts("\n--- Preview (first 512 bytes) ---\n");
        uint32_t preview = (received > 512) ? 512 : received;
        for (uint32_t i = 0; i < preview; i++) {
            if (buf[i] >= 32 && buf[i] < 127) {
                vga_putc(buf[i]);
            } else if (buf[i] == '\n') {
                vga_putc('\n');
            } else {
                vga_putc('.');
            }
        }
        if (received > 512) {
            vga_puts("\n... (truncated)");
        }
        vga_puts("\n--- End ---\n");
    } else {
        vga_puts("[WGET] Download failed\n");
    }
}

// curl - show headers + body
void cmd_curl(const char* url) {
    if (!url || !*url) {
        vga_puts("  Usage: curl <url>\n");
        return;
    }
    
    while (*url == ' ') url++;
    
    uint8_t* buf = (uint8_t*)0x200000;
    uint32_t received = 0;
    
    if (url[0] == 'h' && url[1] == 't' && url[2] == 't' && url[3] == 'p' && url[4] == 's') {
        received = https_get(url, buf, 0x100000);
    } else {
        received = http_get(url, buf, 0x100000);
    }
    
    if (received > 0) {
        vga_puts("\n--- Raw Response ---\n");
        for (uint32_t i = 0; i < received; i++) {
            if (buf[i] >= 32 && buf[i] < 127) {
                vga_putc(buf[i]);
            } else if (buf[i] == '\n') {
                vga_putc('\n');
            } else if (buf[i] == '\r') {
                // skip
            } else {
                vga_putc('.');
            }
        }
        vga_puts("\n--- End ---\n");
    } else {
        vga_puts("[CURL] Request failed\n");
    }
}
