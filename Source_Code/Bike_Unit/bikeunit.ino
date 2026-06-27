#include <WiFi.h>
#include <esp_now.h>
#include "esp_wifi.h"
#include <TinyGPS++.h>

/* ========= ESPNOW ========= */
#define ESPNOW_CHANNEL 1

/* ========= GSM + GPS ========= */
#define GSM_RX 26
#define GSM_TX 27
#define GPS_RX 16
#define GPS_TX 17
#define PHONE_NUMBER "+91----------"

/* ========= USER INPUT ========= */
#define BUZZER_PIN 25
#define CANCEL_BTN 33

/* ========= TIMING ========= */
#define CONFIRM_TIME 10000     // 10 sec cancel window
#define CALL_TIME    20000
#define COOLDOWN     30000

/* ========= OBJECTS ========= */
HardwareSerial sim(1);
HardwareSerial gpsSerial(2);
TinyGPSPlus gps;

/* ========= HELMET DATA ========= */
typedef struct {
  bool helmetWorn;
  bool alcoholDetected;
  uint16_t pulseBPM;
} HelmetData;

/* ========= DETECTOR DATA ========= */
typedef struct {
  bool accidentConfirmed;
  float impactG;
} SensorData;

HelmetData helmet;
SensorData detector;

/* ========= STATE ========= */
bool helmetConnected = false;
bool detectorConnected = false;
bool accidentActive = false;
bool alertSent = false;

unsigned long lastHelmetTime = 0;
unsigned long lastDetectorTime = 0;
unsigned long accidentTime = 0;
unsigned long cooldownStart = 0;

/* ========= GPS ========= */
float lastLat = 0, lastLon = 0;

/* ========= ESPNOW RECEIVE ========= */
void onDataRecv(const esp_now_recv_info_t *,
                const uint8_t *data,
                int len) {

  if (len == sizeof(HelmetData)) {
    memcpy(&helmet, data, sizeof(helmet));
    helmetConnected = true;
    lastHelmetTime = millis();
  }

  else if (len == sizeof(SensorData)) {
    memcpy(&detector, data, sizeof(detector));
    detectorConnected = true;
    lastDetectorTime = millis();

    if (detector.accidentConfirmed && !accidentActive &&
        millis() - cooldownStart > COOLDOWN) {

      accidentActive = true;
      alertSent = false;
      accidentTime = millis();
      Serial.println("🚨 ACCIDENT DETECTED");
    }
  }
}

/* ========= BUZZER ========= */
void buzzerBeep(int d) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(d);
  digitalWrite(BUZZER_PIN, LOW);
}

/* ========= SEND ALERT ========= */
void sendAlert() {

  Serial.println("📨 SENDING SMS");

  sim.println("AT+CMGF=1");
  delay(500);

  sim.print("AT+CMGS=\"");
  sim.print(PHONE_NUMBER);
  sim.println("\"");
  delay(500);

  sim.println("🚨 BIKE ACCIDENT ALERT 🚨");

  sim.print("Helmet: ");
  sim.println(helmetConnected ? "CONNECTED" : "DISCONNECTED");

  if (helmetConnected) {
    sim.print("Helmet Worn: ");
    sim.println(helmet.helmetWorn ? "YES" : "NO");
    sim.print("Pulse: ");
    sim.println(helmet.pulseBPM);
    sim.print("Alcohol: ");
    sim.println(helmet.alcoholDetected ? "YES" : "NO");
  }

  sim.print("Impact G: ");
  sim.println(detector.impactG, 2);

  if (lastLat != 0 && lastLon != 0) {
    sim.print("Location:\nhttps://maps.google.com/?q=");
    sim.print(lastLat, 6);
    sim.print(",");
    sim.print(lastLon, 6);
  } else {
    sim.println("GPS NOT AVAILABLE");
  }

  sim.write(26); // CTRL+Z
  delay(4000);

  Serial.println("📞 CALLING EMERGENCY");

  sim.print("ATD");
  sim.print(PHONE_NUMBER);
  sim.println(";");
  delay(CALL_TIME);
  sim.println("ATH");

  Serial.println("✅ ALERT SENT");
}

/* ========= SETUP ========= */
void setup() {
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(CANCEL_BTN, INPUT_PULLUP);

  sim.begin(9600, SERIAL_8N1, GSM_RX, GSM_TX);
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

  esp_now_init();
  esp_now_register_recv_cb(onDataRecv);

  Serial.print("🚲 BIKE MAC: ");
  Serial.println(WiFi.macAddress());

  Serial.println("✅ BIKE SYSTEM READY");
}

/* ========= LOOP ========= */
void loop() {

  /* ---- GPS UPDATE ---- */
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
    if (gps.location.isValid()) {
      lastLat = gps.location.lat();
      lastLon = gps.location.lng();
    }
  }

  /* ---- CONNECTION TIMEOUT ---- */
  if (helmetConnected && millis() - lastHelmetTime > 3000)
    helmetConnected = false;

  if (detectorConnected && millis() - lastDetectorTime > 3000)
    detectorConnected = false;

  /* ---- CANCEL BUTTON ---- */
  if (accidentActive && digitalRead(CANCEL_BTN) == LOW) {
    Serial.println("❌ ALERT CANCELED");
    accidentActive = false;
    alertSent = false;
    cooldownStart = millis();
    return;
  }

  /* ---- CONFIRM WINDOW ---- */
  if (accidentActive &&
      millis() - accidentTime < CONFIRM_TIME) {
    buzzerBeep(100);
    delay(400);
  }

  /* ---- SEND ALERT ---- */
  if (accidentActive &&
      millis() - accidentTime >= CONFIRM_TIME &&
      !alertSent) {

    sendAlert();
    alertSent = true;
    accidentActive = false;
    cooldownStart = millis();
  }

  /* ---- SERIAL DISPLAY ---- */
  Serial.println("====== BIKE STATUS ======");
  Serial.print("Helmet: ");
  Serial.println(helmetConnected ? "CONNECTED" : "DISCONNECTED");

  Serial.print("Detector: ");
  Serial.println(detectorConnected ? "CONNECTED" : "DISCONNECTED");

  Serial.print("Accident State: ");
  Serial.println(accidentActive ? "ACTIVE" : "IDLE");
  Serial.println("=========================");

  delay(500);
}