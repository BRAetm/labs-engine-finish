"""
test_button_tap.py — prove the SHM bridge delivers button presses to PS5.

Taps Cross (A / confirm) every 2 seconds. Stand on the PS5 home screen,
or in any menu where pressing Cross confirms/advances. Don't touch your
controller.

Expected: menu advances / "bloops" sound every 2 seconds.
If the menu doesn't respond, the SHM bridge is working for STICKS
(test_ls_walk moves the character) but something strips BUTTONS between
DualSenseSource and the PS sink. Send me the log and I'll trace that.
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
import labs_input_bridge as b

print("[TEST] tapping Cross (A) every 2s — watch the PS5 menu", flush=True)
print("[TEST] press STOP SCRIPT to exit", flush=True)
print("-" * 60, flush=True)

n = 0
try:
    while True:
        n += 1
        b.press_button(b.BTN_A, duration_ms=120)
        print(f"[TEST] tap #{n}  button=Cross  120ms", flush=True)
        time.sleep(2.0)
except KeyboardInterrupt:
    pass
finally:
    b.clear()
    print("[TEST] cleared, bye", flush=True)
