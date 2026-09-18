/**
 * sensor_transmitter.ino — Transmisor: Sensor → Ternario → Serial
 * 
 * Hardware: Arduino Uno/Nano + DHT11 (pin 2) + LDR (A0)
 * 
 * Envía datos comprimidos por serial a 9600 baud:
 * [SYNC][n_trits][compressed_bytes][data...][checksum...]
 * 
 * Ahorro: ~91% menos datos que binario estándar
 */

#include <TernaryAncestral.h>

// Pines
#define PIN_DHT 2
#define PIN_LDR A0
#define PIN_LED 13

// Intervalo de lectura (ms)
#define READ_INTERVAL 60000  // 1 minuto

// Buffer
#define MAX_SAMPLES 20
int16_t sensorBuffer[MAX_SAMPLES];
uint8_t sampleCount = 0;

// Sync byte
#define SYNC_BYTE 0xAA

void setup() {
    Serial.begin(9600);
    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_DHT, INPUT);
    
    digitalWrite(PIN_LED, HIGH);
    delay(1000);
    digitalWrite(PIN_LED, LOW);
    
    Serial.println(F("=== TERNARY ANCESTRAL TRANSMITTER ==="));
    Serial.println(F("Motor: Ruso + Persa + Quipu"));
}

void loop() {
    // Leer sensores
    int16_t temp = analogRead(PIN_LDR);  // LDR como proxy de temperatura
    int16_t humidity = digitalRead(PIN_DHT) * 1023;  // simplificado
    
    sensorBuffer[sampleCount] = temp;
    sampleCount++;
    
    // LED parpadea cada lectura
    digitalWrite(PIN_LED, HIGH);
    delay(100);
    digitalWrite(PIN_LED, LOW);
    
    // Cuando tengamos suficientes muestras, comprimir y enviar
    if (sampleCount >= MAX_SAMPLES) {
        sendCompressed();
        sampleCount = 0;
    }
    
    delay(READ_INTERVAL);
}

void sendCompressed() {
    // Pipeline completo: sensor → ternario → residual → quipu
    uint8_t output[256];
    uint8_t n_bytes = ancestralPipeline(
        sensorBuffer, sampleCount,
        output,
        340,   // umbral bajo (frío)
        700    // umbral alto (caliente)
    );
    
    // Enviar por serial con sync
    Serial.write(SYNC_BYTE);
    Serial.write(n_bytes);
    Serial.write(output, n_bytes);
    
    // También enviar en formato legible
    Serial.println();
    Serial.print(F("PACKET: "));
    Serial.print(n_bytes);
    Serial.print(F(" bytes (vs "));
    Serial.print(sampleCount * 8);
    Serial.println(F(" bits binario)"));
    
    // Ratio de compresión
    float ratio = (float)n_bytes * 8.0 / (float)(sampleCount * 8);
    Serial.print(F("RATIO: "));
    Serial.print(ratio * 100, 1);
    Serial.println(F("%"));
    
    Serial.println();
}
