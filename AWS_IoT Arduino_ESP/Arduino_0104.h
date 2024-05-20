#include <DHT11.h>
#include <SimpleDHT.h>
#include <WiFiS3.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "RTC.h"
#include "Arduino_LED_Matrix.h"
ArduinoLEDMatrix matrix;

#define TDS_SENSOR_PIN A1
#define MOISTURE_PIN A0
#define DHT_SENSOR_PIN A2
int pinDHT11 = 2;
SimpleDHT11 dht11;

#define VREF 5.0
#define SCOUNT 30

int relay_1 = 10; // LUCI
int relay_2 = 11; // POMPA, ora sono collegate le ventole
int relay_3 = 12; // Deumidificatore
int relay_4 = 13; //

int LightStatus = 0;
int FanStatus = 0;
int PumpStatus = 0;

//RTC PART
byte Time[8][12] = {
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

byte Digits[5][30]{
  { 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  { 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1 },
  { 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1 },
  { 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1 },
  { 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1 },
};


int currentSecond;
int currentHour;
int currentMinute; 

boolean secondsON_OFF = 1;
int hours, minutes, seconds, year, dayofMon;
String dayofWeek, month;

void displayDigit(int d, int s_x, int s_y) {
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 5; j++)
      Time[i + s_x][11 - j - s_y] = Digits[j][i + d * 3];

  matrix.renderBitmap(Time, 8, 12);
}


DayOfWeek convertDOW(String dow) {
  if (dow == String("Mon")) return DayOfWeek::MONDAY;
  if (dow == String("Tue")) return DayOfWeek::TUESDAY;
  if (dow == String("Wed")) return DayOfWeek::WEDNESDAY;
  if (dow == String("Thu")) return DayOfWeek::THURSDAY;
  if (dow == String("Fri")) return DayOfWeek::FRIDAY;
  if (dow == String("Sat")) return DayOfWeek::SATURDAY;
  if (dow == String("Sun")) return DayOfWeek::SUNDAY;
}

Month convertMonth(String m) {
  if (m == String("Jan")) return Month::JANUARY;
  if (m == String("Feb")) return Month::FEBRUARY;
  if (m == String("Mar")) return Month::MARCH;
  if (m == String("Apr")) return Month::APRIL;
  if (m == String("May")) return Month::MAY;
  if (m == String("Jun")) return Month::JUNE;
  if (m == String("Jul")) return Month::JULY;
  if (m == String("Aug")) return Month::AUGUST;
  if (m == String("Sep")) return Month::SEPTEMBER;
  if (m == String("Oct")) return Month::OCTOBER;
  if (m == String("Nov")) return Month::NOVEMBER;
  if (m == String("Dec")) return Month::DECEMBER;
}

void getCurTime(String timeSTR, String* d_w, int* d_mn, String* mn, int* h, int* m, int* s, int* y) {

  *d_w = timeSTR.substring(0, 3);
  *mn = timeSTR.substring(4, 7);
  *d_mn = timeSTR.substring(8, 11).toInt();
  *h = timeSTR.substring(11, 13).toInt();
  *m = timeSTR.substring(14, 16).toInt();
  *s = timeSTR.substring(17, 19).toInt();
  *y = timeSTR.substring(20, 24).toInt();
}
//End of RTC PART



int analogBuffer[SCOUNT];
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0, copyIndex = 0;
float averageVoltage = 0, tdsValue = 0, temperature = 25;
unsigned long int avgValue;
int buf[10], temp;
byte humidity = 0;  // Declare humidity at a higher scope

// Function declarations
int getMedianNum(int bArray[], int iFilterLen);
void readTdsSensor();
void performMedianFiltering();
void readAndSendSensorReadings();
void LightControl();
void PumpControl();
// Define NTP client to request date and time from an NTP server
char ssid[] = "PosteMobile_CE2947_2.4G";
char pass[] = "18029045";
int status = WL_IDLE_STATUS;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
String formattedTime;
String timeStamp;

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);  // Assuming your Serial1 baud rate is 9600

  pinMode(TDS_SENSOR_PIN, INPUT);
  pinMode(MOISTURE_PIN, INPUT);
  pinMode(relay_1, OUTPUT);
  pinMode(relay_2, OUTPUT);
  pinMode(relay_3, OUTPUT);
  pinMode(relay_4, OUTPUT);

  // Connect to WiFi
  connectToWiFi();

  // Initialize NTP client
  //timeClient.begin();
  //timeClient.setTimeOffset(3600);  // Set offset time in seconds to adjust for your timezone

  matrix.begin();
  RTC.begin();
  String timeStamp = _TIMESTAMP_;
  getCurTime(timeStamp, &dayofWeek, &dayofMon, &month, &hours, &minutes, &seconds, &year);
  RTCTime startTime(dayofMon, convertMonth(month), year, hours, minutes, seconds,
                    convertDOW(dayofWeek), SaveLight::SAVING_TIME_ACTIVE);
  RTC.setTime(startTime);
}

void loop() {
  // Check WiFi connection status and reconnect if necessary
   if (WiFi.status() != WL_CONNECTED) {
     connectToWiFi();
   }

  // Rest of your loop code
  delay(1000);
  formattedTime = timeClient.getFormattedTime();
  timeStamp = formattedTime;
  readTdsSensor();
  performMedianFiltering();
  readAndSendSensorReadings();

  //RTC part
  RTCTime currentTime;
  RTC.getTime(currentTime);
  currentHour = currentTime.getHour();  // Get the current hour from NTPClient
  currentMinute = currentTime.getMinutes();
  currentSecond = currentTime.getSeconds();
  if (currentTime.getSeconds() != currentSecond) {
    secondsON_OFF ? secondsON_OFF = 0 : secondsON_OFF = 1;
    displayDigit((int)(currentTime.getHour() / 10), 0, 0);
    displayDigit(currentTime.getHour() % 10, 4, 0);
    displayDigit((int)(currentTime.getMinutes() / 10), 1, 6);
    displayDigit(currentTime.getMinutes() % 10, 5, 6);
    Time[0][2] = secondsON_OFF;
    Time[0][4] = secondsON_OFF;
    currentSecond = currentTime.getSeconds();
    matrix.renderBitmap(Time, 8, 12);
    //Serial.println(secondsON_OFF);
  }
  delay(1);  // // 5 minutes delay

/// LightControl

  // Modify the light control logic based on your requirements and relay wiring
  // if doesn't work use update

  if (currentHour >= 7 && currentHour < 23) {
    digitalWrite(relay_1, HIGH);  // Turn on the light
    LightStatus = 1;
    //Serial.println("Light ON ");
    delay(1000);
  }

  if (currentHour < 7 && currentHour >= 23 ) {
    digitalWrite(relay_1, LOW);  // Turn off the light
    LightStatus = 0;
    //Serial.println("Light OFF ");
    delay(1000);
  }

// Pump Control

  if (currentHour == 19 && currentMinute == 36 && currentSecond<20) {
    digitalWrite(relay_2, HIGH);  // Turn on the pump
	PumpStatus= 1;
	Serial.println("PUMP ON ");
	delay(29000);
	 digitalWrite(relay_2, LOW);
	Serial.println("PUMP OFF ");
  }



}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, pass);
  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
  } else {
    Serial.println("\nWiFi connection failed. Please check your credentials.");
  }
}

void readTdsSensor() {
  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U) {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TDS_SENSOR_PIN);
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT) {
      analogBufferIndex = 0;
    }
  }
}

void performMedianFiltering() {
  static unsigned long printTimepoint = millis();
  if (millis() - printTimepoint > 800U) {
    printTimepoint = millis();
    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++) {
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
    }

    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0;
    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
    float compensationVoltage = averageVoltage / compensationCoefficient;
    tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage - 255.86 * compensationVoltage * compensationVoltage + 857.39 * compensationVoltage) * 0.5;
  }
}

int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0) {
    bTemp = bTab[(iFilterLen - 1) / 2];
  } else {
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  }
  return bTemp;
}

void readAndSendSensorReadings() {
  byte temperature = 0;
  byte humidity = 0;


  if (dht11.read(pinDHT11, &temperature, &humidity, NULL)) {
    //Serial.print("Read DHT11 failed");
    //Serial.print(SimpleDHTErrCode(err));
    //Serial.print(",");
    //Serial.println(SimpleDHTErrDuration(err));
    return;
  }

  avgValue = 0;
  for (int i = 0; i < SCOUNT; i++)
    avgValue += analogBuffer[i];

  float pHValue = (float)avgValue * 5.0 / 1024 / 6;
  pHValue = -8.61 * pHValue + 35.68;

  // Add UART communication here to send all sensor readings via Serialmonitor
  //Serial.print("Timestamp: "); Sostuituire con RTC
  //Serial.println(timeStamp);
  Serial.print("TDS Value: ");
  Serial.println(tdsValue);
  Serial.print("Moisture Value: ");
  Serial.println(analogRead(MOISTURE_PIN));
  Serial.print("Temperature: ");
  Serial.println(temperature);
  Serial.print("Humidity: ");
  Serial.println(humidity);
  Serial.print("pH Value: ");  // Add pH value
  Serial.println(pHValue);
  Serial.println();  // Add other readings as needed
  Serial.print("Timestamp: ");
  Serial.println(timeStamp);

  // Add UART communication here to send all sensor readings via Serial1
  Serial1.print("Timestamp: ");
  Serial1.println(timeStamp);
  Serial1.print("TDS Value: ");
  Serial1.println(tdsValue);
  Serial1.print("Moisture Value: ");
  Serial1.println(analogRead(MOISTURE_PIN));
  Serial1.print("Temperature: ");
  Serial1.println(temperature);
  Serial1.print("Humidity: ");
  Serial1.println(humidity);
  Serial1.println();            // Add other readings as needed
  Serial1.print("pH Value: ");  // Add pH value
  Serial1.println(pHValue);
  Serial1.println();  // Add other readings as needed

//scaldabagno

  if (humidity >= 55 && FanStatus==0 ) {
    digitalWrite(relay_3, HIGH);  // Turn on the light
    FanStatus = 1;
    Serial.println("SCALDABBAGNO ON ");
    delay(1000);
  }

  if (humidity < 50 && FanStatus==1) {
    digitalWrite(relay_3, LOW);  // Turn off the light
    FanStatus = 0;
    Serial.println("SCALDABBAGNO OFF");
    delay(1000);
  }

}

/*
void LightControl() {
  // Modify the light control logic based on your requirements and relay wiring
  // if doesn't work use update
  RTCTime currentTime;
  RTC.getTime(currentTime);
  int currentHour2 = currentTime.getHour();  // Get the current hour from NTPClient
  int currentMinute2 = currentTime.getMinutes();
  int currentSecond2 = currentTime.getSeconds();

  if (currentHour2 >= 7 && currentHour2 < 23 && LightStatus == 0) {
    digitalWrite(relay_1, HIGH);  // Turn on the light
    LightStatus = 1;
    Serial.println("Light ON ");
    delay(1000);
  }

  if (currentHour2 >= 7 && currentHour2 < 23 && LightStatus == 0) {
    digitalWrite(relay_1, LOW);  // Turn off the light
    LightStatus = 0;
    Serial.println("Light OFF ");
    delay(1000);
  }
}


void HumidityControl() {
  // Modify the light control logic based on your requirements and relay wiring
  // if doesn't work use update
  RTCTime currentTime;
  RTC.getTime(currentTime);
  int currentHour = currentTime.getHour();  // Get the current hour from NTPClient
  int currentMinute = currentTime.getMinutes();
  int currentSecond = currentTime.getSeconds();
    // Attempt to read the humidity value from the DHT11 sensor.

  if (humidity >= 55 && FanStatus == 0) {
    digitalWrite(relay_3, HIGH);  // Turn on the light
    FanStatus = 1;
    Serial.println("SCALDABBAGNO ON ");
  }

  if (humidity < 50 && FanStatus == 1) {
    digitalWrite(relay_3, LOW);  // Turn off the light
    FanStatus = 0;
    Serial.println("SCALDABBAGNO OFF");
  }
}

void PumpControl() {
  // Modify the pump control logic based on your requirements and relay wiring
  RTCTime currentTime;
  RTC.getTime(currentTime);
  int currentHour3 = currentTime.getHour();  // Get the current hour from NTPClient
  int currentMinute3 = currentTime.getMinutes();
  int currentSecond3 = currentTime.getSeconds();

  if (currentHour3 == 18 && currentMinute3 > 0 && currentSecond3>0) {
    digitalWrite(relay_2, HIGH);  // Turn on the pump
	PumpStatus= 1;
	Serial.println("PUMP ON ");
	delay(290000);
	 digitalWrite(relay_2, LOW);
	Serial.println("PUMP OFF ");
  }
  
}
*/