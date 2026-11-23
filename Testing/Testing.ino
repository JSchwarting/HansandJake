// Desk alarm - TEST 4: real desk mode
// The HC-SR04 watches your chair. If it hasn't seen you within DESK_CM
// for GONE_AFTER_MS, the alarm goes off:
//   - active buzzer blares nonstop
//   - passive buzzer does a fast, rising siren sweep
// Open Tools > Serial Monitor at 9600 baud for live readings.

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
const int PASSIVE_BZ = 13;  // passive buzzer + (driven with tone())
// A0 is left unconnected on purpose: its random noise seeds random()

// ============ DESK SETTINGS - tune these ============
// Sit normally and watch the Serial Monitor, then set DESK_CM about
// 20 cm above the distance you usually read.
const int DESK_CM = 20;                   // "at desk" if anything is closer than this (our sensor tops out ~20 cm)
const unsigned long GONE_AFTER_MS = 5000; // no hits for this long = you left
const unsigned long STARTUP_GRACE_MS = 10000; // time to sit down after power-on
const bool SOUND_ON = true;               // false = silent testing, monitor only
// =====================================================

// Siren sweep settings (passive buzzer)
const int SWEEP_LOW_HZ  = 1200;
const int SWEEP_HIGH_HZ = 4000;  // these little buzzers are loudest around 2-4 kHz
const int SWEEP_STEP_HZ = 200;
const unsigned long SWEEP_STEP_MS = 4;   // smaller = faster sweep

const unsigned long SENSOR_EVERY_MS = 60;   // HC-SR04 needs ~60 ms between pings
const unsigned long REPORT_EVERY_MS = 500;  // Serial Monitor summary twice a second

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
unsigned long lastMessageTime = 0;
int lastMessage = -1;
#endif

// ---------- State ----------
bool alarmOn = false;
unsigned long lastSeen = 0;
int sweepHz = SWEEP_LOW_HZ;
unsigned long lastSweep = 0;
unsigned long lastPing = 0;
unsigned long lastReport = 0;
long lastCm = -1;
int pings = 0, hits = 0;              // counted per report window
unsigned long awayStart = 0;
unsigned long totalAwayMs = 0;        // the procrastination metric
int timesLeft = 0;

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

void printDuration(unsigned long ms) {
  unsigned long s = ms / 1000;
  Serial.print(s / 60);
  Serial.print("m ");
  Serial.print(s % 60);
  Serial.print("s");
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
  Serial.println("=== Desk alarm: real desk mode ===");
  Serial.print("DESK_CM = ");       Serial.print(DESK_CM);
  Serial.print("   GONE_AFTER = "); Serial.print(GONE_AFTER_MS / 1000);
  Serial.print("s   SOUND = ");     Serial.println(SOUND_ON ? "on" : "off");

  if (SOUND_ON) {
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
  }

#if USE_LCD
  lcd.begin(16, 2);
  randomSeed(analogRead(A0));
  displayRandomMessage();
#endif

  // Count as "seen" right now so you get STARTUP_GRACE_MS to sit down
  lastSeen = millis() + STARTUP_GRACE_MS - GONE_AFTER_MS;
  Serial.print("You have ");
  Serial.print(STARTUP_GRACE_MS / 1000);
  Serial.println("s to sit down...");
  Serial.println("dist(cm)  hit-rate  status   last-seen  | away total / times left");
}

void loop() {
  unsigned long now = millis();

  // 1) Check the sensor
  if (now - lastPing >= SENSOR_EVERY_MS) {
    lastPing = now;
    lastCm = readDistanceCm();
    bool hit = (lastCm > 0 && lastCm < DESK_CM);
    pings++;
    if (hit) { hits++; lastSeen = now; }

    bool wasOn = alarmOn;
    alarmOn = ((long)(now - lastSeen) >= (long)GONE_AFTER_MS);

    if (alarmOn && !wasOn) {
      awayStart = now;
      timesLeft++;
      Serial.println(">>> YOU LEFT THE DESK - ALARM ON <<<");
#if USE_LCD
      displayRandomMessage();   // roast them right away
#endif
    } else if (!alarmOn && wasOn) {
      totalAwayMs += now - awayStart;
      Serial.print(">>> BACK AT DESK after ");
      printDuration(now - awayStart);
      Serial.println(" <<<");
    }
  }

  // 2) Summary line twice a second
  if (now - lastReport >= REPORT_EVERY_MS) {
    lastReport = now;
    if (lastCm < 0) Serial.print("  ---   ");
    else { Serial.print("  "); Serial.print(lastCm); Serial.print(" cm  "); }
    Serial.print("   ");
    Serial.print(pings ? (hits * 100 / pings) : 0);
    Serial.print("%      ");
    Serial.print(alarmOn ? "GONE " : "DESK ");
    Serial.print("   ");
    long since = (long)(now - lastSeen);
    Serial.print(since < 0 ? 0 : since / 1000.0, 1);
    Serial.print("s ago   | ");
    unsigned long away = totalAwayMs + (alarmOn ? now - awayStart : 0);
    printDuration(away);
    Serial.print(" / ");
    Serial.println(timesLeft);
    pings = 0;
    hits = 0;
  }

#if USE_LCD
  // 3) Rotate the LCD message every 10 seconds
  if (millis() - lastMessageTime >= MESSAGE_EVERY_MS) {
    displayRandomMessage();
  }
#endif

  // 4) Drive the buzzers
  if (alarmOn && SOUND_ON) {
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
