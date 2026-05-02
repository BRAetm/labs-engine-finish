"""xdrive3k_cv.py — CV addon for xDrive3K.

Two pieces:

  * ``CalibrateDialog`` — modal picker. Connects to the Labs Engine SHM frame
    stream, lets the user freeze a frame, drag a rectangle around the shot
    meter, and refine the BGR threshold band by dragging tolerance. Returns
    ``{"roi", "bgr_lo", "bgr_hi"}`` on accept.

  * ``CVDetector`` — ``QThread`` that consumes the same SHM frame stream and
    emits ``fillChanged(float)`` plus ``thresholdCrossed()`` (rising-edge,
    refractory-gated). Runs ``cv2.inRange`` + ``cv2.countNonZero`` on the
    user-calibrated ROI — no contour finding, no auto-cal, no peak history.

The detector is fed a settings snapshot via ``apply_settings()``; the run loop
swaps the ref atomically per tick so the GUI thread can mutate freely.
"""
from __future__ import annotations

import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

import cv2
import numpy as np
from PySide6 import QtCore, QtGui, QtWidgets

import labs_frame_bridge as _frame_io
import xui


# ── snapshot ────────────────────────────────────────────────────────────────
@dataclass(frozen=True)
class CVSnapshot:
    enabled: bool = False
    roi: tuple[int, int, int, int] = (0, 0, 0, 0)   # x, y, w, h
    bgr_lo: tuple[int, int, int] = (0, 0, 0)
    bgr_hi: tuple[int, int, int] = (255, 255, 255)
    fire_at_pct: float = 95.0

    @classmethod
    def from_dict(cls, d: dict) -> "CVSnapshot":
        cv = d.get("cv", {}) or {}
        roi_d = cv.get("roi", {}) or {}
        return cls(
            enabled=bool(cv.get("enabled", False)),
            roi=(int(roi_d.get("x", 0)), int(roi_d.get("y", 0)),
                 int(roi_d.get("w", 0)), int(roi_d.get("h", 0))),
            bgr_lo=tuple(int(v) for v in cv.get("bgr_lo", [0, 0, 0])),
            bgr_hi=tuple(int(v) for v in cv.get("bgr_hi", [255, 255, 255])),
            fire_at_pct=float(cv.get("fire_at_pct", 95)),
        )

    @property
    def calibrated(self) -> bool:
        return self.roi[2] > 4 and self.roi[3] > 4


# ── detector thread ─────────────────────────────────────────────────────────
class CVDetector(QtCore.QThread):
    """Reads frames from SHM, emits fill % and a rising-edge fire signal.

    Signals are connected with ``Qt.QueuedConnection`` from the main window so
    the slot runs on the GUI thread — keeps cv2 + SHM I/O off the GUI loop.
    """
    fillChanged       = QtCore.Signal(float)
    thresholdCrossed  = QtCore.Signal()
    statusChanged     = QtCore.Signal(str)   # human-readable status string

    REFRACTORY_S = 0.45  # min gap between auto-fires
    LOG_EVERY_N  = 30    # rate-limit fillChanged emits to ~2Hz at 60Hz capture

    def __init__(self, labs_pid: int, session_id: int, parent=None):
        super().__init__(parent)
        self._labs_pid = int(labs_pid)
        self._session  = int(session_id)
        self._reader: Optional[_frame_io.ShmFrameReader] = None
        self._snap: CVSnapshot = CVSnapshot()
        self._stop = False
        self._was_above = False
        self._last_fire_t = 0.0
        self._tick = 0

    # called from GUI thread; reassignment of a single attribute is atomic in
    # CPython so the run loop's `s = self._snap` always sees a consistent obj.
    def apply_settings(self, snap: CVSnapshot):
        self._snap = snap
        if not snap.enabled or not snap.calibrated:
            self._was_above = False

    def stop(self):
        self._stop = True

    def run(self):
        try:
            self._reader = _frame_io.ShmFrameReader(self._labs_pid, self._session)
        except Exception as ex:
            self.statusChanged.emit(f"SHM open failed: {ex}")
            return

        self.statusChanged.emit("connected")
        lo_arr = np.zeros(3, dtype=np.uint8)
        hi_arr = np.full(3, 255, dtype=np.uint8)

        while not self._stop:
            s = self._snap
            if not s.enabled or not s.calibrated:
                # Idle. Don't poll the SHM event so the calibrate dialog (which
                # opens its own ShmFrameReader on the same named event) doesn't
                # race with us for frames.
                time.sleep(0.05)
                continue

            frame = self._reader.get_latest_frame(timeout_ms=200)
            if frame is None:
                continue

            x, y, w, h = s.roi
            fh, fw = frame.shape[:2]
            # clamp ROI into frame; capture resolution can change between
            # calibration and runtime (HiDPI rescale, window-mode swap).
            x2 = min(fw, x + w); y2 = min(fh, y + h)
            x  = max(0, min(x, fw - 1))
            y  = max(0, min(y, fh - 1))
            if x2 - x < 4 or y2 - y < 4:
                continue
            roi = frame[y:y2, x:x2]

            np.copyto(lo_arr, s.bgr_lo)
            np.copyto(hi_arr, s.bgr_hi)
            mask = cv2.inRange(roi, lo_arr, hi_arr)
            total = mask.size
            fill = (cv2.countNonZero(mask) / total) * 100.0 if total else 0.0

            self._tick += 1
            if self._tick % self.LOG_EVERY_N == 0:
                self.fillChanged.emit(fill)

            now = time.perf_counter()
            above = fill >= s.fire_at_pct
            if above and not self._was_above and (now - self._last_fire_t) >= self.REFRACTORY_S:
                self._last_fire_t = now
                self.thresholdCrossed.emit()
            self._was_above = above

        try:
            if self._reader: self._reader.close()
        except Exception: pass
        self.statusChanged.emit("stopped")


# ── live preview widget for the picker ──────────────────────────────────────
class _PickerCanvas(QtWidgets.QLabel):
    """Shows the (live or frozen) frame and a draggable ROI rectangle.

    Coordinates are stored in *frame* pixels, not widget pixels. Painting and
    hit-testing translate through the displayed pixmap's geometry.
    """
    roiChanged = QtCore.Signal(QtCore.QRect)  # frame-coord rect

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumSize(640, 360)
        self.setAlignment(QtCore.Qt.AlignmentFlag.AlignCenter)
        self.setStyleSheet(f"background: {xui.BG}; border: 1px solid {xui.BORDER}; border-radius: 6px;")
        self._frame: Optional[np.ndarray] = None
        self._mask_overlay: Optional[np.ndarray] = None
        self._frame_size = QtCore.QSize(0, 0)
        self._roi = QtCore.QRect()
        self._drag_start: Optional[QtCore.QPoint] = None
        self._drag_cur:   Optional[QtCore.QPoint] = None
        self.setMouseTracking(True)

    def set_frame(self, frame: Optional[np.ndarray]):
        self._frame = frame
        if frame is None:
            self._frame_size = QtCore.QSize(0, 0)
            self.setPixmap(QtGui.QPixmap())
            return
        h, w = frame.shape[:2]
        self._frame_size = QtCore.QSize(w, h)
        self._refresh_pixmap()

    def set_mask_overlay(self, mask: Optional[np.ndarray]):
        self._mask_overlay = mask
        self._refresh_pixmap()

    def set_roi(self, roi: QtCore.QRect):
        self._roi = QtCore.QRect(roi)
        self._refresh_pixmap()

    def roi(self) -> QtCore.QRect:
        return QtCore.QRect(self._roi)

    def _refresh_pixmap(self):
        if self._frame is None: return
        disp = self._frame
        if self._mask_overlay is not None and self._mask_overlay.shape[:2] == disp.shape[:2]:
            tint = np.zeros_like(disp); tint[:, :, 1] = 255   # green
            m3 = cv2.cvtColor(self._mask_overlay, cv2.COLOR_GRAY2BGR)
            disp = np.where(m3 > 0, cv2.addWeighted(disp, 0.4, tint, 0.6, 0), disp)
        h, w = disp.shape[:2]
        rgb = cv2.cvtColor(disp, cv2.COLOR_BGR2RGB)
        qimg = QtGui.QImage(rgb.data, w, h, rgb.strides[0], QtGui.QImage.Format.Format_RGB888).copy()
        pix = QtGui.QPixmap.fromImage(qimg)
        # Fit the pixmap inside the label preserving aspect; we'll draw the
        # ROI overlay in paintEvent on top of this pixmap.
        scaled = pix.scaled(self.size(), QtCore.Qt.AspectRatioMode.KeepAspectRatio,
                            QtCore.Qt.TransformationMode.SmoothTransformation)
        self.setPixmap(scaled)
        self.update()

    # widget→frame and frame→widget coordinate maps based on pixmap geometry
    def _widget_to_frame(self, p: QtCore.QPoint) -> QtCore.QPoint:
        pm = self.pixmap()
        if pm.isNull() or self._frame_size.isEmpty():
            return QtCore.QPoint(0, 0)
        # pixmap is centered inside the label
        ox = (self.width()  - pm.width())  // 2
        oy = (self.height() - pm.height()) // 2
        rx = max(0, min(pm.width()  - 1, p.x() - ox))
        ry = max(0, min(pm.height() - 1, p.y() - oy))
        sx = self._frame_size.width()  / pm.width()
        sy = self._frame_size.height() / pm.height()
        return QtCore.QPoint(int(round(rx * sx)), int(round(ry * sy)))

    def _frame_rect_to_widget(self, r: QtCore.QRect) -> QtCore.QRect:
        pm = self.pixmap()
        if pm.isNull() or self._frame_size.isEmpty():
            return QtCore.QRect()
        ox = (self.width()  - pm.width())  // 2
        oy = (self.height() - pm.height()) // 2
        sx = pm.width()  / self._frame_size.width()
        sy = pm.height() / self._frame_size.height()
        return QtCore.QRect(int(r.x() * sx) + ox, int(r.y() * sy) + oy,
                            max(1, int(r.width()  * sx)),
                            max(1, int(r.height() * sy)))

    def mousePressEvent(self, e: QtGui.QMouseEvent):
        if e.button() != QtCore.Qt.MouseButton.LeftButton or self._frame is None:
            return
        self._drag_start = self._widget_to_frame(e.position().toPoint())
        self._drag_cur = self._drag_start
        self.update()

    def mouseMoveEvent(self, e: QtGui.QMouseEvent):
        if self._drag_start is None: return
        self._drag_cur = self._widget_to_frame(e.position().toPoint())
        self.update()

    def mouseReleaseEvent(self, e: QtGui.QMouseEvent):
        if self._drag_start is None or self._drag_cur is None:
            return
        a, b = self._drag_start, self._drag_cur
        self._drag_start = self._drag_cur = None
        rect = QtCore.QRect(QtCore.QPoint(min(a.x(), b.x()), min(a.y(), b.y())),
                            QtCore.QPoint(max(a.x(), b.x()), max(a.y(), b.y())))
        if rect.width() < 6 or rect.height() < 6:
            self.update(); return
        self._roi = rect
        self.roiChanged.emit(rect)
        self.update()

    def resizeEvent(self, e):
        super().resizeEvent(e)
        self._refresh_pixmap()

    def paintEvent(self, e):
        super().paintEvent(e)
        if self._frame is None: return
        p = QtGui.QPainter(self); p.setRenderHint(QtGui.QPainter.RenderHint.Antialiasing)

        # active ROI
        if self._roi.width() > 0:
            wr = self._frame_rect_to_widget(self._roi)
            pen = QtGui.QPen(QtGui.QColor(xui.ACCENT)); pen.setWidth(2)
            p.setPen(pen); p.drawRect(wr)

        # in-progress drag rect
        if self._drag_start and self._drag_cur:
            a, b = self._drag_start, self._drag_cur
            r = QtCore.QRect(QtCore.QPoint(min(a.x(), b.x()), min(a.y(), b.y())),
                             QtCore.QPoint(max(a.x(), b.x()), max(a.y(), b.y())))
            wr = self._frame_rect_to_widget(r)
            pen = QtGui.QPen(QtGui.QColor(xui.OK)); pen.setWidth(1); pen.setStyle(QtCore.Qt.PenStyle.DashLine)
            p.setPen(pen); p.drawRect(wr)


# ── calibrate dialog ────────────────────────────────────────────────────────
class CalibrateDialog(QtWidgets.QDialog):
    """Two-stage calibrator:

      1. LIVE — frame stream from SHM updates the canvas at ~30Hz. User clicks
         FREEZE to capture a still.
      2. SAMPLE — user drags a rectangle around the meter on the still. The
         dialog auto-derives a BGR low/high band from the saturated pixels
         inside the rectangle. A tolerance slider widens/tightens the band;
         the green mask overlay shows what would match.

      Accept → returns ``{"roi": (x,y,w,h), "bgr_lo": [..], "bgr_hi": [..]}``.
    """
    def __init__(self, labs_pid: int, session_id: int, parent=None):
        super().__init__(parent)
        self.setWindowTitle("xDrive 3K — Meter Calibrate")
        self.resize(960, 720)
        self.setStyleSheet(xui.STYLESHEET + f"QDialog {{ background: {xui.BG}; }}")

        self._labs_pid = labs_pid
        self._session  = session_id
        self._reader: Optional[_frame_io.ShmFrameReader] = None
        self._frame:  Optional[np.ndarray] = None       # most recent live frame
        self._frozen: Optional[np.ndarray] = None       # still after FREEZE
        self._sample_bgr: Optional[tuple[int, int, int]] = None
        self._tolerance = 25
        self._result: Optional[dict] = None

        self._build_ui()

        self._timer = QtCore.QTimer(self); self._timer.setInterval(33)
        self._timer.timeout.connect(self._tick_live)
        QtCore.QTimer.singleShot(0, self._open_reader)

    # ── UI ──────────────────────────────────────────────────────────────────
    def _build_ui(self):
        v = QtWidgets.QVBoxLayout(self); v.setContentsMargins(16, 16, 16, 16); v.setSpacing(12)

        title = QtWidgets.QLabel("METER CALIBRATE")
        title.setFont(xui.display_font(18, 800)); title.setStyleSheet(f"color: {xui.TEXT};")
        v.addWidget(title)

        self._status = QtWidgets.QLabel("Connecting to capture…")
        self._status.setFont(xui.ui_font(9)); self._status.setStyleSheet(f"color: {xui.DIM};")
        v.addWidget(self._status)

        self._canvas = _PickerCanvas()
        self._canvas.roiChanged.connect(self._on_roi)
        v.addWidget(self._canvas, 1)

        # tolerance + actions row
        ctrl = QtWidgets.QHBoxLayout(); ctrl.setSpacing(12)
        self._tol_slider = xui.SliderRow("BGR Tolerance", 5, 80, decimals=0, unit="±", step=1)
        self._tol_slider.setValue(self._tolerance)
        self._tol_slider.valueChanged.connect(self._on_tol)
        ctrl.addWidget(self._tol_slider, 1)
        v.addLayout(ctrl)

        # mask preview readout
        self._readout = QtWidgets.QLabel("Drag a rectangle on the meter to sample.")
        self._readout.setFont(xui.mono_font(10, 600)); self._readout.setStyleSheet(f"color: {xui.DIM};")
        v.addWidget(self._readout)

        # buttons
        btns = QtWidgets.QHBoxLayout(); btns.setSpacing(8)
        self._btn_freeze = QtWidgets.QPushButton("FREEZE FRAME")
        self._btn_freeze.setProperty("accent", "true")
        self._btn_freeze.clicked.connect(self._on_freeze)
        btns.addWidget(self._btn_freeze)

        self._btn_resume = QtWidgets.QPushButton("RESUME LIVE")
        self._btn_resume.clicked.connect(self._on_resume)
        self._btn_resume.setEnabled(False)
        btns.addWidget(self._btn_resume)

        btns.addStretch(1)

        self._btn_cancel = QtWidgets.QPushButton("CANCEL")
        self._btn_cancel.clicked.connect(self.reject)
        btns.addWidget(self._btn_cancel)

        self._btn_save = QtWidgets.QPushButton("SAVE")
        self._btn_save.setProperty("accent", "true")
        self._btn_save.clicked.connect(self._on_save)
        self._btn_save.setEnabled(False)
        btns.addWidget(self._btn_save)
        v.addLayout(btns)

        for b in (self._btn_freeze, self._btn_resume, self._btn_cancel, self._btn_save):
            b.setMinimumHeight(36)

    # ── reader lifecycle ───────────────────────────────────────────────────
    def _open_reader(self):
        try:
            self._reader = _frame_io.ShmFrameReader(self._labs_pid, self._session)
        except Exception as ex:
            self._status.setText(f"SHM open failed: {ex}")
            return
        self._status.setText(f"Live  ·  PID {self._labs_pid}  Session {self._session}")
        self._timer.start()

    def _tick_live(self):
        if not self._reader or self._frozen is not None: return
        f = self._reader.get_latest_frame(timeout_ms=10)
        if f is not None:
            self._frame = f
            self._canvas.set_frame(f)

    def _on_freeze(self):
        if self._frame is None:
            self._status.setText("No frame yet — is Labs Engine streaming?")
            return
        self._frozen = self._frame.copy()
        self._canvas.set_frame(self._frozen)
        self._timer.stop()
        self._btn_freeze.setEnabled(False)
        self._btn_resume.setEnabled(True)
        self._status.setText("Frozen — drag a rectangle around the shot meter.")

    def _on_resume(self):
        self._frozen = None
        self._canvas.set_mask_overlay(None)
        self._sample_bgr = None
        self._btn_freeze.setEnabled(True)
        self._btn_resume.setEnabled(False)
        self._btn_save.setEnabled(False)
        self._readout.setText("Drag a rectangle on the meter to sample.")
        self._timer.start()

    # ── sampling ───────────────────────────────────────────────────────────
    def _on_roi(self, rect: QtCore.QRect):
        if self._frozen is None: return
        x, y, w, h = rect.x(), rect.y(), rect.width(), rect.height()
        roi = self._frozen[y:y+h, x:x+w]
        if roi.size == 0:
            return
        # Use the median of pixels filtered to the saturated middle — drops
        # near-black UI background and near-white anti-alias rim. Median
        # beats mean for a noisy ROI that includes background pixels.
        flat = roi.reshape(-1, 3).astype(np.int32)
        bright = flat.sum(axis=1)
        keep = (bright > 60) & (bright < 720)
        sample_pool = flat[keep] if keep.any() else flat
        med = np.median(sample_pool, axis=0).astype(int)
        self._sample_bgr = (int(med[0]), int(med[1]), int(med[2]))
        self._refresh_mask()
        self._btn_save.setEnabled(True)

    def _on_tol(self, v: float):
        self._tolerance = int(v)
        self._refresh_mask()

    def _refresh_mask(self):
        if self._frozen is None or self._sample_bgr is None: return
        b, g, r = self._sample_bgr; t = self._tolerance
        lo = np.array([max(0, b - t), max(0, g - t), max(0, r - t)], np.uint8)
        hi = np.array([min(255, b + t), min(255, g + t), min(255, r + t)], np.uint8)
        # mask the entire frozen frame so the user can see false-positives
        # outside the ROI (UI elements that share the meter color).
        mask = cv2.inRange(self._frozen, lo, hi)
        # mask out everything outside the ROI rectangle so the visible green
        # tint matches what the runtime detector will actually see.
        rect = self._canvas.roi()
        scope = np.zeros(mask.shape, np.uint8)
        scope[rect.y():rect.y()+rect.height(), rect.x():rect.x()+rect.width()] = 1
        mask = mask * scope
        self._canvas.set_mask_overlay(mask)
        # readout
        roi_mask = mask[rect.y():rect.y()+rect.height(), rect.x():rect.x()+rect.width()]
        fill = (cv2.countNonZero(roi_mask) / max(1, roi_mask.size)) * 100.0
        self._readout.setText(
            f"BGR sample  B={b} G={g} R={r}   ±{t}   "
            f"ROI {rect.width()}×{rect.height()} px   fill={fill:5.1f}%"
        )

    # ── accept / cleanup ───────────────────────────────────────────────────
    def _on_save(self):
        if self._sample_bgr is None: return
        b, g, r = self._sample_bgr; t = self._tolerance
        rect = self._canvas.roi()
        self._result = {
            "roi": {"x": rect.x(), "y": rect.y(), "w": rect.width(), "h": rect.height()},
            "bgr_lo": [max(0, b - t), max(0, g - t), max(0, r - t)],
            "bgr_hi": [min(255, b + t), min(255, g + t), min(255, r + t)],
        }
        self.accept()

    def result_settings(self) -> Optional[dict]:
        return self._result

    def closeEvent(self, e):
        self._timer.stop()
        try:
            if self._reader: self._reader.close()
        except Exception: pass
        super().closeEvent(e)
