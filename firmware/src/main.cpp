#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FastLED.h>
#include <LittleFS.h>
#include <WebSocketsServer.h>

#include "api.h"
#include "config.h"
#include "effects.h"

CRGB leds[NUM_LEDS];
AppState state;
WifiConfig wifiCfg;

ESP8266WebServer http(HTTP_PORT);
WebSocketsServer ws(WS_PORT);

const char *STATE_FILE = "/state.json";
const char *WIFI_FILE = "/wifi.json";

void saveWifiConfig() {
  StaticJsonDocument<256> d;
  d["ssid"] = wifiCfg.ssid;
  d["password"] = wifiCfg.password;
  File f = LittleFS.open(WIFI_FILE, "w");
  if (!f) return;
  serializeJson(d, f);
  f.close();
}

void loadWifiConfig() {
  wifiCfg.ssid = DEFAULT_WIFI_SSID;
  wifiCfg.password = DEFAULT_WIFI_PASS;

  if (!LittleFS.exists(WIFI_FILE)) return;
  File f = LittleFS.open(WIFI_FILE, "r");
  if (!f) return;
  StaticJsonDocument<256> d;
  if (deserializeJson(d, f) == DeserializationError::Ok) {
    wifiCfg.ssid = d["ssid"].as<String>();
    wifiCfg.password = d["password"].as<String>();
  }
  f.close();
}

void saveState() {
  StaticJsonDocument<1024> d;
  d["mode"] = modeToString(state.mode);
  d["brightness"] = state.brightness;
  d["speed"] = state.speed;
  d["text_speed"] = state.textSpeed;
  d["text_dir"] = state.textDirectionLTR;
  d["text"] = state.text;
  char colorBuf[8];
  snprintf(colorBuf, sizeof(colorBuf), "#%02X%02X%02X", state.textColor.r, state.textColor.g, state.textColor.b);
  d["text_color"] = colorBuf;
  JsonArray arr = d.createNestedArray("colors");
  for (auto &c : state.colors) {
    snprintf(colorBuf, sizeof(colorBuf), "#%02X%02X%02X", c.r, c.g, c.b);
    arr.add(colorBuf);
  }

  File f = LittleFS.open(STATE_FILE, "w");
  if (!f) return;
  serializeJson(d, f);
  f.close();
}

CRGB parseHex(const String &s) {
  if (!s.startsWith("#") || s.length() != 7) return CRGB::Black;
  long raw = strtol(s.substring(1).c_str(), nullptr, 16);
  return CRGB((raw >> 16) & 0xFF, (raw >> 8) & 0xFF, raw & 0xFF);
}

void loadState() {
  state.mode = EffectMode::STATIC_COLOR;
  state.brightness = SAFE_DEFAULT_BRIGHTNESS;
  state.speed = 80;
  state.textSpeed = 90;
  state.textDirectionLTR = false;
  state.text = "HELLO 8266";
  state.textColor = CRGB::Red;
  state.colors = {CRGB::Blue, CRGB::Purple};

  if (!LittleFS.exists(STATE_FILE)) return;
  File f = LittleFS.open(STATE_FILE, "r");
  if (!f) return;

  StaticJsonDocument<1024> d;
  if (deserializeJson(d, f) == DeserializationError::Ok) {
    EffectMode m;
    if (modeFromString(d["mode"].as<String>(), m)) state.mode = m;
    state.brightness = min((uint8_t)d["brightness"].as<uint8_t>(), BRIGHTNESS_LIMIT_255);
    state.speed = d["speed"] | 80;
    state.textSpeed = d["text_speed"] | 90;
    state.textDirectionLTR = d["text_dir"] | false;
    state.text = d["text"] | "HELLO 8266";
    state.textColor = parseHex(d["text_color"] | "#FF0000");
    if (d["colors"].is<JsonArrayConst>()) {
      state.colors.clear();
      for (String c : d["colors"].as<JsonArrayConst>()) state.colors.push_back(parseHex(c));
      if (state.colors.empty()) state.colors = {CRGB::Blue, CRGB::Purple};
    }
  }
  f.close();
}

void connectWifiOrFallback() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiCfg.ssid.c_str(), wifiCfg.password.c_str());

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(FALLBACK_AP_SSID, FALLBACK_AP_PASS);
}

void setup() {
  Serial.begin(115200);
  LittleFS.begin();
  loadWifiConfig();
  loadState();

  FastLED.addLeds<WS2812B, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(min(state.brightness, BRIGHTNESS_LIMIT_255));
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_CURRENT_MA);
  FastLED.clear(true);

  effectsInit(leds, &state);

  connectWifiOrFallback();
  apiInit(&http, &ws, &state, &wifiCfg, saveState, saveWifiConfig);

  Serial.println("Device ready");
  Serial.print("STA IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  FastLED.setBrightness(min(state.brightness, BRIGHTNESS_LIMIT_255));
  effectsTick(millis());
  FastLED.show();

  apiHandleHttp();
  apiHandleWs();

  delay(MAIN_LOOP_DELAY_MS);
}
