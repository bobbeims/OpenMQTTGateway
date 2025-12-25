/*
  OpenMQTTGateway - ESP8266 or Arduino program for home automation

  Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker
  Send and receiving command by MQTT

  Weather Underground upload gateway

  Copyright: (c)

  This file is part of OpenMQTTGateway.

    OpenMQTTGateway is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenMQTTGateway is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "User_config.h"

#ifdef ZgatewayWU

#  include <ArduinoJson.h>
#  include <HTTPClient.h>

struct WeatherData {
  float temp_f = 0;
  float humidity = 0;
  float windSpeed = 0;
  int windDir = 0;
  float rain = 0;
  unsigned long lastUpdate = 0;
  bool valid = false;
} currentWeather;

unsigned long lastWUUpload = 0;

void setupWU() {
  Log.notice(F("Weather Underground gateway setup" CR));
  Log.notice(F("Station ID: %s" CR), WU_STATION_ID);
  Log.notice(F("Upload interval: %d seconds" CR), WU_UPLOAD_INTERVAL / 1000);
}

void WUtoMQTT() {
  // This function is called periodically by the main loop
  if (!currentWeather.valid) {
    return;
  }

  // Check if enough time has passed
  if (millis() - lastWUUpload < WU_UPLOAD_INTERVAL) {
    return;
  }

  // Check data isn't too old (10 minutes)
  if (millis() - currentWeather.lastUpdate > 600000) {
    Log.warning(F("WU: Weather data too old" CR));
    return;
  }

  HTTPClient http;

  // Build Weather Underground URL
  String url = "https://weatherstation.wunderground.com/weatherstation/updateweatherstation.php?";
  url += "ID=" + String(WU_STATION_ID);
  url += "&PASSWORD=" + String(WU_API_KEY);
  url += "&dateutc=now";
  url += "&tempf=" + String(currentWeather.temp_f, 1);
  url += "&humidity=" + String((int)currentWeather.humidity);
  url += "&windspeedmph=" + String(currentWeather.windSpeed, 1);
  url += "&winddir=" + String(currentWeather.windDir);
  url += "&rainin=" + String(currentWeather.rain, 2);
  url += "&action=updateraw";

  Log.notice(F("WU: Uploading to Weather Underground..." CR));

  http.begin(url);
  http.setTimeout(10000);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String response = http.getString();
    Log.trace(F("WU: Response (%d): %s" CR), httpCode, response.c_str());

    if (response.indexOf("success") >= 0) {
      Log.notice(F("WU: Upload successful!" CR));
      lastWUUpload = millis();
    } else {
      Log.warning(F("WU: Upload failed - %s" CR), response.c_str());
    }
  } else {
    Log.error(F("WU: HTTP error: %d" CR), httpCode);
  }

  http.end();
}

void MQTTtoWU(char* topicOri, JsonObject& WUdata) {
  // Process incoming RTL_433 data
  if (!WUdata.containsKey("model")) {
    return;
  }

  const char* model = WUdata["model"];
  if (strstr(model, "Acurite") == NULL) {
    return; // Not an AcuRite sensor
  }

  Log.trace(F("WU: Processing AcuRite data" CR));

  // Extract temperature
  if (WUdata.containsKey("temperature_F")) {
    currentWeather.temp_f = WUdata["temperature_F"];
  } else if (WUdata.containsKey("temperature_C")) {
    float tempC = WUdata["temperature_C"];
    currentWeather.temp_f = tempC * 9.0 / 5.0 + 32.0;
  }

  // Extract humidity
  if (WUdata.containsKey("humidity")) {
    currentWeather.humidity = WUdata["humidity"];
  }

  // Extract wind speed
  if (WUdata.containsKey("wind_avg_mi_h")) {
    currentWeather.windSpeed = WUdata["wind_avg_mi_h"];
  } else if (WUdata.containsKey("wind_avg_km_h")) {
    float windKmh = WUdata["wind_avg_km_h"];
    currentWeather.windSpeed = windKmh * 0.621371;
  }

  // Extract wind direction
  if (WUdata.containsKey("wind_dir_deg")) {
    currentWeather.windDir = WUdata["wind_dir_deg"];
  }

  // Extract rainfall
  if (WUdata.containsKey("rain_in")) {
    currentWeather.rain = WUdata["rain_in"];
  } else if (WUdata.containsKey("rain_mm")) {
    float rainMm = WUdata["rain_mm"];
    currentWeather.rain = rainMm * 0.0393701;
  }

  currentWeather.lastUpdate = millis();
  currentWeather.valid = true;

  Log.notice(F("WU: Data updated - %.1fF, %.0f%%, %.1fmph" CR),
             currentWeather.temp_f, currentWeather.humidity, currentWeather.windSpeed);
}

#endif