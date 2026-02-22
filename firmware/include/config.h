#pragma once

#include <Arduino.h>

// Matrix hardware config
static const uint16_t MATRIX_W = 45;
static const uint16_t MATRIX_H = 8;
static const uint16_t NUM_LEDS = MATRIX_W * MATRIX_H;

static const bool SERPENTINE = true;
static const bool FIRST_ROW_RIGHT_TO_LEFT = false;

static const uint8_t DATA_PIN = 4; // D2 on NodeMCU
static const EOrder COLOR_ORDER = GRB;

// Power safety limits
static const uint8_t BRIGHTNESS_LIMIT_PERCENT = 30;
static const uint8_t BRIGHTNESS_LIMIT_255 = (uint8_t)((255UL * BRIGHTNESS_LIMIT_PERCENT) / 100UL);
static const uint8_t SAFE_DEFAULT_BRIGHTNESS = 32;
static const uint16_t MAX_CURRENT_MA = 3000; // safety cap for 5V PSU + wiring

// Wi-Fi defaults for first boot
static const char *DEFAULT_WIFI_SSID = "RT-GPON-39C2";
static const char *DEFAULT_WIFI_PASS = "USYHb7EGae";

// Fallback AP
static const char *FALLBACK_AP_SSID = "LED-MATRIX-SETUP";
static const char *FALLBACK_AP_PASS = "12345678";

// Networking
static const uint16_t HTTP_PORT = 80;
static const uint16_t WS_PORT = 81;

// Timing
static const uint16_t MAIN_LOOP_DELAY_MS = 16; // ~60 FPS upper bound
