/**
 * data_receiver.ino — Receptor: Serial → Decode → Display
 * 
 * Hardware: Arduino Uno/Nano + LCD 16x2 (I2C) + LED (pin 13)
 * 
 * Recibe datos comprimidos del transmisor ternario,
 * los decodifica y muestra en LCD.
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <TernaryAncestral.h>

// LCD I2C (dirección 0x27 común)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pines
#define PIN_LED 13
#define SYNC_BYTE 0xAA

// Buffer接收
#define MAX_PACKET 256
uint8_t rxBuffer[MAX_PACKET];
uint8_t rxIndex = 0;
bool receiving = false;

// Estadísticas
uint32_t packetsReceived = 0;
uint32_t totalBytesSaved = 0;

void setup() {
    Serial.begin(9600);
    pinMode(PIN_LED, OUTPUT);
    
    // LCD init
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print(F("Ternary Ancestral"));
    lcd.setCursor(0, 1);
    lcd.print(F("Receiver Ready"));
    
    delay(2000);
    lcd.clear();
    
    Serial.println(F("=== TERNARY ANCESTRAL RECEIVER ==="));
    Serial.println(F("Esperando datos..."));
}

void loop() {
    if (Serial.available()) {
        uint8_t byte = Serial.read();
        
        if (byte == SYNC_BYTE && !receiving) {
            // Iniciar recepción
            receiving = true;
            rxIndex = 0;
            digitalWrite(PIN_LED, HIGH);
        } else if (receiving) {
            rxBuffer[rxIndex++] = byte;
            
            if (rxIndex >= 2) {
                uint8_t expected = rxBuffer[0] + 2;  // n_trits + n_compressed + data
                if (rxIndex >= expected) {
                    processPacket();
                    receiving = false;
                    digitalWrite(PIN_LED, LOW);
                }
            }
            
            if (rxIndex >= MAX_PACKET) {
                receiving = false;  // overflow
                digitalWrite(PIN_LED, LOW);
            }
        }
    }
}

void processPacket() {
    packetsReceived++;
    
    uint8_t n_trits = rxBuffer[0];
    uint8_t n_compressed = rxBuffer[1];
    
    // Decodificar trits
    int8_t trits[100];
    ancestralDecode(rxBuffer, rxIndex, trits);
    
    // Calcular ahorro
    uint8_t original_bits = n_trits * 8;  // 8 bits por valor crudo
    uint8_t compressed_bits = rxIndex * 8;
    uint32_t saved = original_bits - compressed_bits;
    totalBytesSaved += saved;
    
    // Calcular valores reconstruidos (aproximación)
    float avg_trit = 0;
    for (uint8_t i = 0; i < n_trits; i++) {
        avg_trit += trits[i];
    }
    avg_trit /= n_trits;
    
    // Clasificar estado promedio
    const char* state;
    if (avg_trit < -0.3) state = "FRI";
    else if (avg_trit > 0.3) state = "CAL";
    else state = "NOR";
    
    // Mostrar en LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Pkts: "));
    lcd.print(packetsReceived);
    lcd.print(F(" ["));
    lcd.print(state);
    lcd.print(F("]"));
    
    lcd.setCursor(0, 1);
    lcd.print(F("Save: "));
    lcd.print(totalBytesSaved);
    lcd.print(F(" bits"));
    
    // Mostrar por serial
    Serial.println(F("--- PAQUETE RECIBIDO ---"));
    Serial.print(F("Trits: "));
    Serial.println(n_trits);
    Serial.print(F("Bytes: "));
    Serial.println(rxIndex);
    Serial.print(F("Estado: "));
    Serial.println(state);
    Serial.print(F("Ahorro total: "));
    Serial.print(totalBytesSaved);
    Serial.println(F(" bits"));
    Serial.println();
}
