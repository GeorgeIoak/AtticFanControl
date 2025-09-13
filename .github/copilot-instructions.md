# Copilot Instructions for AtticFanControl

## Repository Overview

This is an ESP8266-based attic fan controller that provides intelligent climate control with a web interface. The project creates both a physical controller and web-based dashboard for monitoring and controlling an attic fan based on temperature sensors and weather forecasts.

**Key Technologies:**
- **Platform:** ESP8266 microcontroller (Arduino framework)
- **Language:** C++ for firmware, Python 3 for build tools
- **Web Interface:** HTML/CSS/JavaScript served from LittleFS or embedded in firmware
- **Size:** ~30 source files, medium complexity embedded project
- **Target:** IoT home automation device with OTA updates

## Build Requirements & Setup

### Required Tools (Install Order Matters)
1. **Arduino IDE 2.x or Arduino CLI** with ESP8266 board support
2. **Python 3** (tested with 3.12.3)
3. **mklittlefs** for filesystem builds (from https://github.com/earlephilhower/mklittlefs/releases)

### Essential Setup Steps

**ALWAYS perform these steps in order before any development:**

1. **Configure Arduino IDE:**
   ```
   - Add board manager URL: http://arduino.esp8266.com/stable/package_esp8266com_index.json
   - Install ESP8266 core v3.1.2+
   - Select board: NodeMCU 1.0 or Wemos D1 Mini
   - Flash Size: 4MB (FS:2MB, OTA:~1019KB)
   ```

2. **Create secrets.h file** (REQUIRED - project won't compile without it):
   ```cpp
   #pragma once
   const char* ssid = "YOUR_SSID";
   const char* password = "YOUR_PASSWORD";
   const char* ota_user = "admin";
   const char* ota_password = "your_secure_password";
   const char* mqtt_broker = "YOUR_MQTT_BROKER_IP";
   const int mqtt_port = 1883;
   const char* mqtt_user = "YOUR_MQTT_USER";
   const char* mqtt_password = "YOUR_MQTT_PASSWORD";
   ```

3. **Install Arduino Libraries** (via Library Manager):
   - ElegantOTA (Ayush Sharma)
   - DFRobot_SHT20 (DFRobot)  
   - OneWire (Paul Stoffregen)
   - ArduinoJson (Benoit Blanchon)
   - PubSubClient (Nick O'Leary)
   - DallasTemperature (Miles Burton)

### Build Commands

**Update embedded web assets from data/ folder:**
```bash
python3 manage_ui.py update
```

**Build LittleFS filesystem image:**
```bash
python3 manage_ui.py buildfs
```

**Watch for changes during development:**
```bash
python3 manage_ui.py watch
```

**Arduino compilation:**
- Use Arduino IDE or CLI with standard ESP8266 compile process
- No Makefile or custom build system

### Common Build Issues & Solutions

**"secrets.h not found":** Create the secrets.h file as shown above - it's required for compilation

**"Library not found" errors:** Install all 6 required libraries through Arduino Library Manager

**"mklittlefs command not found":** Download mklittlefs binary for your platform from GitHub releases and add to PATH

**Python script errors:** Ensure Python 3 is installed and accessible as `python3`

**OTA upload failures in Arduino IDE:** Restart the IDE completely to clear cached credentials, or change the hostname in hardware.h

## Project Architecture & Key Files

### Main Source Files
- **AtticFanControl.ino** - Main Arduino sketch with setup() and loop()
- **web_endpoints.h** - HTTP API handlers and web server routes  
- **sensors.h** - Temperature sensor reading and fan control logic
- **config.h** - EEPROM configuration management
- **hardware.h** - GPIO definitions and hardware configuration flags
- **weather.h** - Weather forecast integration via Open-Meteo API
- **mqtt_handler.h** - MQTT integration for Home Assistant

### Generated Files (Don't Edit Manually)
- **webui_embedded.h** - Embedded web UI (generated from data/index.html)
- **help_page.h** - Embedded help page (generated from data/help.html)
- **atticfan_css.h, atticfan_js.h** - Embedded CSS/JS assets
- **favicon_ico.h, favicon_png.h** - Embedded favicon files

### Configuration Files
- **.vscode/settings.json** - VS Code ESP-IDF Python path
- **data/** - Web UI source files (HTML, CSS, JS, icons)
- **filesystem.bin** - Generated LittleFS image for OTA upload

### Build Tools
- **manage_ui.py** - Main build tool for web assets and filesystem
- **embed_html.py** - Embeds web files into C headers with PROGMEM

## Development Workflow

### Web UI Development Mode
**File**: AtticFanControl.ino, line ~21
```cpp
#define USE_FS_WEBUI 0  // Change to 1 for filesystem mode
```
- **0 = Embedded mode:** Uses embedded headers (faster, smaller)
- **1 = Filesystem mode:** Serves from LittleFS (better for UI development)

### Typical Development Cycle
1. **Modify data/ files** (HTML, CSS, JS)
2. **Run:** `python3 manage_ui.py update` (generates new headers)  
3. **Upload code** via Arduino IDE
4. **For filesystem mode:** Also run `python3 manage_ui.py buildfs` and upload filesystem.bin via web UI

### Testing Without Hardware
- Set sensor presence flags in hardware.h to `false`
- Uses mock values: MOCK_ATTIC_TEMP (95.0°F), MOCK_OUTDOOR_TEMP (88.0°F)
- Enable test mode in web UI for sensor simulation

## API Endpoints & Integration

**Key REST API endpoints:**
- `GET /status` - JSON status of all sensors and fan state
- `POST /config` - Update configuration settings
- `GET /fan?state=on|off|auto` - Basic fan control
- `GET /history.csv` - Download sensor history log
- `GET /weather` - Weather forecast data
- `GET /restart` - Trigger device restart

## No CI/CD or Automated Testing
This repository has **no GitHub Actions, automated builds, or test suites**. All validation is manual:
- Flash firmware to hardware and test web interface
- Use browser developer tools for JavaScript debugging  
- Monitor Arduino IDE serial output for diagnostics
- Test OTA updates via web interface

## Hardware Configuration Flags
**Critical settings in hardware.h:**
- `HAS_SHT21` / `HAS_DS18B20` - Enable/disable physical sensors
- Pin assignments for relay, sensors, LED
- Default temperature thresholds and behavior
- WiFi and MQTT settings

## Troubleshooting Common Issues

**WiFi connection failures:** Device falls back to AP mode "AtticFanSetup" for configuration

**Sensor read failures:** Check hardware flags; uses mock values when sensors disabled

**OTA upload issues:** Use correct credentials (username + password for web UI, password only for Arduino IDE)

**Memory issues:** Large embedded assets (~1MB favicons) may cause compilation issues on some ESP8266 variants

**Web UI not loading:** Check USE_FS_WEBUI flag matches your deployment method (embedded vs filesystem)

## Trust These Instructions
These instructions are comprehensive and tested. **Only search for additional information if:**
- Instructions are incomplete for your specific task
- You encounter errors not covered in troubleshooting
- You need to understand internal implementation details not documented here

The build process and architecture are well-defined - follow the documented procedures rather than exploring alternatives.