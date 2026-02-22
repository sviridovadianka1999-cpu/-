#include "effects.h"

#include "config.h"
#include "matrix_map.h"

namespace {
CRGB *gLeds = nullptr;
AppState *gState = nullptr;
uint8_t gHue = 0;
uint32_t gLastStepMs = 0;

std::vector<uint8_t> gHeat(NUM_LEDS, 0);
std::vector<uint8_t> gFireCols(MATRIX_W, 0);
std::vector<uint8_t> gDrops(MATRIX_W, 0);
int16_t gBounceX = 0, gBounceY = 0;
int8_t gVelX = 1, gVelY = 1;
int16_t gTextOffset = MATRIX_W;

const uint8_t FONT_5X7[][5] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}  // 9
};

void fillSolidSafe(const CRGB &c) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) gLeds[i] = c;
}

void drawPixelSafe(int16_t x, int16_t y, const CRGB &c) {
  if (x < 0 || y < 0 || x >= MATRIX_W || y >= MATRIX_H) return;
  gLeds[XY((uint8_t)x, (uint8_t)y)] = c;
}

bool glyphForChar(uint16_t cp, uint8_t out[5]) {
  memset(out, 0, 5);
  if (cp >= '0' && cp <= '9') {
    memcpy(out, FONT_5X7[cp - '0'], 5);
    return true;
  }
  if (cp >= 'A' && cp <= 'Z') {
    static const uint8_t latin[26][5] = {
        {0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36}, {0x3E, 0x41, 0x41, 0x41, 0x22},
        {0x7F, 0x41, 0x41, 0x22, 0x1C}, {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01},
        {0x3E, 0x41, 0x49, 0x49, 0x7A}, {0x7F, 0x08, 0x08, 0x08, 0x7F}, {0x00, 0x41, 0x7F, 0x41, 0x00},
        {0x20, 0x40, 0x41, 0x3F, 0x01}, {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
        {0x7F, 0x02, 0x0C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F}, {0x3E, 0x41, 0x41, 0x41, 0x3E},
        {0x7F, 0x09, 0x09, 0x09, 0x06}, {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
        {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01}, {0x3F, 0x40, 0x40, 0x40, 0x3F},
        {0x1F, 0x20, 0x40, 0x20, 0x1F}, {0x3F, 0x40, 0x38, 0x40, 0x3F}, {0x63, 0x14, 0x08, 0x14, 0x63},
        {0x07, 0x08, 0x70, 0x08, 0x07}, {0x61, 0x51, 0x49, 0x45, 0x43}};
    memcpy(out, latin[cp - 'A'], 5);
    return true;
  }
  if (cp == ' ') return true;
  if (cp == '-') { uint8_t t[5] = {0x08,0x08,0x08,0x08,0x08}; memcpy(out,t,5); return true; }

  // Cyrillic uppercase А..Я in UTF-8 -> U+0410..U+042F mapped roughly to Latin-like glyphs.
  if (cp == 0x0401) {
    uint8_t t[5] = {0x7F, 0x49, 0x49, 0x49, 0x41};
    memcpy(out, t, 5);
    return true;
  }
  if (cp >= 0x0410 && cp <= 0x042F) {
    static const uint8_t simple[5] = {0x00,0x00,0x5F,0x00,0x00};
    memcpy(out, simple, 5);
    switch (cp) {
      case 0x0410: {uint8_t t[5]={0x7E,0x11,0x11,0x11,0x7E}; memcpy(out,t,5);} break; // А
      case 0x0412: {uint8_t t[5]={0x7F,0x49,0x49,0x49,0x36}; memcpy(out,t,5);} break; // В
      case 0x0415: {uint8_t t[5]={0x7F,0x49,0x49,0x49,0x41}; memcpy(out,t,5);} break; // Е
      case 0x0418: {uint8_t t[5]={0x7F,0x04,0x08,0x10,0x7F}; memcpy(out,t,5);} break; // И
      case 0x041A: {uint8_t t[5]={0x7F,0x08,0x14,0x22,0x41}; memcpy(out,t,5);} break; // К
      case 0x041C: {uint8_t t[5]={0x7F,0x02,0x0C,0x02,0x7F}; memcpy(out,t,5);} break; // М
      case 0x041D: {uint8_t t[5]={0x7F,0x08,0x08,0x08,0x7F}; memcpy(out,t,5);} break; // Н
      case 0x041E: {uint8_t t[5]={0x3E,0x41,0x41,0x41,0x3E}; memcpy(out,t,5);} break; // О
      case 0x041F: {uint8_t t[5]={0x7F,0x01,0x01,0x01,0x7F}; memcpy(out,t,5);} break; // П
      case 0x0420: {uint8_t t[5]={0x7F,0x09,0x09,0x09,0x06}; memcpy(out,t,5);} break; // Р
      case 0x0421: {uint8_t t[5]={0x3E,0x41,0x41,0x41,0x22}; memcpy(out,t,5);} break; // С
      case 0x0422: {uint8_t t[5]={0x01,0x01,0x7F,0x01,0x01}; memcpy(out,t,5);} break; // Т
      case 0x0423: {uint8_t t[5]={0x07,0x08,0x70,0x08,0x07}; memcpy(out,t,5);} break; // У
      case 0x0425: {uint8_t t[5]={0x63,0x14,0x08,0x14,0x63}; memcpy(out,t,5);} break; // Х
      default: break;
    }
    return true;
  }
  return false;
}

uint16_t decodeUtf8(const String &s, uint16_t &idx) {
  uint8_t c = (uint8_t)s[idx++];
  if (c < 0x80) return c;
  if ((c & 0xE0) == 0xC0 && idx < s.length()) {
    uint8_t c2 = (uint8_t)s[idx++];
    return ((c & 0x1F) << 6) | (c2 & 0x3F);
  }
  return '?';
}

void renderTextFrame() {
  fillSolidSafe(CRGB::Black);
  int16_t cursor = gTextOffset;
  for (uint16_t i = 0; i < gState->text.length();) {
    uint16_t cp = decodeUtf8(gState->text, i);
    if (cp >= 'a' && cp <= 'z') cp -= 32;
    if (cp >= 0x0430 && cp <= 0x044F) cp -= 0x20;
    if (cp == 0x0451) cp = 0x0401;
    uint8_t glyph[5];
    if (!glyphForChar(cp, glyph)) continue;
    for (uint8_t col = 0; col < 5; col++) {
      int16_t x = cursor + col;
      if (x < 0 || x >= MATRIX_W) continue;
      for (uint8_t row = 0; row < 7; row++) {
        if (glyph[col] & (1 << row)) {
          drawPixelSafe(x, row, gState->textColor);
        }
      }
    }
    cursor += 6;
  }
}

int textPixelLength() {
  int chars = 0;
  for (uint16_t i = 0; i < gState->text.length();) {
    decodeUtf8(gState->text, i);
    chars++;
  }
  return chars * 6;
}

void effectGradient() {
  size_t n = gState->colors.size();
  if (n < 2) {
    fillSolidSafe(n == 1 ? gState->colors[0] : CRGB::Black);
    return;
  }
  for (uint8_t y = 0; y < MATRIX_H; y++) {
    for (uint8_t x = 0; x < MATRIX_W; x++) {
      float pos = (float)x / (float)(MATRIX_W - 1);
      float scaled = pos * (n - 1);
      uint8_t i0 = (uint8_t)scaled;
      uint8_t i1 = min((int)n - 1, (int)i0 + 1);
      float t = scaled - i0;
      CRGB c = blend(gState->colors[i0], gState->colors[i1], (uint8_t)(t * 255));
      gLeds[XY(x, y)] = c;
    }
  }
}

CRGB firePaletteColor(uint8_t v) {
  if (v <= 10) return CRGB(0, 0, 0);
  if (v <= 30) return CRGB(map(v, 11, 30, 8, 28), 0, 0);
  if (v <= 90) return CRGB(map(v, 31, 90, 40, 130), 0, 0);
  if (v <= 150) return CRGB(map(v, 91, 150, 140, 220), map(v, 91, 150, 8, 40), 0);
  if (v <= 210) return CRGB(255, map(v, 151, 210, 45, 130), 0);
  if (v <= 245) return CRGB(255, map(v, 211, 245, 140, 220), map(v, 211, 245, 0, 35));
  return CRGB(255, 245, map(v, 246, 255, 60, 120));
}

void effectFire() {
  std::vector<uint8_t> smoothCols(MATRIX_W, 0);
  std::vector<uint8_t> fireHeight(MATRIX_W, 0);

  for (uint8_t x = 0; x < MATRIX_W; x++) {
    uint8_t base = random8(85, 155);
    if (random8() < 45) base = qadd8(base, random8(25, 75));
    gFireCols[x] = qadd8(scale8(gFireCols[x], 185), base);
  }

  for (uint8_t x = 0; x < MATRIX_W; x++) {
    uint8_t l = gFireCols[(x == 0) ? MATRIX_W - 1 : x - 1];
    uint8_t c = gFireCols[x];
    uint8_t r = gFireCols[(x + 1) % MATRIX_W];
    smoothCols[x] = (l + c + r + c) / 4;
  }

  for (uint8_t x = 0; x < MATRIX_W; x++) {
    uint8_t spike = (random8() < 28) ? random8(18, 65) : 0;
    gFireCols[x] = qadd8(scale8(smoothCols[x], 215), spike);

    uint8_t h = map(gFireCols[x], 0, 255, 2, MATRIX_H);
    if (random8() < 25) h = min((uint8_t)MATRIX_H, (uint8_t)(h + 1));
    if (random8() < 12) h = min((uint8_t)MATRIX_H, (uint8_t)(h + 2));
    fireHeight[x] = h;
  }

  for (uint8_t y = 0; y < MATRIX_H; y++) {
    for (uint8_t x = 0; x < MATRIX_W; x++) {
      uint8_t dy = MATRIX_H - 1 - y;
      uint8_t h = fireHeight[x];
      int16_t heat = 0;

      if (dy <= h) {
        heat = (int16_t)gFireCols[x] - (int16_t)dy * 34;

        if (dy >= h - 1 && random8() < 150) heat -= random8(50, 140);
        if (dy >= h - 2 && random8() < 90) heat -= random8(20, 90);
        if (dy >= 5 && random8() < 120) heat -= random8(30, 110);
        if (dy >= 6 && random8() < 175) heat = 0;

        int16_t jitter = (int16_t)random8(0, 36) - 18;
        heat += jitter;
      } else {
        if (dy <= h + 1 && random8() < 18) heat = random8(18, 60);
      }

      if (dy <= 1 && heat > 220 && random8() < 220) heat = 220;
      if (heat < 0) heat = 0;
      if (heat > 255) heat = 255;

      gLeds[XY(x, y)] = firePaletteColor((uint8_t)heat);
    }
  }
}


void effectMatrixRain() {
  fadeToBlackBy(gLeds, NUM_LEDS, 55);
  for (uint8_t x = 0; x < MATRIX_W; x++) {
    if (gDrops[x] >= MATRIX_H || random8() < 25) gDrops[x] = 0;
    uint8_t y = gDrops[x];
    if (y < MATRIX_H) {
      drawPixelSafe(x, y, CRGB(0, 255, 70));
      if (y > 0) drawPixelSafe(x, y - 1, CRGB(0, 120, 20));
      gDrops[x]++;
    }
  }
}

} // namespace

void effectsInit(CRGB *leds, AppState *state) {
  gLeds = leds;
  gState = state;
  clearLeds();
}

void clearLeds() {
  if (!gLeds) return;
  fill_solid(gLeds, NUM_LEDS, CRGB::Black);
}

void effectsTick(uint32_t nowMs) {
  if (!gLeds || !gState) return;

  uint16_t interval = map(gState->speed, 1, 255, 140, 10);
  if (gState->mode == EffectMode::TEXT) {
    interval = map(gState->textSpeed, 1, 255, 180, 15);
  }
  if (nowMs - gLastStepMs < interval) return;
  gLastStepMs = nowMs;

  switch (gState->mode) {
    case EffectMode::OFF:
      clearLeds();
      break;
    case EffectMode::STATIC_COLOR:
      fillSolidSafe(gState->colors.empty() ? CRGB::Black : gState->colors[0]);
      break;
    case EffectMode::GRADIENT:
      effectGradient();
      break;
    case EffectMode::RAINBOW:
      fill_rainbow(gLeds, NUM_LEDS, gHue++, 4);
      break;
    case EffectMode::TEXT: {
      renderTextFrame();
      int len = textPixelLength();
      gTextOffset += gState->textDirectionLTR ? 1 : -1;
      if (!gState->textDirectionLTR && gTextOffset < -len) gTextOffset = MATRIX_W;
      if (gState->textDirectionLTR && gTextOffset > MATRIX_W) gTextOffset = -len;
      break;
    }
    case EffectMode::FIRE:
      effectFire();
      break;
    case EffectMode::MATRIX_RAIN:
      effectMatrixRain();
      break;
    case EffectMode::BOUNCING_PIXEL:
      clearLeds();
      drawPixelSafe(gBounceX, gBounceY, gState->colors.empty() ? CRGB::White : gState->colors[0]);
      gBounceX += gVelX;
      gBounceY += gVelY;
      if (gBounceX <= 0 || gBounceX >= MATRIX_W - 1) gVelX *= -1;
      if (gBounceY <= 0 || gBounceY >= MATRIX_H - 1) gVelY *= -1;
      break;
  }
}

const char *modeToString(EffectMode mode) {
  switch (mode) {
    case EffectMode::OFF: return "off";
    case EffectMode::STATIC_COLOR: return "color";
    case EffectMode::GRADIENT: return "gradient";
    case EffectMode::RAINBOW: return "rainbow";
    case EffectMode::TEXT: return "text";
    case EffectMode::FIRE: return "fire";
    case EffectMode::MATRIX_RAIN: return "matrix";
    case EffectMode::BOUNCING_PIXEL: return "bounce";
    default: return "color";
  }
}

bool modeFromString(const String &value, EffectMode &out) {
  if (value == "off") out = EffectMode::OFF;
  else if (value == "color") out = EffectMode::STATIC_COLOR;
  else if (value == "gradient") out = EffectMode::GRADIENT;
  else if (value == "rainbow") out = EffectMode::RAINBOW;
  else if (value == "text") out = EffectMode::TEXT;
  else if (value == "fire") out = EffectMode::FIRE;
  else if (value == "matrix") out = EffectMode::MATRIX_RAIN;
  else if (value == "bounce") out = EffectMode::BOUNCING_PIXEL;
  else return false;
  return true;
}
