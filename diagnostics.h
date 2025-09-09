#pragma once
#include <LittleFS.h>
#include <Arduino.h> // For millis()
#include <time.h>
#include <stdarg.h> // For va_list

#define DIAGNOSTICS_LOG_PATH "/diagnostics.log"

extern bool ntpHasSynced; // From AtticFanControl.ino

// Forward-declare the logging function from the main .ino file
void logSerial(const char* format, ...);

inline void logDiagnostics(const char* msg) {
  File f = LittleFS.open(DIAGNOSTICS_LOG_PATH, "a");
  if (f) {
    // --- Timestamp Logic ---
    char timestamp[30];
    if (ntpHasSynced) {
      time_t now;
      time(&now);
      struct tm* timeinfo = localtime(&now);
      strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S] ", timeinfo);
    } else {
      snprintf(timestamp, sizeof(timestamp), "[%lu] ", millis());
    }
    // --- End Timestamp Logic ---
    f.print(timestamp);
    f.println(msg); // Print the original message
    f.close();
  }
}

inline void handleDiagnosticsDownload(ESP8266WebServer &server) {
  if (LittleFS.exists(DIAGNOSTICS_LOG_PATH)) {
    File f = LittleFS.open(DIAGNOSTICS_LOG_PATH, "r");
    server.streamFile(f, "text/plain");
    f.close();
  } else {
    server.send(404, "text/plain", "No diagnostics log found.");
  }
}
