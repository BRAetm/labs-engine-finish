"""Shared XInput reader — used by engine.py and controller_ps.py."""
import ctypes

try:
    _lib = ctypes.windll.xinput1_4
    XI_OK = True
except Exception:
    XI_OK = False


class _XGP(ctypes.Structure):
    _fields_ = [
        ("wButtons",      ctypes.c_ushort),
        ("bLeftTrigger",  ctypes.c_ubyte),
        ("bRightTrigger", ctypes.c_ubyte),
        ("sThumbLX",      ctypes.c_short),
        ("sThumbLY",      ctypes.c_short),
        ("sThumbRX",      ctypes.c_short),
        ("sThumbRY",      ctypes.c_short),
    ]


class _XS(ctypes.Structure):
    _fields_ = [("dwPacketNumber", ctypes.c_ulong), ("Gamepad", _XGP)]


def read_xinput(idx: int = 0):
    if not XI_OK:
        return None
    s = _XS()
    return s.Gamepad if _lib.XInputGetState(idx, ctypes.byref(s)) == 0 else None
