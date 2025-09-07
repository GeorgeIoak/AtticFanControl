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
  - [📦 Project Structure](#-project-structure)
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

## ✨ Features

- **Intuitive Web Interface:**
  - Real-time sensor data display for attic and outdoor conditions.
  - Auto/Manual modes with dedicated controls.
  - Animated fan icon that spins to show the fan's state.
  - A configuration form to adjust temperature thresholds on-the-fly.
  - A 3-day weather forecast from Open-Meteo to inform fan logic.
  - Historical data chart: visualize attic/outdoor temperature and humidity trends from the device log.
  - Downloadable CSV log for offline analysis.
  - Integrated help system with hover tooltips and a dedicated help page.
- **Robust Automation:**
  - Automatic fan control based on attic and outdoor temperatures.
  - Configurable hysteresis to prevent rapid on/off cycling.
  - Smart pre-cooling logic that uses the daily weather forecast to run the fan earlier on hot days.
  - Optional daily automatic restart to ensure long-term stability. The restart is safely delayed until any active fan cycle is complete.
  - Resilient sensor logic that handles temporary hardware failures.
- **Hardware Feedback:**
  - Onboard LED provides at-a-glance status: Off, Solid On, or Blinking (in hysteresis range).
- **Advanced Connectivity:**
  - Non-blocking WiFi connection with progressive backoff retries.
  - Automatic fallback to Access Point (AP) mode if WiFi connection fails, allowing for configuration and recovery.
  - mDNS support for easy access via a local hostname (e.g., `http://AtticFan.local`).
- **Testing & Diagnostics:**
  - **MQTT Integration:** Optional MQTT support with Home Assistant auto-discovery for seamless smart home integration.
  - Optional Test Panel in the UI to simulate sensor values and test fan logic.
  - History chart and CSV log can be tested without hardware.
  - A persistent diagnostics log (`/diagnostics.log`) records errors and warnings for easy troubleshooting.

---

## 📦 Project Structure

```text
AndyAtticFanControl/
├── AndyAtticFanControl.ino   # Main Arduino sketch
├── webui_embedded.h          # Embedded web UI (if USE_FS_WEBUI is 0)
├── help_page.h               # Embedded help page (if USE_FS_WEBUI is 0)
├── types.h                   # Shared type definitions (e.g., FanMode)
├── config.h                  # EEPROM configuration management
├── secrets.h                 # Wi-Fi credentials (excluded from repo)
├── sensors.h                 # Sensor logic
├── hardware.h                # Hardware config and flags
├── data/                     # Filesystem folder for FS upload
│   └── index.html            # Place your custom UI here
│   └── help.html             # Place your custom help page here
├── ReadMe.md                 # This file
└── .gitignore                # Prevents secrets from being committed
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

http://arduino.esp8266.com/stable/package_esp8266com_index.json
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
1. In `AndyAttticFanControl.ino`, set:
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
> - **Arduino IDE (Network Port):** Prompts for a **password only**. Use the `ota_password` from your `secrets.h` file.
> - **Web UI (ElegantOTA):** Prompts for a **username and password**. Use `ota_user` and `ota_password` from `secrets.h`.

### Step 1: Build the Filesystem Image

No matter which upload method you choose, you must first create the filesystem binary. From a terminal in the project's root directory, run:
```bash
mklittlefs -c data -b 4096 -p 256 -s 2048000 ./filesystem.bin
```
This packs the contents of the `data/` folder into a `filesystem.bin` file.

### Step 2: Change the Firmware Flag

In `AndyAtticFanControl.ino`, change the flag to `1` to tell the firmware to use the files from the filesystem:
```cpp
#define USE_FS_WEBUI 1
```

### Step 3: Upload Files to the Device

Choose one of the following methods to upload the filesystem and the new firmware.

#### Method A: OTA Upload (Recommended for Developers)

This is the fastest method if you are actively developing.

1.  **Upload Filesystem via Web:** Navigate to the device's web UI and go to the `/update` page. Click the **"Filesystem"** button, choose the `filesystem.bin` file, and upload it. The device will reboot.
2.  **Upload Firmware via IDE:**
    - In the Arduino IDE, go to **Tools > Port**. You should see your device listed as a network port (e.g., `AtticFan at 192.168.1.123`).
    - Select this network port.
    - Click the regular "Upload" button in the IDE. Arduino will compile and upload the new firmware over the air.

> **Troubleshooting IDE OTA:** If the Arduino IDE fails with an "Authentication Failed" error and doesn't prompt for a password, it has likely cached incorrect credentials.
> 1.  **First, try restarting the IDE.** Completely quit and reopen the Arduino IDE. This is the most common fix, as it clears the IDE's temporary cache and will prompt for the password again on the next upload attempt.
> 2.  **If restarting fails, change the hostname as a last resort.** This is a more forceful way to clear the cache. Change the `MDNS_HOSTNAME` in `hardware.h` (e.g., from `"AtticFan"` to `"AtticFanController"`), upload this change via USB one last time, and restart the IDE. This forces the IDE to discover the device as "new" and re-authenticate.

#### Method B: OTA Upload (Web UI Only)

This method is useful for updating a device without needing the Arduino IDE.

1.  **Export the Firmware:** In the Arduino IDE (with `USE_FS_WEBUI = 1`), compile and export the firmware binary using **Sketch > Export Compiled Binary**. This will create a `.bin` file in your sketch folder.
2.  **Upload Filesystem:** Navigate to the device's web UI and go to the `/update` page. Click the **"Filesystem"** button, choose the `filesystem.bin` file, and upload it. The device will reboot.
3.  **Upload Firmware:** After it reconnects, return to the `/update` page. Click the **"Firmware"** button, choose the new firmware `.bin` file you exported, and upload it.

#### Method C: Wired Upload (Initial Flash or Recovery)

Use this method for the very first time you program a blank device.

1.  Connect the device to your computer via USB.
2.  **Upload Filesystem:** Upload the filesystem image using `esptool.py`. Replace `COM3` with your device's serial port. The flash address `0x200000` is the correct starting address for the filesystem on a 4MB board.
    ```bash
    esptool.py --port COM3 write_flash 0x200000 filesystem.bin
    ```
3.  **Upload Firmware:** Use the Arduino IDE to upload the sketch normally via the USB serial port.

---

##  API Reference

The controller exposes several API endpoints for programmatic control and integration.

- **`GET /status`**
  - Returns a JSON object with the current state of all sensors, the fan, and the controller mode.
  - *Example Response:* `{"firmwareVersion":"0.95","atticTemp":"92.1",...,"fanOn":true,"fanMode":"MANUAL","fanSubMode":"TIMED","timerActive":true,...}`

- **`GET /config`**
  - Returns a JSON object with all current configuration settings.
  - *Example Response:* `{"fanOnTemp":90,"fanDeltaTemp":5,"preCoolingEnabled":true,...}`

- **`POST /config`**
  - Sets new configuration values. The body must be a JSON object containing one or more keys from the `GET /config` response.
  - *Example with curl:* `curl -X POST -d '{"fanOnTemp":92.5}' http://<ip>/config`

- **`GET /weather`**
  - Returns a JSON object with the current weather conditions and a 3-day forecast.
  - *Example Response:* `{"currentTemp":"75.3","currentIcon":"☀️","forecast":[...]}`

- **`GET /fan?state=...`**
  - Provides basic control.
    - `/fan?state=on`: Turns the fan on and enters MANUAL mode.
    - `/fan?state=off`: Turns the fan off and enters MANUAL mode.
    - `/fan?state=auto`: Switches the controller to AUTO mode.
  - *Example with curl:* `curl http://<ip>/fan?state=on`

- **`POST /fan`**
  - Starts a manual timed run.
  - *Example Body:* `{"action":"start_timed", "delay":5, "duration":60, "postAction":"revert_to_auto"}`

- **`GET /history.csv`**
  - Downloads the complete sensor history log as a CSV file.

- **`GET /restart`**
  - Triggers a software restart of the device.

- **`GET /diagnostics`**
  - Downloads the persistent diagnostics log as plain text.

---

🧠 Potential Future Plans
- Add an indoor temperature sensor for more advanced climate control logic (e.g., whole-house fan).

- Always test with low-voltage loads before switching high-power fans.
- Use proper isolation and protection when working with AC
- Secure your device if exposed to public networks
