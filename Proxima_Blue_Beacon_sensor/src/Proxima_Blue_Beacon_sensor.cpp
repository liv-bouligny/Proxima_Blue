/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "c:/Users/Jason.000/Documents/IoT/Proxima_Blue/Proxima_Blue_Beacon_sensor/src/Proxima_Blue_Beacon_sensor.ino"
/*
 * Project Proxima_Blue_Beacon_sensor
 * Description: Beacon device intended to broadcast BLE to communicate Air Quality data with Central devices0
 * Author: Liv Bouligny 
 * Date: 19 August 2022
 */

#include "Grove_Air_quality_Sensor.h"
#include "IOTTimer.h"

void setup();
void loop();
void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);
#line 11 "c:/Users/Jason.000/Documents/IoT/Proxima_Blue/Proxima_Blue_Beacon_sensor/src/Proxima_Blue_Beacon_sensor.ino"
AirQualitySensor sensor(A0);
int quality;
unsigned long last, lastCheck;
const int BUZZPIN = D3;

// These UUIDs were defined by Nordic Semiconductor and are now the defacto standard for
// UART-like services over BLE. Many apps support the UUIDs now, like the Adafruit Bluefruit app.
const BleUuid serviceUuid("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid rxUuid("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid txUuid("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

BleCharacteristic txCharacteristic("tx", BleCharacteristicProperty::NOTIFY, txUuid, serviceUuid);
BleCharacteristic rxCharacteristic("rx", BleCharacteristicProperty::WRITE_WO_RSP, rxUuid, serviceUuid, onDataReceived, NULL);
BleAdvertisingData data;

SYSTEM_MODE(SEMI_AUTOMATIC); //Using BLE and not Wifi

void setup() {
  Serial.begin(9600);

  Serial.printf("Waiting sensor to init...\n");
  delay(15000);
  
  if (sensor.init()) {
    Serial.printf("Sensor ready.\n");
  }
  else {
    Serial.printf("Sensor ERROR!\n");
  }

  //Turn on Bluetooth, establish tx and rx characteristics, set Service UUID, and advertise data
  BLE.on ();
  BLE.addCharacteristic(txCharacteristic);
  BLE.addCharacteristic(rxCharacteristic);
  data.appendServiceUUID(serviceUuid);
  BLE.advertise(&data);
  Serial.printf("Argon BLE Address: %s\n", BLE.address().toString().c_str());

}

void loop() {
  quality = sensor.slope();  

  if (millis() - lastCheck > 1000) {
    Serial.printf("Sensor value: %i\n", sensor.getValue());
    if (quality == AirQualitySensor::FORCE_SIGNAL) {
      Serial.printf("High pollution! Force signal active.\n");
      if (millis() - last > 2000) {
        tone(BUZZPIN, 440, 500);
        last = millis();
      }
    }
    else if (quality == AirQualitySensor::HIGH_POLLUTION) {
      Serial.printf("High pollution!\n");
      if (millis() - last > 3000) {
        tone(BUZZPIN, 220, 250);        
        last = millis();
      }
    }
    else if (quality == AirQualitySensor::LOW_POLLUTION) {
      Serial.printf("Low pollution!\n");
      if (millis() - last > 5000) {
        tone(BUZZPIN, 110, 50);        
      }
      last = millis();
    }
    else if (quality == AirQualitySensor::FRESH_AIR) {
      Serial.printf("Fresh air.\n");
    }  
    lastCheck = millis();
  }  
}

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
  uint8_t i;
  
  Serial.printf("Received data from:%02X:%02X:%02X:%02X:%02X:%02X\n",peer.address()[0], peer.address()[1], peer.address()[2], peer.address()[3], peer.address()[4], peer.address()[5]);
  Serial.printf("Bytes: ");
  for (i = 0;i < len; i++) {
    Serial.printf("%02X ",data[i]);
  }
  Serial.printf("\n");
  Serial.printf("Message: %s\n",(char *)data);  
}