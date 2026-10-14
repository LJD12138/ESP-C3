#!/usr/bin/env python3
"""Generate the Main_Booting loading-spinner GIF for the ESP-C3 EEZ Studio project.

Design notes
------------
* Style: Material-style spinner -- a dim full-circle track plus a bright arc
  that sweeps around it. Chosen because GIF only carries 1-bit transparency,
  so a "bright head + fading tail" cannot be rendered convincingly; an opaque
  dim track + opaque bright arc survives binary alpha perfectly.
* Output is 48x48 RGBA -> palette GIF with a transparent background.
* Supersampled 8x, LANCZOS downscale, then alpha thresholded at 50% so the
  1-bit alpha edge stays crisp instead of dithering.
* 12 frames x 30 deg == 360 deg, so the loop is seamless.
"""

import math
import os

from PIL import Image

SIZE = 48
FRAMES = 12
SS = 8  # supersample factor

RING_R = 16.5        # radius of the spinner ring, in output px
TRACK_W = 2.6        # track stroke width, in output px
ARC_W = 4.2          # bright arc stroke width, in output px
ARC_SWEEP = 105.0    # bright arc angular span, degrees

TRACK_COLOR = (70, 70, 70, 255)    # dim grey track
ARC_COLOR = (255, 255, 255, 255)   # bright white head

OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Image")
OUT_GIF = os.path.join(OUT_DIR, "loading_spinner_48.gif")
OUT_PNG = os.path.join(OUT_DIR, "loading_spinner_48_preview.png")


def _band(dist, half_w, feather_px):
    """0..1 coverage for a stroke of half width `half_w` centred on dist = 0."""
    return max(0.0, min(1.0, half_w + feather_px - abs(dist)))


def _render_split(step, track_mask, arc_mask, big):
    """Rasterise one frame into separate track and arc alpha masks."""
    tp, ap = track_mask.load(), arc_mask.load()
    cx = cy = big / 2.0

    # All radii/widths are authored in output pixels; rasterise in supersampled
    # space, so scale them up by SS first.
    ring_r = RING_R * SS
    track_w = TRACK_W * SS
    arc_w = ARC_W * SS
    f = SS  # feather width in supersampled px, i.e. 1 output px

    start_deg = step * (360.0 / FRAMES) - ARC_SWEEP / 2.0
    ang_feather = math.degrees(f / ring_r)

    for y in range(big):
        dy = (y + 0.5) - cy
        for x in range(big):
            dx = (x + 0.5) - cx
            r = math.hypot(dx, dy)
            if r < ring_r - arc_w - f or r > ring_r + arc_w + f:
                continue

            ang = math.degrees(math.atan2(dy, dx))
            if ang < 0.0:
                ang += 360.0

            tv = int(_band(r - ring_r, track_w / 2.0, f) * 255)
            if tv > tp[x, y]:
                tp[x, y] = tv

            rel = (ang - start_deg) % 360.0
            if rel <= ang_feather:
                ang_a = rel / ang_feather
            elif rel >= ARC_SWEEP - ang_feather:
                ang_a = (ARC_SWEEP - rel) / ang_feather
            elif rel <= ARC_SWEEP:
                ang_a = 1.0
            else:
                continue

            av = int(_band(r - ring_r, arc_w / 2.0, f) * ang_a * 255)
            if av > ap[x, y]:
                ap[x, y] = av


def build():
    os.makedirs(OUT_DIR, exist_ok=True)

    frames = []
    solid_arc = Image.new("RGBA", (SIZE, SIZE), ARC_COLOR)
    solid_track = Image.new("RGBA", (SIZE, SIZE), TRACK_COLOR)
    opaque = Image.new("L", (SIZE, SIZE), 255)
    clear = Image.new("L", (SIZE, SIZE), 0)

    for step in range(FRAMES):
        big = SIZE * SS
        track_mask = Image.new("L", (big, big), 0)
        arc_mask = Image.new("L", (big, big), 0)
        _render_split(step, track_mask, arc_mask, big)

        # Threshold -> crisp 1-bit transparency for GIF.
        track_m = track_mask.resize((SIZE, SIZE), Image.LANCZOS).point(
            lambda v: 255 if v >= 128 else 0, mode="1")
        arc_m = arc_mask.resize((SIZE, SIZE), Image.LANCZOS).point(
            lambda v: 255 if v >= 128 else 0, mode="1")

        frame = solid_track.copy()
        frame.paste(solid_arc, (0, 0), arc_m)
        # Anything outside the ring becomes transparent.
        frame.putalpha(Image.composite(opaque, clear, track_m))
        frames.append(frame)

    frames[0].save(
        OUT_GIF,
        save_all=True,
        append_images=frames[1:],
        duration=60,          # ms per frame -> 720 ms per revolution
        loop=0,               # infinite
        transparency=0,
        disposal=2,
        optimize=True,
    )

    # Contact sheet: two rows of 6, for eyeballing the loop.
    cell = SIZE * 2
    sheet = Image.new("RGBA", (cell * 6, cell * 2), (14, 14, 14, 255))
    for i, f in enumerate(frames):
        sheet.alpha_composite(f.resize((cell, cell), Image.NEAREST),
                              ((i % 6) * cell, (i // 6) * cell))
    sheet.convert("RGB").save(OUT_PNG)

    print("GIF :", OUT_GIF, os.path.getsize(OUT_GIF), "bytes")
    print("PNG :", OUT_PNG)
    print("frames={} size={}x{}".format(FRAMES, SIZE, SIZE))
    print("est. flash @16bpp: ~{} KB".format(SIZE * SIZE * 2 * FRAMES // 1024))


if __name__ == "__main__":
    build()