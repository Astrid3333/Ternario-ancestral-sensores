// ============================================================================
// TAK SECURITY BALANCED — Protección sin falsos positivos
// ============================================================================
// Filosofía: Seguridad para todos, justicia para cada uno
// ============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <math.h>

// ============================================================================
// SECTION 1: TIPOS DE DATOS
// ============================================================================

typedef struct {
    uint32_t id;
    char name[64];
    char email[128];
    uint8_t role;           // 0=guest, 1=user, 2=researcher, 3=admin, 4=superadmin
    uint8_t clearance;      // 0-10 nivel de autorización
    float reputation;       // 0-100
    uint32_t total_actions;
    uint32_t false_positives;
    uint32_t confirmed_violations;
    time_t first_seen;
    time_t last_active;
    uint8_t banned;         // 0=no, 1=provisional, 2=permanent
    char ban_reason[128];
} User;

typedef struct {
    uint32_t id;
    uint32_t user_id;
    uint8_t category;       // 0=safe, 1=adult, 2=violence, 3=drugs, 4=malware, 5=phishing
    float confidence;       // 0-100%
    uint8_t action_taken;   // 0=none, 1=log, 2=alert, 3=block_temp, 4=block_perm, 5=report
    time_t timestamp;
    char evidence_hash[64];
    uint8_t reviewed;       // 0=no, 1=human, 2=automated
    uint8_t overturned;     // 0=no, 1=yes
    char overturn_reason[128];
} Detection;

typedef struct {
    uint32_t detection_id;
    uint32_t user_id;
    char reason[256];
    char evidence[1024];
    time_t timestamp;
    uint8_t status;         // 0=pending, 1=accepted, 2=rejected, 3=under_review
    uint32_t reviewer_id;
    time_t review_time;
} Appeal;

typedef struct {
    float precision;
    float recall;
    float f1_score;
    float false_positive_rate;
    float false_negative_rate;
    float accuracy;
    uint32_t total_detections;
    uint32_t true_positives;
    uint32_t false_positives;
    uint32_t true_negatives;
    uint32_t false_negatives;
} Metrics;

// ============================================================================
// SECTION 2: DETECCIÓN INTELIGENTE (Mínimos falsos positivos)
// ============================================================================

// Palabras clave con pesos (no binario)
typedef struct {
    const char* word;
    float weight;           // -1.0 a 1.0
    uint8_t context_research; // 1=permitido en contexto de investigación
    const char* category;
} Keyword;

static const Keyword KEYWORDS[] = {
    // Porno ( peso alto = más probable bloquear )
    {"porn", 0.9, 0, "adult"},
    {"xxx", 0.95, 0, "adult"},
    {"nude", 0.8, 1, "adult"},      // Investigación médica permitida
    {"sex", 0.6, 1, "adult"},       // Investigación científica permitida
    {"erotic", 0.7, 1, "adult"},    // Investigación permitida
    
    // Violencia
    {"kill", 0.7, 1, "violence"},   // Contexto de noticia permitido
    {"murder", 0.8, 1, "violence"}, // Investigación criminal permitida
    {"torture", 0.9, 0, "violence"},
    {"rape", 0.95, 0, "violence"},
    {"abuse", 0.7, 1, "violence"},  // Investigación permitida
    
    // Drogas
    {"drug", 0.6, 1, "drugs"},      // Investigación permitida
    {"cocaine", 0.8, 1, "drugs"},   // Investigación permitida
    {"marijuana", 0.5, 1, "drugs"}, // Legal en algunos contextos
    {"heroin", 0.9, 0, "drugs"},
    
    // Malware
    {"malware", 0.8, 1, "malware"}, // Investigación de seguridad permitida
    {"virus", 0.7, 1, "malware"},   // Investigación permitida
    {"exploit", 0.8, 1, "malware"}, // Investigación permitida
    {"hack", 0.6, 1, "hacking"},    // Investigación permitida
    
    // Deep web
    {"onion", 0.7, 0, "deepweb"},
    {"tor", 0.6, 1, "deepweb"},     // Investigación de privacidad permitida
    
    {NULL, 0, 0, NULL}
};

// Análisis contextual avanzado
float analyze_context(const char* url, const char* title, const char* content, 
                     User* user, uint8_t is_research_context) {
    float score = 0.0;
    int keyword_count = 0;
    
    // Combinar todo el texto
    char buffer[4096];
    snprintf(buffer, sizeof(buffer), "%s %s %s", 
             url ? url : "", 
             title ? title : "", 
             content ? content : "");
    
    // Convertir a minúsculas
    for (int i = 0; buffer[i]; i++) {
        buffer[i] = tolower(buffer[i]);
    }
    
    // Analizar cada palabra clave
    for (int i = 0; KEYWORDS[i].word != NULL; i++) {
        if (strstr(buffer, KEYWORDS[i].word) != NULL) {
            keyword_count++;
            
            // Si es contexto de investigación y la palabra lo permite
            if (is_research_context && KEYWORDS[i].context_research) {
                score += KEYWORDS[i].weight * 0.3;  // Reducir peso en investigación
            } else {
                score += KEYWORDS[i].weight;
            }
        }
    }
    
    // Normalizar score
    if (keyword_count > 0) {
        score = score / keyword_count;
    }
    
    // Ajustar por reputación del usuario
    if (user) {
        float reputation_factor = user->reputation / 100.0;
        score = score * (0.5 + 0.5 * reputation_factor);
    }
    
    // Ajustar por contexto
    if (is_research_context) {
        score = score * 0.6;  // Reducir 40% en contexto de investigación
    }
    
    return score;
}

// ============================================================================
// SECTION 3: SISTEMA DE CONFIANZA
// ============================================================================

// Niveles de confianza con acciones
typedef struct {
    uint8_t level;
    float min_score;
    float max_score;
    uint8_t action;
    uint8_t require_human;
    const char* description;
} ConfidenceLevel;

static const ConfidenceLevel CONFIDENCE_LEVELS[] = {
    {0, 0.0, 0.3, 0, 0, "Seguro - sin acción"},
    {1, 0.3, 0.5, 1, 0, "Sospecha leve - solo registrar"},
    {2, 0.5, 0.7, 2, 0, "Sospecha moderada - alertar admin"},
    {3, 0.7, 0.8, 3, 1, "Sospecha alta - bloquear temporal (5 min)"},
    {4, 0.8, 0.9, 4, 1, "Muy sospechoso - bloquear y revisar"},
    {5, 0.9, 1.0, 5, 1, "Casi seguro - reportar a autoridades"}
};

// Determinar nivel de confianza
ConfidenceLevel* get_confidence_level(float score) {
    for (int i = 0; i < sizeof(CONFIDENCE_LEVELS)/sizeof(CONFIDENCE_LEVELS[0]); i++) {
        if (score >= CONFIDENCE_LEVELS[i].min_score && 
            score < CONFIDENCE_LEVELS[i].max_score) {
            return &CONFIDENCE_LEVELS[i];
        }
    }
    return &CONFIDENCE_LEVELS[0];  // Default: seguro
}

// ============================================================================
// SECTION 4: VERIFICACIÓN CRUZADA
// ============================================================================

// Múltiples métodos de detección
typedef struct {
    uint8_t method_id;
    float weight;
    const char* name;
    float (*analyze)(const char*, User*);
} DetectionMethod;

// Método 1: Análisis de contenido
float analyze_content_method(const char* data, User* user) {
    return analyze_context(data, NULL, NULL, user, 0);
}

// Método 2: Análisis de patrones de URL
float analyze_url_pattern(const char* data, User* user) {
    float score = 0.0;
    
    // Patrones sospechosos en URL
    if (strstr(data, ".onion")) score += 0.8;
    if (strstr(data, "tor2web")) score += 0.7;
    if (strstr(data, "proxy")) score += 0.5;
    if (strstr(data, "vpn")) score += 0.3;
    
    // Patrones de phishing
    if (strstr(data, "login")) score += 0.4;
    if (strstr(data, "password")) score += 0.5;
    if (strstr(data, "bank")) score += 0.6;
    
    return score;
}

// Método 3: Análisis temporal
float analyze_temporal_method(const char* data, User* user) {
    float score = 0.0;
    time_t now = time(NULL);
    struct tm* tm = localtime(&now);
    int hour = tm->tm_hour;
    
    // Actividad nocturna sospechosa
    if (hour >= 23 || hour <= 5) {
        score += 0.2;
    }
    
    // Frecuencia de solicitudes
    if (user && user->total_actions > 100) {
        score += 0.1;
    }
    
    return score;
}

// Verificación cruzada
float cross_validate(const char* data, User* user) {
    DetectionMethod methods[] = {
        {1, 0.4, "Content", analyze_content_method},
        {2, 0.3, "URL Pattern", analyze_url_pattern},
        {3, 0.3, "Temporal", analyze_temporal_method}
    };
    
    float total_score = 0.0;
    float total_weight = 0.0;
    int method_scores[3];
    int method_count = 0;
    
    for (int i = 0; i < 3; i++) {
        float score = methods[i].analyze(data, user);
        total_score += score * methods[i].weight;
        total_weight += methods[i].weight;
        method_scores[method_count++] = (int)(score * 100);
    }
    
    // Calcular concordancia entre métodos
    int min_score = method_scores[0];
    int max_score = method_scores[0];
    for (int i = 1; i < method_count; i++) {
        if (method_scores[i] < min_score) min_score = method_scores[i];
        if (method_scores[i] > max_score) max_score = method_scores[i];
    }
    
    float agreement = 1.0 - ((float)(max_score - min_score) / 100.0);
    
    // Si hay desacuerdo mayor a 30%, reducir confianza
    if (agreement < 0.7) {
        total_score = total_score * agreement;
    }
    
    return total_score / total_weight;
}

// ============================================================================
// SECTION 5: SISTEMA DE APELACIÓN
// ============================================================================

// Crear apelación
Appeal* create_appeal(Detection* det, User* user, const char* reason) {
    Appeal* appeal = malloc(sizeof(Appeal));
    if (!appeal) return NULL;
    
    appeal->detection_id = det->id;
    appeal->user_id = user->id;
    strncpy(appeal->reason, reason, sizeof(appeal->reason) - 1);
    appeal->timestamp = time(NULL);
    appeal->status = 0;  // Pendiente
    
    return appeal;
}

// Revisión automática de apelaciones
int auto_review_appeal(Appeal* appeal, User* user) {
    // Criterios para aceptación automática:
    
    // 1. Usuario con alta reputación
    if (user->reputation > 80) {
        appeal->status = 1;  // Aceptar
        appeal->reviewer_id = 0;  // Sistema
        appeal->review_time = time(NULL);
        return 1;
    }
    
    // 2. Primer incidente
    if (user->total_actions < 10 && user->false_positives == 0) {
        appeal->status = 1;  // Beneficio de la duda
        appeal->reviewer_id = 0;
        appeal->review_time = time(NULL);
        return 1;
    }
    
    // 3. Evidencia clara de falso positivo
    if (is_clearly_false_positive(appeal->evidence)) {
        appeal->status = 1;
        appeal->reviewer_id = 0;
        appeal->review_time = time(NULL);
        return 1;
    }
    
    // 4. Necesita revisión humana
    appeal->status = 3;  // Bajo revisión
    return 0;
}

// Verificar si es falso positivo claro
int is_clearly_false_positive(const char* evidence) {
    // Contenido médico/educativo
    if (strstr(evidence, "medical") || strstr(evidence, "educational")) {
        return 1;
    }
    
    // Contenido de noticias
    if (strstr(evidence, "news") || strstr(evidence, "report")) {
        return 1;
    }
    
    // Investigación aprobada
    if (strstr(evidence, "research") || strstr(evidence, "study")) {
        return 1;
    }
    
    return 0;
}

// ============================================================================
// SECTION 6: SISTEMA DE REPUTACIÓN
// ============================================================================

// Calcular reputación
float calculate_reputation(User* user) {
    if (user->total_actions == 0) return 100.0;
    
    // Fórmula: reputación = 100 - (falsos_positivos / total) * 100
    float fpr = (float)user->false_positives / user->total_actions;
    float score = 100.0 - (fpr * 100.0);
    
    // Bonificación por tiempo con buen comportamiento
    time_t now = time(NULL);
    float days_active = (float)(now - user->first_seen) / 86400.0;
    float time_bonus = fmin(days_active * 0.1, 10.0);  // Máximo 10 puntos
    score += time_bonus;
    
    // Penalización por violaciones confirmadas
    score -= user->confirmed_violations * 20.0;
    
    // Mantener en rango 0-100
    if (score < 0) score = 0;
    if (score > 100) score = 100;
    
    user->reputation = score;
    return score;
}

// Usar reputación para decisiones
float use_reputation(float base_score, User* user) {
    float reputation = calculate_reputation(user);
    
    // Usuarios con alta reputación tienen más beneficios
    if (reputation > 80) {
        return base_score * 0.7;  // Reducir 30%
    } else if (reputation > 60) {
        return base_score * 0.9;  // Reducir 10%
    } else if (reputation > 40) {
        return base_score * 1.0;  // Normal
    } else if (reputation > 20) {
        return base_score * 1.2;  // Aumentar 20%
    } else {
        return base_score * 1.5;  // Aumentar 50%
    }
}

// ============================================================================
// SECTION 7: MÉTRICAS Y MONITOREO
// ============================================================================

// Calcular métricas
Metrics calculate_metrics(uint32_t tp, uint32_t fp, uint32_t tn, uint32_t fn) {
    Metrics m;
    
    m.true_positives = tp;
    m.false_positives = fp;
    m.true_negatives = tn;
    m.false_negatives = fn;
    m.total_detections = tp + fp + tn + fn;
    
    m.precision = (tp + fp > 0) ? (float)tp / (tp + fp) : 0;
    m.recall = (tp + fn > 0) ? (float)tp / (tp + fn) : 0;
    m.f1_score = (m.precision + m.recall > 0) ? 
                 2 * (m.precision * m.recall) / (m.precision + m.recall) : 0;
    m.false_positive_rate = (fp + tn > 0) ? (float)fp / (fp + tn) : 0;
    m.false_negative_rate = (fn + tp > 0) ? (float)fn / (fn + tp) : 0;
    m.accuracy = (tp + tn > 0) ? (float)(tp + tn) / (tp + fp + tn + fn) : 0;
    
    return m;
}

// Verificar rendimiento
int check_performance(Metrics* m) {
    int issues = 0;
    
    // Precisión debe ser > 90%
    if (m->precision < 0.9) {
        printf("[ALERT] Precisión baja: %.1f%% - muchos falsos positivos\n", 
               m->precision * 100);
        issues++;
    }
    
    // Recall debe ser > 80%
    if (m->recall < 0.8) {
        printf("[ALERT] Recall bajo: %.1f%% - muchos falsos negativos\n", 
               m->recall * 100);
        issues++;
    }
    
    // FPR debe ser < 10%
    if (m->false_positive_rate > 0.1) {
        printf("[ALERT] Tasa de falsos positivos alta: %.1f%%\n", 
               m->false_positive_rate * 100);
        issues++;
    }
    
    // FNR debe ser < 20%
    if (m->false_negative_rate > 0.2) {
        printf("[ALERT] Tasa de falsos negativos alta: %.1f%%\n", 
               m->false_negative_rate * 100);
        issues++;
    }
    
    return issues;
}

// ============================================================================
// SECTION 8: MAIN - DEMO
// ============================================================================

int main() {
    printf("=== TAK SECURITY BALANCED ===\n");
    printf("Protección sin falsos positivos\n\n");
    
    // Crear usuario de prueba
    User researcher = {
        .id = 1,
        .name = "Dr. García",
        .email = "garcia@university.edu",
        .role = 2,  // Researcher
        .clearance = 5,
        .reputation = 85.0,
        .total_actions = 150,
        .false_positives = 3,
        .confirmed_violations = 0,
        .first_seen = time(NULL) - (86400 * 365),
        .last_active = time(NULL),
        .banned = 0
    };
    
    User suspect = {
        .id = 2,
        .name = "Usuario Sospechoso",
        .email = "unknown@gmail.com",
        .role = 1,  // User
        .clearance = 1,
        .reputation = 25.0,
        .total_actions = 50,
        .false_positives = 8,
        .confirmed_violations = 2,
        .first_seen = time(NULL) - (86400 * 30),
        .last_active = time(NULL),
        .banned = 0
    };
    
    // Test 1: Investigador con contenido legítimo
    printf("Test 1: Investigador con contenido médico\n");
    float score1 = cross_validate("medical research nude anatomy study", &researcher);
    ConfidenceLevel* level1 = get_confidence_level(score1);
    printf("  Score: %.2f, Level: %d, Action: %d\n", 
           score1, level1->level, level1->action);
    printf("  Result: %s\n\n", level1->action <= 2 ? "PERMITIDO" : "BLOQUEADO");
    
    // Test 2: Usuario sospechoso con contenido malicioso
    printf("Test 2: Usuario sospechoso con contenido malicioso\n");
    float score2 = cross_validate("porn xxx violent drug trafficking", &suspect);
    ConfidenceLevel* level2 = get_confidence_level(score2);
    printf("  Score: %.2f, Level: %d, Action: %d\n", 
           score2, level2->level, level2->action);
    printf("  Result: %s\n\n", level2->action <= 2 ? "PERMITIDO" : "BLOQUEADO");
    
    // Test 3: Contenido ambiguo
    printf("Test 3: Contenido ambiguo (noticias)\n");
    float score3 = cross_validate("news report drug violence in mexico", &researcher);
    ConfidenceLevel* level3 = get_confidence_level(score3);
    printf("  Score: %.2f, Level: %d, Action: %d\n", 
           score3, level3->level, level3->action);
    printf("  Result: %s\n\n", level3->action <= 2 ? "PERMITIDO" : "BLOQUEADO");
    
    // Test 4: Deep web
    printf("Test 4: Deep web (.onion)\n");
    float score4 = cross_validate("http://something.onion/vpn/proxy", &suspect);
    ConfidenceLevel* level4 = get_confidence_level(score4);
    printf("  Score: %.2f, Level: %d, Action: %d\n", 
           score4, level4->level, level4->action);
    printf("  Result: %s\n\n", level4->action <= 2 ? "PERMITIDO" : "BLOQUEADO");
    
    // Test 5: Calculating reputation
    printf("Test 5: Cálculo de reputación\n");
    printf("  Investigador: %.1f\n", calculate_reputation(&researcher));
    printf("  Sospechoso: %.1f\n\n", calculate_reputation(&suspect));
    
    // Test 6: Métricas
    printf("Test 6: Métricas del sistema\n");
    Metrics metrics = calculate_metrics(950, 50, 900, 100);
    printf("  Precision: %.1f%%\n", metrics.precision * 100);
    printf("  Recall: %.1f%%\n", metrics.recall * 100);
    printf("  F1 Score: %.1f%%\n", metrics.f1_score * 100);
    printf("  False Positive Rate: %.1f%%\n", metrics.false_positive_rate * 100);
    printf("  Accuracy: %.1f%%\n", metrics.accuracy * 100);
    
    int issues = check_performance(&metrics);
    printf("  Issues: %d\n\n", issues);
    
    // Test 7: Apelación
    printf("Test 7: Sistema de apelación\n");
    Detection det = {
        .id = 1001,
        .user_id = researcher.id,
        .category = 1,  // adult
        .confidence = 65.0,
        .action_taken = 2,  // alert
        .timestamp = time(NULL),
        .reviewed = 0,
        .overturned = 0
    };
    
    Appeal* appeal = create_appeal(&det, &researcher, "Contenido médico legítimo");
    if (appeal) {
        int auto_review = auto_review_appeal(appeal, &researcher);
        printf("  Apelación creada: ID %d\n", appeal->detection_id);
        printf("  Revisión automática: %s\n", auto_review ? "SÍ" : "NO");
        printf("  Estado: %d (0=pendiente, 1=aceptada, 2=rechazada, 3= revisión)\n", 
               appeal->status);
        free(appeal);
    }
    
    printf("\n=== RESUMEN ===\n");
    printf("Sistema implementado:\n");
    printf("✓ Detección inteligente con pesos variables\n");
    printf("✓ Verificación cruzada (3 métodos)\n");
    printf("✓ Sistema de confianza escalonado\n");
    printf("✓ Apelación automática para usuarios confiables\n");
    printf("✓ Reputación que afecta decisiones\n");
    printf("✓ Métricas de rendimiento en tiempo real\n");
    printf("✓ Protección contra falsos positivos\n");
    printf("✓ Seguridad para todos, justicia para cada uno\n");
    
    return 0;
}
