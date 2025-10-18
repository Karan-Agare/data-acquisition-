// Define the analog pin connected to the potentiometer
const int potPin = A0;

// --- Moving Average Settings ---
const int numReadings = 10;
int readings[numReadings];      
int readIndex = 0;              
long total = 0;                  
int average = 0;                  
// --- End of Settings ---

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  
  // Initialize all readings in the array to 0
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }
}

void loop() {
  // --- Moving Average Calculation ---
  total = total - readings[readIndex];
  readings[readIndex] = analogRead(potPin);
  total = total + readings[readIndex];
  readIndex = readIndex + 1;
  
  if (readIndex >= numReadings) {
    readIndex = 0;
  }
  
  average = total / numReadings;
  // --- End of Calculation ---

  // Convert the averaged ADC value to a voltage (0-5V)
  float sensorVoltage = average * (5.0 / 1023.0);

  // Apply the precise linear function derived from your data
  float mappedVoltage = 12.4932 * sensorVoltage + 21.8604;

  // --- NEW CONDITION ---
  // If the calculated mapped voltage is less than 20, set it to 0.
  if (average  <=  20) {
    mappedVoltage = 0;
  }
  // --- END OF CONDITION ---

  // Print the final results to the Serial Monitor
  Serial.print("Averaged ADC: ");
  Serial.print(average);
  Serial.print("\t| Final Voltage: ");
  Serial.print(mappedVoltage, 2); // Print with 2 decimal places
  Serial.println(" V");

  delay(50);
}
