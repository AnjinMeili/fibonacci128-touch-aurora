# Claude Code Work Log — 2026-03-26

Repository: `anjinmeili/fibonacci128-touch-aurora`
Branch: `dev` (development), `claude/init-project-sC9qe` (init)

---

## 1. Project Initialisation — CLAUDE.md

**Task:** Analyse the codebase and create a `CLAUDE.md` guidance file for future Claude Code sessions.

**Actions:**
- Explored all source files to understand the project structure
- Created `CLAUDE.md` covering:
  - Build commands (`pio run`, `pio run -t upload`, `pio run -t monitor`)
  - Dependencies with versions (FastLED 3.5.0, Adafruit FreeTouch 1.1.1, FlashStorage_SAMD)
  - Architecture overview: setup/loop flow, pattern dispatch, touch system, LED mapping, palette blending
  - Source file role table
  - Key design patterns (function pointer array, touch calibration, coordinate mapping, palette blending)
- Committed to branch `claude/init-project-sC9qe` and pushed to remote

**Files changed:** `CLAUDE.md` (created)

---

## 2. Branch Setup

**Task:** Create a new branch named `dev` for ongoing development.

**Note:** The name `claude` was unavailable — Git prevents a branch named `claude` while `claude/init-project-sC9qe` exists (namespace conflict in refs). Branch `dev` was created from `claude/init-project-sC9qe` and pushed.

**Branch:** `dev` ← `claude/init-project-sC9qe`

---

## 3. Pattern Refactor — Break Patterns.h into Per-Pattern Include Files

**Task:** Extract all patterns from the monolithic `Patterns.h` into separate `Pattern*.h` include files, following the convention already established by `PatternWave.h`, `PatternSublime.h`, and `PatternFibonacciSpiral.h`.

**Analysis:**
`Patterns.h` contained ~1100 lines across 10+ distinct patterns with no grouping. The existing separate files showed the expected format: `#ifndef`/`#define`/`#endif` include guards, grouped by theme.

**New files created:**

| File | Contents |
|---|---|
| `PatternPlasma.h` | `cos_wave[]`, `exp_gamma[]`, `fastCosineCalc()`, `plasmaAnimation()` |
| `PatternCube.h` | `cubeAnimation()` — 3D wireframe cube projection |
| `PatternFlock.h` | `FVec`, `FBoid` structs; `flockAnimation()`, `attractAnimation()` |
| `PatternDrift.h` | `beatcos8()`, `incrementalDriftAnimation()`, `incrementalDrift2Animation()`, `pendulumWaveAnimation()` |
| `PatternRadar.h` | `radarAnimation()`, `spiralAnimation()`, `swirlAnimation()` |
| `PatternAurora.h` | `electricMandalaAnimation()`, `auroraPlasmaAnimation()` |
| `PatternPalette.h` | `fillWithColorWaves()`, `colorWavesFibonacci()`, `fillWithPride()`, `prideFibonacci()`, `outwardPalettes()`, `rotatingPalettes()`, `outwardRainbow()`, `rotatingRainbow()`, `horizontalRainbow()`, `verticalRainbow()`, `diagonalRainbow()`, `colorTest()` |

**`Patterns.h` after refactor:** Reduced to the shared `PatternRotation` struct (required before `PatternWave.h` and `PatternAurora.h`) followed by `#include` directives for all ten pattern files.

**Dependency note:** `PatternRotation` is used by both `PatternWave.h` (pre-existing) and the new `PatternAurora.h`. It was retained in `Patterns.h` so it is defined before any include that references it.

**Files changed:** `Patterns.h` (rewritten), 7 new `Pattern*.h` files
**Commit:** `claude/init-project-sC9qe`, then merged into `dev`

---

## 4. New Pattern Design — `phyllotaxisBloomAnimation`

**Task:** Review existing patterns, research colour and motion patterns for Fibonacci spiral arrangements in nature and art, then design a new pattern with pleasing hue change and motion.

### Research findings

**Nature:**
- Sunflower seeds are placed at the **golden angle (137.508°)** — irrational relative to 360°, so positions never cluster or repeat; the result is maximum packing efficiency
- Counting spirals in sunflowers, pinecones, and phyllotaxis always yields consecutive Fibonacci numbers (e.g. 34 & 55, 55 & 89). These are called *parastichies*
- The golden ratio φ = (1+√5)/2 ≈ 1.618; the golden angle = 360°/φ² ≈ 137.508°
- Consecutive Fibonacci ratios converge to φ: 5/8 ≈ 0.625, 8/13 ≈ 0.615, 13/21 ≈ 0.619 → 1/φ ≈ 0.618

**For LED animation:**
- Assigning hue by `fibIdx × (golden_angle / 360° × 256)` distributes colours with the same irrational spacing, so no two adjacent spiral-arm LEDs share a hue — all 128 colours are maximally distinct
- Two sine waves at Fibonacci spatial frequencies (×5, ×8) produce one bright crest per spiral arm
- Running them at Fibonacci-ratio time speeds (÷8 ms, ÷13 ms) maintains a ~φ relative drift — they never phase-lock, so the interference pattern is always changing

### Pattern design: `phyllotaxisBloomAnimation`

**Colour:**
- `hue = fibIdx × 98 + timeHue`
- `98` = golden angle in 8-bit hue space (137.508/360 × 256 ≈ 97.8)
- Every 5th Fibonacci LED ≈ same hue → 5 visible colour bands = 5-arm spiral
- Every 8th Fibonacci LED ≈ same hue → 8 visible colour bands = 8-arm spiral
- `timeHue = ms / 80` → full spectrum rotation every ~20 seconds

**Motion:**
- `wave1 = sin8(fibIdx × 5 − timeA)` — outward, 5-arm spatial frequency
- `wave2 = sin8(fibIdx × 8 + timeB)` — inward, 8-arm spatial frequency
- `timeA = ms / 8`, `timeB = ms / 13` — Fibonacci-ratio relative speeds
- `bri = scale8(wave1, wave2)` — multiplicative: bright only where both waves crest → ~40 drifting intersection nodes

**Finish:**
- `bri = qadd8(bri, 15)` — soft glow floor (dark regions still faintly lit)
- `sat = 255 − scale8(bri, 80)` — saturation dip at peaks: white-tipped petal bloom quality

**Files changed:** `PatternFibonacciSpiral.h` (pattern added), `fibonacci128-touch-aurora.ino` (added to `patterns[]` and `patternNames[]` as index 0)

---

## 5. Simulation and Video Render

**Task:** Simulate the `phyllotaxisBloomAnimation` pattern and render to video.

**Approach:**
- Wrote `simulate.py` — a faithful Python translation of the Arduino pattern
- Identical uint8 arithmetic: `sin8` LUT, `scale8`, `qadd8`, FastLED-accurate HSV→RGB
- Same time bases and constants as the firmware
- LED rendering: each of the 128 LEDs painted as a radial Gaussian glow (σ=10 px) on a 600×600 black canvas; halos add additively then soft-clipped with `1 − e^(−x·1.6)` tone mapping; sRGB gamma applied
- Output: `phyllotaxis_bloom.mp4` via `imageio-ffmpeg` (H.264, 60 fps, 24 s)

**Output files:**
| File | Description |
|---|---|
| `simulate.py` | Simulation and render script |
| `phyllotaxis_bloom.mp4` | 24-second rendered video (6.7 MB, H.264, 60 fps) |
| `preview_3s.png` | Still frame at t=3 s |
| `preview_8s.png` | Still frame at t=8 s |
| `preview_16s.png` | Still frame at t=16 s |

**Visual results:** Golden-angle colour banding clearly reveals the 5- and 8-arm parastichy spiral structure. Multiplicative interference nodes drift outward/inward along the arms without ever repeating, demonstrating the φ-ratio irrational drift.

---

## Summary of commits

| Branch | Commit | Description |
|---|---|---|
| `claude/init-project-sC9qe` | `c951a74` | Add CLAUDE.md |
| `claude/init-project-sC9qe` | `525b71a` | Break patterns out into per-pattern include files |
| `dev` | `52df60d` | Add phyllotaxisBloom pattern |
| `dev` | `dc4930f` | Add Python simulator |
| `dev` | `6932a41` | Add rendered video and preview frames |
