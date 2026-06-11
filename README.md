# ⚡ ESP32 Voice-Controlled Smart Room (Cloud-Only)

A lightweight, headless room automation system built on the ESP32 platform, integrated directly with ESP RainMaker and Google Assistant for seamless voice control. 

Designed for embedded stability, this firmware handles asynchronous Wi-Fi reconnections, real-time state management, and secure webhook processing without relying on commercial smart home hubs or local physical switches.

---

## ✨ Features
* **100% Cloud/Voice Driven:** Lean architecture with no physical switch polling overhead.
* **Google Assistant Native:** Works instantly with Google Home via ESP RainMaker integration.
* **Non-Volatile State Memory:** Remembers relay states after a power outage using EEPROM.
* **Anti-Flicker Guard:** Software debouncing (80ms buffer) prevents relay chatter from rapid cloud requests.
* **Active-Low/High Configurable:** Easily swap logic states depending on your specific relay module.

## 🛠️ Hardware Requirements
* **Microcontroller:** ESP32 Development Board (e.g., NodeMCU-32S)
* **Actuators:** 4-Channel Relay Module (5V/3.3V)
* **Power:** 5V Power Supply (Make sure it can handle the relay coil current)

### 🔌 Pin Mapping (Default)
| Device | ESP32 GPIO | Notes |
| :--- | :--- | :--- |
| **Relay 1** (Lights in) | `GPIO 26` | Change `RELAY_ON` to `LOW` if using Active-Low relays |
| **Relay 2** (Lights out) | `GPIO 25` | |
| **Relay 3** (White LED) | `GPIO 33` | |
| **Relay 4** (Yellow LED) | `GPIO 32` | |
| **WiFi LED** | `GPIO 13` | Indication LED for WiFi connection status |

## 🚀 Setup & Installation

1. **Install Dependencies:**
   Ensure you have the ESP32 board package and the **ESP RainMaker** library installed in your Arduino IDE.
2. **Configure Logic:**
   Check your relay module. If it activates on a `LOW` signal, modify these lines in the code:
   ```cpp
   #define RELAY_ON   LOW
   #define RELAY_OFF  HIGH
   ```
3. **Upload:** Select your ESP32 board and upload the code.
4. **Provision:** 
   * Open the **ESP RainMaker** mobile app.
   * Scan the generated QR code (from Serial Monitor) or provision via BLE.
   * **Service Name:** `PROV_12345`
   * **PoP:** `1234567`
