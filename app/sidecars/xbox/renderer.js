// Runs inside the hidden BrowserWindow. Two modes — selected by ?bridge=1 on
// the URL:
//   spike:  runs once, captures the first decoded frame to a PNG, reports done
//   bridge: waits for IPC commands ({start, stop, input}), streams frames back
//           as raw RGBA to main, which repackets them onto stdout.

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
const BRIDGE = qs.get('bridge') === '1';

// ── shared session machinery ────────────────────────────────────────────────

let api = null;
let stream = null;
let player = null;
let gamepad = null;
let keepaliveTimer = null;
let frameCallbackHandle = null;

async function startSession({ mode, consoleId, titleId, msal }) {
    api = new xCloudPlayer.ApiClient({ host: window.location.origin, force_1080p: true });

    let target = (mode === 'cloud') ? titleId : consoleId;
    if (mode === 'home' && !target) {
        try {
            const consoles = await api.getConsoles();
            log('info', 'consoles:', consoles);
            target = consoles?.results?.[0]?.serverId;
        } catch (e) {
            throw new Error('getConsoles failed: ' + (e.message || e));
        }
    }
    if (!target) throw new Error(`no ${mode === 'cloud' ? 'titleId' : 'console'} available`);

    log('info', `startStream(${mode}, ${target})`);
    stream = await api.startStream(mode, target);

    // DO NOT send MSAL preemptively. Doing so makes the server briefly report
    // 'Provisioned' then regress to 'Provisioning' indefinitely, and onProvisioned
    // fires too early to attach the player. Wait for the state machine to enter
    // ReadyToConnect, send MSAL there (per xbox-xcloud-player example + greenlight
    // streammanager.ts), then let it transition to Provisioned cleanly.

    stream.onProvisioned = () => {
        log('info', 'stream provisioned — instantiating Player');
        player = new xCloudPlayer.Player('streamHolder', {
            audio_mono: false,
            video_bitrate: 20000,
            audio_bitrate: 128,
            keyframe_interval: 5,
        });
        player.onConnectionStateChange((state) => log('info', 'rtc state:', state));

        // Chat (audio) SDP side-channel. Without this handler, the server may
        // keep the session in Provisioning waiting for the chat SDP exchange.
        player.setChatSdpHandler?.((offer) => {
            stream.sendChatSDPOffer(offer)
                .then((resp) => {
                    const answer = JSON.parse(resp.exchangeResponse).sdp;
                    player.setRemoteOffer(answer);
                    log('info', 'chat SDP exchanged');
                })
                .catch(e => log('error', 'sendChatSDPOffer:', e.message || e));
        });

        player.createOffer().then((offer) => {
            stream.sendSDPOffer(offer).then((resp) => {
                const answerSdp = JSON.parse(resp.exchangeResponse).sdp;
                player.setRemoteOffer(answerSdp);
                const localIce = (player.getIceCandidates() || []).map(c => JSON.stringify({
                    candidate: c.candidate, sdpMid: c.sdpMid,
                    sdpMLineIndex: c.sdpMLineIndex, usernameFragment: c.usernameFragment,
                }));
                stream.sendIceCandidates(localIce).then((iceResp) => {
                    const remoteIce = JSON.parse(iceResp.exchangeResponse);
                    player.setRemoteIceCandidates(remoteIce);
                    log('info', 'ICE exchanged');
                    attachGamepadAndHookVideo();
                }).catch(e => log('error', 'sendIceCandidates:', e.message || e));
            }).catch(e => log('error', 'sendSDPOffer:', e.message || e));
        }).catch(e => log('error', 'createOffer:', e.message || e));

        keepaliveTimer = setInterval(() => {
            stream.sendKeepalive().then((r) => {
                if (r?.code === 'SessionNotActive' || r?.code === 'SessionNotFound') {
                    log('warn', 'session ended:', r.code);
                    clearInterval(keepaliveTimer);
                }
            }).catch(e => log('warn', 'keepalive:', e.message || e));
        }, 30_000);
    };

    stream.onError = () => log('error', 'stream onError fired');
    stream.onReadyToConnect = () => {
        log('info', 'onReadyToConnect — sending MSAL');
        if (!msal) {
            log('error', 'home stream needs MSAL token but none provided');
            return;
        }
        stream.sendMSALAuth(msal).then(r => log('info', 'MSAL ack:', r))
                                 .catch(e => log('error', 'sendMSALAuth:', e.message || e));
    };

    // Stream._waitInterval polls /state and fires onProvisioned/onReadyToConnect
    // on transitions, then stops once desiredState is reached. Aim for
    // 'Provisioned' so polling keeps running through the ReadyToConnect →
    // Provisioning → Provisioned arc and BOTH callbacks get a chance to fire.
    // Stopping at 'ReadyToConnect' kills the poller before Provisioned hits.
    try { stream.waitForState('Provisioned'); }
    catch (e) { log('error', 'waitForState:', e.message || e); }

    // Diagnostic: log every observed state for the first 30 seconds.
    let lastState = '';
    let stateLogTicks = 0;
    const stateLogger = setInterval(() => {
        const s = stream.getState?.();
        if (s !== lastState) {
            log('info', `state→${s}`);
            lastState = s;
        }
        if (++stateLogTicks > 60) clearInterval(stateLogger);
    }, 500);
}

function attachGamepadAndHookVideo() {
    try {
        gamepad = new xCloudPlayer.Gamepad(0, { enable_keyboard: false });
        gamepad.attach(player);
        log('info', 'virtual gamepad attached');
    } catch (e) {
        log('error', 'gamepad attach failed:', e.message || e);
    }

    const el = player.getVideoElement?.();
    if (!el) { log('error', 'getVideoElement null'); return; }
    el.addEventListener('loadeddata', () => log('info', `video ${el.videoWidth}x${el.videoHeight}`));
    el.addEventListener('playing', () => {
        if (BRIDGE) startFrameStream(el);
        else        captureFirstFrameAndExit(el);
    });
}

// ── input forwarding (engine ControllerState → xCloudPlayer.Gamepad) ────────

const BTN_BITS = [
    [0x0001, 'DPadUp'],      [0x0002, 'DPadDown'],
    [0x0004, 'DPadLeft'],    [0x0008, 'DPadRight'],
    [0x0010, 'Menu'],        [0x0020, 'View'],
    [0x0040, 'LeftThumb'],   [0x0080, 'RightThumb'],
    [0x0100, 'LeftShoulder'],[0x0200, 'RightShoulder'],
    [0x0400, 'Nexus'],
    [0x1000, 'A'],           [0x2000, 'B'],
    [0x4000, 'X'],           [0x8000, 'Y'],
];

let lastButtons = 0;
let lastLX = 0, lastLY = 0, lastRX = 0, lastRY = 0, lastLT = 0, lastRT = 0;

function applyInput(st) {
    if (!gamepad) return;
    const buttons = st.buttons | 0;
    if (buttons !== lastButtons) {
        const diff = buttons ^ lastButtons;
        for (const [bit, name] of BTN_BITS) {
            if (diff & bit) {
                try { gamepad.sendButtonState(name, (buttons & bit) ? 1 : 0); }
                catch (e) { /* ignore per-button failures */ }
            }
        }
        lastButtons = buttons;
    }

    // Axes: XInput thumb = int16 (-32768..32767); xcloud wants normalized -1..1.
    // Engine side already uses XInput convention; Y+ up. xCloud Gamepad API
    // matches browser convention (Y+ down), so invert Y.
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
    } catch (e) { /* ignore */ }
}

// ── bridge mode: frame streaming to main ────────────────────────────────────

function startFrameStream(videoEl) {
    const w = videoEl.videoWidth;
    const h = videoEl.videoHeight;
    if (!w || !h) { log('error', 'video has no dimensions at play time'); return; }

    const canvas = document.createElement('canvas');
    canvas.width = w; canvas.height = h;
    const ctx = canvas.getContext('2d', { willReadFrequently: true });
    let frameIdx = 0;

    const capture = () => {
        ctx.drawImage(videoEl, 0, 0, w, h);
        const img = ctx.getImageData(0, 0, w, h);
        // canvas order is RGBA — publish with format=1.
        ipcRenderer.send('bridge-frame', {
            frameIdx: frameIdx++,
            width: w, height: h, stride: w * 4, format: 1,
            timestampUs: Math.floor(performance.now() * 1000),
        }, new Uint8Array(img.data.buffer));

        if (typeof videoEl.requestVideoFrameCallback === 'function')
            frameCallbackHandle = videoEl.requestVideoFrameCallback(capture);
        else
            frameCallbackHandle = requestAnimationFrame(capture);
    };

    if (typeof videoEl.requestVideoFrameCallback === 'function')
        videoEl.requestVideoFrameCallback(capture);
    else
        requestAnimationFrame(capture);

    log('info', `frame stream started ${w}x${h}`);
}

// ── spike mode: one-shot PNG dump ───────────────────────────────────────────

async function captureFirstFrameAndExit(videoEl) {
    log('info', 'capturing first frame to PNG');
    await new Promise(r => setTimeout(r, 400));
    const w = videoEl.videoWidth  || 1280;
    const h = videoEl.videoHeight ||  720;
    const canvas = document.createElement('canvas');
    canvas.width = w; canvas.height = h;
    canvas.getContext('2d').drawImage(videoEl, 0, 0, w, h);
    const blob = await new Promise(r => canvas.toBlob(r, 'image/png'));
    const bytes = new Uint8Array(await blob.arrayBuffer());
    await ipcRenderer.invoke('save-frame', { bytes: Array.from(bytes), ext: 'png', width: w, height: h });
    clearInterval(keepaliveTimer);
    ipcRenderer.send('done', true, { width: w, height: h });
}

// ── shutdown ────────────────────────────────────────────────────────────────

function stopSession() {
    try { if (frameCallbackHandle) cancelAnimationFrame(frameCallbackHandle); } catch {}
    try { if (keepaliveTimer) clearInterval(keepaliveTimer); } catch {}
    try { gamepad?.detach(); } catch {}
    try { player?.destroy(); } catch {}
    gamepad = null; player = null; stream = null;
}

// ── mode entry ──────────────────────────────────────────────────────────────

if (BRIDGE) {
    ipcRenderer.on('cmd', (_e, cmd) => {
        log('info', `cmd received: ${cmd?.cmd}`);
        try {
            if (cmd.cmd === 'start') {
                startSession({
                    mode: cmd.mode || 'home',
                    consoleId: cmd.consoleId || '',
                    titleId: cmd.titleId || '',
                    msal: cmd.msal || qs.get('msal') || '',
                }).catch(e => log('error', 'startSession:', e.message || e));
            } else if (cmd.cmd === 'stop') {
                log('info', 'stop command');
                stopSession();
            } else if (cmd.cmd === 'input') {
                applyInput(cmd);
            }
        } catch (e) { log('error', 'cmd handler:', e.message || e); }
    });
    log('info', 'renderer bridge mode — waiting for start command');
    ipcRenderer.send('bridge-ready');
} else {
    startSession({
        mode: qs.get('mode') || 'home',
        consoleId: qs.get('consoleId') || '',
        titleId: qs.get('titleId') || '',
        msal: qs.get('msal') || '',
    }).catch(e => {
        log('error', 'spike failed:', e.message || e);
        ipcRenderer.send('done', false, String(e));
    });
}
