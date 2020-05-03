
/*
  MQTT to PowerFunction
  
  Use an ESP8266 to drive an IR Leds array and send commands to LEGO PF receivers

  Theory: 4 receivers, 2 channel per receiver
  Tested: 1 receiver, 2 channel per receiver
  
  Ivan: 03.05.2020 added RGBLed library, test, code cleanup
  Ivan: 19.02.2019 payload parsing
  Ivan: 12.02.2019 first

  Additional Libraries (manual install: download zip, add library):
  RGB: https://github.com/wilmouths/RGBLed  
  PF:  https://github.com/jurriaan/Arduino-PowerFunctions

  IR circuit: 2x 940nm IR Leds in series with 25 ohm resistor on PWM pin.
  IR Led: http://www.everlight.com/file/ProductFile/IR333-A.pdf

  Note:
  Libreria Arduino PF necessita patch: rename RED/BLUE ports to PF_RED/PF_BLUE

  Richiede file config.h con SSID/Password etc. Esempio di contenuto:
  
  #define CONFIG_SSID "ITS"
  #define CONFIG_PASS "trenotreno"
  #define CONFIG_MQTT_HOST "192.168.1.5"
  #define CONFIG_MQTT_PORT 1883

*/


#include <RGBLed.h>         
#include <PowerFunctions.h> 
#include <ESP8266WiFi.h>
#include <PubSubClient.h>   // MQTT
#include "config.h"

// pins

// WeMos 
const int LED_R  = D7;
const int LED_G  = D6;
const int LED_B  = D5;
const int LED_IR = D0;  

/*
// Arduino UNO
const int LED_R  = 9;
const int LED_G  = 6;
const int LED_B  = 5;
const int LED_IR = 3;  
*/

// PF 
// hack
#define PWM_SIZE 16

typedef struct {
   char* k;
   uint8_t v;
 } pwmval;

pwmval pwms[PWM_SIZE];

// payload parsing
#define CMD_SIZE 3
String cmd[CMD_SIZE];

PowerFunctions pf0(LED_IR, 0);  
PowerFunctions pf1(LED_IR, 1);    
PowerFunctions pf2(LED_IR, 2);    
PowerFunctions pf3(LED_IR, 3);    


// WiFi & MQTT
const char* ssid        = CONFIG_SSID;
const char* password    = CONFIG_PASS;
const char* mqtt_server = CONFIG_MQTT_HOST;
int         mqtt_port   = CONFIG_MQTT_PORT;

int  value   = 0;
long lastMsg = 0;
char msg[50];

WiFiClient espClient;
PubSubClient client(espClient);

RGBLed led(LED_R, LED_G, LED_B, COMMON_CATHODE);



void setup() {


  // setup builtin led e accende per notifica avvio setup
  pinMode(LED_BUILTIN, OUTPUT);     
  digitalWrite(LED_BUILTIN, LOW);  // WeMos D1 mini ha logica inversa

  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, LOW);
  
  Serial.println("Testing led.");
  test_led(led, 250);


 
  Serial.begin(115200);


  //setup_wifi();
  
  if(WiFi.status() == WL_CONNECTED) {
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
  }
   

      
  // Setup PF
  // PF speed definitions
  pwms[0] = (pwmval) {"PWM_FLT" , 0x0};
  pwms[1] = (pwmval) {"PWM_FWD1", 0x1};
  pwms[2] = (pwmval) {"PWM_FWD2", 0x2};
  pwms[3] = (pwmval) {"PWM_FWD3", 0x3};
  pwms[4] = (pwmval) {"PWM_FWD4", 0x4};
  pwms[5] = (pwmval) {"PWM_FWD5", 0x5};
  pwms[6] = (pwmval) {"PWM_FWD6", 0x6};
  pwms[7] = (pwmval) {"PWM_FWD7", 0x7};
  pwms[8] = (pwmval) {"PWM_BRK", 0x8};
  pwms[9] = (pwmval) {"PWM_REV7", 0x9};
  pwms[10] = (pwmval) {"PWM_REV6", 0xA};
  pwms[11] = (pwmval) {"PWM_REV5", 0xB};
  pwms[12] = (pwmval) {"PWM_REV4", 0xC};
  pwms[13] = (pwmval) {"PWM_REV3", 0xD};
  pwms[14] = (pwmval) {"PWM_REV2", 0xE};
  pwms[15] = (pwmval) {"PWM_REV1", 0xF};
 
  //Serial.println("Testing PF.");
  //test_ir(PF_BLUE);
  

  // turn off builtin leds when setup is complete
  digitalWrite(LED_BUILTIN, HIGH);



}


void loop() {

  /*
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  delay(200); //Small delay
  */
  
  PFLightGlow(pf0, PF_BLUE, 50);
  
}




/*********** FUNCTIONS ***********/

/*
 * MQTT
 */
 
void callback(char* topic, byte* payload, unsigned int length) {

  Serial.print("MQTT received on topic ["+ (String)topic +"], ");
  
  // Payload must be in form  0/PF_RED/PWM_FWD4/
  // trailing slash is used as terminator
  payload2commands((char*)payload, cmd);

  // convert cmd[1]
  uint8_t pwmpin;
  if( cmd[1] == "PF_RED") pwmpin = 0x00;
  else if( cmd[1] == "PF_BLUE") pwmpin = 0x01;
  else {
    // TODO: publis error to topic
    Serial.println("!!! wrong port !!! use PF_RED or PF_BLUE to selct PF port ");
    pwmpin = 0xFF;
  }
  
  // convert cmd[2]
  uint8_t pwmval;
  for (int x = 0; x < PWM_SIZE; x++) {
    if ((String)pwms[x].k == cmd[2]) 
      {
        pwmval = pwms[x].v;
      }
  } 

  if (cmd[0] == "0") {
    Serial.println("command "+ (String)cmd[2] +" to channel 0 on port " + (String)cmd[1] );
    irsend0(pwmpin, pwmval, 500);
  } else if (cmd[0] == "1") {
    Serial.println("command "+ (String)cmd[2] +" to channel 1 on port " + (String)cmd[1] );
    irsend1(pwmpin, pwmval, 500);
  } else if (cmd[0] == "2") {
    Serial.println("command "+ (String)cmd[2] +" to channel 2 on port " + (String)cmd[1] );
    irsend2(pwmpin, pwmval, 500);
  } else if (cmd[0] == "3") {
    Serial.println("command "+ (String)cmd[2] +" to channel 3 on port " + (String)cmd[1] );
    irsend3(pwmpin, pwmval, 500);
  } else {
    Serial.println("!!! wrong channel !!! use 0, 1, 2 or 3");
    // TODO: publish error to topic
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      // Wait 2 seconds before retrying
      delay(2000);
    }
  }
}

/*  
 payload parsing:
 a "channel/port/command" (example: "0/RED/PWM_FWD4") converted to:

 commands[0] = channel
 commands[1] = port
 commands[2] = command

 cmd_zize must be defined
*/
void payload2commands(char* str, String outcmd[CMD_SIZE]) {

  char * pch;
  pch = strtok (str,"/");
  for(int i=0;i< CMD_SIZE;i++)
  {
    outcmd[i] = (String)pch;
    pch = strtok (NULL, "/");
  }
  
}


/*
 * WiFi
 */

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


/*
 * PowerFunction
 */
 
void irsend0(uint8_t output, uint8_t pwm,  uint16_t time) {
  pf0.single_pwm(output, pwm);
  delay(time);
}
void irsend1(uint8_t output, uint8_t pwm,  uint16_t time) {
  pf1.single_pwm(output, pwm);
  delay(time);
}
void irsend2(uint8_t output, uint8_t pwm,  uint16_t time) {
  pf2.single_pwm(output, pwm);
  delay(time);
}
void irsend3(uint8_t output, uint8_t pwm,  uint16_t time) {
  pf3.single_pwm(output, pwm);
  delay(time);
}

void test_ir(uint8_t port) {

  Serial.print("Test port: ");
  Serial.println(port);
  
  irsend0(port, PWM_FWD1, 200);
  irsend0(port, PWM_FWD2, 200);
  irsend0(port, PWM_FWD3, 200);
  irsend0(port, PWM_FWD4, 200);
  irsend0(port, PWM_FWD5, 200);
  irsend0(port, PWM_FWD6, 200);
  irsend0(port, PWM_FWD7, 200);
  irsend0(port, PWM_BRK, 100);
  irsend0(port, PWM_REV7, 200);
  irsend0(port, PWM_REV6, 200);
  irsend0(port, PWM_REV5, 200);
  irsend0(port, PWM_REV4, 200);
  irsend0(port, PWM_REV3, 200);
  irsend0(port, PWM_REV2, 200);
  irsend0(port, PWM_REV1, 200);
  irsend0(port, PWM_BRK, 500);

}

void PFLightGlow(PowerFunctions pf, uint8_t port, uint16_t time) {

 uint8_t i;

 for(i=0x1; i<0x8; i++) { 
   pf.single_pwm(port, i);
   delay(time);
 }
 for(i=0x7; i>0x0; i--) { 
   pf.single_pwm(port, i);
   delay(time);
 }

}


/*
 * RGB Led
 */

void test_led(RGBLed l, int t) {

  Serial.println("Test RGB Led");
  
  l.setColor(RGBLed::RED);
  delay(t);
  l.off();
  l.setColor(RGBLed::GREEN);
  delay(t);
  l.off();
  l.setColor(RGBLed::BLUE);
  delay(t);
  l.off();
}
