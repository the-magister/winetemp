// Compile for Adafruit Feather HUZZAH ESP8266.
// 80 MHz

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
#include "ArduinoJson.h"
#include "DHTesp.h"
#include "Metro.h"
#include "Streaming.h"

/************************* WiFi Access Point *********************************/

#define WLAN_SSID "Looney"
#define WLAN_PASS "TinyandTooney"

/************************* Adafruit.io Setup *********************************/

#define SERVER "broker.hivemq.com"
#define SERVERPORT 1883  // use 8883 for SSL
#define CLIENTNAME "magister-weather-wine"

#define SEND_INTERVAL (300UL * 1000UL)

DHTesp dht;

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiClientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, SERVER, SERVERPORT, CLIENTNAME);

/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.
Adafruit_MQTT_Publish pubData = Adafruit_MQTT_Publish(&mqtt, "magister/weather/sensor/indoor/wine");

/*************************** Sketch Code ************************************/

void setup() {
  Serial.begin(115200);
  delay(10);

  String thisBoard = ARDUINO_BOARD;
  Serial.println(thisBoard);

  dht.setup(D4, DHTesp::DHT11);  // Connect DHT sensor to D4

  // Connect to WiFi access point.
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    TempAndHumidity values = dht.getTempAndHumidity();
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial << "Setup.  complete." << endl;

  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);  // light on.
}

void loop() {

  // Ensure the connection to the MQTT server is alive (this will make the first
  MQTT_connect();

  if (mqtt.connected()) {
    // get data
    TempAndHumidity values = dht.getTempAndHumidity();

    // Manually assign struct members to JSON keys
    StaticJsonDocument<200> doc;
    doc["Temperature"] = values.temperature;
    doc["Humidity"] = values.humidity;
    doc["Interval"] = SEND_INTERVAL/1000UL;

    char output[200];
    serializeJson(doc, output, sizeof(output));

    // Now we can publish stuff!
    Serial << "Sending: " << output << " ... ";
    if (!pubData.publish(output)) {
      Serial << "Failed!" << endl;
    } else {
      Serial << "Success." << endl;
    }
  } 

  delay(SEND_INTERVAL);
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    digitalWrite(BUILTIN_LED, LOW);

    return;
  }
  digitalWrite(BUILTIN_LED, HIGH);

  Serial.print("Connecting to MQTT... ");

  static uint8_t retries = 100;
  while ((ret = mqtt.connect()) != 0) {  // connect will return 0 for connected
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
