/*
  Arduino Pro Mini 3.3V/8MHz
  Battery optimized / RC522 RFID Reader / NRF24L01
  
  Improved version with:
  - Reliable card detection using optimized polling
  - Proper power management
  - SPI bus sharing
  - RC522 error recovery
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

// Authorized card IDs (add your card IDs here)
const uint32_t authorized_cards[] = {
  0x2492600354,  // Example card 1
  0x87654321   // Example card 2
};
const int num_authorized_cards = sizeof(authorized_cards) / sizeof(authorized_cards[0]);

// Battery voltage calculation constants
const float VREF = 3.3;  // Reference voltage of Arduino Pro Mini 3.3V
const float R1 = 1330.0; // High-side resistor (1.33kΩ)
const float R2 = 330.0;  // Low-side resistor (330Ω)
const float VOLTAGE_DIVIDER = (R1 + R2) / R2;

// System state variables
bool rc522_initialized = false;
unsigned long lastRC522Check = 0;
const unsigned long RC522_CHECK_INTERVAL = 5000; // Check every 5 seconds

// Match gateway's sensorData5 structure
struct {
  uint32_t card_id;
  bool authorized;
  float batt;
  unsigned char sensor_id;
} sensorData5;

// Function prototypes
bool checkRC522Connection();
void reinitRC522();
bool initRF24();
float readBatteryVoltage();
bool isCardAuthorized(uint32_t cardId);
void handleCardDetection();
void configureSPI_RC522();
void configureSPI_RF24();

void setup() {
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  
  Serial.begin(115200);
  delay(100);
  Serial.println(F("\n\nRFID Reader Starting..."));
  
  // Initialize SPI bus
  SPI.begin();
  
  // Initialize RC522
  reinitRC522();
  
  // Initialize RF24
  if (!initRF24()) {
    Serial.println(F("RF24 init failed!"));
    digitalWrite(redLedPin, HIGH);
  }
  
  sensorData5.sensor_id = 1;  // Set RFID reader ID
  
  Serial.println(F("System Ready"));
  digitalWrite(greenLedPin, HIGH);
  delay(100);
  digitalWrite(greenLedPin, LOW);
}

void loop() {
  // Periodic RC522 connection check
  if (millis() - lastRC522Check >= RC522_CHECK_INTERVAL) {
    configureSPI_RC522();
    if (!checkRC522Connection()) {
      Serial.println(F("RC522 connection lost! Reinitializing..."));
      reinitRC522();
    }
    lastRC522Check = millis();
  }

  // Check for cards
  configureSPI_RC522();
  
  // Two-step card detection for better reliability
  if (rfid.PICC_IsNewCardPresent()) {
    delay(10); // Short delay for card stabilization
    if (rfid.PICC_ReadCardSerial()) {
      handleCardDetection();
    }
  }

  // Sleep for power saving (30ms gives good balance between power saving and responsiveness)
  LowPower.powerDown(SLEEP_30MS, ADC_OFF, BOD_OFF);
}

bool checkRC522Connection() {
  configureSPI_RC522();
  byte v = rfid.PCD_ReadRegister(rfid.VersionReg);
  return (v == 0x91 || v == 0x92);
}

void reinitRC522() {
  configureSPI_RC522();
  rfid.PCD_Init();
  delay(50);
  
  if (checkRC522Connection()) {
    rc522_initialized = true;
    rfid.PCD_SetAntennaGain(rfid.RxGain_max);
    digitalWrite(redLedPin, LOW);
    Serial.println(F("RC522 initialized successfully"));
  } else {
    rc522_initialized = false;
    digitalWrite(redLedPin, HIGH);
    Serial.println(F("RC522 initialization failed"));
  }
}

bool initRF24() {
  configureSPI_RF24();
  if (!radio.begin()) return false;
  
  // Match gateway settings exactly
  radio.setChannel(108);
  radio.openWritingPipe(pipe05);
  radio.setPALevel(RF24_PA_HIGH);
  radio.enableDynamicPayloads();
  radio.setDataRate(RF24_250KBPS);
  radio.setRetries(15, 15);  // 15 retries, 15*250us delay
  radio.setAutoAck(true);    // Enable auto-acknowledgements
  radio.setCRCLength(RF24_CRC_16); // Use 16-bit CRC
  radio.stopListening();
  
  // Verify channel is clear
  radio.startListening();
  delayMicroseconds(250);
  radio.stopListening();
  
  radio.powerDown();
  return true;
}

float readBatteryVoltage() {
  digitalWrite(greenLedPin, HIGH);  // Power indicator
  delay(5);  // Let voltage stabilize
  
  int reading = analogRead(A2);
  float vout = (reading * VREF) / 1024.0;
  float voltage = vout * VOLTAGE_DIVIDER;
  
  digitalWrite(greenLedPin, LOW);
  
  // Constrain to reasonable values
  if (voltage < 0) voltage = 0;
  if (voltage > 5.0) voltage = 5.0;
  
  return voltage;
}

bool isCardAuthorized(uint32_t cardId) {
  for(int i = 0; i < num_authorized_cards; i++) {
    if(authorized_cards[i] == cardId) {
      return true;
    }
  }
  return false;
}

void handleCardDetection() {
  digitalWrite(greenLedPin, HIGH);
  
  // Power up radio with extended stabilization
  configureSPI_RF24();
  radio.powerUp();
  delay(10);  // Extended stabilization time
  
  configureSPI_RC522();
  // Convert UID to single uint32_t
  uint32_t cardId = 0;
  for (byte i = 0; i < rfid.uid.size; i++) {
    cardId = (cardId << 8) | rfid.uid.uidByte[i];
  }
  
  Serial.print(F("Card detected! ID: "));
  Serial.println(cardId);
  
  sensorData5.card_id = cardId;
  sensorData5.authorized = isCardAuthorized(cardId);
  sensorData5.batt = readBatteryVoltage();
  
  // Switch to RF24 SPI configuration and send data
  configureSPI_RF24();
  
  // Try transmission with improved error handling
  bool report = false;
  for(int i = 0; i < 5 && !report; i++) {
    // Clear any stale data
    radio.flush_tx();
    
    // Verify channel is still clear
    radio.startListening();
    delayMicroseconds(250);
    radio.stopListening();
    
    report = radio.write(&sensorData5, sizeof(sensorData5));
    
    if(!report) {
      delay(100);  // Longer delay between retries
      
      // Re-initialize radio if multiple failures
      if(i == 2) {
        radio.powerDown();
        delay(10);
        initRF24();
        radio.powerUp();
        delay(10);
      }
    }
  }
  
  if (report) {
    Serial.println(F("Transmission successful"));
    // Double blink green LED
    digitalWrite(greenLedPin, LOW);
    delay(50);
    digitalWrite(greenLedPin, HIGH);
    delay(50);
    digitalWrite(greenLedPin, LOW);
  } else {
    Serial.println(F("Transmission failed"));
    digitalWrite(redLedPin, HIGH);
    delay(100);
    digitalWrite(redLedPin, LOW);
  }
  
  // Clean up with improved timing
  configureSPI_RC522();
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  
  configureSPI_RF24();
  radio.flush_tx();
  delay(10);  // Wait before power down
  radio.powerDown();

  // Extended delay to ensure complete cycle
  delay(500);
}

void configureSPI_RC522() {
  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
  digitalWrite(CSN_PIN, HIGH);  // Deselect RF24
  digitalWrite(SS_PIN, LOW);    // Select RC522
}

void configureSPI_RF24() {
  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
  digitalWrite(SS_PIN, HIGH);   // Deselect RC522
  digitalWrite(CSN_PIN, LOW);   // Select RF24
}
