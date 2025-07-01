// 📦 Include libraries for sensors, WiFi, LEDs, server, etc.
#include <Arduino.h>
#include <Wire.h>
#include <TM1637Display.h>
#include <NewPing.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <RotaryEncoder.h>
#include <MPU6050.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <vector>

// 📶 WiFi and hardware pin setup
const char* ssid = "PI34";
const char* password = "vo123456";

#define ROTARY_CLK 26
#define ROTARY_DT 27
#define ROTARY_SW 33
#define MIC_PIN 34
#define BUZZER_PIN 25
#define TRIG_PIN 5
#define ECHO_PIN 14
#define TM1637_CLK 18
#define TM1637_DIO 19
#define I2C_SDA 21
#define I2C_SCL 22
#define LED_PIN 4
#define NUM_LEDS 83

// 🔧 Initialize all hardware components
TM1637Display display(TM1637_CLK, TM1637_DIO);
NewPing sonar(TRIG_PIN, ECHO_PIN, 400);
RotaryEncoder encoder(ROTARY_CLK, ROTARY_DT);
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
MPU6050 mpu;
Adafruit_NeoPixel leds(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// 🕹️ Game + Timer variables
unsigned long timerDuration = 0, startTime = 0;
bool timerRunning = false, buzzerTriggered = false, helmetDetectedOnce = false;
bool gameStarted = false;
String wordList[10];
int wordCount = 0, currentWordIndex = -1;
long lastPosition = 0;
unsigned long lastNodTime = 0;

// 🌈 LED effect settings
std::vector<uint32_t> currentColors;
String currentEffect = "solid";
bool ledNeedsUpdate = false;
bool ledManualControl = false;

// 🎵 Melody/music variables
String selectedMelody = "none";
int melodyIndex = 0;
unsigned long lastNoteTime = 0;

// 🧱 Structure for buzzer notes
struct Note { int freq; int duration; };
std::vector<Note> melodyNotes;

// 🎶 Return melody data based on name
std::vector<Note> getMelody(String name) {
  if (name == "nokia") return {{1318, 200}, {1174, 200}, {1046, 200}, {0, 100}, {1046, 200}, {988, 200}, {880, 300}, {0, 100}};
  if (name == "wipwup") return {{784, 200}, {880, 200}, {988, 200}, {1046, 200}, {0, 100}, {1174, 200}, {988, 300}, {0, 100}};
  if (name == "ateez") return {{659, 200}, {698, 200}, {784, 200}, {0, 100}, {880, 200}, {988, 200}, {1046, 300}, {0, 100}};
  if (name == "baby") return {{784, 150}, {784, 150}, {659, 150}, {0, 100}, {784, 150}, {587, 200}, {659, 300}, {0, 100}};
  return {};
}

// 🔁 Play melody notes using non-blocking logic
void playMelodyNonBlocking() {
  if (!gameStarted || selectedMelody == "none" || melodyNotes.empty()) return;
  if (millis() - lastNoteTime > melodyNotes[melodyIndex].duration + 50) {
    if (melodyNotes[melodyIndex].freq > 0) tone(BUZZER_PIN, melodyNotes[melodyIndex].freq);
    else noTone(BUZZER_PIN);
    lastNoteTime = millis();
    melodyIndex = (melodyIndex + 1) % melodyNotes.size();
  }
}

// 💡 Set all LEDs to the same color
void setAllLEDs(uint32_t color) {
  for (int i = 0; i < NUM_LEDS; i++) leds.setPixelColor(i, color);
  leds.show();
}

// 🎨 LED color based on current word index
void updateLEDColorPerWord() {
  uint8_t hue = (currentWordIndex * 40) % 255;
  for (int i = 0; i < NUM_LEDS; i++) leds.setPixelColor(i, leds.ColorHSV(hue * 256));
  leds.show();
}

// ⏰ Change LED colors based on countdown
void countdownLEDAlert(int remaining) {
  if (remaining <= 3) setAllLEDs(leds.Color(255, 0, 0));          // Red
  else if (remaining <= 6) setAllLEDs(leds.Color(255, 165, 0));   // Orange
  else setAllLEDs(leds.Color(255, 255, 0));                       // Yellow
}

// 🌐 WebSocket handler for receiving dashboard messages
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len) {
      data[len] = 0;
      String msg = (char*)data;
      if (msg.startsWith("{")) {
        DynamicJsonDocument doc(1024);
        if (deserializeJson(doc, msg)) return;

        // 🕐 Receive game timer
        if (doc.containsKey("timer")) {
          timerDuration = doc["timer"].as<int>();
          display.showNumberDec(timerDuration, true);
          startTime = millis(); timerRunning = true; buzzerTriggered = false;
        }

        // 📝 Receive word
        if (doc.containsKey("word")) {
          if (wordCount < 10) wordList[wordCount++] = doc["word"].as<String>();
        }

        // ❌ Remove word
        if (doc.containsKey("remove")) {
          String toRemove = doc["remove"].as<String>();
          for (int i = 0; i < wordCount; i++) {
            if (wordList[i] == toRemove) {
              for (int j = i; j < wordCount - 1; j++) wordList[j] = wordList[j + 1];
              wordCount--; break;
            }
          }
        }

        // 🎵 Set melody
        if (doc.containsKey("melody")) {
          selectedMelody = doc["melody"].as<String>();
          melodyNotes = getMelody(selectedMelody);
          melodyIndex = 0;
        }

        // ▶️ Start game
        if (doc.containsKey("start") && wordCount > 0) {
          gameStarted = true; currentWordIndex = 0;
          ws.textAll("{\"word\":\"" + wordList[currentWordIndex] + "\"}");
          updateLEDColorPerWord();
        }

        // 🌈 Receive LED colors and effect
        if (doc.containsKey("colors") && doc.containsKey("effect")) {
          JsonArray arr = doc["colors"].as<JsonArray>();
          String effect = doc["effect"].as<String>();
          std::vector<uint32_t> colors;
          for (JsonVariant v : arr) {
            String hex = v.as<String>();
            long rgb = strtol(&hex[1], nullptr, 16);
            colors.push_back(leds.Color((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF));
          }
          currentColors = colors;
          currentEffect = effect;
          ledNeedsUpdate = true;
          ledManualControl = true;
        }
      }
    }
  }
}

// 🌀 LED animation effect variables
unsigned long lastLedUpdate = 0;
bool blinkState = false;
int fadeBrightness = 0;
int fadeDelta = 5;
int breatheBrightness = 0;
int breatheDelta = 5;

// 🚀 Setup runs once
void setup() {
  Serial.begin(115200);
  pinMode(ROTARY_CLK, INPUT_PULLUP);
  pinMode(ROTARY_DT, INPUT_PULLUP);
  encoder.setPosition(0);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(MIC_PIN, INPUT);
  display.setBrightness(7);
  display.showNumberDec(0, true);
  leds.begin(); leds.setBrightness(40); leds.clear(); leds.show();
  WiFi.begin(ssid, password); while (WiFi.status() != WL_CONNECTED) delay(500);
  if (!SPIFFS.begin(true)) return;
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  ws.onEvent(onWebSocketEvent); server.addHandler(&ws); server.begin();
  Wire.begin(I2C_SDA, I2C_SCL); mpu.initialize();
}

// 🔁 Main loop runs repeatedly
void loop() {
  ws.cleanupClients();
  playMelodyNonBlocking(); // 🔉 Play melody in background

  // 🎛️ Check rotary encoder for LED effect changes
  encoder.tick();
  long newPosition = encoder.getPosition();
  if (newPosition != lastPosition) {
    if (ledManualControl && !gameStarted) {
      static int effectIndex = 0;
      const char* effects[] = {"solid", "blink", "fade", "breathing"};
      int modeCount = sizeof(effects) / sizeof(effects[0]);
      effectIndex = (newPosition > lastPosition) ? (effectIndex + 1) % modeCount : (effectIndex - 1 + modeCount) % modeCount;
      currentEffect = effects[effectIndex];
      ledNeedsUpdate = true;
      Serial.print("LED Effect: "); Serial.println(currentEffect);
      ws.textAll("{\"log\":\"Rotary Encoder turned — Effect: " + String(currentEffect) + "\"}");
    }
    lastPosition = newPosition;
  }

  // 🌟 Handle LED effects (non-blocking)
  if (ledManualControl && !gameStarted) {
    unsigned long now = millis();
    if (currentEffect == "solid" && ledNeedsUpdate) {
      for (int i = 0; i < NUM_LEDS; i++) leds.setPixelColor(i, currentColors[i % currentColors.size()]);
      leds.setBrightness(255); leds.show(); ledNeedsUpdate = false;
    } else if (currentEffect == "blink" && now - lastLedUpdate > 300) {
      blinkState = !blinkState;
      for (int i = 0; i < NUM_LEDS; i++) leds.setPixelColor(i, blinkState ? currentColors[i % currentColors.size()] : 0);
      leds.show(); lastLedUpdate = now;
    } else if (currentEffect == "fade" && now - lastLedUpdate > 20) {
      fadeBrightness += fadeDelta;
      if (fadeBrightness >= 255 || fadeBrightness <= 0) fadeDelta = -fadeDelta;
      leds.setBrightness(fadeBrightness);
      for (int i = 0; i < NUM_LEDS; i++) leds.setPixelColor(i, currentColors[i % currentColors.size()]);
      leds.show(); lastLedUpdate = now;
    } else if (currentEffect == "breathing" && now - lastLedUpdate > 30) {
      breatheBrightness += breatheDelta;
      if (breatheBrightness >= 255 || breatheBrightness <= 0) breatheDelta = -breatheDelta;
      leds.setBrightness(breatheBrightness);
      for (int i = 0; i < NUM_LEDS; i++) leds.setPixelColor(i, currentColors[i % currentColors.size()]);
      leds.show(); lastLedUpdate = now;
    }
  }

  // ⛑️ Helmet detection using distance sensor
  int distance = sonar.ping_cm();
  if (distance > 0 && distance <= 2 && !helmetDetectedOnce) {
    helmetDetectedOnce = true;
    ws.textAll("{\"log\":\"Helmet worn detected\"}");
    for (int i = 0; i < 2; i++) {
      setAllLEDs(leds.Color(0, 255, 0)); delay(200);
      setAllLEDs(leds.Color(0, 0, 0)); delay(200);
    }
  }

  // ⏳ Game timer countdown logic
  if (timerRunning) {
    unsigned long elapsed = (millis() - startTime) / 1000;
    int remaining = timerDuration - elapsed;
    if (remaining > 0) {
      display.showNumberDec(remaining, true);
      if (remaining <= 10 && !buzzerTriggered) {
        countdownLEDAlert(remaining);
        tone(BUZZER_PIN, 1000); delay(200);
        noTone(BUZZER_PIN); delay(800);
      }
    } else {
      timerRunning = false; gameStarted = false; melodyNotes.clear(); melodyIndex = 0;
      noTone(BUZZER_PIN); display.showNumberDec(0, true);
      ws.textAll("{\"word\":\"Game Over\"}");
      setAllLEDs(leds.Color(255, 0, 0));
      if (!buzzerTriggered) {
        tone(BUZZER_PIN, 1000); delay(1000);
        noTone(BUZZER_PIN); buzzerTriggered = true;
      }
    }
  }

  // 🤖 Detect head nod using MPU6050
  if (gameStarted && millis() - lastNodTime > 1000) {
    int16_t ax, ay, az;
    mpu.getAcceleration(&ax, &ay, &az);
    if (az < -15000 || az > 15000) {
      ws.textAll("{\"log\":\"MPU6050: Head nod detected\"}");
      currentWordIndex++;
      if (currentWordIndex >= wordCount) {
        currentWordIndex = 0; gameStarted = false; melodyNotes.clear(); melodyIndex = 0;
        noTone(BUZZER_PIN);
        ws.textAll("{\"word\":\"Game Over\"}");
      } else {
        ws.textAll("{\"word\":\"" + wordList[currentWordIndex] + "\"}");
        updateLEDColorPerWord();
      }
      lastNodTime = millis();
    }
  }

  // 🎤 Detect loud sound to skip word
  int micLevel = analogRead(MIC_PIN);
  if (gameStarted && micLevel > 3000 && wordCount > 0 && helmetDetectedOnce && timerRunning) {
    currentWordIndex++;
    if (currentWordIndex >= wordCount) {
      currentWordIndex = 0; gameStarted = false; melodyNotes.clear(); melodyIndex = 0;
      noTone(BUZZER_PIN);
      ws.textAll("{\"word\":\"Game Over\"}");
    } else {
      ws.textAll("{\"word\":\"" + wordList[currentWordIndex] + "\"}");
      updateLEDColorPerWord();
    }
    delay(500);
  }
}
