
void init_gsm();
void gprs_connect();
boolean gprs_disconnect();
boolean is_gprs_connected();
void post_to_firebase(String data);
boolean waitResponse(String expected_answer="OK", unsigned int timeout=2000);

#include <SoftwareSerial.h>
//GSM Module RX pin to Arduino 3
//GSM Module TX pin to Arduino 2
#define rxPin 2
#define txPin 3
SoftwareSerial SIM808(rxPin,txPin);


const String APN  = "safaricom";
const String USER = "saf";
const String PASS = "data";

const String FIREBASE_HOST  = "https://trial-dd23f-default-rtdb.firebaseio.com/";
const String FIREBASE_SECRET  = "1Fkd5dChtCEUk2P83R3D6j9W9WWEwoB77v3QdKoU";


#define USE_SSL true
#define DELAY_MS 500

void call() {
  SIM808.print(F("ATD"));
  SIM808.print("+254705735290");
  SIM808.print(F(";\r\n"));
}


void send_message(String data) {
  SIM808.println("AT+CMGF=1");
  delay(2000);
  SIM808.println("AT+CMGS=\"+254705735290\"");
  delay(2000);
  SIM808.println(data);
  delay(2000);
  SIM808.println((char)26);
  delay(2000);
  SIM808.println();
}


void setup() {

  //Begin serial communication with Serial Monitor
  Serial.begin(9600);

//Begin serial communication with SIM808
  SIM808.begin(9600);
  
  Serial.println("Initializing SIM808...");
  init_gsm();
  call();
}

void loop() {
  // Data to send to the server
  String Data = "{\"message\": \"Hello, World!\"}";

  Serial.println(Data);
  
  if(!is_gprs_connected()){
    gprs_connect();
  }
  send_message(Data);
  
  post_to_firebase(Data);
  
  delay(1000);
  call();
}

/*String get_temprature()
{
  topFn:
  //Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  String h = String(dht.readHumidity(), 2);
  String t = String(dht.readTemperature(), 2);

  // Check if any reads failed and exit early (to try again).
  if (h == "" || t == "") {
    Serial.println(F("Failed to read from DHT sensor!"));
    goto topFn;
  }
  */
 
  
//Function: post_to_firebase() start

void post_to_firebase(String data)
{
  
  //Start HTTP connection
  SIM808.println("AT+HTTPINIT");
  waitResponse();
  delay(DELAY_MS);
  //Enabling SSL 1.0
  if(USE_SSL == true){
    SIM808.println("AT+HTTPSSL=1");
    waitResponse();
    delay(DELAY_MS);
  }
  
  //Setting up parameters for HTTP session
  SIM808.println("AT+HTTPPARA=\"CID\",1");
  waitResponse();
  delay(DELAY_MS);
 
  //Set the HTTP URL - Firebase URL and FIREBASE SECRET
  SIM808.println("AT+HTTPPARA=\"URL\","+FIREBASE_HOST+".json?auth="+FIREBASE_SECRET);
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
  
  for (uint32_t start = millis(); millis() - start < 20000;){
    while(!SIM808.available());
    String response = SIM808.readString();
    if(response.indexOf("+HTTPACTION:") > 0)
    {
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
  waitResponse("OK",1000);
  delay(DELAY_MS);

}

// Initialize GSM Module

void init_gsm()
{
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

void gprs_connect()
{
 
  //DISABLE GPRS
  SIM808.println("AT+SAPBR=0,1");
  waitResponse("OK",60000);
  delay(DELAY_MS);
 
  //Connecting to GPRS: GPRS - bearer profile 1
  SIM808.println("AT+SAPBR=3,1,\"Contype\",\"GPRS\"");
  waitResponse();
  delay(DELAY_MS);
  
  //sets the APN settings for your sim card network provider.
  SIM808.println("AT+SAPBR=3,1,\"APN\","+APN);
  waitResponse();
  delay(DELAY_MS);
 
  //sets the user name settings for your sim card network provider.
  if(USER != ""){
    SIM808.println("AT+SAPBR=3,1,\"USER\","+USER);
    waitResponse();
    delay(DELAY_MS);
  }
 
  //sets the password settings for your sim card network provider.
  if(PASS != ""){
    SIM808.println("AT+SAPBR=3,1,\"PASS\","+PASS);
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
boolean gprs_disconnect()
{
  
  //Disconnect GPRS
  SIM808.println("AT+CGATT=0");
  waitResponse("OK",60000);
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
boolean is_gprs_connected()
{
  SIM808.println("AT+CGATT?");
  if(waitResponse("+CGATT: 1",6000) == 1) { return false; }

  return true;
}

//Handling AT COMMANDS

//boolean waitResponse(String expected_answer="OK", unsigned int timeout=2000) //uncomment if syntax error (arduino)
boolean waitResponse(String expected_answer, unsigned int timeout) //uncomment if syntax error (esp8266)
{
  uint8_t x=0, answer=0;
  String response;
  unsigned long previous;
    
  //Clean the input buffer
  while( SIM808.available() > 0) SIM808.read();
  
  previous = millis();
  do{
    //if data in UART INPUT BUFFER, reads it
    if(SIM808.available() != 0){
        char c = SIM808.read();
        response.concat(c);
        x++;
        //checks if the (response == expected_answer)
        if(response.indexOf(expected_answer) > 0){
            answer = 1;
        }
    }
  }while((answer == 0) && ((millis() - previous) < timeout));
 
  
  Serial.println(response);
  return answer;
}

