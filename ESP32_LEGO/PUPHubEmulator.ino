/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updates by chegewara
*/
// #include <Arduino.h>
 
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
 
// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
 
#define SERVICE_UUID        "00001623-1212-EFDE-1623-785FEABCD123"
#define CHARACTERISTIC_UUID "00001624-1212-EFDE-1623-785FEABCD123"
BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
BLEAdvertisementData oScanResponseData = BLEAdvertisementData();
 
const char advLEGO[] = {0x02,0x01,0x06,0x11,0x07,0x23,0xD1,0xBC,0xEA,0x5F,0x78,0x23,0x16,0xDE,0xEF,
                          0x12,0x12,0x23,0x16,0x00,0x00,0x09,0xFF,0x97,0x03,0x00,0x80,0x06,0x00,0x61,0x00};                    
 
 const char  ArrManufacturerData[8] = {0x97,0x03,0x00,0x80,0x06,0x00,0x41,0x00};
 std::string ManufacturerData(ArrManufacturerData ,sizeof(ArrManufacturerData));
 
const char  ArrScanRsponseData[] = {0x05,0x12,0x10,0x00,0x20,0x00,0x02,0x0a,0x00,0x0c,0x09,0x54,0x65,0x63,0x68,0x6e,0x69,0x63,0x20,0x48,0x75,0x62};
std::string ScanResponseData(ArrScanRsponseData ,sizeof(ArrScanRsponseData));
 
 
 // Set your new MAC Address
uint8_t newMACAddress[] = {0x90, 0x84, 0x2B, 0x4A, 0x3A, 0x0A};

bool deviceConnected = false;
bool initialInfo=false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Device connected");
  
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      initialInfo=false;
      Serial.println("Device disconnected");
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {

        Serial.print("Rcv ");
        Serial.print(rxValue.length());
        Serial.print(" bytes :");

        for (int i = 2; i < rxValue.length(); i++) 
          {

          if (i==2){
            Serial.print(" MSG_TYPE=");
            }
          else if (i==3){
            Serial.print(" PAYLOAD=");
            }
          
          Serial.print("0x");
          Serial.print(rxValue[i], HEX);
          
          if (i>=3){
            Serial.print("(");
            Serial.print(rxValue[i]);
            Serial.print(")");
            }
          Serial.print(" ");
          } 
    
    } 
  }

  void onRead(BLECharacteristic *pCharacteristic) {
    Serial.println("Read request");
    uint8_t CharTemp[]={0x0F, 0x00, 0x04};
    pCharacteristic->setValue(CharTemp,3);
  }
  
  };


BLECharacteristic LEGOCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_NOTIFY);

void setup() {
 
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Starting BLE work!");

  esp_base_mac_addr_set(&newMACAddress[0]);
  BLEDevice::init("Technic Hub");

  Serial.println("Create server");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  Serial.println("Create service");
  BLEService *pService = pServer->createService(SERVICE_UUID);

  Serial.println("Add charactetistic to service")
  pService->addCharacteristic(&LEGOCharacteristic);

  LEGOCharacteristic.setCallbacks(new MyCallbacks());

  Serial.println("Service start");
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
 
  pAdvertising->setScanResponse(true);
  oAdvertisementData.setShortName("ESP32 Hub");

  //oAdvertisementData.setManufacturerData(ManufacturerData);
  //oAdvertisementData.addData(advLEGO);
  //pAdvertising->setAdvertisementData(oAdvertisementData);

  oScanResponseData.setManufacturerData(ManufacturerData);
  oScanResponseData.addData(advLEGO);

  pAdvertising->setScanResponseData(oScanResponseData);

  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);

  Serial.println("Start adv");
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
}

int i=0;
 
void loop() {
  
      //questo codice viene eseguito solo una volta
      //ovvero quando viene stabilita una nuova connessione
      if (deviceConnected==true){
          if (initialInfo==false){
            initialInfo=true;
            delay(200);
            
            //nota: questo codice in realtà sembrerebbe non funzionare
            //quanto meno, i dati non vengono ricevuti dall'App.
            //Potrebbe essere una questione di tempistica?
            byte CharValue[]={0x0F, 0x00, 0x04, 0x00, 0x01, 0x26, 0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x10};
            LEGOCharacteristic.setValue(CharValue,15);
            LEGOCharacteristic.notify();
            Serial.println("Send initial value to the App");
          }
         delay(1000);
    
         //questi dati, invece, vengono ricevuti...
         //LEGOCharacteristic.setValue("Prova");
         //LEGOCharacteristic.notify();
   
         Serial.println("Running");
        }
}
