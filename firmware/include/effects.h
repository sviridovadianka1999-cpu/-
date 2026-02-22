#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include <vector>

enum class EffectMode : uint8_t {
  OFF = 0,
  STATIC_COLOR,
  GRADIENT,
  RAINBOW,
  TEXT,
  FIRE,
  MATRIX_RAIN,
  BOUNCING_PIXEL
};

struct AppState {
  EffectMode mode = EffectMode::STATIC_COLOR;
  uint8_t brightness = 32;
  uint8_t speed = 80;      // generic speed [1..255]
  uint8_t textSpeed = 90;  // separate text speed
  bool textDirectionLTR = false;
  String text = "HELLO 8266";
  CRGB textColor = CRGB::Red;
  std::vector<CRGB> colors = {CRGB::Blue, CRGB::Purple};
};

void effectsInit(CRGB *leds, AppState *state);
void effectsTick(uint32_t nowMs);

const char *modeToString(EffectMode mode);
bool modeFromString(const String &value, EffectMode &out);

void clearLeds();
