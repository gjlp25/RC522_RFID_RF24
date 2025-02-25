# RFID Access Control with NRF24L01 Transmission

Battery-optimized Arduino project using RC522 RFID reader and NRF24L01 radio module for access control and wireless data transmission.

## Hardware Requirements

- Arduino Pro Mini 3.3V/8MHz
- MFRC522 RFID Reader
- NRF24L01 Radio Module
- LEDs (Green and Red)
- Buzzer
- Battery (3.3V compatible)

## Pin Configuration

### RFID RC522
- RST_PIN: 9   (Reset pin for RC522)
- SS_PIN: 8    (SDA/SS pin for SPI chip select)
- MISO: 12     (Master Input Slave Output - data from RC522 to Arduino)
- MOSI: 11     (Master Output Slave Input - data from Arduino to RC522)
- SCK: 13      (Serial Clock for SPI communication)
- 3.3V: VCC    (Power supply - IMPORTANT: Do not use 5V!)
- GND: GND     (Ground)

Notes:
- MISO, MOSI, and SCK are hardware SPI pins and cannot be changed
- SS_PIN can be any digital pin (8 chosen for optimal routing)
- RST_PIN can be any digital pin (9 chosen for optimal routing)
- The module must be powered with 3.3V to avoid damage

### NRF24L01
- CE_PIN: A1
- CSN_PIN: A0
- SCK_PIN: 13 (Hardware SPI)
- MOSI_PIN: 11 (Hardware SPI)
- MISO_PIN: 12 (Hardware SPI)

### Indicators
- Green LED: 6
- Red LED: 7
- Buzzer: 10

### Other
- Battery Voltage Reading: A2

## Features

- Low power operation with sleep mode
- RFID card authentication
- Visual feedback (LEDs)
- Audio feedback (Buzzer)
- Wireless data transmission
- Battery voltage monitoring

## Dependencies

- SPI.h
- MFRC522.h
- RF24.h
- LowPower.h

## Radio Transmission Details

Data packet structure:
```cpp
struct {
    uint32_t card_id;      // RFID card identifier
    bool authorized;       // Authentication status
    float batt;           // Battery voltage
    unsigned char sensor_id; // Device identifier
}
```

Channel: 108
Data Rate: 250KBPS
Power Amplifier Level: High

## Wiring Instructions

### RC522 to Arduino Pro Mini Connection
```
RC522 Module      Arduino Pro Mini
-------------     ----------------
3.3V    <------> VCC (3.3V)
RST     <------> D9
GND     <------> GND
MISO    <------> D12 (MISO)
MOSI    <------> D11 (MOSI)
SCK     <------> D13 (SCK)
SDA     <------> D8  (SS)
```

Visual Connection Diagram:
```
┌──────────────────┐
│                  │
│    RC522 RFID   │
│                  │
└─┬──┬──┬──┬──┬──┬┘
  │  │  │  │  │  │
  │  │  │  │  │  │    ┌──────────────────┐
  │  │  │  │  │  │    │                  │
  │  │  │  │  │  └────┤D8  Arduino   VCC │
  │  │  │  │  └───────┤D11 Pro      GND ├──┐
  │  │  │  └─────────┤D12 Mini         │  │
  │  │  └───────────┤D13              │  │
  │  └─────────────┤D9               │  │
  └───────────────┤GND              │  │
                  │                  │  │
                  └──────────┬───────┘  │
                            │           │
                            └───────────┘
```

Note: Make sure to use 3.3V for powering the RC522 module, as 5V can damage it. The Arduino Pro Mini 3.3V/8MHz version is ideal for this setup.


