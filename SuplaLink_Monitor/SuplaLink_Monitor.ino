#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <time.h>
#include "config.h"

enum SensorType {
  TEMPERATURE = 1,
  AIR = 2
};

struct Sensor {
  const char* url;
  const char* code;
  const char* location;
  SensorType type;
};

// ST7789 display configuration
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

// SPI pins for ST7789 (adjust these for your specific ESP32-S3 board)
#ifndef TFT_CS
#define TFT_CS 5
#endif
#ifndef TFT_RST
#define TFT_RST 16
#endif
#ifndef TFT_DC
#define TFT_DC 17
#endif

Adafruit_ST7789 display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
bool hasDisplay = true;

static const char* sensorTypeToString(SensorType type) {
  switch (type) {
    case TEMPERATURE: return "TEMPERATURE";
    case AIR: return "AIR";
    default: return "UNKNOWN";
  }
}

#if defined(__has_include)
#if __has_include("secrets.h")
#include "secrets.h"
#endif
#endif

// Allow secrets.h to provide sensors via SENSORS_LIST macro
#ifdef SENSORS_LIST
#define SENSORS_DEFINED 1
const Sensor sensors[] = SENSORS_LIST;
const int sensorCount = sizeof(sensors) / sizeof(sensors[0]);
#else
// Default empty sensors list if none provided
const Sensor sensors[] = {};
const int sensorCount = 0;
#endif

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println();
  Serial.print(F("=== "));
  Serial.print(APP_NAME);
  Serial.print(F(" v"));
  Serial.print(APP_VERSION);
  Serial.print(F(" ("));
  Serial.print(APP_BUILD_DATE);
  Serial.print(F(" "));
  Serial.print(APP_BUILD_TIME);
  Serial.println(F(") ==="));
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print(F("Connecting to WiFi SSID: "));
  Serial.print(WIFI_SSID);
  const unsigned long connectTimeoutMs = 15000;
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < connectTimeoutMs) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("Connected to WiFi!"));
  } else {
    Serial.println(F("WiFi connect timeout"));
  }

  // Set timezone using POSIX TZ string (configurable in TZ_STRING)
  // Example CET with DST (last Sun of Mar/Oct): CET-1CEST,M3.5.0,M10.5.0/3
  configTzTime(TZ_STRING, "pool.ntp.org", "time.nist.gov");

  // Initialize ST7789 display
  if (hasDisplay) {
    Serial.println(F("Initializing ST7789 display..."));
    display.init(SCREEN_WIDTH, SCREEN_HEIGHT);
    display.setRotation(2);  // Adjust rotation: 0, 1, 2, or 3
    display.fillScreen(ST77XX_BLACK);
    display.setTextSize(2);
    display.setTextColor(ST77XX_WHITE);
    display.setCursor(10, 20);
    display.println("SuplaLink");
    display.setCursor(10, 50);
    display.println("Monitor");
    display.setCursor(10, 100);
    display.setTextSize(1);
    display.println("WiFi connected!");
    display.setCursor(10, 120);
    display.print("SSID: ");
    display.println(WIFI_SSID);
    Serial.println(F("ST7789 display initialized"));
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    struct tm timeInfo;
    if (getLocalTime(&timeInfo)) {
      char timeStr[16];
      strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeInfo);
      Serial.println(timeStr);
    }
    for (int i = 0; i < sensorCount; i++) {
      readSensor(sensors[i]);
      if (i < sensorCount - 1) {  // Don't delay after last sensor
        delay(SENSOR_DISPLAY_DELAY_MS);
      }
    }
  } else {
    Serial.println("WiFi disconnected!");
  }

  delay(POLL_INTERVAL_MS);
}

void readSensor(const Sensor& sensor) {
  HTTPClient http;
  http.begin(sensor.url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");

  StaticJsonDocument<128> bodyDoc;
  bodyDoc["code"] = sensor.code;
  bodyDoc["action"] = "read";
  bodyDoc["type"] = sensorTypeToString(sensor.type);
  char body[128];
  size_t bodyLen = serializeJson(bodyDoc, body, sizeof(body));

  int httpCode = http.sendRequest("PATCH", (uint8_t*)body, bodyLen);

  if (httpCode > 0) {
    // Serial.printf("\n[%s] -> HTTP %d\n", sensor.location, httpCode);
    // Serial.println(payload);

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, http.getStream());

    if (!error) {
      float temp = doc["temperature"];
      float hum = doc["humidity"];

      Serial.printf("[%s] Temp: %.2f°C, Hum: %.2f%%\n", sensor.location, temp, hum);

      if (hasDisplay) {
        display.fillScreen(ST77XX_BLACK);

        // Display location
        display.setTextSize(2);
        display.setTextColor(ST77XX_CYAN);
        display.setCursor(10, 20);
        display.println(sensor.location);

        // Display temperature
        display.setTextSize(3);
        display.setTextColor(ST77XX_ORANGE);
        display.setCursor(10, 70);
        display.print(temp, 1);
        display.println(" C");

        // Display humidity
        display.setTextSize(3);
        display.setTextColor(ST77XX_GREEN);
        display.setCursor(10, 130);
        display.print(hum, 1);
        display.println(" %");

        // Display time
        struct tm timeInfo;
        if (getLocalTime(&timeInfo)) {
          char timeStr[16];
          strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeInfo);
          display.setTextSize(1);
          display.setTextColor(ST77XX_WHITE);
          display.setCursor(10, 210);
          display.println(timeStr);
        }
      }
    } else {
      Serial.printf("[%s] JSON parse error!\n", sensor.location);
    }
  } else {
    Serial.printf("[%s] HTTP error %d\n", sensor.location, httpCode);
  }

  http.end();
}