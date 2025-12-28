#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- LCD ----------
LiquidCrystal_I2C lcd(0x27, 16, 4);

// ---------- COLUMN OFFSET (FIX) ----------
#define COL_OFFSET -4   // shift text 4 tiles left

// ---------- Thermistor Pins ----------
#define T1_PIN A0
#define T2_PIN A1
#define T3_PIN A2
#define T4_PIN A3

// ---------- Thermistor Constants ----------
const float R_FIXED = 10000.0;
const float R0      = 10000.0;
const float BETA    = 3950.0;
const float T0      = 298.15;

// ---------- ROW MAPPING ----------
uint8_t mapRow(uint8_t row) {
  const uint8_t rowMap[4] = {0, 1, 2, 3};
  return rowMap[row];
}

// ---------- Temperature Reading ----------
float readTempC(int pin) {
  int adc = analogRead(pin);
  if (adc <= 0) return -273.15;

  float voltage = adc * (5.0 / 1023.0);
  float resistance = R_FIXED * (5.0 / voltage - 1.0);
  float tempK = 1.0 / ((1.0 / T0) + (1.0 / BETA) * log(resistance / R0));
  return tempK - 273.15;
}

// ---------- Clear Row ----------
void clearRow(uint8_t row) {
  lcd.setCursor(COL_OFFSET, mapRow(row));
  lcd.print("                ");
}

// ---------- Print Temperature ----------
void printTemp(uint8_t row, const char* label, float value) {
  clearRow(row);
  lcd.setCursor(COL_OFFSET, mapRow(row));
  lcd.print(label);
  lcd.print(": ");
  lcd.print(value, 1);
  lcd.print((char)223);
  lcd.print("C");
}

// ---------- Setup ----------
void setup() {
  lcd.init();
  lcd.backlight();

  clearRow(0);
  lcd.setCursor(COL_OFFSET, mapRow(0));
  lcd.print("Thermal Monitor");
}

// ---------- Loop ----------
void loop() {
  printTemp(0, "T1", readTempC(T1_PIN));
  printTemp(1, "T2", readTempC(T2_PIN));
  printTemp(2, "T3", readTempC(T3_PIN));
  printTemp(3, "T4", readTempC(T4_PIN));

  delay(1000);
}
