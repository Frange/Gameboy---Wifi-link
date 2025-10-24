#include <WiFi.h>
#include <WiFiUdp.h>

const bool isDemo = false;  // true = prueba UDP, false = modo Game Boy real

// ==========================
// CONFIG WIFI - CLIENT
// ==========================
const char* ssid = "GB_SERVER";
const char* password = "12345678";
const int LOCAL_PORT = 7777;
const char* SERVER_IP = "192.168.4.1";
WiFiUDP udp;

// ==========================
// CONFIG LINK CABLE
// ==========================
const int PIN_SO = 27;
const int PIN_SI = 26;
const int PIN_SC = 14;

volatile bool clockTick = false;
volatile int bitBuffer = 0;
volatile int bitCount = 0;

void IRAM_ATTR onClock() {
  clockTick = true;
}

// ==========================
// SETUP
// ==========================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n🎮 ESP32 GB LINK BRIDGE - CLIENT");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.printf("🔗 Conectando a %s", ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.printf("\n✅ Conectado, IP: %s\n", WiFi.localIP().toString().c_str());

  udp.begin(LOCAL_PORT);
  Serial.printf("📡 UDP escuchando en puerto %d\n", LOCAL_PORT);

  pinMode(PIN_SO, INPUT);
  pinMode(PIN_SI, OUTPUT);
  pinMode(PIN_SC, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_SC), onClock, RISING);
}

// ==========================
// LOOP
// ==========================
void loop() {
  if (isDemo) {
    static unsigned long lastDiscovery = 0;
    if (millis() - lastDiscovery > 3000) {
      lastDiscovery = millis();
      udp.beginPacket(SERVER_IP, LOCAL_PORT);
      udp.write(0xAA);
      udp.endPacket();
      Serial.println("📤 [TX] 0xAA (DISCOVERY)");
    }

    int packetSize = udp.parsePacket();
    if (packetSize) {
      byte incoming = udp.read();
      Serial.printf("⬅️ [RX] 0x%02X\n", incoming);
      if (incoming == 0xCC) {
        udp.beginPacket(SERVER_IP, LOCAL_PORT);
        udp.write(0xDD);
        udp.endPacket();
        Serial.println("➡️ [TX] 0xDD (PONG)");
      }
    }
    delay(10);
    return;
  }

  // ----- MODO REAL -----
  if (clockTick) {
    clockTick = false;

    int bit = digitalRead(PIN_SO);
    bitBuffer = (bitBuffer << 1) | (bit & 0x01);
    bitCount++;

    if (bitCount == 8) {
      udp.beginPacket(SERVER_IP, LOCAL_PORT);
      udp.write(bitBuffer);
      udp.endPacket();

      Serial.printf("📤 [GB→ESP] byte=0x%02X\n", bitBuffer);

      bitBuffer = 0;
      bitCount = 0;
    }
  }

  int packetSize = udp.parsePacket();
  if (packetSize) {
    byte incoming = udp.read();
    Serial.printf("⬅️ [ESP→GB] byte=0x%02X\n", incoming);

    for (int i = 7; i >= 0; i--) {
      digitalWrite(PIN_SI, (incoming >> i) & 0x01);
      delayMicroseconds(50);
    }
  }
}
