"""
titan_bridge.py — Titan One / Two hardware bridge via gcdapi.dll ConsoleTuner SDK.
Reconstructed from ui.dll.
"""
import ctypes
import os


class TitanBridge:
    GCAPI_DLL = "gcdapi.dll"
    SLOT_RX   = 4
    SLOT_RY   = 5
    SLOT_R2   = 9

    def __init__(self):
        self._lib = None
        self._load_sdk()

    def _load_sdk(self):
        try:
            dll_path = os.path.join(os.path.dirname(__file__), self.GCAPI_DLL)
            if not os.path.exists(dll_path):
                dll_path = self.GCAPI_DLL
            self._lib = ctypes.CDLL(dll_path)
            self._lib.gcapi_Connect()
        except Exception as e:
            print(f"[TitanBridge] SDK load failed — {e}")
            self._lib = None

    def move_right_stick(self, dx: float, dy: float):
        if not self._lib: return
        try:
            self._lib.gcapi_SetOutput(self.SLOT_RX, int(max(-100, min(100, dx * 100))))
            self._lib.gcapi_SetOutput(self.SLOT_RY, int(max(-100, min(100, dy * 100))))
        except Exception:
            pass

    def press_right_trigger(self, value: float = 1.0):
        if not self._lib: return
        try:
            self._lib.gcapi_SetOutput(self.SLOT_R2, int(value * 100))
        except Exception:
            pass

    def release_right_trigger(self):
        self.press_right_trigger(0.0)

    def reset(self):
        if self._lib:
            try: self._lib.gcapi_Disconnect()
            except Exception: pass
