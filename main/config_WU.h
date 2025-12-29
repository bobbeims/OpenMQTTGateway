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
extern void MQTTtoWU(char* topicOri, JsonObject& WUdata);

/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR WEATHER UNDERGROUND CREDENTIALS----------------*/

// Iris 5-in-1 Station
#ifndef WU_STATION_ID_IRIS
#  define WU_STATION_ID_IRIS "KTXANNA130"
#endif

#ifndef WU_API_KEY_IRIS
#  define WU_API_KEY_IRIS "P0iG2Nku"
#endif

#ifndef WU_UPLOAD_INTERVAL_IRIS
#  define WU_UPLOAD_INTERVAL_IRIS 120000 // 2 minutes
#endif

// Remote Sensor Station  
#ifndef WU_STATION_ID_REMOTE
#  define WU_STATION_ID_REMOTE "KTXANNA131"
#endif

#ifndef WU_API_KEY_REMOTE
#  define WU_API_KEY_REMOTE "DkKnjpYa"
#endif

#ifndef WU_UPLOAD_INTERVAL_REMOTE
#  define WU_UPLOAD_INTERVAL_REMOTE 300000 // 5 minutes
#endif

#endif