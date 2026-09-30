/*
  ============================================================
  ESP32 RECEPTOR V6 - WEB + ESP-NOW
  ============================================================

  NUEVO ARMADO:
  - ESP8266 = EMISOR
  - ESP32   = RECEPTOR + WEB

  Cambios pedidos:
  1) Salida sin entrada: cada card puede no tener entrada física.
     En ese caso se controla solo desde la web/celular.
  2) Test ON/OFF más claro: botones con estado visual, texto de feedback.
  3) Potenciómetro: entrada ESP8266 0-1023, salida PWM ESP32 0-255.
  4) Nueva card vacía: no viene predeterminada.
  5) Máximo 8 cards reales.
  6) Eliminar card sin trabar la web.
*/

#include <WiFi.h>
#include <WebServer.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <EEPROM.h>

#define EEPROM_SIZE 4096
#define MAX_CARDS 8
#define ESPNOW_CHANNEL 1
#define WIFI_PASS "12345678"
#define CONFIG_MAGIC 20260704

#define PIN_DIGITAL 0
#define PIN_ANALOGICO 1

#define MODO_LLAVE 0
#define MODO_PULSADOR 1

WebServer server(80);

struct EspNowMsg {
  uint8_t inputType;
  uint8_t inputPin;
  bool digitalValue;
  uint16_t analogValue; // desde ESP8266: 0 a 1023
};

struct CardConfig {
  bool active;
  bool hasInput;          // false = solo web/celular
  char nombre[28];
  uint8_t inputType;
  uint8_t inputPin;
  uint8_t outputType;     // 0 digital, 1 PWM
  uint8_t outputPin;
  uint8_t modo;
  bool invertir;
  unsigned long retencion;
};

struct Config {
  uint32_t magic;
  bool filtroMacActivo;
  char macPermitida[18];
  char placaEmisora[10];
  char placaReceptora[10];
  CardConfig cards[MAX_CARDS];
};

Config config;

bool estadoDigital[MAX_CARDS];
uint16_t estadoAnalogico[MAX_CARDS]; // 0 a 255
unsigned long tiempoApagado[MAX_CARDS];

// ============================================================
// EEPROM
// ============================================================

void guardarConfig() {
  EEPROM.put(0, config);
  EEPROM.commit();
  Serial.println("CONFIGURACION GUARDADA");
}

void limpiarCard(int i) {
  config.cards[i].active = false;
  config.cards[i].hasInput = false;
  strcpy(config.cards[i].nombre, "");
  config.cards[i].inputType = PIN_DIGITAL;
  config.cards[i].inputPin = 255;
  config.cards[i].outputType = PIN_DIGITAL;
  config.cards[i].outputPin = 255;
  config.cards[i].modo = MODO_LLAVE;
  config.cards[i].invertir = false;
  config.cards[i].retencion = 1000;

  estadoDigital[i] = false;
  estadoAnalogico[i] = 0;
  tiempoApagado[i] = 0;
}

void cargarDefault() {
  config.magic = CONFIG_MAGIC;
  config.filtroMacActivo = false;
  strcpy(config.macPermitida, "");
  strcpy(config.placaEmisora, "esp8266");
  strcpy(config.placaReceptora, "esp32");

  for (int i = 0; i < MAX_CARDS; i++) limpiarCard(i);

  // Una card inicial útil, pero las nuevas se crean vacías.
  config.cards[0].active = true;
  config.cards[0].hasInput = true;
  strcpy(config.cards[0].nombre, "Salida 1");
  config.cards[0].inputType = PIN_DIGITAL;
  config.cards[0].inputPin = 5;      // D1 del ESP8266
  config.cards[0].outputType = PIN_DIGITAL;
  config.cards[0].outputPin = 25;    // GPIO seguro ESP32
  config.cards[0].modo = MODO_LLAVE;
  config.cards[0].invertir = false;
  config.cards[0].retencion = 1000;

  guardarConfig();
}

void cargarConfig() {
  EEPROM.get(0, config);

  if (config.magic != CONFIG_MAGIC) {
    cargarDefault();
    return;
  }

  strcpy(config.placaEmisora, "esp8266");
  strcpy(config.placaReceptora, "esp32");
}

// ============================================================
// WIFI
// ============================================================

String getWifiName() {
  // Uso la MAC real del chip, no la MAC todavía no inicializada del WiFi.
  // Así evitamos que aparezca ESP-REMOTE-0000.
  uint64_t chipid = ESP.getEfuseMac();
  char ultimos4[5];
  sprintf(ultimos4, "%04X", (uint16_t)(chipid & 0xFFFF));
  return String("ESP-REMOTE-") + String(ultimos4);
}

bool pinValido(uint8_t pin) {
  return pin != 255;
}

// ============================================================
// SALIDAS
// ============================================================

void prepararSalida(uint8_t pin) {
  if (!pinValido(pin)) return;
  pinMode(pin, OUTPUT);
}

void setSalidaDigital(int index, bool encender) {
  CardConfig &c = config.cards[index];
  if (!pinValido(c.outputPin)) return;

  bool valorFinal = c.invertir ? !encender : encender;
  digitalWrite(c.outputPin, valorFinal ? HIGH : LOW);

  estadoDigital[index] = encender;
  estadoAnalogico[index] = encender ? 255 : 0;

  Serial.print("SALIDA DIGITAL | card ");
  Serial.print(index);
  Serial.print(" | GPIO ");
  Serial.print(c.outputPin);
  Serial.print(" | estado ");
  Serial.println(encender ? "ON" : "OFF");
}

void setSalidaAnalogica(int index, int value) {
  CardConfig &c = config.cards[index];
  if (!pinValido(c.outputPin)) return;

  value = constrain(value, 0, 255);
  // Deadband para que el PWM no quede apenas prendido cerca de cero.
  if (value < 5) value = 0;
  if (value > 250) value = 255;
  int valorFinal = c.invertir ? 255 - value : value;

  // ESP32 Arduino soporta analogWrite con resolución por defecto de 8 bits.
  analogWrite(c.outputPin, valorFinal);

  estadoAnalogico[index] = value;
  estadoDigital[index] = value > 0;

  Serial.print("SALIDA PWM | card ");
  Serial.print(index);
  Serial.print(" | GPIO ");
  Serial.print(c.outputPin);
  Serial.print(" | valor ");
  Serial.println(value);
}

void apagarCard(int i) {
  if (!config.cards[i].active) return;
  if (config.cards[i].outputType == PIN_DIGITAL) setSalidaDigital(i, false);
  else setSalidaAnalogica(i, 0);
}

void aplicarDesdeMensaje(int index, EspNowMsg msg) {
  CardConfig &c = config.cards[index];
  if (!c.hasInput) return;

  if (c.outputType == PIN_DIGITAL) {
    bool valorDigital = msg.digitalValue;
    if (msg.inputType == PIN_ANALOGICO) valorDigital = msg.analogValue > 512;

    if (c.modo == MODO_PULSADOR) {
      if (valorDigital) {
        setSalidaDigital(index, true);
        tiempoApagado[index] = 0;
      } else {
        tiempoApagado[index] = millis() + c.retencion;
      }
    } else {
      setSalidaDigital(index, valorDigital);
    }
  } else {
    int pwmValue = 0;
    if (msg.inputType == PIN_ANALOGICO) {
      uint16_t raw = msg.analogValue;
      if (raw < 20) pwmValue = 0;
      else if (raw > 1000) pwmValue = 255;
      else pwmValue = map(raw, 20, 1000, 0, 255);
    } else {
      pwmValue = msg.digitalValue ? 255 : 0;
    }
    setSalidaAnalogica(index, pwmValue);
  }
}

// ============================================================
// FILTRO MAC
// ============================================================

bool macEstaPermitida(const uint8_t *mac) {
  if (!config.filtroMacActivo) return true;

  String macConfig = String(config.macPermitida);
  macConfig.trim();
  macConfig.toUpperCase();
  if (macConfig.length() == 0) return true;

  char macStr[18];
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  String recibida = String(macStr);
  recibida.toUpperCase();
  return recibida == macConfig;
}

void procesarMensaje(const uint8_t *mac, const uint8_t *incomingData, int len) {
  if (len != sizeof(EspNowMsg)) return;

  EspNowMsg msg;
  memcpy(&msg, incomingData, sizeof(msg));

  if (!macEstaPermitida(mac)) {
    Serial.println("MENSAJE RECHAZADO POR FILTRO MAC");
    return;
  }

  Serial.print("RECIBIDO | input GPIO ");
  Serial.print(msg.inputPin);
  Serial.print(" | tipo ");
  Serial.print(msg.inputType == PIN_DIGITAL ? "DIGITAL" : "ANALOGICO");
  Serial.print(" | digital ");
  Serial.print(msg.digitalValue);
  Serial.print(" | analog ");
  Serial.println(msg.analogValue);

  for (int i = 0; i < MAX_CARDS; i++) {
    CardConfig &c = config.cards[i];
    if (!c.active || !c.hasInput) continue;
    if (c.inputPin == msg.inputPin && c.inputType == msg.inputType) aplicarDesdeMensaje(i, msg);
  }
}

#if ESP_ARDUINO_VERSION_MAJOR >= 3
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  procesarMensaje(info->src_addr, incomingData, len);
}
#else
void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  procesarMensaje(mac, incomingData, len);
}
#endif

// ============================================================
// HTML HELPERS
// ============================================================

String checked(bool v) { return v ? "checked" : ""; }
String selectedInt(int a, int b) { return a == b ? "selected" : ""; }

String nombreVisible(int i) {
  String n = String(config.cards[i].nombre);
  n.trim();
  if (n.length() == 0) return "Card " + String(i + 1);
  return n;
}

String estadoTexto(int i) {
  if (!config.cards[i].active) return "Sin configurar";
  if (!pinValido(config.cards[i].outputPin)) return "Elegí salida";

  if (config.cards[i].outputType == PIN_ANALOGICO) {
    if (estadoAnalogico[i] == 0) return "Apagada";
    return String("PWM ") + String(estadoAnalogico[i]);
  }
  return estadoDigital[i] ? "Encendida" : "Apagada";
}

String htmlHead(String title) {
  String html = "";
  html += "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>" + title + "</title>";
  html += "<style>";
  html += "*{box-sizing:border-box;}body{margin:0;background:#0b1113;color:#eef5f7;font-family:Arial,Helvetica,sans-serif;}";
  html += ".wrap{max-width:980px;margin:auto;padding:10px;}h1{font-size:22px;margin:8px 0 12px;text-align:center;}";
  html += ".dashTop{display:flex;justify-content:space-between;align-items:center;margin-bottom:14px;gap:12px;flex-wrap:wrap;}";
  html += ".dashTitle{font-size:20px;font-weight:bold;}";
  html += ".configBtn{background:#151b1f;border:1px solid #4b5563;color:white;border-radius:20px;padding:14px 22px;font-size:16px;font-weight:bold;text-decoration:none;display:inline-flex;}";
  html += ".grid{display:grid;grid-template-columns:repeat(2,1fr);gap:8px;}@media(min-width:760px){.grid{grid-template-columns:repeat(4,1fr);}}";
  html += ".mini{background:#24292c;border:1px solid #3b4449;border-radius:18px;padding:10px;min-height:72px;display:flex;align-items:center;gap:9px;color:white;text-decoration:none;cursor:pointer;transition:.15s ease;user-select:none;touch-action:manipulation;}";
  html += ".mini:hover{transform:translateY(-1px);border-color:#93c5fd;} .mini.on{background:#27343a;}";
  html += ".dot{width:18px;height:18px;border-radius:50%;background:#6b7280;flex:0 0 18px;box-shadow:0 0 0 3px rgba(255,255,255,.04);}.mini.on .dot{background:#22c55e;box-shadow:0 0 12px rgba(34,197,94,.75);}";
  html += ".miniText{flex:1;min-width:0;}.name{font-size:14px;font-weight:bold;line-height:1.15;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;}.state{font-size:12px;font-weight:bold;color:#cfd8dc;margin-top:3px;}.muted{color:#8c969b;}";
  html += ".panel,.card{background:#151b1f;border:1px solid #30393e;border-radius:15px;padding:9px;margin-bottom:9px;}";
  html += ".row{display:grid;grid-template-columns:repeat(2,1fr);gap:8px;margin-bottom:7px;}@media(min-width:850px){.row{grid-template-columns:repeat(4,1fr);}}@media(max-width:600px){.row{grid-template-columns:1fr 1fr;}}";
  html += "label{display:block;font-size:11px;color:#aab4b9;margin-bottom:4px;}input,select{width:100%;height:36px;border-radius:10px;border:1px solid #3e4a50;background:#0d1215;color:white;padding:0 8px;font-size:13px;}";
  html += ".checkRow{display:flex;align-items:center;justify-content:space-between;background:#0d1215;border-radius:10px;padding:8px 10px;margin-bottom:8px;font-size:13px;}.checkRow input{width:20px;height:20px;}";
  html += ".btn{height:38px;border:0;border-radius:10px;font-weight:bold;font-size:13px;color:white;background:#2563eb;padding:0 12px;text-decoration:none;display:inline-flex;align-items:center;justify-content:center;cursor:pointer;transition:.12s ease;}";
  html += ".btn:hover{filter:brightness(1.2);transform:translateY(-1px);} .btn.gray{background:#374151;}.btn.red{background:#dc2626;}.btn.green{background:#22c55e;}.btn.full{width:100%;height:48px;font-size:16px;margin-top:8px;}";
  html += ".actions{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-top:8px;}.hidden{display:none!important;}.cardTitle{display:grid;grid-template-columns:1fr auto;gap:8px;margin-bottom:10px;}";
  html += ".warn{background:#2b2114;color:#ffd38a;border:1px solid #77551b;padding:9px;border-radius:12px;margin-bottom:10px;font-size:12px;}";
  html += ".rangeBox{margin-top:8px;}.headerRow{grid-template-columns:1fr;}.macStack{display:grid;grid-template-columns:1fr;gap:8px;}.boardStack{display:grid;grid-template-columns:1fr 1fr;gap:8px;}@media(min-width:760px){.headerRow{grid-template-columns:1.15fr 1fr;align-items:start;}}";
  html += ".testOnly{font-size:12px;color:#aab4b9;margin-top:8px;margin-bottom:6px;text-align:center;font-weight:bold;}.feedback{font-size:12px;text-align:center;color:#93c5fd;margin-top:6px;min-height:16px;}";
  html += ".manualPin{display:none;margin-top:6px;}.rangeMini{width:100%;margin-top:8px;accent-color:#22c55e;}.mini.pwm{display:block;}.miniInner{display:flex;align-items:center;gap:9px;}";
  html += "</style></head><body><div class='wrap'>";
  return html;
}

String htmlEnd() { return "</div></body></html>"; }

// ============================================================
// DASHBOARD
// ============================================================

void handleHome() {
  String html = htmlHead("ESP Remote");
  html += "<div class='dashTop'><div class='dashTitle'>ESP Remote</div><a class='configBtn' href='/config'>Configuración</a></div>";
  html += "<div class='grid'>";

  bool hayCards = false;
  for (int i = 0; i < MAX_CARDS; i++) {
    CardConfig &c = config.cards[i];
    if (!c.active) continue;
    hayCards = true;

    bool activo = c.outputType == PIN_DIGITAL ? estadoDigital[i] : estadoAnalogico[i] > 0;
    String clase = activo ? "mini on" : "mini";

    if (c.outputType == PIN_ANALOGICO && !c.hasInput) {
      html += "<div class='" + clase + " pwm' id='dashCard" + String(i) + "'>";
      html += "<div class='miniInner'><div class='dot'></div><div class='miniText'>";
      html += "<div class='name'>" + nombreVisible(i) + "</div>";
      html += "<div class='state' id='dashState" + String(i) + "'>" + estadoTexto(i) + "</div>";
      html += "</div></div>";
      html += "<input class='rangeMini' type='range' min='0' max='255' value='" + String(estadoAnalogico[i]) + "' oninput='setPwm(" + String(i) + ",this.value)'>";
      html += "</div>";
    } else if (c.outputType == PIN_DIGITAL && !c.hasInput && c.modo == MODO_PULSADOR) {
      html += "<div class='" + clase + "' id='dashCard" + String(i) + "' onpointerdown='pulseStart(" + String(i) + ")' onpointerup='pulseRelease(" + String(i) + ")' onpointerleave='pulseCancel(" + String(i) + ")' onpointercancel='pulseCancel(" + String(i) + ")'>";
      html += "<div class='dot'></div><div class='miniText'>";
      html += "<div class='name'>" + nombreVisible(i) + "</div>";
      html += "<div class='state' id='dashState" + String(i) + "'>" + estadoTexto(i) + "</div>";
      html += "</div></div>";
    } else {
      html += "<div class='" + clase + "' id='dashCard" + String(i) + "' onclick='toggleCard(" + String(i) + ")'>";
      html += "<div class='dot'></div><div class='miniText'>";
      html += "<div class='name'>" + nombreVisible(i) + "</div>";
      html += "<div class='state' id='dashState" + String(i) + "'>" + estadoTexto(i) + "</div>";
      html += "</div></div>";
    }
  }

  if (!hayCards) {
    html += "<a class='mini muted' href='/config'><div class='dot'></div><div><div class='name'>Sin cards</div><div class='state'>Agregar configuración</div></div></a>";
  }

  html += "</div>";
  html += "<script>";
  html += "function applyState(id,t){let card=document.getElementById('dashCard'+id);let state=document.getElementById('dashState'+id);if(!card||!state)return;let activo=t.includes('Encendida')||t.includes('PWM ');card.classList.toggle('on',activo);state.innerHTML=t;}";
  html += "function toggleCard(id){fetch('/toggle?id='+id).then(r=>r.text()).then(t=>applyState(id,t));}";
  html += "function setPwm(id,v){fetch('/testAnalog?id='+id+'&v='+v).then(r=>r.text()).then(t=>applyState(id,t));}";
  html += "let pulseHolding={};function pulseStart(id){pulseHolding[id]=true;let state=document.getElementById('dashState'+id);if(state)state.innerHTML='Soltá para activar';}function pulseCancel(id){pulseHolding[id]=false;}function pulseRelease(id){if(!pulseHolding[id])return;pulseHolding[id]=false;fetch('/pulseRelease?id='+id).then(r=>r.text()).then(t=>applyState(id,t));}";
  html += "</script>";
  html += htmlEnd();
  server.send(200, "text/html", html);
}

// ============================================================
// CONFIG HTML
// ============================================================

String cardConfigHtml(int i) {
  CardConfig &c = config.cards[i];
  String display = c.active ? "" : "hidden";
  String html = "";

  html += "<div class='card " + display + "' id='card" + String(i) + "'>";
  html += "<input type='hidden' id='active" + String(i) + "' name='active" + String(i) + "' value='" + String(c.active ? 1 : 0) + "'>";

  html += "<div class='cardTitle'>";
  html += "<input name='nombre" + String(i) + "' id='nombre" + String(i) + "' placeholder='Nombre de la salida' value='" + String(c.nombre) + "'>";
  html += "<button type='button' class='btn red' onclick='deleteCard(" + String(i) + ")'>Eliminar</button>";
  html += "</div>";

  html += "<div class='checkRow'><span>Usar entrada física ESP-NOW</span><input type='checkbox' id='hasInput" + String(i) + "' name='hasInput" + String(i) + "' onchange='refreshCard(" + String(i) + ")' " + checked(c.hasInput) + "></div>";

  html += "<div class='row' id='inputBox" + String(i) + "'>";
  html += "<div><label>Tipo de entrada</label><select name='inputType" + String(i) + "' id='inputType" + String(i) + "' onchange='refreshCard(" + String(i) + ")'>";
  html += "<option value='0' " + selectedInt(c.inputType, PIN_DIGITAL) + ">Digital</option><option value='1' " + selectedInt(c.inputType, PIN_ANALOGICO) + ">Analógica</option></select></div>";
  html += "<div><label>Pin entrada ESP8266</label><input type='hidden' name='inputPin" + String(i) + "' id='inputPin" + String(i) + "' value='" + String(c.inputPin) + "'><select id='inputPinSelect" + String(i) + "' onchange='changePin(" + String(i) + ",\"input\")'></select><input class='manualPin' type='number' id='inputPinManual" + String(i) + "' value='" + String(c.inputPin) + "' oninput='manualPin(" + String(i) + ",\"input\")'></div>";
  html += "</div>";

  html += "<div class='row'>";
  html += "<div><label>Tipo de salida</label><select name='outputType" + String(i) + "' id='outputType" + String(i) + "' onchange='refreshCard(" + String(i) + ")'>";
  html += "<option value='0' " + selectedInt(c.outputType, PIN_DIGITAL) + ">Digital</option><option value='1' " + selectedInt(c.outputType, PIN_ANALOGICO) + ">PWM</option></select></div>";
  html += "<div><label>Pin salida ESP32</label><input type='hidden' name='outputPin" + String(i) + "' id='outputPin" + String(i) + "' value='" + String(c.outputPin) + "'><select id='outputPinSelect" + String(i) + "' onchange='changePin(" + String(i) + ",\"output\")'></select><input class='manualPin' type='number' id='outputPinManual" + String(i) + "' value='" + String(c.outputPin) + "' oninput='manualPin(" + String(i) + ",\"output\")'></div>";
  html += "<div id='digitalBox" + String(i) + "'><label>Modo digital</label><select name='modo" + String(i) + "'><option value='0' " + selectedInt(c.modo, MODO_LLAVE) + ">Llave</option><option value='1' " + selectedInt(c.modo, MODO_PULSADOR) + ">Pulsador</option></select></div>";
  html += "<div id='retBox" + String(i) + "'><label>Retención ms</label><input type='number' name='retencion" + String(i) + "' value='" + String(c.retencion) + "'></div>";
  html += "</div>";

  html += "<div class='checkRow'><span>Invertir lógica</span><input type='checkbox' name='invertir" + String(i) + "' " + checked(c.invertir) + "></div>";

  html += "<div class='testOnly'>Test de salida</div>";
  html += "<div class='actions' id='digitalTest" + String(i) + "'><button type='button' class='btn green holdBtn' onpointerdown='testPulseStart(" + String(i) + ")' onpointerup='testPulseRelease(" + String(i) + ")' onpointerleave='testPulseCancel(" + String(i) + ")' onpointercancel='testPulseCancel(" + String(i) + ")'>Pulsar y soltar</button><button type='button' class='btn gray' onclick='testDigital(" + String(i) + ",0)'>OFF</button></div>";
  html += "<div class='rangeBox' id='pwmTest" + String(i) + "'><label>Test PWM 0-255</label><input type='range' min='0' max='255' value='" + String(estadoAnalogico[i]) + "' oninput='testAnalog(" + String(i) + ",this.value)'></div>";
  html += "<div class='feedback' id='feedback" + String(i) + "'></div>";
  html += "</div>";
  return html;
}

void handleConfig() {
  String html = htmlHead("Configuracion ESP Remote");
  html += "<h1>Configuración</h1><div class='dashTop'><a class='configBtn' href='/'>Dashboard</a></div>";
  html += "<div class='warn'>Nuevo armado: ESP8266 emisor y ESP32 receptor. Podés crear salidas sin entrada para manejarlas solo desde el celular.</div>";
  html += "<form action='/save' method='POST'>";

  html += "<div class='panel'><div class='row headerRow'><div class='macStack'>";
  html += "<div><label>Filtro por MAC del emisor</label><select name='filtroMacActivo'><option value='0' " + selectedInt(config.filtroMacActivo, 0) + ">Sin filtro</option><option value='1' " + selectedInt(config.filtroMacActivo, 1) + ">Con filtro</option></select></div>";
  html += "<div><label>MAC permitida ESP8266</label><input name='macPermitida' placeholder='AA:BB:CC:DD:EE:FF' value='" + String(config.macPermitida) + "'></div>";
  html += "</div><div class='boardStack'>";
  html += "<div><label>Placa emisora</label><input name='placaEmisora' value='esp8266' readonly></div>";
  html += "<div><label>Placa receptora</label><input name='placaReceptora' value='esp32' readonly></div>";
  html += "</div></div></div>";

  html += "<div id='cards'>";
  for (int i = 0; i < MAX_CARDS; i++) html += cardConfigHtml(i);
  html += "</div>";

  html += "<button type='button' class='btn full' onclick='addCard()'>+ Agregar card</button>";
  html += "<button type='submit' class='btn green full'>Guardar configuración</button>";
  html += "</form>";

  html += "<script>";
  html += "const MAX_CARDS=" + String(MAX_CARDS) + ";";
  html += "const pinMap={inputDigital:[5,4,14,12,13,16],inputAnalog:[0],outputDigital:[25,26,27,32,33,23,22,21,19,18,17,16,5,4,2,15],outputPwm:[25,26,27,32,33,23,22,21,19,18,17,16,5,4,2,15]};";
  html += "function isActive(i){let a=document.getElementById('active'+i);return a&&a.value=='1';}";
  html += "function usedPins(kind,current){let arr=[];for(let i=0;i<MAX_CARDS;i++){if(i==current||!isActive(i))continue;if(kind=='input'&&!document.getElementById('hasInput'+i).checked)continue;let h=document.getElementById(kind+'Pin'+i);if(h&&h.value!==''&&h.value!='255')arr.push(String(h.value));}return arr;}";
  html += "function pinLabel(kind,p){if(kind=='input'&&p==0)return 'A0 / ADC0';let boot=(p==0||p==2||p==15)?' ⚠ boot':'';return 'GPIO '+p+boot;}function fillSelect(sel,values,current,used,kind,manual){let found=false;let h='<option value=\"255\">Elegir pin</option>';values.forEach(p=>{let ps=String(p);if(ps==String(current))found=true;let dis=(used.includes(ps)&&ps!=String(current))?'disabled':'';let s=(!manual&&ps==String(current))?'selected':'';h+=`<option value='${p}' ${s} ${dis}>${pinLabel(kind,p)}${dis?' - usado':''}</option>`;});h+=`<option value='manual' ${manual||(!found&&current!='255')?'selected':''}>Manual</option>`;sel.innerHTML=h;}";
  html += "function syncManual(i,kind){let sel=document.getElementById(kind+'PinSelect'+i);let man=document.getElementById(kind+'PinManual'+i);let hid=document.getElementById(kind+'Pin'+i);if(!sel||!man||!hid)return;if(sel.value=='manual'){sel.dataset.manual='1';man.style.display='block';if(!man.value||man.value=='255')man.value='';hid.value=man.value;}else{sel.dataset.manual='0';man.style.display='none';hid.value=sel.value;man.value=sel.value;}}";
  html += "function refreshPins(i){let has=document.getElementById('hasInput'+i).checked;let it=document.getElementById('inputType'+i).value;let ot=document.getElementById('outputType'+i).value;let inList=it=='1'?pinMap.inputAnalog:pinMap.inputDigital;let outList=ot=='1'?pinMap.outputPwm:pinMap.outputDigital;let inSel=document.getElementById('inputPinSelect'+i);let outSel=document.getElementById('outputPinSelect'+i);let inManual=inSel&&inSel.dataset.manual=='1';let outManual=outSel&&outSel.dataset.manual=='1';fillSelect(inSel,inList,document.getElementById('inputPin'+i).value,usedPins('input',i),'input',inManual);fillSelect(outSel,outList,document.getElementById('outputPin'+i).value,usedPins('output',i),'output',outManual);syncManual(i,'input');syncManual(i,'output');document.getElementById('inputBox'+i).style.display=has?'grid':'none';}";
  html += "function refreshCard(i){let ot=document.getElementById('outputType'+i).value;document.getElementById('digitalBox'+i).style.display=ot=='0'?'block':'none';document.getElementById('retBox'+i).style.display=ot=='0'?'block':'none';document.getElementById('digitalTest'+i).style.display=ot=='0'?'grid':'none';document.getElementById('pwmTest'+i).style.display=ot=='1'?'block':'none';refreshPins(i);}";
  html += "function refreshAll(){for(let i=0;i<MAX_CARDS;i++){if(document.getElementById('card'+i))refreshCard(i);}}";
  html += "function changePin(i,kind){syncManual(i,kind);for(let n=0;n<MAX_CARDS;n++){if(n!=i&&document.getElementById('card'+n))refreshCard(n);}}";
  html += "function manualPin(i,kind){let hid=document.getElementById(kind+'Pin'+i);let man=document.getElementById(kind+'PinManual'+i);let sel=document.getElementById(kind+'PinSelect'+i);if(hid&&man){hid.value=man.value;if(sel)sel.dataset.manual='1';}}";
  html += "function clearNewCard(i){document.getElementById('nombre'+i).value='';document.getElementById('hasInput'+i).checked=false;document.getElementById('inputType'+i).value='0';document.getElementById('inputPin'+i).value='255';document.getElementById('inputPinManual'+i).value='';document.getElementById('outputType'+i).value='0';document.getElementById('outputPin'+i).value='255';document.getElementById('outputPinManual'+i).value='';let a=document.getElementById('inputPinSelect'+i);let b=document.getElementById('outputPinSelect'+i);if(a)a.dataset.manual='0';if(b)b.dataset.manual='0';}";
  html += "function addCard(){let count=0;for(let i=0;i<MAX_CARDS;i++)if(isActive(i))count++;if(count>=MAX_CARDS){alert('Máximo 8 cards');return;}for(let i=0;i<MAX_CARDS;i++){let card=document.getElementById('card'+i);let active=document.getElementById('active'+i);if(card&&active&&active.value=='0'){clearNewCard(i);active.value='1';card.classList.remove('hidden');refreshAll();return;}}}";
  html += "function deleteCard(i){if(!confirm('¿Eliminar esta card?'))return;let active=document.getElementById('active'+i);let card=document.getElementById('card'+i);if(active)active.value='0';if(card)card.classList.add('hidden');fetch('/off?id='+i).catch(()=>{});refreshAll();}";
  html += "function feedback(i,t){let f=document.getElementById('feedback'+i);if(f){f.innerHTML=t;setTimeout(()=>{f.innerHTML='';},900);}}";
  html += "let testHolding={};function testPulseStart(i){testHolding[i]=true;feedback(i,'Soltá para activar');}function testPulseCancel(i){testHolding[i]=false;feedback(i,'Cancelado');}function testPulseRelease(i){if(!testHolding[i])return;testHolding[i]=false;fetch('/pulseRelease?id='+i).then(r=>r.text()).then(t=>feedback(i,t));}";
  html += "function testDigital(i,v){fetch('/testDigital?id='+i+'&v='+v).then(r=>r.text()).then(t=>feedback(i,t));}";
  html += "function testAnalog(i,v){fetch('/testAnalog?id='+i+'&v='+v).then(r=>r.text()).then(t=>feedback(i,t));}";
  html += "refreshAll();";
  html += "</script>";

  html += htmlEnd();
  server.send(200, "text/html", html);
}

// ============================================================
// RUTAS
// ============================================================

void handleSave() {
  config.filtroMacActivo = server.arg("filtroMacActivo") == "1";

  String mac = server.arg("macPermitida");
  mac.trim();
  mac.toUpperCase();
  mac.toCharArray(config.macPermitida, 18);

  strcpy(config.placaEmisora, "esp8266");
  strcpy(config.placaReceptora, "esp32");

  for (int i = 0; i < MAX_CARDS; i++) {
    CardConfig &c = config.cards[i];
    c.active = server.arg("active" + String(i)) == "1";

    if (!c.active) {
      apagarCard(i);
      continue;
    }

    server.arg("nombre" + String(i)).toCharArray(c.nombre, 28);
    c.hasInput = server.hasArg("hasInput" + String(i));
    c.inputType = server.arg("inputType" + String(i)).toInt();
    c.inputPin = server.arg("inputPin" + String(i)).toInt();
    c.outputType = server.arg("outputType" + String(i)).toInt();
    c.outputPin = server.arg("outputPin" + String(i)).toInt();
    c.modo = server.arg("modo" + String(i)).toInt();
    c.invertir = server.hasArg("invertir" + String(i));

    unsigned long r = server.arg("retencion" + String(i)).toInt();
    c.retencion = r > 0 ? r : 1000;

    prepararSalida(c.outputPin);
  }

  guardarConfig();
  server.sendHeader("Location", "/config");
  server.send(303);
}

void handleToggle() {
  int id = server.arg("id").toInt();
  if (id < 0 || id >= MAX_CARDS || !config.cards[id].active) {
    server.send(400, "text/plain", "ERROR");
    return;
  }

  if (config.cards[id].outputType == PIN_DIGITAL) setSalidaDigital(id, !estadoDigital[id]);
  else setSalidaAnalogica(id, estadoAnalogico[id] > 0 ? 0 : 255);

  server.send(200, "text/plain", estadoTexto(id));
}

void handlePulseRelease() {
  int id = server.arg("id").toInt();
  if (id < 0 || id >= MAX_CARDS || !config.cards[id].active) {
    server.send(400, "text/plain", "ERROR");
    return;
  }

  CardConfig &c = config.cards[id];

  // En card pulsador sin entrada física: mientras mantenés apretado NO activa.
  // Recién activa cuando soltás el botón.
  if (c.outputType == PIN_DIGITAL) {
    setSalidaDigital(id, true);
    if (c.modo == MODO_PULSADOR) {
      unsigned long r = c.retencion > 0 ? c.retencion : 1000;
      tiempoApagado[id] = millis() + r;
    }
  } else {
    setSalidaAnalogica(id, 255);
  }

  server.send(200, "text/plain", estadoTexto(id));
}

void handleOff() {
  int id = server.arg("id").toInt();
  if (id >= 0 && id < MAX_CARDS) apagarCard(id);
  server.send(200, "text/plain", "OK");
}

void handleTestDigital() {
  int id = server.arg("id").toInt();
  bool v = server.arg("v").toInt() == 1;
  if (id >= 0 && id < MAX_CARDS && config.cards[id].active) {
    if (config.cards[id].outputType == PIN_DIGITAL) setSalidaDigital(id, v);
    else setSalidaAnalogica(id, v ? 255 : 0);
    server.send(200, "text/plain", estadoTexto(id));
    return;
  }
  server.send(400, "text/plain", "ERROR");
}

void handleTestAnalog() {
  int id = server.arg("id").toInt();
  int v = server.arg("v").toInt();
  if (id >= 0 && id < MAX_CARDS && config.cards[id].active) {
    setSalidaAnalogica(id, v);
    server.send(200, "text/plain", estadoTexto(id));
    return;
  }
  server.send(400, "text/plain", "ERROR");
}

// ============================================================
// SETUP / LOOP
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(200);

  EEPROM.begin(EEPROM_SIZE);
  cargarConfig();

  // En ESP32 core 3.x analogWriteResolution pide pin + bits.
  // No hace falta configurarlo globalmente: usamos analogWrite(pin, 0-255).

  for (int i = 0; i < MAX_CARDS; i++) {
    estadoDigital[i] = false;
    estadoAnalogico[i] = 0;
    tiempoApagado[i] = 0;
    if (config.cards[i].active) prepararSalida(config.cards[i].outputPin);
  }

  WiFi.mode(WIFI_AP_STA);

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  String wifiName = getWifiName();
  WiFi.softAP(wifiName.c_str(), WIFI_PASS, ESPNOW_CHANNEL);

  Serial.println();
  Serial.println("====================================");
  Serial.println("ESP32 RECEPTOR V6 LISTO");
  Serial.print("WiFi: ");
  Serial.println(wifiName);
  Serial.print("Clave: ");
  Serial.println(WIFI_PASS);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("MAC ESP32 RECEPTOR STA: ");
  Serial.println(WiFi.macAddress());
  Serial.print("MAC ESP32 AP: ");
  Serial.println(WiFi.softAPmacAddress());
  Serial.println("====================================");

  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: no se pudo iniciar ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);

  server.on("/", handleHome);
  server.on("/config", handleConfig);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/toggle", handleToggle);
  server.on("/off", handleOff);
  server.on("/pulseRelease", handlePulseRelease);
  server.on("/testDigital", handleTestDigital);
  server.on("/testAnalog", handleTestAnalog);

  server.begin();

  Serial.println("WEB OK");
  Serial.println("ESPNOW OK");
}

void loop() {
  server.handleClient();

  for (int i = 0; i < MAX_CARDS; i++) {
    CardConfig &c = config.cards[i];
    if (!c.active) continue;
    if (c.outputType != PIN_DIGITAL) continue;
    if (c.modo != MODO_PULSADOR) continue;

    if (estadoDigital[i] && tiempoApagado[i] > 0 && millis() > tiempoApagado[i]) {
      setSalidaDigital(i, false);
      tiempoApagado[i] = 0;
    }
  }
}
