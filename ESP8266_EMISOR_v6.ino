/*
  ============================================================
  ESP8266 EMISOR V5 - ESP-NOW BROADCAST
  ============================================================

  NUEVO ARMADO:
  - ESP8266 = EMISOR
  - ESP32   = RECEPTOR + WEB

  Qué hace:
  - Lee botones digitales del ESP8266.
  - Lee un potenciómetro en A0.
  - Envía todo por ESP-NOW en BROADCAST.

  Importante:
  - En ESP8266 el analógico A0 lee 0 a 1023.
  - El receptor ESP32 después lo adapta a PWM 0 a 255.
*/

#include <ESP8266WiFi.h>
#include <espnow.h>

extern "C" {
  #include <user_interface.h>
}

#define ESPNOW_CHANNEL 1

#define PIN_DIGITAL 0
#define PIN_ANALOGICO 1

// ============================================================
// ENTRADAS DEL EMISOR ESP8266
// ============================================================

#define NUM_DIGITAL_INPUTS 3
#define NUM_ANALOG_INPUTS 1

// Botones entre GPIO y GND. Usamos INPUT_PULLUP.
// D1=GPIO5, D2=GPIO4, D5=GPIO14.
#define INPUT_DIGITAL_1 5
#define INPUT_DIGITAL_2 4
#define INPUT_DIGITAL_3 14

const uint8_t digitalInputs[NUM_DIGITAL_INPUTS] = {
  INPUT_DIGITAL_1,
  INPUT_DIGITAL_2,
  INPUT_DIGITAL_3
};

// Potenciómetro en A0.
const uint8_t analogInputs[NUM_ANALOG_INPUTS] = { A0 };

// ============================================================
// PAQUETE ESPNOW
// ============================================================

struct EspNowMsg {
  uint8_t inputType;     // 0 digital, 1 analógico
  uint8_t inputPin;      // GPIO de entrada. Para A0 mandamos 0.
  bool digitalValue;     // true / false
  uint16_t analogValue;  // ESP8266: 0 a 1023
};

// Broadcast = todos los receptores.
uint8_t broadcastMac[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

bool lastDigital[NUM_DIGITAL_INPUTS];
unsigned long lastChange[NUM_DIGITAL_INPUTS];
uint16_t lastAnalog[NUM_ANALOG_INPUTS];

const unsigned long debounceMs = 70;
const int analogThreshold = 5; // 0-1023: chico para que el pote responda fluido.

// ============================================================
// ENVÍO
// ============================================================

void enviarMensaje(EspNowMsg msg) {
  uint8_t result = esp_now_send(broadcastMac, (uint8_t *)&msg, sizeof(msg));

  Serial.print("ENVIO | pin ");
  Serial.print(msg.inputPin);
  Serial.print(" | tipo ");
  Serial.print(msg.inputType == PIN_DIGITAL ? "DIGITAL" : "ANALOGICO");
  Serial.print(" | digital ");
  Serial.print(msg.digitalValue);
  Serial.print(" | analog ");
  Serial.print(msg.analogValue);
  Serial.print(" | resultado ");
  Serial.println(result == 0 ? "OK" : "ERROR");
}

void setup() {
  Serial.begin(115200);
  delay(200);

  for (int i = 0; i < NUM_DIGITAL_INPUTS; i++) {
    pinMode(digitalInputs[i], INPUT_PULLUP);
    lastDigital[i] = digitalRead(digitalInputs[i]) == LOW;
    lastChange[i] = 0;
  }

  for (int i = 0; i < NUM_ANALOG_INPUTS; i++) {
    lastAnalog[i] = analogRead(analogInputs[i]);
  }

  WiFi.mode(WIFI_STA);
  wifi_set_channel(ESPNOW_CHANNEL);

  Serial.println();
  Serial.println("====================================");
  Serial.println("ESP8266 EMISOR V5 LISTO");
  Serial.print("MAC ESP8266 EMISOR: ");
  Serial.println(WiFi.macAddress());
  Serial.println("Canal ESP-NOW: 1");
  Serial.println("====================================");

  if (esp_now_init() != 0) {
    Serial.println("ERROR: no se pudo iniciar ESP-NOW");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_add_peer(broadcastMac, ESP_NOW_ROLE_SLAVE, ESPNOW_CHANNEL, NULL, 0);
}

void loop() {
  // Botones digitales.
  for (int i = 0; i < NUM_DIGITAL_INPUTS; i++) {
    bool actual = digitalRead(digitalInputs[i]) == LOW;

    if (actual != lastDigital[i]) {
      if (millis() - lastChange[i] > debounceMs) {
        lastDigital[i] = actual;
        lastChange[i] = millis();

        EspNowMsg msg;
        msg.inputType = PIN_DIGITAL;
        msg.inputPin = digitalInputs[i];
        msg.digitalValue = actual;
        msg.analogValue = actual ? 1023 : 0;

        enviarMensaje(msg);
      }
    }
  }

  // Potenciómetro A0.
  for (int i = 0; i < NUM_ANALOG_INPUTS; i++) {
    uint16_t actual = analogRead(analogInputs[i]); // 0 a 1023

    if (abs((int)actual - (int)lastAnalog[i]) > analogThreshold) {
      lastAnalog[i] = actual;

      if (actual < 20) actual = 0;
      if (actual > 1000) actual = 1023;

      EspNowMsg msg;
      msg.inputType = PIN_ANALOGICO;
      msg.inputPin = 0; // A0 / ADC0 en la web.
      msg.digitalValue = actual > 512;
      msg.analogValue = actual;

      enviarMensaje(msg);
    }
  }

  delay(20);
}
