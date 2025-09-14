#pragma once
#include <pgmspace.h>

// Auto-generated from index.html. Do not edit by hand.
// To define the chunked handler automatically, keep WEBUI_EMIT_STREAM_HELPER=1
#ifndef WEBUI_EMIT_STREAM_HELPER
#define WEBUI_EMIT_STREAM_HELPER 1
#endif

#undef F
const char EMBEDDED_WEBUI[] PROGMEM = R"EMB1(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Attic Fan Control</title>
  <link rel="stylesheet" href="atticfan.css">
  <link rel="icon" type="image/png" href="favicon.png">
  <link rel="icon" href="favicon.ico">
</head>
<body>
<div class="page-container">
  <h1>🏠 Attic Fan Control</h1>
  <div class="top-layout-grid">
    <div class="sensor-column">
      <div class="sensor-item">
        <strong>🌤️ Outdoor</strong>
        <div class="sensor-value"><span id="outdoorTemp">--</span>°F</div>
      </div>
      <div class="sensor-item">
        <strong>🌡️ Attic</strong>
        <div class="sensor-value"><span id="atticTemp">--</span>°F / <span id="atticHumidity">--</span>%</div>
      </div>
      <div class="sensor-item" id="indoorSensorDisplay" style="display: none;">
        <strong>🏠 Indoor (Avg) <a href="#" onclick="openIndoorSensorsModal(event)" class="help-link" title="Click to see all indoor sensors" aria-label="Help on indoor sensors">(+)</a></strong>
        <div class="sensor-value"><span id="avgIndoorTemp">--</span>°F / <span id="avgIndoorHumidity">--</span>%</div>
      </div>
      <div class="sensor-item" id="indoorSensor1" style="display: none;">
        <strong id="indoorSensor1Name">--</strong>
        <div class="sensor-value"><span id="indoorSensor1Temp">--</span>°F / <span id="indoorSensor1Humidity">--</span>%</div>
      </div>
      <div class="sensor-item" id="indoorSensor2" style="display: none;">
        <strong id="indoorSensor2Name">--</strong>
        <div class="sensor-value"><span id="indoorSensor2Temp">--</span>°F / <span id="indoorSensor2Humidity">--</span>%</div>
      </div>
    </div>
    <div class="control-column">
      <div class="control-group" id="mode-control-box">
  <h3>⚙️ Mode Control <a href="help.html#mode-control" class="help-link" title="Click for detailed help on modes" aria-label="Help on mode control">(?)</a></h3>
        <div class="mode-buttons-container">
          <button id="autoModeBtn" class="section-save-btn" onclick="setMode('AUTO')">AUTO</button>
          <button id="manualModeBtn" class="section-save-btn" onclick="setMode('MANUAL')">MANUAL</button>
        </div>
        <div class="fan-status-container">
          <div id="fan-icon-container">
            <svg id="fan-icon" class="fan-icon" viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg">
              <path d="M50,15 A35,35 0 0,1 85,50 L50,50 Z" fill="white" />
              <path d="M50,15 A35,35 0 0,0 15,50 L50,50 Z" transform="rotate(120 50 50)" fill="white" />
              <path d="M50,15 A35,35 0 0,0 15,50 L50,50 Z" transform="rotate(240 50 50)" fill="white" />
              <circle cx="50" cy="50" r="8" fill="rgba(255,255,255,0.8)"/>
            </svg>
          </div>
          <div class="fan-text-container">
            <p><span class="status-indicator" id="statusIndicator"></span>Mode: <span id="currentMode">--</span></p>
            <p>Fan is <span id="fanState">OFF</span></p>
            <p id="timerStatus"></p>
          </div>
        </div>
        <div>
          <button id="manualOnBtn" class="section-save-btn" onclick="setFan('on')" disabled>Turn ON</button>
          <button id="manualOffBtn" class="section-save-btn" onclick="setFan('off')" disabled>Turn OFF</button>
          <button id="setTimerBtn" class="section-save-btn" onclick="openTimerModal()" disabled>Set Timer</button>
        </div>
      </div>
    </div>
  </div>
  <section class="content-section">
  <h2>📊 Weather Forecast <a href="help.html#pre-cooling" class="help-link" title="Click for help on weather and pre-cooling" aria-label="Help on weather and pre-cooling">(?)</a></h2>
    <p>Current: <span id="currentWeatherIcon"></span> <span id="currentWeatherTemp">--</span>°F / <span id="currentWeatherHumidity">--</span>%</p>
    <div class="weather-forecast" id="forecastContainer"></div>
  </section>
  <section class="content-section">
  <h2>📈 History <a href="help.html#history" class="help-link" title="Click for help on history chart and CSV log" aria-label="Help on history chart and CSV log">(?)</a></h2>
    <div class="chart-container">
      <canvas id="historyChart"></canvas>
      <div id="historyChartMsg"></div>
    </div>
  </section>
  <section id="configForm" class="content-section">
  <h2>🎛️ Fan Temperature Control Settings <a href="help.html#temp-settings" class="help-link" title="Click for detailed help on temperature settings" aria-label="Help on temperature settings">(?)</a></h2>
    <p class="unit-note">(All temperatures in Fahrenheit °F)</p>
    <div class="settings-container">
      <div class="settings-group">
        <h4 class="config-subheading">Core Automation</h4>
        <div class="config-grid">
          <div class="config-item">
            <label for="fanOnTemp" title="The attic temperature at which the fan will consider turning on.">On Temp</label>
            <div class="number-input-wrapper"><input type="number" id="fanOnTemp" step="0.5" value="90.0"></div>
          </div>
          <div class="config-item">
            <label for="fanDeltaTemp" title="Fan only runs if attic is this much hotter than outside. Prevents pulling in hot air.">Delta Temp</label>
            <div class="number-input-wrapper"><input type="number" id="fanDeltaTemp" step="0.5" value="5.0"></div>
          </div>
          <div class="config-item">
            <label for="fanHysteresis" title="Prevents rapid on/off cycling. Fan turns off at (On Temp - Hysteresis).">Hysteresis</label>
            <div class="number-input-wrapper"><input type="number" id="fanHysteresis" step="0.5" value="2.0"></div>
          </div>
          <div class="save-cell">
            <button class="section-save-btn" onclick="saveCoreAutomation()">💾 Save Core Automation</button>
          </div>
        </div>
      </div>
      <div class="settings-group">
        <h4 class="config-subheading">Smart Pre-Cooling</h4>
        <div class="config-grid">
          <div class="config-item">
            <label for="preCoolTriggerTemp" title="If forecast high is at or above this, pre-cooling activates.">Pre-Cool Trigger</label>
            <div class="number-input-wrapper"><input type="number" id="preCoolTriggerTemp" step="0.5" value="90.0"></div>
          </div>
          <div class="config-item">
            <label for="preCoolTempOffset" title="Lowers the 'On Temp' by this amount when pre-cooling is active.">Pre-Cool Offset</label>
            <div class="number-input-wrapper"><input type="number" id="preCoolTempOffset" step="0.5" value="5.0"></div>
          </div>
          <div class="config-item">
            <label for="historyLogInterval" title="How often to log sensor history (in minutes).">History Log Interval</label>
            <div class="number-input-wrapper"><input type="number" id="historyLogInterval" min="1" max="1440" step="1" required value="5"></div>
          </div>
          <div class="save-cell">
            <button class="section-save-btn" onclick="saveSmartPreCooling()">💾 Save Smart Pre-Cooling</button>
          </div>
        </div>
      </div>
    </div>
  </section>
  <section class="content-section" id="feature-toggles">
  <h2>Feature Toggles <a class="help-link" href="help.html#feature-toggles" target="_blank" rel="noopener">(?)</a></h2>
    <div class="config-grid">
      <div class="config-item"><label for="preCoolingEnabled">Enable Pre-Cooling</label><label class="switch"><input type="checkbox" id="preCoolingEnabled"><span class="slider"></span></label></div>
      <div class="config-item"><label for="onboardLedEnabled">Enable Status LED</label><label class="switch"><input type="checkbox" id="onboardLedEnabled"><span class="slider"></span></label></div>
      <div class="config-item"><label for="dailyRestartEnabled">Enable Daily Restart</label><label class="switch"><input type="checkbox" id="dailyRestartEnabled"><span class="slider"></span></label></div>
      <div class="config-item"><label for="mqttEnabled">Enable MQTT Broker</label><label class="switch"><input type="checkbox" id="mqttEnabled"><span class="slider"></span></label></div>
      <div class="config-item"><label for="mqttDiscoveryEnabled">Publish HA Discovery</label><label class="switch"><input type="checkbox" id="mqttDiscoveryEnabled"><span class="slider"></span></label></div>
      <div class="config-item"><label for="testModeEnabled">Enable Test Mode</label><label class="switch"><input type="checkbox" id="testModeEnabled"><span class="slider"></span></label></div>
    </div>
  </section>
  <section id="test-panel" class="content-section">
  <h2>🧪 Test Panel <a href="help.html#feature-toggles" class="help-link" title="Click for help on test mode" aria-label="Help on test mode">(?)</a></h2>
      <h4 class="config-subheading">Simulated Temps</h4>
      <div class="config-grid">
        <div class="config-item">
          <label for="simulatedAtticTemp">Attic: <span id="attic-temp-val">--</span>°F</label>
          <input type="range" id="simulatedAtticTemp" min="50" max="150" step="1" oninput="updateTempDisplay('attic-temp-val', this.value); setSliderInteraction(true)">
        </div>
        <div class="config-item">
          <label for="simulatedOutdoorTemp">Outdoor: <span id="outdoor-temp-val">--</span>°F</label>
          <input type="range" id="simulatedOutdoorTemp" min="30" max="120" step="1" oninput="updateTempDisplay('outdoor-temp-val', this.value); setSliderInteraction(true)">
        </div>
      </div>
      <div class="test-panel-actions">
        <button onclick="updateTestTemps()" class="section-save-btn">Update Temps</button>
      </div>
      </section>
  <section class="content-section" id="ap-mode">
  <h2>Access Point (AP) Mode <a href="help.html#troubleshooting" class="help-link" title="Click for help on AP mode and WiFi recovery" aria-label="Help on AP mode and WiFi recovery">(?)</a></h2>
    <p>Put device into AP Mode to change WiFi credentials:</p>
    <button onclick="forceAPMode()" class="section-save-btn btn-danger">Force AP Mode</button>
  </section>
<section class="content-section">
  <h2>System & Maintenance <a href="help.html#api-reference" class="help-link" title="Click for help on system and API" aria-label="Help on system and API">(?)</a></h2>
      <div class="system-controls">
        <button onclick="window.location.href='/update_wrapper'" class="section-save-btn">Firmware Update</button>
        <a href="/diagnostics" class="section-save-btn" download>Download Diagnostics</a>
        <button onclick="clearDiagnostics()" class="section-save-btn">Clear Diagnostics</button>
        <a href="/history.csv" class="section-save-btn" download>Download History (CSV)</a>
        <div class="danger-row">
          <button onclick="restartDevice()" class="section-save-btn btn-danger">Restart Device</button>
          <button onclick="resetConfig()" class="section-save-btn btn-danger">Reset to Defaults</button>
        </div>
      </div>
    </section>

<footer class="app-footer">Firmware: <span id="firmwareVersion">--</span></footer>
</div>

<!-- Non-visual elements -->
<div id="saveToast" class="toast"></div>
<div id="timerModal" class="modal">
  <div class="modal-content">
    <span class="close-btn" onclick="closeTimerModal()">&times;</span>
    <h3>Set Manual Timer</h3>
    <div class="config-grid">
      <div class="config-item"><label for="timerDelay">Delay Start (mins)</label><div class="number-input-wrapper"><input type="number" id="timerDelay" min="0" step="1" value="0"></div></div>
      <div class="config-item"><label for="timerDuration">Run Duration (mins)</label><div class="number-input-wrapper"><input type="number" id="timerDuration" min="1" step="1" value="30"></div></div>
    </div>
    <div class="timer-options"><label for="postTimerAction">After timer ends:</label>
      <select id="postTimerAction">
        <option value="go_auto">Switch to AUTO</option>
        <option value="stay_manual">Stay MANUAL (Fan Off)</option>
      </select>
    </div>
    <div class="timer-actions"><button class="section-save-btn" onclick="startTimedRun()">Start Timed Run</button></div>
  </div>
</div>

<!-- Indoor Sensors Modal -->
<div id="indoorSensorsModal" class="modal">
  <div class="modal-content">
    <span class="close-btn" onclick="closeIndoorSensorsModal()">&times;</span>
    <h3>All Indoor Sensors</h3>
    <div id="indoorSensorsList"></div>
  </div>
</div>

<!-- Scripts -->
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<script src="atticfan.js" defer></script>
</body>
</html>)EMB1";
const size_t EMBEDDED_WEBUI_LEN = sizeof(EMBEDDED_WEBUI) - 1;

#if WEBUI_EMIT_STREAM_HELPER
// NOTE: Assumes you have a global 'ESP8266WebServer server(80);'
// If your instance is named differently, set WEBUI_EMIT_STREAM_HELPER=0
// and paste a custom handler in your route file. 
#include <ESP8266WebServer.h>                                                   
static void handleEmbeddedWebUI() {
  extern ESP8266WebServer server;                                              
  server.sendHeader("Connection", "close");                                   
  server.send_P(200, "text/html",                                               
                EMBEDDED_WEBUI,
                sizeof(EMBEDDED_WEBUI) - 1);
}
#endif

#define F(string_literal) (FPSTR(PSTR(string_literal)))
