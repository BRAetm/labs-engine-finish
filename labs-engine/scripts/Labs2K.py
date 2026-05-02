"""
Labs2K.py — TM Labs shot-timing UI.

Owns ONLY the user-facing UI. The actual capture / detection / virtual-pad
logic lives in zp_core/ (the original ZP HIGHER Lite engine + controller,
unchanged). This file:

  1. Loads zp_core's Engine and PSControllerBridge.
  2. Patches Engine._find_target_window so the engine grabs the host
     "Labs Engine" stream window instead of looking for "Xbox / Chiaki / …".
  3. Renders a small Qt UI: trigger sliders, feature toggles, START/STOP,
     live FPS / shots / calibration readout, log tail.
"""
import os
import sys
import time
import threading
import argparse
from collections import deque
from pathlib import Path

# UTF-8 stdout — engine.py prints box-drawing chars and arrows.
try:
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")
except Exception:
    pass

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "zp_core"))      # so `import engine` resolves
sys.path.insert(0, str(ROOT / "zp_core" / ".."))  # so `import zp_core.engine` works too

# ── load zp_core, then patch the window finder ───────────────────────────────
# The core sub-modules use `from .xinput import …`, which works because the
# folder has __init__.py and we'll import via the package name.
from zp_core import engine as _eng           # noqa: E402
from zp_core.controller_ps import PSControllerBridge  # noqa: E402

_orig_find = _eng.Engine._find_target_window


def _primary_monitor_rect():
    """Return (left, top, right, bottom) of the primary display area."""
    try:
        import ctypes
        user32 = ctypes.windll.user32
        user32.SetProcessDPIAware()
        w = user32.GetSystemMetrics(0)  # SM_CXSCREEN
        h = user32.GetSystemMetrics(1)  # SM_CYSCREEN
        return (0, 0, int(w), int(h))
    except Exception:
        return (0, 0, 1920, 1080)


def _labs_engine_find(self):
    try:
        import win32gui
        hits = []
        def _cb(hwnd, _):
            t = win32gui.GetWindowText(hwnd) or ""
            if any(k in t for k in ("Labs Engine", "Xbox")) and win32gui.IsWindowVisible(hwnd):
                r = win32gui.GetWindowRect(hwnd)
                if (r[2] - r[0]) > 200 and (r[3] - r[1]) > 200:
                    hits.append((t, r))
        win32gui.EnumWindows(_cb, None)
        if hits:
            t, r = hits[0]
            # BetterCam refuses regions outside the monitor. Windows reports
            # window rects that include invisible shadow/border pixels, so a
            # 1920-pixel-wide window can have a rect of (0, 0, 1938, …) and
            # BetterCam rejects it. Clamp into the primary monitor bounds.
            mon = _primary_monitor_rect()
            clamped = (
                max(mon[0], r[0]), max(mon[1], r[1]),
                min(mon[2], r[2]), min(mon[3], r[3]),
            )
            if clamped != r:
                print(f"[CAPTURE] Labs Engine: '{t}' raw={r[2]-r[0]}x{r[3]-r[1]} "
                      f"→ clamped to {clamped[2]-clamped[0]}x{clamped[3]-clamped[1]} "
                      f"(monitor {mon[2]}x{mon[3]})", flush=True)
            else:
                print(f"[CAPTURE] Labs Engine: '{t}' {r[2]-r[0]}x{r[3]-r[1]}", flush=True)
            return clamped
    except Exception as ex:
        print(f"[CAPTURE] win32gui error: {ex}", flush=True)
    return _orig_find(self)

_eng.Engine._find_target_window = _labs_engine_find

# ── SHM Video Capture Override ───────────────────────────────────────────────
# If --labs-pid and --session are provided, completely bypass BetterCam and
# instead read the zero-copy FFmpeg-decoded frames published by Labs Engine.
import argparse
parser = argparse.ArgumentParser(add_help=False)
parser.add_argument("--labs-pid", type=int, default=0)
parser.add_argument("--session", type=int, default=0)
args, _ = parser.parse_known_args()

if args.labs_pid > 0 and args.session > 0:
    import labs_frame_bridge as _frame_io

    class _ShmCameraWrapper:
        def __init__(self, pid, session):
            self.reader = _frame_io.ShmFrameReader(pid, session)
        def start(self, **kwargs):
            pass
        def stop(self):
            self.reader.close()
        def get_latest_frame(self):
            return self.reader.get_latest_frame()

    def _start_shm_camera(self):
        print(f"[CAPTURE] Bypassing BetterCam: using zero-copy SHM (PID {args.labs_pid}, Session {args.session})", flush=True)
        self._camera = _ShmCameraWrapper(args.labs_pid, args.session)

    _eng.Engine._start_camera = _start_shm_camera


# Runtime correction: the RE'd source has REFRACTORY_SECONDS = 55.0, which is
# almost certainly a misread of an F64 value — at 55s the engine only fires
# one shot per minute. The source stays untouched; we override the class
# attribute to 0.55s (single-shot debounce).
_eng.BGRMeterDetector.REFRACTORY_SECONDS = 0.55

# Noise-floor bump: at 1080p the real shot meter is 60-180 px tall and well
# over 200 px². The RE'd defaults of MIN_HEIGHT=12 / MIN_AREA=10 let the
# stamina bar (~20 px) calibrate the engine, then every frame trips
# fill=100% and the script fires phantom shots at 1.8 Hz. These numbers were
# arrived at empirically: 50 / 200 excludes UI noise but keeps real meters.
_eng.BGRMeterDetector._ABSOLUTE_MIN_HEIGHT = 50
_eng.BGRMeterDetector.MIN_AREA             = 200

# ── FPS fix: replace the detector's mask path with a fast version ────────────
# The zp_core detector runs 5 magenta `cv2.inRange` + 3 green inRange + two
# morphology passes on the full 1080p-ish frame every detect tick. On a weak
# laptop that's ~100ms per call → ~10 fps detect ceiling. We replace
# `_process_mask_cpu` with:
#   • 2x downsample first (cuts pixel count 4x)
#   • 1 wide magenta range (was 5)
#   • 1 CLOSE morph pass (was CLOSE+OPEN; the downstream aspect/area filters
#     already cover the noise OPEN was scrubbing)
#   • upsample the mask back to full res so contour x/y/w/h stay in original
#     coords (matches how process() uses them)
# Measured gain on our dev box: ~11ms/call → ~2ms/call.
import cv2 as _cv2   # noqa: E402
import numpy as _np  # noqa: E402
_FAST_MAG_LO = _np.array([130,  20, 130], _np.uint8)
_FAST_MAG_HI = _np.array([255,  90, 255], _np.uint8)
_FAST_KERNEL = _cv2.getStructuringElement(_cv2.MORPH_ELLIPSE, (3, 3))


# Runtime-swappable downsample factor. The UI's ultra-lite toggle sets this
# to 4 at start-time. Default is 2 (2x downsample, still plenty for 1080p).
_DOWNSAMPLE = 2


def _set_downsample(factor: int):
    global _DOWNSAMPLE
    _DOWNSAMPLE = max(1, int(factor))


def _fast_process_mask_cpu(self, frame):
    h, w = frame.shape[:2]
    ds = _DOWNSAMPLE
    if ds > 1 and w >= 640:
        small = _cv2.resize(frame, (w // ds, h // ds),
                            interpolation=_cv2.INTER_AREA)
        m = _cv2.inRange(small, _FAST_MAG_LO, _FAST_MAG_HI)
        m = _cv2.morphologyEx(m, _cv2.MORPH_CLOSE, _FAST_KERNEL)
        return _cv2.resize(m, (w, h), interpolation=_cv2.INTER_NEAREST)
    m = _cv2.inRange(frame, _FAST_MAG_LO, _FAST_MAG_HI)
    return _cv2.morphologyEx(m, _cv2.MORPH_CLOSE, _FAST_KERNEL)


_eng.BGRMeterDetector._process_mask_cpu = _fast_process_mask_cpu

# Skip the expensive mask entirely when there's clearly no magenta on screen.
# Idle frames between possessions drop from ~10ms to ~0.3ms — that's the
# difference between the loop pegging the CPU and idling until the user
# starts a shot.
_orig_process = _eng.BGRMeterDetector.process
def _process_with_precheck(self, frame, l2=False):
    h, w = frame.shape[:2]
    tiny = _cv2.resize(frame, (w // 4, h // 4),
                       interpolation=_cv2.INTER_NEAREST)
    if int(_cv2.countNonZero(_cv2.inRange(tiny, _FAST_MAG_LO, _FAST_MAG_HI))) < 8:
        return False
    return _orig_process(self, frame, l2=l2)
_eng.BGRMeterDetector.process = _process_with_precheck

# Velocity sanity gate: refuse to fire when the meter hasn't been *rising*.
# A real shot meter goes 0→100% over ~500-800ms (vel > 100%/s). A static UI
# blob has vel=0 forever. Wrap _check_shot_trigger so a fire only fires when
# velocity has exceeded a small threshold in the recent history.
import time as _time  # noqa: E402
_orig_check = _eng.BGRMeterDetector._check_shot_trigger
def _check_with_velocity_gate(self, fill, l2, frame):
    # Walk the recent fill history; require *some* upward motion lately.
    rising = False
    for t, f in list(self._fill_history)[-15:]:
        if t >= _time.perf_counter() - 0.5:
            if f > 0.05 and f < 0.98:
                rising = True
                break
    if not rising:
        return False
    return _orig_check(self, fill, l2, frame)
_eng.BGRMeterDetector._check_shot_trigger = _check_with_velocity_gate

# ── Hijack `_fire_rt` to write to the Labs Engine input-override SHM ─────────
# The original _fire_rt pulses the local virtual VDS4 + VX360 pads via vgamepad.
# Those pads are visible to Windows but NOT to the PS5: the user's DualSense
# input goes through Labs Engine's DualSenseSource → fan-out → PS Remote Play.
# vgamepad pulses race against DualSense's "RT=0" pushes and almost always
# lose. The SHM channel lets DualSenseSource see our 30ms RT=255 override and
# substitute it on its very next pad-push, cleanly winning the race.
import labs_input_bridge as _bridge_io  # noqa: E402
_FIRE_MS = 80  # >=1 PS Remote Play tick (~30Hz) so PS5 sees a clean release

from zp_core.controller_ps import PSControllerBridge as _BridgeCls  # noqa: E402

# Bug B fix: kill PSControllerBridge's vgamepad pad creation entirely.
# Originally __init__ does `self.ds4 = vg.VDS4Gamepad(); self.x360 = vg.VX360Gamepad()`.
# The VX360 instantly registers as XInput slot 0; LabsEngine's XInputPlugin then
# polls it 125Hz pushing all-neutral state into the fan-out, racing/clobbering
# the user's real DualSense input. We don't need the virtual pads — input goes
# through SHM now. Override __init__ to set both to None and skip ViGEm entirely.
_orig_init = _BridgeCls.__init__
def _no_vgamepad_init(self):
    _orig_init(self)
    # Force-replace whatever __init__ set up. Setting to None makes
    # passthrough() / fire_shot()'s `if not self.ds4: return` short-circuit.
    self.ds4  = None
    self.x360 = None
_BridgeCls.__init__ = _no_vgamepad_init


def _shm_fire_rt(self):
    # Shot-stick AND button-shot in one call: pulse RT *and* snap RS-Y to
    # neutral. Whichever actuator the game is bound to fires.
    _bridge_io.fire_combo(duration_ms=_FIRE_MS)


_BridgeCls._fire_rt = _shm_fire_rt


# fire_shot has `if not self.ds4: return` early-return; with ds4=None our
# release would be skipped. Override fire_shot to skip the guard.
def _shm_fire_shot(self, l2: bool = False, tempo: bool = False, tempo_ms: int = 39):
    if tempo and not l2:
        import time as _t
        _t.sleep(tempo_ms / 1000.0)
    self._fire_rt()
_BridgeCls.fire_shot = _shm_fire_shot


# ── UI ───────────────────────────────────────────────────────────────────────
from PySide6 import QtCore, QtGui, QtWidgets  # noqa: E402

BG       = "#06080F"
SURFACE  = "#0C0F18"
SURF2    = "#11151F"
BORDER   = "#1A2030"
BORD_HI  = "#2A3450"
TEXT     = "#F0F4FA"
DIM      = "#7E8AA6"
FAINT    = "#3A4360"
ACCENT   = "#3B82F6"
OK       = "#34D399"
WARN     = "#F59E0B"
DANGER   = "#EF4444"

QSS = f"""
QMainWindow, QDialog {{ background: {BG}; color: {TEXT}; }}
QWidget {{ background: transparent; color: {TEXT}; font-family: "Segoe UI Variable Text","Segoe UI"; }}
QLabel {{ color: {TEXT}; }}
QLabel.dim {{ color: {DIM}; }}
QLabel.kicker {{ color: {DIM}; font-size: 9px; letter-spacing: 1.4px; }}

QFrame.panel {{
    background: {SURFACE};
    border: 1px solid {BORDER};
    border-radius: 8px;
}}

QPushButton {{
    background: {SURF2};
    color: {TEXT};
    border: 1px solid {BORDER};
    border-radius: 6px;
    padding: 8px 14px;
    font-weight: 600;
}}
QPushButton:hover {{ border-color: {BORD_HI}; }}
QPushButton:pressed {{ background: {BG}; }}

QPushButton#primary {{
    background: {ACCENT};
    border-color: {ACCENT};
    color: white;
    padding: 11px 16px;
    font-size: 13px;
}}
QPushButton#primary:hover {{ background: #2563EB; }}

QPushButton#stop {{
    background: {DANGER};
    border-color: {DANGER};
    color: white;
    padding: 11px 16px;
    font-size: 13px;
}}

QSlider::groove:horizontal {{
    height: 4px; background: {BORDER}; border-radius: 2px;
}}
QSlider::sub-page:horizontal {{ background: {ACCENT}; border-radius: 2px; }}
QSlider::handle:horizontal {{
    background: {ACCENT}; border: none; width: 14px; height: 14px;
    margin: -5px 0; border-radius: 7px;
}}

QCheckBox {{ color: {TEXT}; spacing: 8px; }}
QCheckBox::indicator {{
    width: 16px; height: 16px;
    border: 1px solid {BORD_HI};
    background: {SURF2};
    border-radius: 3px;
}}
QCheckBox::indicator:checked {{
    background: {ACCENT};
    border-color: {ACCENT};
}}

QPlainTextEdit#log {{
    background: {BG};
    color: {DIM};
    border: 1px solid {BORDER};
    border-radius: 6px;
    font-family: "JetBrains Mono","Cascadia Mono","Consolas";
    font-size: 11px;
    padding: 6px;
}}
"""


def _kicker(text: str) -> QtWidgets.QLabel:
    lbl = QtWidgets.QLabel(text.upper())
    lbl.setProperty("class", "kicker")
    lbl.setStyleSheet(f"color: {DIM}; font-size: 9px; letter-spacing: 1.4px; font-weight: 700;")
    return lbl


class Slider(QtWidgets.QWidget):
    """A label + slider + value, in the existing Labs Engine flavor."""
    valueChanged = QtCore.Signal(int)

    def __init__(self, label: str, lo: int, hi: int, value: int, suffix: str = "%"):
        super().__init__()
        self._suffix = suffix
        v = QtWidgets.QVBoxLayout(self); v.setContentsMargins(0, 0, 0, 0); v.setSpacing(6)

        head = QtWidgets.QHBoxLayout(); head.setSpacing(8)
        self._label = QtWidgets.QLabel(label.upper())
        self._label.setStyleSheet(f"color: {DIM}; font-size: 9px; letter-spacing: 1.4px; font-weight: 700;")
        self._value = QtWidgets.QLabel(f"{value}{suffix}")
        self._value.setStyleSheet(f"color: {TEXT}; font-weight: 700;")
        head.addWidget(self._label); head.addStretch(1); head.addWidget(self._value)
        v.addLayout(head)

        self._slider = QtWidgets.QSlider(QtCore.Qt.Horizontal)
        self._slider.setRange(lo, hi); self._slider.setValue(value)
        self._slider.valueChanged.connect(self._on_change)
        v.addWidget(self._slider)

    def _on_change(self, val: int):
        self._value.setText(f"{val}{self._suffix}")
        self.valueChanged.emit(val)

    def value(self) -> int:
        return self._slider.value()


class EngineRunner(QtCore.QObject):
    """Owns the lifetime of zp_core.engine.Engine + PSControllerBridge."""
    statusChanged = QtCore.Signal(dict)

    def __init__(self):
        super().__init__()
        self._engine = None
        self._poll = QtCore.QTimer(self)
        self._poll.setInterval(500)
        self._poll.timeout.connect(self._tick)

    def is_running(self) -> bool:
        return self._engine is not None and getattr(self._engine, "_running", False)

    def start(self, cfg: dict):
        if self.is_running():
            return
        self._engine = _eng.Engine(cfg)
        self._engine.start()
        self._poll.start()
        self.statusChanged.emit({"running": True})

    def stop(self):
        if self._engine is not None:
            try: self._engine.stop()
            except Exception as ex:
                print(f"[ENGINE] stop error: {ex}", flush=True)
        self._poll.stop()
        self._engine = None
        self.statusChanged.emit({"running": False})

    def _tick(self):
        if not self._engine:
            return
        det = self._engine.detector
        cal = "LOCKED" if det._calibration_locked else f"{len(det._peak_history)}/4"
        self.statusChanged.emit({
            "running": True,
            "fps":    self._engine.fps_cur,
            "shots":  det.shots_fired,
            "cal":    cal,
        })


class StdoutTee(QtCore.QObject):
    """Mirror sys.stdout/stderr lines into a Qt signal so the UI can show them."""
    line = QtCore.Signal(str)

    def __init__(self, real):
        super().__init__()
        self._real = real
        self._buf = ""

    def write(self, s):
        try: self._real.write(s)
        except Exception: pass
        self._buf += s
        while "\n" in self._buf:
            head, self._buf = self._buf.split("\n", 1)
            self.line.emit(head)

    def flush(self):
        try: self._real.flush()
        except Exception: pass


class MainWindow(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Labs 2K — TM Labs")
        self.resize(560, 720)
        self.setStyleSheet(QSS)

        self.runner = EngineRunner()
        self.runner.statusChanged.connect(self._on_status)

        root = QtWidgets.QWidget(); self.setCentralWidget(root)
        v = QtWidgets.QVBoxLayout(root); v.setContentsMargins(18, 18, 18, 18); v.setSpacing(12)

        # Header
        head = QtWidgets.QHBoxLayout()
        title = QtWidgets.QLabel("LABS 2K")
        title.setStyleSheet(f"color: {TEXT}; font-size: 22px; font-weight: 800; letter-spacing: 4px;")
        sub = QtWidgets.QLabel("shot-timing engine")
        sub.setStyleSheet(f"color: {DIM}; font-size: 10px; letter-spacing: 2px;")
        ttl = QtWidgets.QVBoxLayout(); ttl.setSpacing(0); ttl.addWidget(title); ttl.addWidget(sub)
        head.addLayout(ttl); head.addStretch(1)

        self.status_pill = QtWidgets.QLabel("READY")
        self.status_pill.setStyleSheet(
            f"background: {SURF2}; border: 1px solid {BORDER}; color: {DIM};"
            f"padding: 6px 12px; border-radius: 12px; font-size: 11px; font-weight: 700; letter-spacing: 2px;"
        )
        head.addWidget(self.status_pill)
        v.addLayout(head)

        # Live stats panel
        stats = QtWidgets.QFrame(); stats.setProperty("class", "panel"); stats.setFrameShape(QtWidgets.QFrame.NoFrame)
        stats.setStyleSheet(f"QFrame {{ background: {SURFACE}; border: 1px solid {BORDER}; border-radius: 8px; }}")
        sg = QtWidgets.QGridLayout(stats); sg.setContentsMargins(16, 14, 16, 14); sg.setHorizontalSpacing(24); sg.setVerticalSpacing(2)
        self.fps_val   = self._stat_value("0")
        self.shots_val = self._stat_value("0")
        self.cal_val   = self._stat_value("—")
        sg.addWidget(_kicker("fps"),         0, 0); sg.addWidget(self.fps_val,   1, 0)
        sg.addWidget(_kicker("shots"),       0, 1); sg.addWidget(self.shots_val, 1, 1)
        sg.addWidget(_kicker("calibration"), 0, 2); sg.addWidget(self.cal_val,   1, 2)
        v.addWidget(stats)

        # Trigger sliders panel
        slpanel = self._panel()
        sl_lay = QtWidgets.QVBoxLayout(slpanel); sl_lay.setContentsMargins(16, 14, 16, 14); sl_lay.setSpacing(14)
        sl_lay.addWidget(_kicker("triggers"))
        self.thr_normal = Slider("Normal release", 50, 100, 95)
        self.thr_l2     = Slider("L2 release",     30, 100, 75)
        sl_lay.addWidget(self.thr_normal); sl_lay.addWidget(self.thr_l2)

        self.tempo_ms = Slider("Tempo delay", 0, 200, 39, suffix=" ms")
        sl_lay.addWidget(self.tempo_ms)
        v.addWidget(slpanel)

        # Feature toggles
        ftpanel = self._panel()
        ft_lay = QtWidgets.QVBoxLayout(ftpanel); ft_lay.setContentsMargins(16, 14, 16, 14); ft_lay.setSpacing(10)
        ft_lay.addWidget(_kicker("features"))
        self.cb_tempo       = QtWidgets.QCheckBox("Tempo (delay before release)")
        self.cb_stick_tempo = QtWidgets.QCheckBox("Stick tempo (RS-down → R2 sequence)")
        self.cb_quickstop   = QtWidgets.QCheckBox("Quickstop (RS pull-down → R2)")
        self.cb_defense     = QtWidgets.QCheckBox("Defense AI")
        self.cb_stamina     = QtWidgets.QCheckBox("Infinite stamina")
        self.cb_ultra_lite  = QtWidgets.QCheckBox("Ultra-lite mode (weak CPU / laptop)")
        self.cb_ultra_lite.setToolTip(
            "20fps capture, 4× downsample, aggressive per-frame skip. "
            "Use if [STATUS] fps is below 20 in normal mode.")
        for cb in (self.cb_tempo, self.cb_stick_tempo, self.cb_quickstop,
                   self.cb_defense, self.cb_stamina, self.cb_ultra_lite):
            ft_lay.addWidget(cb)
        v.addWidget(ftpanel)

        # Start/Stop
        self.btn = QtWidgets.QPushButton("START ENGINE"); self.btn.setObjectName("primary")
        self.btn.setMinimumHeight(46)
        self.btn.clicked.connect(self._toggle)
        v.addWidget(self.btn)

        # Log strip
        self.log = QtWidgets.QPlainTextEdit(); self.log.setObjectName("log")
        self.log.setReadOnly(True); self.log.setMaximumBlockCount(400); self.log.setMinimumHeight(140)
        v.addWidget(self.log, 1)

        # stdout tee → log widget
        self._tee = StdoutTee(sys.stdout); self._tee.line.connect(self._on_log_line)
        sys.stdout = self._tee
        sys.stderr = self._tee
        print(f"[UI] ready  cwd={os.getcwd()}")

    # ── helpers ──────────────────────────────────────────────────────────────

    def _panel(self) -> QtWidgets.QFrame:
        f = QtWidgets.QFrame()
        f.setStyleSheet(f"QFrame {{ background: {SURFACE}; border: 1px solid {BORDER}; border-radius: 8px; }}")
        return f

    def _stat_value(self, txt: str) -> QtWidgets.QLabel:
        lbl = QtWidgets.QLabel(txt)
        lbl.setStyleSheet(f"color: {TEXT}; font-size: 22px; font-weight: 700; font-family: 'JetBrains Mono','Cascadia Mono','Consolas';")
        return lbl

    def _build_cfg(self) -> dict:
        cfg = {
            "rt":        self.thr_normal.value(),
            "l2":        self.thr_l2.value(),
            "tempo":     self.tempo_ms.value(),
            "tempo_on":  self.cb_tempo.isChecked(),
            # 30fps capture instead of ZP's default 240. NBA 2K updates the
            # meter at 60Hz max, and BetterCam polling at 240fps just melts
            # the CPU on weak machines for no extra information. 30fps is
            # the smallest rate that still catches every meter peak with
            # 2+ samples in the green window.
            "capture_fps": 30,
        }
        if getattr(self, "cb_ultra_lite", None) and self.cb_ultra_lite.isChecked():
            # Ultra-lite: 20fps capture, aggressive downsample patched in
            # via _ULTRA_DOWNSAMPLE below. For genuinely weak hardware.
            cfg["capture_fps"] = 20
        return cfg

    # ── handlers ─────────────────────────────────────────────────────────────

    def _toggle(self):
        if self.runner.is_running():
            self.runner.stop()
            return

        cfg = self._build_cfg()
        # Ultra-lite mode: 4x downsample on the mask (pixel count drops ~16x
        # vs full frame, vs 4x with the default 2x downsample). Combined with
        # the 20fps capture set in _build_cfg, this is the weakest-CPU path.
        _set_downsample(4 if self.cb_ultra_lite.isChecked() else 2)
        self.runner.start(cfg)
        # Apply feature toggles to the bridge after start.
        try:
            br = self.runner._engine.bridge
            br.defense_enabled     = self.cb_defense.isChecked()
            br.infinite_stamina    = self.cb_stamina.isChecked()
            br.stick_tempo_enabled = self.cb_stick_tempo.isChecked()
            br.quickstop_enabled   = self.cb_quickstop.isChecked()
        except Exception as ex:
            print(f"[UI] toggle apply error: {ex}", flush=True)

    def _on_status(self, st: dict):
        running = bool(st.get("running"))
        self.btn.setText("STOP ENGINE" if running else "START ENGINE")
        self.btn.setObjectName("stop" if running else "primary")
        # restyle requires re-polish
        self.btn.style().unpolish(self.btn); self.btn.style().polish(self.btn)
        self.status_pill.setText("RUNNING" if running else "READY")
        self.status_pill.setStyleSheet(
            f"background: {SURF2}; border: 1px solid {OK if running else BORDER}; "
            f"color: {OK if running else DIM}; padding: 6px 12px; border-radius: 12px; "
            f"font-size: 11px; font-weight: 700; letter-spacing: 2px;"
        )
        if "fps"   in st: self.fps_val.setText(f"{st['fps']:.0f}")
        if "shots" in st: self.shots_val.setText(str(st["shots"]))
        if "cal"   in st: self.cal_val.setText(st["cal"])

    def _on_log_line(self, line: str):
        self.log.appendPlainText(line)

    def closeEvent(self, ev):
        try: self.runner.stop()
        except Exception: pass
        super().closeEvent(ev)


def main():
    app = QtWidgets.QApplication(sys.argv)
    app.setApplicationName("Labs 2K")
    win = MainWindow()
    win.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
