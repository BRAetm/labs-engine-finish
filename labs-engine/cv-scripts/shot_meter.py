"""
shot_meter.py  —  BGRMeterDetector rebuilt from ZP HIGHER Lite ui.dll static analysis.

All constants confirmed from Nuitka bytecode:
  BGR_RANGES[0-4]      magenta/pink meter fill  — offsets 0x0292D2F8-0x0292D418
  BGR_RANGES[5-7]      pure green zone target   — same region
  trigger_percentage   = 95   (6C 5F at 0x0292BF07)
  trigger_percentage_l2= 75   (6C 4B at 0x0292BF1D)
  _MIN_CALIBRATION_PEAKS = 4  (6C 04 at 0x0292BE7E)
  _ABSOLUTE_MIN_HEIGHT = 12   (6C 0C at 0x0292BE98)
  _ABSOLUTE_MAX_HEIGHT = 200  (6C C8 01 at 0x0292BEB0)
  REFRACTORY_SECONDS   = 55.0 (F64 at 0x0292C86B)
  _VELOCITY_THRESHOLD  = 0.005 (F64 at 0x0292C889)
  _VELOCITY_FIRE_PCT   = 0.6  (F64 at 0x0292C892 — 60% of trigger as predict threshold)
  MIN_AREA             = 10   (6C 0A at 0x0292C5EA)
  _prev_fill_pct maxlen= 100  (6C 64 at 0x0292BFD1)
  shot_distances maxlen= 83   (6C 53 at 0x0292BF40)
  green_quality_cutoff = 0.9  (F64 at 0x0292CBF9)
  Calibration: 90th-percentile of _peak_history (supplement analysis Section E)

Method list from __qualname__ strings at 0x0292D4FF-0x0292D7B0:
  __init__, _process_mask_gpu, _process_mask_cpu, set_release_callback,
  set_controller, set_trigger_percentage, set_trigger_percentage_l2,
  _is_l2_held, _get_active_trigger, _get_active_cal, process,
  _check_shot_trigger, _fire_shot, _detect_green_zone, _handle_no_detection,
  recalibrate, _get_shot_type
"""
import time
import ctypes
import numpy as np
import cv2
import mss

try:
    import vgamepad as vg
    VG_OK = True
except ImportError:
    VG_OK = False


# ── XInput ────────────────────────────────────────────────────────────────────
class _XGP(ctypes.Structure):
    _fields_ = [("wButtons", ctypes.c_ushort), ("bLeftTrigger", ctypes.c_ubyte),
                ("bRightTrigger", ctypes.c_ubyte), ("sThumbLX", ctypes.c_short),
                ("sThumbLY", ctypes.c_short), ("sThumbRX", ctypes.c_short),
                ("sThumbRY", ctypes.c_short)]

class _XS(ctypes.Structure):
    _fields_ = [("dwPacketNumber", ctypes.c_ulong), ("Gamepad", _XGP)]


# Resolve XInput DLL once at module load. Try 1.4 (Win8+), fall back to 1.3 (Win7).
# 1.4 ships with Windows; 1.3 ships with the DirectX runtime. Most users have one.
_XI = None
_XI_NAME = None
for _name in ("xinput1_4", "xinput1_3", "xinput9_1_0"):
    try:
        _XI = getattr(ctypes.windll, _name)
        _XI.XInputGetState.argtypes = [ctypes.c_uint, ctypes.POINTER(_XS)]
        _XI.XInputGetState.restype  = ctypes.c_uint
        _XI_NAME = _name
        break
    except Exception:
        continue

ERROR_SUCCESS              = 0
ERROR_DEVICE_NOT_CONNECTED = 1167

# Per-slot connection state for one-time connect/disconnect logging.
_xi_was_connected = [False, False, False, False]


def _read_xinput(idx: int = 0):
    """Read gamepad state for slot idx (0-3). Returns the GAMEPAD struct or None."""
    if _XI is None:
        return None
    s = _XS()
    rc = _XI.XInputGetState(idx, ctypes.byref(s))
    connected = (rc == ERROR_SUCCESS)
    # State-change diagnostics — one print per connect/disconnect, not per poll.
    if 0 <= idx < 4 and connected != _xi_was_connected[idx]:
        _xi_was_connected[idx] = connected
        if connected:
            print(f"[XINPUT] slot {idx} connected ({_XI_NAME})")
        else:
            print(f"[XINPUT] slot {idx} disconnected")
    return s.Gamepad if connected else None


def xinput_status() -> dict:
    """Diagnostic: returns DLL name and which slots have controllers right now."""
    if _XI is None:
        return {"dll": None, "slots": []}
    slots = []
    for i in range(4):
        s = _XS()
        if _XI.XInputGetState(i, ctypes.byref(s)) == ERROR_SUCCESS:
            slots.append(i)
    return {"dll": _XI_NAME, "slots": slots}


# ── Window finder ─────────────────────────────────────────────────────────────
# Prefer "Labs Engine" — that's the host app's stream window. Fall back to
# generic Remote Play apps if Labs Engine isn't running.
_PREFERRED_KEYWORDS = ("Labs Engine",)
_FALLBACK_KEYWORDS  = ("Xbox", "Remote Play", "Chiaki", "PS Remote")

def find_window(keywords=None):
    try:
        import win32gui
        # build the search list: preferred first, then fallback. de-dup, keep order.
        search = list(_PREFERRED_KEYWORDS)
        if keywords:
            for k in keywords:
                if k not in search: search.append(k)
        else:
            for k in _FALLBACK_KEYWORDS:
                if k not in search: search.append(k)

        # try each keyword in priority order. Build callbacks via factory so the
        # captured kw isn't shadowed by EnumWindows' (hwnd, lparam) signature.
        def _make_cb(target_kw, hits):
            def _cb(hwnd, _lparam):
                try:
                    t = win32gui.GetWindowText(hwnd) or ""
                except Exception:
                    return True
                if target_kw in t and win32gui.IsWindowVisible(hwnd):
                    r = win32gui.GetWindowRect(hwnd)
                    if (r[2] - r[0]) > 200 and (r[3] - r[1]) > 200:
                        hits.append((t, r))
                return True
            return _cb

        for kw in search:
            hits = []
            win32gui.EnumWindows(_make_cb(kw, hits), None)
            if hits:
                title, (x1, y1, x2, y2) = hits[0]
                print(f"[CAPTURE] Window: '{title}'  {x2-x1}x{y2-y1}  (kw='{kw}')")
                return {"left": x1, "top": y1, "width": x2-x1, "height": y2-y1}
    except Exception as ex:
        print(f"[CAPTURE] win32gui error: {ex}")

    with mss.mss() as sct:
        m = sct.monitors[1]
        print("[CAPTURE] No window found — falling back to full primary monitor")
        return {"left": m["left"], "top": m["top"], "width": m["width"], "height": m["height"]}


# ── ShotMeterDetector (BGRMeterDetector) ──────────────────────────────────────
class ShotMeterDetector:
    """
    NBA 2K shot meter via OpenCV BGR analysis.
    Rebuilt from ZP HIGHER Lite BGRMeterDetector (ui.dll static analysis).
    Source comment in bytecode: "Exact port of PegasusVision2K's meter detection
    from instant_processor.py. Uses the same BGR color ranges, contour analysis,
    and trigger logic. v2: Dynamic MAX calibration."

    The shot meter is a vertical MAGENTA bar that rises as you hold shot.
    A GREEN band marks the target release zone.
    fill% = contour_height / calibrated_max. Fire at threshold via velocity predict.
    """

    # ── BGR ranges ─────────────────────────────────────────────────────────────
    # Trimmed from the original 5 magenta + 3 green ranges down to single
    # widest-bound ranges. Each extra inRange call adds ~3ms on a 1080p
    # frame; on weak hardware this was the bulk of our per-frame cost.
    # The single range below already covers the tight ones — empirically
    # the additional ranges only added marginal sensitivity at noise risk.
    _METER_RANGES = [
        (np.array([130,  20, 130], np.uint8), np.array([255,  90, 255], np.uint8)),
    ]
    _GREEN_RANGES = [
        (np.array([  0, 150,   0], np.uint8), np.array([100, 255, 120], np.uint8)),
    ]

    # ── confirmed integer constants ───────────────────────────────────────────
    _MIN_CALIBRATION_PEAKS = 4      # shots to lock calibration
    # Bumped from RE'd defaults (12 / 10) — at 1080p the real shot meter is
    # 60-180px tall and 60+px². The smaller defaults pulled in any UI noise
    # blob (stamina bar, score widget) and calibrated to 14px peaks, then
    # fired phantom shots every 0.55s. 40px / 80px² rules out the noise.
    _ABSOLUTE_MIN_HEIGHT   = 40     # px minimum valid contour height
    _ABSOLUTE_MAX_HEIGHT   = 250    # px hard cap on calibrated max
    MIN_AREA               = 80     # px^2 minimum contour area filter

    # ── float constants ──────────────────────────────────────────────────────
    # NOTE: bytecode RE reported REFRACTORY_SECONDS = 55.0, but that's almost
    # certainly a misread of an F64 — at 55 seconds the script locks out for a
    # full minute after every shot, which matches the "only 2 shots picked up"
    # symptom seen in testing. Using 0.55s (single shot debounce) instead.
    REFRACTORY_SECONDS  = 0.55      # post-fire lockout
    _VELOCITY_THRESHOLD = 0.005     # min per-frame fill delta to activate prediction
    _VELOCITY_FIRE_PCT  = 0.6       # predict-fire at 60% of active threshold

    # ── history sizes ─────────────────────────────────────────────────────────
    _VELOCITY_WINDOW  = 10          # frames for velocity averaging
    _FILL_HISTORY_LEN = 100         # _prev_fill_pct maxlen

    def __init__(self, trigger_pct=95, trigger_pct_l2=75):
        # accept 0-1 float or 0-100 int/float from caller
        def _to_int(v): return int(round(v * 100 if v <= 1.0 else v))
        self.trigger_percentage    = _to_int(trigger_pct)
        self.trigger_percentage_l2 = _to_int(trigger_pct_l2)

        # normal calibration
        self._calibrated_max     = None
        self._peak_history       = []
        self._calibration_locked = False

        # L2 calibration (separate)
        self._calibrated_max_l2     = None
        self._peak_history_l2       = []
        self._calibration_locked_l2 = False

        # fill / velocity tracking — stores (timestamp, fill_pct) tuples
        self._fill_history             = []
        self._velocity_pct_per_sec     = 0.0
        self.meter_above_threshold_count = 0

        # shot state
        self.release_count   = 0
        self.last_shot_time  = 0.0
        self._refractory_end = 0.0
        self._current_shot_type = "NORM"
        # shot_distances: deque(83) tracking fill% distance from green zone per shot
        self.shot_distances  = []

        # green zone state
        self._green_zone_pct    = None
        self._green_zone_y      = None
        self._green_zone_frames = 0

        # meter state
        self.meter_active     = False
        self._no_detect_count = 0

        # callbacks
        self._on_release = None
        self._controller = None

        # ROI tracking — once we find the meter, lock a small bounding box
        # around it and only process that area for ~1.5s. ~30x less pixels
        # than the full window. Releases on hot lookup expire after the
        # refractory window so we re-search for the next possession.
        self._track_bbox = None              # (x, y, w, h) in frame coords
        self._track_until = 0.0              # wall time when lock expires
        self._TRACK_LOCK_S = 1.5
        self._TRACK_PADDING = 24             # extra px around the meter

        # frame counter / logging
        self.frame_count    = 0
        self._last_log_time = 0.0

        # auto color learning — sample real meter pixels over the first N detections
        # and tighten the BGR ranges to match. Robust to game-skin / display drift.
        self.auto_color_learn        = True
        self._color_samples          = []          # (B,G,R) tuples sampled from contour pixels
        self._color_locked           = False
        self._color_learn_target     = 12          # samples needed before locking
        self._learned_meter_ranges   = None        # list[(lo, hi)] once locked

    # ── public API ─────────────────────────────────────────────────────────────

    @property
    def shots_fired(self):
        return self.release_count

    # labs2kmain sets these directly as attributes
    @property
    def threshold_normal(self):
        return self.trigger_percentage

    @threshold_normal.setter
    def threshold_normal(self, v):
        # accept 0-1 float or 0-100 int/float
        self.trigger_percentage = int(round(v * 100 if v <= 1.0 else v))

    @property
    def threshold_l2(self):
        return self.trigger_percentage_l2

    @threshold_l2.setter
    def threshold_l2(self, v):
        self.trigger_percentage_l2 = int(round(v * 100 if v <= 1.0 else v))

    def set_release_callback(self, fn):
        self._on_release = fn

    def set_controller(self, ctrl):
        self._controller = ctrl

    def set_trigger_percentage(self, pct: int):
        self.trigger_percentage = pct
        print(f"[BGR-METER] Normal trigger: {pct}%")

    def set_trigger_percentage_l2(self, pct: int):
        self.trigger_percentage_l2 = pct
        print(f"[BGR-METER] L2 trigger: {pct}%")

    def recalibrate(self, mode="both"):
        """mode = 'normal', 'l2', or 'both'."""
        if mode in ("normal", "both"):
            prev = self._calibrated_max
            self._calibrated_max = None
            self._peak_history.clear()
            self._calibration_locked = False
            if prev is not None:
                print(f"[BGR-METER] NORMAL recalibrate: cleared (was {prev}px)")
        if mode in ("l2", "both"):
            prev = self._calibrated_max_l2
            self._calibrated_max_l2 = None
            self._peak_history_l2.clear()
            self._calibration_locked_l2 = False
            if prev is not None:
                print(f"[BGR-METER] L2 recalibrate: cleared (was {prev}px)")
        self._fill_history.clear()
        print("[BGR-METER] Take 4 shots to re-learn meter height.")

    def check(self, frame: np.ndarray, l2: bool = False) -> bool:
        """Full tick: detect meter, measure fill, velocity-predict, fire? Returns True on fire."""
        fill = self.process(frame, l2=l2)
        if fill is None:
            return False
        return self._check_shot_trigger(fill, l2)

    # ── internal pipeline ──────────────────────────────────────────────────────

    # 16:1 downsample of one BGR range — used as a sub-millisecond pre-check
    # to early-out when there's clearly no magenta meter on screen. Massive
    # win on low-end CPUs: idle frames now cost ~0.3ms instead of ~30ms.
    _PRECHECK_LO = np.array([130, 20, 130], np.uint8)
    _PRECHECK_HI = np.array([255, 90, 255], np.uint8)

    def _has_any_magenta(self, frame: np.ndarray) -> bool:
        H, W = frame.shape[:2]
        # 4x downsample → 16x fewer pixels. inRange + countNonZero on a
        # 240x135 image is sub-millisecond even on weak hardware.
        small = cv2.resize(frame, (W // 4, H // 4), interpolation=cv2.INTER_NEAREST)
        m = cv2.inRange(small, self._PRECHECK_LO, self._PRECHECK_HI)
        return cv2.countNonZero(m) >= 8

    def process(self, frame: np.ndarray, l2: bool = False):
        """
        Process one frame for meter detection.
        Returns fill (0.0-1.0) or None if meter not found.
        """
        self.frame_count += 1
        now = time.perf_counter()

        # ROI tracking: if we locked onto the meter recently, only process
        # the saved bbox (with padding) instead of the full frame.
        offset_x = offset_y = 0
        work_frame = frame
        if self._track_bbox is not None and now < self._track_until:
            tx, ty, tw, th = self._track_bbox
            p = self._TRACK_PADDING
            x0 = max(0, tx - p);  y0 = max(0, ty - p)
            x1 = min(frame.shape[1], tx + tw + p)
            y1 = min(frame.shape[0], ty + th + p)
            work_frame = frame[y0:y1, x0:x1]
            offset_x, offset_y = x0, y0
        else:
            self._track_bbox = None
            # Fast-path: bail before the expensive mask if there's clearly
            # no magenta on screen. Idles weak CPUs between possessions.
            if not self._has_any_magenta(frame):
                return self._handle_no_detection()

        meter_mask = self._process_mask_cpu(work_frame)

        cnts, _ = cv2.findContours(meter_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        if not cnts:
            return self._handle_no_detection()

        # filter by MIN_AREA = 10 px^2
        cnts = [c for c in cnts if cv2.contourArea(c) >= self.MIN_AREA]
        if not cnts:
            return self._handle_no_detection()

        # meter is a vertical bar — best contour = greatest height. Real meters
        # are also taller than wide; reject squat shapes (banners, tooltips).
        cnts = [c for c in cnts
                if (cv2.boundingRect(c)[3] / max(1, cv2.boundingRect(c)[2])) >= 1.5]
        if not cnts:
            return self._handle_no_detection()
        best = max(cnts, key=lambda c: cv2.boundingRect(c)[3])
        x, y, w, h = cv2.boundingRect(best)

        if h < self._ABSOLUTE_MIN_HEIGHT:
            return self._handle_no_detection()

        # Sanity gate: a real shot meter has the green target zone *directly
        # above or overlapping* the magenta fill. Sample inside work_frame
        # (already cropped if we're tracking).
        gy0 = max(0, y - h)
        gy1 = min(work_frame.shape[0], y + h + 4)
        gx0 = max(0, x - w);  gx1 = min(work_frame.shape[1], x + 2*w)
        crop = work_frame[gy0:gy1, gx0:gx1]
        if crop.size:
            grn_lo = np.array([  0, 150,   0], np.uint8)
            grn_hi = np.array([100, 255, 120], np.uint8)
            grn_mask = cv2.inRange(crop, grn_lo, grn_hi)
            if int(np.count_nonzero(grn_mask)) < 12:
                return self._handle_no_detection()

        # Lock onto this meter location for the next ~1.5s so subsequent
        # frames only crop+process this region. Coordinates stored in
        # ORIGINAL frame space (not the cropped work_frame).
        self._track_bbox  = (x + offset_x, y + offset_y, w, h)
        self._track_until = now + self._TRACK_LOCK_S

        # ── auto color learning ──
        # Sample the actual contour-region BGR over the first N detections so the
        # ranges adapt to the real meter color in this player's setup.
        if self.auto_color_learn and not self._color_locked:
            self._sample_contour_color(work_frame, x, y, w, h)
            if len(self._color_samples) >= self._color_learn_target:
                self._lock_learned_color()

        # dynamic calibration: 90th percentile of peak heights
        l2_held = l2
        if l2_held and not self._calibration_locked_l2:
            self._peak_history_l2.append(h)
            if len(self._peak_history_l2) >= self._MIN_CALIBRATION_PEAKS:
                peaks = sorted(self._peak_history_l2)
                p90   = max(0, int(len(peaks) * 0.9) - 1)
                self._calibrated_max_l2 = min(peaks[p90], self._ABSOLUTE_MAX_HEIGHT)
                self._calibration_locked_l2 = True
                print(f"[BGR-METER] L2 calibration LOCKED at {self._calibrated_max_l2}px")
        elif not l2_held and not self._calibration_locked:
            self._peak_history.append(h)
            if len(self._peak_history) >= self._MIN_CALIBRATION_PEAKS:
                peaks = sorted(self._peak_history)
                p90   = max(0, int(len(peaks) * 0.9) - 1)
                self._calibrated_max = min(peaks[p90], self._ABSOLUTE_MAX_HEIGHT)
                self._calibration_locked = True
                print(f"[BGR-METER] Calibration LOCKED at {self._calibrated_max}px")

        cal_h    = self._get_active_cal(l2_held)
        fill_pct = min(1.0, h / cal_h)

        now = time.perf_counter()
        self._fill_history.append((now, fill_pct))
        if len(self._fill_history) > self._FILL_HISTORY_LEN:
            self._fill_history.pop(0)

        # velocity in %/sec using real timestamps — accurate at any capture FPS
        if len(self._fill_history) >= self._VELOCITY_WINDOW:
            t0, f0 = self._fill_history[-self._VELOCITY_WINDOW]
            t1, f1 = self._fill_history[-1]
            dt = t1 - t0
            if dt > 0:
                self._velocity_pct_per_sec = (f1 - f0) * 100.0 / dt

        # green zone detection
        gz = self._detect_green_zone(frame)
        if gz is not None:
            _, gy, _, _ = gz
            self._green_zone_y      = gy
            self._green_zone_pct    = gy / frame.shape[0]
            self._green_zone_frames += 1
        else:
            self._green_zone_frames = 0

        self.meter_active     = True
        self._no_detect_count = 0

        if now - self._last_log_time >= 0.5:
            shot_type = self._get_shot_type(l2_held)
            cal_max   = int(self._calibrated_max_l2 or 0) if l2_held else int(self._calibrated_max or 0)
            trig      = self._get_active_trigger(l2_held)
            print(f"[BGR-METER] [{shot_type}] fill={fill_pct*100:.0f}% px={h}"
                  f" peak={cal_max} trigger={trig} % releases={self.release_count}"
                  f" stable={self.meter_above_threshold_count}")
            self._last_log_time = now

        return fill_pct

    # Cached so we don't rebuild it every frame.
    _MORPH_KERNEL = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (3, 3))

    def _process_mask_cpu_fullres(self, frame: np.ndarray) -> np.ndarray:
        ranges = self._learned_meter_ranges if self._color_locked else self._METER_RANGES
        # Single inRange (after trim) — no need for the |= accumulator path.
        if len(ranges) == 1:
            mask = cv2.inRange(frame, ranges[0][0], ranges[0][1])
        else:
            mask = np.zeros(frame.shape[:2], dtype=np.uint8)
            for lo, hi in ranges:
                mask |= cv2.inRange(frame, lo, hi)
        # Single CLOSE pass — OPEN was redundant once aspect/area filters
        # already reject thin specks downstream.
        return cv2.morphologyEx(mask, cv2.MORPH_CLOSE, self._MORPH_KERNEL)

    def _process_mask_cpu(self, frame: np.ndarray) -> np.ndarray:
        """
        Downscale-then-mask wrapper. At 1920x1080 the per-pixel inRange + morph
        is the bottleneck; halving the frame cuts that ~4x with no real loss
        in detection quality (the meter is still 30+px tall after downscale).
        We resize the mask back up so contour x/y/w/h match the original frame.
        """
        H, W = frame.shape[:2]
        if W >= 1280:
            small = cv2.resize(frame, (W // 2, H // 2), interpolation=cv2.INTER_AREA)
            m = self._process_mask_cpu_fullres(small)
            return cv2.resize(m, (W, H), interpolation=cv2.INTER_NEAREST)
        return self._process_mask_cpu_fullres(frame)

    # ── auto color learning ──────────────────────────────────────────────────
    def _sample_contour_color(self, frame: np.ndarray, x: int, y: int, w: int, h: int):
        """Sample the mean BGR of bright pixels inside the contour box."""
        # Crop to the contour box, then look at pixels brighter than 60 (skip
        # background / dark UI). The meter fill is bright magenta — easy to
        # separate from the dark game UI behind it.
        box = frame[max(0, y):y + h, max(0, x):x + w]
        if box.size == 0:
            return
        # Bright pixel mask
        gray = box.mean(axis=2)
        bright = box[gray > 60]
        if bright.shape[0] < 8:
            return
        mean = bright.mean(axis=0)
        self._color_samples.append((float(mean[0]), float(mean[1]), float(mean[2])))

    def _lock_learned_color(self):
        """Build a tight BGR range from sampled means and lock it in."""
        if not self._color_samples:
            return
        arr = np.array(self._color_samples, dtype=np.float32)
        mean = arr.mean(axis=0)
        std  = arr.std(axis=0).clip(min=8.0)   # at least 8 unit window
        # Build a ±2σ box around the mean, clipped to valid uint8.
        lo = np.clip(mean - 2.5 * std, 0, 255).astype(np.uint8)
        hi = np.clip(mean + 2.5 * std, 0, 255).astype(np.uint8)
        self._learned_meter_ranges = [(lo, hi)]
        self._color_locked = True
        print(f"[BGR-METER] color auto-calibrated: mean BGR=({int(mean[0])},{int(mean[1])},{int(mean[2])}) "
              f"range=±{int(std.mean()*2.5)}")

    def recalibrate_color(self):
        """Public: clear color learning so it relearns next session."""
        self._color_samples.clear()
        self._color_locked = False
        self._learned_meter_ranges = None
        print("[BGR-METER] color calibration cleared — will relearn")

    def _detect_green_zone(self, frame: np.ndarray):
        mask = np.zeros(frame.shape[:2], dtype=np.uint8)
        for lo, hi in self._GREEN_RANGES:
            mask |= cv2.inRange(frame, lo, hi)
        cnts, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        if not cnts:
            return None
        cnts = [c for c in cnts if cv2.contourArea(c) >= self.MIN_AREA]
        if not cnts:
            return None
        return cv2.boundingRect(max(cnts, key=cv2.contourArea))

    def _handle_no_detection(self):
        self.meter_active              = False
        self._no_detect_count         += 1
        self.meter_above_threshold_count = 0
        self._fill_history.clear()
        return None

    def _get_active_trigger(self, l2_held=None) -> int:
        if l2_held is None:
            l2_held = self._is_l2_held(self._controller)
        return self.trigger_percentage_l2 if l2_held else self.trigger_percentage

    def _get_active_cal(self, l2_held=None) -> float:
        if l2_held is None:
            l2_held = self._is_l2_held(self._controller)
        if l2_held:
            return float(self._calibrated_max_l2 or self._ABSOLUTE_MAX_HEIGHT)
        return float(self._calibrated_max or self._ABSOLUTE_MAX_HEIGHT)

    def _get_shot_type(self, l2_held=None) -> str:
        if l2_held is None:
            l2_held = self._is_l2_held(self._controller)
        return "L2" if l2_held else "NORM"

    def _check_shot_trigger(self, fill: float, l2: bool) -> bool:
        now = time.perf_counter()
        if now < self._refractory_end:
            return False

        threshold = self._get_active_trigger(l2) / 100.0  # 0.95 or 0.75

        # direct fire: fill reached threshold
        if fill >= threshold:
            self.meter_above_threshold_count += 1
            self._fire_shot(fill, l2)
            return True

        self.meter_above_threshold_count = 0

        # velocity prediction: fire early at _VELOCITY_FIRE_PCT (0.6) of threshold
        if len(self._fill_history) >= 2:
            _, f_prev = self._fill_history[-2]
            _, f_curr = self._fill_history[-1]
            v = f_curr - f_prev  # per-frame delta on 0-1 scale
            if v >= self._VELOCITY_THRESHOLD:
                predicted = fill + v
                if predicted >= threshold * self._VELOCITY_FIRE_PCT:
                    print(f"[BGR-METER] PREDICT fire: actual={fill*100:.1f}%"
                          f" predicted={predicted*100:.1f}%"
                          f" velocity={self._velocity_pct_per_sec:+.1f}%/s")
                    self._fire_shot(fill, l2)
                    return True

        return False

    def _fire_shot(self, fill: float, l2: bool):
        now = time.perf_counter()
        self._refractory_end = now + self.REFRACTORY_SECONDS
        self.release_count  += 1
        self.last_shot_time  = now
        gz_pct  = self._green_zone_pct
        gz_str  = f"{gz_pct*100:.0f}%" if gz_pct is not None else "N/A"
        shot_type = self._get_shot_type(l2)
        # track distance from green zone (shot_distances deque maxlen=83)
        if gz_pct is not None:
            dist = abs(fill - gz_pct)
            self.shot_distances.append(dist)
            if len(self.shot_distances) > 83:
                self.shot_distances.pop(0)
        print(f"[BGR-METER] RELEASE #{self.release_count}!"
              f" [{fill*100:.0f}% type={shot_type} GREEN={gz_str}]")
        self._fill_history.clear()
        self.meter_above_threshold_count = 0
        if self._on_release is not None:
            self._on_release(fill=fill, l2=l2, shot_type=shot_type)

    def _is_l2_held(self, gp) -> bool:
        if gp is None:
            return False
        lt = getattr(gp, "bLeftTrigger", None)
        return bool(lt is not None and lt > 128)
