/* 
This Arduino sketch creates a wireless alarm button / panic button
for integration with virtual alarms on Viewtron IP camera NVRs.
https://www.viewtron.com

When the button is pressed, it triggers a virtual alarm on
a NVR by sending an HTTP Post to the NVR's API / webhook endpoint. 

The ESP8266 NodeMCU CP2102 ESP-12E Arduino Development Board was used for
this project because of the small size and built-in wireless module. 

Wireless connectivity configured via WiFiManager.h by tzapu.
WiFiManager also being used to configure NVR variables.

Default ad-hoc wireless AP connection for initial setup:
SSID:  Viewtron-Button
Password: 12345678

Connect to it and open a web browser to 192.168.1.4 to configure WIFI and NVR variables.
Hold button for 5 seconds to reset all settings and re-enable ad-hoc wireless AP.

Created by Mike Haldas, co-founder at CCTV Camera Pros
mike@viewtron.com
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiManager.h>
#include <EEPROM.h>

// ---------- WiFiManager ----------
WiFiManager wm;

// ---------- Configurable Parameters ----------
String serverIp = "";
int    serverPort = 0;
String userId = "";
String nvrPassword = "";
String alarmPort = "";

// ---------- Pins ----------
const int buttonPin = 5;
const int ledPin = 2;

// ---------- Debounce & Reset ----------
int buttonState = LOW;
int lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
unsigned long buttonPressTime = 0;
const unsigned long resetHoldTime = 5000;

// ---------- XML Payload ----------
const String xmlPayload = R"(<?xml version="1.0" encoding="utf-8" ?>
<config version="2.0.0" xmlns="http://www.Sample.ipc.com/ver10">
        <action>
                <status>true</status>
        </action>
</config>)";

// ---------- EEPROM (Electrically Erasable Programmable Read-Only Memory)----------
#define EEPROM_SIZE 512
#define ADDR_SERVER_IP    0
#define ADDR_SERVER_PORT  64
#define ADDR_USER_ID      80
#define ADDR_NVR_PASS     128
#define ADDR_ALARM_PORT   192

// ---------- WiFiManager Parameters (global) ----------
WiFiManagerParameter *p_serverIp;
WiFiManagerParameter *p_serverPort;
WiFiManagerParameter *p_userId;
WiFiManagerParameter *p_nvrPass;
WiFiManagerParameter *p_alarmPort;

// ---------- Function Prototypes ----------
void saveConfig();
void loadConfig();
void addCustomParameters();
void sendHttpPost();
void blinkLed(int times, int del);
String readStringEEPROM(int addr);
void writeStringEEPROM(int addr, const String& s);
void printConfig();

void setup() {
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  Serial.begin(9600);
  Serial.println("\nBooting...");

  EEPROM.begin(EEPROM_SIZE);
  loadConfig();

  addCustomParameters();

  if (!wm.autoConnect("Viewtron-Button", "12345678")) {
    Serial.println("Failed to connect. Restarting...");
    delay(3000);
    ESP.restart();
  }

  digitalWrite(ledPin, LOW);
  Serial.println("Wi-Fi connected!");
  Serial.print("IP: "); Serial.println(WiFi.localIP());
  printConfig();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi lost. Reconnecting...");
    if (!wm.autoConnect("Viewtron-Button", "12345678")) {
      Serial.println("Reconnect failed. Restarting...");
      delay(3000);
      ESP.restart();
    }
    digitalWrite(ledPin, LOW);
    Serial.println("Reconnected!");
  }

  int reading = digitalRead(buttonPin);

  if (reading == HIGH && lastButtonState == LOW) {
    buttonPressTime = millis();
  }

  if (reading == HIGH && (millis() - buttonPressTime) > resetHoldTime) {
    Serial.println("RESET ALL SETTINGS!");
    blinkLed(5, 100);
    wm.resetSettings();
    EEPROM.begin(EEPROM_SIZE);
    for (int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0);
    EEPROM.commit();
    Serial.println("Erased. Restarting...");
    delay(1000);
    ESP.restart();
  }

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == HIGH && (millis() - buttonPressTime) < resetHoldTime) {
        Serial.println("ALARM TRIGGERED!");
        sendHttpPost();
      }
    }
  }

  lastButtonState = reading;
}

// ------------------------------------------------------------
// Save to EEPROM
// ------------------------------------------------------------
void saveConfig() {
  writeStringEEPROM(ADDR_SERVER_IP,   serverIp);
  writeStringEEPROM(ADDR_SERVER_PORT, String(serverPort));
  writeStringEEPROM(ADDR_USER_ID,     userId);
  writeStringEEPROM(ADDR_NVR_PASS,    nvrPassword);  // <-- FIXED
  writeStringEEPROM(ADDR_ALARM_PORT,  alarmPort);
  EEPROM.commit();
  Serial.println("Config saved.");
}

// ------------------------------------------------------------
// Load from EEPROM
// ------------------------------------------------------------
void loadConfig() {
  serverIp     = readStringEEPROM(ADDR_SERVER_IP);
  serverPort   = readStringEEPROM(ADDR_SERVER_PORT).toInt();
  userId       = readStringEEPROM(ADDR_USER_ID);
  nvrPassword  = readStringEEPROM(ADDR_NVR_PASS);
  alarmPort    = readStringEEPROM(ADDR_ALARM_PORT);

  if (serverIp.length() == 0) serverIp = "192.168.0.100";
  if (serverPort == 0) serverPort = 80;
  if (userId.length() == 0) userId = "admin";
  if (nvrPassword.length() == 0) nvrPassword = "a1111111";
  if (alarmPort.length() == 0) alarmPort = "9";
}

// ------------------------------------------------------------
// Add custom fields for NVR config + save callback
// ------------------------------------------------------------
void addCustomParameters() {
  p_serverIp   = new WiFiManagerParameter("serverIp", "NVR IP", serverIp.c_str(), 16);
  p_serverPort = new WiFiManagerParameter("port", "NVR Port", String(serverPort).c_str(), 6);
  p_userId     = new WiFiManagerParameter("user", "NVR User", userId.c_str(), 32);
  p_nvrPass    = new WiFiManagerParameter("pass", "NVR Password", nvrPassword.c_str(), 32, "password");
  p_alarmPort  = new WiFiManagerParameter("alarm", "Alarm Port", alarmPort.c_str(), 3);

  wm.addParameter(p_serverIp);
  wm.addParameter(p_serverPort);
  wm.addParameter(p_userId);
  wm.addParameter(p_nvrPass);
  wm.addParameter(p_alarmPort);

  wm.setSaveConfigCallback([]() {
    serverIp     = p_serverIp->getValue();
    serverPort   = String(p_serverPort->getValue()).toInt();
    userId       = p_userId->getValue();
    nvrPassword  = p_nvrPass->getValue();
    alarmPort    = p_alarmPort->getValue();
    saveConfig();
  });
}

// ------------------------------------------------------------
// HTTP POST
// ------------------------------------------------------------
void sendHttpPost() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("No Wi-Fi.");
    blinkLed(5, 100);
    return;
  }

  Serial.println("Sending POST...");
  WiFiClient client;
  HTTPClient http;
  String url = "http://" + serverIp + ":" + String(serverPort) + "/TriggerVirtualAlarm/" + alarmPort;

  http.setTimeout(5000);
  if (!http.begin(client, url)) {
    Serial.println("HTTP begin failed");
    blinkLed(5, 100);
    return;
  }

  http.setAuthorization(userId.c_str(), nvrPassword.c_str());
  http.addHeader("Content-Type", "application/xml");

  int code = http.POST(xmlPayload);
  if (code >= 200 && code < 300) {
    Serial.printf("Success: %d\n", code);
    blinkLed(2, 200);
  } else {
    Serial.printf("Failed: %d, %s\n", code, http.errorToString(code).c_str());
    blinkLed(5, 100);
  }
  http.end();
}

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------
void blinkLed(int times, int del) {
  for (int i = 0; i < times; i++) {
    digitalWrite(ledPin, HIGH); delay(del);
    digitalWrite(ledPin, LOW);  delay(del);
  }
}

String readStringEEPROM(int addr) {
  String s = "";
  int i = addr;
  char c;
  while (i < EEPROM_SIZE && (c = EEPROM.read(i++)) != 0) s += c;
  return s;
}

void writeStringEEPROM(int addr, const String& s) {
  int i;
  for (i = 0; i < s.length() && addr + i < EEPROM_SIZE; i++) {
    EEPROM.write(addr + i, s[i]);
  }
  EEPROM.write(addr + i, 0);
}

void printConfig() {
  Serial.println("=== Config ===");
  Serial.printf("IP: %s\n", serverIp.c_str());
  Serial.printf("Port: %d\n", serverPort);
  Serial.printf("User: %s\n", userId.c_str());
  Serial.printf("Pass: %s\n", nvrPassword.c_str());
  Serial.printf("Alarm: %s\n", alarmPort.c_str());
  Serial.println("==============");
}
