#include <ESP8266WiFi.h>
#include <arduino_secrets.h>

class wifiHelper
{

  public:void setup(){
    const char* ssid = SECRET_SSID;
    const char* password = SECRET_WIFI_PWD;
    const char* host = SECRET_HOST;

    WiFiServer server(301);


    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
   Serial.println("");
   Serial.println("WiFi connected");

   // Start the server
   server.begin();
   Serial.println("Server started");

   // Print the IP address
   Serial.println(WiFi.localIP());
}

};
