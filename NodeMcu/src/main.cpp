#include <Arduino.h>
#include <stdio.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <arduino_secrets.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>

#define HTTP_REST_PORT 301
#define WIFI_RETRY_DELAY 500
#define MAX_WIFI_INIT_RETRY 50
#define  SS_PIN D4
#define RST_PIN D3

const char* wifi_ssid = SECRET_SSID;
const char* wifi_passwd = SECRET_WIFI_PWD;
const char* api_url =  SECRET_API_URL;
const char* webserver_url  = SECRET_WEBSERVER_URL;
String auth;
String classroom;


bool is_attendance_open = false;

ESP8266WebServer http_rest_server(HTTP_REST_PORT);
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 mfrc522(SS_PIN, RST_PIN);
HTTPClient http;

typedef struct node {
    String uid;
    struct node * next;
} node_t;

node_t * head = NULL;

void init_already_read_cards(){

  head = (node_t *) malloc(sizeof(node_t));
  if (head == NULL) {

  }

  head->uid = 1;
  head->next = NULL;
}

void push(node_t * head, String val) {
    node_t * current = head;
    while (current->next != NULL) {
        current = current->next;
    }

    /* now we can add a new variable */
    current->next = (node_t *) malloc(sizeof(node_t));
    current->next->uid = val;
    current->next->next = NULL;
}

bool shouldMarkAttendance(String uid, node_t * head){
  node_t * current = head;

    while (current != NULL) {
        if(current->uid ==  uid){
          return false;
        }
        current = current->next;
    }
    return true;
}

// LCD
void init_lcd(){
  Serial.println("# LCD STARTING #");
  SPI.begin();
  lcd.begin(16,2);
  lcd.init();
  lcd.backlight();
  lcd.clear();
}

void write_lcd( String text, bool clear, int line){
if(clear){
  lcd.clear();
 }
 lcd.setCursor(0, line);
 lcd.print(text);
}
// LCD

// WEB SERVER
int init_wifi() {
    int retries = 0;

    Serial.println("Connecting to WiFi AP..........");
    write_lcd("Connecting to", true, 0);
    write_lcd("WiFi", false, 1);

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_passwd);
    // check the status of WiFi connection to be WL_CONNECTED
    while ((WiFi.status() != WL_CONNECTED) && (retries < MAX_WIFI_INIT_RETRY)) {
        retries++;
        delay(WIFI_RETRY_DELAY);
        Serial.print("#");
    }
    return WiFi.status(); // return the WiFi connection status
}

// WEB SERVER

//Login

void rest_state(){
  //Serial.println(String("id: ")+webserver_url+String("\nsenha: ")+ ESP.getChipId());
  write_lcd("Aguardando", true, 0);
  write_lcd("Chamada", false, 1);

}

//Login

// Mark attendance


String mark_attendance(String uid) {

    write_lcd("Realizando", true, 0);
    write_lcd("leitura", false, 1);

    WiFiClient wifiClient;

    const size_t CAPACITY = JSON_OBJECT_SIZE(20);
    StaticJsonDocument<CAPACITY> doc;
    JsonObject object = doc.to<JsonObject>();

    object["classroom"] = classroom;
    object["rfid"] = uid;

    if(!shouldMarkAttendance(uid, head)){
      Serial.println("Repetição de RA");
      write_lcd("Chamada ja", true, 0);
      write_lcd("Registrada ", false, 1);
      delay(1000);
      lcd.clear();

    }else{

      String body;
      serializeJson(doc, Serial);
      serializeJson(doc, body);
      http.begin(wifiClient, api_url+ String("/frequency"));
      http.addHeader("Content-Type", "application/json");
      http.addHeader("Authorization", auth);
      int httpCode = http.POST(body);

      // RESPONSE
      String payload = "";
      Serial.println("HTTP REQUEST");
      if (httpCode > 0) { //Check the returning code

        payload = http.getString();   //Get the request response payload
        //Serial.println(payload);
        StaticJsonDocument<1000> doc;
        deserializeJson(doc, payload);
        String ra = doc["ra"];
        Serial.println(ra);
        if(ra != "null"){
          push(head, uid);
          write_lcd("Sucesso", true, 0);
          write_lcd(ra, false, 1);
        }else{
          write_lcd("RA invalido", true, 0);
        }
        delay(1000);
        //Print the response payload

      http.end();
      return payload;
      }
    }


    //lcd.clear();
    return "";

}



// Mark attendance

// RFID READER
void readRfid(){
  write_lcd("Aproxime o", false, 0);
  write_lcd("cartao", false, 1);
  if(mfrc522.PICC_IsNewCardPresent()){

    if(mfrc522.PICC_ReadCardSerial()){

      if (WiFi.status() == WL_CONNECTED) {
        Serial.print("Tag UID: ");
        String uid = "";
          for(byte i=0; i< mfrc522.uid.size; i++){
            Serial.print(mfrc522.uid.uidByte[i]< 0x10 ? " 0": " ");
            Serial.print(mfrc522.uid.uidByte[i], HEX);
            uid = uid+mfrc522.uid.uidByte[i];
          }
          //write_lcd(uid, true, 0);
          Serial.println(uid);
          mark_attendance(uid);
          mfrc522.PICC_HaltA();

        }

            }

    }
}
//RFID READER


// INIT ATTENDANCE

void init_attendance() {
  init_already_read_cards();
  DynamicJsonDocument doc(2048);
  deserializeJson(doc, http_rest_server.arg("plain"));

  auth = String(doc["token"]);
  classroom = String(doc["classroom"]);
  //Serial.println(auth + "\n" + classroom);

  http_rest_server.send(200, "application/json", "Chamada Iniciada");
  write_lcd("Iniciando", true, 0);
  write_lcd("Chamada", false, 1);
  delay(4);
  Serial.println("INICIANDO CHAMADA");
  is_attendance_open= true;
  write_lcd("", true, 0);
  write_lcd("", true, 1);
  while(is_attendance_open){
    readRfid();
    http_rest_server.handleClient();
  }

}
// INIT ATTENDANCE

void get_status() {

  http_rest_server.send(200, "application/json", String(is_attendance_open));

}

void close_attendance() {

  http_rest_server.send(200, "application/json", "Chamada finalizada");
  Serial.println("Finalizando chamada");
  write_lcd("Finalizando", true, 0);
  write_lcd("Chamada", false, 1);
  delay(1000);
  is_attendance_open = false;
  memset(head, 0, sizeof(head));
  rest_state();
}


void config_rest_server_routing() {
    http_rest_server.on("/", HTTP_GET, []() {
        http_rest_server.send(200, "text/html",
            "Welcome to the ESP8266 REST Web Server");
    });
    http_rest_server.on("/init_attendance", HTTP_POST, init_attendance);
    http_rest_server.on("/close_attendance", HTTP_POST, close_attendance);
    http_rest_server.on("/get_status", HTTP_POST, get_status);
}

// WEB SERVER

void setup(void) {
    Serial.begin(9600);
    init_lcd();
    SPI.begin();
    lcd.begin(16,2);
    lcd.init();
    lcd.backlight();
    lcd.clear();
    mfrc522.PCD_Init();


    if (init_wifi() == WL_CONNECTED) {
        Serial.print("Connetted to: ");
        Serial.println(wifi_ssid);
        Serial.print("--- IP: ");
        Serial.println(WiFi.localIP());
    }
    else {
        Serial.print("Error connecting to: ");
        Serial.println(wifi_ssid);
    }

    config_rest_server_routing();

    http_rest_server.begin();
    Serial.println("HTTP REST Server Started");
    rest_state();
}

void loop(void) {
  http_rest_server.handleClient();
}
