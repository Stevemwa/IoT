
#include <TinyGPS++.h>

float gpslat, gpslong;
TinyGPSPlus gps;

String lat = "0.0000000";
String lon = "0.0000000";
String googleMapsLink = "https://www.google.com/maps?q=0.0000000,0.0000000";


TinyGPSCustom pdop(gps, "GNGLL", 1);  // $GPGSA sentence, 15th element
TinyGPSCustom hdop(gps, "GNGLL", 3);  // $GPGSA sentence, 16th element

#define LED 2

void send_message(String data) {
  Serial.println("AT+CMGF=1");
  delay(2000);
  Serial.println("AT+CMGS=\"+254705735290\"");
  delay(2000);
  Serial.println(data);
  delay(2000);
  Serial.println((char)26);
  delay(2000);
  Serial.println();
}

void call() {
  Serial.print(F("ATD"));
  Serial.print("+254705735290");
  Serial.print(F(";\r\n"));
}


String createGoogleMapsLink() {

  String googleMapsLink = "https://www.google.com/maps?q=" +lat+","+lon;

  return googleMapsLink;
}

void blink() {
  for (int i = 0; i < 5; i++) {
    delay(500);
    digitalWrite(LED, HIGH);
    delay(500);
    digitalWrite(LED, LOW);
  }
}



void setup() {
  Serial.begin(9600);
  Serial2.begin(9600);

  pinMode(LED, OUTPUT);
  blink();

  for (int i = 0; i < 7; i++) {
    delay(5000);
    
    delay(5000);
    
  }
}

void loop() {
  while (Serial2.available() > 0)
    gps.encode(Serial2.read());


  lat = String(atof(pdop.value())/100);
  lon = String(atof(hdop.value())/100);

  String googleMapsLink = createGoogleMapsLink();

  delay(6000);

  send_message(googleMapsLink);
  delay(6000);
  call();
}