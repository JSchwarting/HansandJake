// DHT11 temperature alarm + LCD
//   Temp at or above TEMP_LIMIT_F -> quiet, LCD shows temp and humidity
//   Temp below TEMP_LIMIT_F       -> both buzzers on, LCD shows roasts
// Open Tools > Serial Monitor at 9600 baud.
//
// Needs the "DHT sensor library" by Adafruit (Library Manager > "Install all").
// LiquidCrystal comes built in with the Arduino IDE.
//
// Wiring:
//   DHT11 module: S -> A4, middle -> 5V, - -> GND
//   Active buzzer:  + -> D8,  other leg -> GND
//   Passive buzzer: + -> D13, other leg -> GND
//   LCD1602: see the wiring page (RS D12, E D11, D4-D7 on D5, D4, D3, D2)

#include <DHT.h>
#include <LiquidCrystal.h>

// ---------- Pins ----------
const int DHT_PIN    = A4;
const int ACTIVE_BZ  = 8;
const int PASSIVE_BZ = 13;
// LCD pins: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
DHT dht(DHT_PIN, DHT11);

// ============ SETTINGS ============
const float TEMP_LIMIT_F = 80.0;   // at or above = quiet, below = buzz
const bool  SOUND_ON     = true;   // false = silent testing
const unsigned long READ_EVERY_MS    = 2000;   // DHT11 is slow
const unsigned long MESSAGE_EVERY_MS = 10000;  // new roast every 10 s
// ==================================

// Siren sweep (passive buzzer)
const int SWEEP_LOW_HZ  = 1200;
const int SWEEP_HIGH_HZ = 4000;
const int SWEEP_STEP_HZ = 200;
const unsigned long SWEEP_STEP_MS = 4;

const char* messages[] = {
  "THE COUNCIL WATCHES",
  "YOUR ANCESTORS WEEP",
  "LOCK IN OR PERISH",
  "THE GRIND DEMANDS",
  "WEAKNESS DETECTED",
  "SKILL ISSUE",
  "NPC BEHAVIOR",
  "ABSOLUTE CINEMA",
  "BRO IS STRUGGLING",
  "DIGNITY: -100",
  "MOTIVATION.EXE",
  "PEAK DISAPPOINTMENT"
};
const int messageCount = sizeof(messages) / sizeof(messages[0]);

// ---------- State ----------
bool alarmOn = false;
bool haveReading = false;
float lastTempF = 0, lastHumidity = 0;
unsigned long lastRead = 0;
unsigned long lastMessageTime = 0;
int lastMessage = -1;
int sweepHz = SWEEP_LOW_HZ;
unsigned long lastSweep = 0;

// The LCD is 16 characters wide, so longer messages
// get split onto the second line at the last space that fits.
void printWrapped(const char* msg) {
  lcd.clear();
  int len = strlen(msg);
  if (len <= 16) {
    lcd.setCursor(0, 0);
    lcd.print(msg);
    return;
  }
  int split = 16;
  while (split > 0 && msg[split] != ' ') split--;
  if (split == 0) split = 16;
  lcd.setCursor(0, 0);
  for (int i = 0; i < split; i++) lcd.write(msg[i]);
  int start = (msg[split] == ' ') ? split + 1 : split;
  lcd.setCursor(0, 1);
  for (int i = start; i < len && i < start + 16; i++) lcd.write(msg[i]);
}

void showRandomRoast() {
  int newMessage;
  do {
    newMessage = random(messageCount);
  } while (newMessage == lastMessage);
  lastMessage = newMessage;
  printWrapped(messages[newMessage]);
  lastMessageTime = millis();
}

void showReadings() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(lastTempF, 1);
  lcd.print((char)223);          // degree symbol on this LCD
  lcd.print("F");
  lcd.setCursor(0, 1);
  lcd.print("Humidity: ");
  lcd.print(lastHumidity, 0);
  lcd.print("%");
}

void setup() {
  pinMode(ACTIVE_BZ, OUTPUT);
  pinMode(PASSIVE_BZ, OUTPUT);
  Serial.begin(9600);
  Serial.println("DHT11 temperature alarm starting...");
  Serial.print("Quiet at or above ");
  Serial.print(TEMP_LIMIT_F, 1);
  Serial.println(" F, buzzers below it.");

  dht.begin();
  lcd.begin(16, 2);
  randomSeed(analogRead(A0));     // A0 left unconnected on purpose
  lcd.print("Warming up...");
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
      if (!haveReading) {
        lcd.clear();
        lcd.print("Sensor error");
        lcd.setCursor(0, 1);
        lcd.print("Check A4 wiring");
      }
    } else {
      haveReading = true;
      lastTempF = f;
      lastHumidity = h;

      bool wasOn = alarmOn;
      alarmOn = (lastTempF < TEMP_LIMIT_F);

      Serial.print("  ");
      Serial.print(lastTempF, 1);
      Serial.print("     ");
      Serial.print(lastHumidity, 0);
      Serial.print(" %     ");
      Serial.println(alarmOn ? "BELOW LIMIT - BUZZING" : "warm enough - quiet");

      if (alarmOn && !wasOn) showRandomRoast();   // roast right away
      if (!alarmOn) showReadings();               // refresh numbers
    }
  }

  // 2) While buzzing, rotate roasts every 10 s
  if (alarmOn && millis() - lastMessageTime >= MESSAGE_EVERY_MS) {
    showRandomRoast();
  }

  // 3) Drive the buzzers
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
