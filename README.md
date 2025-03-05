# RC522 RFID Gateway System

A comprehensive IoT system that combines RFID authentication with RF24 wireless communication and MQTT integration for home automation systems like Home Assistant. The system features battery-optimized sensor nodes and a central gateway for seamless communication.

## Project Overview

### Hardware Requirements

**Gateway:**
- ESP8266 board
- NRF24L01+ radio module
- Status LEDs (red and green)

**RFID Sensor Node:**
- Arduino Pro Mini 3.3V/8MHz
- RC522 RFID reader
- NRF24L01+ radio module
- Status LEDs (red and green)
- Battery power supply (with voltage divider for monitoring)

### Key Features
- RFID card authentication
- Long-range wireless communication using NRF24L01+
- MQTT integration with Home Assistant
- Battery optimization for sensor nodes
- OTA updates for gateway
- Status monitoring and heartbeat functionality
- Battery level monitoring and reporting

## System Architecture

### RF24 Communication Pipeline

The system uses five distinct RF24 pipes for different types of sensors:
```
pipe01 (0xE7E8C0F0B1LL) - Temperature sensors
pipe02 (0xE7E8C0F0B2LL) - Doorbell
pipe03 (0xE7E8C0F0B3LL) - Door sensors
pipe04 (0xE7E8C0F0B4LL) - PIR sensors
pipe05 (0xE7E8C0F0B5LL) - RFID sensors
```

Each sensor type has its own data structure for efficient communication.

### MQTT Pipeline

#### Topic Structure
All sensors follow a common topic structure:
```
rf24/<sensor_type><sensor_id>
```

Example topics:
- `rf24/rfid1` - RFID reader #1
- `rf24/temp1` - Temperature sensor #1
- `rf24/door1` - Door sensor #1

#### Gateway Heartbeat
The gateway publishes its status to:
```
gateway/heartbeat
```
This alternates between "on" and "off" every 30 seconds to indicate the gateway is functioning.

### Message Formats

#### RFID Messages
```json
{
  "card_id": "123456789",
  "authorized": "true/false",
  "batt": "4.12",
  "batt_perc": "85"
}
```

#### Home Assistant Integration
The system integrates with Home Assistant using MQTT sensors:

```yaml
# RFID sensor configuration
sensor:
  - platform: mqtt
    name: "RFID Reader 1"
    state_topic: "rf24/rfid1"
    value_template: "{{ value_json.card_id }}"
    json_attributes_topic: "rf24/rfid1"
    json_attributes_template: >-
      {
        "authorized": {{ value_json.authorized }},
        "battery": {{ value_json.batt }},
        "battery_percentage": {{ value_json.batt_perc }}
      }
```

## Power Optimization Features

### Low Power Implementation

The RFID sensor nodes implement several power-saving features:

1. **Sleep Mode**
- Uses the LowPower library for power management
- The RF24 radio is powered down when not transmitting
- The RC522 RFID reader remains active to detect cards

2. **Battery Monitoring**
```cpp
// Voltage divider calculation (1330kΩ/330kΩ)
float vout = (analogRead(A2) * aref_fix) / 1023.0;
float battery_voltage = (1330.0/330.0) * vout;
```

3. **Battery Percentage Calculation**
```cpp
float battery_percentage = ((battery_voltage - 3.3) / (4.2 - 3.3)) * 100;
```

### RF24 Radio Configuration
The radio is configured for optimal power consumption while maintaining reliable communication:
```cpp
radio.setPALevel(RF24_PA_HIGH);
radio.enableDynamicPayloads();
radio.setDataRate(RF24_250KBPS);
radio.setRetries(8, 10);
```

## Setup & Configuration

### Gateway Setup

1. Copy `secrets.h.example` to `secrets.h` and configure your WiFi and MQTT credentials
2. Configure your MQTT broker address in the gateway code
3. Upload the gateway code to your ESP8266 board

### RFID Node Setup

1. Add authorized card IDs to the `authorized_cards` array:
```cpp
const uint32_t authorized_cards[] = {
  0x2492600354,  // Example card 1
  0x87654321   // Example card 2
};
```

2. Configure sensor ID if using multiple RFID readers:
```cpp
sensorData5.sensor_id = 1;  // Set unique ID for each reader
```

### Home Assistant Configuration

1. Add the provided MQTT configuration from `mqtt.yaml` to your Home Assistant configuration
2. Restart Home Assistant to apply changes
3. The system will automatically create:
   - Card ID sensor
   - Authorization status sensor
   - Battery level sensor
   - Gateway connection status sensor

## LED Status Indicators

### Gateway LEDs
- **Green LED**: 
  - Solid: Connected to WiFi and MQTT
  - Blinking: Normal operation
- **Red LED**:
  - Solid: Error state
  - Blinking: WiFi/MQTT connection attempt

### RFID Node LEDs
- **Green LED**:
  - Double blink: Successful card read and transmission
  - Single blink: RC522 initialization successful
- **Red LED**:
  - Solid: RC522 initialization failed
  - Single blink: Transmission failed

## Debug Mode

To enable debug output, set the DEBUG flag in the gateway code:
```cpp
#define DEBUG true
```

This will enable Serial output for troubleshooting.

## Version History

- V4.2: Added retain flag for MQTT messages, code cleanup
- V4.1: Added battery percentage calculation
- V4.0: Major update - JSON format implementation
- V3.1: Added OTA update capability
- V3.0: Added heartbeat function and solar sensor support
