#pragma once

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "hardware.h"

// Structure to hold current weather conditions
struct CurrentWeather {
  float temperature;
  int humidity;
  int weatherCode;
  bool isValid;
  char timeString[20]; // ISO 8601 string, e.g., "2025-09-05T16:00"
};

// Structure for a single day's forecast
struct DailyForecast {
  float tempMax;
  float tempMin;
  int weatherCode;
  int dayOfWeek; // 0=Sun, 1=Mon, ...
};

// Global variables to store weather data
CurrentWeather currentWeather = {0.0, 0, 0, false};
DailyForecast forecast[3];

// Last update time
// Initialize to 1 to prevent running on the very first loop before WiFi is up.
unsigned long lastWeatherUpdate = 1;
static bool initialWeatherFetchDone = false;

/**
 * @brief Calculates the day of the week from a date.
 * @param y Year
 * @param m Month (1-12)
 * @param d Day (1-31)
 * @return Day of week (0=Sun, 1=Mon, ..., 6=Sat)
 */
int getDayOfWeek(int y, int m, int d) {
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) y -= 1;
    return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}

/**
 * @brief Converts a WMO weather code to a simple emoji.
 * @param code The WMO weather code from the API.
 * @return A string containing a weather emoji.
 */
const char* weatherCodeToEmoji(int code) {
  if (code == 0) return "☀️"; // Clear sky
  if (code >= 1 && code <= 3) return "⛅"; // Mainly clear, partly cloudy, overcast
  if (code >= 45 && code <= 48) return "🌫️"; // Fog
  if (code >= 51 && code <= 57) return "💧"; // Drizzle
  if (code >= 61 && code <= 67) return "🌧️"; // Rain
  if (code >= 71 && code <= 77) return "❄️"; // Snow
  if (code >= 80 && code <= 82) return "🌦️"; // Rain showers
  if (code >= 85 && code <= 86) return "🌨️"; // Snow showers
  if (code >= 95 && code <= 99) return "⛈️"; // Thunderstorm
  return "❓";
}

/**
 * @brief Fetches weather data from the Open-Meteo API.
 */
inline void updateWeatherData() {
  // Don't attempt to fetch data if WiFi is not connected.
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  // Check if it's time to update.
  // An update is due if the interval has passed OR if lastWeatherUpdate is 0 (a forced trigger).
  if (lastWeatherUpdate != 0 && (millis() - lastWeatherUpdate < WEATHER_UPDATE_INTERVAL_MS)) {
    return;
  }
  lastWeatherUpdate = millis(); // Update timer immediately to prevent spam on failure

  WiFiClient client;
  HTTPClient http;
  String url = "http://api.open-meteo.com/v1/forecast?latitude=" + String(WEATHER_LATITUDE) +
               "&longitude=" + String(WEATHER_LONGITUDE) +
               "&current=temperature_2m,relativehumidity_2m,weathercode&daily=weathercode,temperature_2m_max,temperature_2m_min&temperature_unit=fahrenheit&windspeed_unit=mph&precipitation_unit=inch&forecast_days=3&timezone=auto";

  http.begin(client, url);
  #if DEBUG_SERIAL
  logSerial("[INFO] Updating weather data...");
  #endif
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      #if DEBUG_SERIAL
      logSerial("[ERROR] Weather JSON parsing failed: %s", error.c_str());
      #endif
      currentWeather.isValid = false;
      http.end();
      return;
    }

    // To be considered valid, both 'current' and 'daily' data must be present.
    if (doc.containsKey("current") && doc.containsKey("daily")) {
      JsonObject current = doc["current"];
      currentWeather.temperature = current["temperature_2m"];
      currentWeather.humidity = current["relativehumidity_2m"];
      currentWeather.weatherCode = current["weathercode"];
      const char* apiTime = current["time"] | "";
      strncpy(currentWeather.timeString, apiTime, sizeof(currentWeather.timeString) - 1);
      currentWeather.timeString[sizeof(currentWeather.timeString) - 1] = '\0';

      // Parse 3-day forecast
      JsonArray daily_time = doc["daily"]["time"];
      for (int i = 0; i < daily_time.size() && i < 3; i++) {
        forecast[i].tempMax = doc["daily"]["temperature_2m_max"][i];
        forecast[i].tempMin = doc["daily"]["temperature_2m_min"][i];
        forecast[i].weatherCode = doc["daily"]["weathercode"][i];
        const char* dateStr = daily_time[i];
        if (dateStr) {
          int year, month, day;
          sscanf(dateStr, "%d-%d-%d", &year, &month, &day);
          forecast[i].dayOfWeek = getDayOfWeek(year, month, day);
        } else {
          forecast[i].dayOfWeek = -1; // Invalid
        }
      }
      currentWeather.isValid = true;

      #if DEBUG_SERIAL
      if (!initialWeatherFetchDone) {
        logSerial("[INFO] Initial weather data received successfully.");
        initialWeatherFetchDone = true;
      } else {
        logSerial("[INFO] Weather data updated successfully.");
      }
      logSerial("  - Current Temp: %.1f°F", currentWeather.temperature);
      #endif
    } else {
      #if DEBUG_SERIAL
      logSerial("[WARN] Weather data response was missing 'current' or 'daily' objects.");
      #endif
      currentWeather.isValid = false;
    }
  } else {
    #if DEBUG_SERIAL
    logSerial("[ERROR] Weather API request failed, error: %s", http.errorToString(httpCode).c_str());
    #endif
    currentWeather.isValid = false;
  }

  http.end();
}

inline void handleWeather(ESP8266WebServer &server) {
  StaticJsonDocument<512> doc;
  doc["currentTemp"] = serialized(String(currentWeather.temperature, 1));
  doc["currentHumidity"] = currentWeather.humidity;
  doc["currentIcon"] = weatherCodeToEmoji(currentWeather.weatherCode);
  JsonArray forecastData = doc.createNestedArray("forecast");
  for (int i = 0; i < 3; i++) {
    JsonObject day = forecastData.createNestedObject();
    day["icon"] = weatherCodeToEmoji(forecast[i].weatherCode);
    day["max"] = serialized(String(forecast[i].tempMax, 0));
    day["min"] = serialized(String(forecast[i].tempMin, 0));
    day["dayOfWeek"] = forecast[i].dayOfWeek;
  }
  server.send(200, "application/json", doc.as<String>());
}