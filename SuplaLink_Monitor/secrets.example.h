// Copy this file to secrets.h and fill in your credentials.
// secrets.h is ignored by git.

#pragma once

#ifndef WIFI_SSID
#define WIFI_SSID "YourWiFiSSID"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "YourWiFiPassword"
#endif

// Optionally define your private sensors here. The main sketch looks for SENSORS_LIST.
// Example:
// #define SENSORS_LIST { \
//   { "https://example.org/direct/123", "CODE123", "LOCATION1", TEMPERATURE }, \
//   { "https://example.org/direct/456", "CODE456", "LOCATION2", AIR } \
// }
