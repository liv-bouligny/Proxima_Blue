/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#line 1 "c:/Users/Jason.000/Documents/IoT/Proxima_Blue/Proxima_Blue_v0-2/src/Proxima_Blue_v0-2.ino"
/*
 * Project: Proxima Blue
 * Description: Detect nearby devices to warn machinery operators or automated machines of potential human presence
 * Author: Liv Bouligny
 * Date: 16 Aug 2022
 */

#include "credentials.h"
#include "Adafruit_SSD1306.h"
#include "Particle.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT/Adafruit_MQTT.h" 
#include "Adafruit_MQTT/Adafruit_MQTT_SPARK.h"

void setup();
void loop();
int isRecognized(byte addr1, byte addr0);
void MQTT_connect();
void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);
#line 15 "c:/Users/Jason.000/Documents/IoT/Proxima_Blue/Proxima_Blue_v0-2/src/Proxima_Blue_v0-2.ino"
TCPClient TheClient;

Adafruit_MQTT_SPARK mqtt(&TheClient,AIO_SERVER,AIO_SERVERPORT,AIO_USERNAME,AIO_KEY); 
Adafruit_MQTT_Subscribe mqttArtemisPull = Adafruit_MQTT_Subscribe(&mqtt,AIO_USERNAME "/feeds/artemisaq");
Adafruit_MQTT_Subscribe mqttAthenaPull = Adafruit_MQTT_Subscribe(&mqtt,AIO_USERNAME "/feeds/athenaaq");
Adafruit_MQTT_Publish mqttArtemisPush = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/artemisfrompersephone");
Adafruit_MQTT_Publish mqttAthenaPush = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/athenafrompersephone");

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;

const size_t SCAN_RESULT_MAX = 30;
const int OLED_RESET = 20;
const int OLEDADD = 0x3C;

Adafruit_SSD1306 display(OLED_RESET);

// These UUIDs were defined by Nordic Semiconductor and are now the defacto standard for
// UART-like services over BLE. Many apps support the UUIDs now, like the Adafruit Bluefruit app.
// const BleUuid serviceUuid("afe7acc5-33a9-478f-bbe1-8944aa08e884");
const BleUuid serviceUuid("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid rxUuid("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid txUuid("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

int counter = 0;
unsigned long lastPub;
long last = -60000;
long scanTime = 10000;
int rssi, i, j, neoSig, artemisSig, athenaSig, artemisAQ, athenaAQ;
bool checked = false;
byte scanMAC[50];

BleCharacteristic txCharacteristic("tx", BleCharacteristicProperty::NOTIFY, txUuid, serviceUuid);
BleCharacteristic rxCharacteristic("rx", BleCharacteristicProperty::WRITE_WO_RSP, rxUuid, serviceUuid, onDataReceived, NULL);
BleAdvertisingData data;

BleScanResult scanResults[SCAN_RESULT_MAX];

// SYSTEM_MODE(SEMI_AUTOMATIC);

void setup() {
  Serial.begin(9600);
  waitFor(Serial.isConnected, 15000);

  (void)logHandler;  // Does nothing, just to eliminate the unused variable warning

  BLE.on ();
  BLE.addCharacteristic(txCharacteristic);
  BLE.addCharacteristic(rxCharacteristic);
  data.appendServiceUUID(serviceUuid);
  BLE.advertise(&data);
    // Set Tx power (-20, -16, -12, -8, -4, 0, 4, 8) & Select the external BLE antenna
  BLE.setTxPower(8);
  BLE.selectAntenna(BleAntennaType::EXTERNAL);

  // Uncomment below to print device BLE Address
  // Serial.printf("Argon BLE Address: %s\n", BLE.address().toString().c_str());

  display.begin(SSD1306_SWITCHCAPVCC, OLEDADD);
  display.stopscroll();
  display.display(); // show splashscreen
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextWrap(false);
  display.setTextColor(WHITE);  
  display.setCursor(0,0);
  display.display();

  mqtt.subscribe(&mqttArtemisPull);
  mqtt.subscribe(&mqttAthenaPull);
}

void loop() {  
  MQTT_connect();

  if (millis() - last > scanTime) {
    j=0;
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(2);
    display.printf("Scanning!");
    display.display();
    display.startscrollleft(0x00,0x03);
    Vector<BleScanResult> scanResults = BLE.scan();
    // For Device OS 2.x and earlier, use scanResults[i].address[0], etc. without the ()
    if (scanResults.size()) {
      display.stopscroll();
      display.clearDisplay();
      Log.info("%d devices found", scanResults.size());
      display.setCursor(0,0);
      display.setTextSize(1);
      display.printf("====Known Devices====\n");
      display.setCursor(0,32);
      display.printf("===Unknown Devices===\n");
      display.display();
      for (i = 0; i < scanResults.size(); i++) {          
        sprintf((char *)scanMAC,"MAC: %02X:%02X:%02X:%02X:%02X:%02X\nRSSI: %i dBm\n",scanResults[i].address()[5], scanResults[i].address()[4], 
          scanResults[i].address()[3], scanResults[i].address()[2], scanResults[i].address()[1], scanResults[i].address()[0], scanResults[i].rssi());
        int device = isRecognized(scanResults[i].address()[1], scanResults[i].address()[0]);
        if (device == 1) {
          artemisSig = scanResults[i].rssi();
          txCharacteristic.setValue(scanMAC, 50);
          Serial.printf("Artemis, device 1: %i\n", artemisSig);
          display.setTextSize(1);
          display.setCursor(0,8);
          display.printf("Artemis nearby!\n");
          display.display();
        }  
        if (device == 2) { 
          athenaSig = scanResults[i].rssi();
          txCharacteristic.setValue(scanMAC, 50);
          Serial.printf("Athena, device 2: %i\n", athenaSig);
          display.setTextSize(1);
          display.setCursor(0,16);
          display.printf("Athena nearby!\n");
          display.display();
        }
        if (device == 3) { 
          neoSig = scanResults[i].rssi();
          txCharacteristic.setValue(scanMAC, 50);
          Serial.printf("Neo Pixel, device 3: %i\n", neoSig);
          display.setTextSize(1);
          display.setCursor(0,24);
          display.printf("Neo nearby!\n");
          display.display();
        }
        if (scanResults[i].rssi() >= -70) {        
          txCharacteristic.setValue(scanMAC, 50);
          if (device == 0) {            
            j++;
          }
        }        
        String name = scanResults[i].advertisingData().deviceName();
        if (name.length() > 0) {
          Log.info("Advertising name: %s", name.c_str());
        }
      }
      if (j > 0) {            
        display.setTextSize(2);
        display.setCursor(0,48);
        display.printf("%i Devices\n", j);
        display.startscrollleft(0x0E, 0x0F);
        display.display();        
      }
    } 
    last = millis();  
    checked = false;  
  }    

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(100))) {
    if (subscription == &mqttArtemisPull) {
      artemisAQ = atoi((char *)mqttArtemisPull.lastread);
    }
    if (subscription == &mqttAthenaPull) {
      athenaAQ = atoi((char *)mqttAthenaPull.lastread);
    }
  }

  if (millis() - lastPub > 10000) {
    if (mqtt.Update()) {
      mqttArtemisPush.publish(artemisAQ);
      Serial.printf("ArtemisAQ: %i\n", artemisAQ);
      mqttAthenaPush.publish(athenaAQ);
      Serial.printf("AthenaAQ: %i\n", athenaAQ);
    }
    lastPub = millis();
  }
}

int isRecognized(byte addr1, byte addr0) {
  // Serial.printf("%02X:%02X\n", addr1, addr0);
  if ((addr1 == 0xF1) || (addr1 == 0x67) || (addr1 == 0xC0)) {
    switch (addr0) { //identify if device is recognized and return value correspondent to device
      case 0x90:        
        return 1;
        break;        
      case 0x3D:        
        return 2;
        break;        
      case 0xC8:
        return 3;
        break;
      default:
        return 0;
        break;
    }
  }
  else {
    return 0;
  }
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