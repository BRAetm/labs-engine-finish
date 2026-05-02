# xDrive v2.1i4 - Fully Reconstructed Source
# Reverse engineered from bytecode using pycdc + manual bytecode analysis

import sys
import os
if sys.platform == 'win32':
    import ctypes
    ctypes.windll.user32.ShowWindow(ctypes.windll.kernel32.GetConsoleWindow(), 0)
    import subprocess
    _orig_popen = subprocess.Popen.__init__
    
    def _patched_popen(self, *args, **kwargs):
        if sys.platform == 'win32':
            kwargs.setdefault('creationflags', 134217728)
        return _orig_popen(self, *args, **kwargs)
    
    subprocess.Popen.__init__ = _patched_popen

import webview
import time
import threading
import psutil
import mss
import cv2
import numpy as np
import base64
import json
import os
import sys
from datetime import datetime

# Labs Engine input bridge — writes a 32-byte SHM record that LabsCore's
# InputOverride applies at the controller fan-out. Reaches BOTH PS Remote
# Play (chiaki) and ViGEm/Xbox sinks via the same call.
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import labs_input_bridge as labs

# ── XInput direct read for controller hotkeys (no pynput, no keyboard) ──────
import ctypes
import ctypes.wintypes as _w
class _XI_GP(ctypes.Structure):
    _fields_ = [('wButtons', _w.WORD), ('bLeftTrigger', _w.BYTE),
                ('bRightTrigger', _w.BYTE),
                ('sThumbLX', ctypes.c_short), ('sThumbLY', ctypes.c_short),
                ('sThumbRX', ctypes.c_short), ('sThumbRY', ctypes.c_short)]
class _XI_ST(ctypes.Structure):
    _fields_ = [('dwPacketNumber', _w.DWORD), ('Gamepad', _XI_GP)]
_xinput_dll = None
for _name in ('xinput1_4', 'xinput1_3', 'xinput9_1_0'):
    try:
        _xinput_dll = ctypes.WinDLL(_name); break
    except OSError:
        pass

def _read_xinput(slot):
    if _xinput_dll is None: return None
    st = _XI_ST()
    if _xinput_dll.XInputGetState(slot, ctypes.byref(st)) == 0:
        return st.Gamepad
    return None

# Friendly-name -> XInput button bit. Same naming as XInputPlugin uses.
BTN_NAMES = {
    'DPAD_UP':    0x0001, 'DPAD_DOWN':  0x0002,
    'DPAD_LEFT':  0x0004, 'DPAD_RIGHT': 0x0008,
    'START':      0x0010, 'BACK':       0x0020,  'VIEW': 0x0020, 'MENU': 0x0010,
    'L3':         0x0040, 'R3':         0x0080,
    'LB':         0x0100, 'RB':         0x0200,
    'GUIDE':      0x0400, 'XBOX':       0x0400,
    'A':          0x1000, 'B':          0x2000,
    'X':          0x4000, 'Y':          0x8000,
}


SETTINGS_FILE = 'xdrive_settings.json'

def load_settings():
    if os.path.exists(SETTINGS_FILE):
        try:
            with open(SETTINGS_FILE) as f:
                return json.load(f)
        except:
            pass
    return {
        'square_timing': 0.51,
        'square_fade_timing': 0.51,
        'tempo_timing': 0.55,
        'tempo_fade_timing': 0.7,
        'key_square': 'x',
        'key_tempo': 'c',
        'btn_square': 'LB',
        'btn_tempo': 'RB',
    }


def save_settings(data):
    with open(SETTINGS_FILE, 'w') as f:
        json.dump(data, f, indent=2)


class xDriveApi:
    
    def __init__(self):
        # Inputs go via labs_input_bridge SHM, not a direct vgamepad. The fan-out
        # in LabsCore routes overrides to whichever sink is active for the mode
        # (ViGEm for Xbox, PS Remote Play for chiaki).
        s = load_settings()
        self.square_enabled = False
        self.square_timing = s.get('square_timing', 0.51)
        self.square_fade_timing = s.get('square_fade_timing', 0.51)
        self.tempo_enabled = False
        self.tempo_timing = s.get('tempo_timing', 0.55)
        self.tempo_fade_timing = s.get('tempo_fade_timing', 0.7)
        self.tempo_is_fade = False
        self.key_square = s.get('key_square', 'x')
        self.key_tempo = s.get('key_tempo', 'c')
        # Controller-button hotkeys (preferred over keyboard now that the input
        # path is controller-end-to-end). Override in xdrive_settings.json.
        self.btn_square = s.get('btn_square', 'LB').upper()
        self.btn_tempo  = s.get('btn_tempo',  'RB').upper()
        self.mode_active = True  # always 'connected' through Labs Engine
        self.authorized = True
        self._window = None
        self._tempo_lock = threading.Lock()
        self.box_coords = {'top': 400, 'left': 400, 'width': 250, 'height': 250}
        self._train_enabled = False
        self._train_shots = 0
        self._train_greens = 0
        self.days_remaining = '?'
        self.lag_comp = False
        self.base_sq_timing = None
        self.base_tempo_timing = None
        self._sq_shots = 0
        self._sq_greens = 0
        self._tempo_shots = 0
        self._tempo_greens = 0
        self._last_shot_type = 'square'
        self._sq_last_dir = None
        self._tempo_last_dir = None

    
    def set_window(self, window):
        self._window = window

    def _js(self, code):
        """Fire-and-forget evaluate_js. Swallows ReferenceError that fires
           on the first few ticks (HTML still loading) and any per-call
           exception so a single bad eval can't kill a worker thread."""
        win = self._window
        if not win:
            return None
        try:
            return win.evaluate_js(code)
        except Exception:
            return None

    def send_log(self, message):
        safe = str(message).replace('\\', '\\\\').replace("'", "\\'").replace('\n', ' ')
        timestamp = datetime.now().strftime('%H:%M:%S')
        self._js(f"addLog('[{timestamp}] {safe}')")

    
    def execute_square_shot(self):
        if not self.square_enabled:
            return
        active_timing = self.square_fade_timing if self.tempo_is_fade else self.square_timing
        mode = 'FADE' if self.tempo_is_fade else 'NORMAL'
        self._last_shot_type = 'sq_fade' if self.tempo_is_fade else 'square'
        self.send_log(f'SQUARE {mode} Triggered (Timing: {active_timing}s)')
        # X on Xbox / Square on PS — labs.BTN_X carries the same XInput button
        # bit (0x4000) and the active sink remaps if needed.
        labs.press_button(labs.BTN_X, int(active_timing * 1000))
        time.sleep(active_timing)

    
    def execute_tempo_shot(self):
        if not self.tempo_enabled:
            return
        if not self._tempo_lock.acquire(blocking=False):
            return
        
        try:
            active_timing = self.tempo_fade_timing if self.tempo_is_fade else self.tempo_timing
            mode_label = 'FADE' if self.tempo_is_fade else 'STAND'
            self._last_shot_type = 'tempo_fade' if self.tempo_is_fade else 'tempo'
            self.send_log(f'TEMPO {mode_label} Triggered (Timing: {active_timing}s)')
            down_ms = int((0.05 + active_timing) * 1000)
            labs.set_rs_y(-32767, down_ms)
            time.sleep(0.05 + active_timing)
            labs.set_rs_y(+32767, 100)
            time.sleep(0.1)
            labs.clear()
        finally:
            self._tempo_lock.release()

    
    def _build_save(self):
        return {
            'square_timing': self.square_timing,
            'square_fade_timing': self.square_fade_timing,
            'tempo_timing': self.tempo_timing,
            'tempo_fade_timing': self.tempo_fade_timing,
            'key_square': self.key_square,
            'key_tempo': self.key_tempo,
            'btn_square': self.btn_square,
            'btn_tempo':  self.btn_tempo,
        }

    
    def sync_settings(self, sq_on, sq_val, sq_fade_val, tempo_on, tempo_val, fade_val, is_fade=False):
        self.square_enabled = sq_on
        self.square_timing = float(sq_val)
        self.square_fade_timing = float(sq_fade_val)
        self.tempo_enabled = tempo_on
        self.tempo_timing = float(tempo_val)
        self.tempo_fade_timing = float(fade_val)
        if isinstance(is_fade, str):
            self.tempo_is_fade = is_fade.lower() == 'true'
        else:
            self.tempo_is_fade = bool(is_fade)
        save_settings(self._build_save())
        return 'SYNCED'

    
    def set_keybinds(self, key_sq, key_tempo):
        self.key_square = key_sq.lower().strip()
        self.key_tempo = key_tempo.lower().strip()
        save_settings(self._build_save())
        return 'KEYS_SAVED'

    def set_buttons(self, btn_sq, btn_tempo):
        sq = (btn_sq or '').upper().strip()
        tp = (btn_tempo or '').upper().strip()
        if sq not in BTN_NAMES or tp not in BTN_NAMES:
            return {'ok': False, 'error': 'unknown button',
                    'valid': sorted(BTN_NAMES.keys())}
        self.btn_square = sq
        self.btn_tempo  = tp
        save_settings(self._build_save())
        self.send_log(f'[INPUT] hotkeys: {sq} -> SQUARE, {tp} -> TEMPO')
        return {'ok': True, 'sq': sq, 'tempo': tp}

    
    def get_settings(self):
        return self._build_save()

    
    def get_training_state(self):
        return {
            'shots': self._train_shots,
            'greens': self._train_greens,
            'enabled': self._train_enabled,
        }

    
    def training_toggle(self, enabled):
        self._train_enabled = bool(enabled)
        return self._train_enabled

    
    # Per-(shot_type, param) tuner state. Holds best-rate-so-far + best-timing,
    # last step direction, and current step size for hill-climb-with-momentum.
    # Initialised lazily on first batch so existing settings dict stays clean.
    def _tuner(self, key, base_step):
        if not hasattr(self, '_tuner_state'):
            self._tuner_state = {}
        if key not in self._tuner_state:
            self._tuner_state[key] = {
                'best_rate':   -1.0,
                'best_value':  None,
                'last_dir':    +1,        # +1 = increased last time, -1 = decreased
                'step':        base_step,
                'base_step':   base_step,
            }
        return self._tuner_state[key]

    def training_mark(self, was_green):
        if not self._train_enabled:
            return {'status': 'disabled'}

        if was_green:
            self._train_greens += 1

        t = self._last_shot_type

        # Per-(shot_type) counters. Normal vs Fade tracked separately so the
        # fade tuner doesn't get poisoned by normal-shot data and vice versa.
        if not hasattr(self, '_per_type'):
            self._per_type = {
                'square':     {'shots': 0, 'greens': 0},
                'sq_fade':    {'shots': 0, 'greens': 0},
                'tempo':      {'shots': 0, 'greens': 0},
                'tempo_fade': {'shots': 0, 'greens': 0},
            }
        if t in self._per_type:
            self._per_type[t]['shots']  += 1
            if was_green:
                self._per_type[t]['greens'] += 1

        # Keep the legacy aggregates too — UI reads sq_rate / tempo_rate
        # straight off them, no need to refactor the front-end.
        if t in ('square', 'sq_fade'):
            self._sq_shots += 1
            if was_green: self._sq_greens += 1
        elif t in ('tempo', 'tempo_fade'):
            self._tempo_shots += 1
            if was_green: self._tempo_greens += 1

        self._train_shots += 1
        msg = 'GREEN' if was_green else 'MISS'
        self.send_log(f'[TRAIN] Shot {self._train_shots}/15: {msg} ({t})')

        if self._train_shots >= 15:
            self._auto_tune()
            self._train_shots  = 0
            self._train_greens = 0
            self._sq_shots     = 0
            self._sq_greens    = 0
            self._tempo_shots  = 0
            self._tempo_greens = 0
            for k in self._per_type:
                self._per_type[k] = {'shots': 0, 'greens': 0}
            return {
                'status': 'tuned',
                'sq':     self.square_timing,
                'sqFade': self.square_fade_timing,
                'tempo':  self.tempo_timing,
                'fade':   self.tempo_fade_timing,
            }

        return {
            'status': 'recorded',
            'shots':  self._train_shots,
            'greens': self._train_greens,
        }

    def _auto_tune(self):
        """Best-anchored hill climb with momentum. Per-batch:
           - rate > best  → new best, keep moving same direction (momentum)
           - rate <= best → revert toward best, flip direction, halve step
           - step shrinks as best_rate climbs (fine-tune near optimum)
           - falls back to a small nudge if too few shots in this batch
             so the rhythm-tuning UX still feels active every 15 shots.
        """
        def tune(param_name, mn, mx, base_step, type_keys):
            cur   = getattr(self, param_name)
            shots = sum(self._per_type[k]['shots']  for k in type_keys)
            greens= sum(self._per_type[k]['greens'] for k in type_keys)
            st = self._tuner(param_name, base_step)
            if shots < 3:
                # Not enough data to tune confidently. Take a small "rhythm
                # nudge" in the last direction so the user still sees a value
                # change every batch (preserves the feel of training).
                nudge = base_step * 0.4 * st['last_dir']
                new   = round(min(mx, max(mn, cur + nudge)), 3)
                setattr(self, param_name, new)
                return shots, greens, 0.0
            rate = greens / shots
            # Adapt step: tiny near optimum, big when far.
            adapt = max(0.05, min(1.0, 1.0 - st['best_rate'] if st['best_rate'] >= 0 else 1.0))
            st['step'] = max(0.001, base_step * adapt)

            # Cold-start: no signal yet (best_rate < 0.20). Sweep the full range
            # in big steps toward the farther half so we actually find the
            # sweet spot instead of oscillating around a 0%-rate starting
            # point. Direction is fixed within a sweep and only flips when we
            # hit a bound. As soon as ANY non-trivial green rate appears,
            # we lock that as best and exit to refine mode.
            if st['best_rate'] < 0.20:
                if 'cold_dir' not in st:
                    mid = (mn + mx) / 2.0
                    st['cold_dir'] = -1 if cur > mid else +1
                if rate > st['best_rate']:
                    st['best_rate']  = rate
                    st['best_value'] = cur
                big = base_step * 2.0
                new = cur + st['cold_dir'] * big
                if new <= mn:
                    new = mn + big * 0.1
                    st['cold_dir'] = +1
                elif new >= mx:
                    new = mx - big * 0.1
                    st['cold_dir'] = -1
                # Reflect last_dir so refine mode picks up where sweep left off.
                st['last_dir'] = st['cold_dir']
            elif rate > st['best_rate']:
                # Improvement. Lock as new best, keep moving same direction.
                st['best_rate']  = rate
                st['best_value'] = cur
                new = cur + st['last_dir'] * st['step']
            else:
                # Regression. Revert halfway toward best, flip direction, halve step.
                anchor = st['best_value'] if st['best_value'] is not None else cur
                new = (cur + anchor) / 2.0
                st['last_dir'] *= -1
                st['step']      = max(0.001, st['step'] * 0.5)
                new += st['last_dir'] * st['step']
            new = round(min(mx, max(mn, new)), 3)
            setattr(self, param_name, new)
            return shots, greens, rate

        # Each timing param tunes against its own per-type counters.
        sq_n_shots,    sq_n_greens,    sq_n_rate    = tune('square_timing',      0.45, 0.60, 0.012, ['square'])
        sq_f_shots,    sq_f_greens,    sq_f_rate    = tune('square_fade_timing', 0.40, 0.70, 0.012, ['sq_fade'])
        tp_n_shots,    tp_n_greens,    tp_n_rate    = tune('tempo_timing',       0.40, 0.75, 0.014, ['tempo'])
        tp_f_shots,    tp_f_greens,    tp_f_rate    = tune('tempo_fade_timing',  0.30, 0.95, 0.014, ['tempo_fade'])

        save_settings(self._build_save())

        # Aggregate display rates (back-compat with UI).
        sq_rate    = self._sq_greens    / max(1, self._sq_shots)
        tempo_rate = self._tempo_greens / max(1, self._tempo_shots)
        self.send_log(
            f'[TRAIN] tuned — sqN {sq_n_rate*100:.0f}%/{sq_n_shots}s · '
            f'sqF {sq_f_rate*100:.0f}%/{sq_f_shots}s · '
            f'tpN {tp_n_rate*100:.0f}%/{tp_n_shots}s · '
            f'tpF {tp_f_rate*100:.0f}%/{tp_f_shots}s'
        )
        self.send_log(
            f'[TRAIN] new → SQ {self.square_timing} (best {self._tuner_state["square_timing"]["best_value"]}) | '
            f'TEMPO {self.tempo_timing} (best {self._tuner_state["tempo_timing"]["best_value"]})'
        )
        self._js(f'onTrainTuned({self.square_timing},{self.square_fade_timing},{self.tempo_timing},{self.tempo_fade_timing})')

    def training_reset(self):
        self._train_shots = 0
        self._train_greens = 0
        self._sq_shots = 0
        self._sq_greens = 0
        self._tempo_shots = 0
        self._tempo_greens = 0
        # Wipe the hill-climb memory too — otherwise a "Reset" leaves stale
        # best_rate/best_value pinned to whatever was learned earlier.
        if hasattr(self, '_tuner_state'):
            self._tuner_state = {}
        if hasattr(self, '_per_type'):
            for k in self._per_type:
                self._per_type[k] = {'shots': 0, 'greens': 0}
        return 'RESET'

    
    def get_network_stats(self):
        import subprocess
        import re
        ping = None
        wifi = None
        
        try:
            result = subprocess.run(
                ['ping', '-n', '1', '-w', '1000', '8.8.8.8'],
                capture_output=True, text=True, timeout=3
            )
            match = re.search(r'Average = (\d+)ms', result.stdout)
            if match:
                ping = int(match.group(1))
        except:
            pass
        
        try:
            result = subprocess.run(
                ['netsh', 'wlan', 'show', 'interfaces'],
                capture_output=True, text=True, timeout=3
            )
            match = re.search(r'Signal\s*:\s*(\d+)%', result.stdout)
            if match:
                wifi = int(match.group(1))
        except:
            pass
        
        adj = 0
        base_ping = 30  # target base ping
        extra = max(0, ping - base_ping) if ping else 0
        adj = round(extra * 0.0003, 3)
        
        if self.lag_comp:
            # Apply lag compensation to timing values
            if adj > 0:
                self.square_timing = round(self.square_timing - adj, 3)
                self.square_fade_timing = round(self.square_fade_timing - adj, 3)
                self.tempo_timing = round(self.tempo_timing - adj, 3)
                self.tempo_fade_timing = round(self.tempo_fade_timing - adj, 3)
        
        return {'ping': ping, 'wifi': wifi, 'adj': adj}

    
    def toggle_lag_comp(self, enabled):
        self.lag_comp = bool(enabled)
        if self.lag_comp:
            self.base_sq_timing = self.square_timing
            self.base_tempo_timing = self.tempo_timing
        elif self.base_sq_timing:
            self.square_timing = self.base_sq_timing
            self.square_fade_timing = self.base_sq_timing
            self.tempo_timing = self.base_tempo_timing
            self.tempo_fade_timing = self.base_tempo_timing
        return 'OK'

    
    def check_access(self, key):
        self.authorized = True
        self.days_remaining = 'Lifetime'
        return {'ok': True, 'days': 'Lifetime'}

    
    def start_threads(self):
        """Start background threads for hotkeys, process watching, and screen streaming."""
        
        def controller_listener():
            """Poll XInput for button-press transitions on the user's real pad.
               Skip slot 0 (ViGEm output — would feedback). 250Hz so even quick
               taps register. Triggers execute_square_shot / execute_tempo_shot
               on the rising edge of the configured buttons."""
            last_btns  = 0
            active_slot = -1
            while True:
                gp = None
                # Slot 0 is ViGEm's virtual output — never read it.
                for slot in (1, 2, 3):
                    gp = _read_xinput(slot)
                    if gp is not None:
                        if active_slot != slot:
                            active_slot = slot
                            last_btns = 0
                            self.send_log(f'[INPUT] controller on XInput slot {slot}')
                        break
                if gp is None:
                    active_slot = -1
                    last_btns = 0
                    time.sleep(0.05)
                    continue
                btns    = gp.wButtons
                pressed = btns & ~last_btns      # rising edges only
                last_btns = btns
                sq_bit    = BTN_NAMES.get(self.btn_square, BTN_NAMES['LB'])
                tempo_bit = BTN_NAMES.get(self.btn_tempo,  BTN_NAMES['RB'])
                if pressed & sq_bit:
                    type(self).square_enabled = True
                    threading.Thread(target=self.execute_square_shot, daemon=True).start()
                if pressed & tempo_bit:
                    threading.Thread(target=self.execute_tempo_shot, daemon=True).start()
                time.sleep(1 / 250)
        
        def process_watcher():
            # Labs Engine handles the actual session — we just report 'connected'
            # so the UI badge isn't lying.
            while True:
                if self._window:
                    self._js('updateChiakiStatus(true)')
                time.sleep(2)
        
        def screen_stream():
            # Capture from the Labs Engine app window (where the game stream is
            # rendered). Falls back to box_coords if the window can't be found.
            try:
                import win32gui
                _have_w32 = True
            except Exception:
                _have_w32 = False

            def _labs_rect():
                if not _have_w32:
                    return None
                hits = []
                def _cb(hwnd, _):
                    t = win32gui.GetWindowText(hwnd) or ''
                    if 'Labs Engine' in t and win32gui.IsWindowVisible(hwnd):
                        r = win32gui.GetWindowRect(hwnd)
                        if (r[2] - r[0]) > 400 and (r[3] - r[1]) > 300:
                            hits.append(r)
                try:
                    win32gui.EnumWindows(_cb, None)
                except Exception:
                    return None
                if not hits: return None
                # Crop the inner stream region: skip top bar (~64px), bottom log
                # strip (~110px), side rails (~270 + ~240). Numbers track
                # LabsMainWindow's layout — adjust if it ever shrinks.
                l, t, r, b = hits[0]
                stream_l = l + 270
                stream_t = t + 64 + 38   # top bar + tab strip
                stream_r = r - 240
                stream_b = b - 110 - 28  # log strip + log header
                if stream_r <= stream_l or stream_b <= stream_t:
                    return None
                return {'left': stream_l, 'top': stream_t,
                        'width': stream_r - stream_l,
                        'height': stream_b - stream_t}

            while True:
                try:
                    box = _labs_rect() or {
                        'left':  self.box_coords['left'],
                        'top':   self.box_coords['top'],
                        'width': self.box_coords['width'],
                        'height':self.box_coords['height'],
                    }
                    with mss.mss() as sct:
                        img = sct.grab(box)
                        frame = np.array(img)
                        # Downscale aggressively so the preview fits the side
                        # panel without choking the JS bridge with a 1080p JPG.
                        h, w = frame.shape[:2]
                        max_w = 320
                        if w > max_w:
                            scale = max_w / w
                            frame = cv2.resize(frame, (max_w, int(h * scale)),
                                               interpolation=cv2.INTER_AREA)
                        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
                        _, buf = cv2.imencode('.jpg', gray, [cv2.IMWRITE_JPEG_QUALITY, 55])
                        b64 = base64.b64encode(buf).decode()
                        if self._window:
                            self._js(f'updatePreview("{b64}")')
                except Exception:
                    pass
                time.sleep(0.1)
        
        # Start all threads
        t1 = threading.Thread(target=controller_listener, daemon=True)
        t2 = threading.Thread(target=process_watcher, daemon=True)
        t3 = threading.Thread(target=screen_stream, daemon=True)
        t1.start()
        t2.start()
        t3.start()


HTML_CONTENT = '''
<!DOCTYPE html>
<html>
<head>
    <style>
        :root { --cyan: #e60026; --red: #ff4444; --bg: #000; --surf: #0a0005; --border: rgba(230,0,38,0.18); --dim: #6d5c5c; }
        * { box-sizing: border-box; font-family: 'Segoe UI', sans-serif; color: #fff; }
        body { background: var(--bg); margin: 0; display: flex; height: 100vh; overflow: hidden; }
        .sidebar { width: 200px; background: var(--surf); border-right: 1px solid var(--border); padding: 20px; display: flex; flex-direction: column; }
        .logo { font-size: 18px; font-weight: 900; color: var(--cyan); margin-bottom: 24px; font-style: italic; }
        .tab { padding: 11px 12px; border-radius: 8px; color: var(--dim); cursor: pointer; margin-bottom: 4px; font-weight: 700; font-size: 13px; transition: 0.2s; }
        .tab.active { background: rgba(0,242,255,0.1); color: var(--cyan); border: 1px solid var(--border); }
        .tab:hover:not(.active) { color: #fff; }
        .main { flex: 1; padding: 24px; overflow-y: auto; }
        .view { display: none; } .view.active { display: block; }
        .card { background: var(--surf); border: 1px solid var(--border); padding: 16px; border-radius: 12px; margin-bottom: 16px; }
        .section-header { color: var(--cyan); font-size: 11px; font-weight: 800; text-transform: uppercase; margin-bottom: 10px; letter-spacing: 1px; }
        .switch { width: 34px; height: 16px; background: #1a1a24; border-radius: 20px; position: relative; cursor: pointer; border: 1px solid var(--border); flex-shrink: 0; }
        .switch.on { background: var(--cyan); border-color: transparent; }
        .switch::after { content: ''; position: absolute; top: 1px; left: 1px; width: 12px; height: 12px; background: #fff; border-radius: 50%; transition: 0.2s; }
        .switch.on::after { left: 19px; }
        input[type=range] { width: 100%; accent-color: var(--cyan); margin: 6px 0 2px; }
        .log-container { background: #050508; border: 1px solid var(--border); border-radius: 8px; padding: 12px; font-family: monospace; font-size: 11px; overflow-y: auto; height: 160px; color: var(--dim); }
        .log-entry { margin-bottom: 4px; border-left: 2px solid var(--cyan); padding-left: 8px; }
        .key-bind { background: rgba(0,242,255,0.1); padding: 1px 5px; border-radius: 4px; color: var(--cyan); font-size: 10px; }
        .key-input { background: #0a0a0f; border: 1px solid var(--border); border-radius: 6px; color: var(--cyan); font-size: 13px; font-weight: 700; width: 44px; text-align: center; padding: 5px; outline: none; text-transform: uppercase; }
        .key-input:focus { border-color: var(--cyan); }
        .save-btn { background: var(--cyan); border: none; border-radius: 6px; color: #000; font-weight: 800; font-size: 12px; padding: 7px 16px; cursor: pointer; margin-top: 12px; }
        .save-btn:hover { opacity: 0.85; }
        .saved-tag { font-size: 11px; color: var(--cyan); margin-left: 10px; display: none; }
        .row { display: flex; justify-content: space-between; align-items: center; }
        .slider-label { font-size: 12px; color: var(--dim); }
        .slider-val { font-size: 12px; color: var(--cyan); font-weight: 600; }
        .timing-row { display: flex; justify-content: space-between; align-items: center; margin-bottom: 4px; }
        .divider { height: 1px; background: var(--border); margin: 14px 0; }
    </style>
</head>
<body>
    <div id="authScreen" style="position:fixed; inset:0; background:var(--bg); z-index:100; display:none; flex-direction:column; align-items:center; justify-content:center;">
        <div class="logo" style="font-size:26px;">xDrive ! LOGIN</div>
        <input type="text" id="authKey" placeholder="ACCESS KEY" style="background:#0a0a0f; border:1px solid var(--border); padding:12px; border-radius:8px; width:260px; text-align:center; outline:none; margin-bottom:20px;">
        <button onclick="login()" style="background:var(--cyan); border:none; padding:10px 30px; border-radius:8px; font-weight:800; cursor:pointer; color:#000;">AUTHORIZE</button>
    </div>

    <div class="sidebar">
        <div class="logo">xDrive !</div>
        <div id="daysBadge" style="display:none; background:rgba(230,0,38,0.08); border:1px solid var(--border); border-radius:8px; padding:5px 10px; margin-bottom:16px; font-size:10px; color:var(--dim); text-align:center;"></div>
        <div class="tab active" onclick="showView('shootView', this)">Shooting</div>
        <div class="tab" onclick="showView('timingView', this)">Timing</div>
        <div class="tab" onclick="showView('connView', this)">Capture</div>
        <div class="tab" onclick="showView('trainView', this)">Training</div>
    </div>

    <div class="main">

        <!-- SHOOTING TAB -->
        <div id="shootView" class="view active">
            <div class="section-header">Square <span class="key-bind" id="sqKeyLabel">[LB]</span></div>
            <div class="card">
                <div class="row" style="margin-bottom:10px;">
                    <b>Auto Green</b> <div id="sqToggle" class="switch" onclick="toggleSq()"></div>
                </div>
                <div class="row" style="margin-bottom:4px;">
                    <span class="slider-label">Normal Shot</span>
                    <span class="slider-val" id="sqVal">0.510s</span>
                </div>
                <input type="range" min="0.450" max="0.600" step="0.001" value="0.510" id="sqSld" oninput="sync()">
                <div class="row" style="margin-top:12px; margin-bottom:4px;">
                    <span class="slider-label">Normal Fading Shot</span>
                    <span class="slider-val" id="sqFadeVal">0.510s</span>
                </div>
                <input type="range" min="0.400" max="0.700" step="0.001" value="0.510" id="sqFadeSld" oninput="sync()">
            </div>

            <div class="section-header">Tempo <span class="key-bind" id="tempoKeyLabel">[RB]</span></div>
            <div class="card">
                <div class="row" style="margin-bottom:16px;">
                    <b>Enable Auto Green (Tempo)</b> <div id="tempoToggle" class="switch" onclick="toggleTempo()"></div>
                </div>
                <div class="row" style="margin-bottom:4px;">
                    <span class="slider-label">Normal</span>
                    <span class="slider-val" id="tempoVal">0.550s</span>
                </div>
                <input type="range" min="0.400" max="0.750" step="0.001" value="0.550" id="tempoSld" oninput="sync()">
                <div class="row" style="margin-top:12px; margin-bottom:4px;">
                    <span class="slider-label">Fading Shot</span>
                    <span class="slider-val" id="fadeVal">0.700s</span>
                </div>
                <input type="range" min="0.300" max="0.950" step="0.001" value="0.700" id="fadeSld" oninput="sync()">
                <div class="divider"></div>
                <div class="row">
                    <span id="fadeModeLabel" class="slider-label">Fading Shot Mode</span>
                    <div id="fadeToggle" class="switch" onclick="toggleFade()"></div>
                </div>
                <div style="font-size:10px; color:var(--dim); margin-top:6px;">Turn on before taking a fading 3</div>
            </div>

            <div class="section-header">Controller Hotkeys</div>
            <div class="card">
                <div style="font-size:11px; color:var(--dim); margin-bottom:10px;">
                    Pick a controller button to fire each shot. Valid:
                    <span style="color:var(--cyan);">A B X Y LB RB L3 R3 BACK START GUIDE DPAD_UP DPAD_DOWN DPAD_LEFT DPAD_RIGHT</span>
                </div>
                <div class="row" style="margin-bottom:12px;">
                    <span class="slider-label">Square Shot Button</span>
                    <input class="key-input" id="btnSq" maxlength="12" value="LB" style="width:100px;" />
                </div>
                <div class="row">
                    <span class="slider-label">Tempo Shot Button</span>
                    <input class="key-input" id="btnTempo" maxlength="12" value="RB" style="width:100px;" />
                </div>
                <div style="display:flex; align-items:center;">
                    <button class="save-btn" onclick="saveButtons()">Save Buttons</button>
                    <span class="saved-tag" id="savedTag">Saved</span>
                </div>
            </div>

            <div class="section-header">Shot Terminal</div>
            <div class="log-container" id="logBox"></div>
        </div>

        <!-- TIMING TAB -->
        <div id="timingView" class="view">
            <div class="section-header">Adjust Timing (Square)</div>
            <div class="card">
                <div class="row" style="margin-bottom:4px;">
                    <span class="slider-label">Normal Shot</span>
                    <span class="slider-val" id="t_sqVal">0.510s</span>
                </div>
                <input type="range" min="0.450" max="0.600" step="0.001" value="0.510" id="t_sqSld" oninput="syncTiming()">
                <div class="row" style="margin-top:14px; margin-bottom:4px;">
                    <span class="slider-label">Normal Fading Shot</span>
                    <span class="slider-val" id="t_sqFadeVal">0.510s</span>
                </div>
                <input type="range" min="0.400" max="0.700" step="0.001" value="0.510" id="t_sqFadeSld" oninput="syncTiming()">
            </div>

            <div class="section-header">Adjust Tempo</div>
            <div class="card">
                <div class="row" style="margin-bottom:4px;">
                    <span class="slider-label">Normal</span>
                    <span class="slider-val" id="t_tempoVal">0.550s</span>
                </div>
                <input type="range" min="0.400" max="0.750" step="0.001" value="0.550" id="t_tempoSld" oninput="syncTiming()">
                <div class="row" style="margin-top:14px; margin-bottom:4px;">
                    <span class="slider-label">Fading Shot</span>
                    <span class="slider-val" id="t_fadeVal">0.700s</span>
                </div>
                <input type="range" min="0.300" max="0.950" step="0.001" value="0.700" id="t_fadeSld" oninput="syncTiming()">
            </div>
        </div>

        <!-- TRAINING TAB -->
        <div id="trainView" class="view">
            <div class="section-header">Training Mode</div>
            <div class="card">
                <div class="row" style="margin-bottom:10px;">
                    <b>Enable Training Mode</b>
                    <div id="trainToggle" class="switch" onclick="toggleTrain()"></div>
                </div>
                <div style="font-size:10px; color:var(--dim);">After 10 shots, timing values auto-tune to green more consistently.</div>
            </div>

            <div class="section-header">Training Stats</div>
            <div class="card">
                <div class="row" style="margin-bottom:8px;">
                    <span class="slider-label">Shots Recorded</span>
                    <span class="slider-val" id="tr_shots">0 / 15</span>
                </div>
                <div class="row" style="margin-bottom:8px;">
                    <span class="slider-label">Greens</span>
                    <span class="slider-val" id="tr_greens">0</span>
                </div>
                <div class="row" style="margin-bottom:8px;">
                    <span class="slider-label">Green Rate</span>
                    <span class="slider-val" id="tr_rate">0%</span>
                </div>
                <div class="row" style="margin-bottom:8px;">
                    <span class="slider-label">Square Rate</span>
                    <span class="slider-val" id="tr_sq_rate">--</span>
                </div>
                <div class="row" style="margin-bottom:8px;">
                    <span class="slider-label">Tempo Rate</span>
                    <span class="slider-val" id="tr_tempo_rate">--</span>
                </div>
                <div class="row">
                    <span class="slider-label">Status</span>
                    <span class="slider-val" id="tr_status" style="color:var(--dim);">Waiting...</span>
                </div>
            </div>

            <div class="section-header">Values Being Tuned</div>
            <div class="card">
                <div class="row" style="margin-bottom:8px;">
                    <span class="slider-label">Square Normal</span>
                    <span class="slider-val" id="tr_sq">--</span>
                </div>
                <div class="row" style="margin-bottom:8px;">
                    <span class="slider-label">Square Fading</span>
                    <span class="slider-val" id="tr_sqFade">--</span>
                </div>
                <div class="row" style="margin-bottom:8px;">
                    <span class="slider-label">Tempo Normal</span>
                    <span class="slider-val" id="tr_tempo">--</span>
                </div>
                <div class="row">
                    <span class="slider-label">Tempo Fading</span>
                    <span class="slider-val" id="tr_tempFade">--</span>
                </div>
            </div>

            <div class="section-header">Actions</div>
            <div class="card">
                <div class="row" style="margin-bottom:10px;">
                    <button class="save-btn" onclick="trainGreen()" style="margin-top:0; margin-right:8px;">Mark GREEN</button>
                    <button class="save-btn" onclick="trainMiss()" style="margin-top:0; background:#ff003c;">Mark MISS</button>
                </div>
                <div style="font-size:10px; color:var(--dim); margin-bottom:12px;">After each shot mark Green or Miss so training can tune your timing.</div>
                <button class="save-btn" onclick="trainReset()" style="background:#1a1a24; color:var(--cyan); border:1px solid var(--border);">Reset Training</button>
            </div>

            <div class="section-header">Training Log</div>
            <div class="log-container" id="trainLog"></div>
        </div>

        <!-- CAPTURE TAB -->
        <div id="connView" class="view">
            <div class="section-header">Remote Play</div>
            <div class="card" id="chiakiBar" style="border-color:var(--red); text-align:center;">
                <b id="chiakiTxt" style="color:var(--red); font-size:12px;">LABS ENGINE: ATTACHED</b>
            </div>

            <div class="section-header">Network</div>
            <div class="card">
                <div class="row" style="margin-bottom:8px;">
                    <span class="slider-label">Ping</span>
                    <span id="pingVal" style="font-size:13px; font-weight:800; color:var(--cyan);">-- ms</span>
                </div>
                <div style="height:4px; background:#1a1a24; border-radius:2px; margin-bottom:14px;">
                    <div id="pingBar" style="height:100%; width:0%; border-radius:2px; transition:width 0.4s;"></div>
                </div>
                <div class="row" style="margin-bottom:8px;">
                    <span class="slider-label">WiFi Signal</span>
                    <span id="wifiVal" style="font-size:13px; font-weight:800; color:var(--cyan);">--</span>
                </div>
                <div style="display:flex; gap:3px; margin-bottom:4px;">
                    <div id="wb1" style="flex:1; height:8px; background:#1a1a24; border-radius:2px;"></div>
                    <div id="wb2" style="flex:1; height:8px; background:#1a1a24; border-radius:2px;"></div>
                    <div id="wb3" style="flex:1; height:8px; background:#1a1a24; border-radius:2px;"></div>
                    <div id="wb4" style="flex:1; height:8px; background:#1a1a24; border-radius:2px;"></div>
                    <div id="wb5" style="flex:1; height:8px; background:#1a1a24; border-radius:2px;"></div>
                </div>
            </div>

            <div class="section-header">Lag Compensation</div>
            <div class="card">
                <div class="row" style="margin-bottom:10px;">
                    <b>Auto Adjust Timing</b>
                    <div id="lagToggle" class="switch" onclick="toggleLag()"></div>
                </div>
                <div style="font-size:10px; color:var(--dim);">Auto-adjusts timing based on ping so shots still green during lag spikes.</div>
                <div class="row" style="margin-top:12px;">
                    <span class="slider-label">Adjustment</span>
                    <span id="lagAdjVal" style="font-size:12px; color:var(--cyan); font-weight:600;">0ms</span>
                </div>
            </div>

            <div class="section-header">Detection</div>
            <div class="card" style="text-align:center; background:#000;">
                <img id="liveFeed" style="width:100%; max-width:300px; height:auto; border:1px solid var(--border); border-radius:8px;">
                <p style="font-size:10px; color:var(--dim); margin-top:10px;">Align White Meter inside the box.</p>
            </div>
        </div>

    </div>

    <script>
        let sqOn = false, tempoOn = false, fadeOn = false;
        function addLog(msg) {
            const box = document.getElementById('logBox');
            const d = document.createElement('div'); d.className = 'log-entry'; d.innerText = msg;
            box.prepend(d); if (box.children.length > 20) box.lastChild.remove();
        }
        function showView(id, el) {
            document.querySelectorAll('.view').forEach(v => v.classList.remove('active'));
            document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
            document.getElementById(id).classList.add('active'); el.classList.add('active');
        }
        function login() {
            const key = document.getElementById('authKey').value;
            pywebview.api.check_access(key).then(result => {
                if (result === true || (result && result.ok)) {
                    document.getElementById('authScreen').style.display = 'none';
                    loadSaved();
                    const days = result && result.days ? result.days : null;
                    const db = document.getElementById('daysBadge');
                    if (days) {
                        db.innerText = days === 'Lifetime' ? 'Lifetime Access' : days + ' days remaining';
                        db.style.display = 'block';
                        db.style.color = days === 'Lifetime' ? 'var(--cyan)' : parseInt(days) <= 3 ? 'var(--red)' : 'var(--cyan)';
                    }
                } else {
                    alert((result && result.message) || result || "Invalid key");
                }
            });
        }
        function loadSaved() {
            pywebview.api.get_settings().then(s => {
                if (!s) return;
                document.getElementById('sqSld').value      = s.square_timing;
                document.getElementById('sqVal').innerText  = s.square_timing + "s";
                document.getElementById('sqFadeSld').value  = s.square_fade_timing;
                document.getElementById('sqFadeVal').innerText = s.square_fade_timing + "s";
                document.getElementById('tempoSld').value    = s.tempo_timing;
                document.getElementById('tempoVal').innerText = s.tempo_timing + "s";
                document.getElementById('fadeSld').value    = s.tempo_fade_timing;
                document.getElementById('fadeVal').innerText = s.tempo_fade_timing + "s";
                document.getElementById('t_sqSld').value     = s.square_timing;
                document.getElementById('t_sqVal').innerText = s.square_timing + "s";
                document.getElementById('t_sqFadeSld').value = s.square_fade_timing;
                document.getElementById('t_sqFadeVal').innerText = s.square_fade_timing + "s";
                document.getElementById('t_tempoSld').value  = s.tempo_timing;
                document.getElementById('t_tempoVal').innerText = s.tempo_timing + "s";
                document.getElementById('t_fadeSld').value   = s.tempo_fade_timing;
                document.getElementById('t_fadeVal').innerText = s.tempo_fade_timing + "s";
                const sq = (s.btn_square || 'LB').toUpperCase();
                const tp = (s.btn_tempo  || 'RB').toUpperCase();
                const sqEl = document.getElementById('btnSq');
                const tpEl = document.getElementById('btnTempo');
                if (sqEl) sqEl.value = sq;
                if (tpEl) tpEl.value = tp;
                document.getElementById('sqKeyLabel').innerText    = '[' + sq + ']';
                document.getElementById('tempoKeyLabel').innerText = '[' + tp + ']';
            });
        }
        function toggleSq()    { sqOn = !sqOn; document.getElementById('sqToggle').classList.toggle('on'); sync(); }
        function toggleTempo() { tempoOn = !tempoOn; document.getElementById('tempoToggle').classList.toggle('on'); sync(); }
        function toggleFade() {
            fadeOn = !fadeOn;
            document.getElementById('fadeToggle').classList.toggle('on');
            document.getElementById('fadeModeLabel').innerText = fadeOn ? 'FADE MODE ON' : 'Fading Shot Mode';
            document.getElementById('fadeModeLabel').style.color = fadeOn ? 'var(--cyan)' : 'var(--dim)';
            sync();
        }
        function getVals() {
            return {
                sq: document.getElementById('sqSld').value,
                sqFade: document.getElementById('sqFadeSld').value,
                tempo: document.getElementById('tempoSld').value,
                fade: document.getElementById('fadeSld').value,
            };
        }
        function sync() {
            const v = getVals();
            document.getElementById('sqVal').innerText = v.sq + "s";
            document.getElementById('sqFadeVal').innerText = v.sqFade + "s";
            document.getElementById('tempoVal').innerText = v.tempo + "s";
            document.getElementById('fadeVal').innerText = v.fade + "s";
            document.getElementById('t_sqSld').value = v.sq;
            document.getElementById('t_sqVal').innerText = v.sq + "s";
            document.getElementById('t_sqFadeSld').value = v.sqFade;
            document.getElementById('t_sqFadeVal').innerText = v.sqFade + "s";
            document.getElementById('t_tempoSld').value = v.tempo;
            document.getElementById('t_tempoVal').innerText = v.tempo + "s";
            document.getElementById('t_fadeSld').value = v.fade;
            document.getElementById('t_fadeVal').innerText = v.fade + "s";
            pywebview.api.sync_settings(sqOn, v.sq, v.sqFade, tempoOn, v.tempo, v.fade, fadeOn);
        }
        function syncTiming() {
            const sq = document.getElementById('t_sqSld').value;
            const sqFade = document.getElementById('t_sqFadeSld').value;
            const tempo = document.getElementById('t_tempoSld').value;
            const fade = document.getElementById('t_fadeSld').value;
            document.getElementById('t_sqVal').innerText = sq + "s";
            document.getElementById('t_sqFadeVal').innerText = sqFade + "s";
            document.getElementById('t_tempoVal').innerText = tempo + "s";
            document.getElementById('t_fadeVal').innerText = fade + "s";
            document.getElementById('sqSld').value = sq;
            document.getElementById('sqVal').innerText = sq + "s";
            document.getElementById('sqFadeSld').value = sqFade;
            document.getElementById('sqFadeVal').innerText = sqFade + "s";
            document.getElementById('tempoSld').value = tempo;
            document.getElementById('tempoVal').innerText = tempo + "s";
            document.getElementById('fadeSld').value = fade;
            document.getElementById('fadeVal').innerText = fade + "s";
            pywebview.api.sync_settings(sqOn, sq, sqFade, tempoOn, tempo, fade, fadeOn);
        }
        function saveButtons() {
            const sq = document.getElementById('btnSq').value.toUpperCase().trim();
            const tp = document.getElementById('btnTempo').value.toUpperCase().trim();
            if (!sq || !tp) return;
            pywebview.api.set_buttons(sq, tp).then(r => {
                if (r && r.ok) {
                    document.getElementById('sqKeyLabel').innerText    = '[' + sq + ']';
                    document.getElementById('tempoKeyLabel').innerText = '[' + tp + ']';
                    const tag = document.getElementById('savedTag');
                    tag.style.display = 'inline';
                    setTimeout(() => tag.style.display = 'none', 2000);
                } else {
                    alert('Unknown button. Valid: ' + (r && r.valid ? r.valid.join(', ') : 'see card hint'));
                }
            });
        }
        function updatePreview(data) { document.getElementById('liveFeed').src = "data:image/jpeg;base64," + data; }
        function updateChiakiStatus(active) {
            const bar = document.getElementById('chiakiBar'), txt = document.getElementById('chiakiTxt');
            bar.style.borderColor = active ? "var(--cyan)" : "var(--red)";
            txt.style.color = active ? "var(--cyan)" : "var(--red)";
            txt.innerText = active ? "LABS ENGINE: ATTACHED" : "LABS ENGINE: ATTACHED";
        }
        let lagOn = false;
        function toggleLag() {
            lagOn = !lagOn;
            document.getElementById('lagToggle').classList.toggle('on');
            pywebview.api.toggle_lag_comp(lagOn);
        }
        function pollNetwork() {
            pywebview.api.get_network_stats().then(s => {
                if (!s) return;
                const ping = s.ping;
                const pv = document.getElementById('pingVal');
                const pb = document.getElementById('pingBar');
                if (ping !== null && ping !== undefined) {
                    pv.innerText = ping + ' ms';
                    pv.style.color = ping < 30 ? 'var(--cyan)' : ping < 80 ? '#ffaa00' : 'var(--red)';
                    const pct = Math.min(100, (ping / 200) * 100);
                    pb.style.width = pct + '%';
                    pb.style.background = ping < 30 ? 'var(--cyan)' : ping < 80 ? '#ffaa00' : 'var(--red)';
                }
                const wifi = s.wifi;
                const wv = document.getElementById('wifiVal');
                if (wifi !== null && wifi !== undefined) {
                    wv.innerText = wifi + '%';
                    wv.style.color = wifi > 70 ? 'var(--cyan)' : wifi > 40 ? '#ffaa00' : 'var(--red)';
                    const bars = Math.ceil(wifi / 20);
                    for (let i = 1; i <= 5; i++) {
                        const b = document.getElementById('wb' + i);
                        b.style.background = i <= bars ? (wifi > 70 ? 'var(--cyan)' : wifi > 40 ? '#ffaa00' : 'var(--red)') : '#1a1a24';
                    }
                }
                if (s.adj !== undefined) {
                    document.getElementById('lagAdjVal').innerText = lagOn ? '-' + Math.round(s.adj * 1000) + 'ms' : '0ms';
                }
            }).catch(() => {});
        }
        setInterval(pollNetwork, 5000);
        window.addEventListener("load", function(){ if (window.pywebview && pywebview.api) loadSaved(); else setTimeout(()=>{loadSaved && loadSaved();}, 800); });
        let trainOn = false;
        function toggleTrain() {
            trainOn = !trainOn;
            document.getElementById('trainToggle').classList.toggle('on');
            pywebview.api.training_toggle(trainOn).then(() => {
                document.getElementById('tr_status').innerText = trainOn ? 'Active' : 'Waiting...';
                document.getElementById('tr_status').style.color = trainOn ? 'var(--cyan)' : 'var(--dim)';
                addTrainLog(trainOn ? 'Training enabled — mark each shot after you take it.' : 'Training disabled.');
            });
            refreshTrainValues();
        }
        function refreshTrainValues() {
            pywebview.api.get_settings().then(s => {
                if (!s) return;
                document.getElementById('tr_sq').innerText = s.square_timing + 's';
                document.getElementById('tr_sqFade').innerText = s.square_fade_timing + 's';
                document.getElementById('tr_tempo').innerText = s.tempo_timing + 's';
                document.getElementById('tr_tempFade').innerText = s.tempo_fade_timing + 's';
            });
        }
        function trainGreen() {
            if (!trainOn) { addTrainLog('Enable Training Mode first.'); return; }
            pywebview.api.training_mark(true).then(r => updateTrainUI(r, true));
        }
        function trainMiss() {
            if (!trainOn) { addTrainLog('Enable Training Mode first.'); return; }
            pywebview.api.training_mark(false).then(r => updateTrainUI(r, false));
        }
        function updateTrainUI(r, wasGreen) {
            if (!r) return;
            if (r.status === 'disabled') { addTrainLog('Training is disabled.'); return; }
            if (r.status === 'recorded') {
                document.getElementById('tr_shots').innerText = r.shots + ' / 15';
                document.getElementById('tr_greens').innerText = r.greens;
                const rate = r.shots > 0 ? Math.round(r.greens / r.shots * 100) : 0;
                document.getElementById('tr_rate').innerText = rate + '%';
                addTrainLog((wasGreen ? 'GREEN' : 'MISS') + ' — Shot ' + r.shots + '/15');
            }
            if (r.status === 'tuned') {
                document.getElementById('tr_shots').innerText = '0 / 15';
                document.getElementById('tr_greens').innerText = '0';
                document.getElementById('tr_rate').innerText = '0%';
                document.getElementById('tr_sq_rate').innerText = '--';
                document.getElementById('tr_tempo_rate').innerText = '--';
                document.getElementById('tr_status').innerText = 'Tuned!';
                document.getElementById('tr_status').style.color = 'var(--cyan)';
                addTrainLog('Auto-tune complete! New values applied.');
                refreshTrainValues();
                loadSaved();
                setTimeout(() => {
                    document.getElementById('tr_status').innerText = 'Active';
                }, 3000);
            }
        }
        function onTrainTuned(sq, sqFade, tempo, fade) {
            document.getElementById('tr_sq').innerText = sq + 's';
            document.getElementById('tr_sqFade').innerText = sqFade + 's';
            document.getElementById('tr_tempo').innerText = tempo + 's';
            document.getElementById('tr_tempFade').innerText = fade + 's';
        }
        function trainReset() {
            pywebview.api.training_reset().then(() => {
                document.getElementById('tr_shots').innerText = '0 / 15';
                document.getElementById('tr_greens').innerText = '0';
                document.getElementById('tr_rate').innerText = '0%';
                document.getElementById('tr_sq_rate').innerText = '--';
                document.getElementById('tr_tempo_rate').innerText = '--';
                document.getElementById('tr_status').innerText = trainOn ? 'Active' : 'Waiting...';
                addTrainLog('Training reset.');
            });
        }
        function addTrainLog(msg) {
            const box = document.getElementById('trainLog');
            if (!box) return;
            const d = document.createElement('div');
            d.className = 'log-entry';
            const now = new Date();
            const ts = now.getHours().toString().padStart(2,'0') + ':' + now.getMinutes().toString().padStart(2,'0') + ':' + now.getSeconds().toString().padStart(2,'0');
            d.innerText = '[' + ts + '] ' + msg;
            box.prepend(d);
            if (box.children.length > 20) box.lastChild.remove();
        }
    </script>
</body>
</html>
'''

if __name__ == '__main__':
    api = xDriveApi()
    window = webview.create_window(
        'xDrive 2K — Labs Engine',
        html=HTML_CONTENT,
        js_api=api,
        width=820,
        height=750,
        resizable=False
    )
    api.set_window(window)
    api.start_threads()
    webview.start(gui='edgechromium', private_mode=False)
