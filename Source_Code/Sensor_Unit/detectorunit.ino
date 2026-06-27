#include <WiFi.h>
#include <esp_now.h>
#include <math.h>

/* ====== GY-61 PINS ====== */
#define X_PIN 34
#define Y_PIN 35
#define Z_PIN 32
#define LED_PIN 2

/* ====== FIXED SETTINGS ====== */
#define SENSITIVITY           0.3
#define IMPACT_DELTA          1.5
#define AFTER_IMPACT_WAIT     2000
#define DECISION_TIME         5000
#define RESET_WAIT_TIME       2000
#define PEAK_WINDOW           30
#define STILL_CONFIRM_TIME    1500

/* ====== BIKE MAC ====== */
uint8_t bikeMAC[] = {0xD4,0xD4,0xDA,0xE4,0x5D,0xB0};

/* ====== ESP-NOW DATA ====== */
typedef struct {
  bool accidentConfirmed;
  float impactG;
} SensorData;

SensorData sensorData;

/* ====== STATE MACHINE ====== */
enum State { WAIT_IMPACT, WAIT_AFTER, WAIT_DECISION, WAIT_RESET };
State state = WAIT_IMPACT;
unsigned long stateTime = 0;

/* ====== VARIABLES ====== */
float ZERO_G = 1.65;
float STABLE_EPSILON = 0.05;   // will be auto-calibrated
float prevG = 1.0;
float peakDelta = 0;
unsigned long peakStart = 0;

float lastAx = 0, lastAy = 0, lastAz = 0;
unsigned long lastMoveTime = 0;

/* ====== ESP-NOW CALLBACK ====== */
void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print("📡 Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "✅ DELIVERED" : "❌ FAILED");
}

/* ====== AUTO CALIBRATION ====== */
void calibrateSensor() {
  Serial.println("🛠 CALIBRATION STARTED");
  Serial.println("⚠️ Keep helmet STILL");

  float sum = 0;
  float noiseMax = 0;

  float lastAx = 0, lastAy = 0, lastAz = 0;

  for (int i = 0; i < 200; i++) {
    float x = analogRead(X_PIN) * 3.3 / 4095.0;
    float y = analogRead(Y_PIN) * 3.3 / 4095.0;
    float z = analogRead(Z_PIN) * 3.3 / 4095.0;

    sum += x + y + z;

    float ax = (x - 1.65) / SENSITIVITY;
    float ay = (y - 1.65) / SENSITIVITY;
    float az = (z - 1.65) / SENSITIVITY;

    if (i > 0) {
      float motion = sqrt(
        pow(ax - lastAx, 2) +
        pow(ay - lastAy, 2) +
        pow(az - lastAz, 2)
      );
      if (motion > noiseMax) noiseMax = motion;
    }

    lastAx = ax;
    lastAy = ay;
    lastAz = az;

    delay(20);
  }

  ZERO_G = sum / 600.0;
  STABLE_EPSILON = noiseMax * 3.0;

  if (STABLE_EPSILON < 0.03) STABLE_EPSILON = 0.03;
  if (STABLE_EPSILON > 0.15) STABLE_EPSILON = 0.15;

  Serial.print("✅ ZERO_G: ");
  Serial.println(ZERO_G, 3);
  Serial.print("✅ MOTION THRESHOLD: ");
  Serial.println(STABLE_EPSILON, 3);

  Serial.println("🟢 CALIBRATION DONE");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  WiFi.mode(WIFI_STA);
  Serial.print("📶 Sensor MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ ESP-NOW INIT FAILED");
    return;
  }
  Serial.println("✅ ESP-NOW INIT OK");

  esp_now_register_send_cb(onDataSent);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, bikeMAC, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  delay(2000);
  calibrateSensor();   // 🔥 AUTO CALIBRATION
}

void loop() {

  float x = analogRead(X_PIN) * 3.3 / 4095.0;
  float y = analogRead(Y_PIN) * 3.3 / 4095.0;
  float z = analogRead(Z_PIN) * 3.3 / 4095.0;

  float ax = (x - ZERO_G) / SENSITIVITY;
  float ay = (y - ZERO_G) / SENSITIVITY;
  float az = (z - ZERO_G) / SENSITIVITY;

  float gNow = sqrt(ax * ax + ay * ay + az * az);
  float deltaG = abs(gNow - prevG);
  prevG = gNow;

  if (millis() - peakStart > PEAK_WINDOW) {
    peakDelta = deltaG;
    peakStart = millis();
  } else if (deltaG > peakDelta) {
    peakDelta = deltaG;
  }

  if (state == WAIT_IMPACT) {
    if (peakDelta >= IMPACT_DELTA) {
      Serial.println("⚠️ IMPACT DETECTED");
      state = WAIT_AFTER;
      stateTime = millis();
    }
  }

  else if (state == WAIT_AFTER) {
    if (millis() - stateTime >= AFTER_IMPACT_WAIT) {
      Serial.println("⏳ DECISION WINDOW");
      state = WAIT_DECISION;
      stateTime = millis();
      lastAx = ax; lastAy = ay; lastAz = az;
      lastMoveTime = millis();
    }
  }

  else if (state == WAIT_DECISION) {

    float motion = sqrt(
      pow(ax - lastAx, 2) +
      pow(ay - lastAy, 2) +
      pow(az - lastAz, 2)
    );

    if (motion > STABLE_EPSILON && motion < 2.5) {
      lastMoveTime = millis();
    }

    lastAx = ax; lastAy = ay; lastAz = az;

    if (millis() - stateTime >= DECISION_TIME) {
      if (millis() - lastMoveTime > STILL_CONFIRM_TIME) {
        Serial.println("🚨 ACCIDENT CONFIRMED");
        digitalWrite(LED_PIN, HIGH);
        sensorData.accidentConfirmed = true;
        sensorData.impactG = peakDelta;
        esp_now_send(bikeMAC, (uint8_t*)&sensorData, sizeof(sensorData));
      } else {
        Serial.println("❎ FALSE IMPACT");
      }
      state = WAIT_RESET;
      stateTime = millis();
    }
  }

  else if (state == WAIT_RESET) {
    if (millis() - stateTime >= RESET_WAIT_TIME) {
      digitalWrite(LED_PIN, LOW);
      peakDelta = 0;
      state = WAIT_IMPACT;
      Serial.println("🔄 READY");
    }
  }

  delay(10);
}