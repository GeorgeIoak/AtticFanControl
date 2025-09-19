#pragma once

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "hardware.h"
#include "secrets.h"

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
  char sunrise[10]; // ISO time string, e.g., "06:30"
  char sunset[10];  // ISO time string, e.g., "19:45"
};

// Structure for hourly forecast data
struct HourlyForecast {
  char timeString[20]; // ISO 8601 string, e.g., "2025-01-15T14:00"
  float temperature;
  int weatherCode;
};

// Global variables to store weather data
CurrentWeather currentWeather = {0.0, 0, 0, false};
DailyForecast forecast[3];
HourlyForecast hourlyForecast[5]; // Store next 4-5 hours

// Last update time
// Initialize to 1 to prevent running on the very first loop before WiFi is up.
unsigned long lastWeatherUpdate = 1;
static bool initialWeatherFetchDone = false;

/**
 * @brief Extracts time (HH:MM) from ISO datetime string.
 * @param isoDateTime ISO datetime string like "2025-01-15T06:30"
 * @param timeOut Output buffer for time string (minimum 6 chars)
 */
void extractTimeFromISO(const char* isoDateTime, char* timeOut, size_t maxLen) {
  if (!isoDateTime || !timeOut || maxLen < 6) {
    if (timeOut && maxLen > 0) timeOut[0] = '\0';
    return;
  }
  
  const char* timeStart = strchr(isoDateTime, 'T');
  if (timeStart && strlen(timeStart) >= 6) {
    timeStart++; // Skip the 'T'
    strncpy(timeOut, timeStart, 5); // Copy HH:MM
    timeOut[5] = '\0';
  } else {
    timeOut[0] = '\0';
  }
}

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
               "&current=temperature_2m,relativehumidity_2m,weathercode&daily=weathercode,temperature_2m_max,temperature_2m_min,sunrise,sunset&hourly=temperature_2m,weathercode&temperature_unit=fahrenheit&windspeed_unit=mph&precipitation_unit=inch&forecast_days=3&timezone=auto";

  http.begin(client, url);
  #if DEBUG_SERIAL
  logSerial("[INFO] Updating weather data...");
  #endif
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    StaticJsonDocument<2048> doc; // Increased size to accommodate new data
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

      // Parse 3-day forecast with sunrise/sunset
      JsonArray daily_time = doc["daily"]["time"];
      JsonArray daily_sunrise = doc["daily"]["sunrise"];
      JsonArray daily_sunset = doc["daily"]["sunset"];
      
      for (int i = 0; i < daily_time.size() && i < 3; i++) {
        forecast[i].tempMax = doc["daily"]["temperature_2m_max"][i];
        forecast[i].tempMin = doc["daily"]["temperature_2m_min"][i];
        forecast[i].weatherCode = doc["daily"]["weathercode"][i];
        
        // Parse sunrise/sunset times
        const char* sunriseStr = daily_sunrise[i] | "";
        const char* sunsetStr = daily_sunset[i] | "";
        extractTimeFromISO(sunriseStr, forecast[i].sunrise, sizeof(forecast[i].sunrise));
        extractTimeFromISO(sunsetStr, forecast[i].sunset, sizeof(forecast[i].sunset));
        
        const char* dateStr = daily_time[i];
        if (dateStr) {
          int year, month, day;
          sscanf(dateStr, "%d-%d-%d", &year, &month, &day);
          forecast[i].dayOfWeek = getDayOfWeek(year, month, day);
        } else {
          forecast[i].dayOfWeek = -1; // Invalid
        }
      }

      // Parse hourly forecast (next 4-5 hours)
      if (doc.containsKey("hourly")) {
        JsonArray hourly_time = doc["hourly"]["time"];
        JsonArray hourly_temp = doc["hourly"]["temperature_2m"];
        JsonArray hourly_weathercode = doc["hourly"]["weathercode"];

        // Find the current hour's index to get the *next* 5 hours
        int startIdx = 0;
        time_t now;
        time(&now);
        struct tm* timeinfo = localtime(&now);
        if (timeinfo && timeinfo->tm_year > 70) { // Check if time is synced
          startIdx = timeinfo->tm_hour;
        }

        // Clear old forecast data
        for (int i = 0; i < 5; i++) {
          hourlyForecast[i].timeString[0] = '\0';
        }

        int forecastCount = 0;
        for (int i = startIdx; i < hourly_time.size() && forecastCount < 5; i++) {
          const char* hourlyTimeStr = hourly_time[i] | "";
          strncpy(hourlyForecast[forecastCount].timeString, hourlyTimeStr, sizeof(hourlyForecast[forecastCount].timeString) - 1);
          hourlyForecast[forecastCount].timeString[sizeof(hourlyForecast[forecastCount].timeString) - 1] = '\0';
          hourlyForecast[forecastCount].temperature = hourly_temp[i] | 0.0;
          hourlyForecast[forecastCount].weatherCode = hourly_weathercode[i] | 0;
          forecastCount++;
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
  StaticJsonDocument<1024> doc; // Increased size for new data
  doc["currentTemp"] = serialized(String(currentWeather.temperature, 1));
  doc["currentHumidity"] = currentWeather.humidity;
  doc["currentIcon"] = weatherCodeToEmoji(currentWeather.weatherCode);
  
  // Add sunrise and sunset from today's forecast (first entry)
  if (strlen(forecast[0].sunrise) > 0 && strlen(forecast[0].sunset) > 0) {
    doc["sunrise"] = forecast[0].sunrise;
    doc["sunset"] = forecast[0].sunset;
  }
  
  // Add hourly forecast in Open-Meteo format
  JsonObject hourlyObj = doc.createNestedObject("hourly");
  JsonArray timeArr = hourlyObj.createNestedArray("time");
  JsonArray tempArr = hourlyObj.createNestedArray("temperature_2m");
  JsonArray codeArr = hourlyObj.createNestedArray("weathercode");
  for (int i = 0; i < 5; i++) {
    if (strlen(hourlyForecast[i].timeString) > 0) {
      timeArr.add(hourlyForecast[i].timeString);
      tempArr.add(hourlyForecast[i].temperature);
      codeArr.add(hourlyForecast[i].weatherCode);
    }
  }
  
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