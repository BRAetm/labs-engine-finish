// Renderer for the Labs Xbox Stream sidecar. Runs xbox-xcloud-player headlessly
// (the <video> element is invisible inside a hidden BrowserWindow), captures
// the inbound video MediaStreamTrack via MediaStreamTrackProcessor, then
// re-encodes to H.264 Annex-B NALs with WebCodecs VideoEncoder and ships them
// to main.js via IPC.
//
// Why re-encode instead of forwarding the network H.264 verbatim?
//   WebRTC purposely hides the encoded payload from JavaScript — there is no
//   standard API to lift the raw H.264 packets off an RTCRtpReceiver. The
//   cleanest reliable path is decode → re-encode. The decoder is hardware-
//   accelerated by Chromium, the encoder ditto. Latency budget on a modern
//   desktop is ~5ms.
//
// If WebCodecs isn't available (very old Electron / no GPU encoder), we fall
// back to MediaRecorder with a webm/H264 mime — main.js demuxes it into NALs.

const { ipcRenderer } = require('electron');
const xCloudPlayerPkg = require('xbox-xcloud-player');
const xCloudPlayer = xCloudPlayerPkg.default || xCloudPlayerPkg;

const logEl = document.getElementById('log');
function log(level, ...args) {
    const msg = args.map(a => {
        if (typeof a === 'string') return a;
        try { return JSON.stringify(a); } catch { return String(a); }
    }).join(' ');
    if (logEl) logEl.textContent += `[${level}] ${msg}\n`;
    ipcRenderer.send('log', level, msg);
}

const qs = new URLSearchParams(location.search);
const QS_MODE      = qs.get('mode') || 'home';
const QS_CONSOLE   = qs.get('consoleId') || '';
const QS_TITLE     = qs.get('titleId') || '';
const QS_MSAL      = qs.get('msal') || '';

// ── session machinery (mirrors the proven flow from sidecars/xbox) ──────────

let api = null;
let stream = null;
let player = null;
let gamepad = null;
let keepaliveTimer = null;
let frameCallbackHandle = null;
let videoEncoder = null;
let trackProcessor = null;
let processorReader = null;
let captureRunning = false;
let videoStartedAt = 0;

// SPS+PPS cache so we can re-emit them every couple of seconds in case the
// downstream FFmpeg decoder lost them on a flush.
let cachedExtradata = null;

async function listConsoles() {
    if (!api) {
        // Construct enough of an api to run getConsoles() before a stream is up.
        api = new xCloudPlayer.ApiClient({ host: window.location.origin, force_1080p: true });
    }
    try {
        const r = await api.getConsoles();
        const consoles = (r?.results || []).map(c => ({
            id:    c.serverId || c.id || '',
            name:  c.consoleName || c.name || '',
            type:  c.consoleType || '',
            state: c.powerState || c.state || '',
        })).filter(c => c.id);
        ipcRenderer.send('event', { type: 'console_list', consoles });
    } catch (e) {
        ipcRenderer.send('event', { type: 'error', code: 'list-consoles-failed', message: e.message || String(e) });
    }
}

async function startSession(opts) {
    api = new xCloudPlayer.ApiClient({ host: window.location.origin, force_1080p: true });

    let target = (opts.mode === 'cloud') ? opts.titleId : opts.consoleId;
    if (opts.mode === 'home' && !target) {
        const r = await api.getConsoles();
        target = r?.results?.[0]?.serverId;
    }
    if (!target) throw new Error(`no ${opts.mode === 'cloud' ? 'titleId' : 'console'} available`);

    log('info', `startStream(${opts.mode}, ${target})`);
    stream = await api.startStream(opts.mode, target);

    stream.onProvisioned = () => {
        log('info', 'stream provisioned — instantiating Player');
        player = new xCloudPlayer.Player('streamHolder', {
            audio_mono: false,
            video_bitrate: 20000,
            audio_bitrate: 128,
            keyframe_interval: 5,
        });
        player.onConnectionStateChange((s) => {
            log('info', 'rtc state:', s);
            ipcRenderer.send('event', { type: 'rtc_state', state: s });
        });
        player.setChatSdpHandler?.((offer) => {
            stream.sendChatSDPOffer(offer)
                .then(resp => { player.setRemoteOffer(JSON.parse(resp.exchangeResponse).sdp); })
                .catch(e => log('error', 'sendChatSDPOffer:', e.message || e));
        });

        player.createOffer().then(offer => {
            stream.sendSDPOffer(offer).then(resp => {
                player.setRemoteOffer(JSON.parse(resp.exchangeResponse).sdp);
                const localIce = (player.getIceCandidates() || []).map(c => JSON.stringify({
                    candidate: c.candidate, sdpMid: c.sdpMid,
                    sdpMLineIndex: c.sdpMLineIndex, usernameFragment: c.usernameFragment,
                }));
                stream.sendIceCandidates(localIce).then(iceResp => {
                    player.setRemoteIceCandidates(JSON.parse(iceResp.exchangeResponse));
                    log('info', 'ICE exchanged');
                    attachGamepadAndHookVideo();
                }).catch(e => log('error', 'sendIceCandidates:', e.message || e));
            }).catch(e => log('error', 'sendSDPOffer:', e.message || e));
        }).catch(e => log('error', 'createOffer:', e.message || e));

        keepaliveTimer = setInterval(() => {
            stream.sendKeepalive().then(r => {
                if (r?.code === 'SessionNotActive' || r?.code === 'SessionNotFound') {
                    log('warn', 'session ended:', r.code);
                    clearInterval(keepaliveTimer);
                    ipcRenderer.send('event', { type: 'session_ended', code: r.code });
                }
            }).catch(e => log('warn', 'keepalive:', e.message || e));
        }, 30_000);
    };
    stream.onError = () => log('error', 'stream onError');
    stream.onReadyToConnect = () => {
        log('info', 'onReadyToConnect — sending MSAL');
        if (!opts.msal) {
            log('error', 'home stream needs MSAL but none provided');
            return;
        }
        stream.sendMSALAuth(opts.msal).catch(e => log('error', 'sendMSALAuth:', e.message || e));
    };

    try { stream.waitForState('Provisioned'); }
    catch (e) { log('error', 'waitForState:', e.message || e); }
}

function attachGamepadAndHookVideo() {
    try {
        gamepad = new xCloudPlayer.Gamepad(0, { enable_keyboard: false });
        gamepad.attach(player);
        log('info', 'virtual gamepad attached');
    } catch (e) { log('error', 'gamepad attach failed:', e.message || e); }

    const el = player.getVideoElement?.();
    if (!el) { log('error', 'getVideoElement null'); return; }
    el.addEventListener('loadeddata', () => {
        log('info', `video ${el.videoWidth}x${el.videoHeight}`);
        ipcRenderer.send('event', { type: 'video_size', width: el.videoWidth, height: el.videoHeight });
    });
    el.addEventListener('playing', () => startEncodePipeline(el));
}

// ── encode pipeline: VideoFrame → VideoEncoder → H.264 NALs → IPC ───────────

async function startEncodePipeline(videoEl) {
    if (captureRunning) return;
    captureRunning = true;
    videoStartedAt = performance.now();

    const w = videoEl.videoWidth || 1280;
    const h = videoEl.videoHeight || 720;
    log('info', `encode pipeline ${w}x${h}`);
    ipcRenderer.send('event', { type: 'stream_started', width: w, height: h });

    // captureStream() is the fastest way to get a MediaStream out of an HTMLVideoElement.
    let trackForCapture = null;
    try {
        const captureStream = videoEl.captureStream || videoEl.mozCaptureStream;
        if (typeof captureStream === 'function') {
            const ms = captureStream.call(videoEl);
            trackForCapture = ms.getVideoTracks()[0];
        }
    } catch (e) {
        log('warn', 'captureStream failed:', e.message || e);
    }

    if (!trackForCapture) {
        log('error', 'no MediaStreamTrack from <video>; falling back to canvas readback');
        return startCanvasFallback(videoEl, w, h);
    }

    // Try WebCodecs path first. Available in Electron ≥ 24.
    if (typeof MediaStreamTrackProcessor !== 'undefined' && typeof VideoEncoder !== 'undefined') {
        try {
            await runWebCodecsPath(trackForCapture, w, h);
            return;
        } catch (e) {
            log('warn', 'WebCodecs path failed, falling back to MediaRecorder:', e.message || e);
        }
    }

    // Fallback — MediaRecorder produces WebM/H264. main.js doesn't currently
    // demux that; this is a last-resort. Tell main so it knows.
    ipcRenderer.send('event', { type: 'error', code: 'webcodecs-unavailable',
        message: 'WebCodecs not available — install a newer Electron, or extend main.js to demux WebM.' });
    captureRunning = false;
}

async function runWebCodecsPath(track, w, h) {
    trackProcessor = new MediaStreamTrackProcessor({ track });
    processorReader = trackProcessor.readable.getReader();

    let configured = false;
    let nextKeyFrame = true; // first frame must be keyframe

    const encoderInit = {
        output: (chunk, meta) => {
            try {
                // meta?.decoderConfig?.description holds AVC extradata when present.
                if (meta?.decoderConfig?.description) {
                    cachedExtradata = new Uint8Array(meta.decoderConfig.description);
                    // Convert AVC extradata → Annex-B SPS+PPS prefix and emit.
                    const annexB = avccExtradataToAnnexB(cachedExtradata);
                    if (annexB) ipcRenderer.send('nal', annexB);
                }
                // chunk.copyTo a Uint8Array.
                const out = new Uint8Array(chunk.byteLength);
                chunk.copyTo(out);
                // VideoEncoder may emit AVCC (4-byte length-prefixed) or Annex-B
                // depending on `avc.format`. We requested annexb, but be safe.
                const annexB = ensureAnnexB(out);
                ipcRenderer.send('nal', annexB);
            } catch (e) {
                log('error', 'encoder output error:', e.message || e);
            }
        },
        error: (e) => {
            log('error', 'VideoEncoder error:', e.message || e);
            captureRunning = false;
        },
    };
    videoEncoder = new VideoEncoder(encoderInit);

    // Try Annex-B output. If unsupported, fall through to default and we'll
    // convert per-chunk.
    let cfg = {
        codec: 'avc1.640028', // H.264 High @ 4.0
        width: w, height: h,
        bitrate: 12_000_000,
        framerate: 60,
        latencyMode: 'realtime',
        avc: { format: 'annexb' },
    };
    try {
        const sup = await VideoEncoder.isConfigSupported(cfg);
        if (!sup?.supported) {
            cfg.codec = 'avc1.42E028'; // Baseline
            const sup2 = await VideoEncoder.isConfigSupported(cfg);
            if (!sup2?.supported) throw new Error('no supported H.264 config');
        }
    } catch (e) {
        log('warn', 'config probe:', e.message || e);
    }
    videoEncoder.configure(cfg);
    configured = true;

    // Pump frames — one read → encode loop. VideoEncoder back-pressures via
    // encodeQueueSize; we drop frames if it backs up to keep latency low.
    (async () => {
        try {
            while (captureRunning) {
                const { value: frame, done } = await processorReader.read();
                if (done) break;
                try {
                    if (videoEncoder.encodeQueueSize > 4) {
                        frame.close();
                        continue;
                    }
                    const opts = { keyFrame: nextKeyFrame };
                    nextKeyFrame = false;
                    videoEncoder.encode(frame, opts);
                } finally {
                    frame.close();
                }
            }
        } catch (e) {
            log('error', 'reader loop:', e.message || e);
        }
        captureRunning = false;
    })();

    // Periodic keyframe request as a safety net (every ~2s).
    const ki = setInterval(() => {
        if (!captureRunning) { clearInterval(ki); return; }
        nextKeyFrame = true;
    }, 2000);
}

// AVCC (length-prefixed) → Annex-B (start-code) conversion. No-op if the
// buffer already starts with an Annex-B start code.
function ensureAnnexB(u8) {
    if (u8.length >= 4 &&
        ((u8[0] === 0 && u8[1] === 0 && u8[2] === 0 && u8[3] === 1) ||
         (u8[0] === 0 && u8[1] === 0 && u8[2] === 1))) {
        return u8;
    }
    // Treat as AVCC (4-byte big-endian length prefix per NAL).
    const out = [];
    let i = 0;
    while (i + 4 <= u8.length) {
        const len = (u8[i] << 24) | (u8[i+1] << 16) | (u8[i+2] << 8) | u8[i+3];
        i += 4;
        if (len <= 0 || i + len > u8.length) break;
        out.push(0, 0, 0, 1);
        for (let k = 0; k < len; k++) out.push(u8[i + k]);
        i += len;
    }
    return Uint8Array.from(out);
}

// AVCDecoderConfigurationRecord → Annex-B SPS+PPS. Format ref: ISO 14496-15.
function avccExtradataToAnnexB(u8) {
    if (!u8 || u8.length < 7) return null;
    if (u8[0] !== 1) return null; // configurationVersion
    const out = [];
    let p = 5;
    const numSps = u8[p++] & 0x1F;
    for (let i = 0; i < numSps; i++) {
        if (p + 2 > u8.length) return null;
        const len = (u8[p] << 8) | u8[p+1]; p += 2;
        if (p + len > u8.length) return null;
        out.push(0, 0, 0, 1);
        for (let k = 0; k < len; k++) out.push(u8[p + k]);
        p += len;
    }
    if (p >= u8.length) return Uint8Array.from(out);
    const numPps = u8[p++];
    for (let i = 0; i < numPps; i++) {
        if (p + 2 > u8.length) return null;
        const len = (u8[p] << 8) | u8[p+1]; p += 2;
        if (p + len > u8.length) return null;
        out.push(0, 0, 0, 1);
        for (let k = 0; k < len; k++) out.push(u8[p + k]);
        p += len;
    }
    return Uint8Array.from(out);
}

// ── canvas fallback (publishes a single error event then idles) ─────────────

function startCanvasFallback(videoEl, w, h) {
    log('error', 'canvas fallback active — Qt plugin will receive no frames');
    ipcRenderer.send('event', { type: 'error', code: 'no-mediastream',
        message: 'video.captureStream unavailable; cannot extract frames' });
    captureRunning = false;
}

// ── input forwarding (engine state → xCloudPlayer.Gamepad) ──────────────────

const BTN_BITS = [
    [0x0001, 'DPadUp'],     [0x0002, 'DPadDown'],
    [0x0004, 'DPadLeft'],   [0x0008, 'DPadRight'],
    [0x0010, 'Menu'],       [0x0020, 'View'],
    [0x0040, 'LeftThumb'],  [0x0080, 'RightThumb'],
    [0x0100, 'LeftShoulder'],[0x0200,'RightShoulder'],
    [0x0400, 'Nexus'],
    [0x1000, 'A'],          [0x2000, 'B'],
    [0x4000, 'X'],          [0x8000, 'Y'],
];
let lastButtons = 0, lastLX=0, lastLY=0, lastRX=0, lastRY=0, lastLT=0, lastRT=0;
function applyInput(st) {
    if (!gamepad) return;
    const buttons = st.buttons | 0;
    if (buttons !== lastButtons) {
        const diff = buttons ^ lastButtons;
        for (const [bit, name] of BTN_BITS) {
            if (diff & bit) {
                try { gamepad.sendButtonState(name, (buttons & bit) ? 1 : 0); }
                catch {}
            }
        }
        lastButtons = buttons;
    }
    const norm = v => Math.max(-1, Math.min(1, v / 32767));
    const lx = norm(st.leftX  | 0);
    const ly = -norm(st.leftY  | 0);
    const rx = norm(st.rightX | 0);
    const ry = -norm(st.rightY | 0);
    const lt = Math.max(0, Math.min(1, (st.leftTrigger  & 0xff) / 255));
    const rt = Math.max(0, Math.min(1, (st.rightTrigger & 0xff) / 255));
    try {
        if (lx !== lastLX) { gamepad.sendButtonState('LeftThumbXAxis',  lx); lastLX = lx; }
        if (ly !== lastLY) { gamepad.sendButtonState('LeftThumbYAxis',  ly); lastLY = ly; }
        if (rx !== lastRX) { gamepad.sendButtonState('RightThumbXAxis', rx); lastRX = rx; }
        if (ry !== lastRY) { gamepad.sendButtonState('RightThumbYAxis', ry); lastRY = ry; }
        if (lt !== lastLT) { gamepad.sendButtonState('LeftTrigger',     lt); lastLT = lt; }
        if (rt !== lastRT) { gamepad.sendButtonState('RightTrigger',    rt); lastRT = rt; }
    } catch {}
}

// ── shutdown ────────────────────────────────────────────────────────────────

function stopSession() {
    captureRunning = false;
    try { processorReader?.cancel(); } catch {}
    processorReader = null;
    try { trackProcessor && (trackProcessor = null); } catch {}
    try { videoEncoder?.flush?.(); } catch {}
    try { videoEncoder?.close?.(); } catch {}
    videoEncoder = null;
    try { if (frameCallbackHandle) cancelAnimationFrame(frameCallbackHandle); } catch {}
    try { if (keepaliveTimer) clearInterval(keepaliveTimer); } catch {}
    try { gamepad?.detach(); } catch {}
    try { player?.destroy(); } catch {}
    gamepad = null; player = null; stream = null;
}

// ── IPC channels from main.js ───────────────────────────────────────────────

ipcRenderer.on('start_stream', (_e, opts) => {
    startSession(opts).catch(e => {
        log('error', 'startSession:', e.message || e);
        ipcRenderer.send('event', { type: 'error', code: 'start-failed', message: e.message || String(e) });
    });
});
ipcRenderer.on('stop_stream', () => stopSession());
ipcRenderer.on('list_consoles', () => listConsoles());
ipcRenderer.on('gamepad', (_e, st) => applyInput(st));

ipcRenderer.send('renderer-ready');
log('info', 'renderer ready');

// If query string already has consoleId/titleId we auto-start (lets the parent
// pass mode + target via URL during early sessions before commands flow).
if (QS_MODE === 'cloud' ? QS_TITLE : QS_CONSOLE) {
    startSession({ mode: QS_MODE, consoleId: QS_CONSOLE, titleId: QS_TITLE, msal: QS_MSAL })
        .catch(e => log('error', 'auto startSession:', e.message || e));
}
