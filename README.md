# 🎮 Word Up! – IoT Voice-Controlled Smart Helmet Game

**Word Up! (Braincap)** is a wearable, ESP32-based charades-style party game that combines sensors, audio/visual feedback, and a custom web dashboard. One ESP32 controls the game logic and sensors inside the helmet, while a second ESP32 drives an external OLED display — both communicating over WebSocket via local Wi-Fi.

---

## 🧠 Project Highlights

- Built with ESP32 using PlatformIO
- OLED screen helmet display with real-time word updates
- Rotary encoder + microphone + ultrasonic + accelerometer for input
- RGB NeoPixel visual effects and buzzer feedback
- WebSocket-powered IoT dashboard (HTML/JS/CSS)
- 3D-modeled helmet design + custom PCB
- Fully documented in the included guide

---

## 📁 Folder Structure

| Folder/File                   | Description |
|------------------------------|-------------|
| `Helmet_Main/`               | Main ESP32 PlatformIO project for the helmet (sensors, game logic) |
| `ESP32_Screen/`              | Second ESP32 project for the OLED word display via WebSocket |
| `Helmet Charades Guide.pdf ` | Full project report and build instructions with schematic and more |
| `README.md`                  | This file |
| `.gitignore`                 | Files to exclude from Git |


---

## 🧢 How It Works

1. One player wears the **helmet** with sensors and LEDs.
2. Another player adds words and sets a timer on the **dashboard** (hosted by ESP32).
3. The word appears on the **OLED screen** (connected to a second ESP32).
4. The helmet player guesses the word based on gestures.
5. Nods, voice volume, and rotary encoder can trigger “correct” or “skip.”
6. LEDs and buzzer give real-time feedback. The game ends when time runs out.

---

## 🌐 Components Used

- **ESP32 DevKit** x2
- TM1637 4-digit display
- MAX9814 microphone
- MPU6050 accelerometer
- HC-SR04 ultrasonic sensor
- Rotary encoder switch
- WS2812B NeoPixel LEDs
- ILI9341 TFT or OLED display (ESP32 Screen)
- WebSocket, SPIFFS, ArduinoJson
- Custom 3D-printed helmet
- Rechargeable battery, PCB module

---

## 📄 Documentation

All build steps, 3D models, PCB layouts, wiring diagrams, and BOM are included in:

📘 `Helmet Charades Guide.pdf`

---

## 🛠 Setup Instructions
Helmet Charades Guide.pdf


## Project by:
 Wanposop Surayut, Seneca Polytechnic , Comp Engineering

-Wearable & Embedded Systems


### 1. Clone the repo:
```bash
git clone https://github.com/vo262/Helmet-Charades-Game.git

