// Desk alarm - TEST 2: sensor + buzzers, alarm when you're GONE
// If the HC-SR04 does NOT see you within DESK_CM, both buzzers go off:
//   - active buzzer blares nonstop
//   - passive buzzer does a fast, rising siren sweep
// Open Tools > Serial Monitor at 9600 baud to see the distance readings.

const int TRIG_PIN   = 9;   // HC-SR04 Trig
const int ECHO_PIN   = 10;  // HC-SR04 Echo
const int ACTIVE_BZ  = 8;   // active buzzer +  (on/off only)
const int PASSIVE_BZ = 12;  // passive buzzer + (driven with tone())

const int DESK_CM       = 80;  // you count as "at desk" if closer than this
const int MISSES_TO_ARM = 3;   // empty readings in a row before the alarm starts

// Siren sweep settings (passive buzzer)
const int SWEEP_LOW_HZ  = 1200;
const int SWEEP_HIGH_HZ = 4000;  // these little buzzers are loudest around 2-4 kHz
const int SWEEP_STEP_HZ = 200;
const unsigned long SWEEP_STEP_MS = 4;   // smaller = faster sweep

const unsigned long SENSOR_EVERY_MS = 100;

int misses = 0;
bool alarmOn = false;
int sweepHz = SWEEP_LOW_HZ;
unsigned long lastSweep = 0;
unsigned long lastPing = 0;

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

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(ACTIVE_BZ, OUTPUT);
  pinMode(PASSIVE_BZ, OUTPUT);
  Serial.begin(9600);
  Serial.println("Desk alarm test: alarm when nobody is at the desk");
}

void loop() {
  unsigned long now = millis();

  // 1) Check the sensor every SENSOR_EVERY_MS
  if (now - lastPing >= SENSOR_EVERY_MS) {
    lastPing = now;
    long cm = readDistanceCm();
    bool atDesk = (cm > 0 && cm < DESK_CM);

    if (atDesk) {
      misses = 0;
    } else if (misses < MISSES_TO_ARM) {
      misses++;
    }
    alarmOn = (misses >= MISSES_TO_ARM);

    Serial.print(cm < 0 ? String("out of range") : String(cm) + " cm");
    Serial.println(alarmOn ? "  -> GONE, ALARM!" : "  -> at desk");
  }

  // 2) Drive the buzzers
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
