"""
controller_ps.py — PSControllerBridge
Fully reconstructed from ui.dll Nuitka bytecode hex analysis (2026-04-21).

Wraps VDS4Gamepad + VX360Gamepad, applies defense AI and stamina transforms,
runs 500Hz XInput passthrough loop, handles stick_tempo and quickstop sequences.

Confirmed constants (bytecode offsets 0x28D5xxx):
  stamina_scale          = 0.70
  _defense_boost_amount  = 1.30  (sensitivity_boost, RY only)
  _defense_lateral_scale = 1.20  (lateral_boost, RX only)
  _defense_r2_cap        = 0.40  (anti_blowby RT cap)
  _defense_flick_dur     = 0.080 (contest RS-up hold)
  _poll_rate             = 500   Hz
  tempo_ms               = 39
  _release_duration      = 0.030 (RT pulse)
"""
import threading
import time

try:
    import vgamepad as vg
    VG_OK = True
except ImportError:
    VG_OK = False

from core.xinput import read_xinput

_STAMINA_SCALE = 0.70
_DEFENSE_BOOST = 1.30
_LATERAL_SCALE = 1.20
_R2_CAP        = 0.40
_CONTEST_DUR   = 0.080
_CONTEST_COOL  = 0.600

_XI_BTNS = []
if VG_OK:
    _XI_BTNS = [
        (0x0001, vg.XUSB_BUTTON.XUSB_GAMEPAD_DPAD_UP),
        (0x0002, vg.XUSB_BUTTON.XUSB_GAMEPAD_DPAD_DOWN),
        (0x0004, vg.XUSB_BUTTON.XUSB_GAMEPAD_DPAD_LEFT),
        (0x0008, vg.XUSB_BUTTON.XUSB_GAMEPAD_DPAD_RIGHT),
        (0x0010, vg.XUSB_BUTTON.XUSB_GAMEPAD_START),
        (0x0020, vg.XUSB_BUTTON.XUSB_GAMEPAD_BACK),
        (0x0040, vg.XUSB_BUTTON.XUSB_GAMEPAD_LEFT_THUMB),
        (0x0080, vg.XUSB_BUTTON.XUSB_GAMEPAD_RIGHT_THUMB),
        (0x0100, vg.XUSB_BUTTON.XUSB_GAMEPAD_LEFT_SHOULDER),
        (0x0200, vg.XUSB_BUTTON.XUSB_GAMEPAD_RIGHT_SHOULDER),
        (0x1000, vg.XUSB_BUTTON.XUSB_GAMEPAD_A),
        (0x2000, vg.XUSB_BUTTON.XUSB_GAMEPAD_B),
        (0x4000, vg.XUSB_BUTTON.XUSB_GAMEPAD_X),
        (0x8000, vg.XUSB_BUTTON.XUSB_GAMEPAD_Y),
    ]


class PSControllerBridge:
    """
    Full PSControllerBridge reconstruction — class name and all attribute
    names confirmed from Nuitka bytecode __qualname__ strings.
    """

    def __init__(self):
        self.defense_enabled       = False
        self.infinite_stamina      = False

        self.anti_blowby           = True
        self.auto_hands_up         = True
        self.contest_assist        = True
        self.lateral_boost         = True
        self.sensitivity_boost     = True

        self.stick_tempo_enabled   = False
        self._tempo_active         = False
        self._tempo_rs_override    = None
        self._tempo_lock           = threading.Lock()
        self._tempo_cool_end       = 0.0

        self.quickstop_enabled     = False
        self._qs_square_held       = False
        self._qs_flicked           = False

        self._contest_end          = 0.0
        self._contesting           = False
        self._contest_lock         = threading.Lock()

        self._dpad_up_prev         = False
        self._qs_toggle_requested  = False

        self._running              = False

        self.ds4  = None
        self.x360 = None
        if VG_OK:
            self.ds4  = vg.VDS4Gamepad()
            self.x360 = vg.VX360Gamepad()
            self.ds4.reset();  self.ds4.update()
            self.x360.reset(); self.x360.update()
            # OPTIONS pulse so Chiaki/SDL detects DS4
            self.ds4.press_button(vg.DS4_BUTTONS.DS4_BUTTON_OPTIONS); self.ds4.update()
            time.sleep(0.05)
            self.ds4.release_button(vg.DS4_BUTTONS.DS4_BUTTON_OPTIONS); self.ds4.update()

    def start(self):
        """Launch 500Hz XInput passthrough thread."""
        self._running = True
        threading.Thread(target=self._passthrough_loop, daemon=True).start()
        print("[PS-CTRL] Input loop started (500Hz)")
        print(f"[PS-CTRL] VDS4={'YES' if self.ds4 else 'NO'}  VX360={'YES' if self.x360 else 'NO'}")

    def stop(self):
        self._running = False

    def _passthrough_loop(self):
        while self._running:
            self.passthrough(read_xinput())
            time.sleep(1 / 500)

    # ── public API ──────────────────────────────────────────────────────────

    def passthrough(self, gp):
        """Apply features and push state to DS4 + VX360. gp = XInput GAMEPAD."""
        if not self.ds4 or not gp:
            return

        lx = gp.sThumbLX / 32768.0
        ly = gp.sThumbLY / 32768.0
        rx = gp.sThumbRX / 32768.0
        ry = gp.sThumbRY / 32768.0
        lt = gp.bLeftTrigger  / 255.0
        rt = gp.bRightTrigger / 255.0

        if self.defense_enabled:
            rx, ry, lt, rt = self._apply_defense_ai(rx, ry, lt, rt)

        if self.infinite_stamina:
            rt = self._apply_stamina(rt)

        with self._contest_lock:
            if self._contesting and time.perf_counter() < self._contest_end:
                ry = 1.0
            else:
                self._contesting = False

        with self._tempo_lock:
            if self._tempo_rs_override is not None:
                rx, ry = self._tempo_rs_override

        self.ds4.left_joystick_float(lx, ly)
        self.ds4.right_joystick_float(rx, ry)
        self.ds4.left_trigger_float(lt)
        self.ds4.right_trigger_float(rt)
        self.ds4.update()

        if self.x360:
            self.x360.left_joystick_float(lx, ly)
            self.x360.right_joystick_float(rx, ry)
            self.x360.left_trigger_float(lt)
            self.x360.right_trigger_float(rt)
            for xi_bit, xusb_btn in _XI_BTNS:
                if gp.wButtons & xi_bit:
                    self.x360.press_button(xusb_btn)
                else:
                    self.x360.release_button(xusb_btn)
            self.x360.update()

        # DPAD_UP alone → signal quickstop toggle
        dpad_up = bool(gp.wButtons == 0x0001)
        if dpad_up and not self._dpad_up_prev:
            self._qs_toggle_requested = True
        self._dpad_up_prev = dpad_up

    def fire_shot(self, l2: bool = False, tempo: bool = False, tempo_ms: int = 39):
        """Route R2 fire through stick_tempo or quickstop if enabled."""
        if not self.ds4:
            return
        if self.stick_tempo_enabled and not l2:
            if time.perf_counter() < self._tempo_cool_end:
                return
            threading.Thread(target=self._stick_tempo_fire,
                             args=(tempo_ms,), daemon=True).start()
            return
        if self.quickstop_enabled and not l2:
            threading.Thread(target=self._quickstop_fire, daemon=True).start()
            return
        if tempo and not l2:
            time.sleep(tempo_ms / 1000.0)
        self._fire_rt()

    def contest_flick(self):
        """defense_contest_assist: RS-up 80ms when shot meter fires on defense."""
        if not self.contest_assist:
            return
        now = time.perf_counter()
        if now < self._contest_end + _CONTEST_COOL:
            return
        with self._contest_lock:
            self._contesting  = True
            self._contest_end = now + _CONTEST_DUR

    # ── private ─────────────────────────────────────────────────────────────

    def _fire_rt(self):
        """30ms RT pulse (_release_duration = 0.030, confirmed from bytecode)."""
        self.ds4.right_trigger_float(1.0); self.ds4.update()
        if self.x360:
            self.x360.right_trigger_float(1.0); self.x360.update()
        time.sleep(0.030)
        self.ds4.right_trigger_float(0.0); self.ds4.update()
        if self.x360:
            self.x360.right_trigger_float(0.0); self.x360.update()

    def _stick_tempo_fire(self, tempo_ms: int):
        """RS-down flick for tempo_ms → R2 → 1000ms cooldown."""
        if VG_OK:
            self.ds4.release_button(vg.DS4_BUTTONS.DS4_BUTTON_SQUARE)
            self.ds4.update()
        with self._tempo_lock:
            self._tempo_rs_override = (0.0, -1.0)
            self._tempo_active = True
        time.sleep(tempo_ms / 1000.0)
        with self._tempo_lock:
            self._tempo_rs_override = None
            self._tempo_active = False
        self._fire_rt()
        self._tempo_cool_end = time.perf_counter() + 1.0

    def _quickstop_fire(self):
        """RS pull-down 20ms → neutral 15ms → R2."""
        with self._tempo_lock:
            self._tempo_rs_override = (0.0, -1.0)
            self._qs_flicked = True
        time.sleep(0.020)
        with self._tempo_lock:
            self._tempo_rs_override = (0.0, 0.0)
        time.sleep(0.015)
        with self._tempo_lock:
            self._tempo_rs_override = None
            self._qs_flicked = False
        self._fire_rt()

    def _apply_defense_ai(self, rx, ry, lt, rt):
        """Mirrors PSControllerBridge._apply_defense_ai from ui.dll."""
        if self.lateral_boost and abs(rx) > 0.05:
            rx = max(-1.0, min(1.0, rx * _LATERAL_SCALE))
        if self.sensitivity_boost and abs(ry) > 0.05:
            ry = max(-1.0, min(1.0, ry * _DEFENSE_BOOST))
        if self.anti_blowby:
            rt = min(rt, _R2_CAP)
        return rx, ry, lt, rt

    def _apply_stamina(self, rt):
        """stamina_scale = 0.70."""
        return rt * _STAMINA_SCALE
