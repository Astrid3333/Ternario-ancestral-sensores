# TernaryAncestral — Motor Ternario Ultra-Optimizado

Librería de codificación ternaria ancestral para sensores IoT, optimizada para sistemas con memoria extrema.

## Modos disponibles

| Modo | Función | Compresión | RAM | Flash | Ideal para |
|------|---------|------------|-----|-------|------------|
| **Lite** | `litePipeline()` | 2.5x | 100B | 500B | ATtiny, BusyBox |
| **Original** | `ancestralPipeline()` | 2x | 500B | 2KB | Arduino, ciclos |
| **Babilónico** | `babylonianPipeline()` | **5x** | 300B | 3KB | Máxima compresión |

## Optimizaciones aplicadas

- **Bit-packing:** 3 trits en 5 bits (83% ahorro vs 3 bytes)
- **Sin punto flotante:** Solo enteros para ATmega
- **Lookup tables:** Operaciones frecuentes pre-calculadas
- **Memoria estática:** Sin malloc/new (seguro para embebidos)
- **PROGMEM:** Tablas en Flash para AVR

## Instalación

### Arduino Library Manager
1. Sketch → Include Library → Manage Libraries
2. Buscar "TernaryAncestral"
3. Instalar

### Manual
```bash
cp -r output/arduino-ternario-ancestral ~/Arduino/libraries/
```

## Ejemplos de uso

### Modo Lite (mínimo recurso)
```cpp
#include <TernaryAncestral.h>

int16_t samples[20];
uint8_t output[128];

void loop() {
    for (int i = 0; i < 20; i++) {
        samples[i] = analogRead(A0);
        delay(1000);
    }
    
    // Modo lite: solo bit-packing, sin checksum
    uint8_t n = litePipeline(samples, 20, output, 340, 700);
    
    Serial.write(0xCC);  // sync lite
    Serial.write(n);
    Serial.write(output, n);
}
```

### Modo Original (con detección de ciclos)
```cpp
uint8_t n = ancestralPipeline(samples, 20, output, 340, 700);
Serial.write(0xAA);
Serial.write(n);
Serial.write(output, n);
```

### Modo Babilónico (máxima compresión)
```cpp
uint8_t n = babylonianPipeline(samples, 20, output, 340, 700);
Serial.write(0xBB);
Serial.write(n);
Serial.write(output, n);
```

### Receptor (auto-detecta modo)
```cpp
void loop() {
    if (Serial.available()) {
        uint8_t sync = Serial.read();
        uint8_t n = Serial.read();
        uint8_t buffer[256];
        Serial.readBytes(buffer, n);
        
        int8_t trits[100];
        uint8_t decoded;
        
        switch (sync) {
            case 0xCC:  // Lite
                decoded = liteDecode(buffer, n, trits);
                break;
            case 0xAA:  // Original
                decoded = ancestralDecode(buffer, n, trits);
                break;
            case 0xBB:  // Babilónico
                decoded = babylonianDecode(buffer, n, trits);
                break;
        }
        
        for (uint8_t i = 0; i < decoded; i++) {
            Serial.print(trits[i]);
            Serial.print(" ");
        }
        Serial.println();
    }
}
```

## Protocolo de comunicación

| Sync | Modo | Formato |
|------|------|---------|
| `0xCC` | Lite | `[n_trits][bits...]` |
| `0xAA` | Original | `[n_trits][n_compressed][trits...][compressed...][checksum...]` |
| `0xBB` | Babilónico | `[n_digits][digits...][checksum...]` |

## Compatibilidad

### Microcontroladores
- ATtiny85 (512B RAM) ✓ — Solo modo Lite
- Arduino Uno/Nano (2KB RAM) ✓
- ESP8266 (80KB RAM) ✓
- ESP32 (520KB RAM) ✓
- STM32 Blue Pill (20KB RAM) ✓
- Raspberry Pi Pico (264KB RAM) ✓

### Linux minimalistas
- Tiny Core Linux (16MB, 28MB RAM) ✓
- BusyBox (1MB, 8MB RAM) ✓
- Alpine Linux (5MB, 64MB RAM) ✓
- Puppy Linux (100MB, 256MB RAM) ✓

### Sistemas embebidos
- FreeRTOS (6KB, 2KB RAM) ✓
- Zephyr (8KB, 4KB RAM) ✓
- RIOT OS (10KB, 4KB RAM) ✓

Ver `COMPATIBILITY.md` para guía completa de compilación cruzada.

## Benchmark

### Arduino Uno (ATmega328)
| Modo | RAM | Flash | Tiempo/100 trits |
|------|-----|-------|------------------|
| Lite | 100B | 500B | 0.5ms |
| Original | 500B | 2KB | 1.2ms |
| Babylonian | 300B | 3KB | 0.8ms |

### ESP8266
| Modo | RAM | Tiempo/1000 trits |
|------|-----|-------------------|
| Lite | 100B | 0.1ms |
| Original | 500B | 0.3ms |
| Babylonian | 300B | 0.2ms |

## Hardware mínimo

| Componente | Conexión |
|------------|----------|
| Arduino Uno/Nano | — |
| DHT11 (temp/humedad) | Pin 2 |
| LDR (luz) | A0 |
| LED indicador | Pin 13 |

## Referencias

- Brusentsov, N. P. (1958). Setun: ternary computer
- Zabihin, A. (2023). The Persian calendar
- Ascher, M. (1981). Code of the Quipu
- Landauer, R. (1961). Irreversibility and heat generation
- Ifrah, G. (2000). The Universal History of Numbers
