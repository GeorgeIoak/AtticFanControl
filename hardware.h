#pragma once

#define ONBOARD_LED_PIN   D4     // Built-in LED on NodeMCU (GPIO2, active LOW)
// === GPIO Assignments ===
#define FAN_RELAY_PIN     D1     // Relay control for attic fan
#define SHT21_SDA_PIN     D2     // I2C SDA for SHT21
#define SHT21_SCL_PIN     D3     // I2C SCL for SHT21
#define DS18B20_PIN       D5     // OneWire data pin for DS18B20 (Moved from D4 to avoid conflict)
#define SHT21_I2C_ADDR    0x40   // I2C address for SHT21

// === Sensor Presence Flags ===
#define HAS_SHT21         false   // Set to false if SHT21 not connected
#define HAS_DS18B20       false   // Set to false if DS18B20 not connected

// === Mock Values for Testing ===
#define MOCK_ATTIC_TEMP   95.0   // °F, used if SHT21 not present
#define MOCK_OUTDOOR_TEMP 88.0   // °F, used if DS18B20 not present

// === Fan Control Logic ===
#define FAN_ON_TEMP_DEFAULT  90.0  // Default attic temp threshold (°F)
#define FAN_DELTA_TEMP_DEFAULT 5.0 // Default temp difference to trigger fan (°F)
#define FAN_HYSTERESIS_DEFAULT 2.0 // Default hysteresis (°F) to prevent rapid cycling
#define PRECOOL_TRIGGER_TEMP_DEFAULT 90.0 // Forecast high to trigger pre-cooling
#define PRECOOL_TEMP_OFFSET_DEFAULT 5.0  // How many degrees to lower the ON temp
#define PRECOOLING_ENABLED_DEFAULT true // Whether pre-cooling is enabled by default
#define ONBOARD_LED_ENABLED_DEFAULT true // Whether the status LED is enabled by default
#define TEST_MODE_ENABLED_DEFAULT false // Whether test mode is enabled by default
#define HISTORY_LOG_INTERVAL_DEFAULT 300000UL // 5 minutes in ms
#define MQTT_ENABLED_DEFAULT false // Whether MQTT is enabled by default
#define MQTT_DISCOVERY_ENABLED_DEFAULT false // Whether to publish Home Assistant discovery topics
#define DAILY_RESTART_ENABLED_DEFAULT true // Whether the daily restart is enabled by default

// === OTA Update Port (optional override) ===
// #define OTA_PORT        8266

// === Weather Forecast ===
#define WEATHER_UPDATE_INTERVAL_MS 1800000 // How often to fetch weather (30 minutes)
#define WEATHER_LATITUDE      38.72   // Latitude for weather forecast
#define WEATHER_LONGITUDE     -121.36 // Longitude for weather forecast

// === WiFi Connection ===
#define WIFI_INITIAL_RETRY_DELAY_MS  30000  // First retry delay (ms)
#define WIFI_MAX_RETRY_DELAY_MS      300000 // Max retry delay (5 minutes)
#define WIFI_RETRY_BACKOFF_FACTOR    2      // Multiplier for retry delay (e.g., 30s, 60s, 120s...)
#define USE_STATIC_IP                false  // Set to true to use the static IP from secrets.h

// === Access Point (AP) Fallback Mode ===
#define AP_FALLBACK_ENABLED   true         // Set to true to enable AP mode after WiFi failures
#define AP_SSID               "AtticFanSetup" // The name of the fallback WiFi network
#define AP_PASSWORD           "fancontrol" // Password for the fallback network (8+ characters)

// === mDNS Configuration ===
#define MDNS_HOSTNAME         "atticfan" // Hostname for .local address

// === Debugging ===
#define DEBUG_SERIAL      true
#define FIRMWARE_VERSION  "0.98"
