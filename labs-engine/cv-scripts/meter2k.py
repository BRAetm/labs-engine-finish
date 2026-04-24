"""
meter2k.py — minimal NBA 2K shot-meter detector for the new vertical meter.

Layout (from in-game screenshot):
  • Vertical bar to the right of the player.
  • Bottom part = magenta/pink "current fill", rises as the shot is held.
  • Top part   = green "target zone" (chevron + bar).
  • When magenta crosses into the green band → release the shot for green.

Design goals:
  • One small ROI (you draw it once); skip full-screen capture entirely.
  • No contours, no calibration, no velocity smoothing — just per-row color
    classification on a tiny strip. Sub-millisecond per frame.
  • Easy to debug: returns the live fill %, the green-band y range, and the
    annotated ROI for live preview.

Public API:
  MeterDetector(roi_in_window: tuple[int,int,int,int])
    .process(frame_bgr) -> Result   # frame_bgr is the FULL captured frame
  Result:
    fill_pct   : float 0..1   — magenta top divided by ROI height (0 = empty)
    in_green   : bool         — magenta top is inside the green band
    has_meter  : bool         — we saw any magenta at all
    green_y0/1 : int          — pixel rows of the green band inside the ROI
    annotated  : ndarray      — debug viz of the ROI with overlays
"""
from dataclasses import dataclass
from typing import Optional, Tuple

import numpy as np

try:
    import cv2
    _CV_OK = True
except Exception:
    _CV_OK = False


@dataclass
class Result:
    fill_pct:  float
    in_green:  bool
    has_meter: bool
    green_y0:  int
    green_y1:  int
    magenta_y: int           # top-most magenta row in ROI (smaller = higher fill)
    annotated: Optional[np.ndarray]  # only set if return_debug=True


class MeterDetector:
    # Permissive HSV ranges — covers the magenta/pink fill across lighting.
    _MAG_H_LO, _MAG_H_HI = 140, 175
    _MAG_S_MIN           = 90
    _MAG_V_MIN           = 120

    # Green target zone ranges. Lime/electric green, very bright, very saturated.
    _GRN_H_LO, _GRN_H_HI = 40, 85
    _GRN_S_MIN           = 100
    _GRN_V_MIN           = 140

    # A row is "magenta" if at least this fraction of its pixels match.
    _ROW_HIT_RATIO = 0.20

    def __init__(self, roi_in_window: Tuple[int, int, int, int]):
        # ROI is (x, y, w, h) in the *captured frame* coordinate system.
        x, y, w, h = roi_in_window
        if w <= 0 or h <= 0:
            raise ValueError("ROI must have positive width and height")
        self.roi = (int(x), int(y), int(w), int(h))

    # ── core ──────────────────────────────────────────────────────────────────

    def process(self, frame_bgr: np.ndarray, return_debug: bool = False) -> Result:
        x, y, w, h = self.roi
        H, W = frame_bgr.shape[:2]
        # Clamp ROI to frame bounds so a resize-on-stretch doesn't crash us.
        x = max(0, min(x, W - 1)); y = max(0, min(y, H - 1))
        w = max(1, min(w, W - x)); h = max(1, min(h, H - y))
        strip = frame_bgr[y:y+h, x:x+w]
        if strip.size == 0:
            return Result(0.0, False, False, 0, 0, h, None)

        hsv = cv2.cvtColor(strip, cv2.COLOR_BGR2HSV) if _CV_OK \
              else _bgr_to_hsv_np(strip)
        H_, S_, V_ = hsv[..., 0], hsv[..., 1], hsv[..., 2]

        mag_mask = ((H_ >= self._MAG_H_LO) & (H_ <= self._MAG_H_HI)
                    & (S_ >= self._MAG_S_MIN) & (V_ >= self._MAG_V_MIN))
        grn_mask = ((H_ >= self._GRN_H_LO) & (H_ <= self._GRN_H_HI)
                    & (S_ >= self._GRN_S_MIN) & (V_ >= self._GRN_V_MIN))

        # Per-row hit fractions (we look for vertically-stacked bands).
        mag_rows = mag_mask.mean(axis=1)
        grn_rows = grn_mask.mean(axis=1)

        # Locate the green band: contiguous rows above the threshold, take the
        # first (top-most) such block.
        grn_lit = grn_rows >= self._ROW_HIT_RATIO
        green_y0, green_y1 = _first_run(grn_lit)

        # Top-most magenta row (smallest y where row is "lit"). Smaller = fuller.
        mag_lit = mag_rows >= self._ROW_HIT_RATIO
        magenta_top = int(np.argmax(mag_lit)) if mag_lit.any() else h

        has_meter = bool(mag_lit.any())
        # Fill %: how high the magenta has risen, as a fraction of ROI height.
        fill_pct = (h - magenta_top) / float(h) if has_meter else 0.0

        in_green = (has_meter and green_y1 > green_y0
                    and green_y0 <= magenta_top <= green_y1)

        annotated = None
        if return_debug:
            annotated = strip.copy()
            if green_y1 > green_y0:
                cv2.rectangle(annotated, (0, green_y0), (w-1, green_y1-1),
                              (0, 255, 0), 1)
            if has_meter:
                cv2.line(annotated, (0, magenta_top), (w-1, magenta_top),
                         (0, 0, 255), 1)
            cv2.putText(annotated, f"{fill_pct*100:.0f}%",
                        (2, 12), cv2.FONT_HERSHEY_SIMPLEX, 0.4,
                        (255, 255, 255), 1)

        return Result(fill_pct, in_green, has_meter,
                      green_y0, green_y1, magenta_top, annotated)


# ── helpers ──────────────────────────────────────────────────────────────────

def _first_run(boolarr: np.ndarray) -> Tuple[int, int]:
    """Return (start, end_exclusive) of the first True-run in a 1-D bool array."""
    if not boolarr.any():
        return (0, 0)
    diff = np.diff(boolarr.astype(np.int8))
    starts = np.where(diff == 1)[0] + 1
    ends   = np.where(diff == -1)[0] + 1
    if boolarr[0]:
        starts = np.concatenate(([0], starts))
    if boolarr[-1]:
        ends = np.concatenate((ends, [len(boolarr)]))
    if starts.size == 0:
        return (0, 0)
    return (int(starts[0]), int(ends[0]))


def _bgr_to_hsv_np(img_bgr: np.ndarray) -> np.ndarray:
    # Cheap fallback when cv2 is unavailable. Matches OpenCV's H 0..180 range.
    bgr = img_bgr.astype(np.float32) / 255.0
    b, g, r = bgr[..., 0], bgr[..., 1], bgr[..., 2]
    cmax = np.max(bgr, axis=-1); cmin = np.min(bgr, axis=-1)
    delta = cmax - cmin
    h = np.zeros_like(cmax)
    mask = delta > 1e-6
    rmax = mask & (cmax == r); gmax = mask & (cmax == g); bmax = mask & (cmax == b)
    h[rmax] = ((g[rmax] - b[rmax]) / delta[rmax]) % 6
    h[gmax] = ((b[gmax] - r[gmax]) / delta[gmax]) + 2
    h[bmax] = ((r[bmax] - g[bmax]) / delta[bmax]) + 4
    h = (h * 30) % 180
    s = np.where(cmax > 0, delta / np.where(cmax > 0, cmax, 1), 0) * 255
    v = cmax * 255
    return np.stack([h, s, v], axis=-1).astype(np.uint8)
