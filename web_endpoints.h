#pragma once

#include <DNSServer.h>
#include <LittleFS.h>
// Web server endpoint handlers (inline for Arduino compatibility)
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include "history.h"
#include "sensors.h"
#include "webui_embedded.h"
#include "help_page.h"
#include "config.h"
#include "weather.h"
#include "types.h"

extern void logDiagnostics(const char* msg);
extern ESP8266WebServer server;

// To access the global timer state from the main .ino file
extern ManualTimerState manualTimer;

// Path for the diagnostics log file, consistent with documentation
#define DIAGNOSTICS_LOG_PATH "/diagnostics.log"

// Forward declarations
void reinitMqtt();

// Endpoint to download the CSV history log
inline void handleHistoryDownload(ESP8266WebServer &server) {
  if (LittleFS.exists(HISTORY_LOG_PATH)) {
    File f = LittleFS.open(HISTORY_LOG_PATH, "r");
    server.streamFile(f, "text/csv");
    f.close();
  } else {
    server.send(404, "text/plain", "No history log found.");
  }
}

/**
 * @brief Handles requests to clear the diagnostics log file.
 */
inline void handleClearDiagnostics(ESP8266WebServer &server) {
  if (LittleFS.exists(DIAGNOSTICS_LOG_PATH)) {
    if (LittleFS.remove(DIAGNOSTICS_LOG_PATH)) {
      logDiagnostics("[INFO] Diagnostics log cleared by user.");
      server.send(200, "text/plain", "Diagnostics log cleared successfully.");
    } else {
      server.send(500, "text/plain", "Failed to clear diagnostics log.");
    }
  } else {
    server.send(200, "text/plain", "Diagnostics log was already empty.");
  }
}

inline void handleHelp(ESP8266WebServer &server) {
#if USE_FS_WEBUI
  File f = LittleFS.open("/help.html", "r");
  if (f) {
    server.streamFile(f, "text/html");
    f.close();
  } else {
    server.send(404, "text/plain", "help.html not found on filesystem.");
  }
#else
  server.send_P(200, "text/html", HELP_PAGE, sizeof(HELP_PAGE) - 1);
#endif
}

// Forward declare the helper function from the main .ino file
extern void setFanState(bool fanOn);
extern void startManualTimer(unsigned long delayMinutes, unsigned long durationMinutes, PostTimerAction action);
extern void cancelManualTimer();

inline void handleFan(ESP8266WebServer &server, FanMode &fanMode) {
  if (server.method() == HTTP_POST) {
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (error) {
      server.send(400, "text/plain", "Invalid JSON");
      return;
    }
    const char* action = doc["action"];
    if (strcmp(action, "start_timed") == 0) {
      unsigned long delay = doc["delay"];
      unsigned long duration = doc["duration"];
      const char* postActionStr = doc["postAction"];
      
      logDiagnostics("[ACTION] Manual timer started via web UI.");
      PostTimerAction postAction = (strcmp(postActionStr, "revert_to_auto") == 0) ? REVERT_TO_AUTO : STAY_MANUAL;
      
      startManualTimer(delay, duration, postAction);
      fanMode = MANUAL_TIMED;
      server.send(200, "text/plain", "Timer started");
    } else {
      server.send(400, "text/plain", "Unknown action");
    }
  } else { // Handle simple GET requests
    String state = server.arg("state");
    if (state == "on") {
      logDiagnostics("[ACTION] Fan turned ON manually via web UI.");
      cancelManualTimer();
      fanMode = MANUAL_ON;
      setFanState(true);
      server.send(200, "text/plain", "ON");
    } else if (state == "ping") {
      server.send(200, "text/plain", "pong");
    } else if (state == "auto") {
      logDiagnostics("[ACTION] Mode changed to AUTO via web UI.");
      cancelManualTimer();
      fanMode = AUTO;
      server.send(200, "text/plain", "Mode set to AUTO");
    } else if (state == "off") {
      logDiagnostics("[ACTION] Fan turned OFF manually via web UI.");
      cancelManualTimer();
      fanMode = MANUAL_OFF;
      setFanState(false);
      server.send(200, "text/plain", "OFF");
    } else {
      logDiagnostics("[WARN] Invalid state received in handleFan.");
      server.send(400, "text/plain", "Invalid state");
    }
  }
}

inline void handleStatus(ESP8266WebServer &server, FanMode fanMode) {
  float atticTemp = readAtticTemp();
  float atticHumidity = readAtticHumidity();
  float outdoorTemp = readOutdoorTemp();
  bool fanOn = digitalRead(FAN_RELAY_PIN) == HIGH;

  StaticJsonDocument<512> doc;
  doc["firmwareVersion"] = FIRMWARE_VERSION;
  doc["atticTemp"] = serialized(String(atticTemp, 1));
  doc["atticHumidity"] = serialized(String(atticHumidity, 1));
  doc["outdoorTemp"] = serialized(String(outdoorTemp, 1));
  doc["fanOn"] = fanOn;

  // Convert FanMode enum to strings for JSON
  switch (fanMode) {
    case MANUAL_ON:
      doc["fanMode"] = "MANUAL";
      doc["fanSubMode"] = "ON"; // e.g. Manually turned on
      break;
    case MANUAL_OFF:
      doc["fanMode"] = "MANUAL";
      doc["fanSubMode"] = "OFF"; // e.g. Manually turned off
      break;
    case MANUAL_TIMED:
      doc["fanMode"] = "MANUAL";
      doc["fanSubMode"] = "TIMED"; // Timer is active
      break;
    default: // AUTO
      doc["fanMode"] = "AUTO";
  }

  // Add timer status
  if (manualTimer.isActive) {
    doc["timerActive"] = true;
    unsigned long now = millis();
    if (now < manualTimer.delayEndTime) {
      doc["timerMode"] = "delay";
      doc["timerRemainingSec"] = (manualTimer.delayEndTime - now) / 1000;
    } else {
      doc["timerMode"] = "run";
      doc["timerRemainingSec"] = (manualTimer.timerEndTime - now) / 1000;
    }
  } else {
    doc["timerActive"] = false;
  }

  doc["testModeEnabled"] = config.testModeEnabled;
  if (config.testModeEnabled) {
    extern float simulatedAtticTemp;
    extern float simulatedOutdoorTemp;
    extern float simulatedAtticHumidity;
    doc["simulatedAtticTemp"] = simulatedAtticTemp;
    doc["simulatedOutdoorTemp"] = simulatedOutdoorTemp;
    doc["simulatedAtticHumidity"] = simulatedAtticHumidity;
  }
  server.send(200, "application/json", doc.as<String>());
}

/**
 * @brief Logs the reason for a restart and then restarts the device.
 * @param reason The reason for the restart, to be logged.
 */
inline void logAndRestart(const char* reason) {
  logDiagnostics(reason);
  delay(100); // Short delay to allow log to write
  ESP.restart();
}

inline void handleRestart(ESP8266WebServer &server) {
  server.send(200, "text/plain", "Restarting...");
  logAndRestart("[RESTART] Manual restart from web UI.");
}

inline void handleResetConfig(ESP8266WebServer &server) {
  setResetFlag();
  server.send(200, "text/plain", "Configuration reset. Restarting...");
  logAndRestart("[RESTART] Configuration reset from web UI.");
}

inline void handleGetConfig(ESP8266WebServer &server) {
  StaticJsonDocument<512> doc;
  doc["fanOnTemp"] = config.fanOnTemp;
  doc["fanDeltaTemp"] = config.fanDeltaTemp;
  doc["fanHysteresis"] = config.fanHysteresis;
  doc["preCoolTriggerTemp"] = config.preCoolTriggerTemp;
  doc["preCoolTempOffset"] = config.preCoolTempOffset;
  doc["preCoolingEnabled"] = config.preCoolingEnabled;
  doc["onboardLedEnabled"] = config.onboardLedEnabled;
  doc["testModeEnabled"] = config.testModeEnabled;
  doc["dailyRestartEnabled"] = config.dailyRestartEnabled;
  doc["mqttEnabled"] = config.mqttEnabled;
  doc["mqttDiscoveryEnabled"] = config.mqttDiscoveryEnabled;
  doc["historyLogIntervalMs"] = config.historyLogIntervalMs;
  server.send(200, "application/json", doc.as<String>());
}

inline void handleSetConfig(ESP8266WebServer &server) {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Bad Request");
    return;
  }

  bool mqttWasEnabled = config.mqttEnabled;
  bool testModeWasEnabled = config.testModeEnabled;
  StaticJsonDocument<512> doc; // Increased size for new field
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error) {
    server.send(400, "text/plain", "Invalid JSON");
    return;
  }

  // Log the incoming changes to the diagnostics file
  String logMessage = "[CONFIG] Web UI change: ";
  bool first = true;
  for (JsonPair kv : doc.as<JsonObject>()) {
    if (!first) {
      logMessage += ", ";
    }
    logMessage += kv.key().c_str();
    logMessage += "=";
    logMessage += kv.value().as<String>();
    first = false;
  }
  logDiagnostics(logMessage.c_str());


  // Update config struct with new values if they exist in the JSON
  if (doc.containsKey("fanOnTemp")) {
    config.fanOnTemp = doc["fanOnTemp"];
  }
  if (doc.containsKey("fanDeltaTemp")) {
    config.fanDeltaTemp = doc["fanDeltaTemp"];
  }
  if (doc.containsKey("fanHysteresis")) {
    config.fanHysteresis = doc["fanHysteresis"];
  }
  if (doc.containsKey("preCoolTriggerTemp")) {
    config.preCoolTriggerTemp = doc["preCoolTriggerTemp"];
  }
  if (doc.containsKey("preCoolTempOffset")) {
    config.preCoolTempOffset = doc["preCoolTempOffset"];
  }
  if (doc.containsKey("preCoolingEnabled")) {
    config.preCoolingEnabled = doc["preCoolingEnabled"];
  }
  if (doc.containsKey("onboardLedEnabled")) {
    config.onboardLedEnabled = doc["onboardLedEnabled"];
  }
  if (doc.containsKey("testModeEnabled")) {
    config.testModeEnabled = doc["testModeEnabled"];
  }
  if (doc.containsKey("dailyRestartEnabled")) {
    config.dailyRestartEnabled = doc["dailyRestartEnabled"];
  }
  if (doc.containsKey("mqttEnabled")) {
    config.mqttEnabled = doc["mqttEnabled"];
  }
  if (doc.containsKey("mqttDiscoveryEnabled")) {
    config.mqttDiscoveryEnabled = doc["mqttDiscoveryEnabled"];
  }
  if (doc.containsKey("historyLogIntervalMs")) {
    config.historyLogIntervalMs = doc["historyLogIntervalMs"];
  }

  bool mqttIsNowEnabled = config.mqttEnabled;
  bool testModeIsNowEnabled = config.testModeEnabled;
  saveConfig(); // Persist the new settings

  if (mqttWasEnabled != mqttIsNowEnabled) {
    reinitMqtt();
  }

  if (testModeWasEnabled != testModeIsNowEnabled) {
    server.send(200, "text/plain", "Configuration saved. A restart is required to apply Test Mode changes.");
  } else {
    server.send(200, "text/plain", "Configuration saved. Changes will apply on the next cycle.");
  }
}

// Forward declarations for test mode
extern float simulatedAtticTemp;
extern float simulatedOutdoorTemp;
extern bool apModeActive;
extern DNSServer dnsServer;

/**
 * @brief Test endpoint to manually set simulated sensor temperatures.
 */
inline void handleSetTestTemps(ESP8266WebServer &server) {
    if (server.hasArg("attic")) {
        simulatedAtticTemp = server.arg("attic").toFloat();
    }
    if (server.hasArg("outdoor")) {
        simulatedOutdoorTemp = server.arg("outdoor").toFloat();
    }
    server.send(200, "text/plain", "Test temperatures updated.");
}

/**
 * @brief Test endpoint to force the device into AP mode.
 */
inline void handleForceAP(ESP8266WebServer &server) {
    #if DEBUG_SERIAL
    logSerial("[TEST] Forcing AP mode via web request...");
    #endif
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    dnsServer.start(53, "*", WiFi.softAPIP());
    apModeActive = true;
    server.send(200, "text/plain", "AP Mode Forced. Connect to 'AtticFanSetup'.");
}

/**
 * @brief Serves a wrapper page for the OTA update UI.
 * This provides a consistent header with a "back" link, embedding the
 * ElegantOTA page inside an iframe.
 */
inline void handleUpdateWrapper(ESP8266WebServer &server) {
  const char* update_page_wrapper = R"rawliteral(
<!DOCTYPE html><html><head><title>Firmware Update</title><meta name="viewport" content="width=device-width, initial-scale=1"><style>body{margin:0;font-family:Arial,sans-serif}.header{background-color:#333;padding:15px;text-align:center}.header a{color:white;text-decoration:none;font-size:1.2em}iframe{border:none;width:100%;height:calc(100vh - 55px)}</style></head><body><div class="header"><a href="/">&larr; Back to Main Control Page</a></div><iframe src="/update"></iframe></body></html>
)rawliteral";
  server.send(200, "text/html", update_page_wrapper);
}