#pragma once

#include "types.h"
#include "diagnostics.h"
#include <EEPROM.h>
#include "hardware.h"

// A "magic number" to verify if EEPROM data is valid
#define EEPROM_MAGIC 0x46414E43 // "FANC" in hex
#define RTC_RESET_MAGIC 0xDEADBEEF

// Define a structure to hold all configurable settings
struct __attribute__((packed)) Config {
  uint32_t magic; // To check for valid data
  FanMode fanMode;
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
  bool indoorSensorsEnabled;
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
 * @brief A helper function to check a float config value, log, and correct if invalid.
 */
inline bool checkAndCorrectFloat(float* value, const char* name, float min, float max, float defaultValue) {
  if (isnan(*value) || *value < min || *value > max) {
    char buffer[128];
    // Use a temporary float to print the bad value, as it might be garbage.
    float badValue = *value;
    *value = defaultValue; // Correct the value first
    snprintf(buffer, sizeof(buffer), "[WARN] Invalid '%s' (val: %f) in config. Reset to default (%.1f).", name, badValue, *value);
    logDiagnostics(buffer);
    return true;
  }
  return false;
}

/**
 * @brief An overloaded helper for unsigned long values.
 */
inline bool checkAndCorrectULong(unsigned long* value, const char* name, unsigned long min, unsigned long max, unsigned long defaultValue) {
  if (*value < min || *value > max) {
    char buffer[128];
    unsigned long badValue = *value;
    *value = defaultValue; // Correct the value first
    snprintf(buffer, sizeof(buffer), "[WARN] Invalid '%s' (val: %lu) in config. Reset to default (%lu).", name, badValue, *value);
    logDiagnostics(buffer);
    return true;
  }
  return false;
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
    config.fanMode = FAN_MODE_DEFAULT;
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
    config.indoorSensorsEnabled = INDOOR_SENSORS_ENABLED_DEFAULT;
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
    config.fanMode = FAN_MODE_DEFAULT;
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
    config.indoorSensorsEnabled = INDOOR_SENSORS_ENABLED_DEFAULT;
    config.historyLogIntervalMs = HISTORY_LOG_INTERVAL_DEFAULT;
    // Save the default configuration for next time
    saveConfig();
  } else {
    bool configWasCorrected = false;
    // --- Sanity checks for all numeric config values ---
    // This prevents uninitialized memory from being used if a new field
    // is added and the device has an older config in EEPROM.

    // Temperature thresholds (reasonable range: 50-150 F)
    configWasCorrected |= checkAndCorrectFloat(&config.fanOnTemp, "fanOnTemp", 50.0f, 150.0f, FAN_ON_TEMP_DEFAULT);
    configWasCorrected |= checkAndCorrectFloat(&config.preCoolTriggerTemp, "preCoolTriggerTemp", 50.0f, 150.0f, PRECOOL_TRIGGER_TEMP_DEFAULT);

    // Temperature differentials (reasonable range: 0-50 F)
    configWasCorrected |= checkAndCorrectFloat(&config.fanDeltaTemp, "fanDeltaTemp", 0.0f, 50.0f, FAN_DELTA_TEMP_DEFAULT);
    configWasCorrected |= checkAndCorrectFloat(&config.fanHysteresis, "fanHysteresis", 0.0f, 50.0f, FAN_HYSTERESIS_DEFAULT);
    configWasCorrected |= checkAndCorrectFloat(&config.preCoolTempOffset, "preCoolTempOffset", 0.0f, 50.0f, PRECOOL_TEMP_OFFSET_DEFAULT);

    // History log interval (1 minute to 24 hours in ms)
    configWasCorrected |= checkAndCorrectULong(&config.historyLogIntervalMs, "historyLogIntervalMs", 60000UL, 86400000UL, HISTORY_LOG_INTERVAL_DEFAULT);

    if (configWasCorrected) {
      #if DEBUG_SERIAL
      logSerial("[WARN] One or more config values were invalid. Corrected and re-saving EEPROM.");
      #endif
      saveConfig(); // Save the corrected configuration back to EEPROM
    }

    #if DEBUG_SERIAL
    logSerial("[INFO] Configuration loaded from EEPROM.");
    const char* modeStr;
    switch(config.fanMode) {
      case MANUAL_ON: modeStr = "MANUAL_ON"; break;
      case MANUAL_OFF: modeStr = "MANUAL_OFF"; break;
      case MANUAL_TIMED: modeStr = "MANUAL_TIMED"; break;
      case AUTO: modeStr = "AUTO"; break;
      default: modeStr = "INVALID"; // Handle corrupt/unknown values
    }
    logSerial("  - Fan Mode: %s (%d)", modeStr, config.fanMode); // Log the loaded mode
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
    logSerial("  - Indoor Sensors Enabled: %s", config.indoorSensorsEnabled ? "true" : "false");
    logSerial("  - History Log Interval: %lu ms", config.historyLogIntervalMs);
    #endif
  }
}