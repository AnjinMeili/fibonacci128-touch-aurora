# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Arduino firmware for the **Fibonacci128 Pico LED array** — a radial 128-LED Fibonacci spiral with 3 capacitive touch pads. Targets the Adafruit QT Py (SAMD21 microcontroller). Built with PlatformIO.

## Build Commands

```bash
# Build firmware
pio run

# Build and upload to device
pio run -t upload

# Open serial monitor
pio run -t monitor
```

Source files live in `./fibonacci128-touch-demo/`. PlatformIO outputs build artifacts to `.pio/` (gitignored).

## Dependencies

Managed via `platformio.ini`:
- **FastLED v3.5.0** — LED animation and control
- **Adafruit FreeTouch Library v1.1.1** — capacitive touch sensing
- **FlashStorage_SAMD** — EEPROM/flash persistence for pattern index

## Architecture

### Main Loop (`fibonacci128-touch-aurora.ino`)

```
setup()
├── Initialize 3 capacitive touch pads (pins A3, A6, A7)
├── Initialize FastLED (128 WS2812B LEDs on pin A10, max 1400mA)
└── Load saved pattern index from EEPROM

loop() [120 FPS target]
├── handleTouch()     — read/normalize capacitive sensor values
├── Pattern dispatch  — call current pattern function from pointer array
├── touchDemo()       — overlay expanding circle animations from touch
├── Palette blending  — smooth transitions between color palettes
└── FastLED.delay(8)  — frame rate control
```

### Source Files

| File | Role |
|------|------|
| `fibonacci128-touch-aurora.ino` | Entry point: setup/loop, touch handling, pattern cycling |
| `Patterns.h` | 20+ animation patterns (plasma, fire, rain, cube, etc.) |
| `PatternSublime.h` | Juggle, fire, rain patterns |
| `PatternFibonacciSpiral.h` | Spiral-specific animations |
| `PatternWave.h` | Wave and particle effects |
| `GradientPalettes.h` | 34 pre-defined color palettes |
| `Map.h` | LED coordinate tables and XY mapping |

### Key Design Patterns

**Pattern system:** A `patterns[]` array of function pointers (26 entries) drives animation selection. The current index is persisted to EEPROM and advances on reset.

**Touch system:** Three `Adafruit_FreeTouch` sensors are calibrated via min/max arrays and mapped to XY coordinates in 0–255 space. Touch events produce expanding circle overlays via `touchDemo()`.

**LED mapping:** `coordsX[]`/`coordsY[]` arrays in `Map.h` map physical LED indices to 2D coordinates. Separate `physicalToFibonacci[]` / `fibonacciToPhysical[]` lookup tables allow addressing by Fibonacci spiral order.

**Color palettes:** 34 palettes in `GradientPalettes.h` are blended with `nblendPaletteTowardPalette()`. Palette auto-cycles every 10 seconds.
