/*
 * Project Proxima_Blue_Beacon_sensor
 * Description: Beacon device intended to broadcast BLE to communicate Air Quality data with Central devices0
 * Author: Liv Bouligny 
 * Date: 19 August 2022
 */

#include "credentials.h"
#include "Grove_Air_quality_Sensor.h"
#include "IOTTimer.h"
#include "DeviceNameHelperRK.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT/Adafruit_MQTT.h" 
#include "Adafruit_MQTT/Adafruit_MQTT_SPARK.h"

TCPClient TheClient;
Adafruit_MQTT_SPARK mqtt(&TheClient,AIO_SERVER,AIO_SERVERPORT,AIO_USERNAME,AIO_KEY); 
Adafruit_MQTT_Publish mqttArtemis = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/artemisaq");
Adafruit_MQTT_Publish mqttAthena = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/athenaaq");

AirQualitySensor sensor(A0);
int quality;
unsigned long last, lastCheck, lastTX;
const int BUZZPIN = D3;
byte dataAQ[25];

//Used for DeviceNameHelperRK
SerialLogHandler logHandler;
SYSTEM_THREAD(ENABLED);
int EEPROM_OFFSET = 1;

// These UUIDs were defined by Nordic Semiconductor and are now the defacto standard for
// UART-like services over BLE. Many apps support the UUIDs now, like the Adafruit Bluefruit app.
const BleUuid serviceUuid("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid rxUuid("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid txUuid("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

BleCharacteristic txCharacteristic("tx", BleCharacteristicProperty::NOTIFY, txUuid, serviceUuid);
BleCharacteristic rxCharacteristic("rx", BleCharacteristicProperty::WRITE_WO_RSP, rxUuid, serviceUuid, onDataReceived, NULL);

BleAdvertisingData data;

// SYSTEM_MODE(SEMI_AUTOMATIC); //Using BLE and not Wifi

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

    // Used for DeviceNameHelperRK / You must call this from setup!
  DeviceNameHelperEEPROM::instance().setup(EEPROM_OFFSET);
  //Log.info("name=%s", DeviceNameHelperEEPROM::instance().getName());  
  Serial.printf("deviceName = %s\n", DeviceNameHelperEEPROM::instance().getName());
}

void loop() {  
  MQTT_connect();
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
  if (millis() - lastTX > 10000) {
    if (mqtt.Update())  {
      // mqttArtemis.publish(sensor.getValue());
      mqttAthena.publish(sensor.getValue());
    }
    sprintf((char *)dataAQ,"%s: %i\n", DeviceNameHelperEEPROM::instance().getName(), sensor.getValue());
    Serial.printf("%s: %i\n", DeviceNameHelperEEPROM::instance().getName(), sensor.getValue());
    dataAQ[24] = 0x0A;    
    txCharacteristic.setValue(dataAQ, 25);
    lastTX = millis();
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

void MQTT_connect() {
  int8_t ret;
 
  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }
 
  Serial.print("Connecting to MQTT... ");
 
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.printf("%s\n",(char *)mqtt.connectErrorString(ret));
    Serial.printf("Retrying MQTT connection in 5 seconds..\n");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
  }
  Serial.printf("MQTT Connected!\n");
}