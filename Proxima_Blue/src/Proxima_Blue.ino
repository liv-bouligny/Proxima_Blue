/*
 * Project: Proximity_Detection
 * Description: Detect nearby devices to warn machinery operators or automated machines of nearby safety concerns
 * Author: Liv Bouligny
 * Date: 16 Aug 2022
 */

#include "Adafruit_SSD1306.h"

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

unsigned long lastScan, last;
BleAddress peripheralAddr;
int rssi, i, j, neoSig, infiniSig, roombaSig, scanCount, duckCount, infiniCount, roombaCount;
int neoDuck[50];
int infinity[50];
int roomba[50];
byte scanMAC[50];


BleCharacteristic txCharacteristic("tx", BleCharacteristicProperty::NOTIFY, txUuid, serviceUuid);
// BleCharacteristic rxCharacteristic("rx", BleCharacteristicProperty::WRITE_WO_RSP, rxUuid, serviceUuid, onDataReceived, NULL);
BleAdvertisingData data;

BleScanResult scanResults[SCAN_RESULT_MAX];
LEDStatus ledOverride(RGB_COLOR_WHITE, LED_PATTERN_SOLID, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);

void scanResultCallback(const BleScanResult *scanResult, void *context);

SYSTEM_MODE(SEMI_AUTOMATIC);

void setup() {
  Serial.begin(9600);
  waitFor(Serial.isConnected, 15000);

  (void)logHandler;  // Does nothing, just to eliminate the unused variable warning

  BLE.on ();
  BLE.addCharacteristic(txCharacteristic);
  // BLE.addCharacteristic(rxCharacteristic);
  data.appendServiceUUID(serviceUuid);
  BLE.advertise(&data);
  BLE.setTxPower(8);
  // Select the external antenna
  BLE.selectAntenna(BleAntennaType::EXTERNAL);
  Serial.printf("Argon BLE Address: %s\n", BLE.address().toString().c_str());

  display.begin(SSD1306_SWITCHCAPVCC, OLEDADD);
  display.display(); // show splashscreen
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.display();
}

void loop() {
  if (millis() - last > 60000) {
    scanCount = 0;
    for (j = 0; j < 10; j++) {
      scanCount++;
      Serial.printf("Scan %i\n",scanCount);      
        //Vector Scan
      Vector<BleScanResult> scanResults = BLE.scan();
      if (scanResults.size()) {
        Log.info("%d devices found", scanResults.size());
        for (i = 0; i < scanResults.size(); i++) {
          // For Device OS 2.x and earlier, use scanResults[i].address[0], etc. without the ()
          // Log.info("MAC: %02X:%02X:%02X:%02X:%02X:%02X | RSSI: %i dBm",
          //   scanResults[i].address()[5], scanResults[i].address()[4], scanResults[i].address()[3],
          //   scanResults[i].address()[2], scanResults[i].address()[1], scanResults[i].address()[0], scanResults[i].rssi());
          sprintf((char *)scanMAC,"MAC: %02X:%02X:%02X:%02X:%02X:%02X\nRSSI: %i dBm\n",scanResults[i].address()[5], scanResults[i].address()[4], 
            scanResults[i].address()[3], scanResults[i].address()[2], scanResults[i].address()[1], scanResults[i].address()[0], scanResults[i].rssi());
          if (isRecognized(scanResults[i].address()[1], scanResults[i].address()[0]) == 1) {            
            neoDuck[duckCount] = scanResults[i].rssi();
            txCharacteristic.setValue(scanMAC, 50);
          }  
          if (isRecognized(scanResults[i].address()[1], scanResults[i].address()[0]) == 2) { 
              infinity[infiniCount] = scanResults[i].rssi();
              txCharacteristic.setValue(scanMAC, 50);
          }
          if (isRecognized(scanResults[i].address()[1], scanResults[i].address()[0]) == 3) { 
              roomba[roombaCount] = scanResults[i].rssi();
              txCharacteristic.setValue(scanMAC, 50);
          }           
          // String name = scanResults[i].advertisingData().deviceName();
          // if (name.length() > 0) {
          //   Log.info("Advertising name: %s", name.c_str());
          // }
        }
      }
    }
    neoSig = getMedian(neoDuck, duckCount);
    infiniSig = getMedian(infinity, infiniCount);
    roombaSig = getMedian(roomba, roombaCount);
    Serial.printf("====================\nDistance\nNeoPixel: %i\nInfinity Cube: %i\nRoomba: %i\n====================\n", neoSig, infiniSig, roombaSig);
    display.clearDisplay();
    display.setCursor(0,0);
    display.printf("====================\nDistance\nNeoPixel: %i\nInfinity Cube: %i\nRoomba: %i\n====================\n", neoSig, infiniSig, roombaSig);
    display.display();
    last = millis();
    BLE.stopScanning();
  }    
}

// void scanResultCallback(const BleScanResult *scanResult, void *context) {

//   BleUuid foundServiceUuid;
// 	size_t svcCount = scanResult->advertisingData().serviceUUID(&foundServiceUuid, 1);
// 	if (svcCount == 0 || !(foundServiceUuid == serviceUuid)) {
// 		/*
// 		Log.info("ignoring %02X:%02X:%02X:%02X:%02X:%02X, not our service",
// 				scanResult->address()[0], scanResult->address()[1], scanResult->address()[2],
// 				scanResult->address()[3], scanResult->address()[4], scanResult->address()[5]);
// 		*/
// 		return;
// 	}
//   display.clearDisplay();
// 	Log.info("rssi=%d \nserver=%02X:%02X:%02X:%02X:%02X:%02X\n",
//     scanResult->rssi(), scanResult->address()[5], scanResult->address()[4], scanResult->address()[3], 
//     scanResult->address()[2], scanResult->address()[1], scanResult->address()[0]);

//   Serial.printf("rssi=%d\nserver= %02X:%02X:%02X:%02X:%02X:%02X\n",
//     scanResult->rssi(), scanResult->address()[5], scanResult->address()[4], scanResult->address()[3], 
//     scanResult->address()[2], scanResult->address()[1], scanResult->address()[0]);

//   display.setTextSize(1);
//   display.printf("rssi=%d\nserver= \n%02X:%02X:%02X:%02X:%02X:%02X\n",
//     scanResult->rssi(), scanResult->address()[5], scanResult->address()[4], scanResult->address()[3], 
//     scanResult->address()[2], scanResult->address()[1], scanResult->address()[0]);
//   display.display();
// 	peripheralAddr = scanResult->address();
// 	rssi = scanResult->rssi();
	
// 	BLE.stopScanning();
// }

// void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
//   uint8_t i;
  
//   Serial.printf("Received data from:%02X:%02X:%02X:%02X:%02X:%02X\n",peer.address()[0], peer.address()[1], peer.address()[2], peer.address()[3], peer.address()[4], peer.address()[5]);
//   Serial.printf("Bytes: ");

//   for (i = 0;i < len; i++) {
//     Serial.printf("%02X ",data[i]);
//   }
//   Serial.printf("\n");
//   Serial.printf("Message: %s\n",(char *)data);
// }

int isRecognized(byte addr1, byte addr0) {
  if (addr1 == (0xF0 || 0xEF || 0xC0)) {
    switch (addr1) { //identify if device is recognized and return value correspondent to device
      case 0xA8:        
        return 2;
        break;        
      case 0xEF:        
        return 3;
        break;        
      case 0xC0:
        return 1;
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

int getMedian(int _rssiArray[], int _count) {
  int medArray[_count];
  for (byte jj = 0; jj < _count; jj++) {
    medArray[jj] = _rssiArray[jj];
  }
  int ii, jj, temp;
  for (jj = 0; jj < _count - 1; jj++) {
    for (ii = 0; ii < _count - jj - 1; ii++) {
      if (medArray[ii] > medArray[ii+1]) {      
        temp = medArray[ii];
        medArray[ii] = medArray[ii+1];
        medArray[ii+1] = temp;
      }
    }
  }  
  if ((_count & 1) > 0) {
    temp = medArray[(_count - 1) / 2];
  }
  else {
    temp = (medArray[_count / 2] + medArray[(_count / 2)-1]) / 2;
  }
  return temp;
}