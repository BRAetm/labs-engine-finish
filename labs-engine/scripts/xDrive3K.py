"""xDrive 3K — Auto Tempo + meter-fill auto-release for Labs Engine.

Two trigger paths share the same shot dispatch:

  * Button (L3 fade / R3 standstill) — rising-edge XInput press fires the full
    rhythm combo (gather hold -> tempo flick -> release hold -> neutral).
  * CV (meter-fill auto-release) — ``xdrive3k_cv.CVDetector`` watches a
    user-calibrated ROI in the SHM frame stream and fires the same combo when
    the ROI's BGR fill % crosses the configured threshold.

CV is opt-in. Without ``--labs-pid``/``--session`` and without the user
running the calibrate flow, the script behaves exactly like the old
button-only build.

Input path (both triggers): labs_input_bridge SHM write -> LabsCore
InputOverride -> fan-out to ViGEm (Xbox mode) AND/OR PS Remote Play (PS mode).
"""

import argparse
import os
import sys
import time
import json
import queue
import ctypes
import ctypes.wintypes as W
import threading
from dataclasses import dataclass
from pathlib import Path

from PySide6 import QtCore, QtGui, QtWidgets

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))

import xui                            # noqa: E402   shared widget kit
import labs_input_bridge as labs      # noqa: E402   SHM bridge to LabsCore
import xdrive3k_cv as xcv             # noqa: E402   picker dialog + detector

# Labs Engine spawns this script via CvPythonPlugin with `--labs-pid <pid>
# --session <sid>` so the SHM frame block name resolves. We accept those plus
# the older `--session-id` LabsMainWindow uses, parsed at import-time so
# MainWindow.__init__ can branch on them. `parse_known_args` so the script
# stays runnable from a plain `python xDrive3K.py` with no flags.
_argparser = argparse.ArgumentParser(add_help=False)
_argparser.add_argument("--labs-pid", type=int, default=0)
_argparser.add_argument("--session", type=int, default=0)
_argparser.add_argument("--session-id", type=int, default=0, dest="session_id_alt")
_argparser.add_argument("--low-end", action="store_true")
ARGS, _ = _argparser.parse_known_args()
if ARGS.session == 0 and ARGS.session_id_alt:
    ARGS.session = ARGS.session_id_alt


SETTINGS_FILE = ROOT / "xdrive3k_settings.json"

DEFAULTS = {
    # Lock screen.
    "discord_id":          "",
    # Tempo shot — the only actual feature now.
    "tempo_enabled":       True,
    "btn_tempo_fade":      "L3",     # fade 3s
    "btn_tempo_normal":    "R3",     # standstill
    "tempo_fade_hold":     0.70,     # gather duration in seconds for fade
    "tempo_normal_hold":   0.55,     # gather duration in seconds for standstill
    # Per-shot rhythm tempo — the half-release wait. The 2K26 rhythm grade
    # window is 35-60 ms; outside reads as Hitch / Rushed.
    "tempo_fade_speed":    45,
    "tempo_normal_speed":  45,
    # CV meter-fill auto-release. ROI is empty until the user runs Calibrate;
    # `enabled=False` means the detector thread idles even when CV is wired.
    "cv": {
        "enabled":         False,
        "roi":             {"x": 0, "y": 0, "w": 0, "h": 0},
        "bgr_lo":          [0, 0, 0],
        "bgr_hi":          [255, 255, 255],
        "fire_at_pct":     95,
        "trigger_kind":    "normal",   # which combo CV fires: "fade"/"normal"/"auto"
    },
}


# Static per-kind shot profile. Both button paths and the CV auto-release pull
# from this table — single source of truth so adding a new fire path means
# adding one row, not editing three call sites.
@dataclass(frozen=True)
class ShotProfile:
    hold_key:    str        # settings key for gather duration (seconds)
    speed_key:   str        # settings key for half-release tempo (ms)
    gather_y:    int
    release_y:   int


SHOT_PROFILES: dict[str, ShotProfile] = {
    "fade":   ShotProfile("tempo_fade_hold",   "tempo_fade_speed",   +32767, -32767),
    "normal": ShotProfile("tempo_normal_hold", "tempo_normal_speed", -32767, +32767),
}


# ── Discord ID gate (lock screen) ───────────────────────────────────────────
_LOCAL_KEYS: dict[str, str] = {
    "0000": "LIFETIME",
    "0001": "WEEKLY",
    "0002": "MONTHLY",
}

TIER_GRADIENTS: dict[str, list[tuple[float, str]]] = {
    "LIFETIME": [(0.0, "#FFD86B"), (1.0, "#FF6B35")],   # gold -> orange
    "MONTHLY":  [(0.0, "#C084FC"), (1.0, "#EC4899")],   # violet -> magenta
    "WEEKLY":   [(0.0, "#67E8F9"), (1.0, "#3B82F6")],   # cyan -> blue
}
LOCKED_GRADIENT = [(0.0, "#7E8AA6"), (1.0, "#7E8AA6")]   # flat dim gray


def validate(discord_id: str) -> tuple[bool, str]:
    did = (discord_id or "").strip()
    dur = _LOCAL_KEYS.get(did, "")
    return bool(dur), dur


# ── XInput direct read ─────────────────────────────────────────────────────
BTN_NAMES = {
    "DPAD_UP":    0x0001, "DPAD_DOWN":  0x0002,
    "DPAD_LEFT":  0x0004, "DPAD_RIGHT": 0x0008,
    "START":      0x0010, "BACK":       0x0020, "VIEW": 0x0020, "MENU": 0x0010,
    "L3":         0x0040, "R3":         0x0080,
    "LB":         0x0100, "RB":         0x0200,
    "GUIDE":      0x0400, "XBOX":       0x0400,
    "A":          0x1000, "B":          0x2000,
    "X":          0x4000, "Y":          0x8000,
}


class _XI_GP(ctypes.Structure):
    _fields_ = [("wButtons", W.WORD), ("bLeftTrigger", W.BYTE),
                ("bRightTrigger", W.BYTE),
                ("sThumbLX", ctypes.c_short), ("sThumbLY", ctypes.c_short),
                ("sThumbRX", ctypes.c_short), ("sThumbRY", ctypes.c_short)]


class _XI_ST(ctypes.Structure):
    _fields_ = [("dwPacketNumber", W.DWORD), ("Gamepad", _XI_GP)]


_xinput = None
for _name in ("xinput1_4", "xinput1_3", "xinput9_1_0"):
    try:
        _xinput = ctypes.WinDLL(_name)
        break
    except OSError:
        pass


def read_xinput(slot):
    if _xinput is None:
        return None
    st = _XI_ST()
    if _xinput.XInputGetState(slot, ctypes.byref(st)) == 0:
        return st.Gamepad
    return None


# ── settings ──────────────────────────────────────────────────────────────
def _deep_merge(base: dict, over: dict) -> dict:
    """Recursive merge — `over` wins, missing keys in `over` get filled from
    `base`. Used so a partial cv block on disk still inherits new defaults
    when DEFAULTS gains a field."""
    out = dict(base)
    for k, v in over.items():
        if k in out and isinstance(out[k], dict) and isinstance(v, dict):
            out[k] = _deep_merge(out[k], v)
        else:
            out[k] = v
    return out


def load_settings() -> dict:
    if SETTINGS_FILE.exists():
        try:
            disk = json.loads(SETTINGS_FILE.read_text())
            return _deep_merge(DEFAULTS, disk)
        except Exception:
            pass
    return _deep_merge(DEFAULTS, {})  # fresh dict so callers can't mutate DEFAULTS


def save_settings(d: dict) -> None:
    """Atomic write — temp file + os.replace so a crash mid-write can't
    leave a half-written settings.json."""
    tmp = SETTINGS_FILE.with_suffix(SETTINGS_FILE.suffix + ".tmp")
    tmp.write_text(json.dumps(d, indent=2))
    os.replace(tmp, SETTINGS_FILE)


# ── tiny stepper widget — label · [-] · value · [+] · unit ────────────────
class StepperRow(QtWidgets.QWidget):
    valueChanged = QtCore.Signal(float)

    def __init__(self, label: str, lo: float, hi: float,
                 step: float, decimals: int = 1, unit: str = ""):
        super().__init__()
        self._lo, self._hi, self._step = float(lo), float(hi), float(step)
        self._dec = int(decimals)
        self._val = self._lo

        h = QtWidgets.QHBoxLayout(self)
        h.setContentsMargins(0, 0, 0, 0); h.setSpacing(8)

        lbl = QtWidgets.QLabel(label.upper())
        lbl.setFont(xui.label_font(8))
        lbl.setStyleSheet(f"color: {xui.DIM};")
        lbl.setFixedWidth(140)
        h.addWidget(lbl)

        btn_style = (f"QPushButton {{ background: {xui.SURFACE}; "
                     f"color: {xui.TEXT}; border: 1px solid {xui.BORDER}; "
                     f"border-radius: 6px; font-weight: 700; font-size: 14px; }}"
                     f"QPushButton:hover {{ border-color: {xui.ACCENT}; }}"
                     f"QPushButton:disabled {{ color: {xui.FAINT}; "
                     f"border-color: {xui.BORDER}; }}")
        self._minus = QtWidgets.QPushButton("−")  # minus
        self._minus.setFixedSize(32, 28)
        self._minus.setStyleSheet(btn_style)
        self._minus.clicked.connect(lambda: self._step_by(-self._step))
        h.addWidget(self._minus)

        self._val_lbl = QtWidgets.QLabel("0")
        self._val_lbl.setFont(xui.mono_font(12, 700))
        self._val_lbl.setAlignment(QtCore.Qt.AlignmentFlag.AlignCenter)
        self._val_lbl.setFixedWidth(70)
        self._val_lbl.setStyleSheet(f"color: {xui.TEXT};")
        h.addWidget(self._val_lbl)

        self._plus = QtWidgets.QPushButton("+")
        self._plus.setFixedSize(32, 28)
        self._plus.setStyleSheet(btn_style)
        self._plus.clicked.connect(lambda: self._step_by(+self._step))
        h.addWidget(self._plus)

        if unit:
            u = QtWidgets.QLabel(unit)
            u.setFont(xui.label_font(7))
            u.setStyleSheet(f"color: {xui.FAINT};")
            u.setFixedWidth(28)
            h.addWidget(u)
        h.addStretch(1)

    def _step_by(self, delta: float):
        new = max(self._lo, min(self._hi, self._val + delta))
        new = round(round(new / self._step) * self._step, self._dec + 2)
        if abs(new - self._val) < 1e-9:
            return
        self._val = new
        self._refresh()
        self.valueChanged.emit(self._val)

    def setValue(self, v):
        self._val = max(self._lo, min(self._hi, float(v)))
        self._refresh()

    def value(self) -> float:
        return self._val

    def _refresh(self):
        self._val_lbl.setText(f"{self._val:.{self._dec}f}")
        self._minus.setEnabled(self._val > self._lo + 1e-9)
        self._plus.setEnabled(self._val < self._hi - 1e-9)


# ── controller poll thread ────────────────────────────────────────────────
class ControllerListener(QtCore.QThread):
    """Polls slots 1/2/3 at 250Hz. Emits rising-edge signals for the two
       tempo buttons (L3 fade / R3 standstill)."""
    fire_tempo_fade   = QtCore.Signal()
    fire_tempo_normal = QtCore.Signal()
    slot_latched      = QtCore.Signal(int)

    def __init__(self, settings_ref: dict):
        super().__init__()
        self._stop = False
        self._settings = settings_ref
        self._active_slot = -1
        self._last_btns = 0

    def run(self):
        while not self._stop:
            gp = None
            for slot in (1, 2, 3):
                gp = read_xinput(slot)
                if gp is not None:
                    if self._active_slot != slot:
                        self._active_slot = slot
                        self._last_btns = 0
                        self.slot_latched.emit(slot)
                    break
            if gp is None:
                self._active_slot = -1
                self._last_btns   = 0
                self.msleep(50)
                continue
            btns = gp.wButtons
            pressed = btns & ~self._last_btns
            self._last_btns = btns
            if self._settings.get("tempo_enabled", True):
                tf_bit = BTN_NAMES.get(self._settings.get("btn_tempo_fade",   "L3"), 0x0040)
                tn_bit = BTN_NAMES.get(self._settings.get("btn_tempo_normal", "R3"), 0x0080)
                if pressed & tf_bit:
                    self.fire_tempo_fade.emit()
                if pressed & tn_bit:
                    self.fire_tempo_normal.emit()
            self.msleep(4)

    def stop(self):
        self._stop = True


# ── main window ───────────────────────────────────────────────────────────
class MainWindow(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("xDrive 3K")
        self.setWindowFlags(QtCore.Qt.WindowType.FramelessWindowHint)
        self.setAttribute(QtCore.Qt.WidgetAttribute.WA_TranslucentBackground, True)
        self.setStyleSheet(xui.STYLESHEET + "QMainWindow { background: transparent; }")
        self.resize(880, 540)
        self.setMinimumSize(800, 480)

        self.s = load_settings()
        self._tempo_lock = threading.Lock()
        self.cv: "xcv.CVDetector | None" = None  # built after UI; _build_cv_card reads it

        # Persistent shot dispatch worker. `_fire_kind` enqueues; the worker
        # pulls one job at a time. queue.maxsize=1 + put_nowait drops new jobs
        # while a combo is already running, so a CV false-positive storm can't
        # spawn 30 combo threads in parallel.
        self._dispatch_q: queue.Queue = queue.Queue(maxsize=1)
        self._dispatch_stop = threading.Event()
        self._dispatcher = threading.Thread(target=self._dispatch_loop,
                                            name="xdrive3k-dispatch",
                                            daemon=True)
        self._dispatcher.start()

        self._build_ui()
        self._apply_lock()
        self._position_ctrls()

        # Settings save debounce: every _set() pushes a 500ms timer; rapid
        # edits coalesce into one disk write instead of stalling the GUI
        # thread on every keystroke.
        self._save_timer = QtCore.QTimer(self)
        self._save_timer.setSingleShot(True)
        self._save_timer.setInterval(500)
        self._save_timer.timeout.connect(lambda: save_settings(self.s))

        # Controller listener thread.
        self.listener = ControllerListener(self.s)
        self.listener.fire_tempo_fade.connect(lambda: self._fire_kind("fade"))
        self.listener.fire_tempo_normal.connect(lambda: self._fire_kind("normal"))
        self.listener.slot_latched.connect(lambda n: self.log(f"Controller on slot {n}"))
        self.listener.start()

        # CV detector: only built when --labs-pid + --session were passed.
        # Otherwise the CV panel still renders but Calibrate is disabled.
        pid, sid = ARGS.labs_pid, ARGS.session
        if pid > 0 and sid > 0:
            self.cv = xcv.CVDetector(pid, sid, parent=self)
            # QueuedConnection routes the slot back to the GUI thread so the
            # cv2 work stays off it. _fire_kind itself is thread-safe (queues
            # to the dispatcher) but the log + UI updates aren't.
            self.cv.thresholdCrossed.connect(
                self._on_cv_fire, QtCore.Qt.ConnectionType.QueuedConnection)
            self.cv.fillChanged.connect(
                self._on_cv_fill, QtCore.Qt.ConnectionType.QueuedConnection)
            self.cv.statusChanged.connect(
                self._on_cv_status, QtCore.Qt.ConnectionType.QueuedConnection)
            self.cv.apply_settings(xcv.CVSnapshot.from_dict(self.s))
            self.cv.start()
            # _build_cv_card ran before self.cv existed; flip the UI now.
            self._cv_calibrate_btn.setEnabled(True)
            self._cv_calibrate_btn.setToolTip("")
            cur = xcv.CVSnapshot.from_dict(self.s)
            if cur.calibrated:
                self._cv_status_lbl.setText("READY")
                self._cv_status_lbl.setStyleSheet(f"color: {xui.OK};")
            else:
                self._cv_status_lbl.setText("NEEDS CAL")
                self._cv_status_lbl.setStyleSheet(f"color: {xui.WARN};")

    # ── layout ────────────────────────────────────────────────────────────
    def _build_ui(self):
        root = xui.RoundedRoot(radius=14, color=xui.BG)
        v = QtWidgets.QVBoxLayout(root); v.setContentsMargins(0, 0, 0, 0); v.setSpacing(0)

        # Window controls (min / close) overlay top-right of the banner.
        self._ctrl_bar = QtWidgets.QWidget(self)
        cb = QtWidgets.QHBoxLayout(self._ctrl_bar)
        cb.setContentsMargins(0, 0, 0, 0); cb.setSpacing(0)
        bmin = xui.WinBtn("min");   bmin.clicked.connect(self.showMinimized)
        bcls = xui.WinBtn("close"); bcls.clicked.connect(self.close)
        cb.addWidget(bmin); cb.addWidget(bcls)

        v.addWidget(xui.HeroBanner(height=180))
        v.addWidget(self._build_topbar())
        v.addWidget(xui._divider())

        # Stacked body — locked screen on page 0, real UI on page 1.
        self._body_stack = QtWidgets.QStackedWidget()
        self._body_stack.setStyleSheet(f"background: {xui.BG};")
        self._body_stack.addWidget(self._build_locked())
        self._body_stack.addWidget(self._build_unlocked())
        v.addWidget(self._body_stack, 1)

        self.setCentralWidget(root)

    def resizeEvent(self, e):
        super().resizeEvent(e)
        if hasattr(self, "_ctrl_bar"):
            self._position_ctrls()
        path = QtGui.QPainterPath()
        path.addRoundedRect(QtCore.QRectF(self.rect()), 14, 14)
        self.setMask(QtGui.QRegion(path.toFillPolygon().toPolygon()))

    def _position_ctrls(self):
        self._ctrl_bar.adjustSize()
        self._ctrl_bar.move(self.width() - self._ctrl_bar.width(), 0)
        self._ctrl_bar.raise_()

    def _build_topbar(self) -> QtWidgets.QWidget:
        bar = QtWidgets.QWidget()
        bar.setFixedHeight(80)
        bar.setStyleSheet(f"background: {xui.SURFACE}; border-bottom: 1px solid {xui.BORDER};")
        h = QtWidgets.QHBoxLayout(bar)
        h.setContentsMargins(28, 0, 28, 0); h.setSpacing(16)

        lbl_top = QtWidgets.QLabel("DISCORD ID")
        f_lbl = xui.body_font(10, 700)
        f_lbl.setCapitalization(QtGui.QFont.Capitalization.AllUppercase)
        f_lbl.setLetterSpacing(QtGui.QFont.SpacingType.AbsoluteSpacing, 2.6)
        lbl_top.setFont(f_lbl)
        lbl_top.setStyleSheet(f"color: {xui.DIM};")
        h.addWidget(lbl_top)

        self._id_field = QtWidgets.QLineEdit()
        self._id_field.setPlaceholderText("0000")
        self._id_field.setText(str(self.s.get("discord_id", "")))
        self._id_field.setFixedHeight(44)
        self._id_field.setMinimumWidth(220)
        self._id_field.setMaximumWidth(280)
        f_in = xui.mono_font(15, 700)
        f_in.setLetterSpacing(QtGui.QFont.SpacingType.AbsoluteSpacing, 1.5)
        self._id_field.setFont(f_in)
        self._id_field.setStyleSheet(
            f"QLineEdit {{"
            f"  background: {xui.BG};"
            f"  color: {xui.TEXT};"
            f"  border: 1px solid {xui.BORDER};"
            f"  border-radius: 8px;"
            f"  padding: 0 14px;"
            f"  selection-background-color: {xui.ACCENT};"
            f"}}"
            f"QLineEdit:hover {{ border-color: {xui.BORD_HI}; }}"
            f"QLineEdit:focus {{ border-color: {xui.ACCENT}; }}"
        )
        self._id_field.textEdited.connect(self._on_id_edit)
        h.addWidget(self._id_field)
        h.addStretch(1)

        self._key_val = xui.GradientLabel("KEY: LOCKED")
        self._key_val.setFont(xui.display_font(14, 900))
        self._key_val.setAlignment(QtCore.Qt.AlignmentFlag.AlignVCenter
                                   | QtCore.Qt.AlignmentFlag.AlignRight)
        self._key_val.setGradient(LOCKED_GRADIENT)
        h.addWidget(self._key_val)

        return bar

    def _on_id_edit(self, v: str):
        self.s["discord_id"] = v
        save_settings(self.s)
        self._apply_lock()

    def _apply_lock(self):
        ok, dur = validate(str(self.s.get("discord_id", "")))
        if ok:
            self._key_val.setText(f"KEY: {dur}")
            self._key_val.setGradient(TIER_GRADIENTS.get(dur, LOCKED_GRADIENT))
        else:
            self._key_val.setText("KEY: LOCKED")
            self._key_val.setGradient(LOCKED_GRADIENT)
        self._body_stack.setCurrentIndex(1 if ok else 0)

    def _build_locked(self) -> QtWidgets.QWidget:
        w = QtWidgets.QWidget()
        w.setStyleSheet(f"background: {xui.BG};")
        v = QtWidgets.QVBoxLayout(w)
        v.setContentsMargins(0, 0, 0, 24)
        v.addStretch(2)
        headline = QtWidgets.QLabel("ENTER DISCORD ID")
        headline.setFont(xui.display_font(34, 900))
        headline.setAlignment(QtCore.Qt.AlignmentFlag.AlignCenter)
        headline.setStyleSheet(f"color: {xui.TEXT};")
        v.addWidget(headline)
        sub = QtWidgets.QLabel("to unlock the engine")
        sub.setFont(xui.ui_font(11))
        sub.setAlignment(QtCore.Qt.AlignmentFlag.AlignCenter)
        sub.setStyleSheet(f"color: {xui.DIM};")
        v.addWidget(sub)
        v.addStretch(3)
        return w

    def _build_unlocked(self) -> QtWidgets.QWidget:
        wrap = QtWidgets.QWidget()
        wrap.setStyleSheet(f"background: {xui.BG};")
        col = QtWidgets.QVBoxLayout(wrap)
        col.setContentsMargins(20, 12, 20, 0); col.setSpacing(8)

        # Tab bar — two modes: button-driven Auto Tempo and CV meter release.
        tab_bar = QtWidgets.QHBoxLayout(); tab_bar.setSpacing(10)
        self._tab_tempo = xui.TabBtn("AUTO TEMPO")
        self._tab_cv    = xui.TabBtn("CV METER")
        self._tab_tempo.setChecked(True)
        tab_bar.addWidget(self._tab_tempo)
        tab_bar.addWidget(self._tab_cv)
        tab_bar.addStretch(1)
        col.addLayout(tab_bar)

        # Stacked pages.
        self._page_stack = QtWidgets.QStackedWidget()
        self._page_stack.setStyleSheet(f"background: {xui.BG};")
        self._page_stack.addWidget(self._wrap_scroll(self._page_auto_tempo()))
        self._page_stack.addWidget(self._wrap_scroll(self._page_cv()))
        col.addWidget(self._page_stack, 1)

        self._tab_tempo.toggled.connect(lambda v: v and self._select_tab(0))
        self._tab_cv.toggled.connect(   lambda v: v and self._select_tab(1))

        return wrap

    def _select_tab(self, idx: int):
        # Mutual exclusion — TabBtn is checkable, no QButtonGroup; do it by hand.
        self._tab_tempo.setChecked(idx == 0)
        self._tab_cv.setChecked(idx == 1)
        self._page_stack.setCurrentIndex(idx)

    def _wrap_scroll(self, page: QtWidgets.QWidget) -> QtWidgets.QScrollArea:
        xui.apply_body_font_recursive(page)
        sa = QtWidgets.QScrollArea()
        sa.setWidget(page); sa.setWidgetResizable(True)
        sa.setFrameShape(QtWidgets.QFrame.Shape.NoFrame)
        sa.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        sa.setStyleSheet(f"QScrollArea {{ background: {xui.BG}; border: none; }}")
        return sa

    def _page_auto_tempo(self) -> QtWidgets.QWidget:
        """Single page — Auto Tempo only. L3 fade / R3 standstill, with
           per-button gather hold and rhythm tempo. No CV, no toggles
           beyond enable/disable, no MODE dropdown."""
        page = QtWidgets.QWidget()
        v = QtWidgets.QVBoxLayout(page)
        v.setContentsMargins(20, 16, 20, 16); v.setSpacing(14)

        tp_card = xui.GridCard()
        cv = QtWidgets.QVBoxLayout(tp_card)
        cv.setContentsMargins(20, 16, 20, 16); cv.setSpacing(12)
        cv.addWidget(xui.SectionLabel("TEMPO SHOTS  (L3 fade / R3 standstill)"))

        self.tp_toggle = xui.StateToggle("Auto Tempo")
        self.tp_toggle.setChecked(self.s["tempo_enabled"])
        self.tp_toggle.toggled.connect(lambda v: self._set("tempo_enabled", v))
        cv.addWidget(self.tp_toggle)

        # Gather durations — micro-adjustments by 0.01 s.
        self.tp_fade_hold = StepperRow(
            "L3 Fade Hold",   0.20, 1.50, step=0.01, decimals=2, unit="s")
        self.tp_fade_hold.setValue(float(self.s.get("tempo_fade_hold", 0.70)))
        self.tp_fade_hold.valueChanged.connect(
            lambda v: self._set("tempo_fade_hold", v))
        cv.addWidget(self.tp_fade_hold)

        self.tp_normal_hold = StepperRow(
            "R3 Stand Hold",  0.20, 1.50, step=0.01, decimals=2, unit="s")
        self.tp_normal_hold.setValue(float(self.s.get("tempo_normal_hold", 0.55)))
        self.tp_normal_hold.valueChanged.connect(
            lambda v: self._set("tempo_normal_hold", v))
        cv.addWidget(self.tp_normal_hold)

        # Rhythm tempo — wait at the half-release position. 35-60 ms is
        # the 2K26 rhythm grade window; outside reads as Hitch / Rushed.
        self.tp_fade_speed = StepperRow(
            "L3 Fade Tempo",  15, 200, step=1, decimals=0, unit="ms")
        self.tp_fade_speed.setValue(float(self.s.get("tempo_fade_speed", 45)))
        self.tp_fade_speed.valueChanged.connect(
            lambda v: self._set("tempo_fade_speed", int(v)))
        cv.addWidget(self.tp_fade_speed)

        self.tp_normal_speed = StepperRow(
            "R3 Stand Tempo", 15, 200, step=1, decimals=0, unit="ms")
        self.tp_normal_speed.setValue(float(self.s.get("tempo_normal_speed", 45)))
        self.tp_normal_speed.valueChanged.connect(
            lambda v: self._set("tempo_normal_speed", int(v)))
        cv.addWidget(self.tp_normal_speed)

        v.addWidget(tp_card)

        # Quick how-to so the user knows the gesture, no scrolling needed.
        hint = QtWidgets.QLabel(
            "Press L3 to fire a fade-3 tempo shot, R3 for standstill. "
            "Each button drives the full RS combo automatically — gather "
            "hold, rhythm flick at the configured tempo, release hold, "
            "and a brief neutral so your stick reclaims control cleanly. "
            "Tune the holds in 0.01s steps and the tempo in 1ms steps."
        )
        hint.setFont(xui.ui_font(9))
        hint.setStyleSheet(f"color: {xui.DIM};")
        hint.setWordWrap(True)
        v.addWidget(hint)

        v.addStretch(1)
        return page

    def _page_cv(self) -> QtWidgets.QWidget:
        """CV tab — meter-fill auto-release. Single card per tab keeps the
        layout breathing; the page itself is just the card + a hint."""
        page = QtWidgets.QWidget()
        v = QtWidgets.QVBoxLayout(page)
        v.setContentsMargins(20, 16, 20, 16); v.setSpacing(14)

        v.addWidget(self._build_cv_card())

        hint = QtWidgets.QLabel(
            "Calibrate once: drag a rectangle around the meter on the frozen "
            "frame, then dial BGR Tolerance until the green overlay covers "
            "the meter cleanly. Toggle Auto Release on, set the fire-at fill "
            "% (95% is a good start), pick which combo CV fires (NORMAL or "
            "FADE — uses the timing tuned in the AUTO TEMPO tab), and the "
            "detector will release the moment the meter peaks. Buttons keep "
            "working as before; CV is additive."
        )
        hint.setFont(xui.ui_font(9))
        hint.setStyleSheet(f"color: {xui.DIM};")
        hint.setWordWrap(True)
        v.addWidget(hint)

        v.addStretch(1)
        return page

    def _build_cv_card(self) -> QtWidgets.QWidget:
        """Meter-fill auto-release. Toggle, fire-at slider, calibrate button,
        live fill % readout, status pill. The Calibrate button is disabled
        when the script wasn't launched with --labs-pid/--session — there's
        no SHM stream to sample."""
        card = xui.GridCard()
        cl = QtWidgets.QVBoxLayout(card)
        cl.setContentsMargins(20, 16, 20, 16); cl.setSpacing(12)
        cl.addWidget(xui.SectionLabel("METER AUTO-RELEASE  (CV)"))

        cv_cfg = self.s.get("cv", {})

        self._cv_toggle = xui.StateToggle("Auto Release on Meter Fill")
        self._cv_toggle.setChecked(bool(cv_cfg.get("enabled", False)))
        self._cv_toggle.toggled.connect(lambda v: self._set("cv.enabled", v))
        cl.addWidget(self._cv_toggle)

        # Fire-at fill threshold.
        fire_at = StepperRow("Fire At", 50, 100, step=1, decimals=0, unit="%")
        fire_at.setValue(float(cv_cfg.get("fire_at_pct", 95)))
        fire_at.valueChanged.connect(lambda v: self._set("cv.fire_at_pct", int(v)))
        cl.addWidget(fire_at)

        # Live readouts row.
        row = QtWidgets.QHBoxLayout(); row.setSpacing(16)

        col_fill = QtWidgets.QVBoxLayout(); col_fill.setSpacing(2)
        col_fill.addWidget(xui.FieldLabel("FILL"))
        self._cv_fill_lbl = QtWidgets.QLabel("--%")
        self._cv_fill_lbl.setFont(xui.mono_font(14, 700))
        self._cv_fill_lbl.setStyleSheet(f"color: {xui.TEXT};")
        col_fill.addWidget(self._cv_fill_lbl)
        row.addLayout(col_fill)

        col_status = QtWidgets.QVBoxLayout(); col_status.setSpacing(2)
        col_status.addWidget(xui.FieldLabel("STATUS"))
        is_cal = bool(cv_cfg.get("roi", {}).get("w", 0)) > 0
        self._cv_status_lbl = QtWidgets.QLabel(
            "READY" if is_cal else "NEEDS CAL")
        self._cv_status_lbl.setFont(xui.mono_font(11, 700))
        self._cv_status_lbl.setStyleSheet(f"color: {xui.OK if is_cal else xui.WARN};")
        col_status.addWidget(self._cv_status_lbl)
        row.addLayout(col_status)

        col_kind = QtWidgets.QVBoxLayout(); col_kind.setSpacing(2)
        col_kind.addWidget(xui.FieldLabel("FIRES"))
        self._cv_kind_combo = QtWidgets.QComboBox()
        self._cv_kind_combo.addItems(["normal", "fade"])
        cur_kind = str(cv_cfg.get("trigger_kind", "normal"))
        if cur_kind in ("normal", "fade"):
            self._cv_kind_combo.setCurrentText(cur_kind)
        self._cv_kind_combo.currentTextChanged.connect(
            lambda t: self._set("cv.trigger_kind", t))
        col_kind.addWidget(self._cv_kind_combo)
        row.addLayout(col_kind)

        row.addStretch(1)

        self._cv_calibrate_btn = QtWidgets.QPushButton("CALIBRATE")
        self._cv_calibrate_btn.setProperty("accent", "true")
        self._cv_calibrate_btn.setMinimumHeight(36)
        self._cv_calibrate_btn.setMinimumWidth(150)
        self._cv_calibrate_btn.clicked.connect(self._on_calibrate)
        row.addWidget(self._cv_calibrate_btn)

        cl.addLayout(row)

        if self.cv is None:
            self._cv_calibrate_btn.setEnabled(False)
            self._cv_calibrate_btn.setToolTip(
                "Launch xDrive3K from Labs Engine (Settings → CV Script Path) "
                "so it receives --labs-pid/--session and can read the SHM frame stream.")
            self._cv_status_lbl.setText("NO STREAM")
            self._cv_status_lbl.setStyleSheet(f"color: {xui.DIM};")

        return card

    def _on_calibrate(self):
        if self.cv is None:
            return
        # Pause the detector so it doesn't fight the picker for SHM frames.
        try:
            cur = xcv.CVSnapshot.from_dict(self.s)
            self.cv.apply_settings(xcv.CVSnapshot(
                enabled=False, roi=cur.roi, bgr_lo=cur.bgr_lo,
                bgr_hi=cur.bgr_hi, fire_at_pct=cur.fire_at_pct))
            dlg = xcv.CalibrateDialog(ARGS.labs_pid, ARGS.session, parent=self)
            if dlg.exec() == QtWidgets.QDialog.DialogCode.Accepted:
                res = dlg.result_settings()
                if res:
                    self._set("cv.roi",    res["roi"])
                    self._set("cv.bgr_lo", res["bgr_lo"])
                    self._set("cv.bgr_hi", res["bgr_hi"])
                    self._cv_status_lbl.setText("READY")
                    self._cv_status_lbl.setStyleSheet(f"color: {xui.OK};")
                    self.log(f"CV calibrated: ROI={res['roi']}  "
                             f"BGR={res['bgr_lo']} -> {res['bgr_hi']}")
        finally:
            # Restore enabled state from settings.
            self._push_cv_snapshot()

    # ── shot execution ────────────────────────────────────────────────────
    def _fire_kind(self, kind: str, source: str = "btn"):
        """Single fire entry point. Buttons call this with kind="fade"/"normal".
           CV calls it with the user-configured `cv.trigger_kind`."""
        if not self.s.get("tempo_enabled", True):
            return
        prof = SHOT_PROFILES.get(kind)
        if prof is None:
            self.log(f"unknown shot kind: {kind!r}"); return
        hold_s   = float(self.s.get(prof.hold_key,  0.55))
        speed_ms = max(15, min(200, int(self.s.get(prof.speed_key, 45))))

        # tempo_lock prevents overlapping combos. CV firing during a combo is
        # a no-op (refractory-gated on the detector side too).
        if not self._tempo_lock.acquire(blocking=False):
            return
        # Try to enqueue the combo job. dispatch_q is bounded so a CV
        # false-positive storm during an in-flight combo gets dropped here
        # rather than spawning threads. The lock is released by the worker.
        try:
            self._dispatch_q.put_nowait((prof.gather_y, prof.release_y,
                                          speed_ms, hold_s))
        except queue.Full:
            self._tempo_lock.release()
            return
        self.log(f"FIRE {source}/{kind.upper()}  hold={hold_s:.2f}s  "
                 f"tempo={speed_ms}ms  ({prof.gather_y:+d} -> {prof.release_y:+d})")

    def _dispatch_loop(self):
        """Persistent worker. Pulls one (gather_y, release_y, speed_ms,
        hold_s) tuple at a time and runs the full combo. Releases
        `_tempo_lock` after the combo (or on exception)."""
        while not self._dispatch_stop.is_set():
            try:
                job = self._dispatch_q.get(timeout=0.25)
            except queue.Empty:
                continue
            try:
                self._run_combo(*job)
            except Exception as ex:
                print(f"[xDrive3K] combo error: {ex}", flush=True)
            finally:
                if self._tempo_lock.locked():
                    self._tempo_lock.release()

    def _run_combo(self, gather_y: int, release_y: int,
                   speed_ms: int, hold_s: float):
        """Full button-triggered tempo combo:
              0. GATHER  — RS at gather_y for hold_s seconds
              1. HALF    — RS at half release_y for speed_ms (2K's "Tempo")
              2. FULL    — RS at full release_y for 350 ms (release anim)
              3. NEUTRAL — RS at 0 for 400 ms; user reclaims the stick
        """
        # PHASE 0 — GATHER. Re-assert the override every 100 ms so it
        # never expires mid-gather (the SHM record's max sane lifetime
        # is ~1 s per the C++ sanity guard).
        slice_ms = 100
        slices   = max(1, int(round(hold_s * 1000 / slice_ms)))
        for _ in range(slices):
            labs.set_rs_xy(0, gather_y, slice_ms + 50)
            time.sleep(slice_ms / 1000.0)

        # PHASE 1 — HALF release for the tempo wait. The Zen pattern
        # (and the 2K26 rhythm grader) reads the time spent at this
        # midpoint as the "Tempo" call.
        half_y   = release_y // 2
        tempo_ms = max(15, int(speed_ms))
        labs.set_rs_xy(0, half_y, tempo_ms + 30)
        time.sleep(tempo_ms / 1000.0)

        # PHASE 2 — FULL release. Sustained full-release flags rhythm.
        labs.set_rs_xy(0, release_y, 380)
        time.sleep(0.350)

        # PHASE 3 — NEUTRAL; user's physical RS returns when the
        # override expires. 400 ms keeps a still-held stick from
        # immediately re-engaging the gather.
        labs.set_rs_xy(0, 0, 400)

    # ── CV slots (run on GUI thread via QueuedConnection) ────────────────
    def _on_cv_fire(self):
        kind = str(self.s.get("cv", {}).get("trigger_kind", "normal"))
        self._fire_kind(kind, source="cv")

    def _on_cv_fill(self, pct: float):
        if hasattr(self, "_cv_fill_lbl"):
            self._cv_fill_lbl.setText(f"{pct:5.1f}%")
            col = xui.OK if pct >= float(self.s["cv"].get("fire_at_pct", 95)) else xui.TEXT
            self._cv_fill_lbl.setStyleSheet(f"color: {col};")

    def _on_cv_status(self, msg: str):
        self.log(f"CV: {msg}")
        if hasattr(self, "_cv_status_lbl"):
            self._cv_status_lbl.setText(msg.upper())

    # ── settings + logging plumbing ───────────────────────────────────────
    def _set(self, key: str, value):
        # Supports nested keys like "cv.fire_at_pct" via dotted paths.
        if "." in key:
            head, _, tail = key.partition(".")
            d = self.s.setdefault(head, {})
            self._set_nested(d, tail, value)
        else:
            self.s[key] = value
        self._push_cv_snapshot()
        self._save_timer.start()  # debounced disk write

    def _set_nested(self, d: dict, dotted: str, value):
        head, _, tail = dotted.partition(".")
        if tail:
            self._set_nested(d.setdefault(head, {}), tail, value)
        else:
            d[head] = value

    def _push_cv_snapshot(self):
        if self.cv is not None:
            self.cv.apply_settings(xcv.CVSnapshot.from_dict(self.s))

    def log(self, msg: str):
        ts = time.strftime("%H:%M:%S")
        print(f"[xDrive3K {ts}] {msg}", flush=True)

    # ── teardown ──────────────────────────────────────────────────────────
    def closeEvent(self, e):
        # Stop CV first — its thread holds an SHM mapping that should be
        # released before we leave Python.
        try:
            if self.cv is not None:
                self.cv.stop(); self.cv.wait(500)
        except Exception: pass
        try:
            self.listener.stop(); self.listener.wait(300)
        except Exception: pass
        self._dispatch_stop.set()
        try: self._dispatcher.join(0.5)
        except Exception: pass
        # Flush any pending debounced write so config edits aren't lost on
        # immediate close.
        try:
            if self._save_timer.isActive():
                self._save_timer.stop()
                save_settings(self.s)
        except Exception: pass
        try: labs.clear()
        except Exception: pass
        super().closeEvent(e)


# ── entry point ───────────────────────────────────────────────────────────
def main():
    # Labs Engine spawns the script with cp1252 stdout encoding, which can't
    # handle unicode. Force UTF-8 so log lines with non-ASCII don't kill the
    # process before the window even opens.
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
        sys.stderr.reconfigure(encoding="utf-8", errors="replace")
    except Exception:
        pass

    app = QtWidgets.QApplication(sys.argv)
    # Register bundled gaming fonts BEFORE setting the stylesheet so the
    # _fam() fallback chains in xui actually find them.
    fonts_dir = ROOT / "userdata" / "fonts"
    if fonts_dir.exists():
        for ttf in fonts_dir.glob("*.ttf"):
            fid = QtGui.QFontDatabase.addApplicationFont(str(ttf))
            if fid >= 0:
                fams = QtGui.QFontDatabase.applicationFontFamilies(fid)
                print(f"[xDrive3K] registered font: {ttf.name} -> {fams}", flush=True)
    app.setStyleSheet(xui.STYLESHEET)
    win = MainWindow()
    win.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
