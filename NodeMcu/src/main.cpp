#include <Arduino.h>
#include <wifiHelper.cpp>


wifiHelper wifi_helper;
void setup() {
  Serial.begin(9600);
  wifi_helper.setup();
}

void loop() {

}
