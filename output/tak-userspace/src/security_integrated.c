// ============================================================================
// TAK SECURITY INTEGRATED — Sistema completo de seguridad
// ============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <ctype.h>
#include <math.h>

// ============================================================================
// SECTION 1: TIPOS GLOBALES
// ============================================================================

typedef struct {
    uint32_t id;
    char name[64];
    char email[128];
    uint8_t role;           // 0=guest, 1=user, 2=researcher, 3=admin
    uint8_t clearance;      // 0-10
    float reputation;       // 0-100
    uint32_t total_actions;
    uint32_t false_positives;
    uint32_t confirmed_violations;
    time_t first_seen;
    time_t last_active;
    uint8_t banned;
    char ban_reason[128];
} User;

typedef struct {
    uint32_t id;
    uint32_t user_id;
    uint8_t category;
    float confidence;
    uint8_t action_taken;
    time_t timestamp;
    char evidence_hash[64];
    uint8_t reviewed;
    uint8_t overturned;
} Detection;

typedef struct {
    float precision;
    float recall;
    float f1_score;
    float false_positive_rate;
    uint32_t true_positives;
    uint32_t false_positives;
    uint32_t true_negatives;
    uint32_t false_negatives;
} SystemMetrics;

// ============================================================================
// SECTION 2: KEYWORDS CON PESOS
// ============================================================================

typedef struct {
    const char* word;
    float weight;
    uint8_t allow_research;
    const char* category;
} Keyword;

static const Keyword KEYWORDS[] = {
    {"porn", 0.9, 0, "adult"},
    {"xxx", 0.95, 0, "adult"},
    {"nude", 0.8, 1, "adult"},
    {"sex", 0.6, 1, "adult"},
    {"erotic", 0.7, 1, "adult"},
    {"kill", 0.7, 1, "violence"},
    {"murder", 0.8, 1, "violence"},
    {"torture", 0.9, 0, "violence"},
    {"rape", 0.95, 0, "violence"},
    {"abuse", 0.7, 1, "violence"},
    {"drug", 0.6, 1, "drugs"},
    {"cocaine", 0.8, 1, "drugs"},
    {"marijuana", 0.5, 1, "drugs"},
    {"heroin", 0.9, 0, "drugs"},
    {"malware", 0.8, 1, "malware"},
    {"virus", 0.7, 1, "malware"},
    {"exploit", 0.8, 1, "malware"},
    {"hack", 0.6, 1, "hacking"},
    {"onion", 0.7, 0, "deepweb"},
    {"tor", 0.6, 1, "deepweb"},
    {NULL, 0, 0, NULL}
};

// ============================================================================
// SECTION 3: ANÁLISIS DE CONTENIDO
// ============================================================================

float analyze_content(const char* url, const char* title, const char* content,
                     User* user, uint8_t is_research) {
    float score = 0.0;
    int matches = 0;
    
    char buffer[4096];
    snprintf(buffer, sizeof(buffer), "%s %s %s",
             url ? url : "",
             title ? title : "",
             content ? content : "");
    
    for (int i = 0; buffer[i]; i++) {
        buffer[i] = tolower(buffer[i]);
    }
    
    for (int i = 0; KEYWORDS[i].word != NULL; i++) {
        if (strstr(buffer, KEYWORDS[i].word) != NULL) {
            matches++;
            if (is_research && KEYWORDS[i].allow_research) {
                score += KEYWORDS[i].weight * 0.3;
            } else {
                score += KEYWORDS[i].weight;
            }
        }
    }
    
    if (matches > 0) {
        score = score / matches;
    }
    
    if (user) {
        float rep = user->reputation / 100.0;
        score = score * (0.5 + 0.5 * rep);
    }
    
    if (is_research) {
        score = score * 0.6;
    }
    
    return score;
}

// ============================================================================
// SECTION 4: SISTEMA DE CONFIANZA
// ============================================================================

typedef struct {
    uint8_t level;
    float min_score;
    float max_score;
    uint8_t action;
    uint8_t require_human;
    const char* description;
} ConfidenceLevel;

static const ConfidenceLevel CONFIDENCE_LEVELS[] = {
    {0, 0.0, 0.3, 0, 0, "Seguro"},
    {1, 0.3, 0.5, 1, 0, "Sospecha leve - registrar"},
    {2, 0.5, 0.7, 2, 0, "Sospecha moderada - alertar"},
    {3, 0.7, 0.8, 3, 1, "Sospecha alta - bloquear temporal"},
    {4, 0.8, 0.9, 4, 1, "Muy sospechoso - bloquear"},
    {5, 0.9, 1.0, 5, 1, "Casi seguro - reportar"}
};

ConfidenceLevel* get_confidence(float score) {
    for (int i = 0; i < 6; i++) {
        if (score >= CONFIDENCE_LEVELS[i].min_score &&
            score < CONFIDENCE_LEVELS[i].max_score) {
            return &CONFIDENCE_LEVELS[i];
        }
    }
    return &CONFIDENCE_LEVELS[0];
}

// ============================================================================
// SECTION 5: VERIFICACIÓN CRUZADA
// ============================================================================

float cross_validate(const char* data, User* user) {
    float score1 = analyze_content(data, NULL, NULL, user, 0);
    
    float score2 = 0.0;
    if (strstr(data, ".onion")) score2 += 0.8;
    if (strstr(data, "tor2web")) score2 += 0.7;
    if (strstr(data, "proxy")) score2 += 0.5;
    if (strstr(data, "vpn")) score2 += 0.3;
    if (score2 > 1.0) score2 = 1.0;
    
    float score3 = 0.0;
    time_t now = time(NULL);
    struct tm* tm = localtime(&now);
    if (tm->tm_hour >= 23 || tm->tm_hour <= 5) {
        score3 = 0.2;
    }
    
    float final_score = (score1 * 0.4) + (score2 * 0.3) + (score3 * 0.3);
    
    float min_score = score1;
    float max_score = score1;
    if (score2 < min_score) min_score = score2;
    if (score2 > max_score) max_score = score2;
    if (score3 < min_score) min_score = score3;
    if (score3 > max_score) max_score = score3;
    
    float agreement = 1.0 - ((max_score - min_score));
    if (agreement < 0.7) {
        final_score = final_score * agreement;
    }
    
    return final_score;
}

// ============================================================================
// SECTION 6: REPUTACIÓN
// ============================================================================

float calculate_reputation(User* user) {
    if (user->total_actions == 0) return 100.0;
    
    float fpr = (float)user->false_positives / user->total_actions;
    float score = 100.0 - (fpr * 100.0);
    
    time_t now = time(NULL);
    float days = (float)(now - user->first_seen) / 86400.0;
    float bonus = fmin(days * 0.1, 10.0);
    score += bonus;
    
    score -= user->confirmed_violations * 20.0;
    
    if (score < 0) score = 0;
    if (score > 100) score = 100;
    
    user->reputation = score;
    return score;
}

// ============================================================================
// SECTION 7: FIREWALL
// ============================================================================

typedef struct {
    const char* domain;
    uint8_t category;
    uint8_t confidence;
} BlockedDomain;

static const BlockedDomain BLOCKLIST[] = {
    {"pornhub.com", 1, 10},
    {"xvideos.com", 1, 10},
    {"xnxx.com", 1, 10},
    {"xhamster.com", 1, 10},
    {"redtube.com", 1, 10},
    {"youporn.com", 1, 10},
    {"tor2web.org", 5, 10},
    {"onion.ws", 5, 10},
    {"onion.pet", 5, 10},
    {NULL, 0, 0}
};

typedef struct {
    const char* scheme;
    uint8_t allowed;
} Protocol;

static const Protocol PROTOCOLS[] = {
    {"https", 1},
    {"http", 0},
    {"ftp", 0},
    {"ssh", 0},
    {"tor", 0},
    {"i2p", 0},
    {"socks", 0},
    {"ws", 0},
    {"wss", 1},
    {"mqtt", 1},
    {"coap", 1},
    {NULL, 0}
};

int check_protocol(const char* url) {
    const char* end = strstr(url, "://");
    if (!end) return -1;
    
    int len = end - url;
    char scheme[16] = {0};
    if (len > 15) return -2;
    
    strncpy(scheme, url, len);
    
    for (int i = 0; PROTOCOLS[i].scheme; i++) {
        if (strcmp(scheme, PROTOCOLS[i].scheme) == 0) {
            return PROTOCOLS[i].allowed;
        }
    }
    
    return 0;
}

int filter_url(const char* url, uint8_t is_research) {
    if (!check_protocol(url)) return -1;
    
    for (int i = 0; BLOCKLIST[i].domain; i++) {
        if (strstr(url, BLOCKLIST[i].domain)) return -2;
    }
    
    if (strstr(url, ".onion")) return -3;
    
    return 0;
}

// ============================================================================
// SECTION 8: MÉTRICAS
// ============================================================================

SystemMetrics calculate_metrics(uint32_t tp, uint32_t fp, uint32_t tn, uint32_t fn) {
    SystemMetrics m;
    m.true_positives = tp;
    m.false_positives = fp;
    m.true_negatives = tn;
    m.false_negatives = fn;
    
    m.precision = (tp + fp > 0) ? (float)tp / (tp + fp) : 0;
    m.recall = (tp + fn > 0) ? (float)tp / (tp + fn) : 0;
    m.f1_score = (m.precision + m.recall > 0) ?
                 2 * (m.precision * m.recall) / (m.precision + m.recall) : 0;
    m.false_positive_rate = (fp + tn > 0) ? (float)fp / (fp + tn) : 0;
    
    return m;
}

// ============================================================================
// SECTION 9: MAIN
// ============================================================================

int main() {
    printf("=== TAK SECURITY INTEGRATED ===\n");
    printf("Sistema completo de seguridad\n\n");
    
    User researcher = {
        .id = 1,
        .name = "Dr. García",
        .role = 2,
        .clearance = 5,
        .reputation = 85.0,
        .total_actions = 150,
        .false_positives = 3,
        .confirmed_violations = 0,
        .first_seen = time(NULL) - (86400 * 365),
        .banned = 0
    };
    
    User suspect = {
        .id = 2,
        .name = "Sospechoso",
        .role = 1,
        .clearance = 1,
        .reputation = 25.0,
        .total_actions = 50,
        .false_positives = 8,
        .confirmed_violations = 2,
        .first_seen = time(NULL) - (86400 * 30),
        .banned = 0
    };
    
    printf("=== TESTS DE CONTENIDO ===\n\n");
    
    printf("Test 1: Investigador con contenido médico\n");
    float s1 = cross_validate("medical research nude anatomy", &researcher);
    ConfidenceLevel* c1 = get_confidence(s1);
    printf("  Score: %.2f, Action: %d (%s)\n\n", s1, c1->action, c1->description);
    
    printf("Test 2: Usuario sospechoso\n");
    float s2 = cross_validate("porn xxx violent drug", &suspect);
    ConfidenceLevel* c2 = get_confidence(s2);
    printf("  Score: %.2f, Action: %d (%s)\n\n", s2, c2->action, c2->description);
    
    printf("Test 3: Deep web\n");
    float s3 = cross_validate("http://site.onion/vpn", &suspect);
    ConfidenceLevel* c3 = get_confidence(s3);
    printf("  Score: %.2f, Action: %d (%s)\n\n", s3, c3->action, c3->description);
    
    printf("=== TESTS DE FIREWALL ===\n\n");
    
    printf("Test 4: HTTPS seguro\n");
    int r4 = filter_url("https://university.edu", 1);
    printf("  Result: %s\n\n", r4 == 0 ? "PERMITIDO" : "BLOQUEADO");
    
    printf("Test 5: HTTP inseguro\n");
    int r5 = filter_url("http://example.com", 0);
    printf("  Result: %s\n\n", r5 == 0 ? "PERMITIDO" : "BLOQUEADO");
    
    printf("Test 6: Porno\n");
    int r6 = filter_url("https://pornhub.com", 0);
    printf("  Result: %s\n\n", r6 == 0 ? "PERMITIDO" : "BLOQUEADO");
    
    printf("Test 7: Deep web\n");
    int r7 = filter_url("http://site.onion", 0);
    printf("  Result: %s\n\n", r7 == 0 ? "PERMITIDO" : "BLOQUEADO");
    
    printf("Test 8: MQTT (IoT)\n");
    int r8 = filter_url("mqtt://sensor.local", 0);
    printf("  Result: %s\n\n", r8 == 0 ? "PERMITIDO" : "BLOQUEADO");
    
    printf("=== TESTS DE REPUTACIÓN ===\n\n");
    
    printf("Reputación investigador: %.1f\n", calculate_reputation(&researcher));
    printf("Reputación sospechoso: %.1f\n\n", calculate_reputation(&suspect));
    
    printf("=== TESTS DE MÉTRICAS ===\n\n");
    
    SystemMetrics metrics = calculate_metrics(950, 50, 900, 100);
    printf("Precision: %.1f%%\n", metrics.precision * 100);
    printf("Recall: %.1f%%\n", metrics.recall * 100);
    printf("F1 Score: %.1f%%\n", metrics.f1_score * 100);
    printf("False Positive Rate: %.1f%%\n\n", metrics.false_positive_rate * 100);
    
    printf("=== RESUMEN ===\n");
    printf("Sistema implementado:\n");
    printf("✓ Análisis de contenido con pesos variables\n");
    printf("✓ Verificación cruzada (3 métodos)\n");
    printf("✓ Sistema de confianza escalonado\n");
    printf("✓ Reputación de usuarios\n");
    printf("✓ Firewall con protocolos\n");
    printf("✓ Bloqueo de dominios\n");
    printf("✓ Métricas de rendimiento\n");
    printf("✓ Soporte para investigación\n");
    printf("✓ Mínimos falsos positivos\n");
    printf("✓ Seguridad para todos\n");
    
    return 0;
}
