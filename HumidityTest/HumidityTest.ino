// DHT11 temperature alarm
//   Temp at or above TEMP_LIMIT_F -> quiet
//   Temp below TEMP_LIMIT_F       -> both buzzers on
// Open Tools > Serial Monitor at 9600 baud.
//
// Needs the "DHT sensor library" by Adafruit (Library Manager > "Install all").
//
// Wiring:
//   DHT11 module: S -> A4, middle -> 5V, - -> GND
//   Active buzzer:  + -> D8,  other leg -> GND
//   Passive buzzer: + -> D13, other leg -> GND

#include <DHT.h>

// ---------- Pins ----------
const int DHT_PIN    = A4;
const int ACTIVE_BZ  = 8;
const int PASSIVE_BZ = 13;
DHT dht(DHT_PIN, DHT11);

// ============ SETTINGS ============
const float TEMP_LIMIT_F = 80.0;   // at or above = quiet, below = buzz
const bool  SOUND_ON     = true;   // false = silent testing
const unsigned long READ_EVERY_MS = 2000;   // DHT11 is slow
// ==================================

// Siren sweep (passive buzzer)
const int SWEEP_LOW_HZ  = 1200;
const int SWEEP_HIGH_HZ = 4000;
const int SWEEP_STEP_HZ = 200;
const unsigned long SWEEP_STEP_MS = 4;

// ---------- State ----------
bool alarmOn = false;
bool haveReading = false;
unsigned long lastRead = 0;
int sweepHz = SWEEP_LOW_HZ;
unsigned long lastSweep = 0;

void setup() {
  pinMode(ACTIVE_BZ, OUTPUT);
  pinMode(PASSIVE_BZ, OUTPUT);
  Serial.begin(9600);
  Serial.println("DHT11 temperature alarm starting...");
  Serial.print("Quiet at or above ");
  Serial.print(TEMP_LIMIT_F, 1);
  Serial.println(" F, buzzers below it.");

  dht.begin();
  delay(1000);                    // let the DHT11 settle
  Serial.println("temp(F)  humidity  status");
}

void loop() {
  unsigned long now = millis();

  // 1) Read the sensor every READ_EVERY_MS
  if (now - lastRead >= READ_EVERY_MS || !haveReading) {
    lastRead = now;
    float h = dht.readHumidity();
    float f = dht.readTemperature(true);   // true = Fahrenheit

    if (isnan(h) || isnan(f)) {
      Serial.println("Read failed - check wiring (S to A4, + to 5V, - to GND)");
    } else {
      haveReading = true;
      alarmOn = (f < TEMP_LIMIT_F);

      Serial.print("  ");
      Serial.print(f, 1);
      Serial.print("     ");
      Serial.print(h, 0);
      Serial.print(" %     ");
      Serial.println(alarmOn ? "BELOW LIMIT - BUZZING" : "warm enough - quiet");
    }
  }

  // 2) Drive the buzzers
  if (alarmOn && SOUND_ON) {
    digitalWrite(ACTIVE_BZ, HIGH);              // nonstop blare
    if (millis() - lastSweep >= SWEEP_STEP_MS) {
      lastSweep = millis();
      sweepHz += SWEEP_STEP_HZ;
      if (sweepHz > SWEEP_HIGH_HZ) sweepHz = SWEEP_LOW_HZ;
      tone(PASSIVE_BZ, sweepHz);
    }
  } else {
    digitalWrite(ACTIVE_BZ, LOW);
    noTone(PASSIVE_BZ);
    sweepHz = SWEEP_LOW_HZ;
  }
}
