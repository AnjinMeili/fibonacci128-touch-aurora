// Fibonacci Spiral Bloom
// A mesmerizing animation that follows the natural Fibonacci spiral arms.
// Two overlapping spiral arm systems (e.g., 5-arm and 8-arm) create organic
// moiré interference patterns. Light pulses travel outward along the golden-angle
// ordering while colors shift through golden-ratio harmonics.
//
// Uses physicalToFibonacci[] to map each LED to its position in the Fibonacci
// sequence, revealing the spiral structure inherent in the hardware layout.

#ifndef PatternFibonacciSpiral_H
#define PatternFibonacciSpiral_H

void fibonacciSpiralAnimation()
{
  // Pairs of Fibonacci spiral arm counts that create beautiful interference
  static const uint8_t armPairs[][2] = {{5, 8}, {8, 13}, {3, 5}, {5, 13}, {3, 8}, {8, 21}};
  static uint8_t pairIdx = 0;
  static uint8_t bloomPhase = 0;

  EVERY_N_SECONDS(8) {
    pairIdx = (pairIdx + 1) % 6;
  }

  fadeToBlackBy(leds, NUM_LEDS, 30);

  uint8_t arms1 = armPairs[pairIdx][0];
  uint8_t arms2 = armPairs[pairIdx][1];

  uint16_t ms = millis();
  uint8_t timeA = ms / 11;  // outward pulse phase
  uint8_t timeB = ms / 17;  // counter-pulse phase
  uint8_t timeHue = ms / 37; // slow color rotation

  // Breathing modulation - center brightens and dims organically
  uint8_t breath = beatsin8(6, 60, 255);

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    uint8_t fibIdx = physicalToFibonacci[i];

    // Normalized position along spiral (0=center, 255=edge)
    uint8_t spiralPos = fibIdx * 2; // 0..254, wraps are fine for wave math

    // --- System 1: outward-traveling pulse along arms1 spiral arms ---
    uint8_t arm1Id = fibIdx % arms1;
    // Phase offset per arm so pulses are staggered
    uint8_t arm1Phase = arm1Id * (256 / arms1);
    uint8_t wave1 = sin8(spiralPos * 3 - timeA + arm1Phase);
    // Threshold to create distinct pulse peaks rather than uniform wash
    uint8_t bri1 = (wave1 > 140) ? (wave1 - 140) * 2 + 25 : 0;
    // Each arm has a distinct hue, golden-ratio spaced and slowly rotating
    uint8_t hue1 = arm1Phase + timeHue + (spiralPos >> 2);

    // --- System 2: counter-rotating pulse along arms2 spiral arms ---
    uint8_t arm2Id = fibIdx % arms2;
    uint8_t arm2Phase = arm2Id * (256 / arms2);
    uint8_t wave2 = sin8(spiralPos * 2 + timeB + arm2Phase);
    uint8_t bri2 = (wave2 > 150) ? (wave2 - 150) * 2 + 20 : 0;
    uint8_t hue2 = arm2Phase - timeHue + 128 + (spiralPos >> 3);

    // --- Breathing center bloom ---
    // LEDs closer to center (low radius) pulse with the breath
    uint8_t centerGlow = 0;
    if (radius[i] < 80) {
      uint8_t centerBri = scale8(80 - radius[i], breath);
      centerGlow = scale8(centerBri, 3); // gentle glow
      // Center bloom hue follows golden ratio offset from main
      CRGB bloom = CHSV(timeHue * 2 + 64, 200, centerGlow);
      leds[i] += bloom;
    }

    // --- Combine the two spiral systems ---
    CRGB c1 = CHSV(hue1, 240, bri1);
    CRGB c2 = CHSV(hue2, 220, bri2);
    leds[i] += c1;
    leds[i] += c2;
  }

  bloomPhase++;
}

// Fibonacci Spiral Chase - a simpler variant
// A single bright pulse chases along each spiral arm in sequence,
// creating an unmistakable spiral motion effect.

void fibonacciChaseAnimation()
{
  static uint8_t chaseArms = 5;
  static uint8_t chaseHueBase = 0;

  EVERY_N_SECONDS(12) {
    // Cycle through Fibonacci arm counts
    static const uint8_t armOptions[] = {3, 5, 8, 13};
    static uint8_t armIdx = 0;
    armIdx = (armIdx + 1) % 4;
    chaseArms = armOptions[armIdx];
  }

  fadeToBlackBy(leds, NUM_LEDS, 40);

  uint16_t ms = millis();
  uint8_t maxArmLen = 128 / chaseArms;

  for (uint8_t arm = 0; arm < chaseArms; arm++) {
    // Each arm's bright dot position oscillates along the arm length
    // Stagger the phase per arm so they chase in sequence
    uint8_t armPhaseOffset = arm * (256 / chaseArms);
    uint8_t dotPos = beatsin8(20, 0, maxArmLen - 1, 0, armPhaseOffset);

    // Walk through all LEDs on this arm
    for (uint8_t pos = 0; pos < maxArmLen; pos++) {
      uint8_t fibIdx = arm + pos * chaseArms;
      if (fibIdx >= 128) break;
      uint8_t physIdx = fibonacciToPhysical[fibIdx];

      // Distance from the bright dot
      int8_t dist = (int8_t)pos - (int8_t)dotPos;
      if (dist < 0) dist = -dist;

      if (dist < 5) {
        uint8_t bri = 255 - dist * 50;
        uint8_t hueVal = chaseHueBase + arm * (256 / chaseArms) + pos * 4;
        leds[physIdx] += CHSV(hueVal, 255, bri);
      }
    }
  }

  EVERY_N_MILLISECONDS(20) { chaseHueBase++; }
}

// Phyllotaxis Bloom - inspired by the golden angle arrangement of seeds in sunflowers.
//
// Each LED is colored using the golden angle (137.5°) in hue space, exactly as
// sunflower seeds are arranged 137.5° apart in physical space. This spacing is
// irrational relative to 360°, so colors never cluster or repeat — producing the
// maximally uniform, non-repeating distribution seen in nature.
//
// Two interference waves (5-arm and 8-arm — consecutive Fibonacci numbers) travel
// along the spiral arms at phi-ratio speeds (time divisors 8 and 13 — also
// consecutive Fibonacci numbers). They never phase-lock, producing ~40 drifting
// bright nodes where waves reinforce — like bioluminescent ripples on a living spiral.
//
// The 5×8 arm counts are exactly the parastichy numbers you count on a sunflower.

void phyllotaxisBloomAnimation()
{
  uint32_t ms = millis();

  // Phi-ratio time bases: consecutive Fibonacci divisors (8, 13) keep the two
  // waves drifting at ~phi relative speed — they never lock into a repeating pattern.
  uint8_t timeA   = (uint8_t)(ms / 8);   // 5-arm wave: travels outward
  uint8_t timeB   = (uint8_t)(ms / 13);  // 8-arm wave: travels inward (phi-ratio slower)
  uint8_t timeHue = (uint8_t)(ms / 80);  // hue rotation: ~20s full spectrum cycle

  // Golden angle in 0-255 hue space: 137.508/360 * 256 = 97.8 → 98.
  // fibIdx * 98 places each successive Fibonacci LED 137.5° away in hue,
  // making 5-arm and 8-arm spiral arms visible as distinct color bands.
  const uint8_t GOLDEN_ANGLE_8 = 98;

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    uint8_t fibIdx = physicalToFibonacci[i];

    // Hue: golden-angle distribution reveals the spiral arm structure as color bands.
    // 5 Fibonacci steps ≈ 5×137.5° = 687.5° ≡ 327.5° — one full lap minus a small gap,
    // so every 5th LED is nearly the same hue, tracing the 5 spiral arms.
    // Similarly for 8 arms. Exactly the parastichy visible in a sunflower.
    uint8_t hue = (uint8_t)(fibIdx * GOLDEN_ANGLE_8) + timeHue;

    // 5-arm spiral wave: spatial frequency 5 places one crest per spiral arm.
    // All 5 arms brighten and dim together as timeA advances outward.
    uint8_t wave1 = sin8(fibIdx * 5 - timeA);

    // 8-arm counter-wave: travels inward at phi-ratio slower speed.
    // Spatial freq 8 = one crest per 8-arm spiral arm.
    uint8_t wave2 = sin8(fibIdx * 8 + timeB);

    // Multiplicative interference: bright only where both waves crest simultaneously.
    // Produces ~40 drifting nodes (5×8 intersections) that travel along the arms —
    // like the growing tip of each spiral arm lighting up in sequence.
    uint8_t bri = scale8(wave1, wave2);

    // Soft glow floor: dark regions stay gently lit, like a starfield behind the bloom.
    bri = qadd8(bri, 15);

    // Saturation dip at brightness peaks: white-tipped "petal" effect.
    // Bright nodes bloom toward white before fading back to saturated color.
    uint8_t sat = 255 - scale8(bri, 80);

    leds[i] = CHSV(hue, sat, bri);
  }
}

#endif
