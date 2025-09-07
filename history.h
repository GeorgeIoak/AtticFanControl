#pragma once

#include <LittleFS.h>
#include "weather.h"
#include "sensors.h"

// Log file path
#define HISTORY_LOG_PATH "/history.csv"

// Write a CSV header if the file is new
inline void ensureHistoryHeader() {
  if (!LittleFS.exists(HISTORY_LOG_PATH)) {
    File f = LittleFS.open(HISTORY_LOG_PATH, "w");
    if (f) {
      f.println("timestamp,attic_temp,outdoor_temp,humidity,fan_on");
      f.close();
    }
  }
}

// Append a new log entry
inline void appendHistoryLog(const char* timestamp, float atticTemp, float outdoorTemp, float humidity, bool fanOn) {
  ensureHistoryHeader();
  File f = LittleFS.open(HISTORY_LOG_PATH, "a");
  if (f) {
    // Use "N/A" for timestamp if it's null to prevent a crash.
    f.printf("%s,%.2f,%.2f,%.2f,%d\n", (timestamp ? timestamp : "N/A"), atticTemp, outdoorTemp, humidity, fanOn ? 1 : 0);
    f.close();
  }
}

// Optionally: add a function to rotate/archive the log file (not implemented here)
