#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>

// Define pins for GPS
static const int RXPin = 4, TXPin = 3;
static const uint32_t GPSBaud = 9600;

// Create TinyGPS++ object
TinyGPSPlus gps;

// Create serial connection for GPS
SoftwareSerial gpsSerial(RXPin, TXPin);

void setup() {
  Serial.begin(9600);
  gpsSerial.begin(GPSBaud);

  Serial.println("GPS Module Initialized...");
  Serial.println("Waiting for GPS signal...");
}

void loop() {
  // Read data from GPS module
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      displayGPSData();
    }
  }

  // If no data received in 5 seconds, print message
  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println("No GPS detected. Check wiring.");
    while (true);
  }
}

void displayGPSData() {
  if (gps.location.isValid()) {
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);

    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);

    Serial.print("Altitude (meters): ");
    Serial.println(gps.altitude.meters());

    Serial.print("Satellites: ");
    Serial.println(gps.satellites.value());

    Serial.print("Speed (km/h): ");
    Serial.println(gps.speed.kmph());

    Serial.println("---------------------------");
  } else {
    Serial.println("Waiting for valid GPS data...");
   
  }
  delay(500);
}
