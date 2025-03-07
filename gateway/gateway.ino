/*
  RF24 GATEWAY BY DANIEL VERHAAL
  MQTT CLIENT FOR NRF24L01+ BASED SENSORS


  V3 - Adds heartbeat function
     - Adds solar sensor (id=99)

  V3.1 - Adds OTA!

  NOTE: In file "C:\Users\...\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\...\platform.txt"
  change line: tools.esptool.upload.network_pattern="{network_cmd}" "{runtime.platform.path}/tools/espota.py" -i "{serial.port}" -p "{network.port}" "--auth={network.password}" -f "{build.path}/{build.project_name}.bin"
  to: tools.esptool.upload.network_pattern="{network_cmd}" "{runtime.platform.path}/tools/espota.py" -i "{serial.port}" -p "{network.port}" -P 8266 "--auth={network.password}" -f "{build.path}/{build.project_name}.bin"
  this way the LISTENING port on your development pc is fixed to 8266 (of course you can change it) and you can write a rule on your firewall

  V4.0 - Code overhaul. Using JSON format for temp, door and pir sensor code including smart topic and payloat creation.
         No need for defining topics anymore for temp, door and pir sensors.
         Changes needed in Hassio: Now only 1 mqtt message per sensor which includes attributes (like battery info).
  
  V4.1 - Added battery percentage calculation for TEMP, DOOR & PIR sensors (DOORBELL update WIP)
       - Battery percentage value included in json transmission
  
  V4.2 - Added retain flag to some MQTT transmissions in order for hassio to save last value after restart
       - Code cleanup
*/

/*===========================================================
 ********** DEBUG ON/OFF
  ===========================================================*/

#define DEBUG false                         // flag to turn on/off debugging
#define Serial if(DEBUG)Serial              // if DEBUG is false, Serial is undefined

/*===========================================================
 ********** INCLUSIONS
  ===========================================================*/

#include <SPI.h>                            // needed for nRF24L01
#include "nRF24L01.h"                       // nRF24L01 #1
#include "RF24.h"                           // nRF24L01 #2

#include <ESP8266WiFi.h>                    // ESP8266 library
#include <PubSubClient.h>                   // needed for MQTT

#include <ESP8266mDNS.h>                    // needed for OTA function
#include <WiFiUdp.h>                        // needed for OTA function
#include <ArduinoOTA.h>                     // needed for OTA function 

/*===========================================================
 ********** DEFINITIONS
  ===========================================================*/

#define wifi_ssid "WiCaRo_IoT"             // define SSID
#define wifi_password "Test@2021"        // define Wifi Password

#define mqtt_server "10.0.30.29"           // define MQTT server IP
#define mqtt_user "mqtt_gjlp25"                    // define MQTT user
#define mqtt_password "o67@4ZeNSq&3k8"            // define MQTT password


unsigned long currentMillis;                // long for current time
long previousMillis1 = 0;                   // for storing last time #1
long previousMillis3 = 0;                   // for storing last time #2
const long interval1 = 2000;                // interval for doorbell_flag
const long interval2 = 5000;                // interval for mqtt initialization
const long interval3 = 30000;               // interval for heartbeat

boolean heartbeat = 1;                      // heartbeat for letting Hassio know Gateway is online
boolean doorbell_flag = false;              // flag for indicating whether doorbell is active or not

int ledRed = D0;                            // pin for red status LED connection
int ledGreen = D1;                          // pin for green status LED connection
int ledState = LOW;                         // int for letting status LED blink

int topic;                                  // used to store rf24 pipeline info

char *myTopics[] = {"temp", "doorbel", "door", "pir", "rfid"};  // for topic string creation (0=temp, 1=doorbell etc)
char *offOnState[] = {"off", "on"};                     // used for mqtt payload
char *closedOpenState[] = {"closed", "open"};           // used for mqtt payload

/*===========================================================
 ********** MQTT TOPICS

   All relevant MQTT topics to be defines below.
   Which can be read in Hass.io or something similar
  ===========================================================*/

#define gateway_topic "gateway/heartbeat"

#define doorbell_topic "rf24/doorbell"
#define doorbell_batt_topic "rf24/doorbell_batt"
#define doorbell_flag_topic "rf24/doorbell_flag"


/*===========================================================
 ********** WIFI & RF24 INITIALIZATION
  ===========================================================*/

WiFiClient espClient;                     // initializing ESP8266
PubSubClient client(espClient);           // initializing MQTT client

RF24 radio(D2, D8);                       // initializing RF24 and defining CE & CSN

const uint64_t pipe01 = 0xE7E8C0F0B1LL;   // RF24 address for temp sensors
const uint64_t pipe02 = 0xE7E8C0F0B2LL;   // RF24 address for doorbell
const uint64_t pipe03 = 0xE7E8C0F0B3LL;   // RF24 address for door sensors
const uint64_t pipe04 = 0xE7E8C0F0B4LL;   // RF24 address for PIR sensors
const uint64_t pipe05 = 0xE7E8C0F0B5LL;   // RF24 address for RFID sensors


/*===========================================================
 ********** STRUCTURES

   Same data structures should be used in relevant sensor code.
  ===========================================================*/

//STRUCTURE FOR TEMP SENSORS
struct {
  float temp;
  float hum;
  float pres;
  float batt;
  unsigned char sensor_id;
} sensorData1;

//STRUCTURE FOR DOORBELL
struct {
  float batt;
  byte doorbell;
} sensorData2;

//STRUCTURE FOR DOOR SENSORS
struct {
  float batt;
  unsigned char sensor_id;
  boolean door;
} sensorData3;

//STRUCTURE FOR PIR SENSORS
struct {
  float batt;
  unsigned char sensor_id;
  boolean motion;
} sensorData4;

//STRUCTURE FOR RFID SENSORS
struct {
  uint32_t card_id;
  bool authorized;
  float batt;
  unsigned char sensor_id;
  char user_name[16];  // Added user name field
} sensorData5;

/*===========================================================
 ********** BATTERY PERCENTAGE CALCULATION
  ===========================================================*/
  
float sensorBatt;

float battery_perc() {
  float output = ((sensorBatt - 3.3) / (4.2 - 3.3)) * 100;

  if (output < 100)
    return output;
  else
    return 100.0f;
}

/*===========================================================
 ********** SETUP
  ===========================================================*/

void setup() {
  Serial.begin(9600);

  pinMode(ledGreen, OUTPUT);
  pinMode(ledRed, OUTPUT);

  digitalWrite(ledGreen, LOW);
  digitalWrite(ledRed, HIGH);

  setup_wifi();

  ArduinoOTA.setPassword((const char *)"password");
  ArduinoOTA.setHostname("RF24Gateway");
  ArduinoOTA.onError([](ota_error_t error) {
    ESP.restart();
  });
  ArduinoOTA.begin();

  client.setServer(mqtt_server, 1883);


  radio.begin();
  //  radio.setPayloadSize(8);
  radio.setChannel(108);
  radio.openReadingPipe(1, pipe01);
  radio.openReadingPipe(2, pipe02);
  radio.openReadingPipe(3, pipe03);
  radio.openReadingPipe(4, pipe04);
  radio.openReadingPipe(5, pipe05);
  radio.setPALevel(RF24_PA_HIGH);
  radio.enableDynamicPayloads();
  radio.setDataRate(RF24_250KBPS);
  radio.startListening();
}

/*===========================================================
 ********** WIFI CONNECTION & RE-CONNECTION
  =============================================================*/

void setup_wifi() {
  digitalWrite(ledRed, HIGH);
  digitalWrite(ledGreen, LOW);
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.mode(WIFI_STA);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    // set the LED with the ledState of the variable:
    digitalWrite(ledRed, ledState);
    delay(1000);
    Serial.print(".");
  }
  digitalWrite(ledRed, HIGH);
  digitalWrite(ledGreen, HIGH);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected

  while (WiFi.status() != WL_CONNECTED) {
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    // set the LED with the ledState of the variable:
    digitalWrite(ledRed, ledState);
    digitalWrite(ledGreen, LOW);
    delay(1000);
    Serial.print(".");
  }

  while (!client.connected()) {
    digitalWrite(ledRed, HIGH);
    digitalWrite(ledGreen, HIGH);
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client2")) {
    if (client.connect("RF24Gateway", mqtt_user, mqtt_password)) {
      digitalWrite(ledRed, LOW);
      digitalWrite(ledGreen, HIGH);
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      // Wait 5 seconds before retrying
      currentMillis = millis();
      previousMillis1 = currentMillis;
      while (currentMillis - previousMillis1 <= interval2) {
        currentMillis = millis();

        // if the LED is off turn it on and vice-versa:
        if (ledState == LOW) {
          ledState = HIGH;
        } else {
          ledState = LOW;
        }

        // set the LED with the ledState of the variable:
        digitalWrite(ledRed, ledState);
        digitalWrite(ledGreen, ledState);
        delay(500);
      }
    }
  }
}

/*===========================================================
 ********** MAIN LOOP
  ===========================================================*/

void loop() {

  ArduinoOTA.handle();                                        // check for OTA update

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  byte pipeRxd;
  if (radio.available(&pipeRxd)) {
    topic = pipeRxd;
    
    if (topic == 1) {
      radio.read(&sensorData1, sizeof(sensorData1));
      MQTT_sensorData1();
    }
    else if (topic == 2) {
      radio.read(&sensorData2, sizeof(sensorData2));
      MQTT_sensorData2();
    }
    else if (topic == 3) {
      radio.read(&sensorData3, sizeof(sensorData3));
      MQTT_sensorData3();
    }
    else if (topic == 4) {
      radio.read(&sensorData4, sizeof(sensorData4));
      MQTT_sensorData4();
    }
    else if (topic == 5) {
      radio.read(&sensorData5, sizeof(sensorData5));
      MQTT_sensorData5();
    }
  }

//=========DOORBEL-OFF SIGNAL FUNCTION=======================

  currentMillis = millis();

  if (doorbell_flag == true) {
    if (currentMillis - previousMillis1 > interval1) {
      doorbell_flag = false;
      client.publish(doorbell_flag_topic, "off", true);
      previousMillis1 = 0;
    }
  }

//=========GATEWAY HEARTBEAT SIGNAL FUNCTION=================

  if (currentMillis - previousMillis3 > interval3) {
    if (heartbeat == 1) {
      client.publish(gateway_topic, "on");
      heartbeat = 0;
      previousMillis3 = millis();
    }
    else {
      client.publish(gateway_topic, "off");
      heartbeat = 1;
      previousMillis3 = millis();
    }
  }
}

/*===========================================================
 ********** MQTT DATA 1 (TEMP/HUM/PRESS)
  ===========================================================*/

void MQTT_sensorData1() {
  char tempString[5];
  dtostrf(sensorData1.temp, 1, 1, tempString);

  char humString[5];
  dtostrf(sensorData1.hum, 1, 1, humString);

  char presString[7];
  dtostrf((sensorData1.pres / 100), 1, 1, presString);

  sensorBatt = sensorData1.batt;
  char battPercString[1];
  dtostrf(battery_perc(), 1, 0, battPercString);

  char battString[5];
  dtostrf(sensorData1.batt, 1, 2, battString);

  String topicString = "rf24/" + String(myTopics[topic - 1]) + String(sensorData1.sensor_id);
//  int length = topicString.length();
  const char *topicBuffer;
  topicBuffer = topicString.c_str();

  String payloadString = "{\"temp\":\"" + String(tempString) + "\",\"hum\":\"" + String(humString) + "\",\"press\":\"" + String(presString) + "\",\"batt\":\"" + String(battString) + "\",\"batt_perc\":\"" + String(battPercString) + "\"}";
//  length = payloadString.length();
  const char *payloadBuffer;
  payloadBuffer = payloadString.c_str();

  client.publish(topicBuffer, payloadBuffer);
}


/*===========================================================
 ********** MQTT DATA 2 (DOORBELL)
  ===========================================================*/

void MQTT_sensorData2() {

  if (sensorData2.doorbell == 1) {
    client.publish(doorbell_topic, "on");
  }

  else if (sensorData2.doorbell == 0) {
    client.publish(doorbell_topic, "off", true);
    char battString[5];
    dtostrf(sensorData2.batt, 1, 2, battString);
    client.publish(doorbell_batt_topic, battString, true);
  }

  else if (sensorData2.doorbell == 2) {
    client.publish(doorbell_flag_topic, "on");
    previousMillis1 = millis();
    doorbell_flag = true;
  }
}


/*===========================================================
 ********** MQTT DATA 3 (DOOR-SENSOR)
  ===========================================================*/

void MQTT_sensorData3() {

  String topicString = "rf24/" + String(myTopics[topic - 1]) + String(sensorData3.sensor_id);
//  int length = topicString.length();
  const char *topicBuffer;
  topicBuffer = topicString.c_str();

  sensorBatt = sensorData3.batt;
  char battPercString[1];
  dtostrf(battery_perc(), 1, 0, battPercString); 

  char battString[5];
  dtostrf(sensorData3.batt, 1, 2, battString);

  String payloadString = "{\"door\":\"" + String(closedOpenState[sensorData3.door]) + "\",\"batt\":\"" + String(battString) + "\",\"batt_perc\":\"" + String(battPercString) + "\"}";
//  length = payloadString.length();
  const char *payloadBuffer;
  payloadBuffer = payloadString.c_str();

  client.publish(topicBuffer, payloadBuffer, true);
}


/*===========================================================
 ********** MQTT DATA 4 (PIR)
  ===========================================================*/
  
void MQTT_sensorData4() {

  String topicString = "rf24/" + String(myTopics[topic - 1]) + String(sensorData4.sensor_id);
//  int length = topicString.length();
  const char *topicBuffer;
  topicBuffer = topicString.c_str();

  sensorBatt = sensorData4.batt;
  char battPercString[1];
  dtostrf(battery_perc(), 1, 0, battPercString); 

  char battString[5];
  dtostrf(sensorData4.batt, 1, 2, battString);
 
  String payloadString = "{\"motion\":\"" + String(offOnState[sensorData4.motion]) + "\",\"batt\":\"" + String(battString) + "\",\"batt_perc\":\"" + String(battPercString) + "\"}";
//  length = payloadString.length();
  const char *payloadBuffer;
  payloadBuffer = payloadString.c_str();

  client.publish(topicBuffer, payloadBuffer, true);
}

/*===========================================================
 ********** MQTT DATA 5 (RFID)
  ===========================================================*/
  
void MQTT_sensorData5() {
  String topicString = "rf24/" + String(myTopics[topic - 1]) + String(sensorData5.sensor_id);
  const char *topicBuffer;
  topicBuffer = topicString.c_str();

  sensorBatt = sensorData5.batt;
  char battPercString[1];
  dtostrf(battery_perc(), 1, 0, battPercString);

  char battString[5];
  dtostrf(sensorData5.batt, 1, 2, battString);

  char cardIdString[11];
  sprintf(cardIdString, "%lu", sensorData5.card_id);
  
  // Calculate signal quality based on retry count
  int retryCount = radio.getARC(); // Get Auto Retry Count
  int signalQuality = 100;
  if(retryCount > 0) {
    signalQuality = max(0, 100 - (retryCount * 20)); // Reduce quality by 20% per retry
  }
 
  String payloadString = "{\"card_id\":\"" + String(cardIdString) + 
                        "\",\"user_name\":\"" + String(sensorData5.user_name) + 
                        "\",\"authorized\":\"" + String(sensorData5.authorized ? "true" : "false") + 
                        "\",\"batt\":\"" + String(battString) + 
                        "\",\"batt_perc\":\"" + String(battPercString) + 
                        "\",\"retries\":\"" + String(retryCount) + 
                        "\",\"signal\":\"" + String(signalQuality) + 
                        "\",\"connected\":\"" + String(radio.isChipConnected() ? "true" : "false") + 
                        "\"}";
  const char *payloadBuffer;
  payloadBuffer = payloadString.c_str();

  client.publish(topicBuffer, payloadBuffer, true);
}
