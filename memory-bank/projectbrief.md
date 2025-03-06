# RC522 RFID Reader Project Brief

## Overview
This project implements an RFID reader system using an Arduino Pro Mini 3.3V/8MHz, RC522 RFID module, and NRF24L01 radio module. The system is battery-optimized and designed for reliable card detection.

## Core Requirements
1. Reliable RFID card detection using RC522 module
2. Power-efficient operation for battery usage
3. Wireless communication via NRF24L01
4. Visual feedback through LEDs
5. Audio feedback through buzzer
6. Support for authorized card management

## Hardware Components
- Arduino Pro Mini 3.3V/8MHz
- RC522 RFID Reader
- NRF24L01 Radio Module
- LEDs (Green and Red)
- Piezo Buzzer
- Battery power supply

## Pin Assignments
- RC522:
  - RST: Pin 5
  - SS: Pin 4
  - IRQ: Pin 2
- NRF24L01:
  - CE: Pin 15 (hardwired)
  - CSN: Pin 14 (hardwired)
- LEDs:
  - Green: Pin 6
  - Red: Pin 7
- Buzzer: Pin 3

## Key Features
1. Interrupt-based card detection
2. Battery voltage monitoring
3. Power-saving sleep modes
4. SPI bus sharing between RC522 and NRF24L01
5. Error recovery mechanisms
6. User feedback:
   - Authorized cards: Green LED + 2kHz tone for 1 second
   - Unauthorized cards: Red LED blinks + 400Hz beeps 3 times
   - Transmission errors: Error tone at 100Hz

## Success Criteria
1. Reliable card detection and reading
2. Clear user feedback for different card states
3. Successful wireless transmission of card data
4. Extended battery life through power optimization
5. Robust error handling and recovery
