#include <WiFi.h>
#include <Stepper.h>

const char* ssid = "";
const char* password = "";

WiFiServer server(80);

String header;

const int stepsPerRevolution = 2048;




Stepper myStepper(stepsPerRevolution, 16, 18, 17, 19);

String stepperState = "stopped";

unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

void setup() {
  Serial.begin(115200);

  myStepper.setSpeed(10);

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

            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

         
            if (header.indexOf("GET /stepper/cw") >= 0) {
              stepperState = "clockwise";
              myStepper.step(512);   //480
            }

            else if (header.indexOf("GET /stepper/ccw") >= 0) {
              stepperState = "counterclockwise";
              myStepper.step(-512);  //-500 , -490 ish
            }

            else if (header.indexOf("GET /stepper/fullcw") >= 0) {
              stepperState = "one full clockwise rotation";
              myStepper.step(stepsPerRevolution);
            }

            else if (header.indexOf("GET /stepper/fullccw") >= 0) {
              stepperState = "one full counterclockwise rotation";
              myStepper.step(-stepsPerRevolution);
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
            client.println("<p>This controls a stepper motor using a ULN2003 driver.</p>");

            client.println("<p>Stepper state: <b>" + stepperState + "</b></p>");

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