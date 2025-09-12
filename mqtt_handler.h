#pragma once

#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include "hardware.h"
#include "config.h"
#include "types.h"
#include "sensors.h"

// Forward declarations from the main .ino file
extern FanMode fanMode;
extern void setFanState(bool fanOn);
extern void cancelManualTimer();

// MQTT Client
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// MQTT Topics
const char* baseTopic = "attic_fan";
char stateTopic[40];
char commandTopic[40];
char modeStateTopic[40];
char modeCommandTopic[40];

/**
 * @brief Handles incoming MQTT messages.
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    char p[length + 1];
    memcpy(p, payload, length);
    p[length] = '\0';

    #if DEBUG_SERIAL
    logSerial("[MQTT] Message arrived [%s]: %s", topic, p);
    #endif

    if (strcmp(topic, commandTopic) == 0) {
        cancelManualTimer();
        // When controlled via switch, always go to a manual state
        if (strcmp(p, "ON") == 0) {
            fanMode = MANUAL_ON;
            config.fanMode = MANUAL_ON;
            setFanState(true);
        } else {
            fanMode = MANUAL_OFF;
            config.fanMode = MANUAL_OFF;
            setFanState(false);
        }
        saveConfig();
    } else if (strcmp(topic, modeCommandTopic) == 0) {
        cancelManualTimer();
        if (strcmp(p, "AUTO") == 0) {
            fanMode = AUTO;
        } else { // Assume any other value means switch to MANUAL
            // When switching to manual, preserve current fan state
            bool fanIsOn = digitalRead(FAN_RELAY_PIN) == HIGH;
            fanMode = fanIsOn ? MANUAL_ON : MANUAL_OFF;
        }
        // Persist the new mode
        config.fanMode = fanMode;
        saveConfig();
    }
}

/**
 * @brief Publishes Home Assistant discovery configuration.
 */
void publishDiscovery() {
    StaticJsonDocument<1024> doc;
    char topicBuffer[128];
    char payloadBuffer[1024];

    // --- Device Info ---
    JsonObject device = doc.createNestedObject("device");
    device["identifiers"] = WiFi.macAddress();
    device["name"] = "Attic Fan Controller";
    device["model"] = "ESP8266 Fan Controller";
    device["manufacturer"] = "DIY";
    device["sw_version"] = FIRMWARE_VERSION;

    // --- Fan Switch Entity ---
    doc["name"] = "Attic Fan";
    doc["unique_id"] = "attic_fan_switch";
    doc["state_topic"] = stateTopic;
    doc["command_topic"] = commandTopic;
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";
    doc["icon"] = "mdi:fan";
    snprintf(topicBuffer, sizeof(topicBuffer), "homeassistant/switch/%s/config", (const char *)doc["unique_id"]);
    serializeJson(doc, payloadBuffer);
    mqttClient.publish(topicBuffer, payloadBuffer, true);
    doc.clear();
    device = doc.createNestedObject("device"); // Re-create device object
    device["identifiers"] = WiFi.macAddress();

    // --- Fan Mode Select Entity ---
    doc["name"] = "Attic Fan Mode";
    doc["unique_id"] = "attic_fan_mode";
    doc["state_topic"] = modeStateTopic;
    doc["command_topic"] = modeCommandTopic;
    JsonArray options = doc.createNestedArray("options");
    options.add("AUTO");
    options.add("MANUAL");
    doc["icon"] = "mdi:cog-transfer";
    snprintf(topicBuffer, sizeof(topicBuffer), "homeassistant/select/%s/config", (const char *)doc["unique_id"]);
    serializeJson(doc, payloadBuffer);
    mqttClient.publish(topicBuffer, payloadBuffer, true);
    doc.clear();
    device = doc.createNestedObject("device");
    device["identifiers"] = WiFi.macAddress();

    // --- Sensor Entities ---
    const char* sensors[][4] = {
        {"attic_temp", "Attic Temperature", "temperature", "°F"},
        {"attic_humidity", "Attic Humidity", "humidity", "%"},
        {"outdoor_temp", "Outdoor Temperature", "temperature", "°F"}
    };

    for (auto &sensor : sensors) {
        doc["name"] = sensor[1];
        doc["unique_id"] = sensor[0];
        snprintf(topicBuffer, sizeof(topicBuffer), "%s/sensor/%s/state", baseTopic, sensor[0]);
        doc["state_topic"] = topicBuffer;
        doc["device_class"] = sensor[2];
        doc["unit_of_measurement"] = sensor[3];
        doc["value_template"] = "{{ value_json.value }}";
        snprintf(topicBuffer, sizeof(topicBuffer), "homeassistant/sensor/%s/config", sensor[0]);
        serializeJson(doc, payloadBuffer);
        mqttClient.publish(topicBuffer, payloadBuffer, true);
        doc.clear();
        device = doc.createNestedObject("device");
        device["identifiers"] = WiFi.macAddress();
    }

    #if DEBUG_SERIAL
    logSerial("[MQTT] Published Home Assistant discovery messages.");
    #endif
}

/**
 * @brief Connects/reconnects to the MQTT broker.
 */
void reconnectMqtt() {
    if (mqttClient.connected()) {
        return;
    }
    #if DEBUG_SERIAL
    logSerial("[MQTT] Attempting connection...");
    #endif
    String clientId = "AtticFan-" + WiFi.macAddress();
    if (mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
        #if DEBUG_SERIAL
        logSerial("[MQTT] Connection successful!");
        #endif
        mqttClient.subscribe(commandTopic);
        mqttClient.subscribe(modeCommandTopic);
        if (config.mqttDiscoveryEnabled) {
            publishDiscovery();
        }
    } else {
        #if DEBUG_SERIAL
        logSerial("[MQTT] Connection failed, rc=%d. Will try again in 5 seconds.", mqttClient.state());
        #endif
    }
}

/**
 * @brief Publishes the current state of all sensors and the fan.
 */
void publishState() {
    if (!mqttClient.connected()) return;

    // Publish fan state
    bool fanIsOn = digitalRead(FAN_RELAY_PIN) == HIGH;
    mqttClient.publish(stateTopic, fanIsOn ? "ON" : "OFF", true);

    // Publish fan mode
    mqttClient.publish(modeStateTopic, (fanMode == AUTO) ? "AUTO" : "MANUAL", true);

    // Publish sensor values
    StaticJsonDocument<128> doc;
    char topicBuffer[80];
    char payloadBuffer[64];

    // Attic Temp
    doc["value"] = readAtticTemp();
    serializeJson(doc, payloadBuffer);
    snprintf(topicBuffer, sizeof(topicBuffer), "%s/sensor/attic_temp/state", baseTopic);
    mqttClient.publish(topicBuffer, payloadBuffer, true);

    // Attic Humidity
    doc["value"] = readAtticHumidity();
    serializeJson(doc, payloadBuffer);
    snprintf(topicBuffer, sizeof(topicBuffer), "%s/sensor/attic_humidity/state", baseTopic);
    mqttClient.publish(topicBuffer, payloadBuffer, true);

    // Outdoor Temp
    doc["value"] = readOutdoorTemp();
    serializeJson(doc, payloadBuffer);
    snprintf(topicBuffer, sizeof(topicBuffer), "%s/sensor/outdoor_temp/state", baseTopic);
    mqttClient.publish(topicBuffer, payloadBuffer, true);

    #if DEBUG_SERIAL
    // Serial.println("[MQTT] Published state updates.");
    #endif
}

/**
 * @brief Initializes the MQTT client and topics.
 */
void initMqtt() {
    if (!config.mqttEnabled) return;

    // Construct topic strings
    snprintf(stateTopic, sizeof(stateTopic), "%s/state", baseTopic);
    snprintf(commandTopic, sizeof(commandTopic), "%s/command", baseTopic);
    snprintf(modeStateTopic, sizeof(modeStateTopic), "%s/mode/state", baseTopic);
    snprintf(modeCommandTopic, sizeof(modeCommandTopic), "%s/mode/command", baseTopic);

    mqttClient.setServer(mqtt_broker, mqtt_port);
    mqttClient.setCallback(mqttCallback);
}

/**
 * @brief Re-initializes the MQTT client.
 */
void reinitMqtt() {
    #if DEBUG_SERIAL
    logSerial("[MQTT] Re-initializing MQTT client...");
    #endif
    if (mqttClient.connected()) {
        mqttClient.disconnect();
    }
    initMqtt();
    reconnectMqtt(); // Attempt to connect immediately after re-initialization
}

/**
 * @brief Main MQTT handler to be called in the main loop.
 */
void handleMqtt() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    static unsigned long lastMqttReconnectAttempt = 0;
    static unsigned long lastStatePublish = 0;
    if (!config.mqttEnabled) return;
    if (!mqttClient.connected()) {
        if (millis() - lastMqttReconnectAttempt > 5000) {
            lastMqttReconnectAttempt = millis();
            reconnectMqtt();
        }
    }

    if (mqttClient.connected()) {
        mqttClient.loop();
        // Publish state periodically
        if (millis() - lastStatePublish > 30000) { // Publish every 30 seconds
            lastStatePublish = millis();
            publishState();
        }
    }
}