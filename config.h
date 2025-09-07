#pragma once

#include <EEPROM.h>
#include "hardware.h"

// A "magic number" to verify if EEPROM data is valid
#define EEPROM_MAGIC 0x46414E43 // "FANC" in hex

// Define a structure to hold all configurable settings
struct Config {
  uint32_t magic; // To check for valid data
  float fanOnTemp;
  float fanDeltaTemp;
  float fanHysteresis;
  float preCoolTriggerTemp;
  float preCoolTempOffset;
  bool preCoolingEnabled;
  bool onboardLedEnabled;
  bool testModeEnabled;
  bool dailyRestartEnabled;
  bool mqttEnabled;
  bool mqttDiscoveryEnabled;
  unsigned long historyLogIntervalMs;
};

// Global instance of our configuration
Config config;

/**
 * @brief Saves the current configuration to EEPROM.
 */
inline void saveConfig() {
  config.magic = EEPROM_MAGIC; // Set the magic number before saving
  EEPROM.put(0, config);
  EEPROM.commit();
#if DEBUG_SERIAL
  Serial.println("[INFO] Configuration saved to EEPROM.");
#endif
}

/**
 * @brief Loads configuration from EEPROM. If EEPROM is invalid or empty,
 * it loads default values and saves them.
 */
inline void loadConfig() {
  EEPROM.begin(sizeof(Config)); // Allocate space
  EEPROM.get(0, config);

  // Check if the magic number matches. If not, data is invalid.
  if (config.magic != EEPROM_MAGIC) {
#if DEBUG_SERIAL
    Serial.println("[WARN] Invalid config in EEPROM or first boot. Loading defaults.");
#endif
    // Load default values from hardware.h
    config.fanOnTemp = FAN_ON_TEMP_DEFAULT;
    config.fanDeltaTemp = FAN_DELTA_TEMP_DEFAULT;
    config.fanHysteresis = FAN_HYSTERESIS_DEFAULT;
    config.preCoolTriggerTemp = PRECOOL_TRIGGER_TEMP_DEFAULT;
    config.preCoolTempOffset = PRECOOL_TEMP_OFFSET_DEFAULT;
    config.preCoolingEnabled = PRECOOLING_ENABLED_DEFAULT;
    config.onboardLedEnabled = ONBOARD_LED_ENABLED_DEFAULT;
    config.testModeEnabled = TEST_MODE_ENABLED_DEFAULT;
    config.dailyRestartEnabled = DAILY_RESTART_ENABLED_DEFAULT;
    config.mqttEnabled = MQTT_ENABLED_DEFAULT;
    config.mqttDiscoveryEnabled = MQTT_DISCOVERY_ENABLED_DEFAULT;
    config.historyLogIntervalMs = HISTORY_LOG_INTERVAL_DEFAULT;
    // Save the default configuration for next time
    saveConfig();
  } else {
#if DEBUG_SERIAL
    Serial.println("[INFO] Configuration loaded from EEPROM.");
    Serial.printf("  - Fan On Temp: %.1f°F\n", config.fanOnTemp);
    Serial.printf("  - Fan Delta Temp: %.1f°F\n", config.fanDeltaTemp);
    Serial.printf("  - Fan Hysteresis: %.1f°F\n", config.fanHysteresis);
    Serial.printf("  - Pre-Cool Trigger: %.1f°F\n", config.preCoolTriggerTemp);
    Serial.printf("  - Pre-Cool Offset: %.1f°F\n", config.preCoolTempOffset);
    Serial.printf("  - Pre-Cooling Enabled: %s\n", config.preCoolingEnabled ? "true" : "false");
    Serial.printf("  - Onboard LED Enabled: %s\n", config.onboardLedEnabled ? "true" : "false");
    Serial.printf("  - Test Mode Enabled: %s\n", config.testModeEnabled ? "true" : "false");
    Serial.printf("  - Daily Restart Enabled: %s\n", config.dailyRestartEnabled ? "true" : "false");
    Serial.printf("  - MQTT Enabled: %s\n", config.mqttEnabled ? "true" : "false");
    Serial.printf("  - MQTT Discovery Enabled: %s\n", config.mqttDiscoveryEnabled ? "true" : "false");
    Serial.printf("  - History Log Interval: %lu ms\n", config.historyLogIntervalMs);
#endif

    // Sanity check for the history log interval. If the value from EEPROM is corrupt or
    // outside a reasonable range (1 min to 24 hours), reset it to the default.
    if (config.historyLogIntervalMs < 60000UL || config.historyLogIntervalMs > 86400000UL) {
#if DEBUG_SERIAL
      Serial.printf("[WARN] Invalid history log interval (%lu ms) from EEPROM. Resetting to default (%lu ms).\n", config.historyLogIntervalMs, HISTORY_LOG_INTERVAL_DEFAULT);
#endif
      config.historyLogIntervalMs = HISTORY_LOG_INTERVAL_DEFAULT;
      saveConfig(); // Save the corrected configuration.
    }
  }
}