#include <DHT.h>
#include <SoftwareSerial.h>
#include <DHT.h>
#include <DHT_U.h>

//The DHT22 sensor is connected to Arduino pin 5
#define DHTPIN 5
#define DHTTYPE DHT22

DHT_Unified dht(DHTPIN, DHTTYPE);

//GSM Module RX pin to Arduino 3
//GSM Module TX pin to Arduino 2
#define rxPin 2
#define txPin 3
SoftwareSerial SIM808(rxPin, txPin);

#define Contact_sensor_pin 7

void init_gsm();
void gprs_connect();
boolean gprs_disconnect();
boolean is_gprs_connected();
void post_to_firebase(String data);
boolean waitResponse(String expected_answer = "OK", unsigned int timeout = 2000);
String get_temperature();
String get_contact_sensor();
String Doorstate;

const String APN = "safaricom";
const String USER = "saf";
const String PASS = "data";

const String FIREBASE_HOST = "https://telekinesis-e029b-default-rtdb.firebaseio.com/";
const String FIREBASE_SECRET = "6xsNgqR6c6ETuwFZK9E63bC18KgjoL0WT2MbCsED";


#define USE_SSL true
#define DELAY_MS 500

void setup() {

  //Begin serial communication with Serial Monitor
  Serial.begin(9600);

  dht.begin();

  //Begin serial communication with SIM808
  SIM808.begin(9600);

  Serial.println("Initializing SIM808...");
  init_gsm();
}

void loop() {
  // Data to send to the server
  String Data = get_temprature();

  Serial.println(Data);

  if (!is_gprs_connected()) {
    gprs_connect();
  }

  post_to_firebase(Data);

  delay(1000);
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//Function: get_temprature() start
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
String get_temprature() {
  //Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  } else {
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  } else {
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
  }

  String h = String(event.relative_humidity, 2);
  dht.temperature().getEvent(&event);
  String t = String(event.temperature, 2);

  int ContactVoltage = analogRead(Contact_sensor_pin);
  if (ContactVoltage >= 3.0) {
    Doorstate = "Door Closed";
  } else {
    Doorstate = "Door Opened";
  }
  Serial.print("Temperature = ");
  Serial.print(t);
  Serial.println(" °C");
  Serial.print("Humidity = ");
  Serial.print(h);
  Serial.println(" %");


  String Data = "{";
  Data += "\"temprature\":\"" + t + "\",";

  Data += "\"humidity\":\"" + h + "\",";

  Data += "\"DoorState\":\"" + Doorstate + "\",";

  Data += "}";

  return Data;
}


//Function: post_to_firebase() start

void post_to_firebase(String data) {

  //Start HTTP connection
  SIM808.println("AT+HTTPINIT");
  waitResponse();
  delay(DELAY_MS);
  //Enabling SSL 1.0
  if (USE_SSL == true) {
    SIM808.println("AT+HTTPSSL=1");
    waitResponse();
    delay(DELAY_MS);
  }

  //Setting up parameters for HTTP session
  SIM808.println("AT+HTTPPARA=\"CID\",1");
  waitResponse();
  delay(DELAY_MS);

  //Set the HTTP URL - Firebase URL and FIREBASE SECRET
  String url = "AT+HTTPPARA=\"URL\"," + FIREBASE_HOST + ".json?auth=" + FIREBASE_SECRET;
  SIM808.println(url);
  waitResponse();
  delay(DELAY_MS);

  //Setting up re direct
  SIM808.println("AT+HTTPPARA=\"REDIR\",1");
  waitResponse();
  delay(DELAY_MS);

  //Setting up content type
  SIM808.println("AT+HTTPPARA=\"CONTENT\",\"application/json\"");
  waitResponse();
  delay(DELAY_MS);

  //Setting up Data Size
  //+HTTPACTION: 1,601,0 - error occurs if data length is not correct
  SIM808.println("AT+HTTPDATA=" + String(data.length()) + ",10000");
  waitResponse("DOWNLOAD");
  //delay(DELAY_MS);

  //Sending Data
  SIM808.println(data);
  waitResponse();
  delay(DELAY_MS);

  //Sending HTTP POST request
  SIM808.println("AT+HTTPACTION=1");

  for (uint32_t start = millis(); millis() - start < 20000;) {
    while (!SIM808.available())
      ;
    String response = SIM808.readString();
    if (response.indexOf("+HTTPACTION:") > 0) {
      Serial.println(response);
      break;
    }
  }

  delay(DELAY_MS);

  //+HTTPACTION: 1,603,0 (POST to Firebase failed)
  //+HTTPACTION: 0,200,0 (POST to Firebase successfull)
  //Read the response
  SIM808.println("AT+HTTPREAD");
  waitResponse("OK");
  delay(DELAY_MS);

  //Stop HTTP connection
  SIM808.println("AT+HTTPTERM");
  waitResponse("OK", 1000);
  delay(DELAY_MS);
}

// Initialize GSM Module

void init_gsm() {
  //Testing AT Command
  SIM808.println("AT");
  waitResponse();
  delay(DELAY_MS);
  //Checks if the SIM is ready
  SIM808.println("AT+CPIN?");
  waitResponse("+CPIN: READY");
  delay(DELAY_MS);

  //Turning ON full functionality
  SIM808.println("AT+CFUN=1");
  waitResponse();
  delay(DELAY_MS);

  //Turn ON verbose error codes
  SIM808.println("AT+CMEE=2");
  waitResponse();
  delay(DELAY_MS);

  //Enable battery checks
  SIM808.println("AT+CBATCHK=1");
  waitResponse();
  delay(DELAY_MS);

  //Register Network (+CREG: 0,1 or +CREG: 0,5 for valid network)
  //+CREG: 0,1 or +CREG: 0,5 for valid network connection
  SIM808.println("AT+CREG?");
  waitResponse("+CREG: 0,");
  delay(DELAY_MS);

  //setting SMS text mode
  SIM808.print("AT+CMGF=1\r");
  waitResponse("OK");
  delay(DELAY_MS);
}
//Connect to the internet

void gprs_connect() {

  //DISABLE GPRS
  SIM808.println("AT+SAPBR=0,1");
  waitResponse("OK", 60000);
  delay(DELAY_MS);

  //Connecting to GPRS: GPRS - bearer profile 1
  SIM808.println("AT+SAPBR=3,1,\"Contype\",\"GPRS\"");
  waitResponse();
  delay(DELAY_MS);

  //sets the APN settings for your sim card network provider.
  SIM808.println("AT+SAPBR=3,1,\"APN\"," + APN);
  waitResponse();
  delay(DELAY_MS);

  //sets the user name settings for your sim card network provider.
  if (USER != "") {
    SIM808.println("AT+SAPBR=3,1,\"USER\"," + USER);
    waitResponse();
    delay(DELAY_MS);
  }

  //sets the password settings for your sim card network provider.
  if (PASS != "") {
    SIM808.println("AT+SAPBR=3,1,\"PASS\"," + PASS);
    waitResponse();
    delay(DELAY_MS);
  }

  //after executing the following command. the LED light of
  //sim800l blinks very fast (twice a second)
  //enable the GPRS: enable bearer 1
  SIM808.println("AT+SAPBR=1,1");
  waitResponse("OK", 30000);
  delay(DELAY_MS);

  //Get IP Address - Query the GPRS bearer context status
  SIM808.println("AT+SAPBR=2,1");
  waitResponse("OK");
  delay(DELAY_MS);
}


/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
* Function: gprs_disconnect()
* AT+CGATT = 1 modem is attached to GPRS to a network. 
* AT+CGATT = 0 modem is not attached to GPRS to a network
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
boolean gprs_disconnect() {

  //Disconnect GPRS
  SIM808.println("AT+CGATT=0");
  waitResponse("OK", 60000);
  //delay(DELAY_MS);

  //DISABLE GPRS
  //SIM800.println("AT+SAPBR=0,1");
  //waitResponse("OK",60000);
  //delay(DELAY_MS);


  return true;
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
* Function: gprs_disconnect()
* checks if the gprs connected.
* AT+CGATT = 1 modem is attached to GPRS to a network. 
* AT+CGATT = 0 modem is not attached to GPRS to a network
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
boolean is_gprs_connected() {
  SIM808.println("AT+CGATT?");
  if (waitResponse("+CGATT: 1", 6000) == 1) { return false; }

  return true;
}

//Handling AT COMMANDS

//boolean waitResponse(String expected_answer="OK", unsigned int timeout=2000) //uncomment if syntax error (arduino)
boolean waitResponse(String expected_answer, unsigned int timeout)  //uncomment if syntax error (esp8266)
{
  uint8_t x = 0, answer = 0;
  String response;
  unsigned long previous;

  //Clean the input buffer
  while (SIM808.available() > 0) SIM808.read();

  previous = millis();
  do {
    //if data in UART INPUT BUFFER, reads it
    if (SIM808.available() != 0) {
      char c = SIM808.read();
      response.concat(c);
      x++;
      //checks if the (response == expected_answer)
      if (response.indexOf(expected_answer) > 0) {
        answer = 1;
      }
    }
  } while ((answer == 0) && ((millis() - previous) < timeout));


  Serial.println(response);
  return answer;
}
