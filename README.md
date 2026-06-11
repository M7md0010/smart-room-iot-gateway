# Smart Room IoT Gateway - ESP32 RainMaker 4-Relay Controller

An ESP32-based smart room controller utilizing **ESP RainMaker** for cloud connectivity, featuring state persistence using EEPROM, active-high/low relay configuration, and software-based anti-flicker protection.

## Features

- **ESP RainMaker Cloud Integration:** Control appliances remotely using the ESP RainMaker app (iOS/Android) or voice assistants (Alexa, Google Assistant).
- **WiFi Provisioning over BLE:** Easy provisioning using the ESP RainMaker app with secure BLE configuration.
- **EEPROM State Persistence:** Automatically saves and restores the last relay states on power cycles or sudden restarts.
- **Software Anti-Flicker Protection:** Implements a state-change guard interval (80ms buffer) to prevent relay bouncing/flickering.
- **Dynamic Device Support:** Creates four virtual Switch devices:
  1. `Lights in`
  2. `Lights out`
  3. `White LED`
  4. `Yellow LED`

---

## Hardware Configuration

### GPIO Pin Mapping

| Component | ESP32 GPIO | Description |
| :--- | :--- | :--- |
| **Relay 1** | `GPIO 26` | Control pin for "Lights in" |
| **Relay 2** | `GPIO 25` | Control pin for "Lights out" |
| **Relay 3** | `GPIO 33` | Control pin for "White LED" |
| **Relay 4** | `GPIO 32` | Control pin for "Yellow LED" |
| **WiFi LED** | `GPIO 13` | Indication LED for WiFi connection status |

### Relay Logic Configuration

Depending on the type of relay module used (Active-High or Active-Low), you can modify the following macros in `Code_ESP32_RainMaker_4Relay_EEPROM.ino`:

```cpp
#define RELAY_ON   HIGH     // Set to LOW if your relays are Active-LOW
#define RELAY_OFF  LOW      // Set to HIGH if your relays are Active-LOW
```

---

## Provisioning Details

To connect the device to your local WiFi network:
1. Download and open the **ESP RainMaker** mobile app.
2. Power on the ESP32 module.
3. Scan the QR code or use the following manual BLE credentials:
   - **Service Name:** `PROV_12345`
   - **Proof of Possession (PoP):** `1234567`

---

## Development & Installation

### Prerequisites

1. Install [Arduino IDE](https://www.arduino.cc/en/software).
2. Install the ESP32 Board package (Tools > Board > Boards Manager > search for `esp32`).
3. Select **ESP32 Dev Module** as your target board.
4. Ensure the **ESP RainMaker** library is installed and updated.

### Setup Instructions

1. Clone or download this repository.
2. Open `Code_ESP32_RainMaker_4Relay_EEPROM.ino` in Arduino IDE.
3. Configure your relay logic (Active-High or Active-Low).
4. Connect the ESP32 to your computer.
5. Select the correct COM port and click **Upload**.
