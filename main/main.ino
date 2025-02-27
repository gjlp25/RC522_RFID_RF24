/*
  Arduino Pro Mini 3.3V/8MHz
  Battery optimized / RC522 RFID Reader / NRF24L01
*/

#include "LowPower.h"
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <MFRC522.h>

// Define pins for RC522
#define RST_PIN 5
#define SS_PIN 4

// Define pins for RF24
#define CE_PIN 15    // Hardwired CE pin
#define CSN_PIN 14   // Hardwired CSN pin

// LED pins for debugging
const int greenLedPin = 6;
const int redLedPin = 7;

// Initialize RF24 and RFID
RF24 radio(CE_PIN, CSN_PIN);
MFRC522 rfid(SS_PIN, RST_PIN);

// Use pipe05 from gateway for RFID communications
const uint64_t pipe05 = 0xE7E8C0F0B5LL;

// Battery voltage calculation
float aref_fix = 1.065;

// Match gateway's sensorData5 structure
struct {
  uint32_t card_id;
  bool authorized;
  float batt;
  unsigned char sensor_id;
} sensorData5;

void setup() {
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("\n\nRFID Reader"));
  
  // Initialize SPI and RFID
  SPI.begin();
  rfid.PCD_Init();
  
  // Test RC522
  byte v = rfid.PCD_ReadRegister(rfid.VersionReg);
  Serial.print(F("RC522 Version: 0x"));
  Serial.println(v, HEX);
  
  if((v == 0x91) || (v == 0x92)) {
    digitalWrite(greenLedPin, HIGH);
    Serial.println(F("RC522 OK"));
  } else {
    digitalWrite(redLedPin, HIGH);
    Serial.println(F("RC522 FAIL"));
  }
  delay(1000);
  digitalWrite(greenLedPin, LOW);
  digitalWrite(redLedPin, LOW);
  
  // Initialize RF24
  Serial.println(F("Init RF24..."));
  radio.begin();
  radio.setChannel(108);
  radio.openWritingPipe(pipe05);
  radio.setPALevel(RF24_PA_HIGH);
  radio.enableDynamicPayloads();
  radio.setDataRate(RF24_250KBPS);
  radio.setRetries(8, 10);
  radio.stopListening();
  
  sensorData5.sensor_id = 1;  // Set RFID reader ID
  
  Serial.println(F("Ready"));
}

void loop() {
  // Check for cards continuously
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    digitalWrite(greenLedPin, HIGH);  // Visual feedback
    
    // Convert UID to single uint32_t
    uint32_t cardId = 0;
    for (byte i = 0; i < rfid.uid.size; i++) {
      cardId = (cardId << 8) | rfid.uid.uidByte[i];
    }
    
    Serial.print(F("Card detected! ID: "));
    Serial.println(cardId);
    
    sensorData5.card_id = cardId;
    sensorData5.authorized = false;  // Gateway will handle authorization
    
    // Read battery voltage
    int reading = analogRead(A2);
    float vout = (reading * aref_fix) / 1023.0;
    sensorData5.batt = (1330.0/330.0) * vout;
    
    Serial.print(F("Battery voltage: "));
    Serial.println(sensorData5.batt);
    
    radio.powerUp();
    delay(100);
    Serial.println(F("Sending data to gateway..."));
    
    bool report = radio.write(&sensorData5, sizeof(sensorData5));
    
    if (report) {
      Serial.println(F("Transmission successful"));
      digitalWrite(greenLedPin, HIGH);
      delay(100);
      digitalWrite(greenLedPin, LOW);
      delay(100);
      digitalWrite(greenLedPin, HIGH);
      delay(100);
      digitalWrite(greenLedPin, LOW);
    } else {
      Serial.println(F("Transmission failed"));
      digitalWrite(redLedPin, HIGH);
      delay(500);
      digitalWrite(redLedPin, LOW);
    }
    
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
  
  delay(100);  // Small delay to prevent too rapid scanning
}
