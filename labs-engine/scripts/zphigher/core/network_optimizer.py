"""
network_optimizer.py — 7 TCP/network registry tweaks for Remote Play latency.
Matches ZP HIGHER Lite's network_optimizer module (confirmed from ui.dll).

Tweaks:
  1. TcpAckFrequency = 1            ACK every packet
  2. TCPNoDelay = 1                 disable Nagle
  3. DisableBandwidthThrottling = 1 no SMB bandwidth cap
  4. DisableLargeSendOffload = 1    disable LSO
  5. NetworkThrottlingIndex = 0xFFFFFFFF  disable multimedia throttle
  6. SystemResponsiveness = 0       MMCSS max gaming priority
  7. DefaultTTL = 64                standard TTL

Requires admin — silently skips keys it cannot write.
Saves originals and can fully restore.
"""
import winreg
import ctypes

_TCP   = r"SYSTEM\CurrentControlSet\Services\Tcpip\Parameters"
_MMCSS = r"SOFTWARE\Microsoft\Windows NT\CurrentVersion\Multimedia\SystemProfile"
_LANMAN = r"SYSTEM\CurrentControlSet\Services\LanmanWorkstation\Parameters"

_TWEAKS = [
    (winreg.HKEY_LOCAL_MACHINE, _TCP,    "TcpAckFrequency",            winreg.REG_DWORD, 1),
    (winreg.HKEY_LOCAL_MACHINE, _TCP,    "TCPNoDelay",                 winreg.REG_DWORD, 1),
    (winreg.HKEY_LOCAL_MACHINE, _TCP,    "DefaultTTL",                 winreg.REG_DWORD, 64),
    (winreg.HKEY_LOCAL_MACHINE, _LANMAN, "DisableBandwidthThrottling", winreg.REG_DWORD, 1),
    (winreg.HKEY_LOCAL_MACHINE, _LANMAN, "DisableLargeSendOffload",    winreg.REG_DWORD, 1),
    (winreg.HKEY_LOCAL_MACHINE, _MMCSS,  "NetworkThrottlingIndex",     winreg.REG_DWORD, 0xFFFFFFFF),
    (winreg.HKEY_LOCAL_MACHINE, _MMCSS,  "SystemResponsiveness",       winreg.REG_DWORD, 0),
]

_originals: list = []


def is_admin() -> bool:
    try:
        return bool(ctypes.windll.shell32.IsUserAnAdmin())
    except Exception:
        return False


def apply() -> int:
    global _originals
    _originals = []
    written = 0
    for hive, path, name, reg_type, value in _TWEAKS:
        try:
            key = winreg.OpenKey(hive, path, 0, winreg.KEY_READ | winreg.KEY_WRITE)
            try:
                old_val, old_type = winreg.QueryValueEx(key, name)
                _originals.append((hive, path, name, old_type, old_val, True))
            except FileNotFoundError:
                _originals.append((hive, path, name, reg_type, None, False))
            winreg.SetValueEx(key, name, 0, reg_type, value)
            winreg.CloseKey(key)
            written += 1
        except Exception:
            pass
    print(f"[NET-OPT] Applied {written}/{len(_TWEAKS)} tweaks")
    return written


def restore() -> int:
    restored = 0
    for hive, path, name, reg_type, old_val, existed in _originals:
        try:
            key = winreg.OpenKey(hive, path, 0, winreg.KEY_WRITE)
            if existed:
                winreg.SetValueEx(key, name, 0, reg_type, old_val)
            else:
                try: winreg.DeleteValue(key, name)
                except FileNotFoundError: pass
            winreg.CloseKey(key)
            restored += 1
        except Exception:
            pass
    print(f"[NET-OPT] Restored {restored}/{len(_originals)} tweaks")
    _originals.clear()
    return restored
