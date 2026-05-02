# labs xbox-stream sidecar

Headless Greenlight engine wrapped in stdio framing for the Qt
`XboxStreamPlugin`. Replaces the older `app/sidecars/xbox/` BGRA-canvas
sidecar.

## protocol

LBSX framing, big-endian length, on stdin/stdout:

```
[0..3] magic 'LBSX' (0x4C 0x42 0x53 0x58)
[4]    type
[5..8] payload length (uint32 BE)
[9..]  payload
```

Outbound (sidecar → plugin):

| type | meaning                                                              |
|------|----------------------------------------------------------------------|
| 0x01 | JSON event (`hello`, `tokens-ok`, `console_list`, `error`, …)        |
| 0x02 | H.264 NAL (Annex-B, with start codes)                                |
| 0x03 | Opus chunk (reserved; not emitted in current build)                  |

Inbound (plugin → sidecar):

| type | meaning                                                              |
|------|----------------------------------------------------------------------|
| 0x10 | JSON command (`ping`, `list_consoles`, `start_stream`, `stop`, …)    |
| 0x11 | Gamepad state JSON `{buttons, leftX/Y, rightX/Y, leftTrigger, rightTrigger}` |

stderr is human-readable logs.

## auth

The Qt plugin owns the OAuth flow (Labs-branded WebView2 dialog, code → token
exchange). It writes the xal-node token JSON into the per-account session
directory (see "multi-account" below).

The sidecar will not initiate sign-in. If no valid tokens are present at
startup it emits `{type:"error", code:"no-tokens"}` and idles until the parent
sends `{cmd:"stop"}`.

## multi-account

Microsoft enforces one active xCloud stream per account. To run multiple
streams in parallel, the parent Qt plugin spawns one sidecar per Microsoft
account and tags each with a stable `--account-key` (e.g. `acct1`, `acct2`,
or a hash of the account email). `default` is the fallback when the flag is
omitted.

Per-account state (token cache, Electron `userData` — cookies, IndexedDB,
WebRTC fingerprint, GPU shader cache, etc.) lives under:

```
<data-dir>/sessions/<account-key>/
    xbox-tokens.json    # xal-node token store (cleartext)
    electron/           # Electron userData; isolates each instance
```

Setting per-account `userData` is required: without it, two concurrent
Electron instances would race on the same user-data lock file and one would
fail to start. The sidecar calls `app.setPath('userData', ...)` *before*
`app.whenReady()` for this reason.

The `accountKey` the sidecar resolved appears in the `hello` event:

```json
{"type":"hello","version":1,"mode":"cloud","accountKey":"acct1","tokensFile":"...\\sessions\\acct1\\xbox-tokens.json"}
```

The Qt plugin can use this to confirm it spawned the right instance.

The LBSX framing magic (`'LBSX'`) is unchanged — each sidecar process owns
its own stdout, no multiplexing, so a per-account magic was unnecessary.

### migration from single-account installs

Older installs cached tokens at `<data-dir>/xbox-tokens.json`. After this
change the sidecar looks under `<data-dir>/sessions/default/xbox-tokens.json`.
Either:

1. Re-run the sign-in flow once under `accountKey=default` (the new default),
   or
2. Move the file:
   ```
   mkdir -p <data-dir>/sessions/default
   mv <data-dir>/xbox-tokens.json <data-dir>/sessions/default/xbox-tokens.json
   ```

`auth.js` accepts the same flags so you can drive a sign-in directly:

```
node ./auth.js --data-dir="%LOCALAPPDATA%/LabsEngine/xbox" --account-key=acct1
```

## H.264 extraction approach

WebRTC purposely hides the encoded payload from JavaScript — there is no
standard API to lift raw H.264 packets off an `RTCRtpReceiver`. So the
renderer does:

1. `xbox-xcloud-player` plays into a hidden `<video>`.
2. `videoEl.captureStream()` exposes a fresh `MediaStreamTrack`.
3. `MediaStreamTrackProcessor` reads decoded `VideoFrame` objects.
4. WebCodecs `VideoEncoder` re-encodes them to H.264 (Annex-B), low-latency
   mode, ~12 Mbps, 60fps.
5. Each chunk is shipped to `main.js` over IPC, framed onto stdout as type
   0x02.

Decoder + encoder are both Chromium-accelerated; the encode pass adds ~5ms on
a modern desktop. If WebCodecs isn't available in the running Electron
(`avc1.640028` / `avc1.42E028` both fail config probe), the renderer emits
`{type:"error", code:"webcodecs-unavailable"}` and idles. Fall-back to
MediaRecorder + WebM demux is sketched in code but not wired through main.js.

## build / install

```
cd app/sidecars/xbox-stream
npm install
```

The plugin's CMakeLists runs this automatically when `npm` is on PATH and
`package.json` has changed.

## CLI flags (main.js)

| flag                   | meaning                                                    |
|------------------------|------------------------------------------------------------|
| `--data-dir <path>`    | Root for per-account state.                                |
| `--account-key <key>`  | Per-account scope; defaults to `default`. Sanitised to `[A-Za-z0-9._-]`. |
| `--mode home\|cloud`   | Required.                                                  |
| `--console-id <id>`    | Required for `home` mode.                                  |
| `--title-id <id>`      | Optional, used for `cloud` mode.                           |
| `--tokens-file <path>` | Override the per-account token-cache path.                 |
| `--show`               | Show the otherwise-hidden BrowserWindow (debugging).       |

## running standalone

```
electron . --data-dir "%LOCALAPPDATA%/LabsEngine/xbox" --account-key acct1 --mode home
```

The Qt plugin runs us with stdin/stdout piped — this CLI is for debugging.
