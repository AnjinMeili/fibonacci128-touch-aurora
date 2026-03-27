"""
Phyllotaxis Bloom simulator for Fibonacci128 LED disc.

Faithfully translates the Arduino C++ phyllotaxisBloomAnimation() into Python,
then renders each frame as a 600×600 image with per-LED glow halos and writes
the sequence to an MP4 via imageio-ffmpeg.

LED rendering model
-------------------
Each of the 128 LEDs is an 8-bit HSV colour (from the pattern) converted to
linear-light RGB, then painted onto the canvas as a soft radial glow using a
2-D Gaussian kernel.  Neighbouring halos add together (like real LEDs do when
you photograph them), so bright clusters bloom naturally.
"""

import math
import struct
import numpy as np
import imageio
from PIL import Image

# ---------------------------------------------------------------------------
# Map.h data — verbatim from the firmware
# ---------------------------------------------------------------------------

physicalToFibonacci = [
    0,21,42,63,84,105,126,113,92,71,50,29,8,16,37,58,79,100,121,108,87,66,45,
    24,3,11,32,53,74,95,116,124,103,82,61,40,19,6,27,48,69,90,111,119,98,77,
    56,35,14,1,22,43,64,85,106,127,114,93,72,51,30,9,17,38,59,80,101,122,109,
    88,67,46,25,4,12,33,54,75,96,117,125,104,83,62,41,20,7,28,49,70,91,112,
    120,99,78,57,36,15,2,23,44,65,86,107,115,94,73,52,31,10,18,39,60,81,102,
    123,110,89,68,47,26,5,13,34,55,76,97,118,
]

coordsX = [
    137,181,201,213,219,221,218,193,198,199,195,184,160,164,176,178,175,167,
    154,127,142,152,158,157,142,140,140,132,119,103,85,45,65,84,101,116,126,
    121,106,89,70,51,32,9,25,43,63,83,104,118,84,62,42,25,11,0,11,19,32,47,
    68,96,81,58,43,33,27,26,49,47,50,58,74,105,94,77,70,69,74,83,125,110,99,
    91,90,96,114,108,112,121,135,152,193,174,156,141,129,123,130,141,155,172,
    191,210,239,221,202,182,164,145,164,186,206,225,241,255,248,237,222,204,
    182,151,170,196,214,227,236,241,
]

coordsY = [
    130,137,150,166,184,204,224,235,216,197,178,159,141,160,182,203,222,240,
    255,251,237,221,202,180,147,167,195,215,229,240,247,227,228,225,216,202,
    181,158,186,199,206,207,203,166,177,184,185,181,166,139,161,165,161,152,
    139,122,96,114,129,140,146,143,132,123,110,93,74,55,39,59,78,96,112,126,
    109,89,69,49,30,13,0,13,28,46,67,90,102,72,50,33,19,9,21,24,31,43,60,85,
    112,75,57,47,42,42,74,66,63,66,75,96,95,86,85,91,101,115,142,126,114,106,
    105,115,121,124,134,148,166,185,
]

NUM_LEDS = 128

# ---------------------------------------------------------------------------
# FastLED maths helpers (uint8_t arithmetic, 0-255)
# ---------------------------------------------------------------------------

def u8(x):
    """Wrap any integer into 0-255 (uint8_t behaviour)."""
    return int(x) & 0xFF

def sin8(x):
    """FastLED sin8: returns 0-255 sine of the 0-255 angle x."""
    return u8(round(127.5 + 127.5 * math.sin(2 * math.pi * u8(x) / 256)))

def scale8(a, b):
    """FastLED scale8: a * b / 255, result clamped to 0-255."""
    return u8((int(a) * int(b)) >> 8)

def qadd8(a, b):
    """FastLED qadd8: saturating add."""
    return min(255, int(a) + int(b))

# Precompute sin8 LUT for speed
_sin8_lut = np.array([
    round(127.5 + 127.5 * math.sin(2 * math.pi * i / 256))
    for i in range(256)
], dtype=np.uint8)

def sin8v(x_arr):
    """Vectorised sin8 over a numpy uint8 array."""
    return _sin8_lut[x_arr]

# ---------------------------------------------------------------------------
# HSV → linear-light RGB  (FastLED-accurate, no gamma)
# ---------------------------------------------------------------------------

def hsv_to_rgb(h, s, v):
    """
    FastLED-style HSV→RGB (section-based, identical to CHSV→CRGB).
    h, s, v are 0-255 integers.  Returns (r, g, b) each 0-255.
    """
    if s == 0:
        return (v, v, v)

    region   = h // 43
    remainder = (h - region * 43) * 6

    p = (v * (255 - s)) >> 8
    q = (v * (255 - ((s * remainder) >> 8))) >> 8
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8

    table = [
        (v, t, p),
        (q, v, p),
        (p, v, t),
        (p, q, v),
        (t, p, v),
        (v, p, q),
    ]
    r, g, b = table[region % 6]
    return (r & 0xFF, g & 0xFF, b & 0xFF)

# ---------------------------------------------------------------------------
# Pattern: phyllotaxisBloomAnimation
# ---------------------------------------------------------------------------

GOLDEN_ANGLE_8 = 98          # 137.508 / 360 * 256 ≈ 97.8 → 98

def compute_frame(ms):
    """
    Compute the 128 LED colours for a given millisecond timestamp.
    Returns a (128, 3) uint8 numpy array of linear RGB values.
    """
    time_a   = u8(ms // 8)
    time_b   = u8(ms // 13)
    time_hue = u8(ms // 80)

    rgb = np.zeros((NUM_LEDS, 3), dtype=np.uint8)

    for i in range(NUM_LEDS):
        fib_idx = physicalToFibonacci[i]

        hue  = u8(fib_idx * GOLDEN_ANGLE_8 + time_hue)
        w1   = sin8(u8(fib_idx * 5 - time_a))
        w2   = sin8(u8(fib_idx * 8 + time_b))
        bri  = scale8(w1, w2)
        bri  = qadd8(bri, 15)
        sat  = u8(255 - scale8(bri, 80))

        r, g, b = hsv_to_rgb(hue, sat, bri)
        rgb[i] = (r, g, b)

    return rgb

# ---------------------------------------------------------------------------
# Renderer: map LEDs onto a canvas with Gaussian glow halos
# ---------------------------------------------------------------------------

CANVAS   = 600          # output image size (pixels)
LED_AREA = 512          # the 0-255 coord space occupies this many pixels
OFFSET   = (CANVAS - LED_AREA) // 2   # padding to centre the disc
SIGMA    = 10.0         # glow radius in pixels
HALO_R   = int(math.ceil(3 * SIGMA))  # kernel half-width

# Pre-compute Gaussian kernel
_ks = 2 * HALO_R + 1
_g  = np.arange(_ks) - HALO_R
_gx, _gy = np.meshgrid(_g, _g)
KERNEL = np.exp(-(_gx**2 + _gy**2) / (2 * SIGMA**2)).astype(np.float32)
# Normalise so a fully-on LED contributes its full brightness
KERNEL /= KERNEL.max()

# Pre-compute integer pixel positions for every LED
px = np.round(np.array(coordsX, dtype=np.float32) * (LED_AREA / 255) + OFFSET).astype(int)
py = np.round(np.array(coordsY, dtype=np.float32) * (LED_AREA / 255) + OFFSET).astype(int)

def render_frame(rgb):
    """
    Paint 128 LEDs (each with a Gaussian glow) onto a black canvas.
    rgb: (128, 3) uint8 array.
    Returns a (CANVAS, CANVAS, 3) uint8 numpy array.
    """
    canvas = np.zeros((CANVAS, CANVAS, 3), dtype=np.float32)

    for i in range(NUM_LEDS):
        cx, cy = px[i], py[i]
        r_f = rgb[i, 0] / 255.0
        g_f = rgb[i, 1] / 255.0
        b_f = rgb[i, 2] / 255.0

        # canvas region bounds (clamped to image)
        x0, x1 = cx - HALO_R, cx + HALO_R + 1
        y0, y1 = cy - HALO_R, cy + HALO_R + 1

        # kernel region to use if canvas is clipped
        kx0 = max(0, -x0);  kx1 = _ks - max(0, x1 - CANVAS)
        ky0 = max(0, -y0);  ky1 = _ks - max(0, y1 - CANVAS)
        x0 = max(0, x0);    x1 = min(CANVAS, x1)
        y0 = max(0, y0);    y1 = min(CANVAS, y1)

        k = KERNEL[ky0:ky1, kx0:kx1]
        canvas[y0:y1, x0:x1, 0] += k * r_f
        canvas[y0:y1, x0:x1, 1] += k * g_f
        canvas[y0:y1, x0:x1, 2] += k * b_f

    # Exposure: soft tone-map so overlapping halos don't clip to white
    canvas = 1.0 - np.exp(-canvas * 1.6)

    # Apply gentle gamma for display (sRGB ≈ 2.2)
    canvas = np.power(np.clip(canvas, 0, 1), 1.0 / 2.2)

    return (canvas * 255).astype(np.uint8)

# ---------------------------------------------------------------------------
# Main: render video
# ---------------------------------------------------------------------------

FPS      = 60
DURATION = 24          # seconds — covers one full hue rotation (20 s) + buffer
N_FRAMES = FPS * DURATION
OUT_PATH = "/home/user/fibonacci128-touch-aurora/phyllotaxis_bloom.mp4"

print(f"Rendering {N_FRAMES} frames at {FPS} fps → {DURATION}s  ({CANVAS}×{CANVAS}px)")
print(f"Output: {OUT_PATH}")

writer = imageio.get_writer(
    OUT_PATH,
    fps=FPS,
    codec="libx264",
    quality=8,
    output_params=["-pix_fmt", "yuv420p"],
)

for frame_idx in range(N_FRAMES):
    ms = int(frame_idx * 1000 / FPS)
    rgb   = compute_frame(ms)
    image = render_frame(rgb)
    writer.append_data(image)

    if frame_idx % FPS == 0:
        print(f"  {frame_idx // FPS:3d}s / {DURATION}s")

writer.close()
print("Done.")
