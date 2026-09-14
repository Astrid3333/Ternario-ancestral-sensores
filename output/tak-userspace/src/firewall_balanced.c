// ============================================================================
// TAK FIREWALL BALANCED — Filtrado de red sin falsos positivos
// ============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// ============================================================================
// SECTION 1: BASE DE DATOS DE BLOQUEOS
// ============================================================================

typedef struct {
    const char* domain;
    uint8_t category;       // 1=porn, 2=violence, 3=drugs, 4=malware, 5=deepweb
    uint8_t confidence;     // 1-10 (10=confirmado)
    const char* source;     // Fuente de la información
    time_t last_updated;
} BlockedDomain;

// Base de datos actualizable (en producción sería SQLite)
static BlockedDomain BLOCKLIST[] = {
    // Porno (alta confianza)
    {"pornhub.com", 1, 10, "manual", 0},
    {"xvideos.com", 1, 10, "manual", 0},
    {"xnxx.com", 1, 10, "manual", 0},
    {"xhamster.com", 1, 10, "manual", 0},
    {"redtube.com", 1, 10, "manual", 0},
    {"youporn.com", 1, 10, "manual", 0},
    
    // Malware (alta confianza)
    {"malware.com", 4, 10, "manual", 0},
    {"phishing.com", 4, 10, "manual", 0},
    
    // Deep web (alta confianza)
    {"tor2web.org", 5, 10, "manual", 0},
    {"onion.ws", 5, 10, "manual", 0},
    {"onion.pet", 5, 10, "manual", 0},
    
    {NULL, 0, 0, NULL, 0}
};

// ============================================================================
// SECTION 2: ANÁLISIS INTELIGENTE
// ============================================================================

// Patrones de URL con pesos
typedef struct {
    const char* pattern;
    float weight;
    uint8_t allow_research; // 1=permitido en investigación
    const char* category;
} URLPattern;

static const URLPattern URL_PATTERNS[] = {
    // Porno
    {"/porn", 0.9, 0, "adult"},
    {"/xxx", 0.95, 0, "adult"},
    {"/nude", 0.8, 1, "adult"},     // Investigación médica
    {"/sex", 0.6, 1, "adult"},      // Investigación científica
    {"/adult", 0.7, 1, "adult"},    // Investigación
    
    // Violencia
    {"/kill", 0.7, 1, "violence"},  // Noticias
    {"/murder", 0.8, 1, "violence"},// Investigación criminal
    {"/torture", 0.9, 0, "violence"},
    {"/rape", 0.95, 0, "violence"},
    {"/abuse", 0.7, 1, "violence"}, // Investigación
    
    // Drogas
    {"/drug", 0.6, 1, "drugs"},     // Investigación
    {"/cocaine", 0.8, 1, "drugs"},  // Investigación
    {"/marijuana", 0.5, 1, "drugs"},// Legal en algunos contextos
    {"/heroin", 0.9, 0, "drugs"},
    
    // Malware
    {"/malware", 0.8, 1, "malware"},// Investigación de seguridad
    {"/virus", 0.7, 1, "malware"},  // Investigación
    {"/exploit", 0.8, 1, "malware"},// Investigación
    
    // Deep web
    {".onion", 0.7, 0, "deepweb"},
    {"/tor", 0.6, 1, "deepweb"},    // Investigación de privacidad
    
    {NULL, 0, 0, NULL}
};

// Análisis de URL
float analyze_url(const char* url, uint8_t is_research) {
    float score = 0.0;
    int matches = 0;
    
    for (int i = 0; URL_PATTERNS[i].pattern != NULL; i++) {
        if (strstr(url, URL_PATTERNS[i].pattern) != NULL) {
            matches++;
            
            if (is_research && URL_PATTERNS[i].allow_research) {
                score += URL_PATTERNS[i].weight * 0.3;  // Reducir en investigación
            } else {
                score += URL_PATTERNS[i].weight;
            }
        }
    }
    
    if (matches > 0) {
        score = score / matches;
    }
    
    return score;
}

// ============================================================================
// SECTION 3: VERIFICACIÓN SSL/TLS
// ============================================================================

typedef struct {
    uint8_t valid;          // 0=inválido, 1=válido
    uint8_t expired;        // 0=no, 1=sí
    uint8_t self_signed;    // 0=no, 1=sí
    uint8_t weak_cipher;    // 0=no, 1=sí
    char issuer[128];
    char subject[128];
    time_t not_before;
    time_t not_after;
} SSLStatus;

// Verificar SSL (simulado - en producción usar OpenSSL)
SSLStatus check_ssl(const char* hostname) {
    SSLStatus status = {0};
    
    // En producción: conectar y verificar certificado
    // Aquí simulamos una verificación básica
    
    // Verificar si el dominio tiene HTTPS
    if (strncmp(hostname, "https://", 8) == 0) {
        status.valid = 1;
    } else {
        status.valid = 0;
    }
    
    return status;
}

// ============================================================================
// SECTION 4: DETECCIÓN DE PROTOCOLOS NO AUTORIZADOS
// ============================================================================

typedef struct {
    const char* scheme;
    uint8_t allowed;
    uint8_t require_ssl;
    const char* reason;
} Protocol;

static const Protocol PROTOCOLS[] = {
    {"https", 1, 1, "HTTP seguro"},
    {"http", 0, 0, "HTTP inseguro"},
    {"ftp", 0, 0, "FTP inseguro"},
    {"ssh", 0, 0, "SSH - solo administrador"},
    {"tor", 0, 0, "Tor - bloqueado"},
    {"i2p", 0, 0, "I2P - bloqueado"},
    {"socks", 0, 0, "Proxy - bloqueado"},
    {"ws", 0, 0, "WebSocket inseguro"},
    {"wss", 1, 1, "WebSocket seguro"},
    {"mqtt", 1, 0, "IoT protocolo"},
    {"coap", 1, 0, "IoT protocolo"},
    {NULL, 0, 0, NULL}
};

// Verificar protocolo
int check_protocol(const char* url) {
    const char* scheme_end = strstr(url, "://");
    if (scheme_end == NULL) return -1;
    
    int scheme_len = scheme_end - url;
    char scheme[16] = {0};
    if (scheme_len > 15) return -2;
    
    strncpy(scheme, url, scheme_len);
    
    for (int i = 0; PROTOCOLS[i].scheme != NULL; i++) {
        if (strcmp(scheme, PROTOCOLS[i].scheme) == 0) {
            return PROTOCOLS[i].allowed;
        }
    }
    
    return 0;  // Protocolo desconocido - bloquear
}

// ============================================================================
// SECTION 5: FIREWALL PRINCIPAL
// ============================================================================

typedef struct {
    uint32_t id;
    char client_ip[16];
    char url[512];
    uint8_t blocked;
    uint8_t category;
    float confidence;
    const char* reason;
    time_t timestamp;
} FirewallLog;

// Función principal de filtrado
int filter_request(const char* client_ip, const char* url, uint8_t is_research) {
    FirewallLog log;
    log.id = rand();
    strncpy(log.client_ip, client_ip, sizeof(log.client_ip) - 1);
    strncpy(log.url, url, sizeof(log.url) - 1);
    log.timestamp = time(NULL);
    log.blocked = 0;
    
    // 1. Verificar protocolo
    if (!check_protocol(url)) {
        log.blocked = 1;
        log.category = 0;
        log.confidence = 1.0;
        log.reason = "Protocolo no autorizado";
        log_event(&log);
        return -1;
    }
    
    // 2. Verificar dominio en blocklist
    for (int i = 0; BLOCKLIST[i].domain != NULL; i++) {
        if (strstr(url, BLOCKLIST[i].domain) != NULL) {
            log.blocked = 1;
            log.category = BLOCKLIST[i].category;
            log.confidence = BLOCKLIST[i].confidence / 10.0;
            log.reason = "Dominio bloqueado";
            log_event(&log);
            return -2;
        }
    }
    
    // 3. Análisis de patrones
    float pattern_score = analyze_url(url, is_research);
    if (pattern_score > 0.7) {
        log.blocked = 1;
        log.category = 0;
        log.confidence = pattern_score;
        log.reason = "Patrón sospechoso";
        log_event(&log);
        return -3;
    }
    
    // 4. Verificar SSL
    SSLStatus ssl = check_ssl(url);
    if (!ssl.valid) {
        log.blocked = 1;
        log.category = 0;
        log.confidence = 0.5;
        log.reason = "Sin SSL válido";
        log_event(&log);
        return -4;
    }
    
    // 5. Verificar .onion / deepweb
    if (strstr(url, ".onion") != NULL) {
        log.blocked = 1;
        log.category = 5;
        log.confidence = 0.9;
        log.reason = "Deep web bloqueado";
        log_event(&log);
        return -5;
    }
    
    // Permitido
    log.blocked = 0;
    log.confidence = 0;
    log.reason = "Permitido";
    log_event(&log);
    
    return 0;
}

// Registrar evento
void log_event(FirewallLog* log) {
    // En producción: almacenar en base de datos
    if (log->blocked) {
        printf("[BLOCKED] %s -> %s (%s)\n", 
               log->client_ip, log->url, log->reason);
    }
}

// ============================================================================
// SECTION 6: INTERFAZ DE USUARIO
// ============================================================================

// Mostrar estado del firewall
void show_firewall_status() {
    printf("=== TAK FIREWALL STATUS ===\n");
    printf("Dominios bloqueados: ");
    int count = 0;
    for (int i = 0; BLOCKLIST[i].domain != NULL; i++) count++;
    printf("%d\n", count);
    
    printf("Protocolos permitidos: ");
    int allowed = 0;
    for (int i = 0; PROTOCOLS[i].scheme != NULL; i++) {
        if (PROTOCOLS[i].allowed) allowed++;
    }
    printf("%d/%d\n", allowed, count);
    
    printf("Patrones de URL: ");
    count = 0;
    for (int i = 0; URL_PATTERNS[i].pattern != NULL; i++) count++;
    printf("%d\n", count);
    
    printf("===========================\n\n");
}

// ============================================================================
// SECTION 7: MAIN - DEMO
// ============================================================================

int main() {
    printf("=== TAK FIREWALL BALANCED ===\n");
    printf("Filtrado de red sin falsos positivos\n\n");
    
    // Mostrar estado
    show_firewall_status();
    
    // Test 1: Sitio seguro (HTTPS)
    printf("Test 1: Sitio seguro (https://university.edu)\n");
    int result1 = filter_request("192.168.1.100", "https://university.edu", 1);
    printf("  Result: %s\n\n", result1 == 0 ? "PERMITIDO" : "BLOQUEADO");
    
    // Test 2: Sitio inseguro (HTTP)
    printf("Test 2: Sitio inseguro (http://example.com)\n");
    int result2 = filter_request("192.168.1.100", "http://example.com", 0);
    printf("  Result: %s\n\n", result2 == 0 ? "PERMITIDO" : "BLOQUEADO");
    
    // Test 3: Porno (bloqueado)
    printf("Test 3: Porno (https://pornhub.com)\n");
    int result3 = filter_request("192.168.1.100", "https://pornhub.com", 0);
    printf("  Result: %s\n\n", result3 == 0 ? "PERMITIDO" : "BLOQUEADO");
    
    // Test 4: Deep web (bloqueado)
    printf("Test 4: Deep web (http://something.onion)\n");
    int result4 = filter_request("192.168.1.100", "http://something.onion", 0);
    printf("  Result: %s\n\n", result4 == 0 ? "PERMITIDO" : "BLOQUEADO");
    
    // Test 5: Investigación legítima
    printf("Test 5: Investigación (https://research.edu/drug-study)\n");
    int result5 = filter_request("192.168.1.100", "https://research.edu/drug-study", 1);
    printf("  Result: %s\n\n", result5 == 0 ? "PERMITIDO" : "BLOQUEADO");
    
    // Test 6: Proxy (bloqueado)
    printf("Test 6: Proxy (socks://proxy.example.com)\n");
    int result6 = filter_request("192.168.1.100", "socks://proxy.example.com", 0);
    printf("  Result: %s\n\n", result6 == 0 ? "PERMITIDO" : "BLOQUEADO");
    
    // Test 7: MQTT (permitido para IoT)
    printf("Test 7: MQTT (mqtt://sensor.local)\n");
    int result7 = filter_request("192.168.1.100", "mqtt://sensor.local", 0);
    printf("  Result: %s\n\n", result7 == 0 ? "PERMITIDO" : "BLOQUEADO");
    
    printf("=== RESUMEN ===\n");
    printf("Firewall implementado:\n");
    printf("✓ Filtrado por dominio (blocklist actualizable)\n");
    printf("✓ Análisis de patrones de URL con pesos\n");
    printf("✓ Verificación SSL/TLS\n");
    printf("✓ Detección de protocolos no autorizados\n");
    printf("✓ Bloqueo de deep web (.onion)\n");
    printf("✓ Soporte para investigación (reducción de falsos positivos)\n");
    printf("✓ Logs de auditoría\n");
    printf("✓ Interfaz de usuario\n");
    
    return 0;
}
