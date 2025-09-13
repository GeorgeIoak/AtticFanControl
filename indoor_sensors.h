#pragma once

#include "types.h"
#include "diagnostics.h"
#include <Arduino.h>

// Global array to store indoor sensor data
IndoorSensorData indoorSensors[MAX_INDOOR_SENSORS];
int activeSensorCount = 0;

/**
 * @brief Initialize the indoor sensors system
 */
inline void initIndoorSensors() {
  for (int i = 0; i < MAX_INDOOR_SENSORS; i++) {
    indoorSensors[i].isActive = false;
    indoorSensors[i].lastUpdate = 0;
    indoorSensors[i].temperature = 0.0;
    indoorSensors[i].humidity = 0.0;
  }
  activeSensorCount = 0;
  logDiagnostics("[INFO] Indoor sensors system initialized.");
}

/**
 * @brief Find an indoor sensor by ID
 * @param sensorId The sensor ID to search for
 * @return Index of the sensor, or -1 if not found
 */
inline int findSensorById(const String& sensorId) {
  for (int i = 0; i < MAX_INDOOR_SENSORS; i++) {
    if (indoorSensors[i].isActive && indoorSensors[i].sensorId == sensorId) {
      return i;
    }
  }
  return -1;
}

/**
 * @brief Find an available slot for a new sensor
 * @return Index of available slot, or -1 if array is full
 */
inline int findAvailableSlot() {
  for (int i = 0; i < MAX_INDOOR_SENSORS; i++) {
    if (!indoorSensors[i].isActive) {
      return i;
    }
  }
  return -1;
}

/**
 * @brief Register or update an indoor sensor
 * @param sensorId Unique identifier for the sensor
 * @param name Human-readable name for the sensor
 * @param temperature Temperature reading in Fahrenheit
 * @param humidity Humidity reading in percentage
 * @param ipAddress IP address of the sensor device
 * @return true if successful, false if failed
 */
inline bool registerOrUpdateSensor(const String& sensorId, const String& name, 
                                  float temperature, float humidity, const String& ipAddress) {
  int sensorIndex = findSensorById(sensorId);
  
  // If sensor exists, update it
  if (sensorIndex >= 0) {
    indoorSensors[sensorIndex].name = name;
    indoorSensors[sensorIndex].temperature = temperature;
    indoorSensors[sensorIndex].humidity = humidity;
    indoorSensors[sensorIndex].lastUpdate = millis();
    indoorSensors[sensorIndex].ipAddress = ipAddress;
    return true;
  }
  
  // If sensor doesn't exist, create new one
  int availableSlot = findAvailableSlot();
  if (availableSlot >= 0) {
    indoorSensors[availableSlot].sensorId = sensorId;
    indoorSensors[availableSlot].name = name;
    indoorSensors[availableSlot].temperature = temperature;
    indoorSensors[availableSlot].humidity = humidity;
    indoorSensors[availableSlot].lastUpdate = millis();
    indoorSensors[availableSlot].ipAddress = ipAddress;
    indoorSensors[availableSlot].isActive = true;
    activeSensorCount++;
    
    char logMsg[128];
    snprintf(logMsg, sizeof(logMsg), "[INFO] New indoor sensor registered: %s (%s)", 
             name.c_str(), sensorId.c_str());
    logDiagnostics(logMsg);
    return true;
  }
  
  // Array is full
  logDiagnostics("[WARN] Cannot register indoor sensor - maximum limit reached");
  return false;
}

/**
 * @brief Clean up expired sensors
 * Removes sensors that haven't reported in for INDOOR_SENSOR_TIMEOUT_MS
 */
inline void cleanupExpiredSensors() {
  unsigned long currentTime = millis();
  
  for (int i = 0; i < MAX_INDOOR_SENSORS; i++) {
    if (indoorSensors[i].isActive && 
        (currentTime - indoorSensors[i].lastUpdate) > INDOOR_SENSOR_TIMEOUT_MS) {
      
      char logMsg[128];
      snprintf(logMsg, sizeof(logMsg), "[INFO] Indoor sensor expired: %s (%s)", 
               indoorSensors[i].name.c_str(), indoorSensors[i].sensorId.c_str());
      logDiagnostics(logMsg);
      
      indoorSensors[i].isActive = false;
      indoorSensors[i].sensorId = "";
      indoorSensors[i].name = "";
      activeSensorCount--;
    }
  }
}

/**
 * @brief Get the number of active indoor sensors
 * @return Number of active sensors
 */
inline int getActiveSensorCount() {
  cleanupExpiredSensors(); // Clean up before counting
  return activeSensorCount;
}

/**
 * @brief Get average indoor temperature from all active sensors
 * @return Average temperature in Fahrenheit, or NaN if no sensors active
 */
inline float getAverageIndoorTemperature() {
  cleanupExpiredSensors();
  
  float totalTemp = 0.0;
  int validSensors = 0;
  
  for (int i = 0; i < MAX_INDOOR_SENSORS; i++) {
    if (indoorSensors[i].isActive) {
      totalTemp += indoorSensors[i].temperature;
      validSensors++;
    }
  }
  
  return (validSensors > 0) ? (totalTemp / validSensors) : NAN;
}

/**
 * @brief Get average indoor humidity from all active sensors
 * @return Average humidity in percentage, or NaN if no sensors active
 */
inline float getAverageIndoorHumidity() {
  cleanupExpiredSensors();
  
  float totalHumidity = 0.0;
  int validSensors = 0;
  
  for (int i = 0; i < MAX_INDOOR_SENSORS; i++) {
    if (indoorSensors[i].isActive) {
      totalHumidity += indoorSensors[i].humidity;
      validSensors++;
    }
  }
  
  return (validSensors > 0) ? (totalHumidity / validSensors) : NAN;
}

/**
 * @brief Remove a specific sensor by ID
 * @param sensorId The sensor ID to remove
 * @return true if sensor was found and removed, false otherwise
 */
inline bool removeSensor(const String& sensorId) {
  int sensorIndex = findSensorById(sensorId);
  if (sensorIndex >= 0) {
    char logMsg[128];
    snprintf(logMsg, sizeof(logMsg), "[INFO] Indoor sensor removed: %s (%s)", 
             indoorSensors[sensorIndex].name.c_str(), sensorId.c_str());
    logDiagnostics(logMsg);
    
    indoorSensors[sensorIndex].isActive = false;
    indoorSensors[sensorIndex].sensorId = "";
    indoorSensors[sensorIndex].name = "";
    activeSensorCount--;
    return true;
  }
  return false;
}