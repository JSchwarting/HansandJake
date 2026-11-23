// DHT11 humidity + temperature test
// Prints humidity, temperature and heat index every 2 seconds.
// Open Tools > Serial Monitor at 9600 baud.
//
// Needs the "DHT sensor library" by Adafruit:
//   Sketch > Include Library > Manage Libraries... > search "DHT sensor library"
//   > Install (click "Install all" so it also gets "Adafruit Unified Sensor").
//
// Wiring (Elegoo DHT11 module, pins labeled S / middle / -):
//   S   (signal) -> A4
//   middle (+)   -> 5V
//   -   (ground) -> GND

#include <DHT.h>

const int DHT_PIN = A4;          // A4 is free in the desk alarm pin map
DHT dht(DHT_PIN, DHT11);

const unsigned long READ_EVERY_MS = 2000;  // DHT11 can only update about once a second

void setup() {
  Serial.begin(9600);
  Serial.println("DHT11 test starting...");
  dht.begin();
  delay(1000);                   // let the sensor settle after power-up
  Serial.println("humidity  temp(C)  temp(F)  heat-index(F)");
}

void loop() {
  float humidity = dht.readHumidity();
  float tempC    = dht.readTemperature();
  float tempF    = dht.readTemperature(true);

  if (isnan(humidity) || isnan(tempC) || isnan(tempF)) {
    Serial.println("Read failed - check wiring (S to A4, + to 5V, - to GND)");
  } else {
    float heatF = dht.computeHeatIndex(tempF, humidity);
    Serial.print("  ");
    Serial.print(humidity, 0);
    Serial.print(" %     ");
    Serial.print(tempC, 1);
    Serial.print("     ");
    Serial.print(tempF, 1);
    Serial.print("     ");
    Serial.println(heatF, 1);
  }

  delay(READ_EVERY_MS);
}
