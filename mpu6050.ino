#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define MPU 0x68
#define HMC 0x1E

LiquidCrystal_I2C lcd(0x27, 20, 4);   // Change to 0x3F if needed

// MPU6050 values
float AccX, AccY, AccZ;
float GyroX, GyroY, GyroZ;

float roll = 0;
float pitch = 0;
float yaw = 0;

float accAngleX;
float accAngleY;

float elapsedTime;
float currentTime;
float previousTime;

// Magnetometer values
int16_t mx, my, mz;
float heading;

// LCD update timer
unsigned long lcdUpdateTime = 0;
const unsigned long lcdInterval = 200;

void setup()
{
  Serial.begin(19200);

  Wire.begin();

  // LCD Initialization
  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print("Telemetry DAQ");
  lcd.setCursor(3,1);
  lcd.print("Initializing");
  delay(1500);
  lcd.clear();

  //---------------- MPU6050 ----------------//

  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission(true);

  // Accelerometer ±2g
  Wire.beginTransmission(MPU);
  Wire.write(0x1C);
  Wire.write(0x00);
  Wire.endTransmission(true);

  // Gyroscope ±250°/s
  Wire.beginTransmission(MPU);
  Wire.write(0x1B);
  Wire.write(0x00);
  Wire.endTransmission(true);

  //---------------- HMC5883L ----------------//

  Wire.beginTransmission(HMC);
  Wire.write(0x00);
  Wire.write(0x70);
  Wire.endTransmission();

  Wire.beginTransmission(HMC);
  Wire.write(0x01);
  Wire.write(0xA0);
  Wire.endTransmission();

  Wire.beginTransmission(HMC);
  Wire.write(0x02);
  Wire.write(0x00);
  Wire.endTransmission();

  delay(100);
}

void loop()
{
  //---------------- ACCELEROMETER ----------------//

  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);

  Wire.requestFrom(MPU, 6, true);

  AccX = (Wire.read() << 8 | Wire.read()) / 16384.0;
  AccY = (Wire.read() << 8 | Wire.read()) / 16384.0;
  AccZ = (Wire.read() << 8 | Wire.read()) / 16384.0;

  accAngleX = atan(AccY / sqrt(AccX * AccX + AccZ * AccZ)) * 180.0 / PI;
  accAngleY = atan(-AccX / sqrt(AccY * AccY + AccZ * AccZ)) * 180.0 / PI;

  //---------------- GYROSCOPE ----------------//

  previousTime = currentTime;
  currentTime = millis();
  elapsedTime = (currentTime - previousTime) / 1000.0;

  Wire.beginTransmission(MPU);
  Wire.write(0x43);
  Wire.endTransmission(false);

  Wire.requestFrom(MPU, 6, true);

  GyroX = (Wire.read() << 8 | Wire.read()) / 131.0;
  GyroY = (Wire.read() << 8 | Wire.read()) / 131.0;
  GyroZ = (Wire.read() << 8 | Wire.read()) / 131.0;

  //---------------- COMPLEMENTARY FILTER ----------------//

  roll = 0.96 * (roll + GyroX * elapsedTime) + 0.04 * accAngleX;
  pitch = 0.96 * (pitch + GyroY * elapsedTime) + 0.04 * accAngleY;

  //---------------- MAGNETOMETER ----------------//

  Wire.beginTransmission(HMC);
  Wire.write(0x03);
  Wire.endTransmission(false);

  Wire.requestFrom(HMC, 6, true);

  mx = Wire.read() << 8 | Wire.read();
  mz = Wire.read() << 8 | Wire.read();
  my = Wire.read() << 8 | Wire.read();

  float rollRad = roll * PI / 180.0;
  float pitchRad = pitch * PI / 180.0;

  float Xh = mx * cos(pitchRad) + mz * sin(pitchRad);

  float Yh = mx * sin(rollRad) * sin(pitchRad)
           + my * cos(rollRad)
           - mz * cos(pitchRad) * sin(rollRad);

  heading = atan2(Yh, Xh) * 180.0 / PI;

  if (heading < 0)
    heading += 360.0;

  //---------------- YAW FILTER ----------------//

  float gyroYaw = yaw + GyroZ * elapsedTime;
  yaw = 0.98 * gyroYaw + 0.02 * heading;

  //---------------- OFFSETS ----------------//

  float rollValue = roll + 3.63;
  float pitchValue = pitch + 12.0;
  float yawValue = yaw - 233.0;

  //---------------- SERIAL ----------------//

  Serial.print("Roll : ");
  Serial.print(rollValue, 2);

  Serial.print("   Pitch : ");
  Serial.print(pitchValue, 2);

  Serial.print("   Yaw : ");
  Serial.println(yawValue, 2);

  //---------------- LCD DISPLAY ----------------//

  if (millis() - lcdUpdateTime >= lcdInterval)
  {
    lcdUpdateTime = millis();

    lcd.setCursor(0,0);
    lcd.print("ROLL : ");
    lcd.print(rollValue,1);
    lcd.print((char)223);
    lcd.print("    ");

    lcd.setCursor(0,1);
    lcd.print("PITCH: ");
    lcd.print(pitchValue,1);
    lcd.print((char)223);
    lcd.print("    ");

    lcd.setCursor(0,2);
    lcd.print("YAW  : ");
    lcd.print(yawValue,1);
    lcd.print((char)223);
    lcd.print("    ");

    lcd.setCursor(0,3);
    lcd.print("MPU6050 + HMC5883L ");
  }

  delay(10);
}
