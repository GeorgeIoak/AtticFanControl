/*
 * ESP8266 Indoor Sensor Client
 * 
 * This sketch runs on an ESP8266 with sensors (SHT21, BME280, etc.) and 
 * periodically sends temperature and humidity data to the main Attic Fan 
 * Controller via HTTP POST requests.
 * 
 * Hardware Requirements:
 * - ESP8266 (NodeMCU, Wemos D1 Mini, etc.)
 * - SHT21 or BME280 temperature/humidity sensor
 * - I2C connections (SDA, SCL)
 * 
 * Configuration:
 * - Update WiFi credentials below
 * - Update ATTIC_FAN_IP to match your main controller's IP address
 * - Update SENSOR_ID to a unique identifier for this sensor
 * - Update SENSOR_NAME to a human-readable name
 */

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <Wire.h>

// Choose your sensor type (uncomment one)
#define USE_SHT21
// #define USE_BME280

#ifdef USE_SHT21
#include "DFRobot_SHT20.h"
#endif

#ifdef USE_BME280
#include <Adafruit_BME280.h>
#endif
#include <ESP8266WiFi.h>
#include "secrets.h" // Include WiFi credentials

// Attic Fan Controller settings
const char* CONTROLLER_MDNS_HOSTNAME = "AtticFan"; // mDNS name of the main controller
const char* FALLBACK_CONTROLLER_IP = "192.168.1.100";  // Fallback IP if mDNS fails
const int ATTIC_FAN_PORT = 80;
const String ENDPOINT = "/indoor_sensors/data";

// Sensor configuration
// =======================================================================
// == EDIT THESE VALUES FOR EACH NEW SENSOR BOARD YOU CREATE            ==
const String SENSOR_ID = "sensor_livingroom_01";    // <-- MUST be unique for each sensor
const String SENSOR_NAME = "Living Room";           // <-- Human-readable name for the UI
const unsigned long POST_INTERVAL_MS = 30000;       // Send data every 30 seconds

// GPIO pins for I2C (adjust for your board)
#define SDA_PIN D2
#define SCL_PIN D1

// Sensor objects
#ifdef USE_SHT21
DFRobot_SHT20 sht21(&Wire, 0x40);
#endif

#ifdef USE_BME280
Adafruit_BME280 bme;
#endif

IPAddress controllerIP; // Will be resolved via mDNS
unsigned long lastPostTime = 0;
WiFiClient wifiClient;
HTTPClient http;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("ESP8266 Indoor Sensor Client Starting...");
  
  // Initialize I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Initialize sensor
  bool sensorOK = false;
#ifdef USE_SHT21
  if (sht21.initSHT20() == 0) { // initSHT20 returns 0 on success
    sensorOK = true;
    Serial.println("SHT21 sensor initialized successfully.");
  }
#endif
#ifdef USE_BME280
  if (bme.begin(0x76)) { // Use 0x76 as a common default address
    sensorOK = true;
    Serial.println("BME280 sensor initialized");
  } else {
    Serial.println("Failed to initialize BME280 sensor!");
  }
#endif

  if (!sensorOK) {
    Serial.println("ERROR: No sensor initialized! Check your configuration.");
    while(1) delay(1000);
  }
  
  WiFi.mode(WIFI_STA);
  WiFi.hostname("IndoorSensor-" + SENSOR_ID); // Set a unique hostname
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
    delay(1000);
    Serial.print(".");
  }
  
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Discover controller via mDNS
    Serial.printf("Querying for mDNS host '%s'...\n", CONTROLLER_MDNS_HOSTNAME);
    controllerIP = MDNS.queryHost(CONTROLLER_MDNS_HOSTNAME, 3000); // 3-second timeout

    if (controllerIP == IPAddress(0,0,0,0)) {
      Serial.printf("WARN: mDNS query failed. Falling back to IP: %s\n", FALLBACK_CONTROLLER_IP);
      if (!controllerIP.fromString(FALLBACK_CONTROLLER_IP)) {
        Serial.println("ERROR: Fallback IP is invalid. Restarting...");
        delay(10000);
        ESP.restart();
      }
    } else {
      Serial.print("SUCCESS: Controller found at IP: ");
      Serial.println(controllerIP);
    }

    Serial.print("Controller URL: http://");
    Serial.print(controllerIP.toString());
    Serial.println(ENDPOINT);

  } else {
    Serial.println("WiFi connection FAILED. Please check credentials. Restarting in 10 seconds...");
    delay(10000);
    ESP.restart();
  }
  
  // Send initial data immediately
  lastPostTime = millis() - POST_INTERVAL_MS;
}

void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    WiFi.begin(ssid, password); // Use begin() for a more robust reconnect
    delay(5000);
    return;
  }
  
  // Send data at regular intervals
  if (millis() - lastPostTime >= POST_INTERVAL_MS) {
    sendSensorData();
    lastPostTime = millis();
  }
  
  delay(1000);
}

void sendSensorData() {
  float temperature = NAN;
  float humidity = NAN;
  
  // Read sensor data
#ifdef USE_SHT21
  temperature = sht21.readTemperature();
  humidity = sht21.readHumidity(); // DFRobot library returns NAN on failure
  if (!isnan(temperature)) {
    temperature = temperature * 1.8 + 32.0; // Convert Celsius to Fahrenheit
  }
#endif

#ifdef USE_BME280
  temperature = bme.readTemperature() * 1.8 + 32.0; // Adafruit library returns temp in C
  humidity = bme.readHumidity(); // Adafruit library returns NAN on failure
#endif

  // Validate readings
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("ERROR: Failed to read sensor data");
    return;
  }
  
  if (temperature < -50 || temperature > 150 || humidity < 0 || humidity > 100) {
    Serial.println("ERROR: Sensor readings out of range");
    return;
  }
  
  Serial.printf("Sensor data: %.1f°F, %.1f%% RH\n", temperature, humidity);
  
  // Create JSON payload
  StaticJsonDocument<200> doc;
  doc["sensorId"] = SENSOR_ID;
  doc["name"] = SENSOR_NAME;
  doc["temperature"] = serialized(String(temperature, 1)); // Send as string with 1 decimal place
  doc["humidity"] = humidity;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Send HTTP POST request
  String url = "http://" + controllerIP.toString() + ":" + String(ATTIC_FAN_PORT) + ENDPOINT;
  
  http.setReuse(true); // Reuse TCP connection for efficiency
  http.begin(wifiClient, url);
  http.addHeader("Content-Type", "application/json");
  
  int httpCode = http.POST(jsonString);
  
  if (httpCode > 0) {
    if (httpCode == 200) {
      Serial.println("✓ Data sent successfully");
    } else {
      Serial.printf("⚠ HTTP response: %d\n", httpCode);
      String response = http.getString();
      if (response.length() > 0) {
        Serial.println("Response: " + response);
      }
    }
  } else {
    Serial.printf("✗ HTTP error: %s\n", http.errorToString(httpCode).c_str());
  }
  
  http.end();
}