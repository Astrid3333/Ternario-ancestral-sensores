# Tritos OS — Sistema Operativo Ternario Ancestral

> Kernel bare-metal de 372 KB con GUI, shell, navegador, enciclopedia y IA ternaria.

## Resumen

Tritos OS es un sistema operativo completo que ejecuta directamente sobre hardware (bare-metal) sin necesidad de Linux. Incluye:

- **Kernel** de 372 KB con scheduler Maya de 13 ciclos
- **Shell VGA** con 238+ comandos
- **GUI GTK3** de escritorio con 19 secciones
- **Navegador WebKit** con bookmarks
- **Navegador TUI** estilo lynx
- **Enciclopedia** integrada (Wikipedia API)
- **IA ternaria** para chatbot y análisis de sentimiento
- **Stack de red** completo: TCP/IP, DNS, DHCP, HTTPS, TLS
- **Sistema de archivos** FAT16
- **Gestor de paquetes**, firewall, VPN, control parental

## Especificaciones

| Métrica | Valor |
|---|---|
| Tamaño ISO | 372 KB |
| RAM kernel | ~500 bytes |
| Arranque | milisegundos |
| Comandos shell | 238+ |
| Módulos kernel | 14 |
| Protocolos red | TCP/IP, UDP, DNS, DHCP, HTTP, HTTPS, FTP |
| Seguridad | TLS 1.2, AES-128-CBC, SHA-256 |

## Arquitectura Ternaria

El ternario se aplica en:

### Scheduler Maya de 13 ciclos
```c
// 13 estados cíclicos (no 2^n)
typedef enum {
    TRIT_MINUS,  // -1
    TRIT_ZERO,   //  0
    TRIT_PLUS    // +1
} trit_t;
```

### Memory Manager Base 60
```c
// Direcciones en base babilónica (60)
typedef struct {
    uint8_t major;  // 0-59
    uint8_t minor;  // 0-59
} b60_addr_t;
```

### IA Ternaria
- 3 estados: sí/no/tal vez
- Más expresivo que binario para incertidumbre

## Comparación

| Sistema | Tamaño | GUI | Shell | Navegador | Enciclopedia |
|---|---|---|---|---|---|
| **Tritos OS** | 372 KB | ✓ | ✓ | ✓ | ✓ |
| Linux + XFCE | ~2 GB | ✓ | ✓ | ✓ | ✗ |
| KolibriOS | 1.4 MB | ✓ | ✓ | ✓ | ✗ |
| MenuetOS | 1.4 MB | ✓ | ✓ | ✗ | ✗ |
| FreeRTOS | ~10 KB | ✗ | ✗ | ✗ | ✗ |

## Componentes

### Kernel (bare-metal)
- `kernel.c` — VGA, serial, IDT, shell
- `scheduler.c` — Maya 13 ciclos
- `memory.c` — Base 60
- `net.c` — RTL8139 + TCP/IP + DNS + DHCP + HTTP + HTTPS
- `tls.c` — SHA-256 + AES-128-CBC
- `pkg.c` — Gestor de paquetes
- `firewall.c` — Filtrado de tráfico
- `mail.c` — Cliente SMTP/POP3
- `chat.c` — Mensajería
- `vpn.c` — Túnel cifrado
- `doh.c` — DNS over HTTPS
- `block.c` — Bloqueo de dominios
- `parental.c` — Control parental
- `ai_client.c` — Cliente de IA
- `ftp.c` — Cliente FTP
- `sockets.c` — API de sockets

### Userspace
- `tritos_gui.py` — GUI GTK3
- `tritos_browser.py` — Navegador WebKit
- `tritos_encyclopedia.py` — Enciclopedia
- `tritos_compress.py` — Codec ternario

## Cómo ejecutar

```bash
# Compilar kernel
cd output/ternary-kernel
make run

# Ejecutar GUI (requiere Linux)
python3 tritos_gui.py
```

## Filosofía

Tritos OS no compite con Linux en rendimiento. Compite en:
- **Tamaño**: 372 KB vs 2 GB
- **Simplicidad**: sin systemd, sin bloatware
- **Educación**: entender cómo funciona un OS
- **IoT**: bajo consumo, scheduler determinista

## Licencia

MIT

## Autor

Astrid3333 — https://github.com/Astrid3333
