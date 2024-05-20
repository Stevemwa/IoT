#include "Certificates.h"
//libraries used for MQTT protocol
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"

//TOPICS FOR MQTT  PROTOCOLS
#define AWS_IOT_PUBLISH_TOPIC "r4/PUB"

// Pin for built-in LED
#define builtInLedPin 2


// This are example values we used in testing

String timeStamp = "no data";
float tdsValue = 0;
int moistureValue = 0;
byte temperature = 0;
byte humidity = 0;
int pH = 0;


//MQTT Protocols
WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);


// #define AWS_IOT_ENDPOINT    "*******.amazonaws.com"  // Replace with your AWS IoT endpoint
// #define MQTT_PORT           1883              // MQTT port (default is 1883)
// #define CLIENT_ID           "arduino_uno_r4_wifi_thing"  // Replace with your MQTT client ID
// #define TOPIC               "sensors/temperature"       // Replace with your MQTT topic

// Adafruit_MQTT_Client mqtt(&client, AWS_IOT_ENDPOINT, MQTT_PORT, CLIENT_ID);


// Rewrote your MQTT_connect() ,connectWifi(); into the function below
void connectAWS() {
  //WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(builtInLedPin, HIGH);
    delay(500);
    digitalWrite(builtInLedPin, LOW);
    delay(500);
  }

    // Print a message to indicate a successful Wi-Fi connection
  Serial.println("\nConnected to Wi-Fi");

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.setServer(AWS_IOT_ENDPOINT, 8883);


  Serial.println("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    digitalWrite(builtInLedPin, HIGH);
    delay(3000);
    digitalWrite(builtInLedPin, LOW);
    delay(500);
    return;
  }

  for (int i = 0; i < 5; i++) {
    digitalWrite(builtInLedPin, HIGH);
    delay(500);
    digitalWrite(builtInLedPin, LOW);
    delay(500);
  }

  Serial.println("AWS IoT Connected!");
}




// Function to publish to topic stated
void publishMessage() {
  StaticJsonDocument<200> doc;
  doc["Timestamp"] = timeStamp;
  doc["Temperature"] = temperature;
  doc["Humidity"] = humidity;
  doc["TDS"] = tdsValue;
  doc["Moisture"] = moistureValue;
  doc["PH"] = pH;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);  // print to client
  Serial.println(jsonBuffer);

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}


void setup() {
  Serial.begin(9600); // Use the Serial Monitor for debugging
  Serial2.begin(9600); // Use Serial2 for communication with UNO R4 WIFI
}


void loop() {

  if (Serial2.available()) {
    // Read the incoming data line by line
    String data = Serial2.readStringUntil('\n');

    // Parse the data and store in variables
    if (data.startsWith("Timestamp: ")) {
      timeStamp = data.substring(11);  // 11 is the length of "Timestamp: "
    } else if (data.startsWith("TDS Value: ")) {
      tdsValue = data.substring(11).toFloat();  // 11 is the length of "TDS Value: "
    } else if (data.startsWith("Moisture Value: ")) {
      moistureValue = data.substring(16).toInt();  // 16 is the length of "Moisture Value: "
    } else if (data.startsWith("Temperature: ")) {
      temperature = data.substring(13).toInt();  // 13 is the length of "Temperature: "
    } else if (data.startsWith("Humidity: ")) {
      humidity = data.substring(10).toInt();  // 10 is the length of "Humidity: "
    } else if (data.startsWith("pHValue: ")) {
      pH = data.substring(10).toFloat();  // 10 is the length of "Humidity: "
    }
  }


  if (!client.connected()) {
    connectAWS();
  } else {
    client.loop();
    publishMessage();
  }


  delay(1000);
}