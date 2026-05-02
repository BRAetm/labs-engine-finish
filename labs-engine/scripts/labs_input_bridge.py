"""
labs_input_bridge.py — Python ↔ Labs Engine input override channel.

Win32 named SHM section "Labs_Input_Override_v2" carries a 32-byte record.
Both sides (this Python module and the C++ InputOverride class in LabsCore)
use the SAME timebase: raw QueryPerformanceCounter ticks. QPC is a hardware
counter — every process on the box reads the same value when called at the
same instant. No epoch math, no time.monotonic vs steady_clock ambiguity.

Layout (matches app/core/InputOverride.h, packed, no padding):
    char     magic[4]          // "LBSO"
    uint64_t expires_qpc       // QPC tick when the override expires
    uint8_t  rt
    uint8_t  l2
    uint8_t  flags             // bit 0 RT, bit 1 L2, bit 2 RS_Y, bit 3 LS_Y
    int16_t  rs_y              // override right-stick Y
    int16_t  ls_y              // override left-stick Y
    uint8_t  reserved[13]
"""
import ctypes
import mmap
import struct

# ── QPC bindings ─────────────────────────────────────────────────────────────
# QueryPerformanceCounter returns the raw counter; QueryPerformanceFrequency
# returns ticks-per-second so we can convert milliseconds → ticks.
_kernel32 = ctypes.windll.kernel32
_qpc      = _kernel32.QueryPerformanceCounter
_qpf      = _kernel32.QueryPerformanceFrequency
_qpc.argtypes = [ctypes.POINTER(ctypes.c_int64)]; _qpc.restype = ctypes.c_int
_qpf.argtypes = [ctypes.POINTER(ctypes.c_int64)]; _qpf.restype = ctypes.c_int

_freq = ctypes.c_int64(0)
_qpf(ctypes.byref(_freq))
_QPC_PER_MS = _freq.value // 1000  # integer ticks per millisecond


def _now_qpc() -> int:
    n = ctypes.c_int64(0)
    _qpc(ctypes.byref(n))
    return n.value


# ── SHM layout ────────────────────────────────────────────────────────────────
_SHM_NAME = "Labs_Input_Override_v2"
_SIZE     = 32
# little-endian packed: magic, expires, rt, l2, flags, rs_y, ls_y, buttons,
# rs_x, ls_x, reserved. Matches InputOverrideRecord in app/core/InputOverride.h.
_FMT      = "<4sQBBBhhHhh7s"

_FLAG_RT   = 0x01
_FLAG_L2   = 0x02
_FLAG_RSY  = 0x04
_FLAG_LSY  = 0x08
_FLAG_BTNS = 0x10
_FLAG_RSX  = 0x20
_FLAG_LSX  = 0x40

# ControllerButton bits — match app/core/InputTypes.h
BTN_DPAD_UP    = 0x0001
BTN_DPAD_DOWN  = 0x0002
BTN_DPAD_LEFT  = 0x0004
BTN_DPAD_RIGHT = 0x0008
BTN_START      = 0x0010  # Options on PS
BTN_BACK       = 0x0020  # Share on PS
BTN_L3         = 0x0040
BTN_R3         = 0x0080
BTN_L1         = 0x0100
BTN_R1         = 0x0200
BTN_GUIDE      = 0x0400  # PS button
BTN_A          = 0x1000  # Cross on PS
BTN_B          = 0x2000  # Circle
BTN_X          = 0x4000  # Square
BTN_Y          = 0x8000  # Triangle

_mm = None


def _ensure_mm():
    global _mm
    if _mm is not None:
        return _mm
    _mm = mmap.mmap(-1, _SIZE, tagname=_SHM_NAME)
    _zero_shm(_mm)
    return _mm


def _zero_shm(mm):
    mm.seek(0)
    mm.write(struct.pack(_FMT, b"LBSO", 0, 0, 0, 0, 0, 0, 0, 0, 0, b"\x00" * 7))


# Eager init: open and zero the SHM at module import. Earlier we did this
# lazily in _ensure_mm on first write, which left a window where stale data
# from a previously-killed script sat in the SHM before this script's first
# pulse — confusing the C++ apply() into thinking an old override was still
# live. Clearing on import slams the door on that race.
try:
    _ensure_mm()
except Exception as _ex:
    print(f"[bridge] eager SHM init failed: {_ex}", flush=True)


# Signal handler: try to clear the SHM cleanly on SIGTERM/SIGINT. Labs Engine
# uses QProcess::terminate which on Windows is a hard kill (TerminateProcess),
# so this handler usually won't fire — but when run from a terminal it will,
# and the eager init above + C++ sanity check below cover the hard-kill case.
import atexit as _atexit  # noqa: E402
import signal as _signal  # noqa: E402


def _cleanup(*_):
    try: clear()
    except Exception: pass


_atexit.register(_cleanup)
for _sig in (getattr(_signal, "SIGTERM", None),
             getattr(_signal, "SIGINT",  None),
             getattr(_signal, "SIGBREAK", None)):
    if _sig is not None:
        try: _signal.signal(_sig, _cleanup)
        except (ValueError, OSError): pass


def _write(rt: int, l2: int, flags: int, rs_y: int, ls_y: int,
           duration_ms: int, buttons: int = 0,
           rs_x: int = 0, ls_x: int = 0):
    mm = _ensure_mm()
    expires = _now_qpc() + duration_ms * _QPC_PER_MS
    mm.seek(0)
    mm.write(struct.pack(_FMT, b"LBSO", expires,
                         int(rt) & 0xFF, int(l2) & 0xFF, flags & 0xFF,
                         max(-32767, min(32767, int(rs_y))),
                         max(-32767, min(32767, int(ls_y))),
                         int(buttons) & 0xFFFF,
                         max(-32767, min(32767, int(rs_x))),
                         max(-32767, min(32767, int(ls_x))),
                         b"\x00" * 7))


def set_rt_pulse(value: int = 255, duration_ms: int = 30):
    _write(value, 0, _FLAG_RT, 0, 0, duration_ms)


def set_l2_pulse(value: int = 255, duration_ms: int = 30):
    _write(0, value, _FLAG_L2, 0, 0, duration_ms)


def set_rs_y(value: int = 0, duration_ms: int = 80):
    """Override right-stick Y. value=0 RELEASES the stick (shot-stick fire)."""
    _write(0, 0, _FLAG_RSY, value, 0, duration_ms)


def set_rs_x(value: int = 0, duration_ms: int = 80):
    """Override right-stick X. value=0 RELEASES the stick toward neutral."""
    _write(0, 0, _FLAG_RSX, 0, 0, duration_ms, rs_x=value)


def set_rs_xy(x: int, y: int, duration_ms: int = 80):
    """Override BOTH right-stick axes simultaneously. Used for multi-direction
       rhythm shot release — flick opposite of the user's hold vector."""
    _write(0, 0, _FLAG_RSY | _FLAG_RSX, y, 0, duration_ms, rs_x=x)


def set_ls_y(value: int = 0, duration_ms: int = 80):
    _write(0, 0, _FLAG_LSY, 0, value, duration_ms)


def fire_combo(duration_ms: int = 80):
    """RT pulse AND RS-Y release in one call. Covers both shot schemes."""
    _write(255, 0, _FLAG_RT | _FLAG_RSY, 0, 0, duration_ms)


def press_button(mask: int, duration_ms: int = 60):
    """Press one or more buttons for `duration_ms`. Use BTN_* constants."""
    _write(0, 0, _FLAG_BTNS, 0, 0, duration_ms, buttons=mask)


def clear():
    _write(0, 0, 0, 0, 0, 1)
