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

// Timer for updating current time (bottom-left)
unsigned long lastTimeUpdate = 0;
const unsigned long TIME_UPDATE_INTERVAL = 60000; // 60 seconds

// Timer for updating work counter (bottom-right)
unsigned long lastWorkCounterUpdate = 0;
const unsigned long WORK_COUNTER_UPDATE_INTERVAL = 300000; // 5 minutes

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

// Function to update current time (bottom-left)
void updateCurrentTime() {
  if (!hasDisplay) return;

  struct tm timeInfo;
  if (getLocalTime(&timeInfo)) {
    // Clear only the time section (left side)
    display.fillRect(0, 288, 120, 30, ST77XX_BLACK);

    // Current time - bottom-left corner (HH:MM without seconds)
    char currentTimeStr[6];
    strftime(currentTimeStr, sizeof(currentTimeStr), "%H:%M", &timeInfo);
    display.setTextSize(3);
    display.setTextColor(ST77XX_YELLOW);
    display.setCursor(10, 290);
    display.println(currentTimeStr);
  }
}

// Function to update work counter (bottom-right)
void updateWorkCounter() {
  if (!hasDisplay) return;

  struct tm timeInfo;
  if (getLocalTime(&timeInfo)) {
    // Clear only the work counter section (right side)
    display.fillRect(120, 288, 120, 30, ST77XX_BLACK);

    // Work time counter - bottom-right corner (8h starting at 8:00)
    int currentHour = timeInfo.tm_hour;
    int currentMin = timeInfo.tm_min;

    // Calculate time since 8:00 (minutes only)
    int workStartHour = 8;
    int minutesSince8AM = (currentHour - workStartHour) * 60 + currentMin;

    // 8 hours = 480 minutes
    int workDayMinutes = 8 * 60;  // 480
    int remainingMinutes = workDayMinutes - minutesSince8AM;

    // Handle case when before 8:00 or after 16:00
    if (currentHour < workStartHour) {
      remainingMinutes = workDayMinutes;  // Full 8h remaining
    } else if (remainingMinutes < 0) {
      remainingMinutes = 0;  // Workday finished
    }

    // Convert to HH:MM
    int hoursLeft = remainingMinutes / 60;
    int minutesLeft = remainingMinutes % 60;

    char workTimeStr[6];
    snprintf(workTimeStr, sizeof(workTimeStr), "%02d:%02d", hoursLeft, minutesLeft);

    display.setTextSize(3);
    display.setTextColor(ST77XX_MAGENTA);
    display.setCursor(150, 290);  // Raised by 2 pixels
    display.println(workTimeStr);
  }
}

void loop() {
  unsigned long currentMillis = millis();

  // Check if it's time to update current time (every 60 seconds)
  if (currentMillis - lastTimeUpdate >= TIME_UPDATE_INTERVAL) {
    updateCurrentTime();
    lastTimeUpdate = currentMillis;
  }

  // Check if it's time to update work counter (every 5 minutes)
  if (currentMillis - lastWorkCounterUpdate >= WORK_COUNTER_UPDATE_INTERVAL) {
    updateWorkCounter();
    lastWorkCounterUpdate = currentMillis;
  }

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

        struct tm timeInfo;
        if (getLocalTime(&timeInfo)) {
          // Display location - left side, same line as timestamp
          display.setTextSize(4);
          display.setTextColor(ST77XX_CYAN);
          display.setCursor(10, 10);
          display.print(sensor.location);

          // Sensor read timestamp - right side, same line (from right edge)
          char readTimeStr[9];
          strftime(readTimeStr, sizeof(readTimeStr), "%H:%M:%S", &timeInfo);
          display.setTextSize(2);
          display.setTextColor(ST77XX_WHITE);
          display.setCursor(144, 45);  // From right edge: 8 chars * 6px * size_2 = 96px, 240-96=144
          display.println(readTimeStr);

          // Display temperature
          display.setTextSize(4);
          display.setTextColor(ST77XX_ORANGE);
          display.setCursor(10, 70);
          display.print(temp, 1);
          display.println(" C");

          // Display humidity
          display.setTextSize(4);
          display.setTextColor(ST77XX_GREEN);
          display.setCursor(10, 130);
          display.print(hum, 1);
          display.println(" %");

          // Update both bottom sections immediately after sensor read
          updateCurrentTime();
          updateWorkCounter();
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