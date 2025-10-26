
### 🧰 Required Components
- 2 × Nintendo Game Boy **DMG-01** (“the brick” model)  
- 2 × **ESP32** boards (DOIT ESP32 Devkit V1 recommended)  
- 1 × **Bi-directional Level Shifter** (5 V ↔ 3.3 V)  
- Standard wiring for **SI / SO / SCK / GND** lines  

---

## 💾 Software Components

### ⚙️ ESP32 Code
- **Server** – Acts as the main WiFi access point, manages synchronization and relays serial data between consoles.  
- **Client** – Connects to the server via WiFi and mirrors the Game Link communication on the second Game Boy.

### 🎮 Game Boy Code
- **Server ROM** – Sends button states and data through the link port (master mode).  
- **Client ROM** – Receives and processes data from the ESP32 link (slave mode).  
Both ROMs are written in **C (GBDK)** and designed to display button input and link status on-screen.

---

## 📡 How It Works

1. Each ESP32 is connected to a Game Boy’s Game Link port via a level shifter.  
2. The **server ESP32** creates a WiFi network; the **client ESP32** connects to it.  
3. Serial data from one Game Boy is read bit-by-bit (synchronized by the Game Link clock), transmitted over WiFi, and reproduced on the opposite ESP32.  
4. The system allows **real-time bidirectional communication** between two Game Boys wirelessly.

---

## 🧠 Technical Notes

- Communication timing follows the original **Game Link protocol (~512 kHz clock)**.  
- Both consoles must share a **common ground** through the level shifters and ESP32 boards.  
- All transfers are **fully synchronous**, respecting the Game Boy’s master/slave SPI-like behavior.  
- Can be adapted for other link-based applications such as **multiplayer games, debugging, or data exchange**.

---

## 🚀 Current Status

✅ Working prototype: button transfer and bidirectional communication tested.  
🧩 Next steps: latency optimization and full multiplayer game testing.

---

## 💡 Project Motivation & Credits

This project was created out of curiosity and passion for classic hardware — to **modernize the iconic Game Link cable** using contemporary IoT technology while preserving the authentic behavior of the original Game Boy hardware.  

Developed and tested on real **DMG-01 units** with **ESP32 Devkit boards** and custom C code written for both sides of the link.  

🧠 Special thanks to the **retro-hardware community** for their resources and documentation on the Game Boy serial protocol.

---

📅 **Author:** Frange  
📍 **Project type:** Open-source hardware experiment  
🛠️ **Languages:** C (GBDK) · Arduino (ESP32)  
💾 **Platforms:** Game Boy DMG-01 · ESP32
