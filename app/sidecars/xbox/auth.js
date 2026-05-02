// One-shot Microsoft-Live / Xbox-Live auth runner. Designed to be launched
// either interactively (human reads stdout) or from the Qt XboxSignInDialog
// (which parses the JSON progress lines on stdout).
//
//   node ./auth.js  [--tokens-file=<path>]           — device-code auth
//   node ./auth.js  show    [--tokens-file=<path>]   — print cache status
//   node ./auth.js  logout  [--tokens-file=<path>]   — wipe the cache
//
// stdout — machine-readable progress, one JSON object per line:
//   {"type":"device-code","verificationUri":"https://microsoft.com/link","userCode":"9DS55UFA","expiresIn":900}
//   {"type":"progress","message":"polling for sign-in..."}
//   {"type":"done","ok":true}
//   {"type":"done","ok":false,"error":"..."}
//
// stderr — human logs.

const path = require('path');
const fs = require('fs');

const { Msal } = require('xal-node');
const TokenStoreMod = require('xal-node/dist/tokenstore');
const TokenStore = TokenStoreMod.default || TokenStoreMod;

function parseArgs() {
    const out = { cmd: 'auth', tokensFile: path.join(__dirname, '.xbox.tokens.json') };
    let positional = 0;
    for (const a of process.argv.slice(2)) {
        const m = a.match(/^--([^=]+)=(.*)$/);
        if (m) {
            if (m[1] === 'tokens-file') out.tokensFile = m[2];
        } else if (positional === 0) {
            out.cmd = a;
            positional++;
        }
    }
    return out;
}

function emit(obj) {
    process.stdout.write(JSON.stringify(obj) + '\n');
}

function bail(err) {
    emit({ type: 'done', ok: false, error: String(err?.message || err) });
    process.exit(1);
}

async function main() {
    const { cmd, tokensFile } = parseArgs();
    console.error(`[auth] cmd=${cmd} tokens=${tokensFile}`);

    const dir = path.dirname(tokensFile);
    try { fs.mkdirSync(dir, { recursive: true }); } catch (e) { /* ok if exists */ }

    const store = new TokenStore();
    store.load(tokensFile, true);

    if (cmd === 'show') {
        const m = store.getAuthenticationMethod?.() ?? '(none)';
        emit({ type: 'status', method: m,
               userTokenValid: store._userToken?.isValid?.() ?? false,
               secondsLeft:    store._userToken?.getSecondsValid?.() ?? 0 });
        return;
    }
    if (cmd === 'logout') {
        store.removeAll?.();
        try { fs.unlinkSync(tokensFile); } catch {}
        emit({ type: 'done', ok: true });
        return;
    }
    if (cmd !== 'auth') return bail(`unknown command: ${cmd}`);

    const msal = new Msal(store);

    if (store.hasValidAuthTokens?.()) {
        console.error('[auth] already signed in, fetching streaming tokens...');
    } else {
        const dc = await msal.doDeviceCodeAuth();
        emit({
            type: 'device-code',
            verificationUri: dc.verification_uri || 'https://microsoft.com/link',
            userCode: dc.user_code || '',
            expiresIn: dc.expires_in || 900,
            fullMessage: dc.message || '',
        });

        try {
            await msal.doPollForDeviceCodeAuth(dc.device_code, (dc.expires_in || 900) * 1000);
        } catch (e) {
            return bail(e);
        }
        emit({ type: 'progress', message: 'signed in, refreshing tokens' });
        await msal.refreshUserToken();
    }

    try {
        emit({ type: 'progress', message: 'fetching streaming tokens' });
        await msal.getStreamingTokens();
    } catch (e) {
        console.error('[auth] getStreamingTokens failed:', e?.message || e);
        // Not fatal — tokens file is still useful; bridge will try again.
    }

    emit({ type: 'done', ok: true });
}

main().catch(bail);
