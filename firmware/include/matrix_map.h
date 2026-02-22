#pragma once

#include <stdint.h>
#include "config.h"

inline uint16_t XY(uint8_t x, uint8_t y) {
  if (x >= MATRIX_W || y >= MATRIX_H) {
    return 0;
  }

  uint8_t logicalX = x;
  if (FIRST_ROW_RIGHT_TO_LEFT) {
    logicalX = MATRIX_W - 1 - x;
  }

  if (!SERPENTINE) {
    return y * MATRIX_W + logicalX;
  }

  bool reverseRow = (y % 2 == 1);
  if (reverseRow) {
    logicalX = MATRIX_W - 1 - logicalX;
  }

  return y * MATRIX_W + logicalX;
}
