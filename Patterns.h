// --- Pattern Rotation System ---
// Shared helper used by PatternWave, PatternAurora.
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

#include "PatternPlasma.h"
#include "PatternCube.h"
#include "PatternFlock.h"
#include "PatternDrift.h"
#include "PatternRadar.h"
#include "PatternWave.h"
#include "PatternSublime.h"
#include "PatternFibonacciSpiral.h"
#include "PatternAurora.h"
#include "PatternPalette.h"
