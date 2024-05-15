//https://www.sparkfun.com/datasheets/Sensors/Imaging/1274419957.pdf
#include <PubSubClient.h>
#include <WiFi.h>

// WiFi
const char* ssid = "Valley Creek 3";
const char* wifi_password = "ValleyCreek";

// MQTT
const char* mqtt_server = "192.168.200.103";
const char* pic_topic = "pic_hex";
const char* mqtt_username = "blind_assist";    // MQTT username
const char* mqtt_password = "blind_assist";    // MQTT password
const char* clientID = "blind_assist_server";  // MQTT client ID

// Initialise the WiFi and MQTT Client objects
WiFiClient wifiClient;

// 1883 is the listener port for the Broker
PubSubClient client(mqtt_server, 1883, wifiClient);

// Custom function to connect to the MQTT broker via WiFi

void connect_MQTT();


byte incomingbyte;


int a = 0x0000,  //Read Starting address
  j = 0,
    k = 0,
    count = 0;
uint8_t MH, ML;
boolean EndFlag = 0;


void setup() {
  Serial.begin(19200);
  Serial2.begin(38400);
  delay(5000);
  connect_MQTT();

  SendResetCmd();
  delay(3000);
}

void loop() {
  if (client.connect(clientID, mqtt_username, mqtt_password)) {
    Serial.println("Connected to MQTT Broker !");
  } else {
    connect_MQTT();
  }

  SendTakePhotoCmd();


  Serial.println("Start pic");
  delay(100);

  while (Serial2.available() > 0) {
    incomingbyte = Serial2.read();
  }
  byte b[32];

  while (!EndFlag) {
    j = 0;
    k = 0;
    count = 0;
    SendReadDataCmd();

    delay(75);  //try going up
    while (Serial2.available() > 0) {
      incomingbyte = Serial2.read();
      k++;
      if ((k > 5) && (j < 32) && (!EndFlag)) {
        b[j] = incomingbyte;
        if ((b[j - 1] == 0xFF) && (b[j] == 0xD9))
          EndFlag = 1;
        j++;
        count++;
      }
    }

    for (j = 0; j < count; j++) {
      if (b[j] < 0x10)
        Serial.print("0");
      Serial.print(b[j], HEX);
      String dataToSend = String(b[j], HEX);
      // PUBLISH to the MQTT Broker (topic = Temperature)
      if (client.publish(pic_topic, dataToSend.c_str())) {

      } else {
        Serial.println("failed to send.Reconnecting to MQTT Broker and trying again");
        client.connect(clientID, mqtt_username, mqtt_password);
        delay(10);  // This delay ensures that client.publish doesn’t clash with the client.connect call
        client.publish(pic_topic, dataToSend.c_str());
      }
    }
    Serial.println();
  }

  delay(3000);
  StopTakePhotoCmd();  //stop this picture so another one can be taken
  EndFlag = 0;         //reset flag to allow another picture to be read
  Serial.println("End of pic");
  Serial.println();
  Serial.println("Data sent");
  client.disconnect();
  while (1)
    ;
}

//Send Reset command
void SendResetCmd() {
  Serial2.write((byte)0x56);
  Serial2.write((byte)0x00);
  Serial2.write((byte)0x26);
  Serial2.write((byte)0x00);
}

//Send take picture command
void SendTakePhotoCmd() {
  Serial2.write((byte)0x56);
  Serial2.write((byte)0x00);
  Serial2.write((byte)0x36);
  Serial2.write((byte)0x01);
  Serial2.write((byte)0x00);

  a = 0x0000;  //reset so that another picture can taken
}

void FrameSize() {
  Serial2.write((byte)0x56);
  Serial2.write((byte)0x00);
  Serial2.write((byte)0x34);
  Serial2.write((byte)0x01);
  Serial2.write((byte)0x00);
}

//Read data
void SendReadDataCmd() {
  MH = a / 0x100;
  ML = a % 0x100;

  Serial2.write((byte)0x56);
  Serial2.write((byte)0x00);
  Serial2.write((byte)0x32);
  Serial2.write((byte)0x0c);
  Serial2.write((byte)0x00);
  Serial2.write((byte)0x0a);
  Serial2.write((byte)0x00);
  Serial2.write((byte)0x00);
  Serial2.write((byte)MH);
  Serial2.write((byte)ML);
  Serial2.write((byte)0x00);
  Serial2.write((byte)0x00);
  Serial2.write((byte)0x00);
  Serial2.write((byte)0x20);
  Serial2.write((byte)0x00);
  Serial2.write((byte)0x0a);

  a += 0x20;
}

void StopTakePhotoCmd() {
  Serial2.write((byte)0x56);
  Serial2.write((byte)0x00);
  Serial2.write((byte)0x36);
  Serial2.write((byte)0x01);
  Serial2.write((byte)0x03);
}
void connect_MQTT() {
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Connect to the WiFi
  WiFi.begin(ssid, wifi_password);

  // Wait until the connection is confirmed
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Connect to MQTT Broker
  if (client.connect(clientID, mqtt_username, mqtt_password)) {
    Serial.println("Connected to MQTT Broker !");
  } else {
    Serial.println("Connection to MQTT Broker failed…");
  }
}
