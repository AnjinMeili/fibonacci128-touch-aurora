// Incremental Drift - adapted from Aurora PatternIncrementalDrift
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon
// Concentric rings of dots orbiting at incrementally different speeds.
// Adapted for Fibonacci128 - orbits scaled to 0-255 space, proximity-based LED rendering.

// Incremental Drift Rose - adapted from Aurora PatternIncrementalDrift2
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon
// Two mirrored groups of orbiting dots creating a rose/flower pattern.
// Adapted for Fibonacci128 - orbits scaled to 0-255 space, proximity-based LED rendering.

// Pendulum Wave - adapted from Aurora PatternPendulumWave
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon, Lunarch Studios (CC0 1.0)
// Dots at evenly spaced positions swing at incrementally different frequencies.
// Adapted for Fibonacci128 - 32 pendulums across x-axis, proximity-based LED rendering.

#ifndef PatternDrift_H
#define PatternDrift_H

// beatcos8: like beatsin8 but cosine (not in standard FastLED)
uint8_t beatcos8(accum88 bpm, uint8_t lo, uint8_t hi, uint32_t tb = 0, uint8_t po = 0) {
  uint8_t beat = beat8(bpm, tb);
  uint8_t bc = cos8(beat + po);
  uint8_t rng = hi - lo;
  return lo + scale8(bc, rng);
}

#define DRIFT_RINGS 16

void incrementalDriftAnimation()
{
  uint8_t dim = beatsin8(2, 230, 250);
  fadeToBlackBy(leds, NUM_LEDS, 255 - dim);

  // Precompute dot positions and colors for all rings
  int16_t dotX[DRIFT_RINGS - 1], dotY[DRIFT_RINGS - 1];
  CRGB ringColor[DRIFT_RINGS - 1];
  uint8_t numRings = 0;

  for (int i = 2; i <= DRIFT_RINGS; i++) {
    ringColor[numRings] = ColorFromPalette(gCurrentPalette, (i - 2) * (240 / DRIFT_RINGS));
    uint8_t ri = i * 8; // orbit radius scaled for 0-255 space
    uint8_t speed = (DRIFT_RINGS + 1 - i) * 2; // inner rings orbit faster
    dotX[numRings] = (int16_t)beatcos8(speed, 128 - ri, 128 + ri);
    dotY[numRings] = (int16_t)beatsin8(speed, 128 - ri, 128 + ri);
    numRings++;
  }

  const int32_t proxSq = 400L; // 20^2

  // Single pass over LEDs, check proximity to all orbiting dots
  for (uint16_t j = 0; j < NUM_LEDS; j++) {
    int16_t lx = coordsX[j];
    int16_t ly = coordsY[j];

    for (uint8_t r = 0; r < numRings; r++) {
      int16_t dx = lx - dotX[r];
      int16_t dy = ly - dotY[r];
      int32_t dSq = (int32_t)dx * dx + (int32_t)dy * dy;
      if (dSq < proxSq) {
        uint8_t bri = (uint8_t)(255L * (proxSq - dSq) / proxSq);
        leds[j] += CRGB(scale8(ringColor[r].r, bri), scale8(ringColor[r].g, bri), scale8(ringColor[r].b, bri));
      }
    }
  }
}

#define DRIFT2_COUNT 32

void incrementalDrift2Animation()
{
  uint8_t dim = beatsin8(2, 170, 250);
  fadeToBlackBy(leds, NUM_LEDS, 255 - dim);

  // Precompute dot positions and colors
  int16_t dotX[DRIFT2_COUNT], dotY[DRIFT2_COUNT];
  CRGB dotColor[DRIFT2_COUNT];

  for (uint16_t i = 0; i < DRIFT2_COUNT; i++) {
    uint8_t lo, hi;

    if (i < DRIFT2_COUNT / 2) {
      lo = i * 8;
      hi = min(255, (DRIFT2_COUNT - i) * 8);
      dotX[i] = (int16_t)beatcos8((i + 1) * 2, lo, hi);
      dotY[i] = (int16_t)beatsin8((i + 1) * 2, lo, hi);
      dotColor[i] = ColorFromPalette(gCurrentPalette, i * 14);
    } else {
      lo = (DRIFT2_COUNT - i) * 8;
      hi = min(255, (i + 1) * 8);
      dotX[i] = (int16_t)beatsin8((DRIFT2_COUNT - i) * 2, lo, hi);
      dotY[i] = (int16_t)beatcos8((DRIFT2_COUNT - i) * 2, lo, hi);
      dotColor[i] = ColorFromPalette(gCurrentPalette, (DRIFT2_COUNT - 1 - i) * 14);
    }
  }

  const int32_t proxSq = 400L; // 20^2

  // Single pass over LEDs, check proximity to all orbiting dots
  for (uint16_t j = 0; j < NUM_LEDS; j++) {
    int16_t lx = coordsX[j];
    int16_t ly = coordsY[j];

    for (uint8_t d = 0; d < DRIFT2_COUNT; d++) {
      int16_t dx = lx - dotX[d];
      int16_t dy = ly - dotY[d];
      int32_t dSq = (int32_t)dx * dx + (int32_t)dy * dy;
      if (dSq < proxSq) {
        uint8_t bri = (uint8_t)(255L * (proxSq - dSq) / proxSq);
        leds[j] += CRGB(scale8(dotColor[d].r, bri), scale8(dotColor[d].g, bri), scale8(dotColor[d].b, bri));
      }
    }
  }
}

#define PENDULUM_COUNT 32

void pendulumWaveAnimation()
{
  fadeToBlackBy(leds, NUM_LEDS, 85); // DimAll(170)

  // Precompute pendulum positions and colors
  int16_t penX[PENDULUM_COUNT], penY[PENDULUM_COUNT];
  CRGB penColor[PENDULUM_COUNT];

  for (uint8_t i = 0; i < PENDULUM_COUNT; i++) {
    penX[i] = i * 8; // evenly spaced across 0-248
    penY[i] = beatsin16(i + 1, 0, 255); // each swings at frequency i+1 BPM
    penColor[i] = ColorFromPalette(gCurrentPalette, i * 7);
  }

  const int32_t proxSq = 400L; // 20^2

  // Single pass over LEDs
  for (uint16_t j = 0; j < NUM_LEDS; j++) {
    int16_t lx = coordsX[j];
    int16_t ly = coordsY[j];

    for (uint8_t p = 0; p < PENDULUM_COUNT; p++) {
      int16_t dx = lx - penX[p];
      int16_t dy = ly - penY[p];
      int32_t dSq = (int32_t)dx * dx + (int32_t)dy * dy;
      if (dSq < proxSq) {
        uint8_t bri = (uint8_t)(255L * (proxSq - dSq) / proxSq);
        leds[j] += CRGB(scale8(penColor[p].r, bri), scale8(penColor[p].g, bri), scale8(penColor[p].b, bri));
      }
    }
  }
}

#endif
