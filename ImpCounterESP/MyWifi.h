#ifndef _WIFI_h
#define _WIFI_h

#define SETUP_PIN D5
#define WIFI_TIMEOUT 1000UL // ms


#include <Arduino.h>
#include <WiFiClient.h>


class MyWifi
{
 protected:
	 IPAddress ip;
	 IPAddress subnet; 
	 IPAddress gw;
	 IPAddress remoteIP; 
	 uint16_t remotePort;

	 void storeConfig( IPAddress ip, IPAddress subnet, IPAddress gw, IPAddress remoteIP, uint16_t remotePort );
	 void loadConfig();

 public:
	 void begin();
	 bool send( const void * data, uint16_t length );
};


#endif

