#include <WiFi.h>

const char* ssid = "";
const char* password = "";

WiFiServer server(80);

String header;

const uint8_t stepPin = 25;
const uint8_t directionPin = 26;
const uint8_t enablePin = 27;
const uint8_t clockwiseLevel = HIGH;
const int motorFullStepsPerRevolution = 200;
const int microstepsPerFullStep = 8;
const int stepsPerRevolution = motorFullStepsPerRevolution * microstepsPerFullStep;
const int stepsPerDispense = stepsPerRevolution / 8;
const unsigned long motorSpeedRpm = 10;
const unsigned long stepIntervalUs = 60000000UL / (stepsPerRevolution * motorSpeedRpm);
const unsigned int stepPulseWidthUs = 4;

String stepperState = "stopped";

unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

void moveStepper(int steps) {
  digitalWrite(directionPin, steps > 0 ? clockwiseLevel : !clockwiseLevel);
  digitalWrite(enablePin, LOW);
  delay(2);

  const int pulseCount = abs(steps);
  for (int pulse = 0; pulse < pulseCount; ++pulse) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(stepPulseWidthUs);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(stepIntervalUs - stepPulseWidthUs);

    // Let the idle task run during longer moves.
    if ((pulse + 1) % 32 == 0) {
      delay(1);
    }
  }

  digitalWrite(enablePin, HIGH);
}

void setup() {
  Serial.begin(115200);

  pinMode(enablePin, OUTPUT);
  digitalWrite(enablePin, HIGH);
  pinMode(stepPin, OUTPUT);
  digitalWrite(stepPin, LOW);
  pinMode(directionPin, OUTPUT);
  digitalWrite(directionPin, clockwiseLevel);

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    currentTime = millis();
    previousTime = currentTime;

    Serial.println("New client");

    String currentLine = "";

    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();

      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        header += c;

        if (c == '\n') {
          if (currentLine.length() == 0) {

            if (header.startsWith("POST /dispense ")) {
              moveStepper(stepsPerDispense);
              stepperState = "dispensed (1/8 turn clockwise)";
              client.println("HTTP/1.1 303 See Other");
              client.println("Location: /");
              client.println("Content-Length: 0");
              client.println("Connection: close");
              client.println();
              break;
            }

            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

         
            if (header.startsWith("GET /stepper/cw ")) {
              stepperState = "clockwise";
              moveStepper(stepsPerRevolution / 4);
            }

            else if (header.startsWith("GET /stepper/ccw ")) {
              stepperState = "counterclockwise";
              moveStepper(-stepsPerRevolution / 4);
            }

            else if (header.startsWith("GET /stepper/fullcw ")) {
              stepperState = "one full clockwise rotation";
              moveStepper(stepsPerRevolution);
            }

            else if (header.startsWith("GET /stepper/fullccw ")) {
              stepperState = "one full counterclockwise rotation";
              moveStepper(-stepsPerRevolution);
            }

           
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");

            client.println("<link rel=\"icon\" href=\"data:,\">");

            client.println("<style>");
            client.println("html{font-family:monospace;text-align:center;}");

            client.println(".button{background:yellowgreen;border:none;color:white;padding:16px 32px;font-size:24px;margin:6px;cursor:pointer;}");
            client.println(".reverse{background:gray;}");
            client.println(".full{background:royalblue;}");
            client.println("</style>");

            client.println("</head><body>");

            client.println("<h1>ESP32 Stepper Motor Control</h1>");
            client.println("<p>TMC2209 stepper motor control</p>");

            client.println("<p>Stepper state: <b>" + stepperState + "</b></p>");
            client.println("<form method=\"post\" action=\"/dispense\"><button class=\"button\" type=\"submit\">DISPENSE (1/8 TURN)</button></form>");

            client.println("<p><a href=\"/stepper/cw\"><button class=\"button\">CLOCKWISE 1/4 TURN</button></a></p>");

            client.println("<p><a href=\"/stepper/ccw\"><button class=\"button reverse\">COUNTERCLOCKWISE 1/4 TURN</button></a></p>");


            client.println("<p><a href=\"/stepper/fullcw\"><button class=\"button full\">FULL TURN CW</button></a></p>");
            client.println("<p><a href=\"/stepper/fullccw\"><button class=\"button full\">FULL TURN CCW</button></a></p>");

            client.println("<p><small>ESP32 IP: " + WiFi.localIP().toString() + "</small></p>");

            client.println("</body></html>");

            client.println();
            break;
          } 
          else {
            currentLine = "";
          }
        } 
        else if (c != '\r') {

          currentLine += c;
        }
      }
    }

    header = "";
    client.stop();

    Serial.println("Client disconnected");
    Serial.println();
  }
}
