#pragma once

// Defines the operational modes for the fan.
// This is in a separate file to be easily shared across the project.
enum FanMode {
  AUTO,
  MANUAL_ON,
  MANUAL_OFF,
  MANUAL_TIMED // A new mode for when a timer is active
};

// Defines what to do after a timed run completes.
enum PostTimerAction {
  STAY_MANUAL,
  REVERT_TO_AUTO
};

// Holds the state for a manual timed run.
struct ManualTimerState {
  bool isActive = false;
  unsigned long delayEndTime = 0;
  unsigned long timerEndTime = 0;
  PostTimerAction postAction = REVERT_TO_AUTO;
};

// Indoor sensor data structure
struct IndoorSensorData {
  String sensorId;        // Unique identifier for the sensor
  String name;            // Human-readable name
  float temperature;      // Temperature in Fahrenheit
  float humidity;         // Relative humidity percentage
  unsigned long lastUpdate; // Timestamp of last update (millis())
  String ipAddress;       // IP address of the sensor device
  bool isActive;          // Whether the sensor is currently active
};

// Maximum number of indoor sensors supported
#define MAX_INDOOR_SENSORS 10

// Indoor sensor data retention period (30 minutes)
#define INDOOR_SENSOR_TIMEOUT_MS 1800000UL