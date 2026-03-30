# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Arduino firmware for the **Fibonacci128 Pico LED array** — a 25.4mm (1-inch) circular PCB with 128 XL-1010RGBC-WS2812B RGB LEDs (1mm²) arranged in a Fermat spiral (phyllotaxis) pattern, with 3 capacitive touch pads. Targets the Adafruit QT Py (SAMD21 microcontroller). Built with PlatformIO.

Hardware reference: https://www.evilgeniuslabs.org/one-inch-fibonacci128

## Build Commands

```bash
# Build firmware
pio run

# Build and upload to device
pio run -t upload

# Open serial monitor
pio run -t monitor
```

Source files live in `./fibonacci128/`. PlatformIO outputs build artifacts to `.pio/` (gitignored).

## Dependencies

Managed via `platformio.ini`:
- **FastLED v3.5.0** — LED animation and control
- **Adafruit FreeTouch Library v1.1.1** — capacitive touch sensing
- **FlashStorage_SAMD** — EEPROM/flash persistence for pattern index (referenced in code via `#include <FlashStorage_SAMD.h>` but not listed in `platformio.ini` `lib_deps`)

## Architecture

### Main Loop (`fibonacci128.ino`)

```
setup()
├── Initialize 3 capacitive touch pads (pins A3, A6, A7)
├── Initialize FastLED (128 WS2812B LEDs on pin A10, max 1400mA)
└── Load saved pattern index from EEPROM (advances index on each boot)

loop() [120 FPS target]
├── handleTouch()     — read/normalize capacitive sensor values
├── Pattern dispatch  — call current pattern function from patterns[] pointer array
├── touchDemo()       — overlay expanding circle animations from touch events
├── Palette blending  — smooth transitions between color palettes (every 40ms)
├── Palette cycling   — switch target palette every 10 seconds
└── FastLED.delay(8)  — frame rate control (~120 FPS)
```

### Source Files

| File | Role |
|------|------|
| `fibonacci128.ino` | Entry point: setup/loop, touch handling, pattern cycling, circle drawing |
| `Patterns.h` | Core animation patterns + PatternRotation system + helper functions |
| `PatternSublime.h` | Juggle, BPM, sinelon, fire, rain patterns (adapted from Sublime Demos) |
| `PatternFibonacciSpiral.h` | Fibonacci spiral-specific animations using golden-angle ordering |
| `PatternWave.h` | Wave/particle effects with proximity-based rendering |
| `GradientPalettes.h` | 34 pre-defined cpt-city gradient color palettes |
| `Map.h` | LED coordinate tables: `coordsX[]`, `coordsY[]`, `angles[]`, `radius[]`, `physicalToFibonacci[]`, `fibonacciToPhysical[]` |

### Pattern Catalog (SimplePatternList — 27 patterns)

Patterns are registered in the `patterns[]` function pointer array in the `.ino` file. The current pattern index auto-advances on each device reset and is persisted to EEPROM.

| # | Function | File | Description |
|---|----------|------|-------------|
| 0 | `fibonacciSpiralAnimation` | PatternFibonacciSpiral.h | Two overlapping Fibonacci spiral arm systems (e.g. 5+8 arms) creating moiré interference with breathing center bloom |
| 1 | `fibonacciChaseAnimation` | PatternFibonacciSpiral.h | Bright pulses chase along each Fibonacci spiral arm in sequence |
| 2 | `sublimeVerticalFireAnimation` | PatternSublime.h | Particle-based embers rising bottom-to-top with HeatColors palette |
| 3 | `sublimeFireAnimation` | PatternSublime.h | Radial fire simulation — heat rises from edge toward center with angular Perlin noise |
| 4 | `sublimeRainAnimation` | PatternSublime.h | Matrix-style green drops falling top-to-bottom with fading trails |
| 5 | `sublimeJuggleAnimation` | PatternSublime.h | Colored dots weaving in/out of sync along radial paths, parameters shift every 10s |
| 6 | `sublimeBpmAnimation` | PatternSublime.h | Radial bands pulsing outward at 62 BPM, colored through current palette |
| 7 | `sublimeSinelonAnimation` | PatternSublime.h | Single colored dot sweeping center-to-edge and back with fading trails |
| 8 | `waveAnimation` | PatternWave.h | Quadwave dots along sine curves with random orientation, mirroring, and slow rotation |
| 9 | `swirlAnimation` | Patterns.h | Six symmetric bouncing dots with fading trails |
| 10 | `spiralAnimation` | Patterns.h | Two rotating spiral arms with oscillating tightness and width |
| 11 | `radarAnimation` | Patterns.h | Sweeping radar arm with long fading trail, colored by radius |
| 12 | `pendulumWaveAnimation` | Patterns.h | 32 pendulum dots swinging at incrementally different frequencies |
| 13 | `incrementalDrift2Animation` | Patterns.h | Two mirrored groups of orbiting dots creating rose/flower patterns |
| 14 | `incrementalDriftAnimation` | Patterns.h | Concentric rings of dots orbiting at incrementally different speeds |
| 15 | `attractAnimation` | Patterns.h | Boids orbiting a gravitational attractor at center with swirling trails |
| 16 | `electricMandalaAnimation` | Patterns.h | Perlin noise with kaleidoscope symmetry (4-fold rotation + diagonal mirror) and slow rotation |
| 17 | `auroraPlasmaAnimation` | Patterns.h | Overlapping sin/cos plasma waves colored through current palette with rotation |
| 18 | `flockAnimation` | Patterns.h | Boids flocking simulation with separation/cohesion/alignment + predator |
| 19 | `cubeAnimation` | Patterns.h | Wireframe 3D cube projected onto LEDs with back-face culling and organic rotation |
| 20 | `plasmaAnimation` | Patterns.h | Classic RGB plasma using cos_wave lookup table with gamma correction |
| 21 | `colorWavesFibonacci` | Patterns.h | Mark Kriegsman's color waves rendered in Fibonacci spiral order |
| 22 | `prideFibonacci` | Patterns.h | Pride2015 rainbow animation rendered in Fibonacci spiral order |
| 23 | `outwardPalettes` | Patterns.h | Current palette mapped outward along Fibonacci index |
| 24 | `rotatingPalettes` | Patterns.h | Current palette mapped by angle (rotating) |
| 25 | `outwardRainbow` | Patterns.h | HSV rainbow mapped outward along Fibonacci index |
| 26 | `rotatingRainbow` | Patterns.h | HSV rainbow mapped by angle (rotating) |

**Additional pattern functions** (defined but not in `patterns[]`):
- `horizontalRainbow` — HSV rainbow mapped by X coordinate
- `verticalRainbow` — HSV rainbow mapped by Y coordinate
- `diagonalRainbow` — HSV rainbow mapped by X+Y
- `colorTest` — Cycles through solid R/G/B/W/Black every 2s (diagnostic)

### Key Design Patterns

**Pattern system:** A `patterns[]` array of function pointers (27 entries) drives animation selection. The current index is persisted to EEPROM and advances on each device reset. Pattern names are stored in a parallel `patternNames[]` array for serial output.

**Proximity-based rendering:** Since LEDs are scattered (not on a grid), most patterns work by computing virtual object positions (dots, edges, particles) then lighting each LED based on squared-distance proximity (`dSq < proxSq`). This avoids sqrt and enables soft falloff.

**PatternRotation system:** A reusable struct (`Patterns.h:781`) that adds slow, organic rotation to patterns. Uses sinusoidal angular velocity (speeds up, slows, pauses, reverses) with randomized parameters, refreshed every 8 seconds.

**Touch system:** Three `Adafruit_FreeTouch` sensors (pins A3, A6, A7) are calibrated via min/max arrays and mapped to XY coordinates in 0–255 space. Touch events produce expanding circle overlays via `touchDemo()`. When waves are active, the current pattern is suppressed.

**LED mapping:** Four coordinate arrays in `Map.h` map physical LED indices:
- `coordsX[]`/`coordsY[]` — 2D Cartesian position (0–255)
- `angles[]` — angular position (0–255 = 0°–360°)
- `radius[]` — distance from center (0–255)
- `physicalToFibonacci[]`/`fibonacciToPhysical[]` — bidirectional mapping between physical LED index and Fibonacci spiral order

**Color palettes:** 34 cpt-city gradient palettes in `GradientPalettes.h` are blended smoothly with `nblendPaletteTowardPalette()`. Target palette auto-cycles every 10 seconds.
