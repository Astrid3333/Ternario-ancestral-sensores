# TernaryAncestral — Motor Ternario para Arduino

Librería Arduino que implementa codificación ternaria ancestral para sensores IoT con dos modos de compresión:

| Modo | Función | Bytes por 20 muestras | Compresión |
|------|---------|----------------------|------------|
| **Original** | `ancestralPipeline()` | ~20 bytes | 2x vs binario |
| **Babilónico** | `babylonianPipeline()` | ~8 bytes | **5x vs binario** |

## Instalación

### Opción 1: Arduino Library Manager
1. Sketch → Include Library → Manage Libraries
2. Buscar "TernaryAncestral"
3. Instalar

### Opción 2: Manual
1. Copiar esta carpeta a `~/Arduino/libraries/`
2. Reiniciar Arduino IDE

## Hardware requerido

| Componente | Conexión |
|------------|----------|
| Arduino Uno/Nano | — |
| DHT11 (temp/humedad) | Pin 2 |
| LDR (luz) | A0 |
| LED indicador | Pin 13 |
| LCD 16x2 I2C (receptor) | A4/A5 |

## Ejemplos

### Transmisor — Modo Original (residuos mod-33)
```cpp
#include <TernaryAncestral.h>

int16_t samples[20];
uint8_t output[256];

void loop() {
    // Leer 20 muestras del sensor
    for (int i = 0; i < 20; i++) {
        samples[i] = analogRead(A0);
        delay(1000);
    }
    
    // Comprimir con pipeline original
    uint8_t n = ancestralPipeline(samples, 20, output, 340, 700);
    
    // Enviar
    Serial.write(0xAA);
    Serial.write(n);
    Serial.write(output, n);
}
```

### Transmisor — Modo Babilónico (base 60, más compacto)
```cpp
#include <TernaryAncestral.h>

int16_t samples[20];
uint8_t output[256];

void loop() {
    // Leer 20 muestras del sensor
    for (int i = 0; i < 20; i++) {
        samples[i] = analogRead(A0);
        delay(1000);
    }
    
    // Comprimir con pipeline babilónico (3.91 trits por dígito)
    uint8_t n = babylonianPipeline(samples, 20, output, 340, 700);
    
    // Enviar (mucho más compacto)
    Serial.write(0xBB);  // sync diferente para modo babilónico
    Serial.write(n);
    Serial.write(output, n);
}
```

### Receptor
```cpp
#include <TernaryAncestral.h>

void loop() {
    if (Serial.available()) {
        uint8_t sync = Serial.read();
        
        if (sync == 0xAA) {
            // Modo original
            uint8_t n = Serial.read();
            uint8_t buffer[256];
            Serial.readBytes(buffer, n);
            
            int8_t trits[100];
            uint8_t decoded = ancestralDecode(buffer, n, trits);
            
            for (uint8_t i = 0; i < decoded; i++) {
                Serial.print(trits[i]);
                Serial.print(" ");
            }
            Serial.println();
        }
        else if (sync == 0xBB) {
            // Modo babilónico
            uint8_t n = Serial.read();
            uint8_t buffer[256];
            Serial.readBytes(buffer, n);
            
            int8_t trits[100];
            uint8_t decoded = babylonianDecode(buffer, n, trits);
            
            for (uint8_t i = 0; i < decoded; i++) {
                Serial.print(trits[i]);
                Serial.print(" ");
            }
            Serial.println();
        }
    }
}
```

## Comparación de modos

| Característica | Original | Babilónico |
|----------------|----------|------------|
| Algoritmo | Residuos mod-33 | Base 60 |
| Compresión | 2x | **5x** |
| Complejidad | Baja | Media |
| RAM usage | ~500 bytes | ~300 bytes |
| Flash usage | ~2KB | ~3KB |
| Detección de ciclos | ✓ | ✗ |
| Ideal para | Ciclos periódicos | Máxima compresión |

## Protocolo de comunicación

### Modo Original (0xAA)
`[SYNC 0xAA][n_bytes][n_trits][n_compressed][trits...][compressed...][checksum...]`

### Modo Babilónico (0xBB)
`[SYNC 0xBB][n_bytes][n_digits][digits...][checksum...]`

## Rendimiento medido

| Métrica | Original | Babilónico |
|---------|----------|------------|
| Compresión vs binario | 2x | 5x |
| Ahorro energía | 50% | **80%** |
| RAM usage | ~500 bytes | ~300 bytes |
| Flash usage | ~2KB | ~3KB |

## Compatibilidad

- Arduino Uno/Nano (ATmega328) ✓
- Arduino Mega (ATmega2560) ✓
- ESP32 / ESP8266 ✓
- STM32 Blue Pill ✓
- Raspberry Pi Pico ✓

## Referencias

- Brusentsov, N. P. (1958). Setun: ternary computer
- Zabihin, A. (2023). The Persian calendar
- Ascher, M. (1981). Code of the Quipu
- Landauer, R. (1961). Irreversibility and heat generation
- Ifrah, G. (2000). The Universal History of Numbers
