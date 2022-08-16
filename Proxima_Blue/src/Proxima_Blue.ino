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

unsigned long lastScan;
BleAddress peripheralAddr;
int rssi;
byte scanMAC[50];

BleCharacteristic txCharacteristic("tx", BleCharacteristicProperty::NOTIFY, txUuid, serviceUuid);
BleCharacteristic rxCharacteristic("rx", BleCharacteristicProperty::WRITE_WO_RSP, rxUuid, serviceUuid, onDataReceived, NULL);
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
  //Vector Scan
  Vector<BleScanResult> scanResults = BLE.scan();

  if (scanResults.size()) {
    Log.info("%d devices found", scanResults.size());

    for (int ii = 0; ii < scanResults.size(); ii++) {
      // For Device OS 2.x and earlier, use scanResults[ii].address[0], etc. without the ()
      Log.info("MAC: %02X:%02X:%02X:%02X:%02X:%02X | RSSI: %i dBm",
        scanResults[ii].address()[5], scanResults[ii].address()[4], scanResults[ii].address()[3],
        scanResults[ii].address()[2], scanResults[ii].address()[1], scanResults[ii].address()[0], scanResults[ii].rssi());
      sprintf((char *)scanMAC,"MAC: %02X:%02X:%02X:%02X:%02X:%02X\nRSSI: %i dBm\n",scanResults[ii].address()[5], scanResults[ii].address()[4], 
        scanResults[ii].address()[3], scanResults[ii].address()[2], scanResults[ii].address()[1], scanResults[ii].address()[0], scanResults[ii].rssi());
      // if (scanResults[ii].rssi() > -85) {
        txCharacteristic.setValue(scanMAC, 50);
      // }
      String name = scanResults[ii].advertisingData().deviceName();
      if (name.length() > 0) {
        Log.info("Advertising name: %s", name.c_str());
      }
    }
  }
  delay(3000);

  // if (millis() - lastScan > 500) {
  //   lastScan = millis();

  //   rssi = 0;
  //   BLE.scan(scanResultCallback, NULL);

  //   // display.clearDisplay();
  //   if (rssi) {
  //     display.setTextSize(2);
  //     display.setTextColor(WHITE);
  //     display.setCursor(0,0);

  //     char buf[32];
  //     snprintf(buf, sizeof(buf), "%i\n",rssi);
  //     display.println(buf);
  //   }
  //   display.display();
  // }

  // Only scan for 500 milliseconds
  // BLE.setScanTimeout(500);
  // int count = BLE.scan(scanResults,SCAN_RESULT_MAX);

  // uint32_t curColorCode;
  // int curRssi = -999;

  // for (int ii = 0; ii < count; ii++) {
  //   uint8_t buf[BLE_MAX_ADV_DATA_LEN];
  //   size_t len;
  // When getting a specific AD Type, the length returned does not include the length or AD Type so len will be one less
  // than what we put in the beacon code, because that includes the AD Type.
    // len = scanResults[ii].advertisingData().get(BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA, buf, BLE_MAX_ADV_DATA_LEN);
    // if (len == 7) {
      // We have manufacturer-specific advertising data (0xff) and it's 7 bytes (without the AD type)

			// Byte: BLE_SIG_AD_TYPE_MANUFACTURER_SPECIFIC_DATA (0xff)
			// 16-bit: Company ID (0xffff)
			// Byte: Internal packet identifier (0x55)
			// 32-bit: Color code
    //   if (buf[0] == 0xff && buf[1] == 0xff && buf[2] == 0x55) {
    //     uint32_t colorCode;
    //     memcpy(&colorCode, &buf[3], 4);

    //     Log.info("colorCode: 0x%lx rssi=%d address=%02X:%02X:%02X:%02X:%02X:%02X ",
    //       colorCode, scanResults[ii].rssi(),scanResults[ii].address()[0],scanResults[ii].address()[1], scanResults[ii].address()[2],
    //       scanResults[ii].address()[3], scanResults[ii].address()[4], scanResults[ii].address()[5]);
    //     Serial.printf("colorCode: 0x%lx rssi=%d address=%02X:%02X:%02X:%02X:%02X:%02X ",
    //       colorCode, scanResults[ii].rssi(),scanResults[ii].address()[0],scanResults[ii].address()[1], scanResults[ii].address()[2],
    //       scanResults[ii].address()[3], scanResults[ii].address()[4], scanResults[ii].address()[5]); 
    //     if (scanResults[ii].rssi() > curRssi) {
    //       // Show whatever device has the strongest signal
    //       curRssi = scanResults[ii].rssi();
		// 			curColorCode = colorCode;          
    //     }
    //   }
    // }
  // }
  // if (curRssi != -999) {
  //   ledOverride.setColor(curColorCode);
  //   ledOverride.setActive(true);
  // }
  // else {
  //   ledOverride.setActive(false);
  // }
}

void scanResultCallback(const BleScanResult *scanResult, void *context) {

  BleUuid foundServiceUuid;
	size_t svcCount = scanResult->advertisingData().serviceUUID(&foundServiceUuid, 1);
	if (svcCount == 0 || !(foundServiceUuid == serviceUuid)) {
		/*
		Log.info("ignoring %02X:%02X:%02X:%02X:%02X:%02X, not our service",
				scanResult->address()[0], scanResult->address()[1], scanResult->address()[2],
				scanResult->address()[3], scanResult->address()[4], scanResult->address()[5]);
		*/
		return;
	}
  display.clearDisplay();
	Log.info("rssi=%d \nserver=%02X:%02X:%02X:%02X:%02X:%02X\n",
    scanResult->rssi(), scanResult->address()[5], scanResult->address()[4], scanResult->address()[3], 
    scanResult->address()[2], scanResult->address()[1], scanResult->address()[0]);

  Serial.printf("rssi=%d\nserver= %02X:%02X:%02X:%02X:%02X:%02X\n",
    scanResult->rssi(), scanResult->address()[5], scanResult->address()[4], scanResult->address()[3], 
    scanResult->address()[2], scanResult->address()[1], scanResult->address()[0]);

  display.setTextSize(1);
  display.printf("rssi=%d\nserver= \n%02X:%02X:%02X:%02X:%02X:%02X\n",
    scanResult->rssi(), scanResult->address()[5], scanResult->address()[4], scanResult->address()[3], 
    scanResult->address()[2], scanResult->address()[1], scanResult->address()[0]);
  display.display();
	peripheralAddr = scanResult->address();
	rssi = scanResult->rssi();
	
	BLE.stopScanning();
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

void getArgonSignalData() {

}