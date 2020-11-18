/***************************************************
  Adafruit MQTT Library ESP8266 Example

  Must use ESP8266 Arduino from:
    https://github.com/esp8266/Arduino

  Works great with Adafruit's Huzzah ESP board & Feather
  ----> https://www.adafruit.com/product/2471
  ----> https://www.adafruit.com/products/2821

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Tony DiCola for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "DHTesp.h"
#include "Metro.h"
#include "Streaming.h"

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "Looney"
#define WLAN_PASS       "TinyandTooney"

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "magister"
#define AIO_KEY         "aio_VEwa41p2bvYUTAHgqNyWOnHZP1Qy"

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiClientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish temp = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temp");

/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

DHTesp dht;

float humidity;
float temperature;

void setup() {
  Serial.begin(115200);
  delay(10);

  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, LOW);
  
  String thisBoard= ARDUINO_BOARD;
  Serial.println(thisBoard);

  dht.setup(D4, DHTesp::DHT11); // Connect DHT sensor to D4

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(dht.getMinimumSamplingPeriod() * 2);
    humidity = dht.getHumidity();
    temperature = dht.getTemperature();
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

}

uint32_t x=0;

void loop() {
  // readings.
  static Metro sampleTimer(dht.getMinimumSamplingPeriod() * 2);
  if( sampleTimer.check() ) {
    humidity = dht.getHumidity();
    temperature = dht.getTemperature();

    Serial.print(dht.getStatusString());
    Serial.print("\t\tH:");
    Serial.print(humidity, 1);
    Serial.print("\t\tT:");
    Serial.print(dht.toFahrenheit(temperature), 1);
    Serial.println();

  }

  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  static Metro mqttConnect(10000UL);
  if( mqttConnect.check() && !mqtt.connected() ) {
    Serial << "Trying to connect to MQTT broker..."  << endl;
    mqtt.connect();
  }
 
  if (mqtt.connected()) {
    Serial << "MQTT Connected." << endl;
    
    // Now we can publish stuff!
    Serial.print(F("\nSending temp val "));
    Serial.print(dht.toFahrenheit(temperature), 1);
    Serial.print("   ...");
    if (! temp.publish(dht.toFahrenheit(temperature))) {
      Serial.println(F("Failed"));
    } else {
      Serial.println(F("OK!"));

      /*
      delay(5000);
      temp.publish(dht.toFahrenheit(temperature));
      
      delay(5000);
      temp.publish(dht.toFahrenheit(temperature));
      */
 
      Serial.println("sleeping for 300 seconds....");

      delay(5000);
      digitalWrite(BUILTIN_LED, HIGH);
 //     temp.publish(dht.toFahrenheit(temperature));

      ESP.deepSleep(300e6); // wake up the module every 10 seconds
    }

  }

  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  /*
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
  */
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

  static uint8_t retries = 100;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       retries--;
       if (retries == 0) {
          return;
       }
  }
  Serial.println("MQTT Connected!");
}
