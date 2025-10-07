
const int VOLTAGE_SENSOR_PIN = A0; // Pin connected to your voltage sensor's output.
const int CURRENT_SENSOR_PIN = A1; // Pin connected to your current sensor's output.

// --- SENSOR MAPPING ---
// Find these values in your sensor module's datasheet.

// What is the MAXIMUM voltage your sensor can read (this is the voltage when it outputs 5V)?
const float MAX_VOLTAGE_READING = 84.0; // Example: 100V for an 84V system.

// What is the MAXIMUM current your sensor can read?
const float MAX_CURRENT_READING = 160.0; // Example: 200A for a 160A system.

// Is your current sensor BIDIRECTIONAL?
// - true:  Measures current in two directions (charge/discharge). 0A is at 2.5V.
// - false: Measures current in one direction (discharge only). 0A is at 0V.
const bool IS_CURRENT_SENSOR_BIDIRECTIONAL = true;

// --- SYSTEM VOLTAGE ---
// For best accuracy, measure your Arduino's 5V pin and enter the exact value.
const float ARDUINO_VREF = 5.0;

// --- MEASUREMENT SETTINGS ---
const int NUM_SAMPLES = 100; // Number of samples to average for each reading.
const unsigned long PRINT_INTERVAL_MS = 1000; // Update interval in milliseconds.

//======================================================================
// ▲▲▲ END OF USER CONFIGURATION ▲▲▲
//======================================================================

// Global variable for timing
unsigned long previousMillis = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial); // Wait for Serial Monitor to open

  Serial.println("Initializing Battery Monitor (0-5V Direct Sensor Mode)");
  Serial.println("-----------------------------------------------------");
}

void loop() {
  // Use a non-blocking timer based on millis()
  if (millis() - previousMillis >= PRINT_INTERVAL_MS) {
    previousMillis = millis(); // Update the timer

    // --- Perform Averaged Voltage Measurement ---
    long voltage_raw_total = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
      voltage_raw_total += analogRead(VOLTAGE_SENSOR_PIN);
    }
    float voltage_raw_avg = (float)voltage_raw_total / NUM_SAMPLES;
    
    // Map the 0-1023 ADC value directly to the 0-MAX_VOLTAGE_READING range
    float battery_voltage = (voltage_raw_avg / 1023.0) * MAX_VOLTAGE_READING;


    // --- Perform Averaged Current Measurement ---
    long current_raw_total = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
      current_raw_total += analogRead(CURRENT_SENSOR_PIN);
    }
    float current_raw_avg = (float)current_raw_total / NUM_SAMPLES;
    
    // Map the ADC value to the current range based on sensor type
    float battery_current = 0.0;
    if (IS_CURRENT_SENSOR_BIDIRECTIONAL) {
      // For bidirectional, 511.5 is the zero point (2.5V).
      // Map -MAX to +MAX current across the 0-1023 range.
      battery_current = (current_raw_avg - 511.5) * (MAX_CURRENT_READING / 511.5);
    } else {
      // For unidirectional, map 0-1023 directly to 0-MAX current.
      battery_current = (current_raw_avg / 1023.0) * MAX_CURRENT_READING;
    }
    
    // --- Calculate Power in Watts ---
    float power_watts = battery_voltage * battery_current;

    // --- Print the final values to the Serial Monitor ---
    printValues(battery_voltage, battery_current, power_watts);
  }
}

/**
 * @brief Prints the formatted sensor data to the Serial Monitor.
 */
void printValues(float voltage, float current, float power) {
  Serial.print("Voltage: ");
  Serial.print(voltage, 2); // Print with 2 decimal places
  Serial.print(" V  |  ");

  Serial.print("Current: ");
  Serial.print(current, 2);
  Serial.print(" A  |  ");

  Serial.print("Power: ");
  Serial.print(power, 2);
  Serial.println(" W");
}
