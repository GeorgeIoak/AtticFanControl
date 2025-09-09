#pragma once

#include <time.h>
#include <LittleFS.h>
#include "diagnostics.h"

// Log file path
#define HISTORY_LOG_PATH "/history.csv"

/**
 * @brief Appends a new entry to the history log file.
 * @param atticTemp Current attic temperature.
 * @param outdoorTemp Current outdoor temperature.
 * @param humidity Current attic humidity.
 * @param fanOn Current state of the fan.
 */
inline void appendHistoryLog(float atticTemp, float outdoorTemp, float humidity, bool fanOn) {
  File historyFile = LittleFS.open(HISTORY_LOG_PATH, "a");
  if (!historyFile) {
    logDiagnostics("[ERROR] Could not open history log for writing.");
    return;
  }

  // If the file is empty, write the header first.
  if (historyFile.size() == 0) {
    historyFile.println("timestamp,attic_temp,outdoor_temp,humidity,fan_on");
  }

  // --- Timestamp Logic ---
  char timestampStr[20]; // Buffer for "YYYY-MM-DDTHH:MM"
  time_t now;
  time(&now);
  struct tm* timeinfo = localtime(&now);

  // Format the timestamp only if the year is valid (time has been synced)
  if (timeinfo && timeinfo->tm_year > 70) { // Year is since 1900, so >70 is a safe check for a valid year like 2023
    strftime(timestampStr, sizeof(timestampStr), "%Y-%m-%dT%H:%M", timeinfo);
  } else {
    timestampStr[0] = '\0'; // Leave timestamp blank if time not synced
  }
  // --- End Timestamp Logic ---

  // Format the data as a CSV line.
  historyFile.printf("%s,%.2f,%.2f,%.2f,%d\n", timestampStr, atticTemp, outdoorTemp, humidity, fanOn ? 1 : 0);
  historyFile.close();
}
