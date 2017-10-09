/***************************************************
  Legal Weed Dude
  This sketch will read the temperature data from our
  last example and control a digital IO pin.
 

/***************************************************
  Adafruit MQTT Library ESP8266 Example

  Must use ESP8266 Arduino from:
    https://github.com/esp8266/Arduino

  Works great with Adafruit's Huzzah ESP board:
  ----> https://www.adafruit.com/product/2471

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Tony DiCola for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

//#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/************************* Low Level Defines *********************************/
#define ALPHA 13
#define BETA 12
#define TEMP_MAX 75

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "ssid"
#define WLAN_PASS       "pass"

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  8883                   // use 8883 for SSL use 1883 for MQTT
#define AIO_USERNAME    "you"
#define AIO_KEY         "key"

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feed called 'time' for subscribing to current time
Adafruit_MQTT_Subscribe timefeed = Adafruit_MQTT_Subscribe(&mqtt, "time/seconds");

// Setup a feed called 'alphafeed' for subscribing to changes on the alpha-override channel
Adafruit_MQTT_Subscribe alphafeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/alpha-override", MQTT_QOS_1);

// Setup a feed called 'temperature' for subscribing to changes to the temperature
Adafruit_MQTT_Subscribe tempfeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/temperature", MQTT_QOS_1);

Adafruit_MQTT_Publish statusfeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/status", MQTT_QOS_1);


/*************************** Sketch Code ************************************/
/*
int sec;
int min;
int hour;
int timeZone = -4; // utc-4 eastern daylight time (nyc)
void timecallback(uint32_t current) {

  // adjust to local time zone
  current += (timeZone * 60 * 60);

  // calculate current time
  sec = current % 60;
  current /= 60;
  min = current % 60;
  current /= 60;
  hour = current % 24;

  // print hour
  if(hour == 0 || hour == 12)
    Serial.print("12");
  if(hour < 12)
    Serial.print(hour);
  else
    Serial.print(hour - 12);

  // print mins
  Serial.print(":");
  if(min < 10) Serial.print("0");
  Serial.print(min);

  // print seconds
  Serial.print(":");
  if(sec < 10) Serial.print("0");
  Serial.print(sec);

  if(hour < 12)
    Serial.println(" am");
  else
    Serial.println(" pm");

} */
// set global status flags
bool too_hot = false;


// set timezone offset from UTC
int timeZone = -4; // UTC - 4 eastern daylight time (nyc)
int interval = 4; // trigger every X hours
int hour = 0; // current hour

void timecallback(uint32_t current) {

  // stash previous hour
  int previous = hour;

  // adjust to local time zone
  current += (timeZone * 60 * 60);

  // calculate current hour
  hour = (current / 60 / 60) % 24;

  // only trigger on interval
  if((hour != previous) && (hour % interval) == 0) {
    Serial.println("Run your code here");
  }

}
void tempcallback(double t) {
  Serial.print("Hey we're in the temperature callback, the temperature value is: ");
  Serial.println(t);
  if(t > TEMP_MAX){
    digitalWrite(ALPHA, LOW);
  }
  else{
    digitalWrite(ALPHA, HIGH);
  }
}


void alphacallback(char *data, uint16_t len) {
  Serial.print("Hey we're in an alpha channel call switch has moved to: ");
  Serial.println(data);
  //digitalWrite(ALPHA, data);
        
      if (strcmp(data, "ON") == 0) {
         digitalWrite(BETA, HIGH); 
      }
      if (strcmp(data, "OFF") == 0) {
         digitalWrite(BETA, LOW); 
      }
}


void setup() {
  // Set relay PINS
  pinMode(ALPHA, OUTPUT);
  pinMode(BETA, OUTPUT);
  
  Serial.begin(115200);
  delay(10);

  Serial.println(F("Adafruit MQTT demo"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  //timefeed.setCallback(timecallback);
  tempfeed.setCallback(tempcallback);
  alphafeed.setCallback(alphacallback);
  
  // Setup MQTT subscription for time feed.
  //mqtt.subscribe(&timefeed);
  mqtt.subscribe(&tempfeed);
  mqtt.subscribe(&alphafeed);

}

uint32_t x=0;

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets and callback em' busy subloop
  // try to spend your time here:
  mqtt.processPackets(5000);
  
  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }

  if(digitalRead(ALPHA)){
    statusfeed.publish(75);
  }
  else{
    statusfeed.publish(25);
  }
  
  Serial.println("TICK");
  
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 10 seconds...");
       mqtt.disconnect();
       delay(10000);  // wait 10 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
