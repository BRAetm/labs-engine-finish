"""xZPHigher — Labs Engine launcher for the ported ZP HIGHER Lite app.

The ported source lives in scripts/zphigher/ as a self-contained tree
(main.py + core/ + assets/ + profiles/). This wrapper:
  1. Switches CWD to scripts/zphigher/ so main.py's `import core.engine`
     and asset paths resolve correctly.
  2. Forces stdout to UTF-8 so the BGR-METER status lines don't crash
     Labs Engine's script-runner stdout pipe (cp1252 by default).
  3. Imports and runs main.main().

Spawned by Labs Engine's script picker like any other entry script.
"""
import os
import sys
from pathlib import Path

ROOT     = Path(__file__).resolve().parent
ZP_ROOT  = ROOT / "zphigher"

if not ZP_ROOT.exists():
    print(f"[xZPHigher] zphigher/ not found at {ZP_ROOT}")
    sys.exit(1)

# Force UTF-8 stdout so non-ASCII status lines don't kill the runner.
try:
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")
except Exception:
    pass

# main.py does `from core.engine import Engine` — for that to resolve we
# need ZP_ROOT first on sys.path AND CWD pointing there (so the assets/
# folder lookup `os.path.join(os.path.dirname(__file__), "assets")` works).
sys.path.insert(0, str(ZP_ROOT))
os.chdir(str(ZP_ROOT))

# Import last so the path tweaks above are in effect when main.py loads.
from main import main as _zp_main   # noqa: E402

if __name__ == "__main__":
    _zp_main()
