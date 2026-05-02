"""
xui — shared UI kit for Labs Engine sidecar scripts.

Lifted out of SecretK.py so other scripts (xDrive3K, future tools) can share
the same palette, fonts, stylesheet, and widget components without
copy/pasting 500 lines of widget code into every script.

Public surface:
  Colors:     BG SURFACE SURF2 BORDER BORD_HI TEXT DIM FAINT ACCENT ACCDIM OK DANGER WARN
  Fonts:      ui_font(size,weight) mono_font(size,weight) label_font(size) _fam(list)
  Stylesheet: STYLESHEET
  Widgets:    HeroBanner WinBtn TabBtn Card SectionLabel SliderRow ToggleRow
              ColumnHeader FieldLabel StepBtn BigStepSlider StateToggle
              StatLine StatBox EngBtn
  Helpers:    _Drag _hr _vsep _divider

Banner image: looks at LABS_SETTINGS_ROOT/assets/tm_labs_banner.png if the env
var is set, else <script-dir>/userdata/assets/tm_labs_banner.png. Missing
banner → HeroBanner just paints a clean SURFACE color.
"""

import os
from pathlib import Path

from PySide6 import QtCore, QtGui, QtWidgets


# ── banner path resolver ──────────────────────────────────────────────────────
# Resolves once at module load so both SecretK and xDrive3K see the same image.
def _resolve_banner() -> Path:
    env = os.environ.get("LABS_SETTINGS_ROOT")
    base = Path(env) if env else (Path(__file__).resolve().parent / "userdata")
    return base / "assets" / "tm_labs_banner.png"

BANNER = _resolve_banner()


# ── palette — deep neutrals + single confident accent ────────────────────────
BG      = "#06080F"
SURFACE = "#0C0F18"
SURF2   = "#11151F"
BORDER  = "#1A2030"
BORD_HI = "#2A3450"
TEXT    = "#F0F4FA"
DIM     = "#7E8AA6"
FAINT   = "#3A4360"
ACCENT  = "#22D3EE"   # bright cyan, matches modern cheat-menu reference
ACCDIM  = "#0E7490"
OK      = "#34D399"
DANGER  = "#EF4444"
WARN    = "#F59E0B"


# ── fonts ─────────────────────────────────────────────────────────────────────
def _fam(candidates: list[str], fallback="Segoe UI") -> str:
    db = set(QtGui.QFontDatabase.families())
    return next((c for c in candidates if c in db), fallback)


def ui_font(size=10, weight=400) -> QtGui.QFont:
    f = QtGui.QFont(_fam(["Inter", "Segoe UI Variable", "Segoe UI"]))
    f.setPointSize(size); f.setWeight(QtGui.QFont.Weight(weight))
    return f


def mono_font(size=11, weight=600) -> QtGui.QFont:
    f = QtGui.QFont(_fam(["JetBrains Mono", "Cascadia Mono", "Consolas"]))
    f.setPointSize(size); f.setWeight(QtGui.QFont.Weight(weight))
    try: f.setFeature(QtGui.QFont.Tag("tnum"), 1)
    except Exception: pass
    return f


def label_font(size=8) -> QtGui.QFont:
    f = ui_font(size, 600)
    f.setCapitalization(QtGui.QFont.Capitalization.AllUppercase)
    f.setLetterSpacing(QtGui.QFont.SpacingType.AbsoluteSpacing, 1.2)
    return f


def body_font(size=10, weight=500) -> QtGui.QFont:
    """Bottom-half UI font. Bumped to Russo One / Bebas Neue first so the
       bottom-of-menu text looks distinctly gaming-bold (matches the KEY
       chip family) instead of generic system sans."""
    f = QtGui.QFont(_fam([
        "Russo One", "Bebas Neue",
        "Chakra Petch", "Rajdhani",
        "Bahnschrift SemiCondensed", "Bahnschrift",
        "Segoe UI Variable", "Segoe UI",
    ]))
    f.setPointSize(size)
    f.setWeight(QtGui.QFont.Weight(weight))
    f.setLetterSpacing(QtGui.QFont.SpacingType.AbsoluteSpacing, 0.6)
    return f


def body_label_font(size=8) -> QtGui.QFont:
    """Caps variant for the small section/eyebrow labels inside the cards."""
    f = body_font(size, 700)
    f.setCapitalization(QtGui.QFont.Capitalization.AllUppercase)
    f.setLetterSpacing(QtGui.QFont.SpacingType.AbsoluteSpacing, 1.6)
    return f


def apply_body_font_recursive(widget: "QtWidgets.QWidget"):
    """Walk a widget tree and re-skin every QLabel / QLineEdit / QPushButton
       with body_font (size scaled from whatever the existing font is using).
       Skips display_font sites by leaving QLabels with point-size >= 16
       alone — those are headlines and should stay Anton/Bebas."""
    for kid in widget.findChildren(QtWidgets.QWidget):
        cur = kid.font()
        ps = cur.pointSize() if cur.pointSize() > 0 else 10
        if ps >= 16:
            continue
        if cur.capitalization() == QtGui.QFont.Capitalization.AllUppercase:
            kid.setFont(body_label_font(ps))
        else:
            kid.setFont(body_font(ps, cur.weight()))


def display_font(size=24, weight=900) -> QtGui.QFont:
    """Big condensed display type for HUD-style headers and jersey numbers.
       Falls back gracefully across user systems."""
    f = QtGui.QFont(_fam(["Anton", "Bebas Neue", "Russo One",
                          "Inter", "Segoe UI Variable Display", "Segoe UI"]))
    f.setPointSize(size)
    f.setWeight(QtGui.QFont.Weight(weight))
    f.setLetterSpacing(QtGui.QFont.SpacingType.AbsoluteSpacing, 1.5)
    return f


# ── global stylesheet ─────────────────────────────────────────────────────────
STYLESHEET = f"""
* {{ outline: none; }}

QMainWindow, QDialog {{ background: {BG}; color: {TEXT}; }}
QWidget {{ background: transparent; color: {TEXT}; font-family: "Segoe UI Variable Text","Segoe UI"; }}

QLabel {{ background: transparent; color: {TEXT}; }}

QScrollArea {{ background: {BG}; border: none; }}
QScrollArea > QWidget > QWidget {{ background: {BG}; }}
QScrollBar:vertical {{
    background: transparent; width: 8px; margin: 0;
}}
QScrollBar::handle:vertical {{
    background: {FAINT}; border-radius: 4px; min-height: 28px;
}}
QScrollBar::handle:vertical:hover {{ background: {BORD_HI}; }}
QScrollBar::add-line, QScrollBar::sub-line {{ height: 0; }}
QScrollBar::add-page, QScrollBar::sub-page {{ background: transparent; }}

QLineEdit {{
    background: {SURF2};
    color: {TEXT};
    border: 1px solid {BORDER};
    border-radius: 6px;
    padding: 6px 12px;
    font-size: 10pt;
    selection-background-color: {ACCENT};
}}
QLineEdit:hover {{ border-color: {BORD_HI}; }}
QLineEdit:focus {{ border-color: {ACCENT}; }}
QLineEdit::placeholder {{ color: {DIM}; }}

QSlider::groove:horizontal {{
    background: {FAINT}; height: 4px; border-radius: 2px;
}}
QSlider::sub-page:horizontal {{
    background: {ACCENT}; border-radius: 2px;
}}
QSlider::add-page:horizontal {{ background: {FAINT}; border-radius: 2px; }}
QSlider::handle:horizontal {{
    background: {TEXT};
    width: 14px; height: 14px;
    margin: -6px 0;
    border-radius: 7px;
    border: 2px solid {ACCENT};
}}
QSlider::handle:horizontal:hover {{ background: {ACCENT}; border-color: {TEXT}; }}

QCheckBox {{ spacing: 10px; color: {DIM}; }}
QCheckBox::indicator {{
    width: 18px; height: 18px;
    border: 1.5px solid {BORDER};
    border-radius: 4px;
    background: {SURF2};
}}
QCheckBox::indicator:hover {{ border-color: {ACCENT}; }}
QCheckBox::indicator:checked {{
    background: {ACCENT};
    border-color: {ACCENT};
    image: url(none);
}}

QPushButton {{
    background: {SURF2};
    color: {TEXT};
    border: 1px solid {BORDER};
    border-radius: 6px;
    padding: 7px 16px;
    font-size: 10pt;
    font-weight: 500;
}}
QPushButton:hover    {{ background: {SURFACE}; border-color: {BORD_HI}; }}
QPushButton:pressed  {{ background: {BG}; }}
QPushButton:disabled {{ background: {SURFACE}; color: {FAINT}; border-color: {BORDER}; }}

QPushButton[accent="true"] {{
    background: {ACCENT};
    color: #001523;
    border: none;
    border-radius: 8px;
    padding: 0 24px;
    font-size: 11pt;
    font-weight: 700;
    letter-spacing: 0.6px;
    min-height: 38px;
}}
QPushButton[accent="true"]:hover    {{ background: #3FB3FF; }}
QPushButton[accent="true"]:pressed  {{ background: #0D8AE8; }}
QPushButton[accent="true"]:disabled {{ background: {SURFACE}; color: {FAINT}; }}

QPushButton[danger="true"] {{
    background: {DANGER};
    color: #fff;
    border: none;
    border-radius: 8px;
    padding: 0 24px;
    font-size: 11pt;
    font-weight: 700;
    letter-spacing: 0.6px;
    min-height: 38px;
}}
QPushButton[danger="true"]:hover    {{ background: #F87171; }}
QPushButton[danger="true"]:pressed  {{ background: #DC2626; }}

QLabel#licenseChip {{
    background: {SURF2};
    border: 1px solid {BORDER};
    border-radius: 14px;
    padding: 4px 14px;
    color: {DIM};
}}
QLabel#licenseChip[state="locked"] {{
    background: {SURF2};
    border: 1px solid {BORDER};
    color: {DIM};
}}
QLabel#licenseChip[state="active"] {{
    background: rgba(52,211,153,0.10);
    border: 1px solid rgba(52,211,153,0.45);
    color: {OK};
}}
"""


# ── frameless drag helper ─────────────────────────────────────────────────────
class _Drag(QtWidgets.QWidget):
    """Mixin: makes a widget drag the parent window."""
    def __init__(self):
        super().__init__()
        self._off: QtCore.QPoint | None = None

    def mousePressEvent(self, e):
        if e.button() == QtCore.Qt.MouseButton.LeftButton:
            self._off = e.globalPosition().toPoint() - self.window().frameGeometry().topLeft()

    def mouseMoveEvent(self, e):
        if self._off and e.buttons() & QtCore.Qt.MouseButton.LeftButton:
            self.window().move(e.globalPosition().toPoint() - self._off)

    def mouseReleaseEvent(self, e):
        self._off = None


class HeroBanner(_Drag):
    def __init__(self, height: int = 170):
        super().__init__()
        self.setFixedHeight(height)
        self._pix = QtGui.QPixmap(str(BANNER)) if BANNER.exists() else QtGui.QPixmap()

    def paintEvent(self, _):
        p = QtGui.QPainter(self)
        r = self.rect()
        p.fillRect(r, QtGui.QColor(SURFACE))
        if not self._pix.isNull():
            p.setRenderHint(QtGui.QPainter.RenderHint.SmoothPixmapTransform)
            # KeepAspectRatioByExpanding — fills the banner edge to edge.
            # Top/bottom (whichever overflow direction) gets cropped, but the
            # image always covers the full width of the panel.
            scaled = self._pix.scaled(r.size(),
                QtCore.Qt.AspectRatioMode.KeepAspectRatioByExpanding,
                QtCore.Qt.TransformationMode.SmoothTransformation)
            ox = (scaled.width()  - r.width())  // 2
            # Bias the visible window slightly upward (0.42 of the overflow)
            # so the TL logo + ball stay vertically centered instead of the
            # crop favoring the bottom court area.
            oy = int((scaled.height() - r.height()) * 0.42)
            p.drawPixmap(r, scaled, QtCore.QRect(ox, oy, r.width(), r.height()))
        # Bottom-half fade into BG so the banner blends smoothly into the
        # body. Starts subtle around the middle, ramps to fully opaque BG by
        # the bottom edge — kills the hard cut between banner and the rest
        # of the UI.
        bg = QtGui.QColor(BG)
        g = QtGui.QLinearGradient(0, 0, 0, r.height())
        g.setColorAt(0.00, QtGui.QColor(0, 0, 0,   0))
        g.setColorAt(0.45, QtGui.QColor(0, 0, 0,   0))
        g.setColorAt(0.70, QtGui.QColor(bg.red(), bg.green(), bg.blue(),  90))
        g.setColorAt(0.90, QtGui.QColor(bg.red(), bg.green(), bg.blue(), 210))
        g.setColorAt(1.00, bg)
        p.fillRect(r, g)
        # No bottom border anymore — the fade itself acts as the seam.


class WinBtn(QtWidgets.QAbstractButton):
    def __init__(self, kind: str, parent=None):
        super().__init__(parent)
        self._kind = kind
        self.setFixedSize(36, 28)
        self.setCursor(QtCore.Qt.CursorShape.PointingHandCursor)

    def paintEvent(self, _):
        p = QtGui.QPainter(self)
        p.setRenderHint(QtGui.QPainter.RenderHint.Antialiasing)
        hover = self.underMouse()
        if hover:
            bg = QtGui.QColor(DANGER if self._kind == "close" else BORD_HI)
            p.fillRect(self.rect(), bg)
        pen = QtGui.QPen(QtGui.QColor(TEXT if hover else DIM), 1)
        p.setPen(pen)
        cx, cy = self.width()//2, self.height()//2
        if self._kind == "min":
            p.drawLine(cx-5, cy+2, cx+5, cy+2)
        else:
            p.drawLine(cx-4, cy-4, cx+4, cy+4)
            p.drawLine(cx-4, cy+4, cx+4, cy-4)

    def enterEvent(self, e): self.update(); super().enterEvent(e)
    def leaveEvent(self, e): self.update(); super().leaveEvent(e)


class TabBtn(QtWidgets.QAbstractButton):
    """Pill-style tab — solid block when active, ghost when not.
       Bigger, bolder, on the bundled gaming font so the feature picker
       reads like a game-mode select instead of a webform tab."""
    def __init__(self, text: str, parent=None):
        super().__init__(parent)
        self.setText(text)
        self.setCheckable(True)
        self.setCursor(QtCore.Qt.CursorShape.PointingHandCursor)
        f = QtGui.QFont(_fam([
            "Russo One", "Bebas Neue",
            "Bahnschrift SemiCondensed", "Bahnschrift",
            "Inter", "Segoe UI Variable", "Segoe UI",
        ]))
        f.setPointSize(13)
        f.setWeight(QtGui.QFont.Weight(700))
        f.setLetterSpacing(QtGui.QFont.SpacingType.AbsoluteSpacing, 1.4)
        self.setFont(f)
        self.setFixedHeight(48)

    def sizeHint(self):
        fm = QtGui.QFontMetrics(self.font())
        return QtCore.QSize(fm.horizontalAdvance(self.text()) + 56, 48)

    def paintEvent(self, _):
        p = QtGui.QPainter(self)
        p.setRenderHint(QtGui.QPainter.RenderHint.Antialiasing)
        r = self.rect().adjusted(3, 5, -3, -5)
        active = self.isChecked()
        hover  = self.underMouse() and not active

        # Rectangle tabs (lightly rounded corners, ~8px radius) instead of
        # full pill ovals — reads as a button bank.
        radius = 8
        if active:
            # Soft cyan glow halo behind the active rectangle
            glow = QtGui.QColor(ACCENT); glow.setAlpha(60)
            p.setBrush(glow); p.setPen(QtCore.Qt.PenStyle.NoPen)
            p.drawRoundedRect(r.adjusted(-4, -4, 4, 4), radius + 3, radius + 3)
            p.setBrush(QtGui.QColor(ACCENT))
            p.drawRoundedRect(r, radius, radius)
            text_color = QtGui.QColor("#001023")
        elif hover:
            p.setBrush(QtGui.QColor(SURF2))
            p.setPen(QtCore.Qt.PenStyle.NoPen)
            p.drawRoundedRect(r, radius, radius)
            text_color = QtGui.QColor(TEXT)
        else:
            p.setBrush(QtCore.Qt.BrushStyle.NoBrush)
            p.setPen(QtGui.QPen(QtGui.QColor(BORDER), 1))
            p.drawRoundedRect(r, radius, radius)
            text_color = QtGui.QColor(DIM)

        p.setFont(self.font())
        p.setPen(text_color)
        p.drawText(r, QtCore.Qt.AlignmentFlag.AlignCenter, self.text())

    def enterEvent(self, e): self.update(); super().enterEvent(e)
    def leaveEvent(self, e): self.update(); super().leaveEvent(e)


class Card(QtWidgets.QFrame):
    """Subtle elevated surface — no visible border, depth from background only."""
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("Card")
        self.setStyleSheet(f"""
            QFrame#Card {{
                background: {SURFACE};
                border: none;
                border-radius: 10px;
            }}
        """)


class SectionLabel(QtWidgets.QWidget):
    """Confident section header — caps eyebrow + thin underline."""
    def __init__(self, text: str, parent=None):
        super().__init__(parent)
        self.setFixedHeight(24)
        self._text = text.upper()

    def paintEvent(self, _):
        p = QtGui.QPainter(self)
        p.setRenderHint(QtGui.QPainter.RenderHint.Antialiasing)
        f = QtGui.QFont(_fam(["Inter", "Segoe UI Variable Display", "Segoe UI"]))
        f.setPointSize(8); f.setWeight(QtGui.QFont.Weight(700))
        f.setLetterSpacing(QtGui.QFont.SpacingType.AbsoluteSpacing, 2.0)
        p.setFont(f)
        p.setPen(QtGui.QColor(DIM))
        fm = QtGui.QFontMetrics(f)
        text_w = fm.horizontalAdvance(self._text)
        p.drawText(QtCore.QRect(0, 0, text_w + 4, self.height()),
                   QtCore.Qt.AlignmentFlag.AlignLeft | QtCore.Qt.AlignmentFlag.AlignVCenter,
                   self._text)
        # Accent dash removed — kept the file as plain caps text only.


class SliderRow(QtWidgets.QWidget):
    valueChanged = QtCore.Signal(float)

    def __init__(self, label: str, lo: float, hi: float,
                 decimals: int = 1, unit: str = "", step: float = 0.1):
        super().__init__()
        self._dec = decimals
        h = QtWidgets.QHBoxLayout(self)
        h.setContentsMargins(0, 0, 0, 0); h.setSpacing(12)

        lbl = QtWidgets.QLabel(label.upper())
        lbl.setFont(label_font(8))
        lbl.setStyleSheet(f"color: {DIM};")
        lbl.setFixedWidth(130)
        h.addWidget(lbl)

        self.slider = QtWidgets.QSlider(QtCore.Qt.Orientation.Horizontal)
        self.slider.setRange(int(lo * 10**decimals), int(hi * 10**decimals))
        self.slider.setSingleStep(max(1, int(step * 10**decimals)))
        self.slider.valueChanged.connect(self._on)
        h.addWidget(self.slider, 1)

        self.val = QtWidgets.QLabel("0")
        self.val.setFont(mono_font(11, 600))
        self.val.setStyleSheet(f"color: {TEXT};")
        self.val.setFixedWidth(52)
        self.val.setAlignment(QtCore.Qt.AlignmentFlag.AlignRight | QtCore.Qt.AlignmentFlag.AlignVCenter)
        h.addWidget(self.val)

        if unit:
            u = QtWidgets.QLabel(unit)
            u.setFont(label_font(7))
            u.setStyleSheet(f"color: {FAINT};")
            u.setFixedWidth(24)
            h.addWidget(u)

    def setValue(self, v: float):
        self.slider.setValue(int(round(v * 10**self._dec)))

    def value(self) -> float:
        return self.slider.value() / 10**self._dec

    def _on(self, _):
        v = self.value()
        self.val.setText(f"{v:.{self._dec}f}")
        self.valueChanged.emit(v)


class ToggleRow(QtWidgets.QWidget):
    toggled = QtCore.Signal(bool)

    def __init__(self, label: str, hint: str = ""):
        super().__init__()
        h = QtWidgets.QHBoxLayout(self)
        h.setContentsMargins(0, 0, 0, 0); h.setSpacing(12)

        col = QtWidgets.QVBoxLayout(); col.setSpacing(1)
        lbl = QtWidgets.QLabel(label.upper())
        lbl.setFont(label_font(8)); lbl.setStyleSheet(f"color: {TEXT};")
        col.addWidget(lbl)
        if hint:
            sub = QtWidgets.QLabel(hint)
            sub.setFont(ui_font(8)); sub.setStyleSheet(f"color: {DIM};")
            col.addWidget(sub)
        h.addLayout(col, 1)

        self._box = QtWidgets.QCheckBox()
        self._box.toggled.connect(self.toggled)
        h.addWidget(self._box)

    def setChecked(self, v: bool): self._box.setChecked(bool(v))
    def isChecked(self) -> bool:   return self._box.isChecked()


class ColumnHeader(QtWidgets.QLabel):
    """Big centered cyan column header (SHOOTING / DEFENSE / BOOSTS)."""
    def __init__(self, text: str):
        super().__init__(text.upper())
        f = QtGui.QFont(_fam(["Inter", "Segoe UI Variable Display", "Segoe UI"]))
        f.setPointSize(13); f.setWeight(QtGui.QFont.Weight(800))
        f.setLetterSpacing(QtGui.QFont.SpacingType.AbsoluteSpacing, 4.5)
        self.setFont(f)
        self.setAlignment(QtCore.Qt.AlignmentFlag.AlignCenter)
        self.setStyleSheet(f"color: {ACCENT}; padding: 4px 0 2px 0;")


class FieldLabel(QtWidgets.QLabel):
    """Small caps label that sits above a control."""
    def __init__(self, text: str):
        super().__init__(text.upper())
        f = QtGui.QFont(_fam(["Inter", "Segoe UI Variable Text", "Segoe UI"]))
        f.setPointSize(8); f.setWeight(QtGui.QFont.Weight(700))
        f.setLetterSpacing(QtGui.QFont.SpacingType.AbsoluteSpacing, 2.0)
        self.setFont(f)
        self.setAlignment(QtCore.Qt.AlignmentFlag.AlignCenter)
        self.setStyleSheet(f"color: {DIM};")


class StepBtn(QtWidgets.QPushButton):
    """Small step button used flanking a slider (-2 / -.5 / +.5 / +2)."""
    def __init__(self, text: str):
        super().__init__(text)
        self.setFixedSize(34, 26)
        f = QtGui.QFont(_fam(["Inter", "Segoe UI Variable Text", "Segoe UI"]))
        f.setPointSize(9); f.setWeight(QtGui.QFont.Weight(600))
        self.setFont(f)
        self.setStyleSheet(f"""
            QPushButton {{
                background: {SURF2}; color: {DIM};
                border: 1px solid {BORDER}; border-radius: 5px;
                padding: 0;
            }}
            QPushButton:hover  {{ color: {TEXT}; border-color: {BORD_HI}; }}
            QPushButton:pressed{{ background: {BG}; }}
        """)


class BigStepSlider(QtWidgets.QWidget):
    """Centered field label + huge value + step buttons + slider."""
    valueChanged = QtCore.Signal(float)

    def __init__(self, label: str, lo: float, hi: float,
                 unit: str = "%", decimals: int = 1, default: float = 0.0,
                 small_step: float = 0.5, big_step: float = 2.0):
        super().__init__()
        self._dec = decimals
        self._unit = unit

        col = QtWidgets.QVBoxLayout(self)
        col.setContentsMargins(0, 0, 0, 0); col.setSpacing(6)

        col.addWidget(FieldLabel(label))

        self.val = QtWidgets.QLabel(f"{default:.{decimals}f}{unit}")
        f = QtGui.QFont(_fam(["Inter", "Segoe UI Variable Display", "Segoe UI"]))
        f.setPointSize(30); f.setWeight(QtGui.QFont.Weight(800))
        f.setLetterSpacing(QtGui.QFont.SpacingType.AbsoluteSpacing, -1.0)
        self.val.setFont(f)
        self.val.setAlignment(QtCore.Qt.AlignmentFlag.AlignCenter)
        self.val.setStyleSheet(f"color: {ACCENT};")
        col.addWidget(self.val)

        row = QtWidgets.QHBoxLayout(); row.setSpacing(4)
        bm2  = StepBtn(f"-{big_step:g}");   bms = StepBtn(f"-{small_step:g}")
        bps  = StepBtn(f"+{small_step:g}"); bp2 = StepBtn(f"+{big_step:g}")
        self.slider = QtWidgets.QSlider(QtCore.Qt.Orientation.Horizontal)
        self.slider.setRange(int(lo * 10**decimals), int(hi * 10**decimals))
        self.slider.setSingleStep(max(1, int(small_step * 10**decimals)))
        self.slider.setPageStep(max(1, int(big_step * 10**decimals)))
        self.slider.setValue(int(round(default * 10**decimals)))
        self.slider.valueChanged.connect(self._on_change)
        bm2.clicked.connect(lambda: self._step(-big_step))
        bms.clicked.connect(lambda: self._step(-small_step))
        bps.clicked.connect(lambda: self._step(+small_step))
        bp2.clicked.connect(lambda: self._step(+big_step))
        row.addWidget(bm2); row.addWidget(bms)
        row.addWidget(self.slider, 1)
        row.addWidget(bps); row.addWidget(bp2)
        col.addLayout(row)

    def _step(self, delta: float):
        self.setValue(self.value() + delta)

    def _on_change(self, _):
        v = self.value()
        self.val.setText(f"{v:.{self._dec}f}{self._unit}")
        self.valueChanged.emit(v)

    def setValue(self, v: float):
        self.slider.setValue(int(round(v * 10**self._dec)))

    def value(self) -> float:
        return self.slider.value() / 10**self._dec


class StateToggle(QtWidgets.QWidget):
    """Checkbox + LABEL + ON/OFF state text."""
    toggled = QtCore.Signal(bool)

    def __init__(self, label: str):
        super().__init__()
        h = QtWidgets.QHBoxLayout(self)
        h.setContentsMargins(0, 0, 0, 0); h.setSpacing(10)

        self._box = QtWidgets.QCheckBox()
        self._box.toggled.connect(self._on_toggle)
        h.addWidget(self._box)

        self._lbl = QtWidgets.QLabel(label.upper())
        f = QtGui.QFont(_fam(["Inter", "Segoe UI Variable Text", "Segoe UI"]))
        f.setPointSize(9); f.setWeight(QtGui.QFont.Weight(700))
        f.setLetterSpacing(QtGui.QFont.SpacingType.AbsoluteSpacing, 1.4)
        self._lbl.setFont(f)
        self._lbl.setStyleSheet(f"color: {TEXT};")
        h.addWidget(self._lbl)

        h.addStretch(1)

        self._state = QtWidgets.QLabel("OFF")
        self._state.setFont(f)
        self._state.setStyleSheet(f"color: {DIM};")
        h.addWidget(self._state)

    def _on_toggle(self, v: bool):
        # No cyan accent highlight on the ON state — user wanted the text
        # plain. Use TEXT for ON, DIM for OFF so it still reads but doesn't
        # glow.
        self._state.setText("ON" if v else "OFF")
        self._state.setStyleSheet(f"color: {TEXT if v else DIM};")
        self.toggled.emit(v)

    def setChecked(self, v: bool):
        self._box.setChecked(bool(v))
        self._on_toggle(bool(v))

    def isChecked(self) -> bool:
        return self._box.isChecked()


class StatLine(QtWidgets.QWidget):
    """Right-aligned big number with caps label on the left."""
    def __init__(self, label: str, value: str = "—"):
        super().__init__()
        h = QtWidgets.QHBoxLayout(self)
        h.setContentsMargins(0, 0, 0, 0); h.setSpacing(12)
        self._lbl = QtWidgets.QLabel(label.upper())
        f1 = QtGui.QFont(_fam(["Inter", "Segoe UI Variable Text", "Segoe UI"]))
        f1.setPointSize(9); f1.setWeight(QtGui.QFont.Weight(700))
        f1.setLetterSpacing(QtGui.QFont.SpacingType.AbsoluteSpacing, 1.6)
        self._lbl.setFont(f1)
        self._lbl.setStyleSheet(f"color: {DIM};")
        h.addWidget(self._lbl)
        h.addStretch(1)
        self._val = QtWidgets.QLabel(value)
        f2 = QtGui.QFont(_fam(["Inter", "Segoe UI Variable Display", "Segoe UI"]))
        f2.setPointSize(20); f2.setWeight(QtGui.QFont.Weight(800))
        self._val.setFont(f2)
        self._val.setStyleSheet(f"color: {ACCENT};")
        self._val.setAlignment(QtCore.Qt.AlignmentFlag.AlignRight | QtCore.Qt.AlignmentFlag.AlignVCenter)
        h.addWidget(self._val)

    def setValue(self, s: str): self._val.setText(s)


class StatBox(QtWidgets.QWidget):
    """Big jersey-number stat. Dim caps label up top, huge bold number below."""
    def __init__(self, label: str, value: str = "—"):
        super().__init__()
        self.setStyleSheet(f"""
            background: {SURF2};
            border: none;
            border-radius: 10px;
        """)
        v = QtWidgets.QVBoxLayout(self)
        v.setContentsMargins(20, 16, 20, 18); v.setSpacing(6)

        self._lbl = QtWidgets.QLabel(label.upper())
        f1 = QtGui.QFont(_fam(["Inter", "Segoe UI Variable Text", "Segoe UI"]))
        f1.setPointSize(8); f1.setWeight(QtGui.QFont.Weight(700))
        f1.setLetterSpacing(QtGui.QFont.SpacingType.AbsoluteSpacing, 2.0)
        self._lbl.setFont(f1)
        self._lbl.setStyleSheet(f"background: transparent; color: {DIM}; border: none;")
        v.addWidget(self._lbl)

        self._val = QtWidgets.QLabel(value)
        f2 = QtGui.QFont(_fam(["Inter", "Segoe UI Variable Display", "Segoe UI"]))
        f2.setPointSize(28); f2.setWeight(QtGui.QFont.Weight(800))
        f2.setLetterSpacing(QtGui.QFont.SpacingType.AbsoluteSpacing, -0.8)
        self._val.setFont(f2)
        self._val.setStyleSheet(f"background: transparent; color: {TEXT}; border: none;")
        v.addWidget(self._val)

    def setValue(self, s: str): self._val.setText(s)


class EngBtn(QtWidgets.QPushButton):
    """Hero Start/Stop button. Picks accent vs danger via property."""
    def __init__(self, text_idle="START ENGINE", text_running="STOP ENGINE"):
        super().__init__(text_idle)
        self._running = False
        self._text_idle = text_idle
        self._text_running = text_running
        self.setMinimumHeight(48)
        self.setProperty("accent", True)
        f = QtGui.QFont(_fam(["Inter", "Segoe UI Variable Display", "Segoe UI"]))
        f.setPointSize(11); f.setWeight(QtGui.QFont.Weight(800))
        f.setLetterSpacing(QtGui.QFont.SpacingType.AbsoluteSpacing, 1.0)
        self.setFont(f)

    def setRunning(self, v: bool):
        self._running = v
        self.setText(self._text_running if v else self._text_idle)
        self.setProperty("accent", not v)
        self.setProperty("danger", v)
        self.style().unpolish(self); self.style().polish(self)


# ── helpers ───────────────────────────────────────────────────────────────────
def _hr(parent=None):
    f = QtWidgets.QFrame(parent)
    f.setFixedHeight(1)
    f.setStyleSheet(f"background: {BORDER};")
    return f


def _vsep(parent=None):
    f = QtWidgets.QFrame(parent)
    f.setFixedWidth(1)
    f.setStyleSheet(f"background: {BORDER};")
    return f


def _divider() -> QtWidgets.QFrame:
    f = QtWidgets.QFrame()
    f.setFixedHeight(1)
    f.setStyleSheet(f"background: {BORDER};")
    return f


# ── HUD / sci-fi widgets ─────────────────────────────────────────────────────
# Replaces the unicode ⬡ that screamed "AI placeholder". Painted vector hex
# with a soft outer glow that slowly pulses. Used as the lock-state icon.

class HexIcon(QtWidgets.QWidget):
    def __init__(self, size: int = 88, color: str = ACCENT, parent=None):
        super().__init__(parent)
        self.setFixedSize(size, size)
        self._color = QtGui.QColor(color)
        self._t = 0.0
        self._timer = QtCore.QTimer(self)
        self._timer.timeout.connect(self._tick)
        self._timer.start(60)

    def _tick(self):
        self._t = (self._t + 0.06) % (3.14159 * 2)
        self.update()

    def paintEvent(self, _):
        import math
        p = QtGui.QPainter(self)
        p.setRenderHint(QtGui.QPainter.RenderHint.Antialiasing)
        cx, cy = self.width()/2, self.height()/2
        radius = min(cx, cy) - 8
        # outer glow rings — radial fade
        pulse = 0.55 + 0.45 * (0.5 + 0.5 * math.sin(self._t))
        for i, (off, alpha) in enumerate(((10, 22), (6, 38), (3, 70))):
            c = QtGui.QColor(self._color); c.setAlpha(int(alpha * pulse))
            pen = QtGui.QPen(c, 2 + (2 - i))
            p.setPen(pen); p.setBrush(QtCore.Qt.BrushStyle.NoBrush)
            self._draw_hex(p, cx, cy, radius + off)
        # core hex — solid line
        c = QtGui.QColor(self._color); c.setAlpha(220)
        p.setPen(QtGui.QPen(c, 2))
        self._draw_hex(p, cx, cy, radius)
        # tiny center dot
        p.setBrush(QtGui.QBrush(c)); p.setPen(QtCore.Qt.PenStyle.NoPen)
        p.drawEllipse(QtCore.QPointF(cx, cy), 3, 3)

    @staticmethod
    def _draw_hex(p: QtGui.QPainter, cx: float, cy: float, r: float):
        import math
        pts = []
        for i in range(6):
            a = math.radians(60 * i - 30)  # flat-top hex
            pts.append(QtCore.QPointF(cx + r * math.cos(a), cy + r * math.sin(a)))
        p.drawPolygon(QtGui.QPolygonF(pts))


class HudFrame(QtWidgets.QFrame):
    """Card variant with hairline border + four corner brackets — like a
       targeting reticle around your data. Use in place of plain Card when
       you want it to feel like a cheat-menu / FUI panel.
       The corner brackets are painted, not styled, so they don't get
       smudged by border-radius rules."""
    def __init__(self, parent=None, accent: str = ACCENT, bracket: int = 14):
        super().__init__(parent)
        self.setObjectName("HudFrame")
        self._accent = QtGui.QColor(accent)
        self._bracket = bracket
        self.setStyleSheet(f"""
            QFrame#HudFrame {{
                background: {SURFACE};
                border: 1px solid {BORDER};
                border-radius: 4px;
            }}
        """)

    def paintEvent(self, e):
        super().paintEvent(e)
        p = QtGui.QPainter(self)
        p.setRenderHint(QtGui.QPainter.RenderHint.Antialiasing)
        c = QtGui.QColor(self._accent); c.setAlpha(230)
        p.setPen(QtGui.QPen(c, 2))
        b = self._bracket
        r = self.rect().adjusted(1, 1, -1, -1)
        # top-left
        p.drawLine(r.left(),  r.top()+b, r.left(),  r.top())
        p.drawLine(r.left(),  r.top(),   r.left()+b, r.top())
        # top-right
        p.drawLine(r.right()-b, r.top(),  r.right(), r.top())
        p.drawLine(r.right(),   r.top(),  r.right(), r.top()+b)
        # bottom-left
        p.drawLine(r.left(),  r.bottom()-b, r.left(),  r.bottom())
        p.drawLine(r.left(),  r.bottom(),   r.left()+b, r.bottom())
        # bottom-right
        p.drawLine(r.right()-b, r.bottom(), r.right(), r.bottom())
        p.drawLine(r.right(),   r.bottom()-b, r.right(), r.bottom())


class StatusStripe(QtWidgets.QWidget):
    """Thin status strip — used right under the hero banner. Two rows of
       label/value pairs separated by tactical brackets. Mimics FiveM /
       trainer-menu HUD strips. All text is monospace + caps + tracked."""
    def __init__(self, items: list[tuple[str, str]], parent=None):
        super().__init__(parent)
        self.setFixedHeight(38)
        self.setStyleSheet(f"background: {BG}; border-bottom: 1px solid {BORDER};")
        h = QtWidgets.QHBoxLayout(self)
        h.setContentsMargins(28, 0, 28, 0); h.setSpacing(22)
        self._vals: dict[str, QtWidgets.QLabel] = {}
        for i, (k, v) in enumerate(items):
            if i:
                sep = QtWidgets.QLabel("//")
                sep.setFont(mono_font(9, 700))
                sep.setStyleSheet(f"color: {FAINT};")
                h.addWidget(sep)
            tag = QtWidgets.QLabel(k.upper())
            tag.setFont(label_font(7))
            tag.setStyleSheet(f"color: {DIM};")
            h.addWidget(tag)
            val = QtWidgets.QLabel(v)
            val.setFont(mono_font(9, 700))
            val.setStyleSheet(f"color: {ACCENT};")
            h.addWidget(val)
            self._vals[k] = val
        h.addStretch(1)

    def setValue(self, k: str, v: str):
        if k in self._vals: self._vals[k].setText(v)


# ── icon sidebar (vector, no SVG asset needed) ──────────────────────────────
# Tiny QPainter routines for the cheat-menu sidebar icons. Each takes
# (painter, rect, color) and draws inside the rect. Used by IconNavBtn.

def _draw_logo(p: QtGui.QPainter, r: QtCore.QRect, c: QtGui.QColor):
    """Stylized 'L' shield — bold geometric mark for the sidebar top."""
    p.setPen(QtGui.QPen(c, 2))
    cx, cy = r.center().x(), r.center().y()
    sz = min(r.width(), r.height()) // 2 - 2
    pts = [
        QtCore.QPointF(cx - sz, cy - sz),
        QtCore.QPointF(cx + sz, cy - sz),
        QtCore.QPointF(cx + sz * 0.5, cy + sz * 0.3),
        QtCore.QPointF(cx,           cy + sz),
        QtCore.QPointF(cx - sz * 0.5, cy + sz * 0.3),
    ]
    p.setBrush(QtGui.QBrush(QtGui.QColor(c.red(), c.green(), c.blue(), 70)))
    p.drawPolygon(QtGui.QPolygonF(pts))


def _draw_crosshair(p: QtGui.QPainter, r: QtCore.QRect, c: QtGui.QColor):
    p.setPen(QtGui.QPen(c, 2))
    p.setBrush(QtCore.Qt.BrushStyle.NoBrush)
    cx, cy = r.center().x(), r.center().y()
    rad = min(r.width(), r.height()) // 2 - 4
    p.drawEllipse(QtCore.QPointF(cx, cy), rad, rad)
    p.drawLine(cx - rad - 2, cy, cx - rad // 2, cy)
    p.drawLine(cx + rad // 2, cy, cx + rad + 2, cy)
    p.drawLine(cx, cy - rad - 2, cx, cy - rad // 2)
    p.drawLine(cx, cy + rad // 2, cx, cy + rad + 2)
    p.setBrush(QtGui.QBrush(c))
    p.drawEllipse(QtCore.QPointF(cx, cy), 2, 2)


def _draw_eye(p: QtGui.QPainter, r: QtCore.QRect, c: QtGui.QColor):
    p.setPen(QtGui.QPen(c, 2))
    p.setBrush(QtCore.Qt.BrushStyle.NoBrush)
    cx, cy = r.center().x(), r.center().y()
    w = r.width() // 2 - 2
    h = r.height() // 4
    path = QtGui.QPainterPath()
    path.moveTo(cx - w, cy)
    path.quadTo(cx, cy - h * 1.7, cx + w, cy)
    path.quadTo(cx, cy + h * 1.7, cx - w, cy)
    p.drawPath(path)
    p.setBrush(QtGui.QBrush(c))
    p.drawEllipse(QtCore.QPointF(cx, cy), 3, 3)


def _draw_target(p: QtGui.QPainter, r: QtCore.QRect, c: QtGui.QColor):
    """Bullseye-style target — for the Shooting tab."""
    p.setPen(QtGui.QPen(c, 2))
    p.setBrush(QtCore.Qt.BrushStyle.NoBrush)
    cx, cy = r.center().x(), r.center().y()
    rad = min(r.width(), r.height()) // 2 - 3
    p.drawEllipse(QtCore.QPointF(cx, cy), rad,        rad)
    p.drawEllipse(QtCore.QPointF(cx, cy), rad * 0.55, rad * 0.55)
    p.setBrush(QtGui.QBrush(c))
    p.drawEllipse(QtCore.QPointF(cx, cy), 2.5, 2.5)


def _draw_dots(p: QtGui.QPainter, r: QtCore.QRect, c: QtGui.QColor):
    p.setBrush(QtGui.QBrush(c)); p.setPen(QtCore.Qt.PenStyle.NoPen)
    cx, cy = r.center().x(), r.center().y()
    p.drawEllipse(QtCore.QPointF(cx - 7, cy), 2.5, 2.5)
    p.drawEllipse(QtCore.QPointF(cx,     cy), 2.5, 2.5)
    p.drawEllipse(QtCore.QPointF(cx + 7, cy), 2.5, 2.5)


ICON_PAINTERS = {
    "logo":       _draw_logo,
    "crosshair":  _draw_crosshair,
    "eye":        _draw_eye,
    "target":     _draw_target,
    "dots":       _draw_dots,
}


class IconNavBtn(QtWidgets.QAbstractButton):
    """Square icon button for the vertical nav sidebar. Active = soft glow
       background + cyan icon. Idle = dim icon. Painted vector — no asset."""
    def __init__(self, kind: str, parent=None):
        super().__init__(parent)
        self._kind = kind
        self.setCheckable(True)
        self.setFixedSize(48, 48)
        self.setCursor(QtCore.Qt.CursorShape.PointingHandCursor)

    def paintEvent(self, _):
        p = QtGui.QPainter(self)
        p.setRenderHint(QtGui.QPainter.RenderHint.Antialiasing)
        active = self.isChecked()
        hover  = self.underMouse() and not active
        if active:
            c = QtGui.QColor(ACCENT); c.setAlpha(38)
            p.setBrush(QtGui.QBrush(c)); p.setPen(QtCore.Qt.PenStyle.NoPen)
            p.drawRoundedRect(self.rect().adjusted(4, 4, -4, -4), 10, 10)
        elif hover:
            c = QtGui.QColor(SURF2)
            p.setBrush(QtGui.QBrush(c)); p.setPen(QtCore.Qt.PenStyle.NoPen)
            p.drawRoundedRect(self.rect().adjusted(4, 4, -4, -4), 10, 10)
        col = QtGui.QColor(ACCENT) if active else QtGui.QColor(DIM)
        if hover and not active: col = QtGui.QColor(TEXT)
        painter = ICON_PAINTERS.get(self._kind)
        if painter:
            painter(p, self.rect(), col)

    def enterEvent(self, e): self.update(); super().enterEvent(e)
    def leaveEvent(self, e): self.update(); super().leaveEvent(e)


class IconSidebar(QtWidgets.QWidget):
    """Vertical strip of IconNavBtns. Emits navChanged(index) on click."""
    navChanged = QtCore.Signal(int)

    def __init__(self, kinds: list[str], parent=None):
        super().__init__(parent)
        self.setFixedWidth(64)
        self.setStyleSheet(f"background: {SURFACE}; border-right: 1px solid {BORDER};")
        v = QtWidgets.QVBoxLayout(self)
        v.setContentsMargins(8, 18, 8, 18); v.setSpacing(6)

        # Top branding hex/logo — non-clickable, decorative
        logo = IconNavBtn("logo")
        logo.setEnabled(False); logo.setChecked(True)
        logo.setStyleSheet("")  # ensure no global QPushButton bg
        v.addWidget(logo, alignment=QtCore.Qt.AlignmentFlag.AlignHCenter)
        v.addSpacing(14)

        self._buttons: list[IconNavBtn] = []
        self._group   = QtWidgets.QButtonGroup(self)
        self._group.setExclusive(True)
        for i, kind in enumerate(kinds):
            b = IconNavBtn(kind)
            b.setChecked(i == 0)
            self._buttons.append(b)
            self._group.addButton(b, i)
            v.addWidget(b, alignment=QtCore.Qt.AlignmentFlag.AlignHCenter)
        v.addStretch(1)
        self._group.idClicked.connect(self.navChanged.emit)

    def setIndex(self, i: int):
        if 0 <= i < len(self._buttons):
            self._buttons[i].setChecked(True)


class GridCard(QtWidgets.QFrame):
    """Flat container — no border, no fill, no hover box. Just defines a
       padded region for grouped controls. The user wanted the boxy look
       removed; this leaves the inner text on the body BG with no chrome."""
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("GridCard")
        self.setStyleSheet(f"""
            QFrame#GridCard {{
                background: transparent;
                border: none;
                border-radius: 0;
            }}
        """)


class RoundedRoot(QtWidgets.QWidget):
    """Central widget that paints a rounded-rect background. Use as the
       MainWindow's centralWidget when the window is frameless + translucent
       so the visible window edges are rounded instead of square."""
    def __init__(self, radius: int = 14, color: str = BG, parent=None):
        super().__init__(parent)
        self._radius = radius
        self._color  = QtGui.QColor(color)

    def paintEvent(self, _):
        p = QtGui.QPainter(self)
        p.setRenderHint(QtGui.QPainter.RenderHint.Antialiasing)
        path = QtGui.QPainterPath()
        path.addRoundedRect(QtCore.QRectF(self.rect()), self._radius, self._radius)
        p.fillPath(path, self._color)


class GradientLabel(QtWidgets.QLabel):
    """QLabel that paints its text with a horizontal linear gradient instead
       of a flat color. Use setGradient([(0.0, c1), (1.0, c2), ...])."""
    def __init__(self, text="", parent=None):
        super().__init__(text, parent)
        self._stops: list[tuple[float, QtGui.QColor]] = [
            (0.0, QtGui.QColor(DIM)), (1.0, QtGui.QColor(DIM)),
        ]

    def setGradient(self, stops: list[tuple[float, str]]):
        self._stops = [(p, QtGui.QColor(c)) for p, c in stops]
        self.update()

    def paintEvent(self, _):
        p = QtGui.QPainter(self)
        p.setRenderHint(QtGui.QPainter.RenderHint.Antialiasing)
        p.setRenderHint(QtGui.QPainter.RenderHint.TextAntialiasing)
        # Measure the actual text rect so the gradient stretches across the
        # GLYPHS only — not the whole label width — otherwise short text on a
        # wide label washes out to one color.
        fm = QtGui.QFontMetrics(self.font())
        tw = fm.horizontalAdvance(self.text())
        align = self.alignment()
        if align & QtCore.Qt.AlignmentFlag.AlignRight:
            x0 = self.width() - tw
        elif align & QtCore.Qt.AlignmentFlag.AlignHCenter:
            x0 = (self.width() - tw) // 2
        else:
            x0 = 0
        grad = QtGui.QLinearGradient(x0, 0, x0 + tw, 0)
        for pos, color in self._stops:
            grad.setColorAt(pos, color)
        p.setPen(QtGui.QPen(QtGui.QBrush(grad), 1))
        p.setFont(self.font())
        p.drawText(self.rect(), int(align), self.text())


class HexBackground(QtWidgets.QWidget):
    """Subtle hex grid pattern for use as a body background. Painted, no
       image asset needed. Soft so it never competes with content."""
    def __init__(self, parent=None, color: str = BORDER, spacing: int = 32):
        super().__init__(parent)
        self._color = QtGui.QColor(color); self._color.setAlpha(70)
        self._spacing = spacing

    def paintEvent(self, _):
        import math
        p = QtGui.QPainter(self)
        p.setRenderHint(QtGui.QPainter.RenderHint.Antialiasing)
        p.fillRect(self.rect(), QtGui.QColor(BG))
        p.setPen(QtGui.QPen(self._color, 1))
        s = self._spacing
        r = s * 0.55
        h = math.sqrt(3) * r
        rows = int(self.height() / h) + 2
        cols = int(self.width() / (1.5 * r)) + 2
        for row in range(rows):
            for col in range(cols):
                cx = col * 1.5 * r
                cy = row * h + (h / 2 if col % 2 else 0)
                pts = []
                for i in range(6):
                    a = math.radians(60 * i)
                    pts.append(QtCore.QPointF(cx + r * math.cos(a),
                                              cy + r * math.sin(a)))
                p.drawPolygon(QtGui.QPolygonF(pts))
