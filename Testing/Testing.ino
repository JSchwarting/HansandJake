// Desk alarm - TEST 1: sensor + buzzers only
// If the HC-SR04 sees something closer than DETECT_CM, both buzzers go off.
// Open Tools > Serial Monitor at 9600 baud to see the distance readings.

const int TRIG_PIN   = 9;   // HC-SR04 Trig
const int ECHO_PIN   = 10;  // HC-SR04 Echo
const int ACTIVE_BZ  = 8;   // active buzzer +  (on/off only)
const int PASSIVE_BZ = 12;  // passive buzzer + (driven with tone())

const int DETECT_CM = 150;   // trigger distance in cm - change to taste

long readDistanceCm() {
  // Send a 10 microsecond ping
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Wait up to 30 ms for the echo (about 5 m). 0 means no echo.
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;
  return duration / 58;     // microseconds -> cm
}

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(ACTIVE_BZ, OUTPUT);
  pinMode(PASSIVE_BZ, OUTPUT);
  Serial.begin(9600);
  Serial.println("Desk alarm test: sensor + buzzers");
}

void loop() {
  long cm = readDistanceCm();

  if (cm < 0) {
    Serial.println("Distance: out of range");
  } else {
    Serial.print("Distance: ");
    Serial.print(cm);
    Serial.println(" cm");
  }

  bool detected = (cm > 0 && cm < DETECT_CM);

  if (detected) {
    digitalWrite(ACTIVE_BZ, HIGH);
    // Two-tone siren on the passive buzzer
    static bool high = false;
    high = !high;
    tone(PASSIVE_BZ, high ? 1500 : 900);
  } else {
    digitalWrite(ACTIVE_BZ, LOW);
    noTone(PASSIVE_BZ);
  }

  delay(150);
}
