#include <mcp_can.h>
#include <mcp_can_dfs.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <WiFiEsp.h>

// ==================== WIFI CONFIGURATION ====================
char ssid[] = "YOUR_WIFI_SSID";            // Put your WiFi Name here
char pass[] = "YOUR_WIFI_PASSWORD";        // Put your WiFi Password here
int status = WL_IDLE_STATUS;

SoftwareSerial espSerial(6, 7);            // ESP8266 TX -> Pin 6, RX -> Pin 7
WiFiEspServer server(80);                  // Web Server on Port 80

// ==================== CAN BUS CONFIGURATION ====================
const int SPI_CS_PIN = 10;                 // CANBed V1 CS Pin
unsigned char len = 8;
unsigned char buf[8] = {0};
long unsigned int rxId;
char msgString[128];

MCP_CAN CAN(SPI_CS_PIN);                   // Set CS pin

// ==================== GLOBAL VARIABLES FOR WEB PAGE ====================
int web_pack_current = 0;
int web_pack_inst_voltage = 0;
int web_soc = 0;
int web_high_temp = 0;
int web_low_temp = 0;

int web_dtc_a = 0;                         // Stores buf[0] from 2147485361 or 1745
int web_dtc_b = 0;                         // Stores buf[2] from 2147485361 or 1745
int web_dtc_c = 0;                         // Stores buf[3] from 2147485361 or 1745

unsigned long last_data_rxId = 0;
unsigned long last_fault_rxId = 0;
unsigned char last_buf[8] = {0};

// ==================== YOUR HELPER FUNCTIONS ====================
void decToBinary(int decimalNum, int binaryArray[], int arraySize) {
  // Initialize binary array to all zeros
  for (int i = 0; i < arraySize; i++) {
    binaryArray[i] = 0;
  }

  // Convert decimal number to binary and store in array
  int i = 0;
  while (decimalNum > 0 && i < arraySize) {
    binaryArray[i] = decimalNum % 2;
    decimalNum /= 2;
    i++;
  }
}

int hexToDec(String hexString) {
  int decimalValue = 0;
  int len = hexString.length();
  for (int i = 0; i < len; i++) {
    char c = hexString.charAt(i);
    int digit;
    if (isDigit(c)) {
      digit = c - '0';
    } else {
      c = toupper(c);
      digit = c - 'A' + 10;
    }
    decimalValue += round(digit * pow(16, len - i - 1));
  }
  return decimalValue;
}

String decToHex(int decimalValue) {
  if (decimalValue == 0) return "00";
  String hexString = "";
  while (decimalValue > 0) {
    int remainder = decimalValue % 16;
    char hexDigit;
    if (remainder < 10) {
      hexDigit = '0' + remainder;
    } else {
      hexDigit = 'A' + remainder - 10;
    }
    hexString = String(hexDigit) + hexString;
    decimalValue /= 16;
  }
  if (hexString.length() == 1) {
    hexString = "0" + hexString;           // Preserves leading zero (e.g. 0x05 -> "05")
  }
  return hexString;
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(9600);
  espSerial.begin(9600);                   // ESP8266 AT baud rate (must be 9600)
  while (!Serial);

  // 1. Initialize CAN Bus
  if (CAN_OK != CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ)) {
    Serial.println(F("CAN BUS FAIL!"));
    while (1) delay(100);
  } else {
    Serial.println(F("CAN BUS OK!"));
    delay(500);
  }
  CAN.setMode(MCP_NORMAL);
  pinMode(2, INPUT);

  // 2. Initialize ESP8266 WiFi
  WiFi.init(&espSerial);

  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println(F("ESP8266 WiFi module not detected. Check wiring and baud rate."));
  } else {
    while (status != WL_CONNECTED) {
      Serial.print(F("Connecting to WiFi SSID: "));
      Serial.println(ssid);
      status = WiFi.begin(ssid, pass);
    }
    Serial.println(F("WiFi Connected!"));
    Serial.print(F("Open this IP in your browser: http://"));
    Serial.println(WiFi.localIP());
    server.begin();
  }
}

// ==================== MAIN LOOP ====================
void loop() {
  if (!digitalRead(2)) {
    CAN.readMsgBuf(&rxId, &len, buf);      // read data, len: data length, buf: data buf

    for (int i = 0; i < 8; i++) {
      last_buf[i] = buf[i];
    }

    Serial.println(F("-----------------------------"));
    Serial.print(F("Get data from ID: "));
    Serial.println(rxId);

    for (int i = 0; i < len; i++) {        // print the data
      Serial.print(buf[i], HEX);
      Serial.print(F("\t"));
    }
    Serial.println();

    // =========================================================================
    // CODE 1 - CAN ADDRESS 1: 2147485360 (DATA PARAMETERS)
    // =========================================================================
    if (rxId == 2147485360UL) {
      last_data_rxId = rxId;

      // CURRENT
      String c1 = decToHex(buf[0]);
      String c2 = decToHex(buf[1]);
      String c = c1 + c2;
      int pack_current = (hexToDec(c)) / 10;
      web_pack_current = pack_current;
      Serial.println(F("Pack Current-"));
      Serial.println(pack_current);

      // VOLTAGE
      String v1 = decToHex(buf[2]);
      String v2 = decToHex(buf[3]);
      String v = v1 + v2;
      int pack_inst_voltage = (hexToDec(v)) / 10;
      web_pack_inst_voltage = pack_inst_voltage;
      Serial.print(F("Pack Inst. Voltage-"));
      Serial.println(pack_inst_voltage);

      // STATE OF CHARGE
      web_soc = buf[4] / 2;
      Serial.print(F("State Of Charge-"));
      Serial.println(web_soc);

      // TEMPERATURE
      web_high_temp = buf[5];
      web_low_temp = buf[6];
      Serial.print(F("High Temperature-"));
      Serial.println(buf[5]);
      Serial.print(F("Low Temperature-"));
      Serial.println(buf[6]);
    }

    // =========================================================================
    // CODE 1 - CAN ADDRESS 2: 2147485361 (DTC FAULTS / ERRORS)
    // =========================================================================
    if (rxId == 2147485361UL) {
      last_fault_rxId = rxId;
      web_dtc_a = buf[0];
      web_dtc_b = buf[2];
      web_dtc_c = buf[3];

      // DTC FLAGS #1
      int a = (buf[0]);
      int flag1[8];
      decToBinary(a, flag1, 8);

      for (int i = 0; i < 8; i++) {
        if ((flag1[i] == 1) && (i == 7)) { Serial.println(F("P0A10 (Pack Too Hot Fault)")); continue; }
        if ((flag1[i] == 1) && (i == 6)) { Serial.println(F("P0A0E (Lowest Cell Voltage Too Low Fault)")); continue; }
        if ((flag1[i] == 1) && (i == 5)) { Serial.println(F("P0A0C (Highest Cell Voltage Too High Fault)")); continue; }
        if ((flag1[i] == 1) && (i == 4)) { Serial.println(F("P0A0B (Internal Software Fault)")); continue; }
        if ((flag1[i] == 1) && (i == 3)) { Serial.println(F("P0A0A (Internal Heatsink Thermistor Fault)")); continue; }
        if ((flag1[i] == 1) && (i == 2)) { Serial.println(F("P0A09 (Internal Hardware Fault)")); continue; }
        if ((flag1[i] == 1) && (i == 1)) { Serial.println(F("P0A08 (Charger Safety Relay Fault)")); continue; }
        if ((flag1[i] == 1) && (i == 0)) { Serial.println(F("P0A07 (Discharge Limit Enforcement Fault)")); continue; }
      }

      // DTC FLAG #2
      int b = (buf[2]);
      int flag2[8];
      decToBinary(b, flag2, 8);

      for (int i = 0; i < 8; i++) {
        if ((flag2[i] == 1) && (i == 7)) { Serial.println(F("P0A0F (Cell ASIC Fault)")); continue; }
        if ((flag2[i] == 1) && (i == 6)) { Serial.println(F("P0A0D (Highest Cell Voltage Over 5V Fault)")); continue; }
        if ((flag2[i] == 1) && (i == 5)) { Serial.println(F("P0AC0 (Current Sensor Fault)")); continue; }
        if ((flag2[i] == 1) && (i == 4)) { Serial.println(F("P0A04 (Open Wiring Fault)")); continue; }
        if ((flag2[i] == 1) && (i == 3)) { Serial.println(F("P0AFA (Low Cell Voltage Fault)")); continue; }
        if ((flag2[i] == 1) && (i == 2)) { Serial.println(F("P0A80 (Weak Cell Fault)")); continue; }
        if ((flag2[i] == 1) && (i == 1)) { Serial.println(F("P0A12 (Cell Balancing Stuck Off Fault)")); continue; }
        if ((flag2[i] == 1) && (i == 0)) { Serial.println(F("P0A1F (Internal Communication Fault)")); continue; }
      }

      // DTC FLAG #3
      int c = (buf[3]);
      int flag3[8];
      decToBinary(c, flag3, 8);

      for (int i = 0; i < 8; i++) {
        if ((flag3[i] == 1) && (i == 7)) { Serial.println(F("P0A06 (Charge Limit Enforcement Fault)")); continue; }
        if ((flag3[i] == 1) && (i == 6)) { Serial.println(F("P0A05 (Input Power Supply Fault)")); continue; }
        if ((flag3[i] == 1) && (i == 5)) { Serial.println(F("P0AA6 (High Voltage Isolation Fault)")); continue; }
        if ((flag3[i] == 1) && (i == 4)) { Serial.println(F("P0560 (Redundant Power Supply Fault)")); continue; }
        if ((flag3[i] == 1) && (i == 3)) { Serial.println(F("U0100 (External Communication Fault)")); continue; }
        if ((flag3[i] == 1) && (i == 2)) { Serial.println(F("P0A9C (Thermistor Fault)")); continue; }
        if ((flag3[i] == 1) && (i == 1)) { Serial.println(F("P0A81 (Fan Monitor Fault)")); continue; }
        if ((flag3[i] == 1) && (i == 0)) { Serial.println(F("P0A02 (Weak Pack Fault)")); continue; }
      }
    }

    // =========================================================================
    // CODE 2 - CAN ADDRESS 3: 1744 (DATA PARAMETERS - CODED SEPARATELY)
    // =========================================================================
    if (rxId == 1744UL) {
      last_data_rxId = rxId;

      // CURRENT
      String c1 = decToHex(buf[0]);
      String c2 = decToHex(buf[1]);
      String c = c1 + c2;
      int pack_current = (hexToDec(c)) / 10;
      web_pack_current = pack_current;
      Serial.println(F("Pack Current-"));
      Serial.println(pack_current);

      // VOLTAGE
      String v1 = decToHex(buf[2]);
      String v2 = decToHex(buf[3]);
      String v = v1 + v2;
      int pack_inst_voltage = (hexToDec(v)) / 10;
      web_pack_inst_voltage = pack_inst_voltage;
      Serial.print(F("Pack Inst. Voltage-"));
      Serial.println(pack_inst_voltage);

      // STATE OF CHARGE
      web_soc = buf[4] / 2;
      Serial.print(F("State Of Charge-"));
      Serial.println(web_soc);

      // TEMPERATURE
      web_high_temp = buf[5];
      web_low_temp = buf[6];
      Serial.print(F("High Temperature-"));
      Serial.println(buf[5]);
      Serial.print(F("Low Temperature-"));
      Serial.println(buf[6]);
    }

    // =========================================================================
    // CODE 2 - CAN ADDRESS 4: 1745 (DTC FAULTS / ERRORS - CODED SEPARATELY)
    // =========================================================================
    if (rxId == 1745UL) {
      last_fault_rxId = rxId;
      web_dtc_a = buf[0];
      web_dtc_b = buf[2];
      web_dtc_c = buf[3];

      // DTC FLAGS #1
      int a = (buf[0]);
      int flag1[8];
      decToBinary(a, flag1, 8);

      for (int i = 0; i < 8; i++) {
        if ((flag1[i] == 1) && (i == 7)) { Serial.println(F("P0A10 (Pack Too Hot Fault)")); continue; }
        if ((flag1[i] == 1) && (i == 6)) { Serial.println(F("P0A0E (Lowest Cell Voltage Too Low Fault)")); continue; }
        if ((flag1[i] == 1) && (i == 5)) { Serial.println(F("P0A0C (Highest Cell Voltage Too High Fault)")); continue; }
        if ((flag1[i] == 1) && (i == 4)) { Serial.println(F("P0A0B (Internal Software Fault)")); continue; }
        if ((flag1[i] == 1) && (i == 3)) { Serial.println(F("P0A0A (Internal Heatsink Thermistor Fault)")); continue; }
        if ((flag1[i] == 1) && (i == 2)) { Serial.println(F("P0A09 (Internal Hardware Fault)")); continue; }
        if ((flag1[i] == 1) && (i == 1)) { Serial.println(F("P0A08 (Charger Safety Relay Fault)")); continue; }
        if ((flag1[i] == 1) && (i == 0)) { Serial.println(F("P0A07 (Discharge Limit Enforcement Fault)")); continue; }
      }

      // DTC FLAG #2
      int b = (buf[2]);
      int flag2[8];
      decToBinary(b, flag2, 8);

      for (int i = 0; i < 8; i++) {
        if ((flag2[i] == 1) && (i == 7)) { Serial.println(F("P0A0F (Cell ASIC Fault)")); continue; }
        if ((flag2[i] == 1) && (i == 6)) { Serial.println(F("P0A0D (Highest Cell Voltage Over 5V Fault)")); continue; }
        if ((flag2[i] == 1) && (i == 5)) { Serial.println(F("P0AC0 (Current Sensor Fault)")); continue; }
        if ((flag2[i] == 1) && (i == 4)) { Serial.println(F("P0A04 (Open Wiring Fault)")); continue; }
        if ((flag2[i] == 1) && (i == 3)) { Serial.println(F("P0AFA (Low Cell Voltage Fault)")); continue; }
        if ((flag2[i] == 1) && (i == 2)) { Serial.println(F("P0A80 (Weak Cell Fault)")); continue; }
        if ((flag2[i] == 1) && (i == 1)) { Serial.println(F("P0A12 (Cell Balancing Stuck Off Fault)")); continue; }
        if ((flag2[i] == 1) && (i == 0)) { Serial.println(F("P0A1F (Internal Communication Fault)")); continue; }
      }

      // DTC FLAG #3
      int c = (buf[3]);
      int flag3[8];
      decToBinary(c, flag3, 8);

      for (int i = 0; i < 8; i++) {
        if ((flag3[i] == 1) && (i == 7)) { Serial.println(F("P0A06 (Charge Limit Enforcement Fault)")); continue; }
        if ((flag3[i] == 1) && (i == 6)) { Serial.println(F("P0A05 (Input Power Supply Fault)")); continue; }
        if ((flag3[i] == 1) && (i == 5)) { Serial.println(F("P0AA6 (High Voltage Isolation Fault)")); continue; }
        if ((flag3[i] == 1) && (i == 4)) { Serial.println(F("P0560 (Redundant Power Supply Fault)")); continue; }
        if ((flag3[i] == 1) && (i == 3)) { Serial.println(F("U0100 (External Communication Fault)")); continue; }
        if ((flag3[i] == 1) && (i == 2)) { Serial.println(F("P0A9C (Thermistor Fault)")); continue; }
        if ((flag3[i] == 1) && (i == 1)) { Serial.println(F("P0A81 (Fan Monitor Fault)")); continue; }
        if ((flag3[i] == 1) && (i == 0)) { Serial.println(F("P0A02 (Weak Pack Fault)")); continue; }
      }
    }
  }

  // ==================== SERVE HTML WEB PAGE OVER WIFI ====================
  serveWebPage();
}

void serveWebPage() {
  WiFiEspClient client = server.available();
  if (client) {
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char ch = client.read();
        if (ch == '\n' && currentLineIsBlank) {
          // Standard HTTP Response Header
          client.println(F("HTTP/1.1 200 OK"));
          client.println(F("Content-Type: text/html"));
          client.println(F("Connection: close"));
          client.println();

          // HTML & CSS Layout
          client.println(F("<!DOCTYPE html><html><head><meta charset='UTF-8'>"));
          client.println(F("<meta http-equiv='refresh' content='2'>")); // Auto-refresh every 2s
          client.println(F("<title>BMS CAN Dashboard</title><style>"));
          client.println(F("body{font-family:Arial,sans-serif;background:#f0f2f5;margin:0;padding:20px;}"));
          client.println(F("h1{text-align:center;color:#222;margin-bottom:20px;}"));
          // Top Parameters Bar
          client.println(F(".top{display:flex;gap:12px;justify-content:space-between;margin-bottom:20px;flex-wrap:wrap;}"));
          client.println(F(".box{background:#fff;flex:1;min-width:130px;padding:15px;border-radius:8px;text-align:center;box-shadow:0 2px 4px rgba(0,0,0,0.1);border-top:4px solid #0066ff;}"));
          client.println(F(".box h3{margin:0;font-size:13px;color:#555;}.box p{margin:8px 0 0;font-size:22px;font-weight:bold;color:#111;}"));
          // Bottom Split Screen (Left = Data, Right = Errors)
          client.println(F(".row{display:flex;gap:20px;flex-wrap:wrap;}"));
          client.println(F(".left,.right{background:#fff;flex:1;min-width:300px;padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}"));
          client.println(F("table{width:100%;border-collapse:collapse;}th,td{padding:10px;border-bottom:1px solid #ddd;text-align:left;}"));
          client.println(F(".err{background:#ffe3e3;color:#c92a2a;padding:10px;margin-bottom:8px;border-left:4px solid #e03131;border-radius:4px;font-weight:bold;}"));
          client.println(F(".ok{background:#d3f9d8;color:#2b8a3e;padding:12px;border-left:4px solid #2f9e44;border-radius:4px;font-weight:bold;}"));
          client.println(F("</style></head><body>"));

          client.println(F("<h1>BMS CAN Bus Live Monitor</h1>"));

          // 1. ALL PARAMETERS ABOVE (TOP CARDS)
          client.println(F("<div class='top'>"));
          client.print(F("<div class='box'><h3>PACK CURRENT</h3><p>")); client.print(web_pack_current); client.println(F(" A</p></div>"));
          client.print(F("<div class='box'><h3>INST. VOLTAGE</h3><p>")); client.print(web_pack_inst_voltage); client.println(F(" V</p></div>"));
          client.print(F("<div class='box'><h3>STATE OF CHARGE</h3><p>")); client.print(web_soc); client.println(F(" %</p></div>"));
          client.print(F("<div class='box'><h3>HIGH TEMP</h3><p>")); client.print(web_high_temp); client.println(F(" &deg;C</p></div>"));
          client.print(F("<div class='box'><h3>LOW TEMP</h3><p>")); client.print(web_low_temp); client.println(F(" &deg;C</p></div>"));
          client.println(F("</div>"));

          // 2. SPLIT CONTAINER BELOW
          client.println(F("<div class='row'>"));

          // BOTTOM LEFT SIDE: DETAILED DATA TABLE
          client.println(F("<div class='left'><h2>Parameters &amp; CAN Data</h2><table>"));
          client.println(F("<tr><th>Parameter</th><th>Live Value</th></tr>"));
          client.print(F("<tr><td>Pack Current</td><td>")); client.print(web_pack_current); client.println(F(" A</td></tr>"));
          client.print(F("<tr><td>Pack Inst. Voltage</td><td>")); client.print(web_pack_inst_voltage); client.println(F(" V</td></tr>"));
          client.print(F("<tr><td>State Of Charge (SOC)</td><td>")); client.print(web_soc); client.println(F(" %</td></tr>"));
          client.print(F("<tr><td>High Temperature</td><td>")); client.print(web_high_temp); client.println(F(" &deg;C</td></tr>"));
          client.print(F("<tr><td>Low Temperature</td><td>")); client.print(web_low_temp); client.println(F(" &deg;C</td></tr>"));
          client.print(F("<tr><td>Data CAN ID</td><td>")); client.print(last_data_rxId); client.println(F("</td></tr>"));
          client.print(F("<tr><td>Fault CAN ID</td><td>")); client.print(last_fault_rxId); client.println(F("</td></tr>"));
          client.print(F("<tr><td>Raw CAN Buffer (Hex)</td><td>"));
          for (int i = 0; i < 8; i++) {
            client.print(decToHex(last_buf[i]));
            client.print(F(" "));
          }
          client.println(F("</td></tr></table></div>"));

          // BOTTOM RIGHT SIDE: ERRORS / DTC FAULTS
          client.println(F("<div class='right'><h2>Active Errors &amp; Faults</h2>"));

          if (web_dtc_a == 0 && web_dtc_b == 0 && web_dtc_c == 0) {
            client.println(F("<div class='ok'>No Errors Detected - System OK</div>"));
          } else {
            int flag1[8], flag2[8], flag3[8];
            decToBinary(web_dtc_a, flag1, 8);
            decToBinary(web_dtc_b, flag2, 8);
            decToBinary(web_dtc_c, flag3, 8);

            // Flag 1 Faults
            for (int i = 0; i < 8; i++) {
              if ((flag1[i] == 1) && (i == 7)) client.println(F("<div class='err'>P0A10 (Pack Too Hot Fault)</div>"));
              if ((flag1[i] == 1) && (i == 6)) client.println(F("<div class='err'>P0A0E (Lowest Cell Voltage Too Low Fault)</div>"));
              if ((flag1[i] == 1) && (i == 5)) client.println(F("<div class='err'>P0A0C (Highest Cell Voltage Too High Fault)</div>"));
              if ((flag1[i] == 1) && (i == 4)) client.println(F("<div class='err'>P0A0B (Internal Software Fault)</div>"));
              if ((flag1[i] == 1) && (i == 3)) client.println(F("<div class='err'>P0A0A (Internal Heatsink Thermistor Fault)</div>"));
              if ((flag1[i] == 1) && (i == 2)) client.println(F("<div class='err'>P0A09 (Internal Hardware Fault)</div>"));
              if ((flag1[i] == 1) && (i == 1)) client.println(F("<div class='err'>P0A08 (Charger Safety Relay Fault)</div>"));
              if ((flag1[i] == 1) && (i == 0)) client.println(F("<div class='err'>P0A07 (Discharge Limit Enforcement Fault)</div>"));
            }

            // Flag 2 Faults
            for (int i = 0; i < 8; i++) {
              if ((flag2[i] == 1) && (i == 7)) client.println(F("<div class='err'>P0A0F (Cell ASIC Fault)</div>"));
              if ((flag2[i] == 1) && (i == 6)) client.println(F("<div class='err'>P0A0D (Highest Cell Voltage Over 5V Fault)</div>"));
              if ((flag2[i] == 1) && (i == 5)) client.println(F("<div class='err'>P0AC0 (Current Sensor Fault)</div>"));
              if ((flag2[i] == 1) && (i == 4)) client.println(F("<div class='err'>P0A04 (Open Wiring Fault)</div>"));
              if ((flag2[i] == 1) && (i == 3)) client.println(F("<div class='err'>P0AFA (Low Cell Voltage Fault)</div>"));
              if ((flag2[i] == 1) && (i == 2)) client.println(F("<div class='err'>P0A80 (Weak Cell Fault)</div>"));
              if ((flag2[i] == 1) && (i == 1)) client.println(F("<div class='err'>P0A12 (Cell Balancing Stuck Off Fault)</div>"));
              if ((flag2[i] == 1) && (i == 0)) client.println(F("<div class='err'>P0A1F (Internal Communication Fault)</div>"));
            }

            // Flag 3 Faults
            for (int i = 0; i < 8; i++) {
              if ((flag3[i] == 1) && (i == 7)) client.println(F("<div class='err'>P0A06 (Charge Limit Enforcement Fault)</div>"));
              if ((flag3[i] == 1) && (i == 6)) client.println(F("<div class='err'>P0A05 (Input Power Supply Fault)</div>"));
              if ((flag3[i] == 1) && (i == 5)) client.println(F("<div class='err'>P0AA6 (High Voltage Isolation Fault)</div>"));
              if ((flag3[i] == 1) && (i == 4)) client.println(F("<div class='err'>P0560 (Redundant Power Supply Fault)</div>"));
              if ((flag3[i] == 1) && (i == 3)) client.println(F("<div class='err'>U0100 (External Communication Fault)</div>"));
              if ((flag3[i] == 1) && (i == 2)) client.println(F("<div class='err'>P0A9C (Thermistor Fault)</div>"));
              if ((flag3[i] == 1) && (i == 1)) client.println(F("<div class='err'>P0A81 (Fan Monitor Fault)</div>"));
              if ((flag3[i] == 1) && (i == 0)) client.println(F("<div class='err'>P0A02 (Weak Pack Fault)</div>"));
            }
          }

          client.println(F("</div></div></body></html>"));
          break;
        }
        if (ch == '\n') {
          currentLineIsBlank = true;
        } else if (ch != '\r') {
          currentLineIsBlank = false;
        }
      }
    }
    delay(10);
    client.stop();
  }
}
