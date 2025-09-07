#pragma once

#if !USE_FS_WEBUI

const char HELP_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Attic Fan - Help</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; line-height: 1.6; margin: 0; padding: 0; background-color: #f8f9fa; }
    .page-container { max-width: 800px; margin: 20px auto; padding: 20px; background-color: #fff; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
    h1, h2 { color: #333; border-bottom: 1px solid #eee; padding-bottom: 10px; }
    h1 { text-align: center; }
    section { margin-bottom: 30px; }
    code { background-color: #e9ecef; padding: 2px 5px; border-radius: 4px; word-wrap: break-word; }
    dt { font-weight: bold; color: #0056b3; }
    dd { margin-left: 20px; margin-bottom: 10px; }
    a { color: #007bff; text-decoration: none; }
    a:hover { text-decoration: underline; }
    .back-link { display: inline-block; margin-bottom: 20px; }
  </style>
</head>
<body>
  <div class="page-container">
    <h1>Help & Documentation</h1>
    <a href="/" class="back-link">&larr; Back to Main Control Page</a>

    <section id="mode-control">
      <h2>Mode Control</h2>
      <p>This panel controls the primary operational mode of the fan.</p>
      <dl>
        <dt>AUTO</dt>
        <dd>This is the primary "set and forget" mode. The fan will automatically turn on and off based on the temperature settings and pre-cooling logic.</dd>
        <dt>MANUAL</dt>
        <dd>This mode gives you direct control over the fan. In this mode, you can turn the fan on, off, or start a timed run. The automatic logic is disabled.</dd>
      </dl>
    </section>

    <section id="manual-timer">
      <h2>Manual Timer</h2>
      <p>The "Set Timer" button (available in MANUAL mode) allows you to run the fan for a specific duration, with an optional delayed start.</p>
      <dl>
        <dt>Delay Start</dt>
        <dd>The number of minutes to wait before the fan turns on.</dd>
        <dt>Run Duration</dt>
        <dd>The number of minutes the fan will run after the delay period ends.</dd>
        <dt>After timer ends</dt>
        <dd>Choose what the controller should do after the timed run is complete. It can either revert to AUTO mode or stay in MANUAL mode with the fan off.</dd>
      </dl>
    </section>

    <section id="temp-settings">
      <h2>Fan Temp Control Settings</h2>
      <p>These settings fine-tune the automatic fan behavior.</p>
      <dl>
        <dt>On Temp</dt>
        <dd>The primary attic temperature at which the fan is allowed to turn on, provided other conditions are met.</dd>
        <dt>Delta Temp</dt>
        <dd>A crucial efficiency setting. The fan will only <strong>turn on and stay on</strong> if the attic is hotter than the outdoors by at least this many degrees. This ensures the fan is always pulling in significantly cooler air and will turn it off if the outdoor temperature rises too close to the attic temperature.</dd>
        <dt>Hysteresis</dt>
        <dd>This creates a "turn-off" temperature that is lower than the "turn-on" temperature, preventing the fan from rapidly switching on and off (known as "cycling") when the attic temperature is hovering around the setpoint. For example, if 'On Temp' is 90°F and 'Hysteresis' is 2°F, the fan will turn on at 90°F but will not turn off until the attic cools down to 88°F.</dd>
        <dt>History Log Interval</dt>
        <dd>How often (in minutes) to log sensor history to the device. Default is 5 minutes. Range: 1–1440 (24 hours). Lower values provide finer-grained history but use more storage.</dd>
      </dl>
    </section>

    <section id="pre-cooling">
      <h2>Pre-Cooling Logic</h2>
      <p>Pre-cooling is a smart feature that uses the daily weather forecast to get a head start on cooling the attic on very hot days.</p>
      <dl>
        <dt>Pre-Cool Trigger</dt>
        <dd>If the forecasted high for the day is at or above this temperature, the pre-cooling logic will activate.</dd>
        <dt>Pre-Cool Offset</dt>
        <dd>When pre-cooling is active, the <code>On Temp</code> is temporarily lowered by this amount. For example, if On Temp is 90°F and the Offset is 5°F, the fan will start at 85°F on hot days, helping to prevent the attic from reaching extreme temperatures.</dd>
      </dl>
    </section>

    <section id="feature-toggles">
      <h2>Feature Toggles</h2>
      <p>These switches control optional features of the controller.</p>
      <dl>
        <dt>Enable Pre-Cooling</dt>
        <dd>The master switch to turn the entire pre-cooling feature on or off.</dd>
        <dt>Enable Status LED</dt>
        <dd>Controls the onboard status light on the device. You may wish to turn this off if the blinking light is in a visible location.</dd>
        <dt>Enable Daily Restart</dt>
        <dd>Automatically restarts the device once every 24 hours. This is a best-practice feature to ensure long-term stability. To avoid interrupting a cooling cycle, the restart will be safely delayed until after the fan has turned off.</dd>
        <dt>Enable MQTT</dt>
        <dd>Enables the MQTT client, which allows the controller to integrate with Home Assistant or other MQTT-compatible smart home systems. Requires MQTT broker details to be set in <code>secrets.h</code>.</dd>
        <dt>Enable Test Mode</dt>
        <dd>Enables the "Test Panel" at the bottom of the main page, which contains sliders to simulate sensor values. This allows you to test the fan's automation logic without needing physical sensors or specific environmental conditions.</dd>
      </dl>
    </section>
    <section id="history">
      <h2>History Chart & CSV Log</h2>
  <p>The <strong>History</strong> section on the main page shows a chart of attic temperature, outdoor temperature, and humidity over time. This data is logged at a configurable interval (default: 5 minutes) and stored in a CSV file on the device. You can adjust the logging interval in the configuration section for finer or coarser history detail.</p>
      <ul>
        <li><strong>Chart:</strong> Visualizes recent trends (up to 100 points) for quick analysis. The purple shaded area at the bottom indicates when the fan was on.</li>
        <li><strong>Download:</strong> Click <em>Download History (CSV)</em> in the System & Maintenance section to save the complete log for offline use.</li>
      </ul>
      <p>The CSV format is: <code>timestamp,attic_temp,outdoor_temp,humidity,fan_on</code></p>
    </section>
    <section id="troubleshooting">
      <h2>Troubleshooting & FAQ</h2>
      <dl>
        <dt>Why do I see a simpler UI after flashing?</dt>
        <dd>The device defaults to a lightweight, embedded UI. To use the full-featured UI (with history chart, etc.), you must set <code>USE_FS_WEBUI</code> to <code>1</code> in the main sketch and upload the `data` folder to the device's filesystem.</dd>
        <dt>How do I recover WiFi if the device fails to connect?</dt>
        <dd>The device will fall back to AP mode. Connect to the "AtticFanSetup" WiFi network and use the web page to reconfigure your WiFi credentials.</dd>
        <dt>How do I update the firmware?</dt>
        <dd>Use the "Firmware Update" link in the System & Maintenance section of the web UI.</dd>
      </dl>
    </section>

    <section id="api-reference">
      <h2>API Reference</h2>
      <p>The controller exposes several API endpoints for programmatic control and integration.</p>
      <dl>
        <dt><code>GET /status</code></dt>
        <dd>Returns a JSON object with the current state of all sensors, the fan, and the controller mode. <br><i>Example Response:</i><br><code>{"firmwareVersion":"0.95","atticTemp":"92.1","fanOn":true,"fanMode":"MANUAL","fanSubMode":"TIMED","timerActive":true,...}</code></dd>

        <dt><code>GET /config</code></dt>
        <dd>Returns a JSON object with all current configuration settings.<br><i>Example Response:</i><br><code>{"fanOnTemp":90,"fanDeltaTemp":5,"preCoolingEnabled":true,...}</code></dd>

        <dt><code>POST /config</code></dt>
        <dd>Sets new configuration values. The body must be a JSON object containing one or more of the keys from the GET response. The device will respond with a message indicating whether a restart is required.<br><i>Example with curl:</i><br><code>curl -X POST -d '{"fanOnTemp":92.5}' http://&lt;ip&gt;/config</code></dd>

        <dt><code>GET /weather</code></dt>
        <dd>Returns a JSON object with the current weather conditions and a 3-day forecast.<br><i>Example Response:</i><br><code>{"currentTemp":"75.3","currentIcon":"☀️","forecast":[...]}</code></dd>

        <dt><code>GET /fan?state=...</code></dt>
        <dd>Provides basic control.
          <ul>
            <li><code>/fan?state=on</code>: Turns the fan on and enters MANUAL mode.</li>
            <li><code>/fan?state=off</code>: Turns the fan off and enters MANUAL mode.</li>
            <li><code>/fan?state=auto</code>: Switches the controller to AUTO mode.</li>
          </ul>
          <i>Example with curl:</i><br><code>curl http://&lt;ip&gt;/fan?state=on</code>
        </dd>

        <dt><code>POST /fan</code></dt>
        <dd>Starts a manual timed run. The body must be a JSON object like the one below.<br><i>Example with curl:</i><br><code>curl -X POST -d '{"action":"start_timed", "duration":30}' http://&lt;ip&gt;/fan</code></dd>
        <dt><code>GET /history.csv</code></dt>
        <dd>Downloads the complete sensor history log as a CSV file.</dd>
        <dt><code>GET /restart</code></dt>
        <dd>Triggers a software restart of the device.</dd>
        <dt><code>GET /diagnostics</code></dt>
        <dd>Downloads the persistent diagnostics log as plain text. This log records device errors and warnings, such as sensor failures and file system issues.</dd>
      </dl>
    </section>
  </div>
</body>
</html>
)rawliteral";

#endif // !USE_FS_WEBUI