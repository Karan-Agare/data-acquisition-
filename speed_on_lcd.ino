#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- LCD ----------
LiquidCrystal_I2C lcd(0x27, 16, 2);   // change to 0x3F if needed

// ---------- Sensor ----------
#define IR_SENSOR_PIN 2
#define PULSES_PER_REV 4

// ---------- Wheel ----------
#define WHEEL_DIAMETER_M 0.44

// ---------- Timing ----------
#define SAMPLE_INTERVAL_MS 150

// ---------- Speedometer Filter ----------
#define FAST_ALPHA 0.6
#define SLOW_ALPHA 0.15

volatile unsigned long pulseCount = 0;
unsigned long lastSampleTime = 0;

float rpmFiltered = 0.0;
float totalDistance_m = 0.0;

// ---------- ISR ----------
void pulseISR() {
  pulseCount++;
}

// ---------- Setup ----------
void setup() {
  pinMode(IR_SENSOR_PIN, INPUT_PULLUP);
  Serial.begin(9600);

  attachInterrupt(
    digitalPinToInterrupt(IR_SENSOR_PIN),
    pulseISR,
    FALLING
  );

  // LCD init
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Speed & Distance");
}

// ---------- Loop ----------
void loop() {
  unsigned long now = millis();

  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {

    noInterrupts();
    unsigned long pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    float dt = SAMPLE_INTERVAL_MS / 1000.0;

    // --- RPM (internal) ---
    float revolutions = pulses / (float)PULSES_PER_REV;
    float rpmRaw = (revolutions / dt) * 60.0;

    // --- Speedometer smoothing ---
    float alpha = (rpmRaw > rpmFiltered) ? FAST_ALPHA : SLOW_ALPHA;
    rpmFiltered += alpha * (rpmRaw - rpmFiltered);

    // --- Speed ---
    float wheelCirc = 3.14159 * WHEEL_DIAMETER_M;
    float speed_kmph = (rpmFiltered / 60.0) * wheelCirc * 3.6;

    // --- Distance ---
    totalDistance_m += revolutions * wheelCirc;
    float totalDistance_km = totalDistance_m / 1000.0;

    // ---------- LCD DISPLAY ----------
    lcd.setCursor(0, 0);
    lcd.print("SPD:");
    lcd.print(speed_kmph, 1);
    lcd.print(" km/h ");
    lcd.print("      ");

    lcd.setCursor(0, 1);
    lcd.print("DST:");
    lcd.print(totalDistance_km, 2);
    lcd.print(" km   ");

    // ---------- Serial (optional debug) ----------
    Serial.print("Speed: ");
    Serial.print(speed_kmph);
    Serial.print(" km/h | Dist: ");
    Serial.print(totalDistance_km);
    Serial.println(" km");

    lastSampleTime = now;
  }
}
