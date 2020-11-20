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
#define KEY1            "aio_mPQT80dLABXry"
#define KEY2            "NgWV9Md7mLQTQA8"

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiClientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
String key1 = KEY1;
String key2 = KEY2;
String key = key1+key2;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, key.c_str());

/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish temp = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temp");

Adafruit_MQTT_Subscribe sleep = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/sleep");
Adafruit_MQTT_Publish sleepGet = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/sleep/get");

/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

DHTesp dht;
float humidity, temperature;

void setup() {
  Serial.begin(115200);
  delay(10);

  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, LOW);

  String thisBoard = ARDUINO_BOARD;
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

  // sub
  Serial << "Subscribing MQTT sleep message...." << endl;
  mqtt.subscribe(&sleep);

  Serial << "Setup.  complete." << endl;
}

uint32_t x = 0;

void loop() {

  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  static Metro mqttConnect(10000UL);
  if ( mqttConnect.check() && !mqtt.connected() ) {
    Serial << "Trying to connect to MQTT broker..."  << endl;
    mqtt.connect();
  }

  if (mqtt.connected()) {
    Serial << "MQTT Connected." << endl;

    Serial << "MQTT sleepGet..." << endl;
    // https://io.adafruit.com/api/docs/mqtt.html#using-the-get-topic
    while (!sleepGet.publish("?")) {
      delay(5000);
    }

    float sleepUS = 3600e6;
    Serial << "MQTT sleep message..." << endl;
    Metro waitForMessage(30000);
    waitForMessage.reset();

    while (!waitForMessage.check()) {
      Adafruit_MQTT_Subscribe *subscription;
      subscription = mqtt.readSubscription(5000);
      // check if its the sleep feed
      if (subscription == &sleep) {
        Serial.print(F("Sleep: "));
        Serial.println((char *)sleep.lastread);

        uint16_t sleepHours = atoi((char *)sleep.lastread);  // convert to a number
        sleepHours = constrain(sleepHours, 1, 24);
        Serial << sleepHours << " hours\t";

        sleepUS = (float)sleepHours * 60.0 * 60.0 * 1e6; // h -> m -> s -> us
        Serial << sleepUS << " microseconds\t";


        Serial << endl;

        break;
      }
    }

    // Now we can publish stuff!
    Serial.print(F("Sending temp val "));
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

      Serial.print("sleeping for ");
      Serial << sleepUS / 1e6 << " seconds....";

      delay(5000);
      digitalWrite(BUILTIN_LED, HIGH);

      ESP.deepSleep(sleepUS); // wake up the module later...
      delay(1000);
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
