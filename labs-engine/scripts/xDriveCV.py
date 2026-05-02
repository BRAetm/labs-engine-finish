"""xDriveCV — meter-on auto-shot for Labs Engine.

Sibling to xDrive3K. Wraps the BGR shot-meter detector at
    %APPDATA%/Labs/Labs Engine/cv-scripts/shot_meter.py

Pipeline:
    Labs Engine SHM (decoded BGRA) -> ShotMeterDetector.process(frame)
    detector velocity-predicts the green release window
    -> on_release callback -> labs_input_bridge.set_rs_xy(0, +32767, 200)
    that's the shot release.

Self-calibrates over the first 4 shots (pulls real meter pixel BGR ranges
from your specific game / display). After that the trigger %s do the work.
"""

import os
import sys
import time
import json
import ctypes
import ctypes.wintypes
import threading
from pathlib import Path

from PySide6 import QtCore, QtGui, QtWidgets

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
# cv-scripts/ sits one level up from scripts/ — add it so we can import
# the shot_meter module that does the actual detection.
CV_SCRIPTS = ROOT.parent / "cv-scripts"
if CV_SCRIPTS.exists():
    sys.path.insert(0, str(CV_SCRIPTS))

import xui                            # noqa: E402   shared widget kit
import labs_input_bridge as labs      # noqa: E402   SHM bridge to LabsCore

try:
    from shot_meter import ShotMeterDetector  # noqa: E402
    SHOT_METER_OK = True
    SHOT_METER_ERR = ""
except Exception as _ex:                       # noqa: BLE001
    SHOT_METER_OK = False
    SHOT_METER_ERR = str(_ex)

import numpy as np                              # noqa: E402

SETTINGS_FILE = ROOT / "xdrivecv_settings.json"

DEFAULTS = {
    "discord_id":       "",
    "cv_enabled":       False,
    "trigger_normal":   95,    # release at this fill % for normal shots
    "trigger_l2":       75,    # release at this fill % when L2 is held
    "release_hold_ms":  220,   # how long to drive RS UP on each release
}


# ── Discord ID gate (mirrors xDrive3K) ────────────────────────────────────
_LOCAL_KEYS: dict[str, str] = {
    "0000": "LIFETIME",
    "0001": "WEEKLY",
    "0002": "MONTHLY",
}
TIER_GRADIENTS = {
    "LIFETIME": [(0.0, "#FFD86B"), (1.0, "#FF6B35")],
    "MONTHLY":  [(0.0, "#C084FC"), (1.0, "#EC4899")],
    "WEEKLY":   [(0.0, "#67E8F9"), (1.0, "#3B82F6")],
}
LOCKED_GRADIENT = [(0.0, "#7E8AA6"), (1.0, "#7E8AA6")]


def validate(discord_id: str) -> tuple[bool, str]:
    did = (discord_id or "").strip()
    dur = _LOCAL_KEYS.get(did, "")
    return bool(dur), dur


def load_settings() -> dict:
    if SETTINGS_FILE.exists():
        try:
            d = json.loads(SETTINGS_FILE.read_text())
            return {**DEFAULTS, **d}
        except Exception:
            pass
    return dict(DEFAULTS)


def save_settings(d: dict) -> None:
    SETTINGS_FILE.write_text(json.dumps(d, indent=2))


# ── Labs Engine frame-SHM reader ──────────────────────────────────────────
# Reads decoded BGRA frames straight from CvPythonPlugin's shared memory
# block (Labs_<pid>_Frame_<sessionId>). Bypasses screen capture, immune to
# window occlusion or HW-acceleration quirks.
class _ShmReader:
    HEADER       = 64
    MAX_PAYLOAD  = 1920 * 1080 * 4
    BLOCK_SIZE   = HEADER + MAX_PAYLOAD
    MAGIC        = 0x4D52464C
    FORMAT_BGRA  = 0

    def __init__(self):
        self._k32 = ctypes.WinDLL("kernel32", use_last_error=True)
        self._k32.OpenFileMappingW.restype  = ctypes.wintypes.HANDLE
        self._k32.OpenFileMappingW.argtypes = [
            ctypes.wintypes.DWORD, ctypes.wintypes.BOOL, ctypes.wintypes.LPCWSTR]
        self._k32.MapViewOfFile.restype  = ctypes.c_void_p
        self._k32.MapViewOfFile.argtypes = [
            ctypes.wintypes.HANDLE, ctypes.wintypes.DWORD,
            ctypes.wintypes.DWORD,  ctypes.wintypes.DWORD, ctypes.c_size_t]
        self._k32.UnmapViewOfFile.argtypes = [ctypes.c_void_p]
        self._k32.CloseHandle.argtypes = [ctypes.wintypes.HANDLE]
        self._handle = None
        self._addr = None
        self._last_seq = 0

    def _find_labs_pid(self):
        try:
            import psutil
            for p in psutil.process_iter(["name", "pid"]):
                if (p.info.get("name") or "").lower() == "labsengine.exe":
                    return int(p.info["pid"])
        except Exception:
            pass
        return None

    def open(self) -> bool:
        FILE_MAP_READ = 0x0004
        pid = self._find_labs_pid()
        if pid is None:
            return False
        for sid in (1, 0, 2):
            name = f"Labs_{pid}_Frame_{sid}"
            h = self._k32.OpenFileMappingW(FILE_MAP_READ, False, name)
            if h:
                addr = self._k32.MapViewOfFile(
                    h, FILE_MAP_READ, 0, 0, self.BLOCK_SIZE)
                if addr:
                    self._handle = h
                    self._addr = addr
                    return True
                self._k32.CloseHandle(h)
        return False

    def read(self):
        if not self._addr:
            return None
        try:
            buf = (ctypes.c_uint8 * self.BLOCK_SIZE).from_address(self._addr)
            magic = int.from_bytes(bytes(buf[0:4]), "little")
            if magic != self.MAGIC:
                return None
            seq = int.from_bytes(bytes(buf[12:16]), "little")
            if seq == 0 or seq == self._last_seq:
                return None
            payload_sz = int.from_bytes(bytes(buf[16:20]), "little")
            width      = int.from_bytes(bytes(buf[20:24]), "little")
            height     = int.from_bytes(bytes(buf[24:28]), "little")
            stride     = int.from_bytes(bytes(buf[28:32]), "little")
            fmt        = int.from_bytes(bytes(buf[32:36]), "little")
            if width <= 0 or height <= 0 or stride <= 0:
                return None
            if payload_sz <= 0 or payload_sz > self.MAX_PAYLOAD:
                return None
            self._last_seq = seq
            if fmt != self.FORMAT_BGRA:
                return None
            view = np.frombuffer(buf, dtype=np.uint8)
            arr  = view[self.HEADER:self.HEADER + payload_sz] \
                    .reshape((height, stride // 4, 4))[:, :width, :3]
            return np.ascontiguousarray(arr)
        except Exception:
            return None

    def close(self):
        try:
            if self._addr:    self._k32.UnmapViewOfFile(self._addr)
            if self._handle: self._k32.CloseHandle(self._handle)
        except Exception:
            pass
        self._addr = None
        self._handle = None


# ── XInput direct read (for L2-held detection) ────────────────────────────
class _XI_GP(ctypes.Structure):
    _fields_ = [("wButtons", ctypes.wintypes.WORD),
                ("bLeftTrigger", ctypes.wintypes.BYTE),
                ("bRightTrigger", ctypes.wintypes.BYTE),
                ("sThumbLX", ctypes.c_short), ("sThumbLY", ctypes.c_short),
                ("sThumbRX", ctypes.c_short), ("sThumbRY", ctypes.c_short)]


class _XI_ST(ctypes.Structure):
    _fields_ = [("dwPacketNumber", ctypes.wintypes.DWORD), ("Gamepad", _XI_GP)]


_xinput = None
for _name in ("xinput1_4", "xinput1_3", "xinput9_1_0"):
    try:
        _xinput = ctypes.WinDLL(_name)
        break
    except OSError:
        pass


def read_l2(slot: int) -> int:
    """Return the LT (L2) byte for the given XInput slot, 0 if disconnected."""
    if _xinput is None:
        return 0
    st = _XI_ST()
    if _xinput.XInputGetState(slot, ctypes.byref(st)) == 0:
        return int(st.Gamepad.bLeftTrigger) & 0xFF
    return 0


# ── CV worker thread ──────────────────────────────────────────────────────
class CVWorker(QtCore.QThread):
    """Pulls SHM frames, hands them to the ShotMeterDetector. Detector
       fires its on_release callback when the meter velocity-predicts the
       green window — we intercept that and emit the RS-up release flick."""
    status     = QtCore.Signal(str)
    fired      = QtCore.Signal(int, float, str)   # release_count, fill, shot_type

    def __init__(self, settings_ref: dict):
        super().__init__()
        self._stop = False
        self._settings = settings_ref
        self._detector: ShotMeterDetector | None = None
        self._shm = _ShmReader()

    def stop(self):
        self._stop = True

    def _on_release(self, fill: float, l2: bool, shot_type: str):
        """Fire the actual shot — RS UP for a configured hold so the game
           registers the release. The detector handles its own refractory
           lockout so we don't double-fire."""
        hold_ms = int(self._settings.get("release_hold_ms", 220))
        hold_ms = max(60, min(500, hold_ms))
        try:
            labs.set_rs_xy(0, +32767, hold_ms)
        except Exception as ex:
            print(f"[xDriveCV] release write failed: {ex}", flush=True)
        self.fired.emit(self._detector.release_count if self._detector
                        else 0, float(fill), str(shot_type))

    def run(self):
        if not SHOT_METER_OK:
            self.status.emit(f"shot_meter import failed: {SHOT_METER_ERR}")
            return
        if not self._shm.open():
            self.status.emit("Labs Engine SHM not found — start a stream first")
            # keep retrying every 2s until SHM appears
            while not self._stop:
                self.msleep(2000)
                if self._shm.open():
                    self.status.emit("SHM connected")
                    break
            if self._stop:
                return

        # Build the detector with settings from disk.
        self._detector = ShotMeterDetector(
            trigger_pct=int(self._settings.get("trigger_normal", 95)),
            trigger_pct_l2=int(self._settings.get("trigger_l2", 75)),
        )
        self._detector.set_release_callback(self._on_release)
        self.status.emit(
            f"detector ready (normal {self._detector.trigger_percentage}% / "
            f"l2 {self._detector.trigger_percentage_l2}%) — waiting for meter")

        last_status = ""
        last_frame_at = 0.0
        while not self._stop:
            # Hot-update trigger %s if the user changed them in the UI.
            tn = int(self._settings.get("trigger_normal", 95))
            tl = int(self._settings.get("trigger_l2", 75))
            if tn != self._detector.trigger_percentage:
                self._detector.trigger_percentage = tn
            if tl != self._detector.trigger_percentage_l2:
                self._detector.trigger_percentage_l2 = tl

            if not self._settings.get("cv_enabled", False):
                if last_status != "paused":
                    self.status.emit("paused (toggle Auto Shoot on)")
                    last_status = "paused"
                self.msleep(120)
                continue

            frame = self._shm.read()
            now = time.perf_counter()
            if frame is None:
                if now - last_frame_at > 2.0 and last_status != "shm_dry":
                    self.status.emit("SHM connected — no frames yet (start a stream)")
                    last_status = "shm_dry"
                self.msleep(8)
                continue
            last_frame_at = now

            # Read L2 from any connected slot (skip 0 = ViGEm).
            l2 = False
            for slot in (1, 2, 3):
                lt = read_l2(slot)
                if lt > 128:
                    l2 = True
                    break

            # Run the detector. Returns the fill % (0..1) or None.
            try:
                fill = self._detector.process(frame, l2=l2)
            except Exception as ex:
                self.status.emit(f"detector error: {ex}")
                self.msleep(50)
                continue

            # Status updates while armed. Throttled to changes only so we
            # don't spam the UI.
            if fill is not None:
                pct = int(fill * 100)
                trigger = (self._detector.trigger_percentage_l2 if l2
                           else self._detector.trigger_percentage)
                tag = "L2" if l2 else "NORM"
                msg = f"{tag} fill={pct}% trig={trigger}% shots={self._detector.release_count}"
                if msg != last_status:
                    self.status.emit(msg)
                    last_status = msg
            else:
                if last_status != "no_meter" and now - last_frame_at < 0.5:
                    # Don't spam — only log no-meter once per dry spell.
                    if not last_status.startswith("no_meter"):
                        self.status.emit("no meter on screen")
                    last_status = "no_meter"

            self.msleep(2)

        try: self._shm.close()
        except Exception: pass


# ── stepper widget (matches xDrive3K's) ───────────────────────────────────
class StepperRow(QtWidgets.QWidget):
    valueChanged = QtCore.Signal(float)

    def __init__(self, label: str, lo: float, hi: float,
                 step: float, decimals: int = 0, unit: str = ""):
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
        self._minus = QtWidgets.QPushButton("−")
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

    def _refresh(self):
        self._val_lbl.setText(f"{self._val:.{self._dec}f}")
        self._minus.setEnabled(self._val > self._lo + 1e-9)
        self._plus.setEnabled(self._val < self._hi - 1e-9)


# ── main window ───────────────────────────────────────────────────────────
class MainWindow(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("xDrive CV")
        self.setWindowFlags(QtCore.Qt.WindowType.FramelessWindowHint)
        self.setAttribute(QtCore.Qt.WidgetAttribute.WA_TranslucentBackground, True)
        self.setStyleSheet(xui.STYLESHEET + "QMainWindow { background: transparent; }")
        self.resize(880, 580)
        self.setMinimumSize(800, 480)

        self.s = load_settings()
        self._build_ui()
        self._apply_lock()
        self._position_ctrls()

        self.worker = CVWorker(self.s)
        self.worker.status.connect(self._on_status)
        self.worker.fired.connect(self._on_fired)
        self.worker.start()

    def _build_ui(self):
        root = xui.RoundedRoot(radius=14, color=xui.BG)
        v = QtWidgets.QVBoxLayout(root); v.setContentsMargins(0, 0, 0, 0); v.setSpacing(0)

        self._ctrl_bar = QtWidgets.QWidget(self)
        cb = QtWidgets.QHBoxLayout(self._ctrl_bar)
        cb.setContentsMargins(0, 0, 0, 0); cb.setSpacing(0)
        bmin = xui.WinBtn("min");   bmin.clicked.connect(self.showMinimized)
        bcls = xui.WinBtn("close"); bcls.clicked.connect(self.close)
        cb.addWidget(bmin); cb.addWidget(bcls)

        v.addWidget(xui.HeroBanner(height=180))
        v.addWidget(self._build_topbar())
        v.addWidget(xui._divider())

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

        lbl = QtWidgets.QLabel("DISCORD ID")
        f_lbl = xui.body_font(10, 700)
        f_lbl.setCapitalization(QtGui.QFont.Capitalization.AllUppercase)
        f_lbl.setLetterSpacing(QtGui.QFont.SpacingType.AbsoluteSpacing, 2.6)
        lbl.setFont(f_lbl)
        lbl.setStyleSheet(f"color: {xui.DIM};")
        h.addWidget(lbl)

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
            f"QLineEdit {{ background: {xui.BG}; color: {xui.TEXT};"
            f"  border: 1px solid {xui.BORDER}; border-radius: 8px;"
            f"  padding: 0 14px;"
            f"  selection-background-color: {xui.ACCENT}; }}"
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
        page = QtWidgets.QWidget()
        v = QtWidgets.QVBoxLayout(page)
        v.setContentsMargins(20, 16, 20, 16); v.setSpacing(14)

        # Auto-shoot card.
        card = xui.GridCard()
        cv = QtWidgets.QVBoxLayout(card)
        cv.setContentsMargins(20, 16, 20, 16); cv.setSpacing(12)
        cv.addWidget(xui.SectionLabel("AUTO SHOT (BGR METER)"))

        self.cv_toggle = xui.StateToggle("Auto Shoot")
        self.cv_toggle.setChecked(self.s.get("cv_enabled", False))
        self.cv_toggle.toggled.connect(lambda v: self._set("cv_enabled", v))
        cv.addWidget(self.cv_toggle)

        # Trigger %s — fire-when-fill-reaches values for normal vs L2 shots.
        self.tn = StepperRow("Normal Trigger", 50, 100, 1, 0, "%")
        self.tn.setValue(int(self.s.get("trigger_normal", 95)))
        self.tn.valueChanged.connect(lambda v: self._set("trigger_normal", int(v)))
        cv.addWidget(self.tn)

        self.tl = StepperRow("L2 Trigger", 30, 100, 1, 0, "%")
        self.tl.setValue(int(self.s.get("trigger_l2", 75)))
        self.tl.valueChanged.connect(lambda v: self._set("trigger_l2", int(v)))
        cv.addWidget(self.tl)

        self.rh = StepperRow("Release Hold", 60, 500, 10, 0, "ms")
        self.rh.setValue(int(self.s.get("release_hold_ms", 220)))
        self.rh.valueChanged.connect(lambda v: self._set("release_hold_ms", int(v)))
        cv.addWidget(self.rh)

        # Recalibrate button — clears the detector's calibration so it
        # re-samples meter pixels on the next 4 shots. Useful after
        # changing 2K's color skin / display.
        recal_row = QtWidgets.QHBoxLayout(); recal_row.setSpacing(8)
        recal_btn = QtWidgets.QPushButton("RECALIBRATE")
        recal_btn.setStyleSheet(
            f"QPushButton {{ background: {xui.SURFACE}; color: {xui.TEXT};"
            f"  border: 1px solid {xui.BORDER}; border-radius: 6px;"
            f"  padding: 6px 14px; font-weight: 700; }}"
            f"QPushButton:hover {{ border-color: {xui.ACCENT}; }}")
        recal_btn.clicked.connect(self._on_recalibrate)
        recal_row.addWidget(recal_btn)
        recal_row.addStretch(1)
        cv.addLayout(recal_row)
        v.addWidget(card)

        # Status card.
        st_card = xui.GridCard()
        sv = QtWidgets.QVBoxLayout(st_card)
        sv.setContentsMargins(20, 14, 20, 14); sv.setSpacing(8)
        sv.addWidget(xui.SectionLabel("STATUS"))
        self.status_lbl = QtWidgets.QLabel("starting…")
        self.status_lbl.setFont(xui.mono_font(11, 600))
        self.status_lbl.setStyleSheet(f"color: {xui.TEXT};")
        sv.addWidget(self.status_lbl)
        self.shots_lbl = QtWidgets.QLabel("0 shots fired")
        self.shots_lbl.setFont(xui.body_label_font(10))
        self.shots_lbl.setStyleSheet(f"color: {xui.DIM};")
        sv.addWidget(self.shots_lbl)
        v.addWidget(st_card)

        # How-to.
        hint = QtWidgets.QLabel(
            "Auto Shoot watches the 2K shot meter on the Labs Engine "
            "stream. The first 4 shots auto-calibrate the meter's color "
            "and peak height for your specific game / display. After "
            "that, the detector velocity-predicts the green release "
            "window and fires the RS-up release flick automatically. "
            "Hold L2 to use the L2 trigger %; let go for the normal one."
        )
        hint.setFont(xui.ui_font(9))
        hint.setStyleSheet(f"color: {xui.DIM};")
        hint.setWordWrap(True)
        v.addWidget(hint)

        v.addStretch(1)
        xui.apply_body_font_recursive(page)
        sa = QtWidgets.QScrollArea()
        sa.setWidget(page); sa.setWidgetResizable(True)
        sa.setFrameShape(QtWidgets.QFrame.Shape.NoFrame)
        sa.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        sa.setStyleSheet(f"QScrollArea {{ background: {xui.BG}; border: none; }}")
        return sa

    def _set(self, key: str, value):
        self.s[key] = value
        save_settings(self.s)

    def _on_recalibrate(self):
        if hasattr(self, "worker") and self.worker._detector is not None:
            try:
                self.worker._detector.recalibrate("both")
                self.log("calibration cleared — next 4 shots will recalibrate")
            except Exception as ex:
                self.log(f"recalibrate failed: {ex}")

    def _on_status(self, s: str):
        if hasattr(self, "status_lbl"):
            self.status_lbl.setText(s)

    def _on_fired(self, count: int, fill: float, shot_type: str):
        if hasattr(self, "shots_lbl"):
            self.shots_lbl.setText(
                f"{count} shots fired  ·  last {int(fill*100)}%  ·  {shot_type}")
        self.log(f"FIRED #{count} @ {int(fill*100)}% ({shot_type})")

    def log(self, msg: str):
        ts = time.strftime("%H:%M:%S")
        print(f"[xDriveCV {ts}] {msg}", flush=True)

    def closeEvent(self, e):
        try:
            if hasattr(self, "worker"):
                self.worker.stop()
                self.worker.wait(500)
        except Exception:
            pass
        super().closeEvent(e)


# ── entry point ───────────────────────────────────────────────────────────
def main():
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
        sys.stderr.reconfigure(encoding="utf-8", errors="replace")
    except Exception:
        pass

    app = QtWidgets.QApplication(sys.argv)
    fonts_dir = ROOT / "userdata" / "fonts"
    if fonts_dir.exists():
        for ttf in fonts_dir.glob("*.ttf"):
            fid = QtGui.QFontDatabase.addApplicationFont(str(ttf))
            if fid >= 0:
                fams = QtGui.QFontDatabase.applicationFontFamilies(fid)
                print(f"[xDriveCV] registered font: {ttf.name} -> {fams}", flush=True)
    app.setStyleSheet(xui.STYLESHEET)
    win = MainWindow()
    win.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
