#include "web_endpoints.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ElegantOTA.h>
#include <ArduinoOTA.h>
#include "hardware.h"
#include "LittleFS.h"
#include "sensors.h"
#include "secrets.h"
#include "config.h"
#include "weather.h"

#include "mqtt_handler.h"

#include "types.h"
#include "history.h"
#include "diagnostics.h"

#define USE_FS_WEBUI 0 // Set to 1 to use index.html from FS

ESP8266WebServer server(80);
DNSServer dnsServer;

const unsigned long SENSOR_UPDATE_INTERVAL_MS = 5000; // ms
FanMode fanMode = AUTO;

// Non-blocking timer for sensor reads
unsigned long lastSensorRead = 0;

// Non-blocking timer for history logging
unsigned long lastHistoryLog = 0;

// Non-blocking timer for WiFi retries
unsigned long lastWifiRetry = 0;
unsigned long wifiRetryInterval = WIFI_INITIAL_RETRY_DELAY_MS;

// Non-blocking timer for daily restart
unsigned long lastDailyRestartCheck = 0;

// State tracking for network mode
bool staConnectionFailed = false;
bool apModeActive = false;
bool dailyRestartPending = false;
bool wifiWasConnected = false;

// Test Mode State Variables (always declared, only used if test mode is enabled)
float simulatedAtticTemp = MOCK_ATTIC_TEMP;
float simulatedOutdoorTemp = MOCK_OUTDOOR_TEMP;
float simulatedAtticHumidity = 50.0; // A reasonable default for humidity

// === Timer State ===
ManualTimerState manualTimer;

/**
 * @brief Sets the fan relay and onboard LED to a specific state.
 * @param fanOn True to turn the fan ON, false to turn it OFF.
 * @note This handles the inverted logic for the onboard LED.
 */
void setFanState(bool fanOn) {
  digitalWrite(FAN_RELAY_PIN, fanOn ? HIGH : LOW); // This function now ONLY controls the fan.
}

/**
 * @brief Starts the manual timer with specified delay and duration.
 */
void startManualTimer(unsigned long delayMinutes, unsigned long durationMinutes, PostTimerAction action) {
  manualTimer.isActive = true;
  manualTimer.postAction = action;
  manualTimer.delayEndTime = millis() + (delayMinutes * 60000UL);
  manualTimer.timerEndTime = manualTimer.delayEndTime + (durationMinutes * 60000UL);

  // If there's no delay, turn the fan on immediately.
  if (delayMinutes == 0) {
    setFanState(true);
  }
  #if DEBUG_SERIAL
  Serial.printf("[INFO] Manual timer started. Delay: %lu min, Duration: %lu min.\n", delayMinutes, durationMinutes);
  #endif
}

/**
 * @brief Cancels any active manual timer.
 */
void cancelManualTimer() {
  if (manualTimer.isActive) {
    manualTimer.isActive = false;
    Serial.println("[INFO] Manual timer cancelled.");
  }
}

void setup() {
  Serial.begin(115200);
  loadConfig(); // Load settings from EEPROM
  initSensors();
  initMqtt(); // Initialize MQTT client
  pinMode(FAN_RELAY_PIN, OUTPUT);
  pinMode(ONBOARD_LED_PIN, OUTPUT);
  analogWriteRange(255); // Set PWM range to 0-255 for LED breathing effect
  setFanState(false); // Default OFF

  // Mount the filesystem

  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS. Formatting...");
    logDiagnostics("[ERROR] Failed to mount LittleFS. Formatting...");
    LittleFS.format();
  }

  // Schedule the first history log to occur sooner than the full interval.
  // This ensures the history file is created promptly after boot for the UI chart.
  lastHistoryLog = millis() - config.historyLogIntervalMs + 15000UL; // Log in ~15 seconds

  // --- Non-Blocking WiFi Setup ---
#if USE_STATIC_IP
  // Apply static IP configuration if enabled
  if (!WiFi.config(staticIP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Failed to configure static IP!");
  }
#endif
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Initiated WiFi connection...");

  server.on("/", [](){ handleRoot(server); });
  server.on("/help", [](){ handleHelp(server); });
  server.on("/fan", HTTP_ANY, [&](){ handleFan(server, fanMode); });
  server.on("/status", [&](){ handleStatus(server, fanMode); });
  server.on("/config", HTTP_GET, [](){ handleGetConfig(server); });
  server.on("/config", HTTP_POST, [](){ handleSetConfig(server); });
  // Conditionally register test endpoints based on runtime config
  if (config.testModeEnabled) {
    server.on("/test/set_temps", [](){ handleSetTestTemps(server); });
    server.on("/test/force_ap", [](){ handleForceAP(server); });
  }
  server.on("/weather", HTTP_GET, [&]() { handleWeather(server); });
  server.on("/history.csv", HTTP_GET, [](){ handleHistoryDownload(server); });
  server.on("/restart", [](){ handleRestart(server); });
  server.on("/diagnostics", [](){ handleDiagnosticsDownload(server); });
  server.on("/update_wrapper", HTTP_GET, [](){ handleUpdateWrapper(server); });
  ElegantOTA.begin(&server, ota_user, ota_password); // Pass username and password

  // --- Arduino IDE OTA Setup ---
  // This is separate from ElegantOTA and handles uploads directly from the IDE.
  ArduinoOTA.setHostname(MDNS_HOSTNAME);
  ArduinoOTA.setPassword(ota_password); // Uses the password from secrets.h

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
      LittleFS.end(); // Unmount FS before updating
    }
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
  });
  ArduinoOTA.begin();

  server.begin();
  Serial.println("Web server started");
}

/**
 * @brief Manages the WiFi connection state, attempting to reconnect if disconnected.
 */
void handleWiFiConnection() {
  // Do nothing if we are already in AP mode
  if (apModeActive) {
    return;
  }

  // --- Handle Connection Success ---
  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiWasConnected) {
      // This block runs ONCE when the state changes to connected.
      Serial.println("\nWiFi connected!");
      Serial.println("IP address: " + WiFi.localIP().toString());
      
      // Reset the retry backoff timer for any future disconnections
      wifiRetryInterval = WIFI_INITIAL_RETRY_DELAY_MS;
      
      if (MDNS.begin(MDNS_HOSTNAME)) {
        Serial.printf("mDNS responder started. Access at http://%s.local\n", MDNS_HOSTNAME);
        // Explicitly advertise the Arduino OTA service. This is what the IDE looks for.
        MDNS.addService("arduino", "tcp", 8266);
        MDNS.addServiceTxt("arduino", "tcp", "board", "esp8266");
      }
      
      // Trigger an immediate weather update on first connect
      #if DEBUG_SERIAL
      Serial.println("[INFO] WiFi connected, triggering initial weather update.");
      #endif
      lastWeatherUpdate = 0;
      
      Serial.println("----------------------------------------");
      
      wifiWasConnected = true;
      staConnectionFailed = false; // We are connected, so we haven't failed.
    }
    // If we are here, WiFi is already connected and reported. Do nothing.
    return; // Exit the function since we are connected.
  }

  // --- Handle Disconnection and Retries ---
  // If we reach here, WiFi is not connected.
  wifiWasConnected = false; // Mark that we are disconnected.

  if (!staConnectionFailed) { // Only try to reconnect if we haven't given up yet.
    if (millis() - lastWifiRetry >= wifiRetryInterval) {
      Serial.printf("WiFi disconnected. Retrying connection (interval: %lu s)...\n", wifiRetryInterval / 1000);
      WiFi.begin(ssid, password); // Re-trigger connection attempt
      lastWifiRetry = millis();
      
      wifiRetryInterval *= WIFI_RETRY_BACKOFF_FACTOR;
      if (wifiRetryInterval > WIFI_MAX_RETRY_DELAY_MS) {
        wifiRetryInterval = WIFI_MAX_RETRY_DELAY_MS;
        Serial.println("Final WiFi connection attempt failed. Will switch to AP mode if enabled.");
        staConnectionFailed = true;
      }
    }
  }

  // --- Handle AP Fallback ---
  if (staConnectionFailed) {
#if AP_FALLBACK_ENABLED
    Serial.printf("Entering AP mode. SSID: %s, Password: %s\n", AP_SSID, AP_PASSWORD);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    dnsServer.start(53, "*", WiFi.softAPIP()); // Start DNS server for captive portal
    if (MDNS.begin(MDNS_HOSTNAME)) {
      Serial.printf("mDNS responder started for AP mode. Access at http://%s.local\n", MDNS_HOSTNAME);
    }
    apModeActive = true;
#endif
  }
}

/**
 * @brief Manages the state of the manual timer.
 */
void handleManualTimer() {
  if (!manualTimer.isActive) return;

  unsigned long now = millis();
  bool fanIsOn = (digitalRead(FAN_RELAY_PIN) == HIGH);

  // Check if delay period is over and fan is not yet on
  if (now >= manualTimer.delayEndTime && !fanIsOn && now < manualTimer.timerEndTime) {
    #if DEBUG_SERIAL
    Serial.println("[INFO] Timer delay finished. Turning fan ON.");
    #endif
    setFanState(true);
  }

  // Check if the total timer duration has expired
  if (now >= manualTimer.timerEndTime) {
    #if DEBUG_SERIAL
    Serial.println("[INFO] Timed run finished.");
    #endif
    setFanState(false); // Always turn fan off

    if (manualTimer.postAction == REVERT_TO_AUTO) {
      fanMode = AUTO;
      #if DEBUG_SERIAL
      Serial.println("[INFO] Reverting to AUTO mode.");
      #endif
    } else {
      fanMode = MANUAL_OFF;
    }
    manualTimer.isActive = false; // Deactivate timer
  }
}

/**
 * @brief Centralized handler for the onboard status LED.
 * This function determines the LED's behavior based on system state.
 * It should be called on every loop cycle.
 */
void updateStatusLED() {
  static unsigned long lastBlinkTime = 0;

  // If the LED is disabled via config, ensure it's off and exit.
  if (!config.onboardLedEnabled) {
    digitalWrite(ONBOARD_LED_PIN, HIGH); // HIGH is OFF
    return;
  }

  // Priority 1: Fast blink if WiFi is disconnected and not in AP mode.
  if (WiFi.status() != WL_CONNECTED && !apModeActive) {
    if (millis() - lastBlinkTime > 150) { // Fast blink (150ms)
      digitalWrite(ONBOARD_LED_PIN, !digitalRead(ONBOARD_LED_PIN));
      lastBlinkTime = millis();
    }
    return; // This state takes precedence
  }

  // Priority 2: "Breathing" pulse if in AP mode.
  if (apModeActive) {
    // Create a sine wave for a smooth breathing effect
    float breath = (exp(sin(millis()/2000.0*PI)) - 0.36787944)*108.0;
    analogWrite(ONBOARD_LED_PIN, 255 - breath); // analogWrite works on D4 for ESP8266
    return; // This state takes precedence
  }

  // Priority 3: Slow blink if fan is ON and in the hysteresis temperature zone.
  if (fanMode == AUTO && digitalRead(FAN_RELAY_PIN) == HIGH) {
    float atticTemp = readAtticTemp();
    float effectiveFanOnTemp = config.fanOnTemp - (config.preCoolingEnabled && currentWeather.isValid && forecast[0].tempMax >= config.preCoolTriggerTemp ? config.preCoolTempOffset : 0);
    float turnOffTemp = effectiveFanOnTemp - config.fanHysteresis;

    if (atticTemp < effectiveFanOnTemp && atticTemp >= turnOffTemp) {
      if (millis() - lastBlinkTime > 500) { // Slow blink (500ms)
        digitalWrite(ONBOARD_LED_PIN, !digitalRead(ONBOARD_LED_PIN));
        lastBlinkTime = millis();
      }
      return; // This state takes precedence
    }
  }

  // Default: Solid ON/OFF matching the fan state.
  bool fanIsOn = (digitalRead(FAN_RELAY_PIN) == HIGH);
  digitalWrite(ONBOARD_LED_PIN, fanIsOn ? LOW : HIGH); // Inverted logic: LOW is ON
}

/**
 * @brief Checks if a daily restart is due and performs it safely.
 * This helps ensure long-term stability by resetting the device once every 24 hours.
 * If the fan is running when the restart is due, it will wait until the fan turns off.
 */
void handleDailyRestart() {
  if (!config.dailyRestartEnabled) return;

  // 1. If a restart is pending, check if it's safe to proceed.
  if (dailyRestartPending) {
    bool fanIsOn = (digitalRead(FAN_RELAY_PIN) == HIGH);
    if (!fanIsOn) {
      #if DEBUG_SERIAL
      Serial.println("[INFO] Fan is off. Proceeding with scheduled daily restart.");
      #endif
      ESP.restart(); // It's safe to restart now.
    }
    #if DEBUG_SERIAL
    else {
      Serial.println("[INFO] Daily restart is pending, but fan is running. Deferring restart.");
    }
    #endif
    // If fan is on, we do nothing and wait for the next loop cycle to check again.
    return;
  }

  // 2. Check if it's time to schedule a restart (check roughly every hour).
  // This check is less frequent to avoid constant evaluation of millis().
  if (millis() - lastDailyRestartCheck > 3600000UL) { // 1 hour
    lastDailyRestartCheck = millis();
    if (millis() > 86400000UL) { // 24 hours in milliseconds
      #if DEBUG_SERIAL
      Serial.println("[INFO] 24-hour mark reached. A daily restart is now pending, will execute when fan is off.");
      #endif
      dailyRestartPending = true;
      // The actual restart is now handled by the logic at the top of this function.
    }
  }
}

void loop() {
  // Handle IDE-based OTA updates.
  ArduinoOTA.handle();

  // Manage WiFi connection state first to ensure network is ready for other tasks.
  handleWiFiConnection();

  // Handle MQTT connection and messages
  handleMqtt();

  // Fetch weather data periodically (the function handles its own timing)
  updateWeatherData();

  // Handle manual timer logic
  handleManualTimer();

  // Update the status LED on every loop cycle for responsiveness.
  updateStatusLED();

  // Check if a daily restart is needed for long-term stability.
  handleDailyRestart();

  // Only run fan logic at the specified interval
  if (millis() - lastSensorRead >= SENSOR_UPDATE_INTERVAL_MS) {
    lastSensorRead = millis();

    // Always read sensors to keep data fresh for the UI
    float atticTemp = readAtticTemp();
    float atticHumidity = readAtticHumidity();
    float outdoorTemp = readOutdoorTemp();

    #if DEBUG_SERIAL
    const char* modeStr;
    switch (fanMode) {
      case MANUAL_ON:
      case MANUAL_OFF:
      case MANUAL_TIMED:
        modeStr = "MANUAL";
        break;
      default:
        modeStr = "AUTO";
    }
    Serial.printf("Attic: %.1f°F, %.1f%% RH | Outdoor: %.1f°F | Mode: %s\n",
                  atticTemp, atticHumidity, outdoorTemp, modeStr);
    #endif

    // Only apply auto logic if in AUTO mode
    if (fanMode == AUTO) {
      bool fanIsOn = (digitalRead(FAN_RELAY_PIN) == HIGH);
      float effectiveFanOnTemp = config.fanOnTemp; // Start with the base temp

      // Check if pre-cooling is active based on today's forecast
      // Ensure weather data is valid and the forecast high is available
      if (config.preCoolingEnabled && currentWeather.isValid && forecast[0].tempMax >= config.preCoolTriggerTemp) {
        effectiveFanOnTemp -= config.preCoolTempOffset;
        #if DEBUG_SERIAL
        Serial.printf("[INFO] Pre-cooling active. Effective ON temp: %.1f°F\n", effectiveFanOnTemp);
        #endif
      }
      float turnOffTemp = effectiveFanOnTemp - config.fanHysteresis;
      
      // Conditions to turn the fan ON
      bool turnOnCondition = (atticTemp > effectiveFanOnTemp) && ((atticTemp - outdoorTemp) >= config.fanDeltaTemp);
      
      // Condition to turn the fan OFF
      bool turnOffCondition = (atticTemp < turnOffTemp) || ((atticTemp - outdoorTemp) < config.fanDeltaTemp);

      if (fanIsOn && turnOffCondition) {
        setFanState(false);
        #if DEBUG_SERIAL
          Serial.println("Fan turned OFF by auto logic (hysteresis/delta).");
        #endif
      } else if (!fanIsOn && turnOnCondition) {
        setFanState(true);
        #if DEBUG_SERIAL
          Serial.println("Fan turned ON by auto logic.");
        #endif
      }
    }

    // --- Periodic CSV Logging ---
    if (millis() - lastHistoryLog >= config.historyLogIntervalMs) {
      lastHistoryLog = millis();
      bool fanIsOn = (digitalRead(FAN_RELAY_PIN) == HIGH);
      // Use weather timestamp if available, otherwise the log function will handle the null case.
      const char* timestamp = (currentWeather.isValid && strlen(currentWeather.timeString) > 0) ? currentWeather.timeString : nullptr;
      appendHistoryLog(timestamp, atticTemp, outdoorTemp, atticHumidity, fanIsOn);
      #if DEBUG_SERIAL
      Serial.printf("[LOG] History: %s, %.2f, %.2f, %.2f, %d\n", timestamp ? timestamp : "N/A", atticTemp, outdoorTemp, atticHumidity, fanIsOn ? 1 : 0);
      #endif
    }
  }

  // Handle DNS requests when in AP mode
  if (apModeActive) {
    dnsServer.processNextRequest();
  }
  // Handle mDNS queries
  MDNS.update();
  server.handleClient();
}
