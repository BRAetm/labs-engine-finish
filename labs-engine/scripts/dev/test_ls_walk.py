"""
test_ls_walk.py — verify SHM override actually reaches the PS5 game.

Cycles the left stick through 4 directions, 1 second each:
  forward (UP) → idle → backward (DOWN) → idle → repeat

Don't touch your controller while this runs. Stand your player on the
court. Expected behaviour:

   ✓ character walks UP, stops, walks DOWN, stops, repeats
     → SHM override path is reaching the PS5. Shot release will work too.

   ✗ character doesn't move at all
     → look at the Labs Engine output log. If you see
       "[OVERRIDE] applied  ... LSY=±32767 flags=0x8"
       lines, the bridge IS working — your player might be stuck against
       a court boundary. Reposition them mid-court.
       If you see NO [OVERRIDE] lines at all, no source is pushing
       through the fan-out — most likely your DualSense isn't connected
       so DualSenseSource isn't producing frames, and the override has
       no carrier wave to ride on.

XInput stick convention (matches our ControllerState struct):
   sThumbLY = +32767 → stick pushed UP   → "forward" in most games
   sThumbLY = -32767 → stick pushed DOWN → "backward"
The PS Remote Play sink negates Y once before handing to chiaki, so the
in-game direction matches what you'd expect from a real Xbox pad held
in the same direction.
"""
import sys
import time
from pathlib import Path

try:
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
except Exception:
    pass

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))

import labs_input_bridge as bridge

PHASE_MS = 1000   # how long each phase lasts

print(f"[TEST] LS-walk loop: UP {PHASE_MS}ms → idle → DOWN {PHASE_MS}ms → idle → repeat", flush=True)
print(f"[TEST] DON'T touch your controller. Watch your player.",                          flush=True)
print(f"[TEST] character moves up/down each cycle → SHM bridge → PS5 OK",                 flush=True)
print(f"[TEST] no movement → look for [OVERRIDE] lines in the log above",                 flush=True)
print(f"[TEST] press STOP SCRIPT to exit",                                                flush=True)
print("-" * 60, flush=True)

try:
    bridge._ensure_mm()
    print("[TEST] SHM mapped OK (Labs_Input_Override_v1)", flush=True)
except Exception as ex:
    print(f"[TEST] SHM map FAILED: {ex}", flush=True)
    sys.exit(1)

# (label, ls_y_value)
PHASES = [
    ("forward (UP)",   +32767),
    ("idle",           None),
    ("backward (DOWN)", -32767),
    ("idle",           None),
]

cycle = 0
try:
    while True:
        cycle += 1
        for label, val in PHASES:
            if val is None:
                bridge.clear()
                print(f"[TEST] cycle {cycle}  {label}", flush=True)
            else:
                bridge.set_ls_y(val, duration_ms=PHASE_MS + 100)
                print(f"[TEST] cycle {cycle}  {label}  LS_Y={val:+6d}  for {PHASE_MS}ms", flush=True)
            time.sleep(PHASE_MS / 1000.0)
except KeyboardInterrupt:
    pass
finally:
    bridge.clear()
    print("[TEST] cleared, bye", flush=True)
