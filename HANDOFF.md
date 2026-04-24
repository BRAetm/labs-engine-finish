# HANDOFF — Labs Engine

Read this before touching anything. It's the minimum context another AI (or you after a break) needs to not do something dumb.

---

## What this project is

Windows-only app that streams **PS5 Remote Play into a capturable window** and provides a **Python script harness** that can drive the controller back to the PS5. Primary use: NBA 2K shot-meter automation, but the host is generic.

**Two binaries:**
- `LabsEngine.exe` — Qt 6.8.3 C++ host. MSVC 14.44, CMake + Ninja. Plugin architecture (display, wgc_capture, xinput_input, vigem_output, cv_python, ps_remote_play, dualsense_input).
- Python scripts under `labs-engine/scripts/` — picked from the left-rail in the Labs Engine UI, spawned as subprocesses. `Labs2K.py` is the current target; `SecretK.py` is legacy-compatible via monkey-patch.

**Build + package:**
```
powershell -ExecutionPolicy Bypass -File app/scripts/build.ps1 -Release
powershell -ExecutionPolicy Bypass -File app/scripts/package.ps1
```
First rebuilds `app/build/msvc-release/bin/`. Second stages `dist/LabsEngine-portable/` + zip, and an NSIS installer if makensis is available.

**Repository layout:**
```
app/                                C++ host
  core/                             LabsCore.dll sources (plugin IDL, fan-out, SHM override)
  engine/                           LabsEngine.exe (main window, lifecycle)
  plugins/                          one subdir per plugin DLL
    display/                        stream video renderer
    wgc_capture/                    Windows Graphics Capture source
    xinput_input/                   XInput polling source
    vigem_output/                   virtual X360 pad sink (Xbox mode)
    cv_python/                      Python subprocess harness
    ps_remote_play/                 chiaki/labs.dll → PS5
    dualsense_input/                DualSense/DS4 HID source (bypasses XInput)
  scripts/
    build.ps1                       release build
    package.ps1                     stage dist/
    installer.nsi                   NSIS single-exe installer
  third-party/
    ViGEmClient/                    vendored (MIT) — statically linked into ViGEmPlugin
    ViGEmBus/ViGEmBus_Setup.exe     kernel driver installer (shipped to users)
    ffmpeg/                         headers + libs (exe runtime brought in by windeployqt)

labs-engine/
  scripts/                          ← what the Labs Engine left-rail UI scans
    Labs2K.py                       target shot-timing script (use this one)
    SecretK.py                      legacy; same monkey-patch pattern applied
    labs_input_bridge.py            Python side of SHM override channel
    zp_core/                        reconstructed ZP HIGHER Lite engine (read-only)
    dev/                            diagnostics — NOT surfaced in the UI
      test_input_bridge.py          RT pulse smoke test
      test_ls_walk.py               LS stick smoke test
      test_button_tap.py            Cross button smoke test
      xinput_test.py                XInput slot sanity
      find_ps5.py                   UDP broadcast probe
  cv-scripts/                       imported modules (not run directly)
    shot_meter.py                   BGR meter detector + XInput helper
    features.py                     legacy PSControllerBridge for SecretK
  userdata/                         runtime settings (gitignored)

refence/                            prior-art clones — READ ONLY, never edit
```

---

## The input pipeline (critical — this is where bugs live)

```
DualSense (HID)  ──> DualSenseSource (C++)  ─────────────┐
                                                          │
                                                          ▼
                                                    FanOutCtrlSink
                                                    ────────────────
                                                    InputOverride::apply(state)
                                                          │
                                      ┌───────────────────┴──────────────┐
                                      ▼                                  ▼
                             ControllerMonitor                 PS Remote Play sink
                             (UI, visible stick/btn)           (chiaki/labs.dll → PS5)

                              ▲
                              │  (writes to SHM)
                              │
Labs2K.py / test_*.py  ──> labs_input_bridge.py (writes "Labs_Input_Override_v2" named SHM)
                              │
                              │  C++ reads it in apply()
                              ▼
                     InputOverride (LabsCore.dll)
```

**Key files:**
- [app/core/InputOverride.h](app/core/InputOverride.h), [.cpp](app/core/InputOverride.cpp) — 32-byte packed SHM record. Timebase is **raw QPC ticks** (not chrono microseconds — that had epoch drift across the Python/C++ boundary). Sanity gate rejects absurd future expirations.
- [app/engine/LabsMainWindow.cpp](app/engine/LabsMainWindow.cpp) — `FanOutCtrlSink::pushState` calls `InputOverride::instance().apply(s)` **at the fan-out**, not inside individual sources. This matters: doing it per-source lost the race against XInputPlugin pushing neutral state from zombie vgamepad pads.
- [labs-engine/scripts/labs_input_bridge.py](labs-engine/scripts/labs_input_bridge.py) — Python writer. Uses `ctypes` to call `QueryPerformanceCounter` directly so its clock matches C++. Eagerly zeroes SHM on import so a previous script's last state doesn't carry over.

**Why there's no skip-mask anymore:** older code skipped XInput slot 0 in PS mode assuming "slot 0 = physical pad, slot 1+ = augmented virtual pad." With DualSenseSource bypassing XInput entirely, the old logic filtered out virtual pulses. Skip mask set to 0 everywhere in `applyMode()`.

---

## The CV pipeline (shot meter detection)

- Full source under `labs-engine/scripts/zp_core/` — reconstructed from ZP HIGHER Lite ui.dll Nuitka bytecode. We treat this as read-only and apply runtime patches from the launcher instead.
- `Labs2K.py` is the launcher. It imports zp_core.engine, monkey-patches it, opens a Qt UI, then runs the engine.

**Runtime patches in `Labs2K.py`** (all there because the bytecode RE got specific constants wrong OR the game has evolved):
1. `_find_target_window` → grabs the Labs Engine window (original looked for "Xbox"/"Chiaki"/"Remote Play" which aren't ours)
2. Clamps the capture region to the primary monitor because BetterCam rejects rects that poke past screen edges (a 1938-wide window on a 1920-wide monitor died otherwise)
3. `REFRACTORY_SECONDS = 0.55` (was 55.0 in the RE — that would fire one shot per minute)
4. `_ABSOLUTE_MIN_HEIGHT = 50, MIN_AREA = 200` (was 12/10 — the stamina bar noise-calibrated the detector)
5. Velocity gate on `_check_shot_trigger` — refuse to fire if the meter hasn't actually been rising (static UI noise has vel=0)
6. `_process_mask_cpu` replaced with a fast version: 2× downsample, single wide magenta BGR range (was 5), one CLOSE morph (was CLOSE+OPEN). Ultra-lite toggle switches to 4× downsample + 20fps capture for weak CPUs.
7. `PSControllerBridge.__init__` nullifies self.ds4/x360 — the original creates `VDS4Gamepad + VX360Gamepad`. The VX360 was the zombie virtual pad XInputPlugin kept polling, drowning out DualSense input.
8. `_fire_rt` rewritten to call `labs_input_bridge.fire_combo()` — sends an RT pulse **and** snaps RS Y to neutral so both shot-stick and button-shot control schemes fire on the same override.

**SecretK.py** has the same monkey-patches on the smaller `features.py` PSControllerBridge + `shot_meter.py` detector. Use Labs2K unless you have a specific reason.

**Known defects in the detector that are NOT fixed:** the BGR color ranges are tuned for a specific skin/lighting. In auto-color-learn mode it adapts, but until 12 contour samples accumulate it can false-calibrate. The noise-floor + velocity gate mask this well enough for ship but it's the fragile part.

---

## Controller input conventions (don't flip these by accident)

- **`ControllerState` struct** uses XInput convention: `+32767` = stick UP, `-32767` = stick DOWN.
- **PS Remote Play sink** negates Y once before handing to chiaki because chiaki uses screen-space (positive Y = down). `negY(-32768)` is special-cased to avoid overflow.
- **DualSense HID** reports Y "up" as byte value ~0. The plugin's `cnv()` function inverts it so `leftThumbY` ends up in XInput convention (positive = up).
- If the character walks the wrong way after you change something in the pipeline, suspect your Y sign and trace it through all three conversions.

---

## Diagnostic scripts

Under `labs-engine/scripts/dev/` (subfolder so the UI's top-level script picker doesn't show them to end users):
- `test_input_bridge.py` — pulses RT=255 every second. Use to verify SHM round-trip end-to-end without needing a game.
- `test_ls_walk.py` — alternates LS forward/back every 1s. Character should walk visibly.
- `test_button_tap.py` — taps Cross every 2s. Use on the PS5 home screen to verify button override.
- `xinput_test.py` — prints any XInput gamepad state for ~10s. Sanity-check pads.
- `find_ps5.py` — standalone UDP broadcast PS5 discovery probe.

These are the first things to run when debugging. If they all work but Labs2K doesn't fire in-game, the problem is detection, not the input bridge.

---

## What's shipped vs what's left

**Shipped:**
- C++ host is feature-complete. Input bridge works end-to-end.
- Packaging produces a 120 MB portable zip.
- ViGEmBus driver installer is bundled (`third-party/ViGEmBus/ViGEmBus_Setup.exe`).
- ViGEmClient is statically linked into ViGEmPlugin (vendored under `app/third-party/ViGEmClient/`) — no `ViGEmClient.dll` runtime dependency.

**Known gaps for consumer distribution:**
1. **Python runtime not bundled.** Scripts need PySide6, numpy, opencv-python, bettercam, vgamepad. Consumer needs Python 3.11+ installed and a `pip install` run. Add `setup.ps1` + `requirements.txt` OR bundle embedded Python (~200MB extra).
2. **Unsigned exe** — SmartScreen warning on first launch. Code-signing cert needed long-term; document click-through in README short-term.
3. **No custom icon** — LabsEngine.exe has the default Qt/cmake icon.
4. No auto-update, no telemetry, no crash reporting. Post-launch adds.

---

## Build environment assumptions

- Windows 11.
- Qt 6.8.3 at `C:\Qt\6.8.3\msvc2022_64`.
- MSVC 14.44 via VS Build Tools 2022.
- Python 3.11 at the user's default install path. Scripts run via `python <file.py>` (plain `python` on PATH).
- Paths contain spaces (`labs engine final`) — quote them everywhere.
- Shell is bash (git-bash / WSL style); use `/dev/null`, forward slashes, etc.

## Reference trees (read-only, never edit)

- `refence/labs-engine/` — prior C# WPF version. Still has the chiaki native stuff we use (`labs.dll`).
- `refence/helios-source/` — different project, ignore unless debugging plugin-loading patterns.

Ignore anything under `.claude/`, `.gstack/`, `~/.claude/skills/` — author's AI tooling, not part of the repo.

---

## When you pick this up on a new machine

1. `git pull` and skim recent commits.
2. Read `CLAUDE.md` in the repo root.
3. Read this file end to end.
4. Run `build.ps1 -Release` and confirm it builds.
5. Run `test_input_bridge.py` with a PS5 stream active to confirm the input bridge still works.
6. Then you can actually change things.
