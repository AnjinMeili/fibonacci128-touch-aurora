// Electric Mandala - adapted from Aurora PatternElectricMandala
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon, Stefan Petrick (FunkyNoise)
// Perlin noise with kaleidoscope symmetry (diagonal mirror + 4-fold rotation).
// Adapted for Fibonacci128 - symmetry applied as coordinate folds before noise lookup.

// Aurora Plasma - adapted from PatternPlasma by Robert Atkins / Jason Coon
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon, (c) 2013 Robert Atkins
// Overlapping sin/cos waves colored through the current palette.
// Adapted for Fibonacci128 - iterates over LED coordinates instead of grid.

#ifndef PatternAurora_H
#define PatternAurora_H

void electricMandalaAnimation()
{
  static uint32_t emNoiseX, emNoiseY, emNoiseZ;
  static uint16_t emScaleX = 6000, emScaleY = 6000;
  static int16_t emDx = 0, emDy = 0, emDz = 0;
  static bool emInitialized = false;
  static uint8_t emSmooth[NUM_LEDS];
  static PatternRotation emRot = {0, 0, 0, 0, false};

  if (!emInitialized) {
    emNoiseX = random16();
    emNoiseY = random16();
    emNoiseZ = random16();
    emDx = random8();
    emDy = random8();
    emDz = random8();
    memset(emSmooth, 128, NUM_LEDS);
    emInitialized = true;
  }

  // Randomize noise drift and scale every 5 seconds
  EVERY_N_SECONDS(5) {
    emDx = random16(500) - 250;
    emDy = random16(500) - 250;
    emDz = random16(500) - 250;
    emScaleX = random16(10000) + 2000;
    emScaleY = random16(10000) + 2000;
  }

  // Re-randomize rotation speeds every 8 seconds
  EVERY_N_SECONDS(8) {
    emRot.randomizeSpeeds();
  }

  emNoiseX += emDx;
  emNoiseY += emDy;
  emNoiseZ += emDz;

  // Update rotation (sinusoidal speed: speeds up, slows, pauses, reverses)
  float emAngle = emRot.update();
  float emCosA = cosf(emAngle), emSinA = sinf(emAngle);

  const uint8_t smoothing = 200;

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    // Rotate coordinates around center before kaleidoscope folds
    float fx = (float)coordsX[i] - 128.0f;
    float fy = (float)coordsY[i] - 128.0f;
    int16_t rx = (int16_t)(128.0f + emCosA * fx - emSinA * fy);
    int16_t ry = (int16_t)(128.0f + emSinA * fx + emCosA * fy);

    // Distance from center — folds all quadrants (Caleidoscope1)
    int16_t dx = abs(rx - 128);
    int16_t dy = abs(ry - 128);

    // Diagonal mirror (Caleidoscope3): ensures noise[x][y] == noise[y][x]
    int16_t kx = dx < dy ? dx : dy;
    int16_t ky = dx < dy ? dy : dx;

    // Scale to match original ~16-pixel matrix half-width
    kx >>= 3;
    ky >>= 3;

    uint32_t ioffset = (uint32_t)emScaleX * kx;
    uint32_t joffset = (uint32_t)emScaleY * ky;
    uint8_t data = inoise16(emNoiseX + ioffset, emNoiseY + joffset, emNoiseZ) >> 8;

    // Temporal smoothing (200/256 old + 56/256 new)
    uint8_t olddata = emSmooth[i];
    data = scale8(olddata, smoothing) + scale8(data, 256 - smoothing);
    emSmooth[i] = data;

    // Noise value used as both palette index and brightness (matches original)
    leds[i] = ColorFromPalette(gCurrentPalette, data, data);
  }
}

void auroraPlasmaAnimation()
{
  static int auroraPlasmaTime = 0;
  static int auroraPlasmaFrames = 0;
  static PatternRotation apRot = {0, 0, 0, 0, false};

  // Re-randomize rotation speeds every 8 seconds
  EVERY_N_SECONDS(8) {
    apRot.randomizeSpeeds();
  }

  // Update rotation (sinusoidal speed: speeds up, slows, pauses, reverses)
  float apAngle = apRot.update();
  float apCosA = cosf(apAngle), apSinA = sinf(apAngle);

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    // Rotate coordinates around center before plasma calculation
    float fx = (float)coordsX[i] - 128.0f;
    float fy = (float)coordsY[i] - 128.0f;
    int16_t rx = (int16_t)(128.0f + apCosA * fx - apSinA * fy);
    int16_t ry = (int16_t)(128.0f + apSinA * fx + apCosA * fy);

    // Scale rotated coords to ~0-31 range to match original matrix dimensions
    uint8_t x = (uint8_t)constrain(rx, 0, 255) >> 3;
    uint8_t y = (uint8_t)constrain(ry, 0, 255) >> 3;

    int16_t v = 0;
    uint8_t wibble = sin8(auroraPlasmaTime);
    v += sin16(x * wibble * 2 + auroraPlasmaTime);
    v += cos16(y * (128 - wibble) * 2 + auroraPlasmaTime);
    v += sin16(y * x * cos8(-auroraPlasmaTime) / 2);

    leds[i] = ColorFromPalette(gCurrentPalette, (v >> 8) + 127);
  }

  auroraPlasmaTime += 1;
  auroraPlasmaFrames++;

  if (auroraPlasmaFrames >= 2048) {
    auroraPlasmaTime = 0;
    auroraPlasmaFrames = 0;
  }
}

#endif
