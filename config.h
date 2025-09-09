#pragma once

#include <EEPROM.h>
#include "hardware.h"

// A "magic number" to verify if EEPROM data is valid
#define EEPROM_MAGIC 0x46414E43 // "FANC" in hex
#define RTC_RESET_MAGIC 0xDEADBEEF

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

// RTC Memory functions to handle reset flag
void setResetFlag() {
  uint32_t magic = RTC_RESET_MAGIC;
  ESP.rtcUserMemoryWrite(0, &magic, sizeof(magic));
}

bool isResetFlagged() {
  uint32_t magic;
  ESP.rtcUserMemoryRead(0, &magic, sizeof(magic));
  return magic == RTC_RESET_MAGIC;
}

void clearResetFlag() {
  uint32_t magic = 0;
  ESP.rtcUserMemoryWrite(0, &magic, sizeof(magic));
}

/**
 * @brief Saves the current configuration to EEPROM.
 */
inline void saveConfig() {
  config.magic = EEPROM_MAGIC; // Set the magic number before saving
  EEPROM.put(0, config);
  if (EEPROM.commit()) {
    #if DEBUG_SERIAL
    logSerial("[INFO] Configuration saved to EEPROM.");
    #endif
  }
}

/**
 * @brief Clears the configuration from EEPROM, forcing a load of default values on next boot.
 */
inline void clearConfig() {
  setResetFlag();
  config.magic = 0; // Invalidate the magic number
  EEPROM.put(0, config);
  if (EEPROM.commit()) {
    #if DEBUG_SERIAL
    logSerial("[INFO] Configuration cleared from EEPROM.");
    #endif
  }
}

/**
 * @brief Loads configuration from EEPROM. If EEPROM is invalid or empty,
 * it loads default values and saves them.
 */
inline void loadConfig() {
  EEPROM.begin(sizeof(Config)); // Allocate space
  
  if (isResetFlagged()) {
    clearResetFlag();
    #if DEBUG_SERIAL
    logSerial("[INFO] Reset flag detected. Loading default configuration.");
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
    return;
  }

  EEPROM.get(0, config);

  // Check if the magic number matches. If not, data is invalid.
  if (config.magic != EEPROM_MAGIC) {
    #if DEBUG_SERIAL
    logSerial("[WARN] Invalid config in EEPROM or first boot. Loading defaults.");
    #endif
    logDiagnostics("[WARN] Invalid config in EEPROM. Loading defaults.");
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
    logSerial("[INFO] Configuration loaded from EEPROM.");
    logSerial("  - Fan On Temp: %.1f°F", config.fanOnTemp);
    logSerial("  - Fan Delta Temp: %.1f°F", config.fanDeltaTemp);
    logSerial("  - Fan Hysteresis: %.1f°F", config.fanHysteresis);
    logSerial("  - Pre-Cool Trigger: %.1f°F", config.preCoolTriggerTemp);
    logSerial("  - Pre-Cool Offset: %.1f°F", config.preCoolTempOffset);
    logSerial("  - Pre-Cooling Enabled: %s", config.preCoolingEnabled ? "true" : "false");
    logSerial("  - Onboard LED Enabled: %s", config.onboardLedEnabled ? "true" : "false");
    logSerial("  - Test Mode Enabled: %s", config.testModeEnabled ? "true" : "false");
    logSerial("  - Daily Restart Enabled: %s", config.dailyRestartEnabled ? "true" : "false");
    logSerial("  - MQTT Enabled: %s", config.mqttEnabled ? "true" : "false");
    logSerial("  - MQTT Discovery Enabled: %s", config.mqttDiscoveryEnabled ? "true" : "false");
    logSerial("  - History Log Interval: %lu ms", config.historyLogIntervalMs);
    #endif

    // Sanity check for the history log interval. If the value from EEPROM is corrupt or
    // outside a reasonable range (1 min to 24 hours), reset it to the default.
    if (config.historyLogIntervalMs < 60000UL || config.historyLogIntervalMs > 86400000UL) {
      #if DEBUG_SERIAL
      logSerial("[WARN] Invalid history log interval (%lu ms) from EEPROM. Resetting to default (%lu ms).", config.historyLogIntervalMs, HISTORY_LOG_INTERVAL_DEFAULT);
      #endif
      config.historyLogIntervalMs = HISTORY_LOG_INTERVAL_DEFAULT;
      saveConfig(); // Save the corrected configuration.
    }
  }
}