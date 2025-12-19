// Compile for LOLIN(WEMOS) D1 mini pro (ESP8266)
// 80 MHz

#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "ArduinoJson.h"
#include "DHTesp.h"
#include "Metro.h"
#include "Streaming.h"

#define WLAN_SSID "Looney"
#define WLAN_PASS "TinyandTooney"

#define SERVER "broker.hivemq.com"
#define SERVERPORT 1883  // use 8883 for SSL
#define CLIENTNAME "magister-weather-wine"

#define SEND_INTERVAL (300UL * 1000UL)

DHTesp dht;

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, SERVER, SERVERPORT, CLIENTNAME);

void setup() {
  Serial.begin(115200);
  Serial.flush();
  delay(100UL);

  Serial << endl << endl << "Startup." << endl;

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);  // light on.

  String thisBoard = ARDUINO_BOARD;
  Serial.println(thisBoard);

  // Connect DHT sensor to D4
  dht.setup(D4, DHTesp::DHT11);
  // get data
  TempAndHumidity values = dht.getTempAndHumidity();

  // Manually assign struct members to JSON keys
  StaticJsonDocument<200> doc;
  doc["Temperature"] = values.temperature;
  doc["Humidity"] = values.humidity;
  doc["Interval"] = SEND_INTERVAL / 1000UL;

  char output[200];
  serializeJson(doc, output, sizeof(output));

  // Now we can publish stuff!
  Serial << "Sending: " << output << endl;

  // Connect to WiFi access point.
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) delay(1);

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.print("Connecting to MQTT... ");
  static uint8_t retries = 100;
  int8_t ret;
  while ((ret = mqtt.connect()) != 0) {  // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000UL);
    retries--;
    if (retries == 0) {
      return;
    }
  }
  Serial.println("MQTT Connected!");

  // Setup a feed  for publishing.
  String pub = "magister/weather/sensor/" + String(WiFi.macAddress());
  Adafruit_MQTT_Publish pubData = Adafruit_MQTT_Publish(&mqtt, pub.c_str());

  Serial << "Publishing to " << pub << endl;
  if (!pubData.publish(output)) {
    Serial << "Publish Failed!" << endl;
  } else {
    Serial << "Publish Success." << endl;
  }

  digitalWrite(LED_BUILTIN, HIGH);  // light off.
  delay(5000UL);
  Serial << "Sleeping...." << endl;

  ESP.deepSleep(SEND_INTERVAL * 1000UL); // microseconds

}

void loop() {
  // never reached
  Serial << "Didn't sleep!" << endl;
  delay(500);
}
