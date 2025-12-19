// Compile for Adafruit Feather ESP32-S2
// Unit has a built-in BME280

#include <WiFi.h>

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "ArduinoJson.h"
#include "Metro.h"
#include "Streaming.h"

/************************* WiFi Access Point *********************************/

#define WLAN_SSID "Looney"
#define WLAN_PASS "TinyandTooney"

/************************* Adafruit.io Setup *********************************/

#define SERVER "broker.hivemq.com"
#define SERVERPORT 1883  // use 8883 for SSL
#define CLIENTNAME "magister-weather-other"

#define SEND_INTERVAL (300UL * 1000UL) // sec * ms/sec [=] ms

Adafruit_BME280 bme;  // I2C 0x77

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, SERVER, SERVERPORT, CLIENTNAME);

void setup() {
  Serial.begin(115200);
  Serial.flush();
  delay(100UL);

  Serial << endl << endl << "Startup." << endl;

  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);  // light on.

  String thisBoard = ARDUINO_BOARD;
  Serial.println(thisBoard);

  // default settings
  unsigned status = bme.begin();

  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
    Serial.print("SensorID was: 0x");
    Serial.println(bme.sensorID(), 16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
  }

  // sampling strategy
  bme.setSampling(Adafruit_BME280::MODE_SLEEP,
                  Adafruit_BME280::SAMPLING_X16, Adafruit_BME280::SAMPLING_X16, Adafruit_BME280::SAMPLING_X16,
                  Adafruit_BME280::FILTER_OFF,
                  Adafruit_BME280::STANDBY_MS_1000);

  // Manually assign struct members to JSON keys
  StaticJsonDocument<200> doc;
  doc["Temperature"] = bme.readTemperature();
  doc["Pressure"] = bme.readPressure() / 100.0F;
  doc["Humidity"] = bme.readHumidity();
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
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.print("Connecting to MQTT... ");
  static uint8_t retries = 100;
  int8_t ret;
  while ((ret=mqtt.connect()) != 0) {  // connect will return 0 for connected
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

  digitalWrite(BUILTIN_LED, LOW);  // light off.

  delay(5000UL);

  Serial << "Sleeping...." << endl;

  esp_sleep_enable_timer_wakeup(SEND_INTERVAL * 1000UL);  // microseconds
  esp_deep_sleep_start();
}

void loop() {
  // never reached
}
