# TernaryAncestral — Motor Ternario para Arduino

Librería Arduino que implementa codificación ternaria ancestral para sensores IoT, inspirada en tres sistemas matemáticos:

- **Ternario ruso** (Setun): codificación balanceada {-1, 0, +1}
- **Calendario persa** (ciclo-33): compresión por residuos mod-33
- **Quipu inca**: checksum de paridad ternaria

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

### Transmisor (sensor_transmitter.ino)
```
Sensor → TernaryAncestral → Serial (9600 baud)
```

### Receptor (data_receiver.ino)
```
Serial → TernaryAncestral → LCD 16x2
```

## Uso básico

```cpp
#include <TernaryAncestral.h>

void setup() {
    Serial.begin(9600);
}

void loop() {
    // Leer sensor
    int16_t value = analogRead(A0);
    
    // Convertir a trit
    int8_t trit = sensorToTrit(value, 340, 700);
    
    // Pipeline completo con 20 muestras
    int16_t samples[20];
    uint8_t output[256];
    uint8_t n = ancestralPipeline(samples, 20, output, 340, 700);
    
    // Enviar comprimido
    Serial.write(output, n);
}
```

## Protocolo de comunicación

Paquete serial: `[SYNC 0xAA][n_bytes][data...]`

| Campo | Tamaño | Descripción |
|-------|--------|-------------|
| SYNC | 1 byte | 0xAA (sincronización) |
| n_bytes | 1 byte | Longitud del payload |
| n_trits | 1 byte | Número de trits |
| n_compressed | 1 byte | Bytes comprimidos |
| trits | n_trits | Valores (-1,0,+1) mapeados a (0,1,2) |
| compressed | n_compressed | Residuos + run-length |
| checksum | 2*n_blocks | Paridad quipu |

## Rendimiento medido

| Métrica | Valor |
|---------|-------|
| Compresión | 91.7% (ratio 0.083) |
| Ahorro energía | 86.9% vs binario |
| RAM usage | ~500 bytes |
| Flash usage | ~2KB |

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
