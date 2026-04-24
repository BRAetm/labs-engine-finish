"""
ZP_Higher_Lite.py — launcher that runs the unmodified ZP HIGHER Lite source
(under ./zp_higher_lite/) inside the Labs Engine scripts harness.

We don't touch the original code. We just:
  1. Add zp_higher_lite/ to sys.path so its `core.*` imports resolve.
  2. Monkey-patch its window finder to prefer the "Labs Engine" window
     (otherwise it'd look for "Xbox / Chiaki / PS Remote / Remote Play"
     and miss our app entirely).
  3. exec main.main() in-process.
"""
import os
import sys
from pathlib import Path

# ZP prints box-drawing chars (═) and arrows. Default Windows stdout is cp1252,
# which can't encode them — first such print crashes the engine thread. Force
# UTF-8 with replace fallback so a stray exotic char never kills us.
try:
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")
except Exception:
    pass

ROOT  = Path(__file__).resolve().parent
ZPDIR = ROOT / "zp_higher_lite"
if not ZPDIR.exists():
    print(f"[LAUNCHER] zp_higher_lite/ not found at {ZPDIR}", flush=True)
    sys.exit(1)

# Make `import core.engine`, `import core.controller_ps` resolve to the
# bundled tree.
sys.path.insert(0, str(ZPDIR))
os.chdir(ZPDIR)

# ── Window-finder patch ──────────────────────────────────────────────────────
# core/engine.py looks for windows whose title contains "Xbox", "Chiaki",
# "PS Remote", or "Remote Play". Our host window is "Labs Engine". We patch
# Engine._find_target_window to prefer that, falling back to the originals.
from core import engine as _eng  # noqa: E402

_orig_find = _eng.Engine._find_target_window  # keep around for fallback


def _patched_find(self):
    try:
        import win32gui
        hits = []
        def _cb(hwnd, _):
            t = win32gui.GetWindowText(hwnd) or ""
            if "Labs Engine" in t and win32gui.IsWindowVisible(hwnd):
                r = win32gui.GetWindowRect(hwnd)
                if (r[2] - r[0]) > 200 and (r[3] - r[1]) > 200:
                    hits.append((t, r))
        win32gui.EnumWindows(_cb, None)
        if hits:
            t, r = hits[0]
            print(f"[LAUNCHER] Capturing Labs Engine window: '{t}' "
                  f"{r[2]-r[0]}x{r[3]-r[1]}", flush=True)
            return r
    except Exception as ex:
        print(f"[LAUNCHER] win32gui error: {ex}", flush=True)
    # Fall back to ZP's original logic (Xbox / Chiaki / Remote Play / fullscreen)
    return _orig_find(self)


_eng.Engine._find_target_window = _patched_find

# ── Run ──────────────────────────────────────────────────────────────────────
import main as _zpmain  # noqa: E402

if __name__ == "__main__":
    _zpmain.main()
