/*
  Arduino Pro Mini 3.3V/8MHz
  Battery optimized / RC522 RFID Reader / NRF24L01
*/

#include <SPI.h>
#include <MFRC522.h>
#include <RF24.h>
#include <LowPower.h>

// Define pins for RC522
#define RST_PIN 9
#define SS_PIN 8

// Define pins for RF24
#define CE_PIN A1  
#define CSN_PIN A0 
#define SCK_PIN 13
#define MOSI_PIN 11
#define MISO_PIN 12

// Initialize RFID and RF24
MFRC522 rfid(SS_PIN, RST_PIN);
RF24 radio(CE_PIN, CSN_PIN);

// Define RF24 pipe
const uint64_t pipe = 0xE7E8C0F0B1LL;

// Define LED pins
const int greenLedPin = 12;
const int redLedPin = 13;
const int buzzerPin = 10;

// Dictionary for authorized cards with names
struct AuthorizedCard {
  uint32_t id;
  const char* name;
};

AuthorizedCard authorizedCards[] = {
  {1036396588, "Wies"},
  {571511444, "Tim"}
};

// Add structure for RF24 transmission
struct {
  uint32_t card_id;
  bool authorized;
  float batt;
  unsigned char sensor_id;
} sensorData;

// Function to check if card is authorized
const char* checkAuthorized(uint32_t cardId) {
  for (auto& card : authorizedCards) {
    if (card.id == cardId) {
      return card.name;
    }
  }
  return nullptr;
}

// Function to reset LEDs
void resetLeds() {
  digitalWrite(greenLedPin, LOW);
  digitalWrite(redLedPin, LOW);
}

// Function to beep buzzer
void beepBuzzer(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(buzzerPin, HIGH);
    delay(delayMs);
    digitalWrite(buzzerPin, LOW);
    delay(delayMs);
  }
}

// Function to handle card
void handleCard(uint32_t cardId, const char* name) {
  bool isAuthorized = name != nullptr;
  
  if (isAuthorized) {
    Serial.print("Card ID: ");
    Serial.print(cardId);
    Serial.print(" (Name: ");
    Serial.print(name);
    Serial.println(") PASS: Green Light Activated");
    digitalWrite(greenLedPin, HIGH);
    beepBuzzer(1, 500);
    digitalWrite(greenLedPin, LOW);
  } else {
    Serial.print("Card ID: ");
    Serial.print(cardId);
    Serial.println(" UNKNOWN CARD! Red Light Activated");
    digitalWrite(redLedPin, HIGH);
    beepBuzzer(3, 100);
    digitalWrite(redLedPin, LOW);
  }

  // Prepare and send data via RF24
  sensorData.card_id = cardId;
  sensorData.authorized = isAuthorized;
  sensorData.batt = analogRead(A2) * (3.3 / 1023.0);  // Assuming battery voltage reading
  sensorData.sensor_id = 1;  // Change this if you have multiple RFID readers

  radio.powerUp();
  radio.write(&sensorData, sizeof(sensorData));
}

// Setup function
void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init(SS_PIN, RST_PIN);  // Initialize RFID with new SS and RST pins
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  radio.begin();
  radio.setChannel(108);
  radio.openWritingPipe(pipe);
  radio.setPALevel(RF24_PA_HIGH);
  radio.enableDynamicPayloads();
  radio.setDataRate(RF24_250KBPS);
  radio.setRetries(8, 10);
  radio.stopListening();
  Serial.println("Bring RFID TAG Closer...\n");
}

// Loop function
void loop() {
  radio.powerDown();
  for (int i = 0; i < 8; i++) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }

  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  uint32_t cardId = 0;
  for (byte i = 0; i < rfid.uid.size; i++) {
    cardId = (cardId << 8) | rfid.uid.uidByte[i];
  }

  const char* name = checkAuthorized(cardId);
  handleCard(cardId, name);
}
