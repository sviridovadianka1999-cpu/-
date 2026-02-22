#pragma once

#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include "effects.h"

struct WifiConfig {
  String ssid;
  String password;
};

void apiInit(ESP8266WebServer *http, WebSocketsServer *ws, AppState *state, WifiConfig *wifiCfg,
             std::function<void(void)> saveStateCb, std::function<void(void)> saveWifiCb);
void apiHandleHttp();
void apiHandleWs();
void apiBroadcastState();
