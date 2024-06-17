#include <WiFi.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "CDED"; // CHANGE IT
const char* password = "CDED2024."; // CHANGE I

// Create an instance of the server
AsyncWebServer server(80);

// Create an instance of HardwareSerial for UART2


const int uart2BaudRate = 115200;
String uart2Received = "";

void setup() {
  // Start Serial Monitor
  Serial.begin(115200);

  // Initialize UART2
  Serial2.begin(115200);
  

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("ESP32 Web Server's IP address: ");
  Serial.println(WiFi.localIP());

  // Serve the HTML form
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", R"rawliteral(
      <!DOCTYPE HTML><html>
      <head>
        <title>ESP32 Web Input</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
          body { font-family: Arial, sans-serif; margin: 20px; background-color: black; color: red; }
          h2 { color:crimson; }
          form { margin-bottom: 20px; color:orange;}
          input[type="text"] { padding: 5px; margin: 5px 0; width: 200px; border-color: aquamarine }
          input[type="submit"] { padding: 8px 10px; border-radius: 12%; border-color: aquamarine; background-color:aqua;}
          .section { margin-bottom: 20px; }
          .section label { display: block; margin-bottom: 5px; }
        </style>
      </head>
      <body>
        <h2>SLAMBOT COMMAND LINE</h2>
        <div class="section">
          <h3>Distance and Speed</h3>
          <form action="/get">
            <label for="distance">Distance:(m)</label>
            <input type="text" id="distance" name="distance"><br>
            <label for="speed">Speed:(m/s)</label>
            <input type="text" id="speed" name="speed"><br>
            <input type="submit" value="Submit">
          </form>
        </div>
        <div class="section">
          <h3>Turn Angle</h3>
          <form action="/angle">
            <label for="angle">Angle:(degrees)</label>
            <input type="text" id="angle" name="angle"><br>
            <input type="submit" value="Submit">
          </form>
        </div>
        <div class="section">
          <h3>Serial2 Output</h3>
          <div id="serialOutput"></div>
        </div>
        <script>
          setInterval(function() {
            fetch('/serial2')
              .then(response => response.text())
              .then(data => {
                document.getElementById('serialOutput').innerText = data;
              });
          }, 1000);
        </script>
      </body>
      </html>
    )rawliteral");
  });

  // Handle form submission
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request){
    String distance = request->getParam("distance")->value();
    String speed = request->getParam("speed")->value();
    
    
    String distanceCmd = "DIS:" + distance;
    String speedCmd = "SPEED:" + speed;

    Serial2.println(distanceCmd);
    Serial.println(distanceCmd);
    Serial2.println(speedCmd);
    Serial.println(speedCmd);

    String response = "Distance: " + distance + "<br>Speed: " + speed;
    request->send(200, "text/html", response + "<br><a href=\"/\">Return</a>");
  });

  // Handle form submission for angle
  server.on("/angle", HTTP_GET, [](AsyncWebServerRequest *request){
    String angle = request->getParam("angle")->value();
    
    String angleCmd = "ANGLE:" + angle;

    Serial2.println(angleCmd);
    Serial.println(angleCmd);

    String response = "Angle: " + angle + "<br><a href=\"/\">Return</a>";
    request->send(200, "text/html", response);
  });

  // Endpoint to get Serial2 data
  server.on("/serial2", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", uart2Received);
  });


  // Start server
  server.begin();
}

void loop() {
  // Nothing needed here
  // Read from Serial2
  while (Serial2.available()) {
    char incomingChar = (char)Serial2.read();
    uart2Received += incomingChar;

    // Print the received data to Serial Monitor
    Serial.print(incomingChar);
  }

  // Clear uart2Received if it gets too long
  if (uart2Received.length() > 1024) {
    uart2Received = "";
  }
}