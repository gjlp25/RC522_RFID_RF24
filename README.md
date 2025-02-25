# RC522 RFID and RF24 Gateway

This project is an implementation of an RF24 gateway using an ESP8266 and an RC522 RFID reader. The gateway communicates with various sensors using the nRF24L01+ module and sends data to an MQTT broker.

## Functionality

- **RF24 Gateway**: Receives data from nRF24L01+ based sensors and sends it to an MQTT broker.
- **RFID Reader**: Reads RFID tags and checks if they are authorized. Sends the data to the gateway.
- **MQTT Communication**: Publishes sensor data to an MQTT broker.
- **OTA Updates**: Supports Over-The-Air (OTA) updates for the ESP8266.

## Structure

The project consists of the following main files:

- `rf24gatewayV4.2.ino`: The main gateway code for the ESP8266.
- `main_rf24_rc522.ino`: The code for the Arduino Pro Mini with the RC522 RFID reader.
- `secrets.h`: Contains WiFi, MQTT, and OTA credentials.

## Libraries

The following libraries are used in this project:

- `SPI.h`: For SPI communication.
- `nRF24L01.h` and `RF24.h`: For nRF24L01+ communication.
- `ESP8266WiFi.h`: For WiFi connectivity.
- `PubSubClient.h`: For MQTT communication.
- `ESP8266mDNS.h`, `WiFiUdp.h`, and `ArduinoOTA.h`: For OTA updates.
- `MFRC522.h`: For RFID reader.
- `LowPower.h`: For power management on the Arduino Pro Mini.

## Secrets

The `secrets.h` file contains the following definitions:

```cpp
#ifndef SECRETS_H
#define SECRETS_H

// WiFi credentials
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

// MQTT credentials
#define MQTT_SERVER "your_mqtt_server_ip"
#define MQTT_USER "your_mqtt_username"
#define MQTT_PASSWORD "your_mqtt_password"

// OTA password
#define OTA_PASSWORD "your_ota_password"

#endif
```

## Diagram

```mermaid
graph TD;
    A[ESP8266] -->|WiFi| B[MQTT Broker]
    A -->|nRF24L01+| C[Temp Sensor]
    A -->|nRF24L01+| D[Door Sensor]
    A -->|nRF24L01+| E[PIR Sensor]
    A -->|nRF24L01+| F[RFID Reader]
    F -->|RF24| A
    G[Arduino Pro Mini] -->|SPI| F
```

## Setup

1. **ESP8266 Setup**:
   - Flash the `rf24gatewayV4.2.ino` code to the ESP8266.
   - Ensure the `secrets.h` file is correctly filled with your credentials.

2. **Arduino Pro Mini Setup**:
   - Flash the `main_rf24_rc522.ino` code to the Arduino Pro Mini.
   - Connect the RC522 RFID reader and nRF24L01+ module as per the pin definitions.

3. **MQTT Broker**:
   - Set up an MQTT broker and ensure the ESP8266 can connect to it using the credentials in `secrets.h`.

4. **OTA Updates**:
   - Use the OTA password defined in `secrets.h` for secure updates.

## Wiring

To wire up the RC522 RFID module to the Arduino Pro Micro, you need to connect the pins as follows:

- **VCC**: Connect to the 3.3V pin on the Arduino Pro Micro.
- **GND**: Connect to the GND pin on the Arduino Pro Micro.
- **SDA (SS)**: Connect to pin 10 on the Arduino Pro Micro.
- **SCK**: Connect to pin 15 (SCK) on the Arduino Pro Micro.
- **MOSI**: Connect to pin 16 (MOSI) on the Arduino Pro Micro.
- **MISO**: Connect to pin 14 (MISO) on the Arduino Pro Micro.
- **RST**: Connect to pin 9 on the Arduino Pro Micro.

Here is a summary of the connections:

| RC522 Pin | Arduino Pro Micro Pin |
|-----------|------------------------|
| VCC       | 3.3V                   |
| GND       | GND                    |
| SDA (SS)  | 10                     |
| SCK       | 15 (SCK)               |
| MOSI      | 16 (MOSI)              |
| MISO      | 14 (MISO)              |
| RST       | 9                      |

Make sure to double-check the pin numbers and connections to ensure proper communication between the RC522 module and the Arduino Pro Micro.

## Usage

- Power on the ESP8266 and Arduino Pro Mini.
- The ESP8266 will connect to WiFi and start listening for data from the nRF24L01+ sensors.
- The Arduino Pro Mini will read RFID tags and send the data to the ESP8266 via the nRF24L01+ module.
- The ESP8266 will publish the sensor data to the MQTT broker.


