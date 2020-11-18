#include "DHTesp.h"

#ifdef ESP32
#pragma message(THIS EXAMPLE IS FOR ESP8266 ONLY!)
#error Select ESP8266 board.
#endif

DHTesp dht;

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  String thisBoard= ARDUINO_BOARD;
  Serial.println(thisBoard);

  // Autodetect is not working reliable, don't use the following line
  // dht.setup(17);
  // use this instead: 
  dht.setup(D4, DHTesp::DHT11); // Connect DHT sensor to D4
}

void loop()
{
  static byte loops = 0;
  delay(dht.getMinimumSamplingPeriod() * 2);

  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();
  int8_t digits = dht.getNumberOfDecimalsTemperature()+1;
  loops++;
  
  Serial.print(loops);
  Serial.print("\t\t");
  Serial.print(dht.getStatusString());
  Serial.print("\t\tH:");
  Serial.print(humidity, digits);
  Serial.print("\t\tT:");
//  Serial.print(temperature, digits);
//  Serial.print("\t\t");
  Serial.print(dht.toFahrenheit(temperature), digits);
  Serial.print("\t\t");
//  Serial.print(dht.computeHeatIndex(temperature, humidity, false), digits);
//  Serial.print("\t\t");
//  Serial.println(dht.computeHeatIndex(dht.toFahrenheit(temperature), humidity, true), digits);
  Serial.println();

  if(loops >=3 ) {
  }
}
