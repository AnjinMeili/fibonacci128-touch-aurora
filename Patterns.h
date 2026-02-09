
// PlasmaAnimation - adapted from PlazINT by Edmund "Skorn" Horn
// Original: https://github.com/marcmerlin/FastLED_NeoMatrix_SmartMatrix_LEDMatrix_GFX_Demos/tree/master/FastLED/PlasmaAnimation
// Adapted for Fibonacci128 radial LED layout using coordsX/coordsY mapping.

//Byte val 2PI Cosine Wave, offset by 1 PI
//supports fast trig calcs and smooth LED fading/pulsing.
uint8_t const cos_wave[256] PROGMEM =
{0,0,0,0,1,1,1,2,2,3,4,5,6,6,8,9,10,11,12,14,15,17,18,20,22,23,25,27,29,31,33,35,38,40,42,
45,47,49,52,54,57,60,62,65,68,71,73,76,79,82,85,88,91,94,97,100,103,106,109,113,116,119,
122,125,128,131,135,138,141,144,147,150,153,156,159,162,165,168,171,174,177,180,183,186,
189,191,194,197,199,202,204,207,209,212,214,216,218,221,223,225,227,229,231,232,234,236,
238,239,241,242,243,245,246,247,248,249,250,251,252,252,253,253,254,254,255,255,255,255,
255,255,255,255,254,254,253,253,252,252,251,250,249,248,247,246,245,243,242,241,239,238,
236,234,232,231,229,227,225,223,221,218,216,214,212,209,207,204,202,199,197,194,191,189,
186,183,180,177,174,171,168,165,162,159,156,153,150,147,144,141,138,135,131,128,125,122,
119,116,113,109,106,103,100,97,94,91,88,85,82,79,76,73,71,68,65,62,60,57,54,52,49,47,45,
42,40,38,35,33,31,29,27,25,23,22,20,18,17,15,14,12,11,10,9,8,6,6,5,4,3,2,2,1,1,1,0,0,0,0
};

//Gamma Correction Curve
uint8_t const exp_gamma[256] PROGMEM =
{0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,3,3,3,3,3,
4,4,4,4,4,5,5,5,5,5,6,6,6,7,7,7,7,8,8,8,9,9,9,10,10,10,11,11,12,12,12,13,13,14,14,14,15,15,
16,16,17,17,18,18,19,19,20,20,21,21,22,23,23,24,24,25,26,26,27,28,28,29,30,30,31,32,32,33,
34,35,35,36,37,38,39,39,40,41,42,43,44,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,
61,62,63,64,65,66,67,68,70,71,72,73,74,75,77,78,79,80,82,83,84,85,87,89,91,92,93,95,96,98,
99,100,101,102,105,106,108,109,111,112,114,115,117,118,120,121,123,125,126,128,130,131,133,
135,136,138,140,142,143,145,147,149,151,152,154,156,158,160,162,164,165,167,169,171,173,175,
177,179,181,183,185,187,190,192,194,196,198,200,202,204,207,209,211,213,216,218,220,222,225,
227,229,232,234,236,239,241,244,246,249,251,253,254,255
};

inline uint8_t fastCosineCalc(uint16_t preWrapVal)
{
  int16_t wrapVal = (preWrapVal % 255);
  if (wrapVal < 0) wrapVal = 255 + wrapVal;
  return (pgm_read_byte_near(cos_wave + wrapVal));
}

void plasmaAnimation()
{
  static unsigned long frameCount = 25500; // arbitrary seed for time displacement
  frameCount++;

  uint16_t t  = fastCosineCalc((42 * frameCount) / 100);
  uint16_t t2 = fastCosineCalc((35 * frameCount) / 100);
  uint16_t t3 = fastCosineCalc((38 * frameCount) / 100);

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    uint8_t x = coordsX[i];
    uint8_t y = coordsY[i];

    // Calculate 3 separate plasma waves, one for each color channel
    // coordsX/coordsY already span 0-255, so no shift needed (the original
    // used x<<3 to scale small grid indices into the 0-255 cosine table)
    uint8_t r = fastCosineCalc(((x) + (t >> 1) + fastCosineCalc((t2 + (y)))));
    uint8_t g = fastCosineCalc(((y) + t + fastCosineCalc(((t3 >> 2) + (x)))));
    uint8_t b = fastCosineCalc(((y) + t2 + fastCosineCalc((t + x + (g >> 2)))));

    // gamma correction for richer colors
    r = pgm_read_byte_near(exp_gamma + r);
    g = pgm_read_byte_near(exp_gamma + g);
    b = pgm_read_byte_near(exp_gamma + b);

    leds[i] = CRGB(r, g, b);
  }
}

// Rotating Cube - adapted from Aurora PatternCube
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon, Noel Bundy, Windell H Oskay
// Adapted for Fibonacci128 radial LED layout - wireframe cube projected onto scattered LEDs.
// Instead of drawing lines on a grid, each LED is colored by its proximity to projected edges.

void cubeAnimation()
{
  static float Angx = 20.0f, Angy = 10.0f;
  static uint8_t cubeHue = 0;
  static uint8_t cubeStep = 0;

  const float cubeW = 80.0f;       // cube half-width in 3D space
  const float focal = 100.0f;      // camera focal length
  const float Ox = 127.5f, Oy = 127.5f; // center of 0-255 coordinate space

  // Organic rotation speeds via beat functions
  float AngxSpeed = beatsin8(3, 1, 5) / 100.0f;
  float AngySpeed = beatsin8(5, 1, 5, 0, 64) / 100.0f; // phase offset = cosine-like

  float zCamera = (float)beatsin8(2, 120, 200);

  Angx += AngxSpeed;
  Angy += AngySpeed;
  if (Angx >= TWO_PI) Angx -= TWO_PI;
  if (Angy >= TWO_PI) Angy -= TWO_PI;

  // Rotation matrix
  float cosx = cos(Angx), sinx = sin(Angx);
  float cosy = cos(Angy), siny = sin(Angy);

  // 8 cube vertices in local space
  static const float lx[8] = {-1, 1, 1,-1,-1, 1, 1,-1};
  static const float ly[8] = { 1, 1,-1,-1, 1, 1,-1,-1};
  static const float lz[8] = { 1, 1, 1, 1,-1,-1,-1,-1};

  // Project vertices to 2D (integer coords for fast LED distance calc)
  int16_t sxi[8], syi[8];
  for (uint8_t i = 0; i < 8; i++) {
    float vx = lx[i] * cubeW, vy = ly[i] * cubeW, vz = lz[i] * cubeW;
    float ax = cosy * vx + (-siny) * vz;
    float ay = sinx * siny * vx + cosx * vy + sinx * cosy * vz;
    float az = cosx * siny * vx + (-sinx) * vy + cosx * cosy * vz + zCamera;
    sxi[i] = (int16_t)(Ox + focal * ax / az);
    syi[i] = (int16_t)(Oy - focal * ay / az);
  }

  // 12 edges (vertex index pairs)
  static const uint8_t edgeVerts[12][2] = {
    {0,1}, {1,2}, {2,3}, {3,0},   // front face
    {4,5}, {5,6}, {6,7}, {7,4},   // back face
    {0,4}, {1,5}, {2,6}, {3,7}    // connecting
  };

  // 6 faces (4 vertex indices each, wound consistently)
  static const uint8_t faceVerts[6][4] = {
    {1,0,3,2}, {0,4,7,3}, {4,0,1,5},
    {4,5,6,7}, {1,2,6,5}, {2,3,7,6}
  };

  // Determine front-facing edges via face normal cross product
  bool edgeVisible[12];
  memset(edgeVisible, 0, sizeof(edgeVisible));

  for (uint8_t f = 0; f < 6; f++) {
    int32_t cross = (int32_t)(sxi[faceVerts[f][1]] - sxi[faceVerts[f][0]]) *
                              (syi[faceVerts[f][2]] - syi[faceVerts[f][0]]) -
                    (int32_t)(syi[faceVerts[f][1]] - syi[faceVerts[f][0]]) *
                              (sxi[faceVerts[f][2]] - sxi[faceVerts[f][0]]);
    if (cross >= 0) { // front-facing
      for (uint8_t ei = 0; ei < 4; ei++) {
        uint8_t v0 = faceVerts[f][ei];
        uint8_t v1 = faceVerts[f][(ei + 1) % 4];
        for (uint8_t e = 0; e < 12; e++) {
          if ((edgeVerts[e][0] == v0 && edgeVerts[e][1] == v1) ||
              (edgeVerts[e][0] == v1 && edgeVerts[e][1] == v0)) {
            edgeVisible[e] = true;
            break;
          }
        }
      }
    }
  }

  fadeToBlackBy(leds, NUM_LEDS, 40);

  const int32_t thresholdSq = 900L; // 30^2 - proximity threshold squared

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    int16_t px = coordsX[i];
    int16_t py = coordsY[i];

    int32_t minDistSq = 999999L;
    bool nearestVis = false;

    for (uint8_t e = 0; e < 12; e++) {
      int16_t x1 = sxi[edgeVerts[e][0]], y1 = syi[edgeVerts[e][0]];
      int16_t x2 = sxi[edgeVerts[e][1]], y2 = syi[edgeVerts[e][1]];

      int32_t dx = x2 - x1, dy = y2 - y1;
      int32_t lenSq = dx * dx + dy * dy;

      int32_t closestX, closestY;
      if (lenSq < 1) {
        closestX = x1; closestY = y1;
      } else {
        int32_t dot = (int32_t)(px - x1) * dx + (int32_t)(py - y1) * dy;
        if (dot <= 0) {
          closestX = x1; closestY = y1;
        } else if (dot >= lenSq) {
          closestX = x2; closestY = y2;
        } else {
          closestX = x1 + (dot * dx) / lenSq;
          closestY = y1 + (dot * dy) / lenSq;
        }
      }

      int32_t ex = px - closestX, ey = py - closestY;
      int32_t distSq = ex * ex + ey * ey;

      if (distSq < minDistSq) {
        minDistSq = distSq;
        nearestVis = edgeVisible[e];
      }
    }

    if (minDistSq < thresholdSq) {
      // Quadratic brightness falloff - no sqrt needed
      uint8_t bri = (uint8_t)(255L * (thresholdSq - minDistSq) / thresholdSq);
      if (!nearestVis) bri /= 3; // back-face edges are dimmer
      leds[i] += CHSV(cubeHue, 255, bri);
    }
  }

  cubeStep++;
  if (cubeStep >= 8) {
    cubeStep = 0;
    cubeHue += 3;
  }
}

// Flocking Animation - adapted from Aurora PatternFlock
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon, Daniel Shiffman
// Based on Craig Reynolds' "Flocking" behavior (Separation, Cohesion, Alignment)
// Adapted for Fibonacci128 - boids simulated in 0-255 space, LEDs lit by proximity.

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

// Attract - adapted from Aurora PatternAttract
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon, Daniel Shiffman (Nature of Code)
// Boids orbit a gravitational attractor at the center, creating swirling trails.
// Adapted for Fibonacci128 - reuses FVec/FBoid, proximity-based LED rendering.

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

// Incremental Drift - adapted from Aurora PatternIncrementalDrift
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon
// Concentric rings of dots orbiting at incrementally different speeds.
// Adapted for Fibonacci128 - orbits scaled to 0-255 space, proximity-based LED rendering.

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

// Incremental Drift Rose - adapted from Aurora PatternIncrementalDrift2
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon
// Two mirrored groups of orbiting dots creating a rose/flower pattern.
// Adapted for Fibonacci128 - orbits scaled to 0-255 space, proximity-based LED rendering.

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

// --- Pattern Rotation System ---
// Maximum rotation speed in radians per frame (tune this to taste)
float patternMaxRotSpeed = 0.05f;

struct PatternRotation {
  float angle;       // current rotation angle (radians)
  float phase;       // sine-wave phase for angular velocity oscillation
  float phaseSpeed;  // how fast the sine wave advances (controls oscillation period)
  float curMaxSpeed; // current peak angular velocity (randomized within patternMaxRotSpeed)
  bool initialized;

  void init() {
    angle = 0;
    phase = random(0, 628) / 100.0f; // random starting phase 0..2PI
    randomizeSpeeds();
    initialized = true;
  }

  void randomizeSpeeds() {
    // Phase speed: controls how quickly direction reverses (0.003..0.018 rad/frame)
    phaseSpeed = 0.003f + random(0, 150) / 10000.0f;
    // Peak angular velocity: 30-100% of the global max
    curMaxSpeed = patternMaxRotSpeed * (0.3f + random(0, 70) / 100.0f);
  }

  // Call once per frame. Returns current angle.
  float update() {
    if (!initialized) init();
    phase += phaseSpeed;
    if (phase >= TWO_PI) phase -= TWO_PI;
    float angVel = sinf(phase) * curMaxSpeed;
    angle += angVel;
    return angle;
  }
};

// Electric Mandala - adapted from Aurora PatternElectricMandala
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon, Stefan Petrick (FunkyNoise)
// Perlin noise with kaleidoscope symmetry (diagonal mirror + 4-fold rotation).
// Adapted for Fibonacci128 - symmetry applied as coordinate folds before noise lookup.

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

// Aurora Plasma - adapted from PatternPlasma by Robert Atkins / Jason Coon
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon, (c) 2013 Robert Atkins
// Overlapping sin/cos waves colored through the current palette.
// Adapted for Fibonacci128 - iterates over LED coordinates instead of grid.

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

// ColorWavesWithPalettes by Mark Kriegsman: https://gist.github.com/kriegsman/8281905786e8b2632aeb
// This function draws color waves with an ever-changing,
// widely-varying set of parameters, using a color palette.
void fillWithColorWaves(CRGB* ledarray, uint16_t numleds, CRGBPalette16& palette, bool useFibonacciOrder) {
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  // uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 300, 1500);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for ( uint16_t i = 0 ; i < numleds; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;
    uint16_t h16_128 = hue16 >> 7;
    if ( h16_128 & 0x100) {
      hue8 = 255 - (h16_128 >> 1);
    } else {
      hue8 = h16_128 >> 1;
    }

    brightnesstheta16 += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    uint8_t index = hue8;
    //index = triwave8( index);
    index = scale8( index, 240);

    CRGB newcolor = ColorFromPalette( palette, index, bri8);

    uint16_t pixelnumber = i;

    if (useFibonacciOrder) pixelnumber = fibonacciToPhysical[i];

    pixelnumber = (numleds - 1) - pixelnumber;

    nblend(ledarray[pixelnumber], newcolor, 128);
  }
}

void colorWavesFibonacci() {
  fillWithColorWaves(leds, NUM_LEDS, gCurrentPalette, true);
}

// Pride2015 by Mark Kriegsman: https://gist.github.com/kriegsman/964de772d64c502760e5
// This function draws rainbows with an ever-changing,
// widely-varying set of parameters.
void fillWithPride(bool useFibonacciOrder)
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  // uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t sat8 = beatsin88( 43.5, 220, 250);
  // uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint8_t brightdepth = beatsin88(171, 96, 224);
  // uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint16_t brightnessthetainc16 = beatsin88( 102, (25 * 256), (40 * 256));
  // uint8_t msmultiplier = beatsin88(147, 23, 60);
  uint8_t msmultiplier = beatsin88(74, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  // uint16_t hueinc16 = beatsin88(113, 1, 3000);
  uint16_t hueinc16 = beatsin88(57, 1, 128);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  // sHue16 += deltams * beatsin88( 400, 5, 9);
  sHue16 += deltams * beatsin88( 200, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for ( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    CRGB newcolor = CHSV( hue8, sat8, bri8);

    uint16_t pixelnumber = i;

    if (useFibonacciOrder) pixelnumber = fibonacciToPhysical[i];

    pixelnumber = (NUM_LEDS - 1) - pixelnumber;

    nblend( leds[pixelnumber], newcolor, 64);
  }
}

void prideFibonacci() {
  fillWithPride(true);
}

void outwardPalettes() {
  for (uint16_t i = 0; i < NUM_LEDS; i++) { 
    leds[i] = ColorFromPalette(gCurrentPalette, physicalToFibonacci[i] - hue);
  }
}

void rotatingPalettes() {
  for (uint16_t i = 0; i < NUM_LEDS; i++) { 
    leds[i] = ColorFromPalette(gCurrentPalette, angles[i] - hue);
  }
}

void outwardRainbow() {
  for (uint16_t i = 0; i < NUM_LEDS; i++) { 
    leds[i] = CHSV(physicalToFibonacci[i] - hue, 255, 255);
  }
}

void rotatingRainbow() {
  for (uint16_t i = 0; i < NUM_LEDS; i++) { 
    leds[i] = CHSV(angles[i] - hue, 255, 255);
  }
}

void horizontalRainbow() {
  for (uint16_t i = 0; i < NUM_LEDS; i++) { 
    leds[i] = CHSV(coordsX[i] + hue, 255, 255);
  }
}

void verticalRainbow() {
  for (uint16_t i = 0; i < NUM_LEDS; i++) { 
    leds[i] = CHSV(coordsY[i] + hue, 255, 255);
  }
}

void diagonalRainbow() {
  for (uint16_t i = 0; i < NUM_LEDS; i++) { 
    leds[i] = CHSV(coordsX[i] + coordsY[i] + hue, 255, 255);
  }
}

void colorTest() {
  CRGB colors[] = { CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::White, CRGB::Black };
  const uint8_t colorCount = ARRAY_SIZE(colors);
  static uint8_t colorIndex = 0;
  EVERY_N_SECONDS(2) { colorIndex = (colorIndex + 1) % colorCount; }
  fill_solid(leds, NUM_LEDS, colors[colorIndex]);
}