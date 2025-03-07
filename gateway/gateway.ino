/*
  RF24 Gateway by Daneel Verhaall
  MQTT Client for NRF24L01+ Based Sensors
*/

#include <SPI.h>
#include <RF24.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// Network configuration
#define wifi_ssid "WiCaRo_IoT"
#define wifi_password "Test@2021"
#define mqtt_server "10.0.30.29"
#define mqtt_user "mqtt_gjlp25"
#define mqtt_password "o67@4ZeNSq&3k8"

// Pin definitions
#define CE_PIN D2
#define CSN_PIN D8
const int ledRed = D0;
const int ledGreen = D1;

// RF24 initialization
RF24 radio(CE_PIN, CSN_PIN);
const uint64_t pipe01 = 0xE7E8C0F0B1LL;
const uint64_t pipe02 = 0xE7E8C0F0B2LL;
const uint64_t pipe03 = 0xE7E8C0F0B3LL;
const uint64_t pipe04 = 0xE7E8C0F0B4LL;
const uint64_t pipe05 = 0xE7E8C0F0B5LL;

// MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

// Topic strings
const char *myTopics[] = {"temp", "doorbell", "door", "pir", "rfid"};
const char *offOnState[] = {"off", "on"};
const char *closedOpenState[] = {"closed", "open"};

// Struct definitions with proper packing
#pragma pack(push, 1)

struct TempData {
    float temp;         // 4 bytes
    float hum;          // 4 bytes
    float pres;         // 4 bytes
    float batt;         // 4 bytes
    uint8_t sensor_id;  // 1 byte
} __attribute__((packed));

struct DoorBellData {
    float batt;         // 4 bytes
    uint8_t doorbell;   // 1 byte
} __attribute__((packed));

struct DoorData {
    float batt;         // 4 bytes
    uint8_t sensor_id;  // 1 byte
    bool door;          // 1 byte
} __attribute__((packed));

struct PIRData {
    float batt;         // 4 bytes
    uint8_t sensor_id;  // 1 byte
    bool motion;        // 1 byte
} __attribute__((packed));

struct RFIDData {
    uint32_t card_id;   // 4 bytes
    bool authorized;    // 1 byte
    float batt;         // 4 bytes
    uint8_t sensor_id;  // 1 byte
    char user_name[16]; // 16 bytes
} __attribute__((packed));

#pragma pack(pop)

// Global sensor data instances
static struct TempData sensorData1;
static struct DoorBellData sensorData2;
static struct DoorData sensorData3;
static struct PIRData sensorData4;
static struct RFIDData sensorData5;

// Global variables
float sensorBatt;
int topic;
bool heartbeat = true;
bool doorbell_flag = false;
int ledState = LOW;

unsigned long currentMillis;
long previousMillis1 = 0;
long previousMillis3 = 0;
const long interval1 = 2000;
const long interval2 = 5000;
const long interval3 = 30000;

// Function prototypes
void setup_wifi();
void reconnect();
float battery_perc();
void MQTT_sensorData1();
void MQTT_sensorData2();
void MQTT_sensorData3();
void MQTT_sensorData4();
void MQTT_sensorData5();
void handle_rfid(uint8_t bytes);

// WiFi setup function
void setup_wifi() {
    Serial.println(F("\nSetting up WiFi..."));
    digitalWrite(ledRed, HIGH);  // Red LED on during WiFi connection

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(F("."));
        // Blink red LED while connecting
        digitalWrite(ledRed, !digitalRead(ledRed));
    }

    digitalWrite(ledRed, LOW);  // Turn off red LED once connected
    Serial.println(F("\nWiFi connected"));
    Serial.print(F("IP address: "));
    Serial.println(WiFi.localIP());
}

// MQTT reconnect function
void reconnect() {
    Serial.println(F("Attempting MQTT connection..."));
    digitalWrite(ledRed, HIGH);  // Red LED on during reconnection
    
    // Loop until we're reconnected
    while (!client.connected()) {
        // Create a random client ID
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        
        // Attempt to connect
        if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
            Serial.println(F("Connected to MQTT broker"));
            // Once connected, publish an announcement
            client.publish("gateway/status", "online", true);
        } else {
            Serial.print(F("Failed to connect, rc="));
            Serial.print(client.state());
            Serial.println(F(" retrying in 5 seconds"));
            // Blink red LED while waiting
            digitalWrite(ledRed, !digitalRead(ledRed));
            delay(5000);
        }
    }
    
    digitalWrite(ledRed, LOW);  // Turn off red LED once connected
}

// Battery percentage calculation
float battery_perc() {
    float output = ((sensorBatt - 3.3) / (4.2 - 3.3)) * 100;
    return output < 100 ? output : 100.0f;
}

// MQTT publish function for RFID data
void MQTT_sensorData5() {
    char mqtt_message[256];
    char card_id_str[16];
    snprintf(card_id_str, sizeof(card_id_str), "0x%08X", sensorData5.card_id);
    
    // Calculate battery percentage
    sensorBatt = sensorData5.batt;
    float batt_percentage = battery_perc();
    
    // Create JSON message with all required fields
    snprintf(mqtt_message, sizeof(mqtt_message),
        "{\"card_id\":\"%s\","
        "\"user_name\":\"%s\","
        "\"authorized\":%s,"
        "\"batt\":%.2f,"
        "\"batt_perc\":%.1f,"
        "\"retries\":0,"
        "\"signal\":100,"
        "\"connected\":true}",
        card_id_str,
        sensorData5.user_name,
        sensorData5.authorized ? "true" : "false",
        sensorData5.batt,
        batt_percentage
    );
    
    // Publish to MQTT
    client.publish("rf24/rfid1", mqtt_message, true);
    
    // Debug output
    Serial.println(F("Published RFID data to MQTT:"));
    Serial.println(mqtt_message);
}

// Placeholder functions for other sensor types
void MQTT_sensorData1() {
    // Temperature sensor data
    char mqtt_message[128];
    sensorBatt = sensorData1.batt;
    
    snprintf(mqtt_message, sizeof(mqtt_message),
        "{\"temp\":%.2f,\"hum\":%.2f,\"pres\":%.2f,\"batt\":%.2f,\"batt_perc\":%.1f}",
        sensorData1.temp, sensorData1.hum, sensorData1.pres, sensorData1.batt, battery_perc());
    
    char topic[32];
    snprintf(topic, sizeof(topic), "rf24/temp%d", sensorData1.sensor_id);
    client.publish(topic, mqtt_message, true);
}

void MQTT_sensorData2() {
    // Doorbell sensor data
    sensorBatt = sensorData2.batt;
    if (sensorData2.doorbell == 1) {
        client.publish("rf24/doorbell_flag", "on", true);
        doorbell_flag = true;
        previousMillis1 = millis();
    }
}

void MQTT_sensorData3() {
    // Door sensor data
    char mqtt_message[128];
    sensorBatt = sensorData3.batt;
    
    snprintf(mqtt_message, sizeof(mqtt_message),
        "{\"door\":\"%s\",\"batt\":%.2f,\"batt_perc\":%.1f}",
        sensorData3.door ? "open" : "closed", sensorData3.batt, battery_perc());
    
    char topic[32];
    snprintf(topic, sizeof(topic), "rf24/door%d", sensorData3.sensor_id);
    client.publish(topic, mqtt_message, true);
}

void MQTT_sensorData4() {
    // PIR motion sensor data
    char mqtt_message[128];
    sensorBatt = sensorData4.batt;
    
    snprintf(mqtt_message, sizeof(mqtt_message),
        "{\"motion\":\"%s\",\"batt\":%.2f,\"batt_perc\":%.1f}",
        sensorData4.motion ? "detected" : "clear", sensorData4.batt, battery_perc());
    
    char topic[32];
    snprintf(topic, sizeof(topic), "rf24/pir%d", sensorData4.sensor_id);
    client.publish(topic, mqtt_message, true);
}

// Improved RFID handling function
void handle_rfid(uint8_t bytes) {
    Serial.println(F("\n=== RF24 Debug Info ==="));
    
    // Print memory layout info first
    Serial.println(F("Memory Layout:"));
    Serial.print(F("- Expected struct size: ")); Serial.println(sizeof(sensorData5));
    Serial.print(F("- Received data size:  ")); Serial.println(bytes);
    Serial.println(F("Struct field sizes:"));
    Serial.print(F("- card_id:    ")); Serial.println(sizeof(sensorData5.card_id));
    Serial.print(F("- authorized: ")); Serial.println(sizeof(sensorData5.authorized));
    Serial.print(F("- batt:      ")); Serial.println(sizeof(sensorData5.batt));
    Serial.print(F("- sensor_id:  ")); Serial.println(sizeof(sensorData5.sensor_id));
    Serial.print(F("- user_name:  ")); Serial.println(sizeof(sensorData5.user_name));
    
    // Verify size matches
    if (bytes != sizeof(sensorData5)) {
        Serial.println(F("Error: Size mismatch between sender and receiver!"));
        radio.flush_rx();
        return;
    }

    // Read the data with SPI protection
    SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
    digitalWrite(CSN_PIN, LOW);
    delayMicroseconds(5);
    radio.read(&sensorData5, sizeof(sensorData5));
    digitalWrite(CSN_PIN, HIGH);
    delayMicroseconds(5);
    SPI.endTransaction();

    // Print received data for debugging
    Serial.println(F("\nReceived Data:"));
    Serial.print(F("- Card ID: 0x")); Serial.println(sensorData5.card_id, HEX);
    Serial.print(F("- User: '")); Serial.print(sensorData5.user_name); Serial.println("'");
    Serial.print(F("- Auth: ")); Serial.println(sensorData5.authorized ? "Yes" : "No");
    Serial.print(F("- Batt: ")); Serial.println(sensorData5.batt);
    Serial.print(F("- Sensor: ")); Serial.println(sensorData5.sensor_id);

    // If we got here, everything worked - send to MQTT
    MQTT_sensorData5();
    Serial.println(F("RFID processing completed successfully"));
}

void setup() {
    // Initialize hardware
    pinMode(ledGreen, OUTPUT);
    pinMode(ledRed, OUTPUT);
    pinMode(CSN_PIN, OUTPUT);
    digitalWrite(CSN_PIN, HIGH);
    digitalWrite(ledRed, HIGH);    // Red LED on during startup
    digitalWrite(ledGreen, LOW);   // Green LED off initially

    // Start serial and wait for it to be ready
    Serial.begin(115200);
    delay(100);  // Give serial port time to initialize
    Serial.println(F("\n\nRF24 Gateway Starting..."));

    setup_wifi();
    
    Serial.println(F("Initializing RF24 radio..."));
    if (!radio.begin()) {
        Serial.println(F("RF24 init failed!"));
        // Fast blink red LED to indicate RF24 init failure
        while(1) {
            digitalWrite(ledRed, !digitalRead(ledRed));
            delay(200);
        }
    }
    Serial.println(F("RF24 initialized successfully"));
    
    // Configure radio
    Serial.println(F("Configuring radio settings..."));
    radio.setChannel(108);
    radio.openReadingPipe(1, pipe01);
    radio.openReadingPipe(2, pipe02);
    radio.openReadingPipe(3, pipe03);
    radio.openReadingPipe(4, pipe04);
    radio.openReadingPipe(5, pipe05);
    radio.setPALevel(RF24_PA_HIGH);
    radio.enableDynamicPayloads();
    radio.setDataRate(RF24_250KBPS);
    radio.setAutoAck(true);
    radio.setCRCLength(RF24_CRC_16);
    radio.startListening();
    
    // Print radio configuration
    Serial.println(F("Radio configuration:"));
    Serial.print(F("Channel: ")); Serial.println(108);
    Serial.print(F("PA Level: ")); Serial.println("HIGH");
    Serial.print(F("Data Rate: ")); Serial.println("250KBPS");
    
    // Setup MQTT and OTA
    Serial.println(F("Setting up MQTT and OTA..."));
    client.setServer(mqtt_server, 1883);
    ArduinoOTA.begin();

    // All initialization complete
    digitalWrite(ledRed, LOW);     // Turn off red LED
    digitalWrite(ledGreen, HIGH);  // Turn on green LED
    Serial.println(F("Gateway initialization complete!"));
}

void loop() {
    ArduinoOTA.handle();
    
    // Handle MQTT connection
    if (!client.connected()) {
        digitalWrite(ledGreen, LOW);  // Green LED off while disconnected
        digitalWrite(ledRed, HIGH);   // Red LED on while disconnected
        reconnect();
        digitalWrite(ledRed, LOW);    // Red LED off when reconnected
        digitalWrite(ledGreen, HIGH); // Green LED on when reconnected
    }
    client.loop();

    // Check for radio data
    byte pipeNum;
    if (radio.available(&pipeNum)) {
        digitalWrite(ledGreen, LOW);  // Turn off green LED during processing
        topic = pipeNum;
        
        if (topic == 5) {  // RFID sensor
            uint8_t bytes = radio.getDynamicPayloadSize();
            handle_rfid(bytes);
        }
        else {  // Other sensors
            switch(topic) {
                case 1:
                    radio.read(&sensorData1, sizeof(sensorData1));
                    MQTT_sensorData1();
                    break;
                case 2:
                    radio.read(&sensorData2, sizeof(sensorData2));
                    MQTT_sensorData2();
                    break;
                case 3:
                    radio.read(&sensorData3, sizeof(sensorData3));
                    MQTT_sensorData3();
                    break;
                case 4:
                    radio.read(&sensorData4, sizeof(sensorData4));
                    MQTT_sensorData4();
                    break;
                default:
                    radio.flush_rx();
                    break;
            }
        }
        digitalWrite(ledGreen, HIGH); // Turn green LED back on after processing
    }

    // Handle doorbell flag timeout
    currentMillis = millis();
    if (doorbell_flag && (currentMillis - previousMillis1 > interval1)) {
        doorbell_flag = false;
        client.publish("rf24/doorbell_flag", "off", true);
        previousMillis1 = 0;
    }

    // Handle heartbeat
    if (currentMillis - previousMillis3 > interval3) {
        client.publish("gateway/heartbeat", heartbeat ? "on" : "off");
        heartbeat = !heartbeat;
        previousMillis3 = currentMillis;
        // Blink green LED briefly to show heartbeat
        digitalWrite(ledGreen, LOW);
        delay(50);
        digitalWrite(ledGreen, HIGH);
    }
}
