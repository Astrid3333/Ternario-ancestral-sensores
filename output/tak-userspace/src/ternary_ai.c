/**
 * ternary_ai.c — Motor de IA Ternaria para TRITOS
 *
 * Sistema de razonamiento con lógica ternaria:
 *   ⊕ (+1) = Certeza / Confirmado
 *    0 (0)  = Incierto / Necesita verificación
 *   ⊖ (-1) = Descartado / Falso
 *
 * Aplicaciones:
 * - Clasificación de papers por relevancia ternaria
 * - Análisis de datos de sensores con incertidumbre
 * - Razonamiento bajo incertidumbre
 * - Scoring de confianza para resultados científicos
 *
 * Uso: ./tritos_ai <comando> [args]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>

/* ==================== TERNARY LOGIC ENGINE ==================== */

typedef int8_t trit_t;
#define TRIT_NEG  (-1)
#define TRIT_ZERO (0)
#define TRIT_POS  (1)

typedef struct {
    trit_t value;
    float confidence;  /* 0.0 to 1.0 */
    char label[64];
} ternary_assertion_t;

typedef struct {
    ternary_assertion_t *assertions;
    int count;
    int capacity;
    char context[256];
} ternary_context_t;

/* Create context */
ternary_context_t* tc_create(const char *context) {
    ternary_context_t *tc = malloc(sizeof(ternary_context_t));
    tc->capacity = 64;
    tc->assertions = malloc(sizeof(ternary_assertion_t) * tc->capacity);
    tc->count = 0;
    strncpy(tc->context, context, 255);
    return tc;
}

/* Add assertion */
void tc_add(ternary_context_t *tc, trit_t value, float confidence, const char *label) {
    if (tc->count >= tc->capacity) {
        tc->capacity *= 2;
        tc->assertions = realloc(tc->assertions, sizeof(ternary_assertion_t) * tc->capacity);
    }
    ternary_assertion_t *a = &tc->assertions[tc->count++];
    a->value = value;
    a->confidence = confidence;
    strncpy(a->label, label, 63);
}

/* Ternary AND with confidence */
trit_t tc_and(trit_t a, float ca, trit_t b, float cb, float *result_conf) {
    trit_t r;
    if (a == TRIT_NEG || b == TRIT_NEG) {
        r = TRIT_NEG;
        *result_conf = fminf(ca, cb);
    } else if (a == TRIT_POS && b == TRIT_POS) {
        r = TRIT_POS;
        *result_conf = fminf(ca, cb);
    } else {
        r = TRIT_ZERO;
        *result_conf = fmaxf(1.0f - ca, 1.0f - cb) * 0.5f;
    }
    return r;
}

/* Ternary OR with confidence */
trit_t tc_or(trit_t a, float ca, trit_t b, float cb, float *result_conf) {
    trit_t r;
    if (a == TRIT_POS || b == TRIT_POS) {
        r = TRIT_POS;
        *result_conf = fmaxf(ca, cb);
    } else if (a == TRIT_NEG && b == TRIT_NEG) {
        r = TRIT_NEG;
        *result_conf = fminf(ca, cb);
    } else {
        r = TRIT_ZERO;
        *result_conf = 0.5f;
    }
    return r;
}

/* Ternary NOT */
trit_t tc_not(trit_t a) { return -a; }

/* Weighted vote from multiple assertions */
trit_t tc_vote(ternary_context_t *tc, float *avg_confidence) {
    float sum = 0;
    float weight_sum = 0;
    int pos = 0, neg = 0, zero = 0;

    for (int i = 0; i < tc->count; i++) {
        float w = tc->assertions[i].confidence;
        sum += tc->assertions[i].value * w;
        weight_sum += w;
        if (tc->assertions[i].value > 0) pos++;
        else if (tc->assertions[i].value < 0) neg++;
        else zero++;
    }

    *avg_confidence = weight_sum > 0 ? weight_sum / tc->count : 0;

    if (weight_sum == 0) return TRIT_ZERO;
    float normalized = sum / weight_sum;
    if (normalized > 0.3f) return TRIT_POS;
    if (normalized < -0.3f) return TRIT_NEG;
    return TRIT_ZERO;
}

/* Print assertion */
void tc_print_assertion(ternary_assertion_t *a) {
    const char *sym;
    switch (a->value) {
        case TRIT_NEG: sym = "⊖"; break;
        case TRIT_ZERO: sym = "0"; break;
        case TRIT_POS: sym = "⊕"; break;
        default: sym = "?";
    }
    printf("  %s %-40s conf=%.0f%%\n", sym, a->label, a->confidence * 100);
}

/* Print full context */
void tc_print(ternary_context_t *tc) {
    printf("\n┌─── Contexto: %s ───┐\n", tc->context);
    for (int i = 0; i < tc->count; i++) {
        tc_print_assertion(&tc->assertions[i]);
    }

    float avg_conf;
    trit_t verdict = tc_vote(tc, &avg_conf);
    const char *vsym;
    switch (verdict) {
        case TRIT_NEG: vsym = "⊖ DESCARTADO"; break;
        case TRIT_ZERO: vsym = "0 INCIERTO"; break;
        case TRIT_POS: vsym = "⊕ CONFIRMADO"; break;
        default: vsym = "?";
    }
    printf("├─── Veredicto: %s (confianza promedio: %.0f%%) ───┤\n", vsym, avg_conf * 100);
    printf("└────────────────────────────────────────────────────┘\n\n");
}

void tc_destroy(ternary_context_t *tc) {
    free(tc->assertions);
    free(tc);
}

/* ==================== PAPER ANALYSIS ==================== */

typedef struct {
    char title[256];
    char authors[256];
    char abstract[1024];
    char year[16];
    char source[64];
} paper_t;

/* Analyze paper relevance to ternary systems */
ternary_context_t* analyze_paper(paper_t *paper) {
    ternary_context_t *tc = tc_create(paper->title);

    /* Check title for ternary keywords */
    char lower[256];
    strncpy(lower, paper->title, 255);
    for (int i = 0; lower[i]; i++) lower[i] = tolower(lower[i]);

    if (strstr(lower, "ternary") || strstr(lower, "ternario") || strstr(lower, "balanced ternary")) {
        tc_add(tc, TRIT_POS, 0.95f, "Title mentions ternary systems");
    } else if (strstr(lower, "three-state") || strstr(lower, "trinary") || strstr(lower, "trit")) {
        tc_add(tc, TRIT_POS, 0.85f, "Title mentions three-state systems");
    } else if (strstr(lower, "base-3") || strstr(lower, "base 3") || strstr(lower, "triadic")) {
        tc_add(tc, TRIT_POS, 0.80f, "Title mentions base-3");
    } else {
        tc_add(tc, TRIT_ZERO, 0.5f, "Title has no direct ternary reference");
    }

    /* Check abstract */
    char abs_lower[1024];
    strncpy(abs_lower, paper->abstract, 1023);
    for (int i = 0; abs_lower[i]; i++) abs_lower[i] = tolower(abs_lower[i]);

    int ternary_count = 0;
    const char *keywords[] = {"ternary", "ternario", "balanced ternary", "trit", "three-valued", "three-state", "base-3", NULL};
    for (int k = 0; keywords[k]; k++) {
        const char *p = abs_lower;
        while ((p = strstr(p, keywords[k])) != NULL) { ternary_count++; p++; }
    }

    if (ternary_count >= 5) tc_add(tc, TRIT_POS, 0.90f, "Abstract heavily references ternary");
    else if (ternary_count >= 2) tc_add(tc, TRIT_POS, 0.70f, "Abstract references ternary");
    else if (ternary_count == 1) tc_add(tc, TRIT_ZERO, 0.50f, "Abstract has one ternary mention");
    else tc_add(tc, TRIT_NEG, 0.60f, "Abstract has no ternary references");

    /* Check for IoT/sensor relevance */
    if (strstr(abs_lower, "sensor") || strstr(abs_lower, "iot") || strstr(abs_lower, "internet of things")) {
        tc_add(tc, TRIT_POS, 0.75f, "Relevant to IoT/sensors");
    } else {
        tc_add(tc, TRIT_ZERO, 0.40f, "Not directly IoT-related");
    }

    /* Check for compression relevance */
    if (strstr(abs_lower, "compression") || strstr(abs_lower, "compresión") || strstr(abs_lower, "encoding")) {
        tc_add(tc, TRIT_POS, 0.70f, "Relevant to compression");
    }

    /* Check year recency */
    int year = atoi(paper->year);
    if (year >= 2023) tc_add(tc, TRIT_POS, 0.80f, "Recent publication (2023+)");
    else if (year >= 2020) tc_add(tc, TRIT_ZERO, 0.60f, "Moderately recent (2020-2022)");
    else tc_add(tc, TRIT_NEG, 0.50f, "Older publication (pre-2020)");

    return tc;
}

/* ==================== SENSOR ANALYSIS ==================== */

typedef struct {
    float temperature;
    float humidity;
    float pressure;
    float light;
    int motion;
    float acoustic;
    int air_quality;
} sensor_data_t;

/* Analyze sensor readings with ternary logic */
ternary_context_t* analyze_sensors(sensor_data_t *sensors) {
    ternary_context_t *tc = tc_create("Sensor Network Analysis");

    /* Temperature analysis */
    if (sensors->temperature > 40.0f)
        tc_add(tc, TRIT_POS, 0.90f, "Temperature CRITICAL (>40°C)");
    else if (sensors->temperature > 30.0f)
        tc_add(tc, TRIT_POS, 0.70f, "Temperature HIGH (30-40°C)");
    else if (sensors->temperature > 15.0f)
        tc_add(tc, TRIT_ZERO, 0.80f, "Temperature NORMAL (15-30°C)");
    else if (sensors->temperature > 5.0f)
        tc_add(tc, TRIT_NEG, 0.70f, "Temperature LOW (5-15°C)");
    else
        tc_add(tc, TRIT_NEG, 0.90f, "Temperature CRITICAL LOW (<5°C)");

    /* Humidity analysis */
    if (sensors->humidity > 80.0f)
        tc_add(tc, TRIT_POS, 0.85f, "Humidity HIGH (>80%) - rain likely");
    else if (sensors->humidity > 40.0f)
        tc_add(tc, TRIT_ZERO, 0.80f, "Humidity NORMAL (40-80%)");
    else
        tc_add(tc, TRIT_NEG, 0.75f, "Humidity LOW (<40%) - dry conditions");

    /* Pressure analysis */
    if (sensors->pressure < 1000.0f)
        tc_add(tc, TRIT_NEG, 0.70f, "Pressure LOW (<1000 hPa) - storm possible");
    else if (sensors->pressure > 1020.0f)
        tc_add(tc, TRIT_POS, 0.75f, "Pressure HIGH (>1020 hPa) - stable");
    else
        tc_add(tc, TRIT_ZERO, 0.80f, "Pressure NORMAL (1000-1020 hPa)");

    /* Motion detection */
    if (sensors->motion)
        tc_add(tc, TRIT_POS, 0.85f, "Motion DETECTED");
    else
        tc_add(tc, TRIT_NEG, 0.80f, "No motion detected");

    /* Air quality */
    if (sensors->air_quality > 200)
        tc_add(tc, TRIT_POS, 0.95f, "Air quality HAZARDOUS (>200 AQI)");
    else if (sensors->air_quality > 100)
        tc_add(tc, TRIT_POS, 0.75f, "Air quality UNHEALTHY (100-200 AQI)");
    else if (sensors->air_quality > 50)
        tc_add(tc, TRIT_ZERO, 0.70f, "Air quality MODERATE (50-100 AQI)");
    else
        tc_add(tc, TRIT_NEG, 0.80f, "Air quality GOOD (<50 AQI)");

    /* Overall environmental assessment */
    float conf;
    trit_t env_verdict = tc_vote(tc, &conf);
    tc_add(tc, env_verdict, conf, "Overall environmental assessment");

    return tc;
}

/* ==================== HYPOTHESIS TESTING ==================== */

typedef struct {
    char hypothesis[256];
    int n_evidence;
    struct {
        char description[256];
        trit_t supports;  /* +1 supports, -1 contradicts, 0 neutral */
        float strength;   /* 0.0 to 1.0 */
    } evidence[20];
} hypothesis_t;

hypothesis_t* hypothesis_create(const char *h) {
    hypothesis_t *hyp = malloc(sizeof(hypothesis_t));
    strncpy(hyp->hypothesis, h, 255);
    hyp->n_evidence = 0;
    return hyp;
}

void hypothesis_add_evidence(hypothesis_t *hyp, const char *desc, trit_t supports, float strength) {
    if (hyp->n_evidence >= 20) return;
    int i = hyp->n_evidence++;
    strncpy(hyp->evidence[i].description, desc, 255);
    hyp->evidence[i].supports = supports;
    hyp->evidence[i].strength = strength;
}

ternary_context_t* hypothesis_evaluate(hypothesis_t *hyp) {
    ternary_context_t *tc = tc_create(hyp->hypothesis);

    for (int i = 0; i < hyp->n_evidence; i++) {
        tc_add(tc, hyp->evidence[i].supports, hyp->evidence[i].strength,
               hyp->evidence[i].description);
    }

    return tc;
}

void hypothesis_destroy(hypothesis_t *hyp) {
    free(hyp);
}

/* ==================== RESPONSE CLASSIFIER ==================== */

typedef struct {
    char input[1024];
    trit_t classification;
    float confidence;
    char reasoning[512];
} classified_response_t;

classified_response_t classify_response(const char *input) {
    classified_response_t r;
    strncpy(r.input, input, 1023);

    char lower[1024];
    strncpy(lower, input, 1023);
    for (int i = 0; lower[i]; i++) lower[i] = tolower(lower[i]);

    /* Positive indicators */
    int pos = 0, neg = 0, zero = 0;

    const char *pos_words[] = {"yes", "si", "correct", "confirm", "true", "verdadero",
        "funciona", "éxito", "aprobado", "válido", "probado", "verificado", NULL};
    const char *neg_words[] = {"no", "not", "false", "falso", "error", "fallo",
        "failed", "rejected", "inválido", "incorrecto", "descartado", NULL};
    const char *uncertain_words[] = {"maybe", "talvez", "posiblemente", "incertain",
        "unclear", "no sé", "pendiente", "verificar", "check", NULL};

    for (int w = 0; pos_words[w]; w++) if (strstr(lower, pos_words[w])) pos++;
    for (int w = 0; neg_words[w]; w++) if (strstr(lower, neg_words[w])) neg++;
    for (int w = 0; uncertain_words[w]; w++) if (strstr(lower, uncertain_words[w])) zero++;

    int total = pos + neg + zero;
    if (total == 0) {
        r.classification = TRIT_ZERO;
        r.confidence = 0.3f;
        snprintf(r.reasoning, 511, "No sentiment indicators found");
    } else {
        float pos_ratio = (float)pos / total;
        float neg_ratio = (float)neg / total;

        if (pos_ratio > 0.5f) {
            r.classification = TRIT_POS;
            r.confidence = pos_ratio;
            snprintf(r.reasoning, 511, "Positive signals: %d/%d (%.0f%%)", pos, total, pos_ratio * 100);
        } else if (neg_ratio > 0.5f) {
            r.classification = TRIT_NEG;
            r.confidence = neg_ratio;
            snprintf(r.reasoning, 511, "Negative signals: %d/%d (%.0f%%)", neg, total, neg_ratio * 100);
        } else {
            r.classification = TRIT_ZERO;
            r.confidence = 1.0f - fmaxf(pos_ratio, neg_ratio);
            snprintf(r.reasoning, 511, "Mixed signals: +%d/-%d/~%d", pos, neg, zero);
        }
    }

    return r;
}

/* ==================== TERNARY CHAT ==================== */

/* Simple pattern-matching chatbot with ternary confidence */
typedef struct {
    char pattern[128];
    char response[512];
    trit_t sentiment;
} chat_pattern_t;

static chat_pattern_t chat_patterns[] = {
    {"hola", "Hola! Soy TRITOS AI. Puedo analizar papers, datos de sensores, o hipótesis con lógica ternaria.", TRIT_POS},
    {"hello", "Hello! I'm TRITOS AI. I can analyze papers, sensor data, or hypotheses using ternary logic.", TRIT_POS},
    {"ayuda", "Puedo: (1) analizar papers con lógica ternaria, (2) evaluar sensores, (3) testear hipótesis, (4) clasificar respuestas.", TRIT_ZERO},
    {"help", "I can: (1) analyze papers with ternary logic, (2) evaluate sensors, (3) test hypotheses, (4) classify responses.", TRIT_ZERO},
    {"ternary", "The ternary system uses 3 values: ⊕ (+1=certain), 0 (uncertain), ⊖ (-1=false). More nuanced than binary.", TRIT_POS},
    {"sensor", "I can analyze sensor networks. Provide temperature, humidity, pressure data and I'll give a ternary assessment.", TRIT_ZERO},
    {"paper", "I can analyze research papers for ternary relevance. Provide title, abstract, and I'll score them.", TRIT_ZERO},
    {"hypothesis", "I can evaluate hypotheses with ternary evidence scoring. State your hypothesis and I'll gather evidence.", TRIT_ZERO},
    {"binary vs ternary", "Binary: 2 states (0/1). Ternary: 3 states (⊖/0/⊕). Ternary handles uncertainty natively - no need to force binary decisions.", TRIT_POS},
    {"open science", "TRITOS integrates with OpenScience for paper search and Octave for computation. Use the Research section to connect.", TRIT_POS},
    {"error", "An error? Let me analyze: ⊖ indicates a definite problem, 0 means uncertain, ⊕ means partial success.", TRIT_ZERO},
    {"gracias", "De nada! La lógica ternaria permite expresar la incertidumbre de forma natural.", TRIT_POS},
    {"thanks", "You're welcome! Ternary logic allows expressing uncertainty naturally.", TRIT_POS},
};

typedef struct {
    char message[1024];
    char response[2048];
    trit_t sentiment;
    float confidence;
} chat_exchange_t;

chat_exchange_t ternary_chat(const char *user_input) {
    chat_exchange_t exchange;
    strncpy(exchange.message, user_input, 1023);

    char lower[1024];
    strncpy(lower, user_input, 1023);
    for (int i = 0; lower[i]; i++) lower[i] = tolower(lower[i]);

    /* Try to match patterns */
    for (int i = 0; i < (int)(sizeof(chat_patterns) / sizeof(chat_pattern_t)); i++) {
        if (strstr(lower, chat_patterns[i].pattern)) {
            strncpy(exchange.response, chat_patterns[i].response, 2047);
            exchange.sentiment = chat_patterns[i].sentiment;
            exchange.confidence = 0.85f;
            return exchange;
        }
    }

    /* Classify the input */
    classified_response_t cls = classify_response(user_input);
    exchange.sentiment = cls.classification;
    exchange.confidence = cls.confidence;

    /* Generate response based on classification */
    const char *sym;
    switch (cls.classification) {
        case TRIT_NEG: sym = "⊖"; break;
        case TRIT_ZERO: sym = "0"; break;
        case TRIT_POS: sym = "⊕"; break;
        default: sym = "?";
    }

    snprintf(exchange.response, 2047,
        "Analicé tu mensaje: %s (%.0f%% confianza)\n"
        "Razonamiento: %s\n\n"
        "Para un análisis más profundo, probá:\n"
        "  • 'analyze paper <título>' — análisis de paper\n"
        "  • 'evaluate sensors' — evaluar red de sensores\n"
        "  • 'test <hipótesis>' — testear hipótesis",
        sym, cls.confidence * 100, cls.reasoning);

    return exchange;
}

/* ==================== CLI ==================== */

void print_help(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║         🧠 TRITOS — IA Ternaria v1.0 🧠                 ║\n");
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    printf("║                                                         ║\n");
    printf("║  chat <msg>          Chat con lógica ternaria            ║\n");
    printf("║  analyze <text>      Clasificar sentimiento ternario     ║\n");
    printf("║  paper <title>       Analizar relevancia de paper        ║\n");
    printf("║  sensors             Demo de análisis de sensores        ║\n");
    printf("║  hypothesis <text>   Evaluar hipótesis                   ║\n");
    printf("║  demo                Demo completo                       ║\n");
    printf("║  help                Esta ayuda                          ║\n");
    printf("║                                                         ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
}

void demo_sensors(void) {
    printf("=== Demo: Análisis de Sensores Ternario ===\n\n");

    sensor_data_t sensors = {
        .temperature = 35.5f,
        .humidity = 75.0f,
        .pressure = 1005.0f,
        .light = 45000.0f,
        .motion = 1,
        .acoustic = 65.0f,
        .air_quality = 85
    };

    printf("Lecturas:\n");
    printf("  Temperatura: %.1f°C\n", sensors.temperature);
    printf("  Humedad:     %.0f%%\n", sensors.humidity);
    printf("  Presión:     %.0f hPa\n", sensors.pressure);
    printf("  Luz:         %.0f lux\n", sensors.light);
    printf("  Movimiento:  %s\n", sensors.motion ? "SÍ" : "NO");
    printf("  Acústica:    %.0f dB\n", sensors.acoustic);
    printf("  Calidad aire:%d AQI\n", sensors.air_quality);

    ternary_context_t *tc = analyze_sensors(&sensors);
    tc_print(tc);
    tc_destroy(tc);
}

void demo_paper(void) {
    printf("=== Demo: Análisis de Paper ===\n\n");

    paper_t paper;
    strncpy(paper.title, "Balanced Ternary Computing for IoT Sensor Networks", 255);
    strncpy(paper.authors, "Astrid et al.", 255);
    strncpy(paper.abstract, "This paper presents a novel approach to IoT sensor data compression using balanced ternary encoding. "
            "We demonstrate 8x compression ratios for ternary sensor data with energy savings of 80% "
            "compared to binary encoding. Our ternary encoding scheme uses base-3 representation with "
            "three states: negative, zero, and positive. Results show that ternary systems are particularly "
            "effective for environmental sensor networks with inherent uncertainty.", 1023);
    strncpy(paper.year, "2026", 15);
    strncpy(paper.source, "arXiv", 63);

    printf("Title: %s\n", paper.title);
    printf("Authors: %s\n", paper.authors);
    printf("Year: %s\n", paper.year);

    ternary_context_t *tc = analyze_paper(&paper);
    tc_print(tc);
    tc_destroy(tc);
}

void demo_hypothesis(void) {
    printf("=== Demo: Evaluación de Hipótesis ===\n\n");

    hypothesis_t *hyp = hypothesis_create("Los sistemas ternarios son superiores a los binarios para IoT");

    hypothesis_add_evidence(hyp, "8x compression demonstrated", TRIT_POS, 0.90f);
    hypothesis_add_evidence(hyp, "80% energy savings measured", TRIT_POS, 0.85f);
    hypothesis_add_evidence(hyp, "Hardware not widely available", TRIT_NEG, 0.70f);
    hypothesis_add_evidence(hyp, "Software ecosystem limited", TRIT_NEG, 0.65f);
    hypothesis_add_evidence(hyp, "Natural handling of uncertainty", TRIT_POS, 0.80f);
    hypothesis_add_evidence(hyp, "No standardization yet", TRIT_NEG, 0.60f);

    printf("Hipótesis: %s\n", hyp->hypothesis);
    printf("Evidencia: %d items\n\n", hyp->n_evidence);

    ternary_context_t *tc = hypothesis_evaluate(hyp);
    tc_print(tc);
    tc_destroy(tc);
    hypothesis_destroy(hyp);
}

int main(int argc, char *argv[]) {
    if (argc < 2) { print_help(); return 0; }

    if (strcmp(argv[1], "help") == 0) { print_help(); return 0; }

    if (strcmp(argv[1], "demo") == 0) {
        demo_sensors();
        demo_paper();
        demo_hypothesis();
        return 0;
    }

    if (strcmp(argv[1], "sensors") == 0) { demo_sensors(); return 0; }

    if (strcmp(argv[1], "paper") == 0) {
        paper_t paper;
        memset(&paper, 0, sizeof(paper));
        snprintf(paper.title, 255, "%s", argc > 2 ? argv[2] : "Unknown Paper");
        snprintf(paper.year, 15, "%d", 2024);
        ternary_context_t *tc = analyze_paper(&paper);
        tc_print(tc);
        tc_destroy(tc);
        return 0;
    }

    if (strcmp(argv[1], "analyze") == 0) {
        char text[1024] = "";
        for (int i = 2; i < argc; i++) {
            if (i > 2) strcat(text, " ");
            strcat(text, argv[i]);
        }
        classified_response_t r = classify_response(text);
        const char *sym;
        switch (r.classification) {
            case TRIT_NEG: sym = "⊖ DESCARTADO"; break;
            case TRIT_ZERO: sym = "0 INCIERTO"; break;
            case TRIT_POS: sym = "⊕ CONFIRMADO"; break;
            default: sym = "?";
        }
        printf("\n  Texto:   %s\n", r.input);
        printf("  Clase:   %s (%.0f%%)\n", sym, r.confidence * 100);
        printf("  Razón:   %s\n\n", r.reasoning);
        return 0;
    }

    if (strcmp(argv[1], "hypothesis") == 0) {
        char hyp_text[256] = "";
        for (int i = 2; i < argc; i++) {
            if (i > 2) strcat(hyp_text, " ");
            strcat(hyp_text, argv[i]);
        }
        hypothesis_t *hyp = hypothesis_create(hyp_text);
        hypothesis_add_evidence(hyp, "Available evidence supports", TRIT_POS, 0.70f);
        hypothesis_add_evidence(hyp, "Some contradicting data exists", TRIT_NEG, 0.50f);
        hypothesis_add_evidence(hyp, "More research needed", TRIT_ZERO, 0.60f);
        ternary_context_t *tc = hypothesis_evaluate(hyp);
        tc_print(tc);
        tc_destroy(tc);
        hypothesis_destroy(hyp);
        return 0;
    }

    if (strcmp(argv[1], "chat") == 0) {
        char msg[1024] = "";
        for (int i = 2; i < argc; i++) {
            if (i > 2) strcat(msg, " ");
            strcat(msg, argv[i]);
        }
        chat_exchange_t ex = ternary_chat(msg);
        printf("\n  🧠 TRITOS AI: %s\n\n", ex.response);
        return 0;
    }

    printf("Comando desconocido: %s\n", argv[1]);
    print_help();
    return 1;
}
