// Reserved for future use. The current renderer uses nodeIntegration to keep
// the diff small; if/when the plugin migrates to contextIsolation:true, this
// preload would expose `window.labsBridge` with the IPC channels in
// renderer.js.
