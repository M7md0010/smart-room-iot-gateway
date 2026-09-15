/**********************************************************************************
 * ESP RainMaker - 4 Relays + Deep Sleep Switch + Activity Heartbeat
 * Upgraded with Preferences.h (Wear-leveling) & Cloud State Sync
 **********************************************************************************/

#include <Preferences.h>
#include <nvs_flash.h>
#include "RMaker.h"
#include "WiFi.h"
#include "WiFiProv.h"
#include "driver/rtc_io.h" // For RTC GPIO sleep control

// ================== CONFIG ==================
#define ENABLE_MEM          true

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

static uint8_t wifiLed   = 13;

// Switch GPIO (RTC-capable, wakes when grounded)
const gpio_num_t SWITCH_PIN = GPIO_NUM_27;

// ================== STATE ==================
bool toggleState_1 = false;
bool toggleState_2 = false;
bool toggleState_3 = false;
bool toggleState_4 = false;

// ================== ANTI-FLICKER ==================
unsigned long lastRelayChange[4] = {0, 0, 0, 0};
const unsigned long RELAY_GUARD_MS = 80;

// ================== SLEEP & SWITCH DEBOUNCE ==================
unsigned long switchHighStartTime = 0;
const unsigned long SLEEP_DEBOUNCE_MS = 150; 

// ================== OBJECTS ==================
Preferences prefs;
static Switch my_switch1(deviceName_1, &RelayPin1);
static Switch my_switch2(deviceName_2, &RelayPin2);
static Switch my_switch3(deviceName_3, &RelayPin3);
static Switch my_switch4(deviceName_4, &RelayPin4);

// ================== MEMORY (PREFERENCES) ==================
void saveState(const char* key, bool state) {
  if (ENABLE_MEM) prefs.putBool(key, state);
}

// ================== RELAY CONTROL ==================
void setRelay(uint8_t pin, const char* key, bool state) {
  digitalWrite(pin, state ? RELAY_ON : RELAY_OFF);
  saveState(key, state);
}

void setRelayBuffered(uint8_t index, uint8_t pin, const char* key, bool state) {
  unsigned long now = millis();
  if (now - lastRelayChange[index] < RELAY_GUARD_MS) return;
  lastRelayChange[index] = now;
  setRelay(pin, key, state);
}

// ================== PROVISION EVENTS ==================
void sysProvEvent(arduino_event_t *sys_event) {
  if (sys_event->event_id == ARDUINO_EVENT_PROV_START) {
    printQR(service_name, pop, "ble");
  }
}

// ================== RAINMAKER CALLBACK ==================
void write_callback(Device *device, Param *param, const param_val_t val, void*, write_ctx_t*) {
  if (strcmp(param->getParamName(), ESP_RMAKER_DEF_POWER_NAME) != 0) return;
  
  bool newState = val.val.b;
  const char* currentDeviceName = device->getDeviceName();

  if (!strcmp(currentDeviceName, deviceName_1)) {
    setRelayBuffered(0, RelayPin1, "r1", newState);
    toggleState_1 = newState;
  } else if (!strcmp(currentDeviceName, deviceName_2)) {
    setRelayBuffered(1, RelayPin2, "r2", newState);
    toggleState_2 = newState;
  } else if (!strcmp(currentDeviceName, deviceName_3)) {
    setRelayBuffered(2, RelayPin3, "r3", newState);
    toggleState_3 = newState;
  } else if (!strcmp(currentDeviceName, deviceName_4)) {
    setRelayBuffered(3, RelayPin4, "r4", newState);
    toggleState_4 = newState;
  }
}

// ================== SAFE SLEEP EXECUTION ==================
void enterDeepSleep() {
  Serial.println("[POWER] Switch OFF detected. Entering deep sleep...");

  // Force relays OFF
  digitalWrite(RelayPin1, RELAY_OFF);
  digitalWrite(RelayPin2, RELAY_OFF);
  digitalWrite(RelayPin3, RELAY_OFF);
  digitalWrite(RelayPin4, RELAY_OFF);
  digitalWrite(wifiLed, LOW);

  WiFi.disconnect(true);
  delay(50);

  // Enable RTC pull-up for safety, then arm the wake trigger
  rtc_gpio_pullup_en(SWITCH_PIN);
  esp_sleep_enable_ext0_wakeup(SWITCH_PIN, 0);

  Serial.flush();
  esp_deep_sleep_start();
}

// ================== ACTIVITY & HEARTBEAT LED ==================
void updateActivityLed() {
  unsigned long now = millis();
  static unsigned long lastBlinkTime = 0;
  static bool blinkState = false;
  wl_status_t status = WiFi.status();

  if (status == WL_CONNECTED) {
    static unsigned long lastPwmUpdate = 0;
    
    // Update the brightness only every 15 milliseconds (60 FPS)
    if (now - lastPwmUpdate >= 15) {
      lastPwmUpdate = now;
      // Smooth breathing effect capped at ~70% brightness (max 178)
      int brightness = (sin(now / 400.0) + 1) * 89; 
      analogWrite(wifiLed, brightness);
    }
  } else if (status == WL_IDLE_STATUS || status == WL_DISCONNECTED) {
    if (now - lastBlinkTime >= 500) {
      lastBlinkTime = now;
      blinkState = !blinkState;
      digitalWrite(wifiLed, blinkState ? HIGH : LOW);
    }
  } else {
    if (now - lastBlinkTime >= 100) {
      lastBlinkTime = now;
      blinkState = !blinkState;
      digitalWrite(wifiLed, blinkState ? HIGH : LOW);
    }
  }
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(SWITCH_PIN, INPUT_PULLUP);
  if (digitalRead(SWITCH_PIN) == HIGH) enterDeepSleep();

  if (ENABLE_MEM) prefs.begin("relays", false);

  // Boot-safe relay setup (Write HIGH before setting OUTPUT to prevent glitches)
  digitalWrite(RelayPin1, RELAY_OFF);
  digitalWrite(RelayPin2, RELAY_OFF);
  digitalWrite(RelayPin3, RELAY_OFF);
  digitalWrite(RelayPin4, RELAY_OFF);

  pinMode(RelayPin1, OUTPUT);
  pinMode(RelayPin2, OUTPUT);
  pinMode(RelayPin3, OUTPUT);
  pinMode(RelayPin4, OUTPUT);

  pinMode(wifiLed, OUTPUT);
  digitalWrite(wifiLed, LOW);

  // Restore states
  toggleState_1 = prefs.getBool("r1", false);
  toggleState_2 = prefs.getBool("r2", false);
  toggleState_3 = prefs.getBool("r3", false);
  toggleState_4 = prefs.getBool("r4", false);

  setRelay(RelayPin1, "r1", toggleState_1);
  setRelay(RelayPin2, "r2", toggleState_2);
  setRelay(RelayPin3, "r3", toggleState_3);
  setRelay(RelayPin4, "r4", toggleState_4);

  // Initialize RainMaker
  Node my_node = RMaker.initNode("ESP32_Relay_4");

  my_switch1.addCb(write_callback);
  my_switch2.addCb(write_callback);
  my_switch3.addCb(write_callback);
  my_switch4.addCb(write_callback);

  my_node.addDevice(my_switch1);
  my_node.addDevice(my_switch2);
  my_node.addDevice(my_switch3);
  my_node.addDevice(my_switch4);

  // SYNC physical state to cloud dashboard on boot
  my_switch1.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_1);
  my_switch2.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_2);
  my_switch3.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_3);
  my_switch4.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_4);

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
  if (digitalRead(SWITCH_PIN) == HIGH) {
    if (switchHighStartTime == 0) {
      switchHighStartTime = millis();
    } else if (millis() - switchHighStartTime >= SLEEP_DEBOUNCE_MS) {
      enterDeepSleep();
    }
  } else {
    switchHighStartTime = 0;
  }

  updateActivityLed();
}