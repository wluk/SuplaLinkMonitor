// Basic configuration for ESP32-S3 Arduino project
// Adjust values for your board and environment

#pragma once

// Serial console baud rate
#ifndef SERIAL_BAUD
#define SERIAL_BAUD 115200
#endif

// WiFi credentials
#ifndef WIFI_SSID
#define WIFI_SSID "YOUR_WIFI_SSID"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#endif

// Application name (used only for logs)
#ifndef APP_NAME
#define APP_NAME "SuplaLink Monitor"
#endif

// App versioning
#ifndef APP_VERSION
#define APP_VERSION "0.1.0"
#endif

#ifndef APP_BUILD_DATE
#define APP_BUILD_DATE __DATE__
#endif

#ifndef APP_BUILD_TIME
#define APP_BUILD_TIME __TIME__
#endif

// Polling interval for fetching data (milliseconds)
#ifndef POLL_INTERVAL_MS
#define POLL_INTERVAL_MS 180000 // 3 minutes
#endif

// Delay between reading individual sensors (milliseconds)
#ifndef SENSOR_DISPLAY_DELAY_MS
#define SENSOR_DISPLAY_DELAY_MS 60000 // 60 seconds
#endif

// Timezone for configTzTime (POSIX TZ format). Default: CET with DST (CEST).
#ifndef TZ_STRING
#define TZ_STRING "CET-1CEST,M3.5.0,M10.5.0/3"
#endif

// ST7789 Display configuration
// Uncomment and adjust these pins for your specific ESP32-S3 board
// #define TFT_CS    5   // Chip Select pin
// #define TFT_RST   16  // Reset pin
// #define TFT_DC    17  // Data/Command pin
// Default SPI pins: MOSI=11, MISO=13, SCK=12 (varies by ESP32-S3 board)
