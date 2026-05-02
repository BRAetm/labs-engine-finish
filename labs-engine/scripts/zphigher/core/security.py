"""
security.py — Process integrity checks.
Reconstructed from ui.dll. Blocks: CE, x64dbg, IDA, Wireshark, Fiddler.
Does NOT detect Frida or Ghidra (confirmed from bytecode scan).
"""
import ctypes
import psutil


class Security:
    BLOCKED_PROCESSES = [
        "cheatengine", "ollydbg", "x64dbg", "x32dbg",
        "ida", "ida64", "idat", "idat64",
        "wireshark", "fiddler", "procmon", "processhacker",
        "httpdebugger", "charles",
    ]

    def verify(self) -> bool:
        if self._debugger_present():
            return False
        if self._analysis_tools_running():
            return False
        return True

    def _debugger_present(self) -> bool:
        try:
            return bool(ctypes.windll.kernel32.IsDebuggerPresent())
        except Exception:
            return False

    def _analysis_tools_running(self) -> bool:
        try:
            for proc in psutil.process_iter(["name"]):
                name = (proc.info["name"] or "").lower()
                if any(b in name for b in self.BLOCKED_PROCESSES):
                    return True
        except Exception:
            pass
        return False
