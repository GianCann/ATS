//20190216 Ver. 0.4
//Prima di caricare impostare il parametro Flash Size a : 4M (1M SPIFFS) altrimenti non viene 
//creato il File system
//La prima volta sarebbe meglio anche far cancellare completamente la memoria e non solo lo spazio sketch

//Aggiunta la gestione mediante FS per i parametri:
//  - Indirizzo IP server MQTT
//  - Posta server MQTT
//  - Tipologia dispositivo/Identificativo non è il nome con qui si connette al server MQTT
//Ho notato che se non vengono impostati correttamente i parametri per il WiFi e di conseguenza il nodo 
//non si collega,i campi custom non vengono salvati

//test GitHub

//20190215
//Modificato host name del dispositivo, ora è identico al device MQTT
//20190211
//1) Ora il nodo all avvio manda le info su se stesso mediante una stringa json
//2) Aggiunto topic per chiedere info del nodo,come quelle inviate all'accensione
//3) Introdotto lampeggio ogni .2 sec durante l'attesa della connessione al server MQTT

//Da fare : Introdurre la gestione da WiFiManager dei parametri variabili:
//Indirizzo Ip del server MQTT
//Porta MQTT
//Tipo Nodo

#include <FS.h>   // Include the SPIFFS library
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ESP8266HTTPUpdateServer.h>
 
//Per lampeggio led durante le connessioni
#include "Ticker.h"
Ticker ticker;

// ChA = PWM e ChB = 0: motore forward
// ChA = 0 e ChB = PWM: motore backward
#define ChA 14   //Canale A motore 
#define ChB 13   //Canale B motore
#define LedOnBoard 2 //led sul chip

#define FORWARD 1 //direzione forward
#define BACKWARD 2 //direzione backaward
#define STOP 0 //stop del motore

String VersioneFW = "0.5 beta";

String Valore = "";
String Canale = "";
String Velocita = "0";
String Direzione = "";
String IndirizzoIP = "";

//Questi parametri devono andare in settaggio WiFiManager
String TipoNodo ="MotorControl";
//Parametri del server broker
char* mqtt_server = "192.168.100.1";
//const int mqtt_port = 1883;
char* mqtt_user = "MQTT-User";
char* mqtt_password = "MQTT-Password";

char mqtt_port_s[6] = "1883";
char ATS_NodeType[34] = "Motor Control";
int mqtt_port = atoi(mqtt_port_s);

const int HTTP_PORT = 3500;

//Creo il nome del dispositivo partendo dall'id del chip ESP8266
String id = "ATS-"+String(ESP.getChipId());
const char* MQTTClientName= id.c_str();

// Set web server port number to 3500
ESP8266WebServer server(HTTP_PORT);
// Variable to store the HTTP request

String header; //da cancellare

ESP8266HTTPUpdateServer httpUpdater;

WiFiClient espClient;
PubSubClient client(espClient);

//Preparo i topic 
String MainTopic = "ATS/" + id;
String InTopic01 = MainTopic + "/MotorForward";
String InTopic02 = MainTopic + "/MotorBackward";
String InTopic06 = MainTopic + "/MotorStop";
String InTopic03 = MainTopic + "/Reset";
String InTopic04 = MainTopic + "/SoftReset";
String InTopic05 = MainTopic + "/Info";

//Da Esp a MQTT
String OutTopic01 = MainTopic + "/Response"; 

//variabilie globali in cui vengono memorizzati
//eventuali parametri memorizzati nella EEPROM
String Data1;  //max 20 byte
String Data2;  //max 20 byte
String Data3;  //non usata al momento
String Data4;  //non usata al momento

//parametri per gestire il lampeggio
//del LED durante il funzionamento a regime
//vedi funzione 'flashled'
int iLedTimer;
int iLedState;

//Parametri per il tentativo di connessione
//al broker MQTT. Il valore maxRetryAttemptMQTT è da parametrizzare?
int maxRetryAttemptMQTT=3;
int tooManyAttempt = 0;

//Creo l'oggetto WiFiManager
WiFiManager wifiManager;

//flag per salvataggio dati
bool shouldSaveConfig = false;

File fsUploadFile;              // a File object to temporarily store the received file

String getContentType(String filename); // convert the file extension to the MIME type
bool handleFileRead(String path);       // send the right file to the client (if it exists)
void handleFileUpload();                // upload a new file to the SPIFFS

//---------------------------------------------------
// Procedura di 'boot' del dispositivo
//---------------------------------------------------
void setup() {
  //Imposto la seriale
  Serial.begin(9600);
  delay(200);
  
  Serial.println("Startup");
  Serial.print("Versione: ");
  Serial.println(VersioneFW);

  readsettings();

  //Imposto hostname
  WiFi.hostname(MQTTClientName);
  
  readJsonFile();

  
  //Imposto i canali di uscita
  pinMode(ChA, OUTPUT);     // Imposta i pin come uscite
  pinMode(ChB, OUTPUT);
  pinMode(LedOnBoard, OUTPUT);
  digitalWrite(ChA, LOW);
  digitalWrite(ChB, LOW);
  
  // Faccio lampeggiare il led ogni .5 sec in attesa di connettersi alla
  // rete wifi
  ticker.attach(0.6, tick);

// The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port_s, 6);
  WiFiManagerParameter custom_ATS_NodeType("ATS Node Type", "Tipo Nodo ATS", ATS_NodeType, 32);

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_ATS_NodeType);

 
  //Se non riesco a collegarmi alla rete WiFi mi imposto come acces point
  wifiManager.setAPCallback(configModeCallback);

  if (!wifiManager.autoConnect(MQTTClientName,"Password")) {
    Serial.println("Connessione Fallita e time-out");
    //Resetto il tutto e ci riprovo
    ESP.reset();
    delay(1000);
  }
  
  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port_s, custom_mqtt_port.getValue());
  strcpy(ATS_NodeType, custom_ATS_NodeType.getValue());
  
    //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("Salvataggio configurazione");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port_s;
    json["ATS_NodeType"] = ATS_NodeType;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("Fallita apertura file configurazione per il salvataggio");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    
    //Converto da stringa a int
    mqtt_port= atoi(mqtt_port_s);
    //end save
    }

  server.on("/", HTTP_GET, []() {                 // if the client requests the upload page
    if (!handleFileRead("/welcome.html"))                // send it if it exists
      server.send(202, "text/plain", "ATS - Firmware: " + VersioneFW); // otherwise, respond with a 404 (Not Found) error
  });

   server.on("/upload", HTTP_GET, []() {                 // if the client requests the upload page
    if (!handleFileRead("/upload.html"))                // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });

  server.on("/test", HTTP_GET, []() {                 // if the client requests the upload page
    if (!handleFileRead("/ReadMe.md"))                // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });

  server.on("/upload", HTTP_POST,                       // if the client posts to the upload page
    [](){ server.send(200); },                          // Send status 200 (OK) to tell the client we are ready to receive
    handleFileUpload                                    // Receive and save the file
  );
  
   server.on("/chipinfo", HTTP_GET, []() {  // funzione http per le informazioni
      chipinfo();
   });

   server.on("/info", HTTP_GET, []() {  // funzione http per le informazioni
      httphandleInfo();
   });
   
   server.on("/motorforward", HTTP_GET, []() {  // funzione http per il motore
      httphandleMotor(FORWARD);
   });
   server.on("/motorbackward", HTTP_GET, []() {  // funzione http per il motore
      httphandleMotor(BACKWARD);
   });
    server.on("/motorstop", HTTP_GET, []() {  // funzione http per il motore
      httphandleMotor(STOP);
   });
   server.onNotFound(httphandleNotFound); //pagina non trovata


   httpUpdater.setup(&server,"/firmware", "ats","aggiornamento");
   //avvio il web server per le rest-api
   server.begin();
 
  Serial.println("Connesso... :)");
  ticker.detach();
  
  Serial.println("");
  Serial.println("WiFi Connesso");
  Serial.print("Indirizzo IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Nome dispositivo: ");
  Serial.println (MQTTClientName);
  Serial.print ("Server ATS-MQTT:");
  Serial.println(mqtt_server);
    
  client.setServer(mqtt_server, mqtt_port); //Mi collego al Server Broker MQTT
  client.setCallback(callback); //Attivo la ricezione dei topic
  reconnect();

  if (tooManyAttempt==0){
    Serial.println("Topic Sottoscritti: ");
    Serial.println (InTopic01);
    Serial.println (InTopic02);
    Serial.println (InTopic03);
    Serial.println (InTopic04);
    Serial.println (InTopic05);
    Serial.println (InTopic06);
  }else{
    Serial.println("Nessun topic MQTT sottoscritto");
  }

}

//---------------------------------------------------
// Procedura a regime
//---------------------------------------------------
void loop() {

   //gestione led durante l'operatività normale
   flashled();

  //La riconnessione al broker MQTT viene
  //disabilitata dopo 3 tentativi consecutivi falliti
  if (tooManyAttempt == 0){
    if (!client.connected()) {
      reconnect();
    }
  }

  delay(50);
  client.loop();

  //gestisco eventuali connessioni http
  server.handleClient();
}

//---------------------------------------------------
//---------------------------------------------------
// gestione HTTP
//---------------------------------------------------
//---------------------------------------------------

//---------------------------------------------------
// info http json
//---------------------------------------------------
void httphandleInfo(){

  IndirizzoIP = "";
  IndirizzoIP = WiFi.localIP().toString();
  char* cPayload = &IndirizzoIP[0u];
  StaticJsonBuffer<400> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject(); 
  JSONencoder["DevId"] = id;
  JSONencoder["Type"] = ATS_NodeType; //TipoNodo.c_str();
  JSONencoder["Speed"] = Velocita.c_str();
  JSONencoder["Dir"] = Direzione.c_str();
  JSONencoder["Ip"] = IndirizzoIP.c_str();
  
  char JSONmessageBuffer[600];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

  server.send(200,"text/plain",JSONmessageBuffer);

}

//---------------------------------------------------
// Pagina non trovata
//---------------------------------------------------
void httphandleNotFound(){

   if (!handleFileRead(server.uri())){  // cerca la pagina sullo SPIFFS, e se esiste la invia al client
       server.send(404, "text/plain", "404: Spiacente, la pagina o il comando richiesto non sono disponibili");
   }
}

void httphandleMotor(int dir){

  String message = "";
//  message += server.args();
//  for (int i = 0; i < server.args(); i++) {
//    message += "Arg n." + (String)i + ": ";
//    message += server.argName(i) + ": ";
//    message += server.arg(i) + "\n";
//  } 

  int mspeed=0;
  if (server.args() > 0){
    mspeed=server.arg(0).toInt();
  }
  
  if (dir > 0){
     Serial.println("Motore ok");
    motordrive(dir,mspeed);
    message="Motor speed:" + server.arg(0);
   
  } else {
    Serial.println("Motore stop");
    motordrive(STOP,0);
    message="Motor Stop";
    
  }
  
  server.send(200, "text/plain", message); // 
}



//---------------------------------------------------
//callback notifying us of the need to save config
//---------------------------------------------------
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

//---------------------------------------------------
//gets called when WiFiManager enters configuration mode
//---------------------------------------------------
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  ticker.attach(0.2, tick);
}

//---------------------------------------------------
//Gestione Messaggio MQTT in arrivo
//---------------------------------------------------
void callback(char* topic, byte* payload, unsigned int length) {
  Valore = "";
  Canale = "";
  Canale =String(topic);
  Serial.print("Msq MQTT Ricevuto :[");
  Serial.print(Canale);
  Serial.print("] ");
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    Valore +=(char)payload[i];
  }

  Serial.println();
  
  if (Canale == InTopic01) {  //Forward
    Serial.println("MotorForward");
    motordrive(FORWARD,Valore.toInt());
    //analogWrite(ChA, Valore.toInt());
    Velocita = Valore;
    Direzione="Forward"; 
  }

  if (Canale == InTopic02) {  //Backward
    Serial.println("MotorBackward");
    motordrive(BACKWARD,Valore.toInt());
    Velocita = Valore;
    Direzione="Backward"; 
  }

  if (Canale == InTopic06) {  //Stop
    Serial.println("MotorStop");
    motordrive(STOP,0);
    Velocita = "0";
    Direzione="Stop"; 
  }

  if (Canale == InTopic03) {  //Reset Impostazioni WiFi 
    Serial.println("Reset");
    wifiManager.resetSettings();
    ESP.reset();
  }

  if (Canale == InTopic04) {  //Riavvio il nodo 
    Serial.println("Soft Reset");
    delay(1000);
    ESP.reset();
    delay(1000);
  }

  if (Canale == InTopic05) {  //Info
    Serial.println("Invia Info");
    SendInfoMQTT();  
  }
}

//---------------------------------------------------
// Spedisce il messaggio MQTT con le informazioni
// del dispositivo
//---------------------------------------------------
void SendInfoMQTT() {
  IndirizzoIP = "";
  IndirizzoIP = WiFi.localIP().toString();
  char* cPayload = &IndirizzoIP[0u];
  StaticJsonBuffer<400> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject(); 
  JSONencoder["DevId"] = id;
  JSONencoder["Type"] = ATS_NodeType; //TipoNodo.c_str();
  JSONencoder["Speed"] = Velocita.c_str();
  JSONencoder["Dir"] = Direzione.c_str();
  JSONencoder["Ip"] = IndirizzoIP.c_str();
  
  char JSONmessageBuffer[600];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

  if (client.publish(OutTopic01.c_str(), JSONmessageBuffer,false) == true) {
    Serial.println("Msg Inviato con successo");
  } else {
    Serial.println("Errore invio msg");
    Serial.println (sizeof(JSONmessageBuffer));
  }
  Serial.println(JSONmessageBuffer);
}

//---------------------------------------------------
// Procedura per la connessione al broker MQTT
//---------------------------------------------------
void reconnect() {

  //variabile globale per segnalare i troppi
  //tentantivi falliti di connessione al broker MQTT
  tooManyAttempt=0;

  //variabile locale per contatare i tentativi di connessione
  int retryAttempt=0;

  //durante il tentativo di connessione al broker
  //il led lampeggia velocemente
  ticker.attach(0.03, tick);
  
  while (!client.connected()) {
    Serial.println("-In attesa di connettersi al server ATS-MQTT...");
    // Attempt to connect
    if (client.connect(MQTTClientName, mqtt_user, mqtt_password)) {
      Serial.println("Connesso");
      client.subscribe(InTopic01.c_str());   //Sottoscrivo il topic per la ricezione dei comandi per il canale A
      client.subscribe(InTopic02.c_str());   //Sottoscrivo il topic per la ricezione dei comandi per il canale B
      client.subscribe(InTopic03.c_str());   //Sottoscrivo il topic per il reset dei parametri WiFi
      client.subscribe(InTopic04.c_str());   //Sottoscrivo il topic per il soft reset del nodo
      client.subscribe(InTopic05.c_str());   //Sottoscrivo il topic per la richiesta di info del nodo
      client.subscribe(InTopic06.c_str());   //Sottoscrivo il topic per stop motore
      tooManyAttempt=0;
      SendInfoMQTT();
    } else {

      //dopo 3 tentativi -vedi maxRetryAttemptMQTT- che non si connette al broker MQTT
      //esco...
      retryAttempt++;
      if (retryAttempt > maxRetryAttemptMQTT){
        tooManyAttempt=1;
        Serial.println("Broker MQTT non raggiungibile... verifica impostazioni");
        break;
      }
      Serial.print("Fallito, rc=");
      Serial.print(client.state());
      Serial.println(" riprovo tra 5 secondi");
      // Aspetto 5 sec prima di riprovare
      delay(5000);
    }
  }

  //disattivo il lampeggio del led
  ticker.detach();
  
  if (tooManyAttempt==1){
    Serial.println("Modalità senza broker MQTT");
  } else {
    Serial.println("Modalità mista, con broker MQTT");  
  }
}

//---------------------------------------------------
// Procedura per il pilotaggio vero e proprio del
// motore
//---------------------------------------------------
void motordrive(int mdir, int mspeed){
  if (mdir==FORWARD){
    analogWrite(ChA, 0);
    analogWrite(ChB, mspeed);
    Serial.print("Motor forward: ");
    Serial.println(mspeed);
  } else if (mdir==BACKWARD)  {
    analogWrite(ChB, 0);
    analogWrite(ChA, mspeed);
    Serial.print("Motor backward:");
    Serial.println(mspeed);
  } else {
    analogWrite(ChA, 0);
    analogWrite(ChB, 0);  
    Serial.println("Motor Stop!");
  }
}

//---------------------------------------------------
//legge alcune impostazioni salvate nella EEPROM
//---------------------------------------------------
void readsettings(){

  //TODO: gestione della prima lettura/inizializzaione
  //della EEPROM. Andrebbe aggiungo un header di 3 byte
  //con i caratteri A-T-S: se non presenti, la EEPROM deve
  //essere 'inizializzata' per non avere dati sporchi

  Serial.println("Leggo i dati dalla EEPROM");

  //inizializzo solo 40 byte, in due slot da 20
  EEPROM.begin(40);
  // leggo i primi 20 byte, fermandomi al primo byte '0'
  for (int i = 0; i < 20; ++i)
    {
      //Serial.print(EEPROM.read(i));
      //Serial.print(" - ");
      //Serial.println(char(EEPROM.read(i)));
      if (EEPROM.read(i) !=0){
        Data1 += char(EEPROM.read(i));
      }
    }
  Serial.print("Data 1: ");
  Serial.println(Data1);
  
  //leggo il secondo sloto da 20 byte, fermandomi al primo byte '0'
  for (int i = 20; i < 40; ++i)
    {
      if (EEPROM.read(i) !=0){
        Data2 += char(EEPROM.read(i));
      }
    }
  Serial.print("Data 2: ");
  Serial.println(Data2);
}

//---------------------------------------------------
//Funzionamento LED durante l'esecuzione del programma
//Flash singolo, ogni x secondi: attivo solo in HTTP
//Multiflash, ogni x secondi: attivo in HTTP e MQTT
//---------------------------------------------------
void flashled(){
 
   iLedTimer++;

   //modalità HTTP
   if (tooManyAttempt==1){
       iLedState=digitalRead(LedOnBoard);
       if (iLedTimer==5 and iLedState==LOW){
          digitalWrite(LedOnBoard, !iLedState);
          iLedTimer=0;
          }
       if (iLedTimer==70 and iLedState==HIGH){
          digitalWrite(LedOnBoard, !iLedState);
          iLedTimer=0;
          }

   ///modalità con broker MQTT + HTTP
   } else {
    
       if (iLedTimer==150 and iLedState==0){
          iLedState=1;
          ticker.attach(0.02,tick);
          iLedTimer=0;
          }
       if (iLedTimer==30 and iLedState==1){
          iLedState=0;
          ticker.detach();
          digitalWrite(LedOnBoard, HIGH);
          iLedTimer=0;
          }
   }
}

//---------------------------------------------------
//funzione di callbak per libreria ticker
//---------------------------------------------------
void tick() {
  //toggle state
  int state = digitalRead(LedOnBoard);
  digitalWrite(LedOnBoard, !state);
}

//---------------------------------------------------
// procedura per leggere dal file system
//---------------------------------------------------

void readJsonFile(){
  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("file system montato");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("lettura file configurazione");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("apertura file configurazione");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port_s, json["mqtt_port"]);
          strcpy(ATS_NodeType, json["ATS_NodeType"]);

        } else {
          Serial.println("Caricamento json config fallito");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}


void handleFileUpload(){ // upload a new file to the SPIFFS
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile) {                                    // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location","/uploadok.html");      // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

void chipinfo(){

  String message = "";
  uint32_t realSize = ESP.getFlashChipRealSize();
  uint32_t ideSize = ESP.getFlashChipSize();
  FlashMode_t ideMode = ESP.getFlashChipMode();

  Serial.printf("Flash real id:   %08X\n", ESP.getFlashChipId());
  Serial.printf("Flash real size: %u bytes\n\n", realSize);

  Serial.printf("Flash ide  size: %u bytes\n", ideSize);
  Serial.printf("Flash ide speed: %u Hz\n", ESP.getFlashChipSpeed());
  Serial.printf("Flash ide mode:  %s\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));

  if (ideSize != realSize) {
    Serial.println("Flash Chip configuration wrong!\n");
  } else {
    Serial.println("Flash Chip configuration ok.\n");
  }

  server.send(200, "text/plain", "ok");

}

//---------------------------------------------------
// Procedura per la gestione della chiamate HTTP
// sia per la gestione del motore, sia per gestire
// parametri o altre funzionalità del dispositivo
//---------------------------------------------------
//void WebClientCheck(){

//  String contenuto="";
//  String metarefresh="";
//  int speedPar;
//  int a=0;
//  int b=0;
//
//  //testo della pagina di default, con la spiegazione dei comandi
//  //il webserver è in ascolto sulla porta 3500 http://indirizzoip:3500
//  String body="- /motorforward/(value)</br>- /motorbackward/(value)</br>- /motorstop</br>- /settings";
//  WiFiClient client = server.available();   // Listen for incoming clients
//
//  if (client) {                             // If a new client connects,
//    Serial.println("New Client.");          // print a message out in the serial port
//    String currentLine = "";                // make a String to hold incoming data from the client
//    while (client.connected()) {            // loop while the client's connected
//      if (client.available()) {             // if there's bytes to read from the client,
//        char c = client.read();             // read a byte, then
//        Serial.write(c);                    // print it out the serial monitor
//        header += c;
//        if (c == '\n') {                    // if the byte is a newline character
//          // if the current line is blank, you got two newline characters in a row.
//          // that's the end of the client HTTP request, so send a response:
//          if (currentLine.length() == 0) {
//            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
//            // and a content-type so the client knows what's coming, then a blank line:
//            client.println("HTTP/1.1 200 OK");
//            client.println("Content-type:text/html");
//            client.println("Connection: close");
//            client.println();
//            
//            // In base al tipo di richiesta specifica nell'url
//            // eseguo il comando specificato
//            if (header.indexOf("GET /motorforward/") >= 0) {
//              //recupera il valore della velocità ed azione il motore...
//              a=header.indexOf("motorforward/");  //questo codice e' un po' delicato: dà per scontato che ci sia un int dopo lo "/"
//              contenuto=header.substring(a+13); //13 è la lunghezza della stringa 'motorforward/'
//              Serial.println(contenuto);
//              motordrive(FORWARD,contenuto.toInt());
//              body="Ok, motor forward";
//            } else if (header.indexOf("GET /motorbackward/") >= 0) {
//              //recupera il valore della velocità ed azione il motore...
//              a=header.indexOf("motorbackward/");  //questo codice e' un po' delicato: dà per scontato che ci sia un int
//              contenuto=header.substring(a+14); //14 è la lunghezza della stringa 'motorforward/'
//              motordrive(BACKWARD,contenuto.toInt());
//              body="Ok, motor backward";
//            } else if (header.indexOf("GET /motorstop") >= 0) {
//              //ferma il motore
//              motordrive(STOP,0);
//              body="Ok, motor stop";
//            } else if (header.indexOf("GET /settings") >= 0) {
//              Serial.println("Modifica impostazioni");
//              //gestisci impostazioni
//              body="<form method='get' action='savedata'><label>Setting 1: </label><input name='data1' length=20 value='";
//              body += Data1;
//              body += "'></br><label>Setting 2: </label><input name='data2' length=24 value='";
//              body += Data2;
//              body += "'><br/><input value='Salva impostazioni' type='submit'></form>";
//
//             } else if (header.indexOf("GET /newfw") >= 0) {
//              Serial.println("Modifica impostazioni");
//              //gestisci impostazioni
//              body="<form method='post' enctype='multipart/form-data'>";
//              body += "<label>Seleziona nuovo Fw </label>";
//              body += "<input type='file' name='name'>";
//              body += "<input class='button' type='submit' value='Upload fw'>";
//              body += "</form>";
//              
//            } else if (header.indexOf("GET /savedata?") >= 0) {
//                Serial.println("Salvataggio dati...");
//                //imposto un ritorno automatico alla pagina dei settings, dopo averli salvati
//                metarefresh="<meta http-equiv='refresh' content='2;url=/settings' />";
//                body="Dati correttamente salvati: attendere...";
//
//                //estraggo i valori dei parametri dall'url
//                //i parametri sono in slot dal 20 caratteri
//                //prima del salvataggio, azzero il contenuto precedente
//                //con un byte 0
//
//                //estrazione Data1
//                a=header.indexOf("=");
//                b=header.indexOf("&",a+1);
//                contenuto=header.substring(a+1,b);
//                Serial.print("Salvo il Dato 1: ");
//                Serial.println(contenuto);      
//          
//                //memorizzo il primo dato nella variabile globale
//                //e poi lo salvo sulla eeprom, azzerano prima il contenuto con un byte 0
//                Data1=contenuto;
//                for (int i = 0; i < 20; ++i) { EEPROM.write(i, 0); }
//                for (int i = 0; i < contenuto.length(); ++i) { EEPROM.write(i, contenuto[i]); }
//                
//                //estrazione Data2
//                a=header.indexOf("=",a+1);
//                b=header.indexOf(" ",a+1);
//                contenuto = header.substring(a+1,b);
//                Serial.print("Salvo il Dato 2: ");
//                Serial.println(contenuto);    
//
//                //memorizzo il secondo dato nella variabile globale
//                //e poi lo salvo sulla eeprom, azzerano prima il contenuto con un byte 0
//                Data2=contenuto;
//                for (int i = 20; i < 40; ++i) { EEPROM.write(i, 0); }
//                for (int i = 0; i < contenuto.length(); ++i){ EEPROM.write(20+i, contenuto[i]); }
//
//                 //finalizzo la scrittura sulla eeprom
//                 EEPROM.commit();
//              
//            }
//            
//            // Display the HTML web page
//            client.println("<!DOCTYPE html><html>");
//            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
//            client.println(metarefresh);
//            client.println("<link rel=\"icon\" href=\"data:,\">");
//            // CSS to style the on/off buttons 
//            // Feel free to change the background-color and font-size attributes to fit your preferences
//            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
//            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
//            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
//            client.println(".button2 {background-color: #77878A;}</style></head>");
//            
//            // Web Page Heading
//            client.println("<body><h1>ATS - Web Interface</h1>");
//            client.println(body);
//            client.println("</body></html>");
//
//            // The HTTP response ends with another blank line
//            client.println();
//            // Break out of the while loop
//            break;
//          } else { // if you got a newline, then clear currentLine
//            currentLine = "";
//          }
//        } else if (c != '\r') {  // if you got anything else but a carriage return character,
//          currentLine += c;      // add it to the end of the currentLine
//        }
//      }
//    }
//    // Clear the header variable
//    header = "";
//    // Close the connection
//    client.stop();
//    Serial.println("Client disconnected.");
//    Serial.println("");
//  }
//}
