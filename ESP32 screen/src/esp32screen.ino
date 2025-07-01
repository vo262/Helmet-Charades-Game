/*******************************************************************
    Modified ESP32 LCD Code for Helmet Project
    - Connects to Main ESP32 via WebSocket
    - Displays word updates and helmet status
*******************************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <TFT_eSPI.h>
#include <ArduinoJson.h>

// WiFi Credentials (phone hotspot)
const char* ssid = "PI34";
const char* password = "vo123456";

// IP of the main ESP32 WebSocket server (change if needed)
const char* serverIP = "192.168.26.167";  // or your ESP32’s IP
const int serverPort = 80;

TFT_eSPI tft = TFT_eSPI();
WebSocketsClient webSocket;

// Variables to store received data
String currentWord = "";
String helmetStatus = "";

// WebSocket event handler
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  if (type == WStype_TEXT) {
    Serial.printf("Received: %s\n", payload);

    // Try to parse JSON
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      if (doc.containsKey("word")) {
        currentWord = doc["word"].as<String>();
      } else if (doc.containsKey("helmet")) {
        helmetStatus = doc["helmet"].as<String>() == "on" ? "Helmet On!" : "Helmet Off!";
      } else if (doc.containsKey("command")) {
        String command = doc["command"].as<String>();
        if (command == "skip") {
          currentWord = "Skipped!";
        }
      }
    } else {
      // Handle plain text fallback
      currentWord = (char*)payload;
    }

    // Refresh screen
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(10, 40);
    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW);
    tft.println("Word:");
    tft.setTextSize(3);
    tft.setTextColor(TFT_WHITE);
    tft.println(currentWord);

    if (helmetStatus != "") {
      tft.setCursor(10, 120);
      tft.setTextSize(2);
      tft.setTextColor(TFT_GREEN);
      tft.println(helmetStatus);
    }
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Waiting for word...");

  // Connect to WebSocket server
  webSocket.begin(serverIP, serverPort, "/ws");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
}

void loop() {
  webSocket.loop();
}

