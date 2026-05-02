"""
ZP HIGHER Lite — Engine
Fully reconstructed from ui.dll Nuitka bytecode hex analysis (2026-04-21).

BGRMeterDetector constants all verified from bytecode (0x28D5xxx / 0x292D4FF regions):
  trigger_percentage     = 95    (integer /100 at runtime)
  trigger_percentage_l2  = 75
  _MIN_CALIBRATION_PEAKS = 4
  _ABSOLUTE_MIN_HEIGHT   = 12 px
  _ABSOLUTE_MAX_HEIGHT   = 200 px
  MIN_AREA               = 10 px²
  REFRACTORY_SECONDS     = 55.0
  _VELOCITY_THRESHOLD    = 0.005
  _VELOCITY_FIRE_PCT     = 0.6
  Calibration            = 90th percentile of _peak_history
  L2 calibration         = separate (_calibrated_max_l2)
  morphologyEx           = CLOSE then OPEN, 3×3 ellipse kernel
  _prev_fill_pct maxlen  = 100
  shot_distances maxlen  = 83

BGR_RANGES: 5 magenta/pink ranges + 3 green zone ranges (8 total).
"""
import collections
import queue
import threading
import time

import cv2
import numpy as np

try:
    import bettercam
    BC_OK = True
except ImportError:
    BC_OK = False

from core.xinput import read_xinput

try:
    import cupy as cp
    CUPY_OK = True
except ImportError:
    CUPY_OK = False

# ── BGR inRange thresholds (8 ranges, from bytecode) ─────────────────────────
# Ranges 1–5: magenta/pink fill  B:130-255, G:20-90, R:130-255
# Ranges 6–8: green zone         lower(0,150,0) / upper(80,255,80)
_LOWERS = [
    np.array([130,  20, 130], np.uint8),
    np.array([140,  25, 140], np.uint8),
    np.array([150,  30, 150], np.uint8),
    np.array([160,  35, 160], np.uint8),
    np.array([170,  40, 170], np.uint8),
    np.array([  0, 150,   0], np.uint8),
    np.array([  0, 160,   0], np.uint8),
    np.array([  0, 170,   0], np.uint8),
]
_UPPERS = [
    np.array([255,  90, 255], np.uint8),
    np.array([255,  90, 255], np.uint8),
    np.array([255,  90, 255], np.uint8),
    np.array([255,  90, 255], np.uint8),
    np.array([255,  90, 255], np.uint8),
    np.array([ 80, 255,  80], np.uint8),
    np.array([ 80, 255,  80], np.uint8),
    np.array([ 80, 255,  80], np.uint8),
]
_KERNEL = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (3, 3))


# ── BGRMeterDetector ──────────────────────────────────────────────────────────

class BGRMeterDetector:
    """
    Exact port of ZP HIGHER Lite's BGRMeterDetector from ui.dll.
    Described in source as "Exact port of PegasusVision2K's meter detection
    from instant_processor.py".

    Method names confirmed from __qualname__ strings at offset 0x0292D4FF:
    __init__, _process_mask_gpu, _process_mask_cpu, set_release_callback,
    set_controller, set_trigger_percentage, set_trigger_percentage_l2,
    _is_l2_held, _get_active_trigger, _get_active_cal, process,
    _check_shot_trigger, _fire_shot, _detect_green_zone, _handle_no_detection,
    recalibrate, _get_shot_type
    """

    REFRACTORY_SECONDS     = 55.0
    _MIN_CALIBRATION_PEAKS = 4
    _ABSOLUTE_MIN_HEIGHT   = 12
    _ABSOLUTE_MAX_HEIGHT   = 200
    MIN_AREA               = 10
    _VELOCITY_THRESHOLD    = 0.005
    _VELOCITY_FIRE_PCT     = 0.6
    _VELOCITY_WINDOW       = 10
    _FILL_HISTORY_LEN      = 100

    def __init__(self, trigger_pct=95, trigger_pct_l2=75):
        self.trigger_percentage    = self._to_int(trigger_pct)
        self.trigger_percentage_l2 = self._to_int(trigger_pct_l2)

        # normal calibration
        self._calibrated_max     = None
        self._calibration_locked = False
        self._peak_history       = []

        # separate L2 calibration (confirmed — distinct state)
        self._calibrated_max_l2     = None
        self._calibration_locked_l2 = False
        self._peak_history_l2       = []

        self._fill_history         = collections.deque(maxlen=self._FILL_HISTORY_LEN)
        self._velocity_pct_per_sec = 0.0
        self._last_fire            = 0.0

        self.shots_fired    = 0
        self.shot_distances = collections.deque(maxlen=83)

        self._release_cb  = None
        self._controller  = None

    @staticmethod
    def _to_int(v):
        return int(round(v * 100 if v <= 1.0 else v))

    def set_release_callback(self, cb):
        self._release_cb = cb

    def set_controller(self, ctrl):
        self._controller = ctrl

    def set_trigger_percentage(self, v):
        self.trigger_percentage = self._to_int(v)

    def set_trigger_percentage_l2(self, v):
        self.trigger_percentage_l2 = self._to_int(v)

    def recalibrate(self):
        self._calibrated_max = None;  self._calibration_locked = False;  self._peak_history = []
        self._calibrated_max_l2 = None; self._calibration_locked_l2 = False; self._peak_history_l2 = []
        print("[BGR-METER] Recalibrated")

    def _get_active_trigger(self, l2: bool) -> float:
        return (self.trigger_percentage_l2 if l2 else self.trigger_percentage) / 100.0

    def _get_active_cal(self, l2: bool):
        return self._calibrated_max_l2 if l2 else self._calibrated_max

    def _get_shot_type(self, l2: bool) -> str:
        return "l2" if l2 else "normal"

    def _process_mask_gpu(self, frame: np.ndarray) -> np.ndarray:
        """CuPy GPU inRange — falls back to CPU if CuPy not available."""
        if not CUPY_OK:
            return self._process_mask_cpu(frame)
        f = cp.asarray(frame)
        mask = cp.zeros(f.shape[:2], dtype=cp.uint8)
        for lo, hi in zip(_LOWERS, _UPPERS):
            lo_g = cp.asarray(lo); hi_g = cp.asarray(hi)
            m = cp.all((f >= lo_g) & (f <= hi_g), axis=2).astype(cp.uint8) * 255
            mask |= m
        # morphology back on CPU (cv2 doesn't accept cupy arrays)
        out = cv2.morphologyEx(cp.asnumpy(mask), cv2.MORPH_CLOSE, _KERNEL)
        out = cv2.morphologyEx(out,               cv2.MORPH_OPEN,  _KERNEL)
        return out

    def _process_mask_cpu(self, frame: np.ndarray) -> np.ndarray:
        mask = np.zeros(frame.shape[:2], dtype=np.uint8)
        for lo, hi in zip(_LOWERS, _UPPERS):
            mask |= cv2.inRange(frame, lo, hi)
        mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, _KERNEL)
        mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN,  _KERNEL)
        return mask

    def _detect_green_zone(self, frame: np.ndarray):
        lo = np.array([0, 150, 0], np.uint8)
        hi = np.array([80, 255, 80], np.uint8)
        m  = cv2.inRange(frame, lo, hi)
        pct = float(np.sum(m > 0)) / m.size if m.size else 0.0
        return pct if pct > 0.01 else None

    def _handle_no_detection(self):
        pass

    def process(self, frame: np.ndarray, l2: bool = False) -> bool:
        mask = self._process_mask_gpu(frame) if CUPY_OK else self._process_mask_cpu(frame)
        contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

        valid = [c for c in contours if cv2.contourArea(c) >= self.MIN_AREA]
        if not valid:
            self._handle_no_detection()
            return False

        best = max(valid, key=lambda c: cv2.boundingRect(c)[3])
        _, _, _, h = cv2.boundingRect(best)
        if h < self._ABSOLUTE_MIN_HEIGHT:
            self._handle_no_detection()
            return False

        cal = self._get_active_cal(l2)
        if cal is None or cal <= 0:
            self._update_calibration(h, l2)
            return False

        fill = min(1.0, h / cal)
        now  = time.perf_counter()
        self._fill_history.append((now, fill))

        if len(self._fill_history) >= self._VELOCITY_WINDOW:
            t0, f0 = self._fill_history[-self._VELOCITY_WINDOW]
            t1, f1 = self._fill_history[-1]
            dt = t1 - t0
            if dt > 0:
                self._velocity_pct_per_sec = (f1 - f0) * 100.0 / dt

        return self._check_shot_trigger(fill, l2, frame)

    def _update_calibration(self, h: int, l2: bool):
        if l2:
            if not self._calibration_locked_l2:
                self._peak_history_l2.append(h)
                if len(self._peak_history_l2) >= self._MIN_CALIBRATION_PEAKS:
                    p = sorted(self._peak_history_l2)
                    self._calibrated_max_l2 = min(
                        p[max(0, int(len(p) * 0.9) - 1)], self._ABSOLUTE_MAX_HEIGHT)
                    self._calibration_locked_l2 = True
                    print(f"[BGR-METER] L2 cal locked: max={self._calibrated_max_l2}px")
        else:
            if not self._calibration_locked:
                self._peak_history.append(h)
                if len(self._peak_history) >= self._MIN_CALIBRATION_PEAKS:
                    p = sorted(self._peak_history)
                    self._calibrated_max = min(
                        p[max(0, int(len(p) * 0.9) - 1)], self._ABSOLUTE_MAX_HEIGHT)
                    self._calibration_locked = True
                    print(f"[BGR-METER] cal locked: max={self._calibrated_max}px")

    def _check_shot_trigger(self, fill: float, l2: bool, frame: np.ndarray) -> bool:
        threshold = self._get_active_trigger(l2)

        if time.perf_counter() - self._last_fire < self.REFRACTORY_SECONDS:
            return False

        # velocity-predict fire
        if len(self._fill_history) >= 2:
            _, f_prev = self._fill_history[-2]
            _, f_curr = self._fill_history[-1]
            v = f_curr - f_prev
            if v >= self._VELOCITY_THRESHOLD:
                if (fill + v) >= threshold * self._VELOCITY_FIRE_PCT:
                    print(f"[BGR-METER] PREDICT fire: fill={fill:.3f} v={v:.4f}")
                    self._fire_shot(fill, l2, frame)
                    return True

        if fill >= threshold:
            self._fire_shot(fill, l2, frame)
            return True

        return False

    def _fire_shot(self, fill: float, l2: bool, frame: np.ndarray):
        self._last_fire = time.perf_counter()
        self.shots_fired += 1
        gz = self._detect_green_zone(frame)
        if gz is not None:
            self.shot_distances.append(abs(fill - gz))
        print(f"[BGR-METER] FIRE #{self.shots_fired} fill={fill:.3f} "
              f"type={self._get_shot_type(l2)} vel={self._velocity_pct_per_sec:.2f}%/s")
        if self._release_cb:
            self._release_cb(l2=l2)


# ── Engine ────────────────────────────────────────────────────────────────────

class Engine:
    """
    Orchestrates BetterCam capture, BGRMeterDetector, and PSControllerBridge.
    Config keys match ShootingColumn.get_state() / profile JSON.
    """

    def __init__(self, config: dict | None = None):
        cfg = config or {}
        self.detector = BGRMeterDetector(
            trigger_pct    = cfg.get("rt",    95),
            trigger_pct_l2 = cfg.get("l2",    75),
        )
        from core.controller_ps import PSControllerBridge
        self.bridge = PSControllerBridge()

        # wire detector → bridge fire
        tempo_ms = int(cfg.get("tempo", 39))
        tempo_on = bool(cfg.get("tempo_on", False))
        self.detector.set_release_callback(
            lambda l2=False: self.bridge.fire_shot(
                l2=l2, tempo=tempo_on, tempo_ms=tempo_ms)
        )

        self.capture_gpu = int(cfg.get("capture_gpu", 0))
        self.capture_fps = int(cfg.get("capture_fps", 240))

        self._running = False
        self._frame_q = queue.Queue(maxsize=2)
        self._camera  = None
        self.fps_cur  = 0.0

    @property
    def shots_fired(self) -> int:
        return self.detector.shots_fired

    def _find_target_window(self):
        # BetterCam validates the region against the primary monitor's
        # bounds (typically 1920x1080). If the Labs Engine window is
        # positioned anywhere that pushes its rect past the primary
        # display edge — partial off-screen, second monitor, etc. — the
        # raw GetWindowRect tuple would crash bettercam. Clamp here.
        try:
            import win32gui
            try:
                from ctypes import windll
                MON_W = windll.user32.GetSystemMetrics(0)   # SM_CXSCREEN
                MON_H = windll.user32.GetSystemMetrics(1)   # SM_CYSCREEN
            except Exception:
                MON_W, MON_H = 1920, 1080
            hits = []
            def _cb(hwnd, _):
                t = win32gui.GetWindowText(hwnd)
                if any(k in t for k in ("Labs Engine", "Xbox", "Chiaki", "PS Remote", "Remote Play")):
                    hits.append(win32gui.GetWindowRect(hwnd))
            win32gui.EnumWindows(_cb, None)
            if hits:
                l, t, r, b = hits[0]
                # Clamp to primary monitor bounds (BetterCam constraint).
                cl = max(0, l)
                ct = max(0, t)
                cr = min(MON_W, r)
                cb = min(MON_H, b)
                if cr - cl >= 200 and cb - ct >= 200:
                    print(f"[CAPTURE] Remote Play window: raw=({l},{t},{r},{b}) "
                          f"clamped=({cl},{ct}) {cr-cl}x{cb-ct}")
                    return (cl, ct, cr, cb)
                print(f"[CAPTURE] window rect {(l,t,r,b)} doesn't fit primary "
                      f"monitor ({MON_W}x{MON_H}) — falling back to full screen")
        except Exception as ex:
            print(f"[CAPTURE] win32gui: {ex}")
        print("[CAPTURE] No Remote Play window found - full screen")
        return None

    def _start_camera(self):
        if not BC_OK:
            raise RuntimeError("bettercam not installed")
        region = self._find_target_window()
        self._camera = bettercam.create(device_idx=self.capture_gpu, output_color="BGR")
        self._camera.start(region=region, target_fps=self.capture_fps, video_mode=True)
        print(f"[CAPTURE] BetterCam GPU={self.capture_gpu} {self.capture_fps}fps region={region}")

    def _capture_loop(self):
        while self._running:
            frame = self._camera.get_latest_frame()
            if frame is not None:
                if self._frame_q.full():
                    try: self._frame_q.get_nowait()
                    except Exception: pass
                self._frame_q.put(frame)

    def _detect_loop(self):
        fc = 0
        tw = time.perf_counter()
        while self._running:
            try:
                frame = self._frame_q.get(timeout=0.1)
            except queue.Empty:
                continue
            gp = read_xinput()
            l2 = bool(gp and gp.bLeftTrigger > 128)
            self.detector.process(frame, l2=l2)
            fc += 1
            t = time.perf_counter()
            if t - tw >= 1.0:
                self.fps_cur = fc / (t - tw)
                fc = 0
                tw = t

    def start(self):
        self.bridge.start()
        self._start_camera()
        self._running = True
        threading.Thread(target=self._capture_loop, daemon=True).start()
        threading.Thread(target=self._detect_loop,  daemon=True).start()
        print("[ENGINE] ═══ Started ═══")

    def stop(self):
        self._running = False
        if self._camera:
            self._camera.stop()
        self.bridge.stop()
        print("[ENGINE] Stopped")
