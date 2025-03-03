/*
  Arduino Pro Mini 3.3V/8MHz
  Battery optimized / DHT22 Sensor / NRF24L01
*/


/*===========================================================
 ********** INCLUSIONS
===========================================================*/

#include "LowPower.h"     // include LowPower library for deepsleep
#include <SPI.h>          // include SPI library for communication with nRF24L01 transceiver
#include "nRF24L01.h"     // include nRF24L01 library for transceiver
#include "RF24.h"         // include RF24 library for protocol
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>


/*===========================================================
 ********** RF24 INITIALIZATION
===========================================================*/


RF24 radio(15, 14);         // CE, CSN 


const uint64_t pipe01 = 0xE7E8C0F0B1LL;   // temp/hum sensors
//const uint64_t pipe02 = 0xE8E8F0F0A2LL;   // deurbel
//const uint64_t pipe03 = 0xE8E8F0F0A3LL;   // deur sensors
//const uint64_t pipe03 = 0xE8E8F0F0A4LL;   // beweging sensors

/*===========================================================
 ********** OTHER SETUP
===========================================================*/
// vout = rb/(ra+rb)*vin
// 1.05 = 330/(1000+330)*4.2

float aref_fix = 1.065;

//const int wakeUpPin = 2;      // connect PIR OUT to pin 2

Adafruit_BME280 bme; // I2C

/*===========================================================
 ********** STRUCTURES
===========================================================*/

struct{
  float temp;
  float hum;
  float pres;
  float batt;
  unsigned char sensor_id=3;
}sensorData1;


void wakeUp()
{
    // Just a handler for the pin interrupt.
}


/*===========================================================
 ********** SETUP
===========================================================*/

void setup() {

  analogReference(INTERNAL);

  bme.begin(0x76);
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X1, // temperature
                  Adafruit_BME280::SAMPLING_X1, // pressure
                  Adafruit_BME280::SAMPLING_X1, // humidity
                  Adafruit_BME280::FILTER_OFF   );

  radio.begin();
//  radio.setPayloadSize(8);
  radio.setChannel(108);
  radio.openWritingPipe(pipe01);
  radio.setPALevel(RF24_PA_HIGH);
  radio.enableDynamicPayloads();
  radio.setDataRate(RF24_250KBPS);
  radio.setRetries(8,10);
  radio.stopListening();

}

/*===========================================================
 ********** MAIN LOOP
===========================================================*/

void loop() {
    
  radio.powerDown();
  
  // 64 s / 8 s = 8
  
  unsigned int sleepCounter;
  for (sleepCounter = 8; sleepCounter > 0; sleepCounter--){
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }

  ReadSensors();
  ReadBattery();

  radio.powerUp();

  radio.write(&sensorData1, sizeof(sensorData1));
  



}
/*===========================================================
 ********** READ SENSORS
===========================================================*/

void ReadSensors(){
  bme.takeForcedMeasurement();
  sensorData1.temp=bme.readTemperature();
  sensorData1.hum=bme.readHumidity();
  sensorData1.pres=bme.readPressure();
}

/*===========================================================
 ********** READ BATTERY
===========================================================*/

void ReadBattery(){
  int reading = analogRead(A2);
  float vout = (reading*aref_fix) / 1023.0;
  sensorData1.batt = (1330/330)*vout;
}
