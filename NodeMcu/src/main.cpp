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

String login(){
  //Serial.println(String("id: ")+webserver_url+String("\nsenha: ")+ ESP.getChipId());

  return "";
}

//Login

// Mark attendance

String mark_attendance(String uid) {

    WiFiClient wifiClient;

    const size_t CAPACITY = JSON_OBJECT_SIZE(8);
    StaticJsonDocument<CAPACITY> doc;
    JsonObject object = doc.to<JsonObject>();

    object["classroom"] = classroom;
    object["rfid"] = uid;

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
      Serial.println(payload);             //Print the response payload

    http.end();

    }

    lcd.clear();

    return payload;
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
          write_lcd(uid, true, 0);
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
  DynamicJsonDocument doc(2048);
  deserializeJson(doc, http_rest_server.arg("plain"));

  auth = String(doc["token"]);
  classroom = String(doc["classroom"]);
  Serial.println(auth + "\n" + classroom);

  http_rest_server.send(200, "application/json", "Chamada Iniciada");
  write_lcd("Iniciando", true, 0);
  write_lcd("Chamada", false, 1);
  delay(4);
  Serial.println("INICIANDO CHAMADA");
  is_attendance_open= true;
  login();
  while(is_attendance_open){
    http_rest_server.handleClient();
    readRfid();
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
  is_attendance_open = false;
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
    write_lcd("Aguardando", true, 0);
    write_lcd("Chamada", false, 1);
}

void loop(void) {
  http_rest_server.handleClient();
}
