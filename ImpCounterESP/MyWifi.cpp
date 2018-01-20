#include "MyWifi.h"
#include "Logging.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#include <DNSServer.h>            // Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     // Local WebServer used to serve the configuration portal
#include <WiFiManager.h>		  // Configuration portal.

#include <EEPROM.h>

WiFiClient client;



/* Handles wifi connection. If user boots wit setup-pin low, we start a configuration portal AP.
   Otherwise just connect the wifi so that it is ready for data transfer */
void MyWifi::begin() {
	pinMode( SETUP_PIN, INPUT_PULLUP );

	// If the Wifi setup-pin is held low while booting, then we start up the wifimanager portal.
	if ( digitalRead( SETUP_PIN ) == LOW ) {
		LOG_NOTICE( "WIF", "User requested captive portal" );
		WiFi.disconnect( true );
		WiFiManager wifiManager;

		// Setup wifimanager with extra input fields. All the fields we want to save en EEPROM.
		// Values here are default values that will show up on the configuration web page
		WiFiManagerParameter parm_ip( "IP", "Static IP", "192.168.1.200", 20 );
		wifiManager.addParameter( &parm_ip );
		WiFiManagerParameter parm_subnet( "Subnet", "Subnet", "255.255.255.0", 20 );
		wifiManager.addParameter( &parm_subnet );
		WiFiManagerParameter parm_gw( "GW", "Gateway", "192.168.0.1", 20 );
		wifiManager.addParameter( &parm_gw );
		WiFiManagerParameter parm_remoteIP( "RemoteIP", "Remote IP", "192.168.1.100", 20 );
		wifiManager.addParameter( &parm_remoteIP );
		WiFiManagerParameter parm_remotePort( "Port", "Remote Port", "5001", 6 );
		wifiManager.addParameter( &parm_remotePort );

		// Start the portal with the SSID ENV_SENSOR
		wifiManager.startConfigPortal( "ENV_SENSOR" );
		LOG_NOTICE( "WIF", "Connected to wifi" );

		// Get all the values that user entered in the portal and save it in EEPROM
		ip.fromString( parm_ip.getValue() );
		subnet.fromString( parm_subnet.getValue() );
		gw.fromString( parm_gw.getValue() );
		remoteIP.fromString( parm_remoteIP.getValue() );
		remotePort = atoi( parm_remotePort.getValue() );
		storeConfig( ip, subnet, gw, remoteIP, remotePort );

		//TODO: do some led flashing
	} else {
		// Under normal boot we load config from EEPROM and start the wifi normally
		loadConfig();
		LOG_NOTICE( "WIF", "Starting Wifi" );
		WiFi.config( ip, gw, subnet );
		WiFi.begin();
		while ( WiFi.status() != WL_CONNECTED ) delay(1);
		LOG_NOTICE( "WIF", "Wifi connected, got IP address: " << WiFi.localIP().toString() );
		client.setTimeout( WIFI_TIMEOUT ); // If things doesn't happen within XX milliseconds, we are loosing battery!
	}
}



/* Takes all the IP information we got from Wifimanger and saves it in EEPROM */
void MyWifi::storeConfig(IPAddress ip, IPAddress subnet, IPAddress gw, IPAddress remoteIP, uint16_t remotePort) {
	byte* portPtr = (byte*)&remotePort;
	EEPROM.begin( 512 );
	for ( int i = 0; i < 4; i++ ) EEPROM.write( i + 0 , ip[i] );
	for ( int i = 0; i < 4; i++ ) EEPROM.write( i + 4 , subnet[i] );
	for ( int i = 0; i < 4; i++ ) EEPROM.write( i + 8 , gw[i] );
	for ( int i = 0; i < 4; i++ ) EEPROM.write( i + 12, remoteIP[i] );
	for ( int i = 0; i < 2; i++ ) EEPROM.write( i + 16, portPtr[i] );
	EEPROM.commit();
	LOG_NOTICE( "WIF", "Config stored: IP=" << ip.toString() << ", Subnet=" << subnet.toString() << ", Gw=" << gw.toString() << ", Remote IP=" << remoteIP.toString() << ", Remote Port=" << remotePort );
}



/* At every boot the IP information is loaded from EEPROM */
void MyWifi::loadConfig() {
	EEPROM.begin( 512 );
	byte* portPtr = (byte*) &remotePort;
	for ( int i = 0; i < 4; i++ ) ip[i] = EEPROM.read( i + 0 );
	for ( int i = 0; i < 4; i++ ) subnet[i] = EEPROM.read( i + 4 );
	for ( int i = 0; i < 4; i++ ) gw[i] = EEPROM.read( i + 8 );
	for ( int i = 0; i < 4; i++ ) remoteIP[i] = EEPROM.read( i + 12 );
	for ( int i = 0; i < 2; i++ ) portPtr[i] = EEPROM.read( i + 16 );
	LOG_NOTICE( "WIF", "Config loaded: IP=" << ip.toString() << ", Subnet=" << subnet.toString() << ", Gw=" << gw.toString() << ", Remote IP=" << remoteIP.toString() << ", Remote Port=" << remotePort );
}



/* Sends provided data to server */
bool MyWifi::send( const void * data, uint16_t length ) {
	LOG_NOTICE( "WIF", "Making TCP connection to " << remoteIP.toString() << ", Port " << remotePort );

	if ( !client.connect( remoteIP, remotePort ) ) {
		LOG_ERROR( "WIF", "Connection to server failed" );
		return false;
	}

	LOG_NOTICE( "WIF", "Sending " << length << " bytes of data" );
	uint16_t bytesSent = client.write( (char*) data, length );

	client.stop();
	if ( bytesSent == length ) {
		LOG_NOTICE( "WIF", "Data sent successfully" );
		return true;
	} else {
		LOG_ERROR( "WIF", "Could not send data" );
		return false;
	}
}