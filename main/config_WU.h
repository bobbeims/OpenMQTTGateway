/*
  OpenMQTTGateway - Weather Underground Configuration

  Copyright: (c)

  This file is part of OpenMQTTGateway.
*/
#ifndef config_WU_h
#define config_WU_h

#include <ArduinoJson.h>

extern void setupWU();
extern void WUtoMQTT();
extern void MQTTtoWU(char* topicOri, ArduinoJson::JsonObject& WUdata);

/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR WEATHER UNDERGROUND CREDENTIALS----------------*/

#ifndef WU_STATION_ID
#  define WU_STATION_ID "KTXANNA129" // Replace with your station ID
#endif

#ifndef WU_API_KEY
# define WU_API_KEY "Q0HqPJed" // Replace with your API key
#endif

#ifndef WU_UPLOAD_INTERVAL
# define WU_UPLOAD_INTERVAL 300000 // Upload every 5 minutes (milliseconds)
#endif

/*-------------------PLACEHOLDERS FOR STATUS REPORTING--------------------*/
// You can add MQTT status topics here if desired

#endif