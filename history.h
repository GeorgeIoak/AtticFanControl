#pragma once

#include <time.h>
#include <LittleFS.h>
#include "diagnostics.h"

// Log file path
#define HISTORY_LOG_PATH "/history.csv"
#define HISTORY_MAX_SIZE_BYTES (1024 * 1024) // 1MB limit
#define HISTORY_PRUNE_TO_LINES 5000 // Keep the most recent 5000 lines

/**
 * @brief Prunes the history log if it exceeds a defined size.
 * This function reads the end of the large file, stores it in memory,
 * deletes the old file, and writes a new one with the pruned content.
 * This prevents the filesystem from filling up over time.
 */
inline void pruneHistoryLog() {
  File historyFile = LittleFS.open(HISTORY_LOG_PATH, "r");
  if (!historyFile || historyFile.size() < HISTORY_MAX_SIZE_BYTES) {
    if (historyFile) historyFile.close();
    return;
  }

  logDiagnostics("[INFO] History log exceeds max size. Pruning...");

  // Create a temporary buffer to hold the last N lines
  String lastLines = "";
  int lineCount = 0;

  // Seek to a position near the end to start reading
  // This is an optimization to avoid reading the entire large file
  size_t seekPos = historyFile.size() > 256000 ? historyFile.size() - 256000 : 0; // 5000 lines * ~50 bytes/line
  historyFile.seek(seekPos, fs::SeekSet);
  if (seekPos > 0) {
    historyFile.readStringUntil('\n'); // Discard partial line
  }

  // Read the remaining lines into the buffer
  while (historyFile.available()) {
    lastLines += historyFile.readStringUntil('\n') + "\n";
    lineCount++;
  }
  historyFile.close();

  // If we have more lines than we want to keep, trim the buffer
  if (lineCount > HISTORY_PRUNE_TO_LINES) {
    int firstNewLine = -1;
    for (int i = 0; i < (lineCount - HISTORY_PRUNE_TO_LINES); i++) {
      firstNewLine = lastLines.indexOf('\n', firstNewLine + 1);
    }
    lastLines = lastLines.substring(firstNewLine + 1);
  }

  // Write the pruned content back to a new file
  historyFile = LittleFS.open(HISTORY_LOG_PATH, "w"); // "w" to overwrite
  historyFile.print("timestamp,attic_temp,outdoor_temp,humidity,fan_on\n" + lastLines);
  historyFile.close();
  logDiagnostics("[INFO] History log pruning complete.");
}

/**
 * @brief Appends a new entry to the history log file.
 * @param atticTemp Current attic temperature.
 * @param outdoorTemp Current outdoor temperature.
 * @param humidity Current attic humidity.
 * @param fanOn Current state of the fan.
 */
inline void appendHistoryLog(float atticTemp, float outdoorTemp, float humidity, bool fanOn) {
  // Check if the log needs pruning before writing
  pruneHistoryLog();

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
