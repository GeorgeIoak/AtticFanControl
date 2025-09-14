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

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Attic Fan Controller settings
const char* ATTIC_FAN_IP = "192.168.1.100";  // Update with your controller's IP
const int ATTIC_FAN_PORT = 80;
const String ENDPOINT = "/indoor_sensors/data";

// Sensor configuration
const String SENSOR_ID = "sensor_livingroom_01";    // Unique ID for this sensor
const String SENSOR_NAME = "Living Room";           // Human-readable name
const unsigned long POST_INTERVAL = 30000;          // Send data every 30 seconds

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

unsigned long lastPost = 0;
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
  sht21.initSHT20();
  sensorOK = true;
  Serial.println("SHT21 sensor initialized");
#endif

#ifdef USE_BME280
  if (bme.begin()) {
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
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Controller URL: http://");
  Serial.print(ATTIC_FAN_IP);
  Serial.println(ENDPOINT);
  Serial.println();
  
  // Send initial data immediately
  lastPost = millis() - POST_INTERVAL;
}

void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    WiFi.reconnect();
    delay(5000);
    return;
  }
  
  // Send data at regular intervals
  if (millis() - lastPost >= POST_INTERVAL) {
    sendSensorData();
    lastPost = millis();
  }
  
  delay(1000);
}

void sendSensorData() {
  float temperature = NAN;
  float humidity = NAN;
  
  // Read sensor data
#ifdef USE_SHT21
  float tempC = sht21.readTemperature();
  if (!isnan(tempC)) {
    temperature = tempC * 1.8 + 32.0; // Convert to Fahrenheit
  }
  humidity = sht21.readHumidity();
#endif

#ifdef USE_BME280
  temperature = bme.readTemperature() * 1.8 + 32.0; // Convert to Fahrenheit
  humidity = bme.readHumidity();
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
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Send HTTP POST request
  String url = "http://" + String(ATTIC_FAN_IP) + ":" + String(ATTIC_FAN_PORT) + ENDPOINT;
  
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