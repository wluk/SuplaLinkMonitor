# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32-S3 Arduino project that fetches sensor data from Supla direct links via HTTP PATCH requests and displays readings over serial console. Built for Arduino IDE 2.x with ESP32 boards package.

## Build & Development Commands

### Compilation (Arduino CLI)
```bash
arduino-cli compile --fqbn esp32:esp32:esp32s3 SuplaLink_Monitor
```

### Upload to board
```bash
arduino-cli upload --fqbn esp32:esp32:esp32s3 -p <PORT> SuplaLink_Monitor
```

### CI Build (GitHub Actions)
The CI workflow in [.github/workflows/ci.yml](.github/workflows/ci.yml) automatically:
- Installs ESP32 core from `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
- Installs required libraries: ArduinoJson, Adafruit GFX Library, Adafruit ST7735 and ST7789 Library
- Compiles for `esp32:esp32:esp32s3` FQBN

### Arduino IDE setup
Required settings (Tools menu):
- Board: ESP32S3 Dev Module (or specific board model)
- USB CDC On Boot: Enabled (critical for serial output)
- Upload Mode/Port: USB CDC
- Baud rate: 115200

## Architecture

### Configuration System
Two-layer configuration using C preprocessor:

1. **[SuplaLink_Monitor/config.h](SuplaLink_Monitor/config.h)**: Default values for all configuration
   - Serial baud rate, WiFi credentials (placeholders), app metadata
   - `POLL_INTERVAL_MS`: sensor polling interval (default 20 minutes)
   - `TZ_STRING`: POSIX timezone for SNTP (default: CET/CEST with DST)

2. **secrets.h** (gitignored, optional): Overrides config.h values
   - WiFi credentials (`WIFI_SSID`, `WIFI_PASSWORD`)
   - `SENSORS_LIST` macro: array of Sensor structs
   - Create from [SuplaLink_Monitor/secrets.example.h](SuplaLink_Monitor/secrets.example.h)

The main sketch uses `__has_include("secrets.h")` to conditionally include secrets.h, falling back to config.h defaults if absent.

### Sensor System
- **SensorType enum**: `TEMPERATURE = 1`, `AIR = 2`
- **Sensor struct**: `{ url, code, location, type }`
- **SENSORS_LIST macro**: Expands to array initializer in secrets.h
  ```c
  #define SENSORS_LIST { \
    { "https://example.org/direct/123", "CODE123", "LOCATION1", TEMPERATURE }, \
    { "https://example.org/direct/456", "CODE456", "LOCATION2", AIR } \
  }
  ```

### Data Flow
1. **setup()**: Connect WiFi, initialize SNTP with configTzTime
2. **loop()**:
   - Get and print local time
   - For each sensor in `sensors[]`:
     - Build JSON body `{ code, action: "read", type }` using ArduinoJson StaticJsonDocument<128>
     - Send HTTP PATCH to sensor.url
     - Parse streaming JSON response with StaticJsonDocument<256>
     - Print formatted readings: `[LOCATION] Temp: XX.XX°C, Hum: XX.XX%`
   - Sleep for POLL_INTERVAL_MS

### Display Support

ST7789 TFT display (240x320) is now supported and enabled:

- `hasDisplay` flag set to true
- Display shows sensor location, temperature (orange), humidity (green), and timestamp
- SPI interface with configurable pins (TFT_CS, TFT_RST, TFT_DC)
- Default pins: CS=5, RST=16, DC=17 (can be overridden in config.h)
- Color display with multiple text sizes for better readability
- Rotation can be adjusted via `display.setRotation()`

## Dependencies

- **ESP32 core**: WiFi, HTTPClient, SNTP/time.h, SPI
- **ArduinoJson**: JSON serialization/deserialization (streaming parser for responses)
- **Adafruit_GFX + Adafruit_ST7789**: TFT color display driver for ST7789 controller

## File Structure
```
SuplaLink_Monitor/
├── SuplaLink_Monitor.ino    # Main sketch (setup, loop, readSensor)
├── config.h                  # Default configuration
├── secrets.example.h         # Template for secrets.h
└── secrets.h                 # Gitignored WiFi/sensor credentials
```

## Important Notes

- **secrets.h is never committed** (listed in .gitignore)
- Many ESP32-S3 boards use WS2812 RGB LEDs, not simple GPIO LEDs
- HTTP responses are parsed as streams to conserve memory
- Timezone handling uses POSIX TZ strings with automatic DST transitions
- Serial baud rate must match `SERIAL_BAUD` in config.h (115200)
- ST7789 display uses SPI interface - verify pin assignments for your specific ESP32-S3 board
- Display rotation can be adjusted (0-3) to match physical orientation
