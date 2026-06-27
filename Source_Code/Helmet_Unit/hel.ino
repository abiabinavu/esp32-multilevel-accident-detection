#include <WiFi.h>
#include <esp_now.h>

#define HELMET_BTN   27
#define PULSE_PIN    32
#define ALCOHOL_PIN  34

uint8_t bikeAddress[] = {0xD4,0xD4,0xDA,0xE4,0x5D,0xB0};

typedef struct __attribute__((packed)) {
  bool helmetWorn;
  bool alcoholDetected;
  uint16_t pulseBPM;
} HelmetData;

HelmetData data;
unsigned long lastSend = 0;

void setup() {
  Serial.begin(115200);
  pinMode(HELMET_BTN, INPUT_PULLUP);
  WiFi.mode(WIFI_STA);
  esp_now_init();

  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, bikeAddress, 6);
  esp_now_add_peer(&peer);

  Serial.println("🪖 HELMET ESP32 READY");
}

void loop() {

  data.helmetWorn = digitalRead(HELMET_BTN) == LOW;

  int pulseRaw = analogRead(PULSE_PIN);
  data.pulseBPM = pulseRaw > 100 ?
                  constrain(map(pulseRaw,0,4095,60,120),55,120) : 0;

  data.alcoholDetected = analogRead(ALCOHOL_PIN) > 1800;

  if (millis() - lastSend > 1000) {
    lastSend = millis();
    esp_now_send(bikeAddress,(uint8_t*)&data,sizeof(data));
  }
}