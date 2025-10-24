#include <WiFi.h>
#include <WiFiUdp.h>

const bool isDemo = false;

const char* ssid = "GB_SERVER";
const char* password = "12345678";
const int LOCAL_PORT = 7777;
const char* CLIENT_IP = "192.168.4.2";
WiFiUDP udp;

const int PIN_SO = 27;
const int PIN_SI = 26;
const int PIN_SC = 14;

volatile bool clockTick = false;
volatile int bitBuffer = 0;
volatile int bitCount = 0;

void IRAM_ATTR onClock() {
  clockTick = true;
}

String toBinary(byte v) {
  String s = "";
  for (int i = 7; i >= 0; i--) s += (v & (1 << i)) ? "1" : "0";
  return s;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n🎮 ESP32 GB LINK BRIDGE - SERVER (sync test)");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.printf("✅ AP: %s  IP: %s\n", ssid, myIP.toString().c_str());

  udp.begin(LOCAL_PORT);
  pinMode(PIN_SO, INPUT);
  pinMode(PIN_SI, OUTPUT);
  pinMode(PIN_SC, INPUT);

  // ⚠️ cambio: FALLING en vez de RISING
  attachInterrupt(digitalPinToInterrupt(PIN_SC), onClock, FALLING);
}

void loop() {
  if (isDemo) return;

  if (clockTick) {
    clockTick = false;

    int bit = digitalRead(PIN_SO);
    bitBuffer |= (bit & 0x01) << bitCount;  // ⚠️ cambio: LSB-first
    bitCount++;

    if (bitCount == 8) {
      Serial.printf("📤 [GB→ESP] byte=0x%02X bin=%s\n", bitBuffer, toBinary(bitBuffer).c_str());

      udp.beginPacket(CLIENT_IP, LOCAL_PORT);
      udp.write(bitBuffer);
      udp.endPacket();

      bitBuffer = 0;
      bitCount = 0;
    }
  }

  int packetSize = udp.parsePacket();
  if (packetSize) {
    byte incoming = udp.read();
    Serial.printf("⬅️ [ESP→GB] byte=0x%02X bin=%s\n", incoming, toBinary(incoming).c_str());

    for (int i = 0; i < 8; i++) {
      digitalWrite(PIN_SI, (incoming >> i) & 0x01);
      delayMicroseconds(50);
    }
  }
}
