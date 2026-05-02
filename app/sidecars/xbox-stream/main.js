// Headless Greenlight-based Xbox Remote Play / xCloud sidecar.
//
// Speaks the LBSX framed-pipe protocol (see README.md) on stdin/stdout to the
// parent Qt plugin (XboxStreamPlugin). All log output goes to stderr.
//
// Architecture:
//   - One hidden BrowserWindow runs renderer.js
//   - renderer.js drives xbox-xcloud-player → WebRTC → captures the inbound
//     video track, extracts H.264 NALs via WebCodecs (MediaStreamTrackProcessor
//     + VideoDecoder description fall-back, see renderer.js for details)
//   - main.js framepacks the NAL chunks onto stdout
//
// CLI args:
//   --data-dir <path>        Where to cache token JSON (DPAPI envelope written
//                            by the parent Qt plugin; sidecar reads cleartext).
//   --account-key <key>      Stable per-account scoping key (default: "default").
//                            Token cache + Electron userData are namespaced by
//                            this so multiple sidecar instances can run side by
//                            side without colliding on tokens or WebRTC state.
//                            Effective dirs:
//                              tokens:    <data-dir>/sessions/<key>/xbox-tokens.json
//                              userData:  <data-dir>/sessions/<key>/electron
//   --mode <home|cloud>      Required.
//   --console-id <id>        Required for home mode (parent passes after picker).
//   --tokens-file <path>     Override token JSON path (default derives from
//                            --data-dir + --account-key as above).
//   --show                   For debugging — show the BrowserWindow.

const { app, BrowserWindow, ipcMain } = require('electron');
const express = require('express');
const path = require('path');
const fs = require('fs');

// ── arg parsing ─────────────────────────────────────────────────────────────

function parseArgs() {
    const out = {
        dataDir:    process.env.LABS_XBOX_DATA_DIR || '',
        accountKey: process.env.LABS_XBOX_ACCOUNT_KEY || '',
        tokensFile: '',
        sessionDir: '',
        userDataDir:'',
        mode:       process.env.LABS_XBOX_MODE || 'home',
        consoleId:  process.env.LABS_XBOX_CONSOLE || '',
        titleId:    process.env.LABS_XBOX_TITLE   || '',
        show:       !!process.env.LABS_XBOX_SHOW,
    };
    // Electron forwards every Chromium switch through process.argv. Skip the
    // first positional (the script path) when launched via `electron main.js`.
    const argv = process.argv.slice(1);
    for (const a of argv) {
        const m = a.match(/^--([^=]+)(?:=(.*))?$/);
        if (!m) continue;
        const [, k, v] = m;
        switch (k) {
            case 'data-dir':    out.dataDir    = v; break;
            case 'account-key': out.accountKey = v; break;
            case 'tokens-file': out.tokensFile = v; break;
            case 'mode':        out.mode       = v; break;
            case 'console-id':  out.consoleId  = v; break;
            case 'title-id':    out.titleId    = v; break;
            case 'show':        out.show       = (v !== 'false'); break;
        }
    }
    // Sanitise accountKey to a filesystem-safe slug; default to "default" for
    // back-compat with single-account installs that don't pass --account-key.
    out.accountKey = (out.accountKey || 'default').replace(/[^A-Za-z0-9._-]/g, '_');

    if (out.dataDir) {
        out.sessionDir  = path.join(out.dataDir, 'sessions', out.accountKey);
        out.userDataDir = path.join(out.sessionDir, 'electron');
        if (!out.tokensFile) out.tokensFile = path.join(out.sessionDir, 'xbox-tokens.json');
        try { fs.mkdirSync(out.sessionDir,  { recursive: true }); } catch {}
        try { fs.mkdirSync(out.userDataDir, { recursive: true }); } catch {}
    } else if (!out.tokensFile) {
        out.tokensFile = path.join(__dirname, '.xbox.tokens.json');
    }
    return out;
}

// Parse args at module load. Per-account `userData` MUST be set before
// `app.whenReady()` resolves — otherwise Chromium has already initialised the
// default profile and concurrent sidecars deadlock on the user-data lock file.
const CFG = parseArgs();
if (CFG.userDataDir) {
    try { app.setPath('userData', CFG.userDataDir); }
    catch (e) { console.error('[sidecar] setPath(userData) failed:', e.message || e); }
}

// ── LBSX framing ────────────────────────────────────────────────────────────
//
//   [4-byte magic 'LBSX'][1-byte type][4-byte length BE][payload...]
//
// Outbound types: 0x01 JSON event, 0x02 H.264 NAL chunk, 0x03 Opus chunk
// Inbound types:  0x10 JSON command, 0x11 Gamepad state JSON

const MAGIC = Buffer.from([0x4C, 0x42, 0x53, 0x58]); // 'LBSX'
const T_EVENT_JSON = 0x01;
const T_VIDEO_NAL  = 0x02;
const T_AUDIO_OPUS = 0x03;
const T_CMD_JSON   = 0x10;
const T_GAMEPAD    = 0x11;

function writeFrame(type, payload) {
    if (!Buffer.isBuffer(payload)) payload = Buffer.from(payload);
    const hdr = Buffer.alloc(9);
    MAGIC.copy(hdr, 0);
    hdr.writeUInt8(type, 4);
    hdr.writeUInt32BE(payload.length, 5);
    process.stdout.write(hdr);
    process.stdout.write(payload);
}

function emitEvent(obj) {
    writeFrame(T_EVENT_JSON, Buffer.from(JSON.stringify(obj), 'utf8'));
}

function emitVideoNal(buf) {
    writeFrame(T_VIDEO_NAL, buf);
}

// Stdin parser: reads framed messages from the parent.
class StdinParser {
    constructor(onMsg) {
        this.buf = Buffer.alloc(0);
        this.onMsg = onMsg;
    }
    feed(chunk) {
        this.buf = this.buf.length ? Buffer.concat([this.buf, chunk]) : chunk;
        for (;;) {
            if (this.buf.length < 9) return;
            // Resync on magic if needed.
            if (this.buf.compare(MAGIC, 0, 4, 0, 4) !== 0) {
                const idx = this.buf.indexOf(MAGIC, 1);
                if (idx < 0) { this.buf = Buffer.alloc(0); return; }
                this.buf = this.buf.slice(idx);
                continue;
            }
            const type = this.buf.readUInt8(4);
            const len  = this.buf.readUInt32BE(5);
            if (len > 16 * 1024 * 1024) {
                console.error('[sidecar] implausible inbound payload', len);
                this.buf = this.buf.slice(9);
                continue;
            }
            if (this.buf.length < 9 + len) return;
            const payload = this.buf.slice(9, 9 + len);
            this.buf = this.buf.slice(9 + len);
            try { this.onMsg(type, payload); }
            catch (e) { console.error('[sidecar] stdin handler:', e.message || e); }
        }
    }
}

// ── token cache (reuses xal-node, mirrors existing app/sidecars/xbox) ───────

async function loadCachedTokens(mode, tokensFile) {
    if (!fs.existsSync(tokensFile)) return null;
    let Msal, Xal, TokenStore;
    try {
        ({ Msal, Xal } = require('xal-node'));
        const mod = require('xal-node/dist/tokenstore');
        TokenStore = mod.default || mod;
    } catch (e) {
        console.error('[sidecar] xal-node missing — cannot load tokens');
        return null;
    }
    const store = new TokenStore();
    store.load(tokensFile, true);
    const method = store.getAuthenticationMethod?.() ?? 'msal';
    const auth = method === 'xal' ? new Xal(store) : new Msal(store);
    try {
        if (!store.hasValidAuthTokens?.()) {
            if (method === 'xal') await auth.refreshTokens();
            else                  await auth.refreshUserToken();
        }
        const st = await auth.getStreamingTokens();
        const pick = (mode === 'cloud') ? st.xCloudToken : st.xHomeToken;
        if (!pick) return null;
        const region = pick.getDefaultRegion?.() ?? {};
        const d = pick.data ?? {};
        const jwt  = d.gsToken || d.GsToken || d.streamToken || d.StreamToken || d.token || null;
        const host = region.baseUri || region.BaseUri || region.uri || region.endpoint || null;
        let msal = null;
        if (mode === 'home') {
            const mt = await auth.getMsalToken?.();
            msal = mt?.data?.lpt || mt?.data?.access_token || mt?.lpt || mt?.access_token || null;
        }
        if (!jwt || !host) return null;
        return { token: jwt, host, msal: msal || '' };
    } catch (e) {
        console.error('[sidecar] token refresh failed:', e.message);
        return null;
    }
}

// ── token-bearing proxy (auth header injection for the renderer) ────────────

function startProxy(cfg) {
    const server = express();
    server.use(express.json({ limit: '4mb' }));
    server.use(express.text({ limit: '4mb', type: ['application/sdp', 'text/*'] }));

    const forward = async (req, res) => {
        const target = cfg.host.replace(/\/+$/, '') + req.originalUrl;
        const headers = {
            'Accept': req.headers['accept'] || 'application/json',
            'Content-Type': req.headers['content-type'] || 'application/json',
            'X-Gssv-Client': req.headers['x-gssv-client'] || 'XboxComBrowser',
            'X-MS-Device-Info': req.headers['x-ms-device-info'] || '',
            'Authorization': 'Bearer ' + cfg.token,
        };
        let body;
        if (req.method !== 'GET' && req.method !== 'DELETE') {
            body = (typeof req.body === 'string') ? req.body : JSON.stringify(req.body ?? {});
        }
        try {
            const upstream = await fetch(target, { method: req.method, headers, body });
            const ct = upstream.headers.get('content-type') || 'application/octet-stream';
            const buf = Buffer.from(await upstream.arrayBuffer());
            res.status(upstream.status).set('content-type', ct).send(buf);
        } catch (e) {
            console.error(`[proxy] ${req.method} ${req.originalUrl} → ERR ${e.message}`);
            res.status(502).json({ proxyError: e.message, target });
        }
    };

    server.all('/v5/*', forward);
    server.all('/v6/*', forward);
    server.use(express.static(__dirname));

    return new Promise(resolve => {
        const listener = server.listen(0, '127.0.0.1', () => {
            resolve({ port: listener.address().port });
        });
    });
}

// ── main ────────────────────────────────────────────────────────────────────

let win = null;
let rendererReady = false;
const queuedRendererMsgs = [];

function sendRenderer(channel, msg) {
    if (!win) return;
    if (rendererReady) win.webContents.send(channel, msg);
    else queuedRendererMsgs.push([channel, msg]);
}

app.whenReady().then(async () => {
    const cfg = CFG;
    emitEvent({
        type: 'hello',
        version: 1,
        mode: cfg.mode,
        accountKey: cfg.accountKey,
        tokensFile: cfg.tokensFile,
    });

    // ── Cloud mode: VISIBLE browser at xbox.com/play ──────────────────────
    // User signs in directly on the page. Per-account Electron userData
    // isolates cookies. Frame capture is done by the parent via WGC against
    // the window HWND we report below — no headless Greenlight engine.
    if (cfg.mode === 'cloud') {
        // Open with show:false so the window never flashes on the desktop —
        // the parent (Qt plugin) reparents it via Win32 SetParent into a
        // Labs Engine widget, then sets WS_VISIBLE inside that container.
        win = new BrowserWindow({
            show: false,
            frame: false,                     // strip OS frame so reparent looks clean
            width: 1280,
            height: 800,
            title: `Labs Engine — xCloud (${cfg.accountKey})`,
            autoHideMenuBar: true,
            backgroundColor: '#070A14',
            webPreferences: {
                contextIsolation: true,
                nodeIntegration: false,
            },
        });
        // Standard desktop UA so xbox.com/play serves the full PC experience.
        win.webContents.setUserAgent(
            'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 ' +
            '(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36 Edg/120.0.0.0'
        );
        // The HWND is valid as soon as BrowserWindow is constructed (even with
        // show:false). Report it immediately so the parent can reparent before
        // any layout/paint happens.
        const reportHwnd = () => {
            try {
                const hbuf = win.getNativeWindowHandle();
                let hwnd = 0n;
                if (hbuf.length >= 8)      hwnd = hbuf.readBigUInt64LE(0);
                else if (hbuf.length >= 4) hwnd = BigInt(hbuf.readUInt32LE(0));
                emitEvent({ type: 'window-hwnd', hwnd: hwnd.toString(), title: win.getTitle() });
            } catch (e) {
                emitEvent({ type: 'log', level: 'warn', message: 'getNativeWindowHandle failed: ' + (e.message || e) });
            }
        };
        reportHwnd();  // fire immediately, parent reparents before we navigate

        // Do NOT app.exit on closed — once reparented, "closed" can fire
        // spuriously when the host widget re-creates / restyles. Lifetime is
        // owned by the parent via the LBSX pipe (cmd:"stop"). Only log it.
        win.on('closed', () => {
            emitEvent({ type: 'log', level: 'info', message: 'browserwindow closed event' });
        });
        try {
            await win.loadURL('https://www.xbox.com/play');
        } catch (e) {
            emitEvent({ type: 'error', code: 'load-failed', message: String(e.message || e) });
        }
        // Set up stdin parser; gamepad frames forwarded as a JS shim into the page.
        setupStdinSession(cfg);
        return;
    }

    // ── Home mode: headless Greenlight engine (existing path) ─────────────
    // Token bootstrap. If no valid tokens yet, the parent must drive the
    // sign-in flow in its own WebView2 dialog and write tokens to
    // cfg.tokensFile before starting us. We just emit an error event here.
    const cached = await loadCachedTokens(cfg.mode, cfg.tokensFile);
    if (!cached) {
        emitEvent({ type: 'error', code: 'no-tokens',
                    message: `No streaming tokens for mode=${cfg.mode}. Sign in via the Labs Xbox dialog.`,
                    tokensFile: cfg.tokensFile });
        // Stay alive so the parent can read the event and shut us down cleanly.
        return setupStdinNoSession();
    }
    cfg.token = cached.token;
    cfg.host  = cached.host;
    cfg.msal  = cached.msal;
    emitEvent({ type: 'tokens-ok', host: cfg.host });

    const { port } = await startProxy(cfg);

    win = new BrowserWindow({
        show: cfg.show,
        width: 1280,
        height: 720,
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: false,
        },
    });
    win.webContents.on('render-process-gone', (_e, d) => {
        emitEvent({ type: 'error', code: 'renderer-crash', detail: d });
        app.exit(3);
    });

    ipcMain.on('log', (_e, level, msg) => {
        console.error(`[renderer:${level}] ${msg}`);
        emitEvent({ type: 'log', level, message: String(msg) });
    });
    ipcMain.on('event', (_e, evt) => emitEvent(evt));
    ipcMain.on('nal', (_e, bytes) => {
        emitVideoNal(Buffer.from(bytes.buffer, bytes.byteOffset, bytes.byteLength));
    });
    ipcMain.on('renderer-ready', () => {
        rendererReady = true;
        for (const [ch, m] of queuedRendererMsgs) win.webContents.send(ch, m);
        queuedRendererMsgs.length = 0;
        emitEvent({ type: 'renderer-ready' });
    });

    setupStdinSession(cfg);

    const qs = new URLSearchParams({
        mode:      cfg.mode,
        consoleId: cfg.consoleId,
        titleId:   cfg.titleId,
        msal:      cfg.msal || '',
    });
    try {
        await win.loadURL(`http://127.0.0.1:${port}/index.html?${qs.toString()}`);
    } catch (e) {
        emitEvent({ type: 'error', code: 'load-failed', message: String(e.message || e) });
    }
});

// Stdin parser when we're alive enough to dispatch commands to the renderer.
function setupStdinSession(cfg) {
    const parser = new StdinParser((type, payload) => {
        if (type === T_CMD_JSON) {
            let cmd;
            try { cmd = JSON.parse(payload.toString('utf8')); }
            catch { return; }
            handleCommand(cmd, cfg);
        } else if (type === T_GAMEPAD) {
            // Forward verbatim to renderer for WebRTC data-channel input.
            try {
                const st = JSON.parse(payload.toString('utf8'));
                sendRenderer('gamepad', st);
            } catch {}
        }
    });
    process.stdin.on('data', c => parser.feed(c));
    process.stdin.on('end',  () => app.quit());
    process.stdin.on('error', () => app.quit());
}

// Drained stdin loop for the no-tokens / error case so we can still hear "stop".
function setupStdinNoSession() {
    const parser = new StdinParser((type, payload) => {
        if (type !== T_CMD_JSON) return;
        let cmd; try { cmd = JSON.parse(payload.toString('utf8')); } catch { return; }
        if (cmd?.cmd === 'stop' || cmd?.cmd === 'shutdown') app.quit();
    });
    process.stdin.on('data', c => parser.feed(c));
    process.stdin.on('end',  () => app.quit());
}

function handleCommand(cmd, cfg) {
    if (!cmd || typeof cmd !== 'object') return;
    switch (cmd.cmd) {
        case 'ping':
            emitEvent({ type: 'pong', ts: Date.now() });
            break;
        case 'list_consoles':
            sendRenderer('list_consoles', {});
            break;
        case 'start_stream':
            sendRenderer('start_stream', {
                mode:      cmd.mode      || cfg.mode,
                consoleId: cmd.consoleId || cfg.consoleId || '',
                titleId:   cmd.titleId   || cfg.titleId   || '',
                msal:      cfg.msal      || '',
            });
            break;
        case 'stop_stream':
            sendRenderer('stop_stream', {});
            break;
        case 'shutdown':
        case 'stop':
            sendRenderer('stop_stream', {});
            setTimeout(() => app.quit(), 250);
            break;
    }
}

app.on('window-all-closed', () => app.quit());
