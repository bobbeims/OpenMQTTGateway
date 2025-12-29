/*
  OpenMQTTGateway - ESP8266 or Arduino program for home automation

  Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker
  Send and receiving command by MQTT

  Weather Underground upload gateway - Dual Station Support

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

#  include "config_WU.h"
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
};

// Separate data for each station
WeatherData irisWeather;
WeatherData remoteWeather;

unsigned long lastIrisUpload = 0;
unsigned long lastRemoteUpload = 0;

void setupWU() {
  Log.notice(F("Weather Underground gateway setup (Dual Station)" CR));
  Log.notice(F("Iris Station ID: %s (interval: %d sec)" CR), WU_STATION_ID_IRIS, WU_UPLOAD_INTERVAL_IRIS / 1000);
  Log.notice(F("Remote Station ID: %s (interval: %d sec)" CR), WU_STATION_ID_REMOTE, WU_UPLOAD_INTERVAL_REMOTE / 1000);
}

bool uploadToWU(const char* stationId, const char* apiKey, WeatherData& data, unsigned long& lastUpload, const char* sensorName, unsigned long uploadInterval) {
  // Check if enough time has passed
  if (millis() - lastUpload < WU_UPLOAD_INTERVAL) {
    return false;
  }

  // Check data isn't too old (10 minutes)
  if (millis() - data.lastUpdate > 600000) {
    Log.warning(F("WU: %s data too old" CR), sensorName);
    return false;
  }

  HTTPClient http;

  // Build Weather Underground URL
  String url = "https://weatherstation.wunderground.com/weatherstation/updateweatherstation.php?";
  url += "ID=" + String(stationId);
  url += "&PASSWORD=" + String(apiKey);
  url += "&dateutc=now";
  url += "&tempf=" + String(data.temp_f, 1);
  url += "&humidity=" + String((int)data.humidity);
  
  // Add wind data if available (only for Iris)
  if (data.windSpeed > 0 || data.windDir > 0) {
    url += "&windspeedmph=" + String(data.windSpeed, 1);
    url += "&winddir=" + String(data.windDir);
  }
  
  // Add rain data if available (only for Iris)
  if (data.rain > 0) {
    url += "&rainin=" + String(data.rain, 2);
  }
  
  url += "&action=updateraw";

  Log.notice(F("WU: Uploading %s to Weather Underground..." CR), sensorName);

  http.begin(url);
  http.setTimeout(10000);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String response = http.getString();
    Log.trace(F("WU: Response (%d): %s" CR), httpCode, response.c_str());

    if (response.indexOf("success") >= 0) {
      Log.notice(F("WU: %s upload successful!" CR), sensorName);
      lastUpload = millis();
      http.end();
      return true;
    } else {
      Log.warning(F("WU: %s upload failed - %s" CR), sensorName, response.c_str());
    }
  } else {
    Log.error(F("WU: %s HTTP error: %d" CR), sensorName, httpCode);
  }

  http.end();
  return false;
}

void WUtoMQTT() {
  // Upload Iris data if valid
  if (irisWeather.valid) {
    uploadToWU(WU_STATION_ID_IRIS, WU_API_KEY_IRIS, irisWeather, lastIrisUpload, "Iris", WU_UPLOAD_INTERVAL_IRIS);
  }
  
  // Upload Remote data if valid
  if (remoteWeather.valid) {
    uploadToWU(WU_STATION_ID_REMOTE, WU_API_KEY_REMOTE, remoteWeather, lastRemoteUpload, "Remote", WU_UPLOAD_INTERVAL_REMOTE);
  }
}

void MQTTtoWU(char* topicOri, JsonObject& WUdata) {
  // Process incoming RTL_433 data
  if (!WUdata.containsKey("model")) {
    return;
  }

  const char* model = WUdata["model"];
  
  // Determine which sensor this is and route to appropriate struct
  WeatherData* targetWeather = nullptr;
  const char* sensorName = nullptr;
  
  if (strstr(model, "Acurite-Tower") != NULL) {
    targetWeather = &remoteWeather;
    sensorName = "Remote (06002M)";
  } else if (strstr(model, "Acurite-6045M") != NULL || strstr(model, "Acurite-5n1") != NULL) {
    targetWeather = &irisWeather;
    sensorName = "Iris (5-in-1)";
  } else {
    // Not a sensor we're tracking
    return;
  }

  Log.trace(F("WU: Processing %s data" CR), sensorName);
  
  // Log the full JSON for debugging
  String jsonStr;
  serializeJson(WUdata, jsonStr);
  Log.notice(F("WU: Received JSON from %s: %s" CR), sensorName, jsonStr.c_str());
  
  // Extract temperature
  if (WUdata.containsKey("temperature_F")) {
    targetWeather->temp_f = WUdata["temperature_F"];
  } else if (WUdata.containsKey("temperature_C")) {
    float tempC = WUdata["temperature_C"];
    targetWeather->temp_f = tempC * 9.0 / 5.0 + 32.0;
  }

  // Extract humidity
  if (WUdata.containsKey("humidity")) {
    targetWeather->humidity = WUdata["humidity"];
  }

  // Extract wind speed (Iris only)
  if (WUdata.containsKey("wind_avg_mi_h")) {
    targetWeather->windSpeed = WUdata["wind_avg_mi_h"];
  } else if (WUdata.containsKey("wind_avg_km_h")) {
    float windKmh = WUdata["wind_avg_km_h"];
    targetWeather->windSpeed = windKmh * 0.621371;
  }

  // Extract wind direction (Iris only)
  if (WUdata.containsKey("wind_dir_deg")) {
    targetWeather->windDir = WUdata["wind_dir_deg"];
  }

  // Extract rainfall (Iris only)
  if (WUdata.containsKey("rain_in")) {
    targetWeather->rain = WUdata["rain_in"];
  } else if (WUdata.containsKey("rain_mm")) {
    float rainMm = WUdata["rain_mm"];
    targetWeather->rain = rainMm * 0.0393701;
  }

  targetWeather->lastUpdate = millis();
  targetWeather->valid = true;

  Log.notice(F("WU: %s data updated - %.1fF, %.0f%%" CR),
             sensorName, targetWeather->temp_f, targetWeather->humidity);
}

#endif