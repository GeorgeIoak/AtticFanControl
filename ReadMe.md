# 🌀 Attic Fan Controller (ESP8266 + Web UI + OTA)

**Minimum Requirements:**

- ESP8266 board (NodeMCU, Wemos D1 Mini, etc.)
- [ESP8266 core v3.1.2 or newer](http://arduino.esp8266.com/stable/package_esp8266com_index.json) for Arduino IDE
- [Python 3](https://www.python.org/downloads/)
- [mklittlefs](https://github.com/earlephilhower/mklittlefs/releases)
- [esptool.py](https://docs.espressif.com/projects/esptool/en/latest/)

This project lets you control an attic fan using a web interface hosted on an ESP8266.

## Table of Contents

- [🌀 Attic Fan Controller (ESP8266 + Web UI + OTA)](#-attic-fan-controller-esp8266--web-ui--ota)
  - [Table of Contents](#table-of-contents)
  - [✨ Features](#-features)
    - [Required Libraries](#required-libraries)
    - [Software Setup](#software-setup)
    - [Wi-Fi Credentials](#wi-fi-credentials)
  - [🖥️ Web UI Options](#️-web-ui-options)
  - [🚀 Deploying the Filesystem UI](#-deploying-the-filesystem-ui)
    - [Step 1: Build the Filesystem Image](#step-1-build-the-filesystem-image)
    - [Step 2: Change the Firmware Flag](#step-2-change-the-firmware-flag)
    - [Step 3: Upload Files to the Device](#step-3-upload-files-to-the-device)
      - [Method A: OTA Upload (Recommended for Developers)](#method-a-ota-upload-recommended-for-developers)
      - [Method B: OTA Upload (Web UI Only)](#method-b-ota-upload-web-ui-only)
      - [Method C: Wired Upload (Initial Flash or Recovery)](#method-c-wired-upload-initial-flash-or-recovery)
  - [API Reference](#api-reference)
  - [🏠 Indoor Sensor Integration](#-indoor-sensor-integration)
    - [Features](#features)
    - [API Endpoints](#api-endpoints)
    - [Indoor Sensor Setup](#indoor-sensor-setup)
    - [Example Indoor Sensor Configuration](#example-indoor-sensor-configuration)
    - [Integration with Fan Logic](#integration-with-fan-logic)

## ✨ Features

  - Real-time sensor data display for attic, outdoor, and **indoor sensors** (with modal view and averages).
  - Auto/Manual modes with dedicated controls.
  - Animated fan icon that spins to show the fan's state.
  - A configuration form to adjust temperature thresholds and advanced options on-the-fly.
  - 3-day weather forecast from Open-Meteo to inform fan logic.
  - Historical data chart: visualize attic/outdoor temperature and humidity trends from the device log.
  - Downloadable CSV log for offline analysis.
  - Integrated help system with hover tooltips and a dedicated help page.
  - **Test Panel**: Simulate sensor values and test automation logic directly from the web UI.
  - **System & Maintenance**: Firmware update, diagnostics log download/clear, device restart, and config reset from the web UI.
- **Robust Automation:**
  - Automatic fan control based on attic and outdoor temperatures.
  - Automatic fan control based on attic, outdoor, and (optionally) indoor sensor data.
  - Configurable hysteresis to prevent rapid on/off cycling.
  - Smart pre-cooling logic that uses the daily weather forecast to run the fan earlier on hot days.
  - Optional daily automatic restart to ensure long-term stability (delayed until fan cycle completes).
  - Resilient sensor logic that handles temporary hardware failures.
  - Timeout management for inactive indoor sensors.
  - Onboard LED provides at-a-glance status: Off, Solid On, or Blinking (in hysteresis range).
- **Advanced Connectivity:**
  - Non-blocking WiFi connection with progressive backoff retries.
  - Automatic fallback to Access Point (AP) mode if WiFi connection fails, allowing for configuration and recovery.
  - mDNS support for easy access via a local hostname (e.g., `http://AtticFan.local`).
  - Non-blocking WiFi connection with progressive backoff retries.
  - Automatic fallback to Access Point (AP) mode if WiFi connection fails, allowing for configuration and recovery.
  - mDNS support for easy access via a local hostname (e.g., `http://AtticFan.local`).
  - **OTA firmware and filesystem updates** via web UI or Arduino IDE.
  - **MQTT integration** with Home Assistant auto-discovery (main fan, mode, all sensors, and all indoor sensors).
  - Optional Test Panel in the UI to simulate sensor values and test fan logic.
  - History chart and CSV log can be tested without hardware.
  - **Test Panel**: Simulate sensor values and test fan logic without hardware.
  - History chart and CSV log can be tested without hardware.
  - **Persistent diagnostics log** (`/diagnostics.log`) records errors and warnings for easy troubleshooting.
  - Download and clear diagnostics log from the web UI.

```text
AtticFanControl/                  # Main project folder
├── AtticFanControl.ino           # Main Arduino sketch for the fan controller
├── webui_embedded.h              # Embedded web UI (if USE_FS_WEBUI is 0)
├── help_page.h                   # Embedded help page (if USE_FS_WEBUI is 0)
├── types.h                       # Shared type definitions (e.g., FanMode)
├── config.h                      # EEPROM configuration management
├── secrets.h                     # Wi-Fi credentials (excluded from repo)
├── sensors.h                     # Sensor logic
├── hardware.h                    # Hardware config and flags
├── IndoorSensorClient/           # --- SEPARATE SKETCH for the Indoor Sensor Node ---
│   ├── secrets_example.h         # Example credentials file
│   ├── secrets.h                 # WiFi credentials for the sensor node (gitignored)
│   └── IndoorSensorClient.ino    # Code to be flashed onto the indoor sensor ESP8266
├── data/                     # Filesystem folder for FS upload
│   └── index.html            # Place your custom UI here
│   └── help.html             # Place your custom help page here

├── ReadMe.md                 # This file
├── embed_html.py             # Script to embed HTML/JS/CSS/binary files as C headers for ESP8266/ESP32
├── manage_ui.py              # Script to automate embedding, watching, and building the filesystem
├── test_indoor_sensors.py    # Script to test the indoor sensor API endpoints
└── .gitignore                # Prevents secrets from being committed
---

## Project Scripts & Utilities

- **embed_html.py**: Embeds HTML, JS, CSS, or binary files (e.g., images) as C header files for use in ESP8266/ESP32 firmware. Supports both plain and gzipped output, and can generate HTTP handler helpers. Used internally by `manage_ui.py`.
  - Example usage:
    ```sh
    python3 embed_html.py data/index.html webui_embedded.h --var-name EMBEDDED_WEBUI --func-name handleEmbeddedWebUI
    ```

- **manage_ui.py**: Automates embedding of all web UI files and assets. Can update headers, watch for changes, and build the LittleFS filesystem image.
  - Usage:
    - `python3 manage_ui.py update` — Regenerate all embedded headers
    - `python3 manage_ui.py watch` — Watch for changes and auto-update headers
    - `python3 manage_ui.py buildfs` — Build the `filesystem.bin` image from `/data`

- **test_indoor_sensors.py**: Simulates indoor sensor devices by sending test data to the controller's REST API. Useful for development and integration testing.
  - Usage:
    ```sh
    python3 test_indoor_sensors.py [controller_ip]
    ```
    If no IP is provided, defaults to `192.168.1.100`.
```

---

### Required Libraries

Install these libraries via Arduino IDE > Tools > Manage Libraries:

| Library Name        | Author / Source      | Purpose                                 |
|---------------------|---------------------|------------------------------------------|
| [ElegantOTA](https://github.com/ayushsharma82/ElegantOTA)          | Ayush Sharma        | Web-based firmware updates               |
| [DFRobot_SHT20](https://github.com/DFRobot/DFRobot_SHT20)       | DFRobot             | Read temperature & humidity from SHT20/SHT21 |
| [OneWire](https://github.com/PaulStoffregen/OneWire)             | Paul Stoffregen     | Communicate with DS18B20                 |
| [ArduinoJson](https://arduinojson.org/)         | Benoit Blanchon     | Efficient JSON handling                  |
| [PubSubClient](https://github.com/knolleary/pubsubclient)      | Nick O'Leary        | MQTT client for integration              |
| [DallasTemperature](https://github.com/milesburton/Arduino-Temperature-Control-Library)   | Miles Burton        | High-level DS18B20 interface             |

### Software Setup

- Arduino IDE with ESP8266 board support  
- Add this URL to **Arduino > Preferences > Additional Board Manager URLs**:

<http://arduino.esp8266.com/stable/package_esp8266com_index.json>

- Use ESP8266 core v3.1.2 or newer for full OTA compatibility

- In **Tools > Flash Size**, select your board's memory size. For most NodeMCU/Wemos boards, this is **4MB**.
- For the partition scheme, choose **`4MB (FS:2MB, OTA:~1019KB)`**. This is often the default and provides 2MB for the data log filesystem, which is ideal for long-term history.
  - *Note: Older ESP8266 cores might name this `4M (1M LittleFS)`. If you choose that, you must adjust the `-s` parameter in the `mklittlefs` command below to `1024000`.*

### Wi-Fi Credentials

Create a `secrets.h` file in the sketch directory with the following content. This file stores your sensitive information and is ignored by version control.

```cpp
#pragma once
// WiFi Network
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// OTA Update Credentials
const char* ota_user = "admin"; // Default username for OTA updates
const char* ota_password = "your_secure_password"; // Default password for OTA updates

// --- MQTT Configuration (optional) ---
const char* mqtt_broker = "YOUR_MQTT_BROKER_IP";
const int mqtt_port = 1883;
const char* mqtt_user = "YOUR_MQTT_USER";
const char* mqtt_password = "YOUR_MQTT_PASSWORD";
```

Add `secrets.h` to your `.gitignore` file to ensure it is not committed:

```
secrets.h
```

---

## 🖥️ Web UI Options

**Default:** Uses the embedded web UI (`webui_embedded.h`).

**Custom UI:** Serve your own `index.html` from the ESP8266 filesystem (LittleFS).

**How to switch:**

1. In `AttticFanControl.ino`, set:

 ```cpp
 #define USE_FS_WEBUI 1
 ```

2. Place your `index.html` and `help.html` files in the `data/` folder.
3. Upload the filesystem:

- Use the command-line method described in the next section. **Note:** The old Arduino IDE 1.x filesystem plugin is deprecated and not supported in modern IDEs (2.x and newer).

4. On reboot, the device will serve `index.html` if present.

If `USE_FS_WEBUI` is set to `0`, the embedded UI will be used.

---

## 🚀 Deploying the Filesystem UI

This project can serve a rich web interface from its onboard flash memory (LittleFS). This is required for features like the history chart.

**The Golden Rule:** The filesystem (`filesystem.bin`) must be uploaded to the device *before* the firmware that uses it (`USE_FS_WEBUI = 1`).

> **Note on OTA Credentials:**
>
> - **Arduino IDE (Network Port):** Prompts for a **password only**. Use the `ota_password` from your `secrets.h` file.
> - **Web UI (ElegantOTA):** Prompts for a **username and password**. Use `ota_user` and `ota_password` from `secrets.h`.

### Step 1: Build the Filesystem Image

No matter which upload method you choose, you must first create the filesystem binary. From a terminal in the project's root directory, run:

```bash
mklittlefs -c data -b 4096 -p 256 -s 2048000 ./filesystem.bin
```

This packs the contents of the `data/` folder into a `filesystem.bin` file.

### Step 2: Change the Firmware Flag

In `AtticFanControl.ino`, change the flag to `1` to tell the firmware to use the files from the filesystem:

```cpp
#define USE_FS_WEBUI 1
```

### Step 3: Upload Files to the Device

Choose one of the following methods to upload the filesystem and the new firmware.

#### Method A: OTA Upload (Recommended for Developers)

This is the fastest method if you are actively developing.

1. **Upload Filesystem via Web:** Navigate to the device's web UI and go to the `/update` page. Click the **"Filesystem"** button, choose the `filesystem.bin` file, and upload it. The device will reboot.
2. **Upload Firmware via IDE:**
    - In the Arduino IDE, go to **Tools > Port**. You should see your device listed as a network port (e.g., `AtticFan at 192.168.1.123`).
    - Select this network port.
    - Click the regular "Upload" button in the IDE. Arduino will compile and upload the new firmware over the air.

> **Troubleshooting IDE OTA:** If the Arduino IDE fails with an "Authentication Failed" error and doesn't prompt for a password, it has likely cached incorrect credentials.
>
> 1. **First, try restarting the IDE.** Completely quit and reopen the Arduino IDE. This is the most common fix, as it clears the IDE's temporary cache and will prompt for the password again on the next upload attempt.
> 2. **If restarting fails, change the hostname as a last resort.** This is a more forceful way to clear the cache. Change the `MDNS_HOSTNAME` in `hardware.h` (e.g., from `"AtticFan"` to `"AtticFanController"`), upload this change via USB one last time, and restart the IDE. This forces the IDE to discover the device as "new" and re-authenticate.

#### Method B: OTA Upload (Web UI Only)

This method is useful for updating a device without needing the Arduino IDE.

1. **Export the Firmware:** In the Arduino IDE (with `USE_FS_WEBUI = 1`), compile and export the firmware binary using **Sketch > Export Compiled Binary**. This will create a `.bin` file in your sketch folder.
2. **Upload Filesystem:** Navigate to the device's web UI and go to the `/update` page. Click the **"Filesystem"** button, choose the `filesystem.bin` file, and upload it. The device will reboot.
3. **Upload Firmware:** After it reconnects, return to the `/update` page. Click the **"Firmware"** button, choose the new firmware `.bin` file you exported, and upload it.

#### Method C: Wired Upload (Initial Flash or Recovery)

Use this method for the very first time you program a blank device.

1. Connect the device to your computer via USB.
2. **Upload Filesystem:** Upload the filesystem image using `esptool.py`. Replace `COM3` with your device's serial port. The flash address `0x200000` is the correct starting address for the filesystem on a 4MB board.

    ```bash
    esptool.py --port COM3 write_flash 0x200000 filesystem.bin
    ```

3. **Upload Firmware:** Use the Arduino IDE to upload the sketch normally via the USB serial port.

---

## API Reference

The controller exposes several API endpoints for programmatic control, integration, and diagnostics.

- **`GET /status`**
  - Returns a JSON object with the current state of all sensors, the fan, and the controller mode.
  - *Example Response:* `{ "firmwareVersion": "0.95", "atticTemp": "92.1", ..., "fanOn": true, "fanMode": "MANUAL", "fanSubMode": "TIMED", "timerActive": true, ... }`

- **`GET /config`**
  - Returns a JSON object with all current configuration settings.
  - *Example Response:* `{ "fanOnTemp": 90, "fanDeltaTemp": 5, "preCoolingEnabled": true, ... }`

- **`POST /config`**
  - Sets new configuration values. The body must be a JSON object containing one or more keys from the `GET /config` response.
  - *Example with curl:* `curl -X POST -d '{"fanOnTemp":92.5}' http://<ip>/config`

- **`GET /weather`**
  - Returns a JSON object with the current weather conditions and a 3-day forecast.
  - *Example Response:* `{ "currentTemp": "75.3", "currentIcon": "☀️", "forecast": [...] }`

- **`GET /fan?state=on|off|auto|manual|ping`**
  - Provides basic fan and mode control:
    - `on`: Turns the fan on and enters MANUAL mode.
    - `off`: Turns the fan off and enters MANUAL mode.
    - `auto`: Switches the controller to AUTO mode.
    - `manual`: Switches to MANUAL mode (preserves current fan state).
    - `ping`: Returns `pong` (for connectivity testing).

- **`POST /fan`**
  - Starts a manual timed run.
  - *Example Body:* `{ "action": "start_timed", "delay": 5, "duration": 60, "postAction": "revert_to_auto" }`

- **`GET /history.csv`**
  - Downloads the complete sensor history log as a CSV file.

- **`GET /diagnostics`**
  - Downloads the persistent diagnostics log as plain text.

- **`POST /clear_diagnostics`**
  - Clears the persistent diagnostics log file.

- **`GET /restart`**
  - Triggers a software restart of the device.

- **`GET /reset_config`**
  - Resets all configuration to defaults and restarts the device.

- **`GET /help`**
  - Returns the help page (HTML).

- **`GET /update_wrapper`**
  - Returns a wrapper page for the OTA update UI (HTML with iframe).

- **Test/Development Endpoints:**
  - **`POST /test/set_temps?attic=...&outdoor=...`**: Set simulated sensor values (test mode only).
  - **`POST /test/force_ap`**: Force the device into Access Point (AP) mode for WiFi recovery.

See the Indoor Sensor Integration section for `/indoor_sensors` API endpoints.

----

## 🏠 Indoor Sensor Integration


The Attic Fan Controller supports multiple ESP8266-based indoor sensors that report temperature and humidity to the main controller via HTTP POST requests. Indoor sensors are auto-discovered and displayed in the web UI, and can be integrated with Home Assistant via MQTT auto-discovery.

### Features

- **Multiple Sensor Support**: Register up to 10 indoor sensors, each with a unique ID and name.
- **Automatic Registration & Discovery**: Sensors register themselves automatically when they send data; no manual setup required on the controller.
- **Web UI Integration**: Indoor sensor data (average and per-room) is displayed in the main controller's dashboard, with a modal for details.
- **Timeout Management**: Inactive sensors are automatically removed after 30 minutes.
- **Data Validation**: Sensor values are checked for reasonable ranges before being accepted.
- **REST API**: Full API for submitting, listing, and removing sensors and their data.
- **MQTT & Home Assistant Integration**: Indoor sensor data is published to MQTT (if enabled), with Home Assistant auto-discovery for all sensors.

### API Endpoints

- **`POST /indoor_sensors/data`**: Submit sensor data
  - Required JSON fields: `sensorId`, `name`, `temperature` (°F), `humidity` (%)
  - Example: `{ "sensorId": "sensor1", "name": "Living Room", "temperature": 72.5, "humidity": 45.2 }`

- **`GET /indoor_sensors`**: Get all registered sensors and their data
  - Returns a list of sensors, count, and average temperature/humidity

- **`DELETE /indoor_sensors/{sensorId}`**: Remove a specific sensor by ID

### Indoor Sensor Setup

1. **Hardware**: Use an ESP8266 board (NodeMCU, Wemos D1 Mini, etc.) with a supported temperature/humidity sensor (SHT21, SHT20, or BME280) connected via I2C.

2. **Software**: Open the `IndoorSensorClient/IndoorSensorClient.ino` sketch in the Arduino IDE (separate from the main controller).

  - Copy `secrets_example.h` to `secrets.h` and enter your WiFi credentials.
  - Set a unique `SENSOR_ID` and `SENSOR_NAME` in the sketch for each device.
  - By default, the sensor will auto-discover the main controller using mDNS (`AtticFan.local`). If mDNS fails, set `FALLBACK_CONTROLLER_IP` in the sketch.
  - Upload the sketch to your ESP8266 indoor sensor.

3. **Configuration**:

  - Sensors register automatically when they send data—no manual steps needed on the controller.
  - Enable or disable indoor sensor support in the main controller's config (web UI > config panel).

### Example Indoor Sensor Configuration

```cpp
// In IndoorSensorClient.ino
const String SENSOR_ID = "bedroom_01";        // Unique identifier
const String SENSOR_NAME = "Master Bedroom";  // Display name
const unsigned long POST_INTERVAL = 30000;    // Send every 30 seconds
```


### Integration with Fan Logic

Currently, indoor sensor data is collected, displayed, and published to MQTT/Home Assistant. Future firmware updates may use indoor sensor data for advanced automation, such as:

- Whole-house fan control based on indoor/outdoor temperature differential
- Humidity-based ventilation control
- Zone-specific climate monitoring
- Smart scheduling based on occupancy patterns

---

🧠 Potential Future Plans

- ~~Add an indoor temperature sensor for more advanced climate control logic (e.g., whole-house fan).~~ ✅ **Implemented**
- Advanced fan control logic using indoor sensor data
- Sensor discovery and auto-configuration protocol
- Mobile app for sensor management

- Always test with low-voltage loads before switching high-power fans.
- Use proper isolation and protection when working with AC
- Secure your device if exposed to public networks
