# DATA ACQUISITION #

We are implementing DAQ to monitor the motor temp, battery temp, motorcontroller temp, vehicle rpm, motor rpm, GPS COORDINATES, 


## ENERGY METER ## 


## BMS DTC ERRORS MONITORING 
In our vehicle system, the Orion BMS2 (Battery Management System) is responsible for monitoring the health, safety, and performance of the high-voltage accumulator. Using the MCP2515 CAN module, we continuously read error frames and status flags sent by the BMS on the CAN network.

Real time debugging rather than manually plugging the orion bms 2 Can adaptor and checking for the error that may have occurred. 
Below are the following errors that Orion bms lets us monitor. Along with 5 custom monitoring parameters. 

Bit #1 (0x0001): P0A07 (Discharge Limit Enforcement Fault)
Bit #2 (0x0002): P0A08 (Charger Safety Relay Fault)
Bit #3 (0x0004): P0A09 (Internal Hardware Fault)
Bit #4 (0x0008): P0A0A (Internal Heatsink Thermistor Fault)
Bit #5 (0x0010): P0A0B (Internal Software Fault)
Bit #6 (0x0020): P0A0C (Highest Cell Voltage Too High Fault)
Bit #7 (0x0040): P0A0E (Lowest Cell Voltage Too Low Fault)
Bit #8 (0x0080): P0A10 (Pack Too Hot Fault)
Bit #1 (0x0001): P0A1F (Internal Communication Fault)
Bit #2 (0x0002): P0A12 (Cell Balancing Stuck Off Fault)
Bit #3 (0x0004): P0A80 (Weak Cell Fault)
Bit #4 (0x0008): P0AFA (Low Cell Voltage Fault)
Bit #5 (0x0010): P0A04 (Open Wiring Fault)
Bit #6 (0x0020): P0AC0 (Current Sensor Fault)
Bit #7 (0x0040): P0A0D (Highest Cell Voltage Over 5V Fault)
Bit #8 (0x0080): P0A0F (Cell ASIC Fault)
Bit #9 (0x0100): P0A02 (Weak Pack Fault)
Bit #10 (0x0200): P0A81 (Fan Monitor Fault)
Bit #11 (0x0400): P0A9C (Thermistor Fault)
Bit #12 (0x0800): U0100 (External Communication Fault)
Bit #13 (0x1000): P0560 (Redundant Power Supply Fault)
Bit #14 (0x2000): P0AA6 (High Voltage Isolation Fault)
Bit #15 (0x4000): P0A05 (Input Power Supply Fault)
Bit #16 (0x8000): P0A06 (Charge Limit Enforcement Fault



## IMD MONITORING ##
IMD BENDOR ERRORS BEING MONITORED
0 Hz
High → Short-circuit to Ub + (Kl. 15)
Low → IMD off or short-circuit to Kl. 31

10 Hz – Normal Condition
Insulation measurement (DCP)
Starts 2 seconds after power-on
First successful insulation measurement ≤ 17.5 s
PWM active 5–95%

20 Hz – Undervoltage Condition
Insulation measurement DCP (continuous)
Starts 2 seconds after power-on
PWM active 5–95%
First successful insulation measurement ≤ 17.5 s
Undervoltage detection range: 0–500 V (Bender configurable)

30 Hz – Speed Start Measurement
Insulation measurement (good/bad evaluation only)
Starts immediately after power-on (≤ 2 s)
PWM: 5–10% (good), 90–95% (bad)

40 Hz – Device Error
Device error detected
PWM 47.5–52.5%

50 Hz – Connection Fault to Earth
Fault detected on earth connection (Kl. 31)
PWM 47.5–52.5%

DATASHEET OF BENDER PCB https://www.bender.de/fileadmin/content/Products/d/e/IR155-32xx-V004_D00115_D_XXEN.pdf
