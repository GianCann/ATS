#include <M5Atom.h>
#include <WiFi.h>
#include <PubSubClient.h>



// Connection info.
const char* ssid = "LABINF";
const char* password =  "YOUR_NETWORK_PASSWORD";
const char* mqttServer = "mqtt.pndsn.com";
const int mqttPort = 1883;
const char* clientID = "YOUR_PUB_KEY_HERE/YOUR_SUB_KEY_HERE/CLIENT_ID";
const char* channelName = "hello_world";


WiFiClient MQTTclient;
PubSubClient client(MQTTclient);
void callback(char* topic, byte* payload, unsigned int length) {
  String payload_buff;
  for (int i=0;i<length;i++) {
    payload_buff = payload_buff+String((char)payload[i]);
  }
  Serial.println(payload_buff); // Print out messages.
}
long lastReconnectAttempt = 0;
boolean reconnect() {
  if (client.connect(clientID)) {
    client.subscribe(channelName); // Subscribe to channel.
  }
  return client.connected();
}
void setup() {
  Serial.begin(9600);
  Serial.println("Attempting to connect...");
  WiFi.begin(ssid, password); // Connect to WiFi.
  if(WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("Couldn't connect to WiFi.");
      while(1) delay(100);
  }
  client.setServer(mqttServer, mqttPort); // Connect to PubNub.
  client.setCallback(callback);
  lastReconnectAttempt = 0;
}
void loop() {
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) { // Try to reconnect.
      lastReconnectAttempt = now;
      if (reconnect()) { // Attempt to reconnect.
        lastReconnectAttempt = 0;
      }
    }
  } else { // Connected.
    client.loop();
    client.publish(channelName,"Hello world!"); // Publish message.
    delay(1000);
  }
}

/**
 * A Legoino example to control a train which has a motor connected
 * to the Port A of the Hub
 * 
 * (c) Copyright 2019 - Cornelius Munz
 * Released under MIT License
 * 
 */

#include "PoweredUpRemote.h"
#include "PoweredUpHub.h"

// create a hub instance
PoweredUpHub myTrainHub1;
PoweredUpHub myTrainHub2;
PoweredUpHub myTrainHub3;
PoweredUpHub myTrainHub4;
PoweredUpHub myTrainHub5;
PoweredUpHub myTrainHub6;
PoweredUpHub myTrainHub7;
PoweredUpHub myTrainHub8;
PoweredUpHub myTrainHub9;
//PoweredUpRemote myRemote1;
//PoweredUpRemote myRemote2;

bool iConnected = false;
int iDelay=50;

int iLoop=0;
int iColor=1;
Color ledColor = WHITE;

PoweredUpHub::Port _port = PoweredUpHub::Port::A;

void setup() {
    Serial.begin(115200);
    Serial.println("");
    M5.begin(true, false, true);
    delay(50);
    M5.dis.drawpix(0, 0x000000);
    Serial.println("");
    Serial.println("Wait for connection");


} 

// main loop
void loop() {

  if (!myTrainHub1.isConnected() && !myTrainHub1.isConnecting()) 
  {
    myTrainHub1.init(); // initalize the PoweredUpHub instance
    //myTrainHub.init("90:84:2b:03:19:7f"); //example of initializing an hub with a specific address
  }

  // connect flow. Search for BLE services and try to connect if the uuid of the hub is found
  if (myTrainHub1.isConnecting()) {
    myTrainHub1.connectHub();
    if (myTrainHub1.isConnected()) {
      Serial.println("Connected to HUB1");
      M5.dis.drawpix(0, 0xFF0000);
      myTrainHub2.init();
    } else {
      Serial.println("Failed to connect to HUB");
    }
  }
  

  if (myTrainHub2.isConnecting()) {
    myTrainHub2.connectHub();
    if (myTrainHub2.isConnected()) {
      Serial.println("Connected to HUB2");
      M5.dis.drawpix(1, 0xFF0000);
      myTrainHub3.init();
    } else {
      Serial.println("Failed to connect to HUB");
    }
  }

  if (myTrainHub3.isConnecting()) {
    myTrainHub3.connectHub();
    if (myTrainHub3.isConnected()) {
      Serial.println("Connected to HUB3");
      M5.dis.drawpix(2, 0xFF0000);
      myTrainHub4.init();
    } else {
      Serial.println("Failed to connect to HUB");
    }
  }

   if (myTrainHub4.isConnecting()) {
    myTrainHub4.connectHub();
    if (myTrainHub4.isConnected()) {
      Serial.println("Connected to HUB4");
      M5.dis.drawpix(3, 0xFF0000);
      myTrainHub5.init();
    } else {
      Serial.println("Failed to connect to HUB");
    }
  }

     if (myTrainHub5.isConnecting()) {
    myTrainHub5.connectHub();
    if (myTrainHub5.isConnected()) {
      Serial.println("Connected to HUB5");
      M5.dis.drawpix(4, 0xFF0000);
      myTrainHub6.init();
    } else {
      Serial.println("Failed to connect to HUB");
    }
  }

     if (myTrainHub6.isConnecting()) {
    myTrainHub6.connectHub();
    if (myTrainHub6.isConnected()) {
      Serial.println("Connected to HUB6");
      M5.dis.drawpix(5, 0xFF0000);
      myTrainHub7.init();
    } else {
      Serial.println("Failed to connect to HUB");
    }
  }

     if (myTrainHub7.isConnecting()) {
    myTrainHub7.connectHub();
    if (myTrainHub7.isConnected()) {
      Serial.println("Connected to HUB7");
      M5.dis.drawpix(6, 0xFF0000);
      myTrainHub8.init();
    } else {
      Serial.println("Failed to connect to HUB");
    }
  }

   if (myTrainHub8.isConnecting()) {
    myTrainHub8.connectHub();
    if (myTrainHub8.isConnected()) {
      Serial.println("Connected to HUB8");
      M5.dis.drawpix(7, 0xFF0000);
      myTrainHub9.init();
    } else {
      Serial.println("Failed to connect to HUB");
    }
  }

   if (myTrainHub9.isConnecting()) {
    myTrainHub9.connectHub();
    if (myTrainHub9.isConnected()) {
      Serial.println("Connected to HUB9");
      M5.dis.drawpix(8, 0xFF0000);
      iConnected=true;
      //myTrainHub9.init();
    } else {
      Serial.println("Failed to connect to HUB");
    }
  }
 

  /*
  
  if (myRemote1.isConnecting()) {
    myRemote1.connectHub();
    if (myRemote1.isConnected()) {
      Serial.println("Connected to Remote1");
      M5.dis.drawpix(6, 0xFF0000);
      myRemote2.init();
    } else {
      Serial.println("Failed to connect to HUB");
    }
  }

  if (myRemote2.isConnecting()) {
    myRemote2.connectHub();
    if (myRemote2.isConnected()) {
      Serial.println("Connected to Remote2");
      M5.dis.drawpix(7, 0xFF0000);
      iConnected=true;
    } else {
      Serial.println("Failed to connect to HUB");
    }
  }

  */

  delay(50);
   M5.update();
   /*
      BLACK = 0,
  PINK = 1,
  PURPLE = 2,
  BLUE = 3,
  LIGHTBLUE = 4,
  CYAN = 5,
  GREEN = 6,
  YELLOW = 7,
  ORANGE = 8,
  RED = 9,
  WHITE = 10,
  NONE = 255
    */

   switch (iColor) {
    case 1:
      ledColor=PINK;
      break;
    case 2:
      ledColor=PURPLE;
      break;
    case 3:
      ledColor=BLUE;
      break;
    case 4:
      ledColor=LIGHTBLUE;
      break;
     case 5:
      ledColor=CYAN;
      break;
     case 6:
      ledColor=GREEN;
      break;
     case 7:
      ledColor=YELLOW;
      break;
     case 8:
      ledColor=ORANGE;
      break;
     case 9:
      ledColor=RED;
      break;
    case 10:
      ledColor=WHITE;
      break;
  }

 if (iConnected==true){
    myTrainHub1.setLedColor(ledColor);
    delay(iDelay);
    myTrainHub2.setLedColor(ledColor);
    delay(iDelay);
    myTrainHub3.setLedColor(ledColor);
    delay(iDelay);
    myTrainHub4.setLedColor(ledColor);
    delay(iDelay);
    myTrainHub5.setLedColor(ledColor);
    delay(iDelay);
    myTrainHub6.setLedColor(ledColor);
    delay(iDelay);
    myTrainHub7.setLedColor(ledColor);
    delay(iDelay);
    myTrainHub8.setLedColor(ledColor);
    delay(iDelay);
    myTrainHub9.setLedColor(ledColor);
    delay(iDelay);
    /*
    myRemote1.setLedColor(ledColor);
    delay(iDelay);
    myRemote2.setLedColor(ledColor);
    delay(iDelay);
    */
    iColor++;
    iLoop++;
 }

  if (iColor==11){
    iColor=1;
  }

  if (iLoop==20) {
    iLoop=0;
    if (iConnected==true){
      myTrainHub1.shutDownHub();
      delay(iDelay);
      M5.dis.drawpix(0, 0x000000);
      myTrainHub2.shutDownHub();
      delay(iDelay);
      M5.dis.drawpix(1, 0x000000);
      myTrainHub3.shutDownHub();
      delay(iDelay);
      M5.dis.drawpix(2, 0x000000);
      myTrainHub4.shutDownHub();
      delay(iDelay);
      M5.dis.drawpix(3, 0x000000);
      myTrainHub5.shutDownHub();
      delay(iDelay);
      M5.dis.drawpix(4, 0x000000);
      myTrainHub6.shutDownHub();
      delay(iDelay);
      M5.dis.drawpix(5, 0x000000);
      myTrainHub7.shutDownHub();
      delay(iDelay);
      M5.dis.drawpix(6, 0x000000);
      myTrainHub8.shutDownHub();
      delay(iDelay);
      M5.dis.drawpix(7, 0x000000);
      myTrainHub9.shutDownHub();
      delay(iDelay);
      M5.dis.drawpix(8, 0x000000);
      /*
      myRemote1.shutDownHub();
      delay(iDelay);
      M5.dis.drawpix(6, 0x000000);
      myRemote2.shutDownHub();
      delay(iDelay);
      M5.dis.drawpix(7, 0x000000);
      */
      iConnected=false;
    }
  }
  
} // End of loop
