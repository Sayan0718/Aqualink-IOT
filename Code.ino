#include "WiFi.h"
#include "ESPAsyncWebServer.h"

#define TRIGGER_PIN 33
#define ECHO_PIN 26

// Set to true to define Relay as Normally Open (NO)
#define RELAY_NO    true

// Set number of relays
#define NUM_RELAYS  1

// Assign each GPIO to a relay
int relayGPIOs[NUM_RELAYS] = {27}; // Assuming GPIO 27 for relay control, change as needed

// Replace with your network credentials
const char* ssid = "Sayan";
const char* password = "";

const char* PARAM_INPUT_1 = "relay";  
const char* PARAM_INPUT_2 = "state";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .container {border: 2px solid #000; width: 300px; height: 300px; position: relative;}
    .water-level {background-color: blue; position: absolute; bottom: 0; left: 0; right: 0; height: 0; transition: height 0.5s ease;}
  </style>
</head>
<body>
  <h2>AQUALINK</h2>
  <div class="container">
    <div class="water-level" id="waterLevel"></div>
  </div>
  <h3>Distance: <span id="distance">--</span></h3>
  %BUTTONPLACEHOLDER%
  <script>
    function toggleCheckbox(element) {
      var xhr = new XMLHttpRequest();
      if(element.checked) {
        xhr.open("GET", "/update?relay="+element.id+"&state=1", true);
      } else {
        xhr.open("GET", "/update?relay="+element.id+"&state=0", true);
      }
      xhr.send();
    }

    setInterval(function() {
      var xhr = new XMLHttpRequest();
      xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          var distance = parseInt(this.responseText);
          document.getElementById("distance").innerHTML = distance + " cm";
          
          // Calculate water level based on distance
          var containerHeight = 300; // Height of the container
          var waterLevel = (distance - 4) * (containerHeight / (9 - 4)); // Convert distance to percentage
          if (waterLevel < 0) waterLevel = 0;
          if (waterLevel > containerHeight) waterLevel = containerHeight;
          waterLevel = containerHeight - waterLevel; // Invert water level to fill from bottom
          document.getElementById("waterLevel").style.height = waterLevel + "px";
          
          // Check if distance is less than equal to 4cm then turn off relay accordingly
          if (distance <= 4) {
            document.getElementById("relay0").checked = false;
            toggleCheckbox(document.getElementById("relay0"));
          }
          
          // Check if distance is more than 9cm then turn on relay accordingly
          else if (distance >= 9) {
            document.getElementById("relay0").checked = true;
            toggleCheckbox(document.getElementById("relay0"));
          }
        }
      };
      xhr.open("GET", "/distance", true);
      xhr.send();
    }, 1000);
  </script>
</body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var){
  if(var == "BUTTONPLACEHOLDER"){
    String buttons ="";
    for(int i=0; i<NUM_RELAYS; i++){
      String relayStateValue = relayState(i);
      buttons+= "<label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"relay" + String(i) + "\" "+ relayStateValue +"><span class=\"slider\"></span></label>";
    }
    return buttons;
  }
  return String();
}

String relayState(int numRelay){
  if(RELAY_NO){
    if(digitalRead(relayGPIOs[numRelay])){
      return "";
    } else {
      return "checked";
    }
  } else {
    if(digitalRead(relayGPIOs[numRelay])){
      return "checked";
    } else {
      return "";
    }
  }
}

void setup(){
  Serial.begin(115200);
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  for(int i=0; i<NUM_RELAYS; i++){
    pinMode(relayGPIOs[i], OUTPUT);
    if(RELAY_NO){
      digitalWrite(relayGPIOs[i], HIGH);
    } else {
      digitalWrite(relayGPIOs[i], LOW);
    }
  }
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputMessage2;

    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      digitalWrite(relayGPIOs[inputMessage.toInt()], inputMessage2.toInt());
    }

    request->send(200, "text/plain", "OK");
  });

  server.on("/distance", HTTP_GET, [] (AsyncWebServerRequest *request) {
    long duration, distance;

    // Clear the trigger pin before starting a new reading
    digitalWrite(TRIGGER_PIN, LOW);
    delayMicroseconds(2);

    // Set the trigger pin high for 10 microseconds
    digitalWrite(TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGGER_PIN, LOW);

    // Read the pulse from the echo pin
    duration = pulseIn(ECHO_PIN, HIGH);

    // Calculate the distance in centimeters
    distance = duration * 0.0343 / 2;

    // If no echo received, set distance to 0
    if (duration == 0) {
      distance = 0;
    }

    // Send the distance as response
    request->send_P(200, "text/plain", String(distance).c_str());
  });

  server.begin();
}
  
void loop() {
  // Nothing to do in loop, all operations are handled in setup or event handlers
}
