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
// Define pin for buzzer
#define BUZZER_PIN 3

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

// Card to user mapping
struct CardUser {
  uint32_t card_id;
  const char* user_name;
  bool authorized;
};

const CardUser card_users[] = {
  {0x94921022, "Tim", true},    // Tim's card
  {0x87654321, "Guest", false}    // Example unauthorized card
};
const int num_cards = sizeof(card_users) / sizeof(card_users[0]);

// Battery voltage calculation constants
const float VREF = 3.3;  // Reference voltage of Arduino Pro Mini 3.3V
const float R1 = 1330.0; // High-side resistor (1.33kΩ)
const float R2 = 330.0;  // Low-side resistor (330Ω)
const float VOLTAGE_DIVIDER = (R1 + R2) / R2;

// System state variables
bool rc522_initialized = false;
unsigned long lastRC522Check = 0;
const unsigned long RC522_CHECK_INTERVAL = 5000; // Check every 5 seconds

// Match gateway's sensorData5 structure exactly
#pragma pack(push, 1)  // Force 1-byte alignment
struct RFIDData {
  uint32_t card_id;    // 4 bytes
  bool authorized;     // 1 byte
  float batt;         // 4 bytes
  uint8_t sensor_id;  // 1 byte
  char user_name[16]; // 16 bytes
} __attribute__((packed));
#pragma pack(pop)  // Restore default packing

static struct RFIDData sensorData5;

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
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(SS_PIN, OUTPUT);
  pinMode(CSN_PIN, OUTPUT);
  
  // Initial state for SPI pins
  digitalWrite(SS_PIN, HIGH);
  digitalWrite(CSN_PIN, HIGH);
  
  Serial.begin(115200);
  while (!Serial) { ; }  // Wait for serial port
  Serial.println(F("\n\nRFID Reader Starting..."));
  Serial.println(F("Debug mode enabled"));
  
  // Initialize SPI bus with proper settings
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV8); // Slower speed for reliability
  
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

  // Reset RC522 for reliable card detection
  configureSPI_RC522();
  rfid.PCD_WriteRegister(rfid.ComIrqReg, 0x7F);    // Clear all interrupt bits
  rfid.PCD_WriteRegister(rfid.FIFOLevelReg, 0x80); // Flush FIFO buffer
  
  // Basic card detection
  if (rfid.PICC_IsNewCardPresent()) {
    if (rfid.PICC_ReadCardSerial()) {
      handleCardDetection();
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }
  }

  // Sleep for power saving
  LowPower.powerDown(SLEEP_30MS, ADC_OFF, BOD_OFF);
}

bool checkRC522Connection() {
  configureSPI_RC522();
  byte v = rfid.PCD_ReadRegister(rfid.VersionReg);
  return (v == 0x91 || v == 0x92);
}

void reinitRC522() {
  configureSPI_RC522();
  rfid.PCD_Reset();
  delay(50);
  rfid.PCD_Init();
  delay(50);
  
  if (checkRC522Connection()) {
    rc522_initialized = true;
    rfid.PCD_SetAntennaGain(rfid.RxGain_max);
    rfid.PCD_WriteRegister(rfid.TxModeReg, 0x00);
    rfid.PCD_WriteRegister(rfid.RxModeReg, 0x00);
    rfid.PCD_WriteRegister(rfid.ModWidthReg, 0x26);
    digitalWrite(redLedPin, LOW);
    Serial.println(F("RC522 initialized successfully"));
  } else {
    rc522_initialized = false;
    digitalWrite(redLedPin, HIGH);
    Serial.println(F("RC522 initialization failed"));
  }
  
  rfid.PCD_WriteRegister(rfid.ComIrqReg, 0x7F);
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
  radio.setAutoAck(true);
  radio.setCRCLength(RF24_CRC_16);
  radio.stopListening();
  
  // Verify RF24 settings
  radio.startListening();
  delayMicroseconds(250);
  radio.stopListening();
  
  radio.powerDown();
  return true;
}

float readBatteryVoltage() {
  delay(5);  // Let voltage stabilize
  int reading = analogRead(A2);
  float vout = (reading * VREF) / 1024.0;
  float voltage = vout * VOLTAGE_DIVIDER;
  
  // Constrain to reasonable values
  if (voltage < 0) voltage = 0;
  if (voltage > 5.0) voltage = 5.0;
  
  return voltage;
}

const CardUser* findCard(uint32_t cardId) {
  for(int i = 0; i < num_cards; i++) {
    if(card_users[i].card_id == cardId) {
      return &card_users[i];
    }
  }
  return nullptr;
}

void handleCardDetection() {
  Serial.println(F("\n--- Card Detection Started ---"));
  
  digitalWrite(greenLedPin, HIGH);
  tone(BUZZER_PIN, 2000);
  
  unsigned long startTime = millis();
  
  // Power up radio with stabilization
  Serial.println(F("Powering up RF24..."));
  configureSPI_RF24();
  radio.powerUp();
  delay(20);
  
  configureSPI_RC522();
  uint32_t cardId = 0;
  for (byte i = 0; i < rfid.uid.size; i++) {
    cardId = (cardId << 8) | rfid.uid.uidByte[i];
  }
  
  Serial.print(F("Card detected! ID: "));
  Serial.println(cardId);
  
  const CardUser* user = findCard(cardId);
  memset(&sensorData5, 0, sizeof(sensorData5));
  
  sensorData5.card_id = cardId;
  sensorData5.sensor_id = 1;
  sensorData5.batt = readBatteryVoltage();
  
  if (user) {
    strncpy(sensorData5.user_name, user->user_name, sizeof(sensorData5.user_name) - 1);
    sensorData5.user_name[sizeof(sensorData5.user_name) - 1] = '\0';
    sensorData5.authorized = user->authorized;
  } else {
    strncpy(sensorData5.user_name, "Unknown", sizeof(sensorData5.user_name));
    sensorData5.authorized = false;
  }
  
  Serial.print(F("Sending data: ID="));
  Serial.print(sensorData5.card_id);
  Serial.print(F(" User="));
  Serial.print(sensorData5.user_name);
  Serial.print(F(" Auth="));
  Serial.println(sensorData5.authorized ? "yes" : "no");
  
  // Switch to RF24 and send data
  configureSPI_RF24();
  radio.stopListening();
  
  bool report = false;
  radio.flush_tx();
  
  for(int i = 0; i < 5 && !report; i++) {
    Serial.print(F("Transmission attempt "));
    Serial.print(i + 1);
    Serial.print(F(" of 5... "));
    
    radio.startListening();
    delayMicroseconds(250);
    radio.stopListening();
    
    report = radio.write(&sensorData5, sizeof(sensorData5));
    
    if(!report) {
      Serial.println(F("Failed"));
      delay(100);
      
      if(i == 2) {
        Serial.println(F("Reinitializing radio..."));
        radio.powerDown();
        delay(10);
        initRF24();
        radio.powerUp();
        delay(10);
      }
    } else {
      Serial.println(F("Success!"));
    }
  }
  
  if (report) {
    Serial.println(F("Transmission successful"));
  } else {
    Serial.println(F("All transmission attempts failed"));
    digitalWrite(redLedPin, HIGH);
    delay(100);
    digitalWrite(redLedPin, LOW);
  }
  
  unsigned long elapsedTime = millis() - startTime;
  if (elapsedTime < 1000) {
    delay(1000 - elapsedTime);
  }
  
  digitalWrite(greenLedPin, LOW);
  noTone(BUZZER_PIN);
  
  configureSPI_RC522();
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  
  configureSPI_RF24();
  radio.flush_tx();
  delay(10);
  radio.powerDown();

  delay(500);
  
  Serial.println(F("--- Card Detection Completed ---\n"));
}

void configureSPI_RC522() {
  SPI.endTransaction();
  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
  digitalWrite(CSN_PIN, HIGH);  // Deselect RF24
  delayMicroseconds(5);
  digitalWrite(SS_PIN, LOW);    // Select RC522
  delayMicroseconds(5);
}

void configureSPI_RF24() {
  SPI.endTransaction();
  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
  digitalWrite(SS_PIN, HIGH);   // Deselect RC522
  delayMicroseconds(5);
  digitalWrite(CSN_PIN, LOW);   // Select RF24
  delayMicroseconds(5);
}
