// Desk alarm - TEST 3: sensor + buzzers + LCD roasts
// If the HC-SR04 hasn't seen you within DESK_CM for GONE_AFTER_MS:
//   - active buzzer blares nonstop
//   - passive buzzer does a fast, rising siren sweep
//   - LCD jumps straight to a new roast
// The LCD rotates a random roast every 10 seconds the whole time.
// Open Tools > Serial Monitor at 9600 baud to see the distance readings.

// LCD is switched off for now. Change 0 to 1 to turn it back on.
#define USE_LCD 0

#if USE_LCD
#include <LiquidCrystal.h>
// LCD pins: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
#endif

// ---------- Pins ----------

const int TRIG_PIN   = 9;   // HC-SR04 Trig
const int ECHO_PIN   = 10;  // HC-SR04 Echo
const int ACTIVE_BZ  = 8;   // active buzzer +  (on/off only)
const int PASSIVE_BZ = 13;  // passive buzzer + (moved from 12, the LCD uses 12 now)
// A0 is left unconnected on purpose: its random noise seeds random()

// ---------- Alarm settings ----------
// Bodies and clothes are soft and curved, so the sensor only gets a clean
// echo off a person some of the time. Instead of needing every reading to
// see you, ONE good reading counts as "you're here" for GONE_AFTER_MS.
const int DESK_CM = 120;                 // "at desk" if anything is closer than this
const unsigned long GONE_AFTER_MS = 3000; // no hits for this long = you left

// true  = buzzers go off when the sensor SEES something (easy hand test)
// false = buzzers go off when you're GONE (the real procrastination alarm)
const bool ALARM_WHEN_SEEN = true;
const int HAND_CM = 30;   // hand-test mode: only things closer than this set it off

const int SWEEP_LOW_HZ  = 1200;
const int SWEEP_HIGH_HZ = 4000;  // these little buzzers are loudest around 2-4 kHz
const int SWEEP_STEP_HZ = 200;
const unsigned long SWEEP_STEP_MS = 4;   // smaller = faster sweep

const unsigned long SENSOR_EVERY_MS = 60;  // HC-SR04 needs ~60 ms between pings

#if USE_LCD
// ---------- LCD messages ----------
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
const unsigned long MESSAGE_EVERY_MS = 10000; // 10 seconds
#endif

// ---------- State ----------
unsigned long lastSeen = 0;
bool alarmOn = false;
int sweepHz = SWEEP_LOW_HZ;
unsigned long lastSweep = 0;
unsigned long lastPing = 0;
unsigned long lastMessageTime = 0;
int lastMessage = -1;

long readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);  // 0 = no echo
  if (duration == 0) return -1;
  return duration / 58;
}

#if USE_LCD
// The LCD is only 16 characters wide, so longer messages
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
  if (split == 0) split = 16;              // no space: hard cut

  lcd.setCursor(0, 0);
  for (int i = 0; i < split; i++) lcd.write(msg[i]);

  int start = (msg[split] == ' ') ? split + 1 : split;
  lcd.setCursor(0, 1);
  for (int i = start; i < len && i < start + 16; i++) lcd.write(msg[i]);
}

void displayRandomMessage() {
  int newMessage;
  do {                                     // no repeats in a row
    newMessage = random(messageCount);
  } while (newMessage == lastMessage);
  lastMessage = newMessage;
  printWrapped(messages[newMessage]);
  lastMessageTime = millis();
}
#endif

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(ACTIVE_BZ, OUTPUT);
  pinMode(PASSIVE_BZ, OUTPUT);
  Serial.begin(9600);
  Serial.println("Desk alarm test: sensor + buzzers + LCD");

  // Startup self-test: each buzzer on its own for half a second
  Serial.println("Self-test: ACTIVE buzzer (D8)...");
  digitalWrite(ACTIVE_BZ, HIGH);
  delay(500);
  digitalWrite(ACTIVE_BZ, LOW);
  delay(300);
  Serial.println("Self-test: PASSIVE buzzer (D13)...");
  tone(PASSIVE_BZ, 2500);
  delay(500);
  noTone(PASSIVE_BZ);
  Serial.println("Self-test done.");

#if USE_LCD
  lcd.begin(16, 2);
  randomSeed(analogRead(A0));
  displayRandomMessage();
#endif
  lastSeen = millis() - GONE_AFTER_MS;  // start out as "nobody seen yet"
}

void loop() {
  unsigned long now = millis();

  // 1) Check the sensor every SENSOR_EVERY_MS
  if (now - lastPing >= SENSOR_EVERY_MS) {
    lastPing = now;
    long cm = readDistanceCm();
    bool atDesk = (cm > 0 && cm < DESK_CM);

    if (atDesk) lastSeen = now;

    bool wasOn = alarmOn;
    if (ALARM_WHEN_SEEN) {
      // Hand test: instant on/off, only for something really close
      alarmOn = (cm > 0 && cm < HAND_CM);
    } else {
      // Real alarm: you're gone after GONE_AFTER_MS with no hits
      alarmOn = (now - lastSeen >= GONE_AFTER_MS);
    }
#if USE_LCD
    if (alarmOn && !wasOn) displayRandomMessage();   // roast them right away
#endif

    Serial.print(cm < 0 ? String("out of range") : String(cm) + " cm");
    Serial.print(atDesk ? "  HIT " : "  miss");
    Serial.println(alarmOn ? "  -> ALARM!" : "  -> quiet");
  }

#if USE_LCD
  // 2) Rotate the LCD message every 10 seconds
  if (millis() - lastMessageTime >= MESSAGE_EVERY_MS) {
    displayRandomMessage();
  }
#endif

  // 3) Drive the buzzers
  if (alarmOn) {
    digitalWrite(ACTIVE_BZ, HIGH);              // nonstop blare
    if (millis() - lastSweep >= SWEEP_STEP_MS) {
      lastSweep = millis();
      sweepHz += SWEEP_STEP_HZ;
      if (sweepHz > SWEEP_HIGH_HZ) sweepHz = SWEEP_LOW_HZ;  // snap back and climb again
      tone(PASSIVE_BZ, sweepHz);
    }
  } else {
    digitalWrite(ACTIVE_BZ, LOW);
    noTone(PASSIVE_BZ);
    sweepHz = SWEEP_LOW_HZ;
  }
}
