// Radar - adapted from Aurora PatternRadar
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon
// Sweeping radar arm with fading trail, colored by radius through the palette.
// Adapted for Fibonacci128 - uses angles[] array for natural radial sweep.

// Spiral - adapted from Aurora PatternSpiral
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Stefan Petrick (Funky Clouds)
// Rotating spiral arms with oscillating tightness and width.
// Adapted for Fibonacci128 - uses angles[]/radius[] for natural spiral rendering.

// Swirl - adapted from Aurora PatternSwirl
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon, Mark Kriegsman (SmartMatrixSwirl)
// Six symmetric bouncing dots with fading trails.
// Adapted for Fibonacci128 - proximity-based LED rendering replaces blur2d.

#ifndef PatternRadar_H
#define PatternRadar_H

void radarAnimation()
{
  static byte radarTheta = 0;
  static byte radarHueOffset = 0;

  fadeToBlackBy(leds, NUM_LEDS, 3); // very slow fade for long radar trail

  EVERY_N_MILLIS(25) {
    radarTheta += 2;
    radarHueOffset += 1;
  }

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    // Shortest angular distance from sweep line (0-128)
    int16_t diff = (int16_t)angles[i] - (int16_t)radarTheta;
    if (diff < 0) diff += 256;
    if (diff > 128) diff = 256 - diff;

    if (diff < 8) { // narrow sweep arc (~11 degrees)
      uint8_t bri = 255 - diff * 32;
      // Color shifts with radius and time, matching original's per-ring hue offset
      byte colorIdx = 255 - (radius[i] + radarHueOffset);
      CRGB color = ColorFromPalette(gCurrentPalette, colorIdx);
      leds[i] += CRGB(scale8(color.r, bri), scale8(color.g, bri), scale8(color.b, bri));
    }
  }
}

void spiralAnimation()
{
  fadeToBlackBy(leds, NUM_LEDS, 31); // DimAll(224)

  // Oscillating parameters for organic movement (inspired by original's 5 oscillators)
  uint8_t baseAngle = beat8(7);            // rotation speed
  uint8_t tightness = beatsin8(3, 1, 3);   // spiral tightness oscillates
  uint8_t colorPhase = beat8(11);           // color cycling
  uint8_t armWidth = beatsin8(5, 10, 20);  // arm width breathes

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    uint8_t r = radius[i];
    uint8_t a = angles[i];

    // Two spiral arms, 180 degrees apart
    for (uint8_t arm = 0; arm < 2; arm++) {
      uint8_t spiralAngle = baseAngle + r * tightness + arm * 128;

      // Shortest angular distance
      int16_t diff = (int16_t)a - (int16_t)spiralAngle;
      if (diff < 0) diff += 256;
      if (diff > 128) diff = 256 - diff;

      if (diff < armWidth) {
        uint8_t bri = (uint8_t)(255L * (armWidth - diff) / armWidth);
        byte colorIdx = r + colorPhase + arm * 80;
        CRGB color = ColorFromPalette(gCurrentPalette, colorIdx);
        leds[i] += CRGB(scale8(color.r, bri), scale8(color.g, bri), scale8(color.b, bri));
      }
    }
  }
}

#define SWIRL_DOTS 6

void swirlAnimation()
{
  // Oscillating fade replaces blur2d (lossy blur trends toward black)
  uint8_t blurAmount = beatsin8(2, 10, 128);
  fadeToBlackBy(leds, NUM_LEDS, 256 - blurAmount);

  // Two out-of-sync sine waves for base positions (scaled to 0-255)
  uint8_t si = beatsin8(27, 16, 240);
  uint8_t sj = beatsin8(41, 16, 240);
  // Reflections
  uint8_t ni = 255 - si;
  uint8_t nj = 255 - sj;

  // 6 symmetric dot positions
  int16_t dotPosX[SWIRL_DOTS] = { si, sj, ni, nj, si, ni };
  int16_t dotPosY[SWIRL_DOTS] = { sj, si, nj, ni, nj, sj };

  // Each dot shifts color at a different rate
  uint16_t ms = millis();
  uint8_t colorIdx[SWIRL_DOTS];
  colorIdx[0] = ms / 11;
  colorIdx[1] = ms / 13;
  colorIdx[2] = ms / 17;
  colorIdx[3] = ms / 29;
  colorIdx[4] = ms / 37;
  colorIdx[5] = ms / 41;

  CRGB dotColors[SWIRL_DOTS];
  for (uint8_t d = 0; d < SWIRL_DOTS; d++) {
    dotColors[d] = ColorFromPalette(gCurrentPalette, colorIdx[d]);
  }

  const int32_t proxSq = 400L; // 20^2

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    int16_t lx = coordsX[i];
    int16_t ly = coordsY[i];

    for (uint8_t d = 0; d < SWIRL_DOTS; d++) {
      int16_t dx = lx - dotPosX[d];
      int16_t dy = ly - dotPosY[d];
      int32_t dSq = (int32_t)dx * dx + (int32_t)dy * dy;
      if (dSq < proxSq) {
        uint8_t bri = (uint8_t)(255L * (proxSq - dSq) / proxSq);
        leds[i] += CRGB(scale8(dotColors[d].r, bri), scale8(dotColors[d].g, bri), scale8(dotColors[d].b, bri));
      }
    }
  }
}

#endif
