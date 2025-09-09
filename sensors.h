
#pragma once
#include <math.h>
#include "hardware.h"           // GPIO and flags
#include "config.h"             // To access the runtime config
#include "diagnostics.h"
#include <Wire.h>                // I2C for SHT21
#include "DFRobot_SHT20.h"      // SHT21 sensor
#include <OneWire.h>            // DS18B20
#include <DallasTemperature.h>  // DS18B20

extern float simulatedAtticTemp;
extern float simulatedOutdoorTemp;
extern float simulatedAtticHumidity;

// Helper for jump check and logging
inline float validateSensorJump(const char* label, float newValue, float lastGood, float maxDelta) {
  if (fabs(newValue - lastGood) > maxDelta) {
    char buf[80];
    snprintf(buf, sizeof(buf), "[WARN] %s jump: %.1f -> %.1f", label, lastGood, newValue);
    #if DEBUG_SERIAL
    logSerial("%s", buf);
    #endif
    logDiagnostics(buf);
    return lastGood;
  }
  return newValue;
}

// Delta thresholds for sensor sanity checks
#define ATTIC_TEMP_DELTA_MAX 5.0   // °F, max allowed change per reading
#define ATTIC_HUMIDITY_DELTA_MAX 10.0 // %, max allowed change per reading
#define OUTDOOR_TEMP_DELTA_MAX 5.0 // °F, max allowed change per reading for outdoor sensor

// Last good values for sensors, initialized with mock values
static float lastGoodAtticTempF = MOCK_ATTIC_TEMP;
static float lastGoodAtticHumidity = 50.0;
static float lastGoodOutdoorTempF = MOCK_OUTDOOR_TEMP;

// === DS18B20 Setup ===
#if HAS_DS18B20
OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);
#endif

// === SHT21 Setup ===
#if HAS_SHT21
DFRobot_SHT20 sht21(&Wire, SHT21_I2C_ADDR);
#endif

// === Sensor Initialization ===
inline void initSensors() {
#if HAS_SHT21
  Wire.begin(SHT21_SDA_PIN, SHT21_SCL_PIN);
  sht21.initSHT20();
#endif

#if HAS_DS18B20
  ds18b20.begin();
#endif

  if (HAS_SHT21 || HAS_DS18B20) {
    #if DEBUG_SERIAL
    logSerial("[INFO] Physical sensors initialized.");
    #endif
  } else {
    #if DEBUG_SERIAL
    logSerial("[INFO] No physical sensors enabled. Using mock data.");
    #endif
  }
}

// === Attic Temperature (SHT21 or fallback) ===
inline float readAtticTemp() {
  if (config.testModeEnabled) { // Checks the runtime flag
    return simulatedAtticTemp;
  }
#if HAS_SHT21
  float tempC = sht21.readTemperature();
  float tempF = tempC * 1.8 + 32.0;
  // A valid reading should be within a plausible range and not NaN.
  if (!isnan(tempF) && tempF > -50 && tempF < 200) {
    // Guardband: skip if jump is too large
  lastGoodAtticTempF = validateSensorJump("Attic temp", tempF, lastGoodAtticTempF, ATTIC_TEMP_DELTA_MAX);
  } else {
    // Reading is invalid (NaN or out of range), likely a sensor error. Do nothing and use last known good value.
    logDiagnostics("[ERROR] Invalid attic temperature reading (NaN or out of range)");
  }
  return lastGoodAtticTempF;
#else
  return lastGoodAtticTempF;
#endif
}

// === Attic Humidity (SHT21 or fallback) ===
inline float readAtticHumidity() {
  if (config.testModeEnabled) {
    return simulatedAtticHumidity; // In test mode, use the simulated value.
  }
#if HAS_SHT21
  float humidity = sht21.readHumidity();
  // A valid reading should be within the 0-100% range and not NaN.
  if (!isnan(humidity) && humidity >= 0 && humidity <= 100) {
  lastGoodAtticHumidity = validateSensorJump("Attic humidity", humidity, lastGoodAtticHumidity, ATTIC_HUMIDITY_DELTA_MAX);
  } else {
    // Reading is invalid (NaN or out of range), likely a sensor error. Do nothing and use last known good value.
    logDiagnostics("[ERROR] Invalid attic humidity reading (NaN or out of range)");
  }
#endif
  return lastGoodAtticHumidity;
}

// === Outdoor Temperature (DS18B20 or fallback) ===
inline float readOutdoorTemp() {
  if (config.testModeEnabled) {
    return simulatedOutdoorTemp;
  }
#if HAS_DS18B20
  ds18b20.requestTemperatures();
  float tempF = ds18b20.getTempFByIndex(0);
  // Check for valid reading (not the error code) and that it's within a reasonable range.
  if (tempF != DEVICE_DISCONNECTED_F && tempF > -50 && tempF < 150) {
  lastGoodOutdoorTempF = validateSensorJump("Outdoor temp", tempF, lastGoodOutdoorTempF, OUTDOOR_TEMP_DELTA_MAX);
  } else {
    // Reading is invalid, likely a sensor error. Do nothing and use last known good value.
    logDiagnostics("[ERROR] Invalid outdoor temperature reading (NaN, out of range, or disconnected)");
  }
  return lastGoodOutdoorTempF;
#else
  return lastGoodOutdoorTempF; // Always return the static value if sensor not present
#endif
}
