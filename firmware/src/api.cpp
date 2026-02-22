#include "api.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

#include "config.h"

namespace {
ESP8266WebServer *gHttp = nullptr;
WebSocketsServer *gWs = nullptr;
AppState *gState = nullptr;
WifiConfig *gWifi = nullptr;
std::function<void(void)> gSaveState;
std::function<void(void)> gSaveWifi;

uint8_t clampBrightness(uint8_t v) { return min(v, BRIGHTNESS_LIMIT_255); }

CRGB parseColor(JsonVariantConst v) {
  if (!v.is<const char *>()) return CRGB::Black;
  String s = v.as<String>();
  if (s.startsWith("#") && s.length() == 7) {
    long raw = strtol(s.substring(1).c_str(), nullptr, 16);
    return CRGB((raw >> 16) & 0xFF, (raw >> 8) & 0xFF, raw & 0xFF);
  }
  return CRGB::Black;
}

String colorToHex(const CRGB &c) {
  char buf[8];
  snprintf(buf, sizeof(buf), "#%02X%02X%02X", c.r, c.g, c.b);
  return String(buf);
}

String stateJson() {
  StaticJsonDocument<768> doc;
  doc["mode"] = modeToString(gState->mode);
  doc["brightness"] = gState->brightness;
  doc["brightness_limit"] = BRIGHTNESS_LIMIT_255;
  doc["speed"] = gState->speed;
  doc["text_speed"] = gState->textSpeed;
  doc["text_direction_ltr"] = gState->textDirectionLTR;
  doc["text"] = gState->text;
  doc["text_color"] = colorToHex(gState->textColor);
  doc["ip"] = WiFi.localIP().toString();
  JsonArray colors = doc.createNestedArray("colors");
  for (auto &c : gState->colors) colors.add(colorToHex(c));

  String out;
  serializeJson(doc, out);
  return out;
}

void applyCommand(const JsonDocument &doc) {
  if (doc.containsKey("set_mode")) {
    EffectMode m;
    if (modeFromString(doc["set_mode"].as<String>(), m)) gState->mode = m;
  }
  if (doc.containsKey("set_brightness")) gState->brightness = clampBrightness(doc["set_brightness"].as<uint8_t>());
  if (doc.containsKey("set_speed")) gState->speed = doc["set_speed"].as<uint8_t>();
  if (doc.containsKey("set_text_speed")) gState->textSpeed = doc["set_text_speed"].as<uint8_t>();
  if (doc.containsKey("set_text_direction_ltr")) gState->textDirectionLTR = doc["set_text_direction_ltr"].as<bool>();
  if (doc.containsKey("set_text")) gState->text = doc["set_text"].as<String>();
  if (doc.containsKey("set_text_color")) gState->textColor = parseColor(doc["set_text_color"]);
  if (doc.containsKey("set_colors") && doc["set_colors"].is<JsonArrayConst>()) {
    gState->colors.clear();
    for (JsonVariantConst v : doc["set_colors"].as<JsonArrayConst>()) gState->colors.push_back(parseColor(v));
    if (gState->colors.empty()) gState->colors.push_back(CRGB::Blue);
  }
  if (doc.containsKey("wifi_config") && doc["wifi_config"].is<JsonObjectConst>()) {
    JsonObjectConst wc = doc["wifi_config"].as<JsonObjectConst>();
    gWifi->ssid = wc["ssid"].as<String>();
    gWifi->password = wc["password"].as<String>();
    gSaveWifi();
  }
  if (doc.containsKey("save_config") && doc["save_config"].as<bool>()) gSaveState();
}

void handleApply() {
  if (!gHttp->hasArg("plain")) {
    gHttp->send(400, "application/json", "{\"ok\":false,\"error\":\"missing body\"}");
    return;
  }
  StaticJsonDocument<1024> doc;
  auto err = deserializeJson(doc, gHttp->arg("plain"));
  if (err) {
    gHttp->send(400, "application/json", "{\"ok\":false,\"error\":\"bad json\"}");
    return;
  }
  applyCommand(doc);
  String out = stateJson();
  gHttp->send(200, "application/json", out);
  gWs->broadcastTXT(out);
}

void handleGetStatus() { gHttp->send(200, "application/json", stateJson()); }

void handleWifiPage() {
  String html = "<html><body><h2>LED Matrix Wi-Fi setup</h2><form method='POST' action='/wifi'>"
                "SSID:<br><input name='ssid'><br>Password:<br><input name='password' type='password'><br><br>"
                "<button type='submit'>Save</button></form></body></html>";
  gHttp->send(200, "text/html", html);
}

void handleWifiPost() {
  gWifi->ssid = gHttp->arg("ssid");
  gWifi->password = gHttp->arg("password");
  gSaveWifi();
  gHttp->send(200, "text/html", "Saved. Rebooting in 2 sec...");
  delay(2000);
  ESP.restart();
}

void wsEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t len) {
  if (type == WStype_CONNECTED) {
    String payload = stateJson();
    gWs->sendTXT(num, payload);
    return;
  }
  if (type != WStype_TEXT) return;

  StaticJsonDocument<1024> doc;
  auto err = deserializeJson(doc, payload, len);
  if (err) {
    gWs->sendTXT(num, "{\"ok\":false,\"error\":\"bad json\"}");
    return;
  }
  applyCommand(doc);
  String out = stateJson();
  gWs->broadcastTXT(out);
}

} // namespace

void apiInit(ESP8266WebServer *http, WebSocketsServer *ws, AppState *state, WifiConfig *wifiCfg,
             std::function<void(void)> saveStateCb, std::function<void(void)> saveWifiCb) {
  gHttp = http;
  gWs = ws;
  gState = state;
  gWifi = wifiCfg;
  gSaveState = saveStateCb;
  gSaveWifi = saveWifiCb;

  gHttp->on("/", HTTP_GET, []() { gHttp->send(200, "application/json", "{\"device\":\"esp8266-led-matrix\"}"); });
  gHttp->on("/api/status", HTTP_GET, handleGetStatus);
  gHttp->on("/api/apply", HTTP_POST, handleApply);
  gHttp->on("/wifi", HTTP_GET, handleWifiPage);
  gHttp->on("/wifi", HTTP_POST, handleWifiPost);

  gWs->begin();
  gWs->onEvent(wsEvent);
}

void apiHandleHttp() { gHttp->handleClient(); }
void apiHandleWs() { gWs->loop(); }
void apiBroadcastState() {
  String payload = stateJson();
  gWs->broadcastTXT(payload);
}
