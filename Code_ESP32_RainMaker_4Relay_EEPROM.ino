/**********************************************************************************
 * ESP RainMaker - 4 Relays
 * Inverted logic + software buffer (anti-flicker)
 **********************************************************************************/

#include <EEPROM.h>
#include <nvs_flash.h> // <-- Added the correct memory wipe library
#include "RMaker.h"
#include "WiFi.h"
#include "WiFiProv.h"

// ================== CONFIG ==================
#define ENABLE_EEPROM       true
#define EEPROM_SIZE         10

// Logic inverted: Active-LOW relays turn ON when the pin is pulled LOW
#define RELAY_ON    LOW    
#define RELAY_OFF   HIGH

const char *service_name = "PROV_12345";
const char *pop = "1234567";

// ================== DEVICE NAMES ==================
char deviceName_1[] = "Lights in";
char deviceName_2[] = "Lights out";
char deviceName_3[] = "White LED";
char deviceName_4[] = "Yellow LED";

// ================== GPIO ==================
static uint8_t RelayPin1 = 26;
static uint8_t RelayPin2 = 25;
static uint8_t RelayPin3 = 33;
static uint8_t RelayPin4 = 32;

static uint8_t wifiLed    = 13;

// ================== STATE ==================
bool toggleState_1 = false;
bool toggleState_2 = false;
bool toggleState_3 = false;
bool toggleState_4 = false;

// ================== ANTI-FLICKER ==================
unsigned long lastRelayChange[4] = {0, 0, 0, 0};
const unsigned long RELAY_GUARD_MS = 80;

// ================== RAINMAKER VIRTUAL DEVICES ==================
static Switch my_switch1(deviceName_1, &RelayPin1);
static Switch my_switch2(deviceName_2, &RelayPin2);
static Switch my_switch3(deviceName_3, &RelayPin3);
static Switch my_switch4(deviceName_4, &RelayPin4);

// ================== EEPROM ==================
void writeEEPROM(int addr, bool state) {
  if (!ENABLE_EEPROM) return;
  EEPROM.write(addr, state);
  EEPROM.commit();
}

bool readEEPROM(int addr) {
  if (!ENABLE_EEPROM) return false;
  return EEPROM.read(addr);
}

// ================== RELAY CONTROL ==================
void setRelay(uint8_t pin, int addr, bool state) {
  digitalWrite(pin, state ? RELAY_ON : RELAY_OFF);
  if (ENABLE_EEPROM) writeEEPROM(addr, state);
}

void setRelayBuffered(uint8_t index, uint8_t pin, int addr, bool state) {
  unsigned long now = millis();
  if (now - lastRelayChange[index] < RELAY_GUARD_MS) return;
  lastRelayChange[index] = now;
  setRelay(pin, addr, state);
}

// ================== PROVISION EVENTS ==================
void sysProvEvent(arduino_event_t *sys_event) {
  if (sys_event->event_id == ARDUINO_EVENT_PROV_START) {
    printQR(service_name, pop, "ble");
  }
}

// ================== RAINMAKER CALLBACK ==================
void write_callback(Device *device, Param *param,
                    const param_val_t val, void*, write_ctx_t*) {
  if (strcmp(param->getParamName(), ESP_RMAKER_DEF_POWER_NAME) != 0) return;
  
  bool newState = val.val.b;
  const char* currentDeviceName = device->getDeviceName();

  if (!strcmp(currentDeviceName, deviceName_1)) {
    setRelayBuffered(0, RelayPin1, 0, newState);
    toggleState_1 = newState;
  } else if (!strcmp(currentDeviceName, deviceName_2)) {
    setRelayBuffered(1, RelayPin2, 1, newState);
    toggleState_2 = newState;
  } else if (!strcmp(currentDeviceName, deviceName_3)) {
    setRelayBuffered(2, RelayPin3, 2, newState);
    toggleState_3 = newState;
  } else if (!strcmp(currentDeviceName, deviceName_4)) {
    setRelayBuffered(3, RelayPin4, 3, newState);
    toggleState_4 = newState;
  }
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);
  delay(1000); // Give the serial monitor a second to connect
  Serial.println("\n--- ESP32 IS ALIVE ---");

  // --- HARDWARE MEMORY WIPE (NVS) ---
  /*
  nvs_flash_erase();
  nvs_flash_init();
  Serial.println("Memory wiped. Ready for new Wi-Fi credentials.");
  */
  // ----------------------------------

  digitalWrite(14, 1);
  
  if (ENABLE_EEPROM) EEPROM.begin(EEPROM_SIZE);

  // Boot-safe OFF (Since logic is inverted, RELAY_OFF will safely pull these HIGH)
  digitalWrite(RelayPin1, RELAY_OFF);
  digitalWrite(RelayPin2, RELAY_OFF);
  digitalWrite(RelayPin3, RELAY_OFF);
  digitalWrite(RelayPin4, RELAY_OFF);

  pinMode(RelayPin1, OUTPUT);
  pinMode(RelayPin2, OUTPUT);
  pinMode(RelayPin3, OUTPUT);
  pinMode(RelayPin4, OUTPUT);

  pinMode(wifiLed, OUTPUT);

  // Restore states from EEPROM
  toggleState_1 = readEEPROM(0);
  toggleState_2 = readEEPROM(1);
  toggleState_3 = readEEPROM(2);
  toggleState_4 = readEEPROM(3);

  setRelay(RelayPin1, 0, toggleState_1);
  setRelay(RelayPin2, 1, toggleState_2);
  setRelay(RelayPin3, 2, toggleState_3);
  setRelay(RelayPin4, 3, toggleState_4);

  // Initialize RainMaker Node
  Node my_node = RMaker.initNode("ESP32_Relay_4");

  my_switch1.addCb(write_callback);
  my_switch2.addCb(write_callback);
  my_switch3.addCb(write_callback);
  my_switch4.addCb(write_callback);

  my_node.addDevice(my_switch1);
  my_node.addDevice(my_switch2);
  my_node.addDevice(my_switch3);
  my_node.addDevice(my_switch4);

  // <-- Changed to TOPICS so the cloud OTA dashboard actually works
  RMaker.enableOTA(OTA_USING_TOPICS); 
  RMaker.enableTZService();
  RMaker.enableSchedule();
  RMaker.start();

  WiFi.onEvent(sysProvEvent);
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE,
                          WIFI_PROV_SCHEME_HANDLER_FREE_BTDM,
                          WIFI_PROV_SECURITY_1,
                          pop, service_name);
}

// ================== LOOP ==================
void loop() {
  // Purely handling WiFi LED status now, RainMaker runs asynchronously 
  digitalWrite(wifiLed, WiFi.status() == WL_CONNECTED);
}