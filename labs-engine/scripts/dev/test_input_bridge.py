"""
test_input_bridge.py — end-to-end SHM bridge sanity check.

Pulses RT=255 for 80ms once per second. With Labs Engine running, you should
see one `[OVERRIDE] applied  RT=255 L2=0 flags=0x1` line in the Labs Engine
output log per pulse, and the controller monitor (in the Labs Engine "video
display ↔ controller monitor" tabs) should briefly highlight RT.

If the Labs Engine log shows the override line: SHM bridge is working. If it
doesn't, the script and engine aren't talking and the override path is broken.

Run from the Labs Engine left rail (pick test_input_bridge.py and click run),
or from a terminal:  python test_input_bridge.py
"""
import os
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

INTERVAL_S      = 1.0
PULSE_MS        = 80
PULSE_VALUE_RT  = 255

print(f"[TEST] cwd={os.getcwd()}", flush=True)
print(f"[TEST] pulsing RT={PULSE_VALUE_RT} for {PULSE_MS}ms every {INTERVAL_S:.1f}s", flush=True)
print(f"[TEST] expect Labs Engine to log an [OVERRIDE] line per pulse",   flush=True)
print(f"[TEST] expect controller monitor to flicker RT each second",      flush=True)
print(f"[TEST] press Ctrl+C / STOP SCRIPT to exit",                       flush=True)
print("-" * 60, flush=True)

# Touch the SHM once up front so we know it created cleanly.
try:
    bridge._ensure_mm()
    print("[TEST] SHM mapped OK (Labs_Input_Override_v1)", flush=True)
except Exception as ex:
    print(f"[TEST] SHM map FAILED: {ex}", flush=True)
    sys.exit(1)

n = 0
try:
    while True:
        n += 1
        t0 = time.perf_counter()
        bridge.set_rt_pulse(PULSE_VALUE_RT, duration_ms=PULSE_MS)
        print(f"[TEST] pulse #{n}  RT={PULSE_VALUE_RT}  duration={PULSE_MS}ms", flush=True)
        # Hold until next interval boundary.
        elapsed = time.perf_counter() - t0
        time.sleep(max(0.0, INTERVAL_S - elapsed))
except KeyboardInterrupt:
    print("[TEST] stopping", flush=True)
finally:
    bridge.clear()
    print("[TEST] override cleared, bye", flush=True)
