## SuplaLink Monitor (ESP32-S3, Arduino IDE) ![Version](https://img.shields.io/badge/version-0.1.0-blue)

Lightweight ESP32-S3 app that fetches and prints sensor data from Supla direct links to the serial console.

### Requirements
- **Arduino IDE 2.x**
- **ESP32 boards package for Arduino** by Espressif

### Installing ESP32 support in Arduino IDE
1. Open: Preferences → Additional Boards Manager URLs
2. Add URL: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
3. Open: Tools → Board → Boards Manager, search for "esp32" and install "ESP32 by Espressif Systems"

### Getting started
1. Open the `SuplaLink_Monitor` folder in Arduino IDE and the `SuplaLink_Monitor.ino` file.
2. Select your board: Tools → Board → ESP32 → e.g., "ESP32S3 Dev Module" (or your exact model).
3. Recommended settings (if available in your core version):
   - USB CDC On Boot: Enabled
   - Upload Mode/Port: USB CDC
   - Flash Mode/Speed/Size: board defaults
   - Partition Scheme: Default
4. Select the correct port (Tools → Port), typically `cu.usbmodem...` on macOS.
5. Configure WiFi and sensors in `SuplaLink_Monitor/secrets.h` (details below).
6. Click Upload.

### Configuration
In `SuplaLink_Monitor/config.h`:
- **SERIAL_BAUD**: serial console speed (115200 by default)
- **APP_NAME**: application name printed on boot

In your private `SuplaLink_Monitor/secrets.h` (not committed):
- **WIFI_SSID/WIFI_PASSWORD**: WiFi credentials
- **SENSORS_LIST**: list of Supla direct links with codes and types (see below)

Note: Many ESP32-S3 boards have an addressable RGB LED (WS2812) instead of a simple GPIO LED. In that case, the basic GPIO blink will not be visible.

### Structure
```
├─ SuplaLink_Monitor/
│  ├─ SuplaLink_Monitor.ino
│  ├─ secrets.example.h   # template to copy into secrets.h (not committed)
│  └─ config.h
├─ .gitignore
└─ README.md
```

### Troubleshooting and tips
- No logs in Serial Monitor? Ensure "USB CDC On Boot" is Enabled, the baud rate is 115200, and reset the board.
- Upload error? Unplug/replug USB, select the correct port, check the cable.
- No onboard classic LED? Change `LED_PIN` accordingly or disable blinking by setting `-1`.

### Private WiFi credentials (not committed)
To keep your WiFi password out of the repo:
1. Copy `SuplaLink_Monitor/secrets.example.h` to `SuplaLink_Monitor/secrets.h`.
2. Edit `SuplaLink_Monitor/secrets.h` and set `WIFI_SSID` and `WIFI_PASSWORD`.

Notes:
- `secrets.h` is listed in `.gitignore`, so it won't be committed.
- The sketch includes `secrets.h` only if it exists; otherwise it uses values from `config.h`.

#### One-liners to create secrets.h
macOS/Linux (zsh/bash):
```bash
cp SuplaLink_Monitor/secrets.example.h SuplaLink_Monitor/secrets.h && sed -i '' 's/YourWiFiSSID/MyNetworkSSID/' SuplaLink_Monitor/secrets.h && sed -i '' 's/YourWiFiPassword/MyStrongPassword123!/' SuplaLink_Monitor/secrets.h
```

Linux (GNU sed):
```bash
cp SuplaLink_Monitor/secrets.example.h SuplaLink_Monitor/secrets.h && sed -i 's/YourWiFiSSID/MyNetworkSSID/' SuplaLink_Monitor/secrets.h && sed -i 's/YourWiFiPassword/MyStrongPassword123!/' SuplaLink_Monitor/secrets.h
```

Windows PowerShell:
```powershell
Copy-Item SuplaLink_Monitor/secrets.example.h SuplaLink_Monitor/secrets.h; (Get-Content SuplaLink_Monitor/secrets.h) -replace 'YourWiFiSSID','MyNetworkSSID' -replace 'YourWiFiPassword','MyStrongPassword123!' | Set-Content SuplaLink_Monitor/secrets.h
```

### Private sensors list (not committed)
You can define your private sensors directly in `SuplaLink_Monitor/secrets.h` via the `SENSORS_LIST` macro. Example:
```c
#define SENSORS_LIST { \
  { "https://example.org/direct/123", "CODE123", "LOCATION1", TEMPERATURE }, \
  { "https://example.org/direct/456", "CODE456", "LOCATION2", AIR } \
}
```
If `SENSORS_LIST` is not defined, the build uses an empty sensors list by default.

### How it works (code overview)
- Connects to WiFi using `WIFI_SSID`/`WIFI_PASSWORD` and prints a short status with `APP_NAME`.
- Keeps time via NTP (`pool.ntp.org`) and prints formatted time each cycle.
- Iterates over `sensors[]` (from `SENSORS_LIST`) and for each sensor:
  - Builds a small JSON body `{ code, action:"read", type }` on the stack (ArduinoJson).
  - Sends an HTTP PATCH to the Supla direct URL and parses the JSON response stream.
  - Prints readings like: `[LOCATION] Temp: 23.45°C, Hum: 45.67%`.
- Sleeps 20 minutes between cycles (adjust `delay(1200000)` if needed).

Key types:
- `SensorType` enum: `TEMPERATURE = 1`, `AIR = 2`
- `Sensor` struct: `url`, `code`, `location`, `type`

### Dependencies
- ESP32 core (includes `WiFi`, `HTTPClient`)
- ArduinoJson (via Library Manager)
- NTPClient (via Library Manager)
- Optional (not used yet): `Adafruit_GFX`, `Adafruit_SSD1306`

### Usage
1. Create `SuplaLink_Monitor/secrets.h` with WiFi credentials and `SENSORS_LIST`.
2. Select board/port and upload.
3. Open Serial Monitor at `115200` to view logs and readings.

### Name
SuplaLink Monitor


