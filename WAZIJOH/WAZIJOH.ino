#include <WiFi.h>
#include <HTTPClient.h>
#include "DHT.h"
#define DHTPin 4
#define DHTTYPE DHT11

DHT dht(DHTPin, DHTTYPE);

char* ssid = "CDED";
char* password = "CDED2024.";

char* device_id = "Esp32";
char* sensor_id_1 = "TC";
char* sensor_id_2 = "TC_1";

void connectWifi() {
  WiFi.begin(ssid, password);  //Initiate the wifi connection here with the credentials earlier preset

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
  return;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);  //Set the baudrate to the board youre using (115200 is fine)
  delay(100);
  connectWifi();
  dht.begin();
}

void loop() {
  // put your main code here, to run repeatedly:

  //  float hum = dht.readHumidity();      //Read the moisture content in %.
  //  float temp = dht.readTemperature();  //Read the temperature in degrees Celsius
  float hum = random(10, 30);
  float temp = random(40, 70);

  Serial.print("Humidity: ");
  Serial.print(hum);
  Serial.print("%  Temperature: ");
  Serial.print(temp);
  Serial.println("°C, ");

  sendDataToWaziCloud(temp, sensor_id_1);
  delay(6000);
  sendDataToWaziCloud(hum, sensor_id_2);

  delay(300);  // Wait for 5 minutes (in ms) to send the next generated random value
}


void sendDataToWaziCloud(float value, char* sensor_id) {

  // We cancel the send process if our board is not yet connectedd to the internet, and try reconnecting to wifi again.
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected.");
    connectWifi();
    return;
  }

  HTTPClient http;

  // Initialize the API endpoint to send data. This endpoint is responsible for receiving the data we send
  String endpoint = "https://api.waziup.io/api/v2/devices/" + String(device_id) + "/sensors/" + String(sensor_id) + "/value";
  http.begin(endpoint);

  // Header content for the data to send
  http.addHeader("Content-Type", "application/json;charset=utf-8");
  http.addHeader("accept", "application/json;charset=utf-8");

  // Data to send
  String data = "{ \"value\": " + String(value) + " }";

  int httpResponseCode = http.POST(data);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("HTTP Response code: " + String(httpResponseCode));
    Serial.println("Response: " + response);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}