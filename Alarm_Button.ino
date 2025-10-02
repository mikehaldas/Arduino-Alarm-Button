/* 
This Arduino sketch creates a wireless alarm button.
When the button is pressed, it triggers a virtual alarm on
a Viewtron IP camera NVR by sending an HTTP Post to the API webhook
endpoint on the NVR. 

The ESP8266 NodeMCU CP2102 ESP-12E Development Board was used for
this project because of the built-in wireless module. 

Wireless access point connection and status code is included, as well 
as debounce code for the push button.

Written by Mike Haldas
mike@viewtron.com

Learn more about this virtual project and Viewtron IP camera NVRs at:
https://www.Viewtron.com/valarm
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// WiFi credentials
const char* ssid = "";
const char* wifiPassword = "";

// Viewtron NVR HTTP POST API Configuration
const char* serverIp = "192.168.0.147"; // Change to IP address or hostname of your Viewtron NVR
const int serverPort = 80; // NVR HTTP port
const char* endpoint = "/TriggerVirtualAlarm/17"; //  Virtual alarm API endpoint. Alarm port # on end
const char* userId = "admin"; // NVR user
const char* password = "my_password"; // NVR password

// XML payload for virtual alarm API call
const String xmlPayload = R"(<?xml version="1.0" encoding="utf-8" ?>
<config version="2.0.0" xmlns="http://www.Sample.ipc.com/ver10">
        <action>
                <status>true</status>
        </action>
</config>)";

// Setup Pins
const int buttonPin = 5;  // D1 (GPIO5) for push button
const int ledPin = 2;     // D4 (GPIO2) for onboard LED (active-low)

// Variables for button debouncing
int buttonState = LOW;       // Current debounced state of the button
int lastButtonState = LOW;   // Previous reading from the input pin
unsigned long lastDebounceTime = 0;  // Last time the button state changed
unsigned long debounceDelay = 50;    // Debounce time in ms

void sendHttpPost() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi not connected. Skipping POST.");
    for (int i = 0; i < 5; i++) {
      digitalWrite(ledPin, HIGH);  // LED off
      delay(100);
      digitalWrite(ledPin, LOW);   // LED on
      delay(100);
    }
    return;
  }

  Serial.println("Starting HTTP POST...");
  WiFiClient client;
  HTTPClient http;
  String url = "http://" + String(serverIp) + ":" + String(serverPort) + endpoint;
  http.setTimeout(5000); // Add timeout for robustness
  if (!http.begin(client, url)) {
    Serial.println("Failed to begin HTTP connection");
    for (int i = 0; i < 5; i++) {
      digitalWrite(ledPin, HIGH);  // LED off
      delay(100);
      digitalWrite(ledPin, LOW);   // LED on
      delay(100);
    }
    return;
  }
  
  http.setAuthorization(userId, password);
  http.addHeader("Content-Type", "application/xml");
  http.addHeader("Content-Length", String(xmlPayload.length()));

  int httpCode = http.POST(xmlPayload);
  if (httpCode >= 200 && httpCode < 300) {
    String response = http.getString();
    Serial.printf("POST successful! Code: %d\nResponse: %s\n", httpCode, response.c_str());
    for (int i = 0; i < 2; i++) {
      digitalWrite(ledPin, HIGH);  // LED off
      delay(200);
      digitalWrite(ledPin, LOW);   // LED on
      delay(200);
    }
  } else {
    Serial.printf("POST failed! Code: %d, Error: %s\n", httpCode, http.errorToString(httpCode).c_str());
    for (int i = 0; i < 5; i++) {
      digitalWrite(ledPin, HIGH);  // LED off
      delay(100);
      digitalWrite(ledPin, LOW);   // LED on
      delay(100);
    }
  }
  http.end();
}

void setup() {
  pinMode(buttonPin, INPUT); // Button with external pull-down resistor
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);  // LED off initially (active-low)

  Serial.begin(9600);
  Serial.println("Setup complete. Connecting to Wi-Fi...");

  WiFi.begin(ssid, wifiPassword);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(ledPin, LOW);   // LED on
    delay(500);
    digitalWrite(ledPin, HIGH);  // LED off
    delay(500);
    Serial.print(".");
  }

  digitalWrite(ledPin, LOW);  // LED stays solid on when Wi-Fi is connected
  Serial.println("\nWi-Fi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Wi-Fi reconnection check
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi disconnected, attempting to reconnect...");
    WiFi.reconnect();
    while (WiFi.status() != WL_CONNECTED) {
      digitalWrite(ledPin, LOW);
      delay(500);
      digitalWrite(ledPin, HIGH);
      delay(500);
      Serial.print(".");
    }
    digitalWrite(ledPin, LOW);
    Serial.println("\nWi-Fi reconnected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }

  // Read button state
  int reading = digitalRead(buttonPin);
  // Serial.print("Button reading: "); // Debug output
  // Serial.println(reading);

  // Check if button state changed
  if (reading != lastButtonState) {
    lastDebounceTime = millis(); // Reset debouncing timer
  }

  // If stable for longer than debounce delay
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Update button state if changed
    if (reading != buttonState) {
      buttonState = reading;
      // Trigger on HIGH (button pressed)
      if (buttonState == HIGH) {
        Serial.println("Button pressed!");
        sendHttpPost();
      }
    }
  }
  // Update lastButtonState for next iteration
  lastButtonState = reading;
}