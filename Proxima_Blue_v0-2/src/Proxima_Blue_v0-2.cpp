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
#line 15 "c:/Users/Jason.000/Documents/IoT/Proxima_Blue/Proxima_Blue_v0-2/src/Proxima_Blue_v0-2.ino"
TCPClient TheClient;
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
BleUuid counterCharacteristicUuid("520be804-2cc6-455e-b449-558a4555687e");

BlePeerDevice peer;
BleCharacteristic counterCharacteristic;
BleCharacteristic athenaCharacteristic;
BleCharacteristic artemisCharacteristic;

enum class State {
	SCAN,
	CONNECT,
	RUN,
	WAIT
};

State state = State::SCAN;
BleAddress serverAddr;
int counter = 0;
unsigned long lastScan, stateTime;
long last = -60000;
long scanTime = 1000;
BleAddress peripheralAddr;
int rssi, i, j, neoSig, infiniSig, roombaSig, scanCount, duckCount, infiniCount, roombaCount;
int neoDuck[50];
int infinity[50];
int roomba[50];
byte scanMAC[50];

void onPairingEvent(const BlePairingEvent& event, void* context);
void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);
void scanResultCallback(const BleScanResult *scanResult, void *context);
void stateConnect();
void stateRun();


BleCharacteristic txCharacteristic("tx", BleCharacteristicProperty::NOTIFY, txUuid, serviceUuid);
BleCharacteristic rxCharacteristic("rx", BleCharacteristicProperty::WRITE_WO_RSP, rxUuid, serviceUuid, onDataReceived, NULL);
BleAdvertisingData data;

BleScanResult scanResults[SCAN_RESULT_MAX];
LEDStatus ledOverride(RGB_COLOR_WHITE, LED_PATTERN_SOLID, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);

void scanResultCallback(const BleScanResult *scanResult, void *context);

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

  switch(state) {
		case State::SCAN:
      Serial.printf("scan\n");
			state = State::WAIT;
			BLE.scan(scanResultCallback, NULL);
			break;
			
		case State::CONNECT:
      Serial.printf("connect\n");
			stateConnect();
			break;

    case State::RUN:
      //Serial.printf("run\n");
			stateRun();
			break;

    case State::WAIT:
      Serial.printf("wait\n");
			if (millis() - stateTime >= 5000) {
				state = State::SCAN;
			}
			break;
	}

  // if (millis() - last > scanTime) {
  //   // scanCount = 0;    
  //   // for (j = 0; j < 10; j++) {
  //     // scanCount++;
  //     // Serial.printf("Scan %i\n%u ms Start\n",scanCount, millis());      
  //       //Vector Scan
  //     // BLE.setScanTimeout(100);
  //     Vector<BleScanResult> scanResults = BLE.scan();
  //     if (scanResults.size()) {
  //       Log.info("%d devices found", scanResults.size());
  //       for (i = 0; i < scanResults.size(); i++) {
  //         // For Device OS 2.x and earlier, use scanResults[i].address[0], etc. without the ()
  //         Serial.printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X | RSSI: %i dBm",
  //           scanResults[i].address()[5], scanResults[i].address()[4], scanResults[i].address()[3],
  //           scanResults[i].address()[2], scanResults[i].address()[1], scanResults[i].address()[0], scanResults[i].rssi());
  //         sprintf((char *)scanMAC,"MAC: %02X:%02X:%02X:%02X:%02X:%02X\nRSSI: %i dBm\n",scanResults[i].address()[5], scanResults[i].address()[4], 
  //           scanResults[i].address()[3], scanResults[i].address()[2], scanResults[i].address()[1], scanResults[i].address()[0], scanResults[i].rssi());
  //         int device = isRecognized(scanResults[i].address()[1], scanResults[i].address()[0]);
  //         if (device == 1) {    
  //           BlePeerDevice peer = BLE.connect(scanResults[i].address());
  //           if (peer.connected()) {
  //             peerTxCharacteristic = peer.getCharacteristicByUUID(txUuid);
  //             peerRxCharacteristic = peer.getCharacteristicByUUID(rxUuid);              
  //           }
  //           neoSig = scanResults[i].rssi();
  //           txCharacteristic.setValue(scanMAC, 50);
  //           Serial.printf("NeoDuck device 1: %i\n", neoSig);
  //         }  
  //         if (device == 2) { 
  //           infiniSig = scanResults[i].rssi();
  //           txCharacteristic.setValue(scanMAC, 50);
  //           Serial.printf("Infinity device 2: %i\n", infiniSig);
  //         }
  //         if (device == 3) { 
  //           roombaSig = scanResults[i].rssi();
  //           txCharacteristic.setValue(scanMAC, 50);
  //           Serial.printf("Roomba device 3: %i\n", roombaSig);
  //         }
  //         if (scanResults[i].rssi() >= -80) {
  //           txCharacteristic.setValue(scanMAC, 50);
  //         }
  //         // Serial.printf("====================\nDistance\nNeoPixel: %i\nInfinity Cube: %i\nRoomba: %i\n====================\n", neoSig, infiniSig, roombaSig);
  //         display.clearDisplay();
  //         display.setCursor(0,0);
  //         display.printf("====================\nDistance\nNeoPixel: %i\nInfinity Cube: %i\nRoomba: %i\n====================\n", neoSig, infiniSig, roombaSig);
  //         display.display();
  //         String name = scanResults[i].advertisingData().deviceName();
  //         // if (name.length() > 0) {
  //         //   Log.info("Advertising name: %s", name.c_str());
  //         // }
  //       }
  //     }
  //     Serial.printf("Scan %i\n%u ms End\n",scanCount, millis()); 
  //   // }    
  //   last = millis();    
  // }    
}

void scanResultCallback(const BleScanResult *scanResult, void *context) {
  BleUuid foundServiceUuid;
	size_t svcCount = scanResult->advertisingData().serviceUUID(&foundServiceUuid, 1);
	if (svcCount == 0 || !(foundServiceUuid == serviceUuid)) {
		//Log.info("ignoring %02X:%02X:%02X:%02X:%02X:%02X, not our service",
		//		scanResult->address()[0], scanResult->address()[1], scanResult->address()[2],
		//		scanResult->address()[3], scanResult->address()[4], scanResult->address()[5]);
		return;
	}
  Log.info("found server %02X:%02X:%02X:%02X:%02X:%02X",
    scanResult->address()[0], scanResult->address()[1], scanResult->address()[2],
    scanResult->address()[3], scanResult->address()[4], scanResult->address()[5]);
    serverAddr = scanResult->address();
	state = State::CONNECT;
  
  	// We always stop scanning after the first lock device is found
	BLE.stopScanning();
}

void onPairingEvent(const BlePairingEvent& event, void* context) {
	if (event.type == BlePairingEventType::REQUEST_RECEIVED) {
		Log.info("onPairingEvent REQUEST_RECEIVED");
	}
	else
	if (event.type == BlePairingEventType::PASSKEY_DISPLAY) {
		char passKeyStr[BLE_PAIRING_PASSKEY_LEN + 1];
		memcpy(passKeyStr, event.payload.passkey, BLE_PAIRING_PASSKEY_LEN);
		passKeyStr[BLE_PAIRING_PASSKEY_LEN] = 0;
    Log.info("onPairingEvent PASSKEY_DISPLAY %s", passKeyStr);
	}
	else
	if (event.type == BlePairingEventType::STATUS_UPDATED) {
		Log.info("onPairingEvent STATUS_UPDATED status=%d lesc=%d bonded=%d", 
			event.payload.status.status,
			(int)event.payload.status.lesc,
			(int)event.payload.status.bonded);
	}
	else
	if (event.type == BlePairingEventType::NUMERIC_COMPARISON) {
		Log.info("onPairingEvent NUMERIC_COMPARISON");
	}
}

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
  size_t i;
  display.setCursor(0,0);
  display.printf("Received data from: %02X:%02X:%02X:%02X:%02X:%02X \n", peer.address()[0], peer.address()[1],peer.address()[2], peer.address()[3], peer.address()[4], peer.address()[5]);
  display.display();
  Serial.printf("Received data from: %02X:%02X:%02X:%02X:%02X:%02X \n", peer.address()[0], peer.address()[1],peer.address()[2], peer.address()[3], peer.address()[4], peer.address()[5]);
  for (i = 0; i < len; i++) {
      Serial.printf("%c",data[i]);
      display.printf("%c",data[i]);
      display.display();
  }
  Serial.printf("\n");
  display.printf("\n");
  display.display();
}

void stateConnect() {
	Log.info("about to connect");
  
  peer = BLE.connect(serverAddr);
	if (!peer.connected()) {
		Log.info("failed to connect, retrying");
		state = State::WAIT;
		stateTime = millis();
		return;
	}
	Log.info("Connected, starting pairing...");
  
  peer.getCharacteristicByUUID(counterCharacteristic, counterCharacteristicUuid);
	
	BLE.startPairing(peer);

  state = State::RUN;
	stateTime = millis();
}

void stateRun() {
	int counter2;
  counter2 = random(0x30,0x3A);
  if (!peer.connected()) {
		// Server disconnected
		state = State::WAIT;
		stateTime = millis();
		return;
	}
	if (BLE.isPairing(peer)) {
		// Still in process of doing LESC secure pairing
		return;
	}
	if (millis() - stateTime < 2000) {
		return;
	}
	stateTime = millis();
  
  counterCharacteristic.setValue((const uint8_t *)&counter2, sizeof(counter2));
	Log.info("sent counter=0x%02X", counter2);
	counter++;
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

// int getClosest(int _rssiArray[], int _count) {
//   int medArray[_count];
//   for (byte jj = 0; jj < _count; jj++) {
//     medArray[jj] = _rssiArray[jj];
//   }
//   int ii, jj, temp;
//   for (jj = 0; jj < _count - 1; jj++) {
//     for (ii = 0; ii < _count - jj - 1; ii++) {
//       if (medArray[ii] > medArray[ii+1]) {      
//         temp = medArray[ii];
//         medArray[ii] = medArray[ii+1];
//         medArray[ii+1] = temp;
//       }
//     }
//   }  
//   if ((_count & 1) > 0) {
//     temp = medArray[(_count - 1) / 2];
//   }
//   else {
//     temp = (medArray[_count / 2] + medArray[(_count / 2)-1]) / 2;
//   }
//   return temp;
// }