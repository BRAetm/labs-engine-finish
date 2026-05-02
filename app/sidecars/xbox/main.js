// Electron sidecar. Two modes:
//
// (default, "spike") — runs the streaming flow once, dumps the first decoded
// frame to spike-first-frame.png, exits. Used to validate auth + transport.
//
// --bridge — long-lived, speaks a binary protocol over stdout/stdin to a C++
// parent (XboxStreamPlugin). stdout = packed frame headers + BGRA payload;
// stdin = newline-delimited JSON commands (start, stop, input); stderr = logs.
//
// Either way: hosts a local Express proxy that injects the xCloud Bearer token
// into /v5/* and /v6/* calls and serves the renderer. Renderer runs
// xbox-xcloud-player in a hidden BrowserWindow.

const { app, BrowserWindow, ipcMain } = require('electron');
const express = require('express');
const path = require('path');
const fs = require('fs');

// ── token cache ─────────────────────────────────────────────────────────────

async function loadCachedTokens(mode, tokensFile) {
    if (!fs.existsSync(tokensFile)) return null;
    let Msal, Xal, TokenStore;
    try {
        ({ Msal, Xal } = require('xal-node'));
        const mod = require('xal-node/dist/tokenstore');
        TokenStore = mod.default || mod;
    } catch (e) {
        console.error('[sidecar] xal-node not available, skipping cache load');
        return null;
    }

    const store = new TokenStore();
    store.load(tokensFile, true);
    const method = store.getAuthenticationMethod?.() ?? 'msal';
    const auth = method === 'xal' ? new Xal(store) : new Msal(store);

    try {
        if (!store.hasValidAuthTokens?.()) {
            if (method === 'xal') await auth.refreshTokens();
            else await auth.refreshUserToken();
        }
        const st = await auth.getStreamingTokens();
        const pick = (mode === 'cloud') ? st.xCloudToken : st.xHomeToken;
        if (!pick) {
            console.error(`[sidecar] no ${mode === 'cloud' ? 'xCloud' : 'xHome'} token on this account`);
            return null;
        }
        const region = pick.getDefaultRegion?.() ?? {};
        const d = pick.data ?? {};
        const jwt  = d.gsToken || d.GsToken || d.streamToken || d.StreamToken || d.token || null;
        const host = region.baseUri || region.BaseUri || region.uri || region.endpoint || null;
        let msal = null;
        if (mode === 'home') {
            const mt = await auth.getMsalToken?.();
            // xal-node MsalToken stores the home-streaming MSAL credential as
            // `data.lpt` (Logon Provider Token). Fallbacks for other token shapes.
            msal = mt?.data?.lpt || mt?.data?.access_token || mt?.lpt || mt?.access_token || null;
        }
        if (!jwt || !host) {
            console.error('[sidecar] cached streaming token missing jwt/host — see token-dump.json');
            return null;
        }
        return { token: jwt, host, msal: msal || '' };
    } catch (e) {
        console.error('[sidecar] cached token refresh failed:', e.message);
        return null;
    }
}

// ── args ────────────────────────────────────────────────────────────────────

function parseArgs() {
    const out = {
        token: process.env.LABS_XCLOUD_TOKEN || '',
        msal:  process.env.LABS_XCLOUD_MSAL  || '',
        host:  process.env.LABS_XCLOUD_HOST  || '',
        mode:  process.env.LABS_XCLOUD_MODE  || 'home',
        consoleId: process.env.LABS_XCLOUD_CONSOLE || '',
        titleId:   process.env.LABS_XCLOUD_TITLE   || '',
        show: !!process.env.LABS_XCLOUD_SHOW_WINDOW,
        bridge: false,
        tokensFile: process.env.LABS_XCLOUD_TOKENS_FILE
                    || path.join(__dirname, '.xbox.tokens.json'),
    };
    for (const a of process.argv.slice(2)) {
        if (a === '--bridge') { out.bridge = true; continue; }
        const m = a.match(/^--([^=]+)=(.*)$/);
        if (!m) continue;
        const [, k, v] = m;
        if (k === 'token') out.token = v;
        else if (k === 'msal' || k === 'msal-token') out.msal = v;
        else if (k === 'host' || k === 'xcloud-host') out.host = v;
        else if (k === 'mode') out.mode = v;
        else if (k === 'console') out.consoleId = v;
        else if (k === 'title') out.titleId = v;
        else if (k === 'show') out.show = v !== 'false';
        else if (k === 'tokens-file') out.tokensFile = v;
    }
    return out;
}

// ── proxy (shared) ──────────────────────────────────────────────────────────

function startProxy(cfg) {
    const server = express();
    server.use(express.json({ limit: '4mb' }));
    server.use(express.text({ limit: '4mb', type: ['application/sdp', 'text/*'] }));

    const forward = async (req, res) => {
        const target = cfg.host.replace(/\/+$/, '') + req.originalUrl;
        const t0 = Date.now();
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
            const txt = (buf.length < 600 && ct.includes('json'))
                ? '  body=' + buf.toString('utf8').replace(/\s+/g, ' ').slice(0, 400) : '';
            console.error(`[proxy] ${req.method} ${req.originalUrl} → ${upstream.status} (${Date.now()-t0}ms, ${buf.length}b)${txt}`);
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
            resolve({ server, listener, port: listener.address().port });
        });
    });
}

// ── bridge mode ─────────────────────────────────────────────────────────────
//
// Binary frame protocol on stdout:
//
//   offset  size  field
//   0       4     magic     'LXBX' (0x5842584C, little-endian uint32)
//   4       4     frameIdx  monotonic
//   8       4     width
//   12      4     height
//   16      4     stride    bytes per row
//   20      4     format    0=BGRA8, 1=RGBA8
//   24      8     timestampUs (little-endian uint64)
//   32      4     payloadLen
//   36      ...   payload (payloadLen bytes)
//
// Commands on stdin (newline-delimited JSON):
//   {"cmd":"start","mode":"home|cloud","consoleId":"...","titleId":"..."}
//   {"cmd":"stop"}
//   {"cmd":"input","slot":0,"buttons":0x1000,"leftX":0,"leftY":0,"rightX":0,"rightY":0,"leftTrigger":0,"rightTrigger":0}

const FRAME_MAGIC = 0x5842584C;

function redirectLogsToStderr() {
    for (const m of ['log', 'info', 'debug', 'warn']) {
        const orig = console[m];
        console[m] = (...args) => console.error(...args);
        orig;
    }
}

function writeFrameToStdout(meta, pixels) {
    const hdr = Buffer.alloc(36);
    hdr.writeUInt32LE(FRAME_MAGIC, 0);
    hdr.writeUInt32LE(meta.frameIdx >>> 0, 4);
    hdr.writeUInt32LE(meta.width, 8);
    hdr.writeUInt32LE(meta.height, 12);
    hdr.writeUInt32LE(meta.stride, 16);
    hdr.writeUInt32LE(meta.format, 20);
    hdr.writeBigUInt64LE(BigInt(meta.timestampUs || Date.now() * 1000), 24);
    hdr.writeUInt32LE(pixels.length, 32);

    // Two writes — Node coalesces at the pipe boundary. Backpressure handled
    // by the stream; if stdout is full the writes queue internally.
    process.stdout.write(hdr);
    process.stdout.write(pixels);
}

// Control channel is a TCP server on 127.0.0.1, ephemeral port. The port is
// announced on stderr ("[sidecar] control-port=<n>") so the parent can connect.
// Stdin is NOT used — Electron-on-Windows emits stdin EOF spuriously when the
// parent is a GUI app, which tears down the renderer mid-load.
function setupControlServer(onCommand) {
    const net = require('net');
    return new Promise(resolve => {
        let activeSocket = null;
        const server = net.createServer(socket => {
            console.error('[sidecar] control client connected');
            activeSocket = socket;
            let buf = '';
            socket.setEncoding('utf8');
            socket.on('data', chunk => {
                buf += chunk;
                let idx;
                while ((idx = buf.indexOf('\n')) !== -1) {
                    const line = buf.slice(0, idx).trim();
                    buf = buf.slice(idx + 1);
                    if (!line) continue;
                    try { onCommand(JSON.parse(line)); }
                    catch (e) { console.error('[sidecar] bad control line:', line.slice(0, 120)); }
                }
            });
            socket.on('close', () => { console.error('[sidecar] control client disconnected'); activeSocket = null; });
            socket.on('error', e => console.error('[sidecar] control sock err:', e.message));
        });
        server.listen(0, '127.0.0.1', () => {
            const port = server.address().port;
            console.error(`[sidecar] control-port=${port}`);
            resolve({ port, server });
        });
    });
}

async function runBridgeMode(cfg) {
    redirectLogsToStderr();
    console.error('[sidecar] bridge mode');

    const { port } = await startProxy(cfg);
    const win = new BrowserWindow({
        show: cfg.show,
        width: 1280,
        height: 720,
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: false,
        },
    });

    win.webContents.on('render-process-gone', (_e, d) => {
        console.error('[sidecar] render gone:', d);
        app.exit(3);
    });

    ipcMain.on('log', (_e, level, msg) => console.error(`[renderer:${level}] ${msg}`));
    ipcMain.on('bridge-frame', (_e, meta, bytes) => {
        // bytes arrives as Uint8Array after structured clone.
        writeFrameToStdout(meta, Buffer.from(bytes.buffer, bytes.byteOffset, bytes.byteLength));
    });
    let rendererReady = false;
    const queuedCommands = [];
    ipcMain.on('bridge-ready', () => {
        console.error(`[sidecar] bridge-ready received, flushing ${queuedCommands.length} queued cmd(s)`);
        for (const c of queuedCommands) win.webContents.send('cmd', c);
        queuedCommands.length = 0;
        rendererReady = true;
    });

    await setupControlServer((cmd) => {
        console.error(`[sidecar] control cmd ${cmd.cmd} (rendererReady=${rendererReady})`);
        if (rendererReady) win.webContents.send('cmd', cmd);
        else queuedCommands.push(cmd);
    });

    const qs = new URLSearchParams({
        mode: cfg.mode,
        consoleId: cfg.consoleId,
        titleId: cfg.titleId,
        msal: cfg.msal,
        bridge: '1',
    });
    try {
        await win.loadURL(`http://127.0.0.1:${port}/renderer.html?${qs.toString()}`);
    } catch (e) {
        console.error('[sidecar] renderer loadURL failed:', e.message || e);
    }
}

// ── spike mode (first-frame-to-PNG validation) ──────────────────────────────

async function runSpikeMode(cfg) {
    const { port } = await startProxy(cfg);
    console.log(`[sidecar] proxy on http://127.0.0.1:${port}`);

    const win = new BrowserWindow({
        show: cfg.show,
        width: 1280,
        height: 720,
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: false,
        },
    });

    win.webContents.on('render-process-gone', (_e, d) => {
        console.error('[sidecar] render gone:', d);
        app.exit(3);
    });

    ipcMain.on('log', (_e, level, msg) => {
        if (level === 'error') console.error(`[renderer:${level}] ${msg}`);
        else console.log(`[renderer:${level}] ${msg}`);
    });
    ipcMain.handle('save-frame', (_e, { bytes, ext, width, height }) => {
        const out = path.join(process.cwd(), `spike-first-frame.${ext || 'png'}`);
        fs.writeFileSync(out, Buffer.from(bytes));
        console.log(`[sidecar] first frame → ${out}  (${width}x${height}, ${bytes.length} bytes)`);
        return out;
    });
    ipcMain.on('done', (_e, ok, summary) => {
        console.log(`[sidecar] renderer done, ok=${ok}`, summary || '');
        setTimeout(() => app.exit(ok ? 0 : 4), 250);
    });

    const qs = new URLSearchParams({
        mode: cfg.mode,
        consoleId: cfg.consoleId,
        titleId: cfg.titleId,
        msal: cfg.msal,
    });
    try {
        await win.loadURL(`http://127.0.0.1:${port}/renderer.html?${qs.toString()}`);
    } catch (e) {
        console.error('[sidecar] renderer loadURL failed:', e.message || e);
    }
}

// ── entry ───────────────────────────────────────────────────────────────────

app.whenReady().then(async () => {
    const cfg = parseArgs();

    if (!cfg.token || !cfg.host) {
        if (!cfg.bridge) console.log('[sidecar] --token/--host not supplied, attempting token cache...');
        const cached = await loadCachedTokens(cfg.mode, cfg.tokensFile);
        if (cached) {
            cfg.token = cfg.token || cached.token;
            cfg.host  = cfg.host  || cached.host;
            cfg.msal  = cfg.msal  || cached.msal;
            if (!cfg.bridge) console.log('[sidecar] loaded tokens from .xbox.tokens.json');
        }
    }

    if (!cfg.token) {
        console.error('[sidecar] no xCloud token. Run `npm run auth` or pass --token=<Bearer>.');
        app.exit(2);
        return;
    }
    if (!cfg.host) {
        console.error('[sidecar] no regional host. Run `npm run auth` or pass --host=<https://...xboxlive.com>.');
        app.exit(2);
        return;
    }
    if (cfg.mode === 'home' && !cfg.msal) {
        console.error('[sidecar] WARN: home mode without MSAL token — console auth will fail.');
    }
    if (cfg.mode === 'cloud' && !cfg.titleId) {
        console.error('[sidecar] cloud mode requires --title=<titleId>.');
        app.exit(2);
        return;
    }

    if (cfg.bridge) await runBridgeMode(cfg);
    else            await runSpikeMode(cfg);
});

app.on('window-all-closed', () => app.quit());
