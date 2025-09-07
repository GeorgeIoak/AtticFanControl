#pragma once
#include <LittleFS.h>

#define DIAGNOSTICS_LOG_PATH "/diagnostics.log"

inline void logDiagnostics(const char* msg) {
  File f = LittleFS.open(DIAGNOSTICS_LOG_PATH, "a");
  if (f) {
    f.println(msg);
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
