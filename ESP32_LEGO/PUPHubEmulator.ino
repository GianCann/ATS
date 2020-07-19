#include <M5Atom.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
 
#define PORT_A 0x00
#define PORT_B 0x01
#define PORT_C 0x02
#define PORT_D 0x03

#define PORT_HUB_LIGHT 0x32

#define PORT_ID 0x03
#define MSG_TYPE 0x02

#define OUT_PORT_SUB_CMD_TYPE 5
#define OUT_PORT_CMD 0x81
#define OUT_PORT_CMD_WRITE_DIRECT 0x51
#define OUT_PORT_FBK 0x082

#define HUB_ACTION_CMD 0x02
#define ACTION_SWITCH_OFF 0x01

#define WRITE_DIRECT_VALUE 0x07

int led_color=1;
int new_led_color=1;
 
#define SERVICE_UUID        "00001623-1212-EFDE-1623-785FEABCD123"
#define CHARACTERISTIC_UUID "00001624-1212-EFDE-1623-785FEABCD123"
BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
BLEAdvertisementData oScanResponseData = BLEAdvertisementData();
 
const char advLEGO[] = {0x02,0x01,0x06,0x11,0x07,0x23,0xD1,0xBC,0xEA,0x5F,0x78,0x23,0x16,0xDE,0xEF,
                          0x12,0x12,0x23,0x16,0x00,0x00,0x09,0xFF,0x97,0x03,0x00,0x41,0x07,0xB2,0x43,0x00};                    

//Techinc HUB
const char  ArrManufacturerData[8] = {0x97,0x03,0x00,0x80,0x06,0x00,0x41,0x00};

//City HUB
//const char  ArrManufacturerData[8] = {0x97,0x03,0x00,0x41,0x07,0xB2,0x43,0x00};

std::string ManufacturerData(ArrManufacturerData ,sizeof(ArrManufacturerData));

BLEServer* pServer = NULL;
BLECharacteristic* pLEGOCharacteristic = NULL;
 
 // Set your new MAC Address
uint8_t newMACAddress[] = {0x90, 0x84, 0x2B, 0x4A, 0x3A, 0x0A};

bool deviceConnected = false;
bool initialInfo=false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      initialInfo=false;
      Serial.println("Device connected");
    };

    void onDisconnect(BLEServer* pServer) {
      M5.dis.drawpix(0, 0xFFFFFF);
      deviceConnected = false;
      initialInfo=false;
      Serial.println("Device disconnected");
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      
      std::string msgReceived = pCharacteristic->getValue();

      if (msgReceived.length() > 0) {

        Serial.print("Rcv ");
        Serial.print(msgReceived.length());
        Serial.print(" bytes :");

        for (int i = 2; i < msgReceived.length(); i++){
          if (i==MSG_TYPE){
            Serial.print(" MSG_TYPE > ");
          } else if (i == (MSG_TYPE + 1)){
            Serial.print(" PAYLOAD > ");
          }
           
          Serial.print("0x");
          Serial.print(msgReceived[i], HEX);
          Serial.print("(");
          Serial.print(msgReceived[i],DEC);
          Serial.print(")");
          Serial.print(" ");
         } 
         Serial.println("");
          
      //if MSG_TYPE=2 and Payload=1, it's a shutdown request
      // [len] [0] [HUB_ACTION_CMD] [Command]
      if (msgReceived[MSG_TYPE]==HUB_ACTION_CMD && msgReceived[3]==ACTION_SWITCH_OFF){
          Serial.print("Disconnect");
            delay(30);
            byte msgDisconnectionReply[]={0x04, 0x00, 0x02, 0x31};
            pLEGOCharacteristic->setValue(msgDisconnectionReply,sizeof(msgDisconnectionReply));
            pLEGOCharacteristic->notify();
            delay(100);
            //uint16_t IdConnessione = pServer->getConnId();
            //pServer->disconnect(IdConnessione);
          Serial.print("Restart the micro");
          delay(1000);
          ESP.restart();
      }

      //It's a port out command:
      //execute and send feedback to the App
      if (msgReceived[MSG_TYPE]==OUT_PORT_CMD){
        Serial.println("Port command received");
        delay(30);

        //Reply to the App "Command excecuted"
        byte msgPortCommandFeedbackReply[]={0x05, 0x00, 0x82, 0x00, 0x0A}; //0x0A Command complete+buffer empty+idle
        msgPortCommandFeedbackReply[PORT_ID]=msgReceived[PORT_ID]; //set the port_id
        pLEGOCharacteristic->setValue(msgPortCommandFeedbackReply,sizeof(msgPortCommandFeedbackReply));
        pLEGOCharacteristic->notify();

        int port_id_value;
        int port_write_value;

        if (msgReceived[OUT_PORT_SUB_CMD_TYPE] == OUT_PORT_CMD_WRITE_DIRECT){
          Serial.print("Write Direct on port: ");
          
          port_id_value=msgReceived[PORT_ID];
          port_write_value=msgReceived[WRITE_DIRECT_VALUE];
          Serial.print(port_id_value, HEX);
          Serial.print(" Value: ");
          Serial.println(port_write_value, HEX);

          //Port A
          if (port_id_value == PORT_A){
            CRGB c_pixel_color = 0x000000;
            if (port_write_value > 0 && port_write_value < 101){
              Serial.println("LED ON on PORT A");
              c_pixel_color=0xFFFFFF;   
            } else {
              Serial.println("LED OFF on PORT A");
            }
            M5.dis.drawpix(0,2, c_pixel_color);
            M5.dis.drawpix(1,2, c_pixel_color);
            M5.dis.drawpix(0,3, c_pixel_color);
            M5.dis.drawpix(1,3, c_pixel_color);
          }

          //Port B
          if (port_id_value == PORT_B){
            CRGB c_pixel_color = 0x000000;
            if (port_write_value > 0 && port_write_value < 101){
              Serial.println("LED ON on PORT B");
              c_pixel_color=0xFFFFFF;   
            } else {
              Serial.println("LED OFF on PORT B");
            }
            M5.dis.drawpix(3,2, c_pixel_color);
            M5.dis.drawpix(4,2, c_pixel_color);
            M5.dis.drawpix(3,3, c_pixel_color);
            M5.dis.drawpix(4,3, c_pixel_color);
          }

         //Hub Light
          if (port_id_value == PORT_HUB_LIGHT){
            Serial.println("HUB LIGHT");
            new_led_color=port_write_value;
          }
         
        }
      }
    }
 }

  void onRead(BLECharacteristic *pCharacteristic) {
    Serial.println("Read request");
    uint8_t CharTemp[]={0x0F, 0x00, 0x04};
    //pCharacteristic->setValue(CharTemp,3);
  }
  
  };

/*
// | BLECharacteristic::PROPERTY_NOTIFY
BLECharacteristic LEGOCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | 
                                      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR | 
                                      BLECharacteristic::PROPERTY_NOTIFY);
                                      */

void setup() {
 
  Serial.begin(115200);
  Serial.println("");

  set_led_color(led_color);
  
  M5.begin(true, false, true);
  delay(50);
  M5.dis.drawpix(0, 0xFFFFFF);
    
  Serial.println("Starting BLE work!");

  esp_base_mac_addr_set(&newMACAddress[0]);
  BLEDevice::init("Fake Hub");

  Serial.println("Create server");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  Serial.println("Create service");
  BLEService *pService = pServer->createService(SERVICE_UUID);

   // Create a BLE Characteristic
  pLEGOCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_WRITE_NR
                    );
  // Create a BLE Descriptor and set the callback
  pLEGOCharacteristic->addDescriptor(new BLE2902());
  pLEGOCharacteristic->setCallbacks(new MyCallbacks());

  Serial.println("Service start");
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
 
  pAdvertising->setScanResponse(true);

  oScanResponseData.setShortName("Fake Hub");
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

  //Hub light: start with white color. Blue when the App is connected
  if (led_color != new_led_color){
    led_color=new_led_color;
    set_led_color(led_color);
  }
  
      if (deviceConnected==true){
          if (initialInfo==false){
            initialInfo=true;
            delay(1000);
            Serial.println("Send Hub port configuration");
            
            //Boost motor on Port_A
            byte PORT_A_INFORMATION[]={0x0F, 0x00, 0x04, 0x00, 0x01, 0x26, 0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x10};
            pLEGOCharacteristic->setValue(PORT_A_INFORMATION,15);
            pLEGOCharacteristic->notify();
            delay(100);
            //Boost motor on Port_B
            byte PORT_B_INFORMATION[]={0x0F, 0x00, 0x04, 0x01, 0x01, 0x26, 0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x10};
            pLEGOCharacteristic->setValue(PORT_B_INFORMATION,15);
            pLEGOCharacteristic->notify();
            delay(100);
            //Boost motor on Port_C
            byte PORT_C_INFORMATION[]={0x0F, 0x00, 0x04, 0x02, 0x01, 0x26, 0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x10};
            pLEGOCharacteristic->setValue(PORT_C_INFORMATION,15);
            pLEGOCharacteristic->notify();
            delay(100);
            //Boost motor on Port_D
            byte PORT_D_INFORMATION[]={0x0F, 0x00, 0x04, 0x03, 0x01, 0x26, 0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x10};
            pLEGOCharacteristic->setValue(PORT_D_INFORMATION,15);
            pLEGOCharacteristic->notify();
            delay(100);
            //Led
            byte PORT_LIGHT_INFORMATION[]={0x0F, 0x00, 0x04, 0x32, 0x01, 0x17, 0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x10};
            pLEGOCharacteristic->setValue(PORT_LIGHT_INFORMATION,15);
            pLEGOCharacteristic->notify();
          }

        //TODO: add here the code to send value from the sensor
          
         delay(50);
        }

    delay(50);
    M5.update();
}

void set_led_color(int color){

  CRGB n_color = 0x000000;
  
  int led_position=0; //position on neopixel LED
  switch (color) {
    case 0: //none
      n_color=0x000000;
      break;
    case 1: //???
      n_color=0x000000;  
      break;
    case 2: //viola
      n_color=0xFF00FF;  
      break;
     case 3: //blu
      n_color=0x0000FF;  
      break;
     case 4: //celeste
      n_color=0x00ffff;  
      break;
     case 5: //verde acqua
      n_color=0x00cc99;  
      break;
     case 6: //verde
      n_color=0x00cc00;  
      break;
     case 7: //giallo
      n_color=0xffff00;  
      break;
     case 8: //arancione
      n_color=0xff6600;  
      break;
     case 9: //rosso
      n_color=0xFF0000;  
      break;
     case 10: //bianco
      n_color=0xFFFFFF; 
      break;
  }
  M5.dis.drawpix(led_position, n_color);  
}


