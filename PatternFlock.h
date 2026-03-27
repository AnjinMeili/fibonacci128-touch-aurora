// Flocking Animation - adapted from Aurora PatternFlock
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon, Daniel Shiffman
// Based on Craig Reynolds' "Flocking" behavior (Separation, Cohesion, Alignment)
// Adapted for Fibonacci128 - boids simulated in 0-255 space, LEDs lit by proximity.

// Attract - adapted from Aurora PatternAttract
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon, Daniel Shiffman (Nature of Code)
// Boids orbit a gravitational attractor at the center, creating swirling trails.
// Adapted for Fibonacci128 - reuses FVec/FBoid, proximity-based LED rendering.

#ifndef PatternFlock_H
#define PatternFlock_H

struct FVec {
  float x, y;
  FVec() : x(0), y(0) {}
  FVec(float _x, float _y) : x(_x), y(_y) {}
  FVec operator+(const FVec& v) const { return FVec(x + v.x, y + v.y); }
  FVec operator-(const FVec& v) const { return FVec(x - v.x, y - v.y); }
  FVec& operator+=(const FVec& v) { x += v.x; y += v.y; return *this; }
  FVec& operator-=(const FVec& v) { x -= v.x; y -= v.y; return *this; }
  FVec operator*(float s) const { return FVec(x * s, y * s); }
  FVec& operator*=(float s) { x *= s; y *= s; return *this; }
  FVec& operator/=(float s) { x /= s; y /= s; return *this; }
  float mag() const { return sqrtf(x * x + y * y); }
  float magSq() const { return x * x + y * y; }
  float dist(const FVec& v) const { float dx = v.x - x, dy = v.y - y; return sqrtf(dx * dx + dy * dy); }
  FVec& normalize() { float m = mag(); if (m > 0) { x /= m; y /= m; } return *this; }
  void limit(float max) { if (magSq() > max * max) { normalize(); *this *= max; } }
};

#define FLOCK_COUNT 8

struct FBoid {
  FVec loc, vel, acc;
  float maxspeed, maxforce, desiredsep, neighbordist;
  bool enabled;

  void init(float x, float y, float ms, float mf, float ds, float nd) {
    acc = FVec(0, 0);
    vel = FVec((random(256) - 128) / 256.0f, (random(256) - 128) / 256.0f);
    loc = FVec(x, y);
    maxspeed = ms; maxforce = mf;
    desiredsep = ds; neighbordist = nd;
    enabled = true;
  }

  void applyForce(FVec f) { acc += f; }

  void update() {
    vel += acc;
    vel.limit(maxspeed);
    loc += vel;
    acc *= 0;
  }

  void wrap() {
    if (loc.x < 0) loc.x += 256;
    if (loc.y < 0) loc.y += 256;
    if (loc.x >= 256) loc.x -= 256;
    if (loc.y >= 256) loc.y -= 256;
  }

  void repelFrom(FVec obstacle, float radius) {
    FVec futPos = loc + vel;
    FVec d = obstacle - futPos;
    if (d.mag() <= radius) {
      FVec repel = loc - obstacle;
      repel.normalize();
      repel *= (maxforce * 7);
      applyForce(repel);
    }
  }

  FVec seek(FVec target) {
    FVec desired = target - loc;
    desired.normalize();
    desired *= maxspeed;
    FVec steer = desired - vel;
    steer.limit(maxforce);
    return steer;
  }

  // Combined separation, alignment, cohesion in one pass for efficiency
  void flock(FBoid boids[], uint8_t count) {
    FVec sep(0, 0), ali(0, 0), coh(0, 0);
    int sepCount = 0, flockCount = 0;

    for (uint8_t i = 0; i < count; i++) {
      if (!boids[i].enabled) continue;
      float d = loc.dist(boids[i].loc);
      if (d > 0 && d < desiredsep) {
        FVec diff = loc - boids[i].loc;
        diff.normalize();
        diff /= d;
        sep += diff;
        sepCount++;
      }
      if (d > 0 && d < neighbordist) {
        ali += boids[i].vel;
        coh += boids[i].loc;
        flockCount++;
      }
    }

    if (sepCount > 0) {
      sep /= (float)sepCount;
      if (sep.mag() > 0) { sep.normalize(); sep *= maxspeed; sep -= vel; sep.limit(maxforce); }
    }
    if (flockCount > 0) {
      ali /= (float)flockCount;
      ali.normalize(); ali *= maxspeed;
      FVec aliSteer = ali - vel; aliSteer.limit(maxforce); ali = aliSteer;
      coh /= (float)flockCount;
      coh = seek(coh);
    }

    sep *= 1.5f;
    applyForce(sep);
    applyForce(ali);
    applyForce(coh);
  }

  void run(FBoid boids[], uint8_t count) {
    flock(boids, count);
    update();
  }
};

static FBoid flockBoids[FLOCK_COUNT];
static FBoid flockPredator;
static bool flockInitialized = false;

void flockAnimation()
{
  // Initialize boids on first call
  if (!flockInitialized) {
    for (uint8_t i = 0; i < FLOCK_COUNT; i++) {
      // Speeds/distances scaled for 256x256 space (original was ~32x32)
      flockBoids[i].init(128, 128, 3.0f, 0.12f, 32.0f, 64.0f);
    }
    flockPredator.init(64, 64, 3.08f, 0.16f, 0.0f, 128.0f);
    flockInitialized = true;
  }

  fadeToBlackBy(leds, NUM_LEDS, 25); // equivalent to DimAll(230)

  static uint8_t flockHue = 0;
  EVERY_N_MILLIS(200) { flockHue++; }

  // Random wind gusts
  bool applyWind = random(256) > 250;
  FVec wind(0, 0);
  if (applyWind) {
    wind.x = (random(256) - 128) / 256.0f * 0.12f;
    wind.y = (random(256) - 128) / 256.0f * 0.12f;
  }

  // Simulate boids
  CRGB boidColor = ColorFromPalette(gCurrentPalette, flockHue);
  for (uint8_t b = 0; b < FLOCK_COUNT; b++) {
    flockBoids[b].repelFrom(flockPredator.loc, 80.0f);
    flockBoids[b].run(flockBoids, FLOCK_COUNT);
    flockBoids[b].wrap();
    if (applyWind) {
      flockBoids[b].applyForce(wind);
      applyWind = false;
    }
  }

  // Simulate predator
  flockPredator.run(flockBoids, FLOCK_COUNT);
  flockPredator.wrap();
  CRGB predColor = ColorFromPalette(gCurrentPalette, flockHue + 128);

  // Convert boid positions to integers for fast proximity check
  int16_t bxi[FLOCK_COUNT], byi[FLOCK_COUNT];
  for (uint8_t b = 0; b < FLOCK_COUNT; b++) {
    bxi[b] = (int16_t)flockBoids[b].loc.x;
    byi[b] = (int16_t)flockBoids[b].loc.y;
  }
  int16_t pxi = (int16_t)flockPredator.loc.x;
  int16_t pyi = (int16_t)flockPredator.loc.y;

  const int32_t proxSq = 400L; // 20^2 proximity threshold

  // Light LEDs near boids (single pass over LEDs for all boids)
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    int16_t lx = coordsX[i];
    int16_t ly = coordsY[i];

    for (uint8_t b = 0; b < FLOCK_COUNT; b++) {
      int16_t dx = lx - bxi[b];
      int16_t dy = ly - byi[b];
      int32_t dSq = (int32_t)dx * dx + (int32_t)dy * dy;
      if (dSq < proxSq) {
        uint8_t bri = (uint8_t)(255L * (proxSq - dSq) / proxSq);
        leds[i] += CRGB(scale8(boidColor.r, bri), scale8(boidColor.g, bri), scale8(boidColor.b, bri));
      }
    }

    // Predator in contrasting color
    int16_t dx = lx - pxi;
    int16_t dy = ly - pyi;
    int32_t dSq = (int32_t)dx * dx + (int32_t)dy * dy;
    if (dSq < proxSq) {
      uint8_t bri = (uint8_t)(255L * (proxSq - dSq) / proxSq);
      leds[i] += CRGB(scale8(predColor.r, bri), scale8(predColor.g, bri), scale8(predColor.b, bri));
    }
  }
}

#define ATTRACT_COUNT 10

void attractAnimation()
{
  static FBoid attractBoids[ATTRACT_COUNT];
  static uint8_t attractColors[ATTRACT_COUNT];
  static bool attractInit = false;

  if (!attractInit) {
    int dir = random(0, 2) == 0 ? -1 : 1;
    for (uint8_t i = 0; i < ATTRACT_COUNT; i++) {
      attractBoids[i].acc = FVec(0, 0);
      // Start in a vertical column near center (scaled for 0-255 space)
      attractBoids[i].loc = FVec(127, 255 - i * (256 / ATTRACT_COUNT));
      // Horizontal velocity, random magnitude, all same direction (scaled 8x from original)
      attractBoids[i].vel = FVec(dir * (1.6f + random(0, 48) / 10.0f), 0);
      attractBoids[i].maxspeed = 12.0f;  // 1.5 * 8
      attractBoids[i].maxforce = 10.0f;
      attractBoids[i].desiredsep = 0;
      attractBoids[i].neighbordist = 0;
      attractBoids[i].enabled = true;
      attractColors[i] = i * (240 / ATTRACT_COUNT);
    }
    attractInit = true;
  }

  // Oscillating dim for trailing effect (matches original)
  uint8_t dim = beatsin8(2, 170, 250);
  fadeToBlackBy(leds, NUM_LEDS, 255 - dim);

  // Gravitational attractor at center
  // G=0.5, M=10 in original 32x32 space; scaled by 512 for 256x256 space
  const FVec attractorLoc(128, 128);
  const float GM = 2560.0f;

  // Precompute boid colors and integer positions
  CRGB boidColors[ATTRACT_COUNT];
  int16_t bxi[ATTRACT_COUNT], byi[ATTRACT_COUNT];

  for (uint8_t i = 0; i < ATTRACT_COUNT; i++) {
    // Gravitational force toward center
    FVec force = attractorLoc - attractBoids[i].loc;
    float d = force.mag();
    if (d < 40.0f) d = 40.0f;   // min clamp (5 * 8)
    if (d > 256.0f) d = 256.0f; // max clamp (32 * 8)
    force.normalize();
    force *= GM / (d * d);

    attractBoids[i].applyForce(force);
    attractBoids[i].update();

    bxi[i] = (int16_t)attractBoids[i].loc.x;
    byi[i] = (int16_t)attractBoids[i].loc.y;
    boidColors[i] = ColorFromPalette(gCurrentPalette, attractColors[i]);
  }

  const int32_t proxSq = 400L; // 20^2

  // Single pass over LEDs for all boids
  for (uint16_t j = 0; j < NUM_LEDS; j++) {
    int16_t lx = coordsX[j];
    int16_t ly = coordsY[j];

    for (uint8_t i = 0; i < ATTRACT_COUNT; i++) {
      int16_t dx = lx - bxi[i];
      int16_t dy = ly - byi[i];
      int32_t dSq = (int32_t)dx * dx + (int32_t)dy * dy;
      if (dSq < proxSq) {
        uint8_t bri = (uint8_t)(255L * (proxSq - dSq) / proxSq);
        leds[j] += CRGB(scale8(boidColors[i].r, bri), scale8(boidColors[i].g, bri), scale8(boidColors[i].b, bri));
      }
    }
  }
}

#endif
