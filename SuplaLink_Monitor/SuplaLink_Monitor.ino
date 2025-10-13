#include <Arduino.h>
#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>

#if defined(__has_include)
  #if __has_include("secrets.h")
    #include "secrets.h"
  #endif
#endif

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

// NTP handled by ESP32 SNTP via configTzTime

bool hasDisplay = false;

enum SensorType {
  TEMPERATURE = 1,
  AIR = 2
};

static const char* sensorTypeToString(SensorType type) {
  switch (type) {
    case TEMPERATURE: return "TEMPERATURE";
    case AIR: return "AIR";
    default: return "UNKNOWN";
  }
}

struct Sensor {
  const char* url;
  const char* code;
  const char* location;
  SensorType type;
};

#ifndef SENSORS_DEFINED
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
#endif

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println();
  Serial.print(F("=== ")); Serial.print(APP_NAME); Serial.print(F(" v")); Serial.print(APP_VERSION);
  Serial.print(F(" (")); Serial.print(APP_BUILD_DATE); Serial.print(F(" ")); Serial.print(APP_BUILD_TIME); Serial.println(F(") ==="));
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

  if (hasDisplay) {
    Serial.println(F("OLED not found, running headless"));
  } else {
    // display.clearDisplay();
    // display.setTextSize(1);
    // display.setTextColor(SSD1306_WHITE);
    // display.setCursor(0, 0);
    // display.println("WiFi connected!");
    // display.display();
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
        // display.clearDisplay();
        // display.setCursor(0, 0);
        // display.printf("%s\n", sensor.location);
        // display.printf("Temp: %.2f C\n", temp);
        // display.printf("Hum:  %.2f %%", hum);
        // display.display();
      }
    } else {
      Serial.printf("[%s] JSON parse error!\n", sensor.location);
    }
  } else {
    Serial.printf("[%s] HTTP error %d\n", sensor.location, httpCode);
  }

  http.end();
}