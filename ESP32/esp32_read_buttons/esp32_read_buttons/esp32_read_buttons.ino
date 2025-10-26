#include <Arduino.h>

// ==== Pines del conector Link (GB lado master) ====
// Cable link Game Boy → ESP32
// Pin 1: VCC  (3.3V solo para referencia, NO alimentar GB)
// Pin 2: SOUT (Datos GB → ESP32)
// Pin 3: SIN  (Datos ESP32 → GB) [no lo usamos ahora]
// Pin 4: SCK  (Reloj GB → ESP32)
// Pin 5: GND

// ==== Pines asignados al ESP32 ====
const int PIN_GB_SOUT = 27; // Datos desde Game Boy
const int PIN_GB_SCK  = 14; // Reloj desde Game Boy

volatile uint8_t receivedByte = 0;
volatile bool byteReady = false;

// ==== Interrupción para leer 8 bits sincronizados por reloj ====
void IRAM_ATTR onClockPulse() {
  static uint8_t bitCount = 0;
  static uint8_t tempByte = 0;

  int bit = digitalRead(PIN_GB_SOUT);
  tempByte = (tempByte << 1) | (bit & 0x01);
  bitCount++;

  if (bitCount == 8) {
    receivedByte = tempByte;
    bitCount = 0;
    tempByte = 0;
    byteReady = true;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("=== ESP32 Game Boy Server Test ===");

  pinMode(PIN_GB_SOUT, INPUT);
  pinMode(PIN_GB_SCK, INPUT);

  attachInterrupt(digitalPinToInterrupt(PIN_GB_SCK), onClockPulse, RISING);
}

void loop() {
  if (byteReady) {
    byteReady = false;
    uint8_t val = receivedByte;
    Serial.printf("Recibido: 0x%02X -> ", val);

    switch (val) {
      case 0xA1: Serial.println("BOTON A"); break;
      case 0xB2: Serial.println("BOTON B"); break;
      case 0xA3: Serial.println("ARRIBA"); break;
      case 0xA4: Serial.println("ABAJO"); break;
      case 0xA5: Serial.println("IZQUIERDA"); break;
      case 0xA6: Serial.println("DERECHA"); break;
      case 0xA7: Serial.println("SELECT"); break;
      case 0xA8: Serial.println("START"); break;
      case 0xD0: Serial.println("-- DISCOVERY --"); break;
      default:
        Serial.println("(otro valor)");
        break;
    }
  }
}