# Package xDrive 3K for the Labs marketplace.
#
# Output: dist/xdrive3k.zip — a flat archive the client extracts directly into
# cv-scripts/. Entry point is xDrive3K.py (matches manifest "filename" field).
#
# Files included:
#   xDrive3K.py             — main UI / shot dispatcher
#   xdrive3k_cv.py          — calibrate dialog + CV detector
#   xui.py                  — shared widget kit (display fonts, gradients, …)
#   labs_input_bridge.py    — SHM bridge for stick/button overrides
#   labs_frame_bridge.py    — SHM frame reader (CV path)
#   userdata/fonts/*.ttf    — bundled display fonts (Bebas Neue, Russo One)
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File tools/pack_xdrive3k.ps1

$ErrorActionPreference = 'Stop'

$repo    = Split-Path $PSScriptRoot -Parent
$srcRoot = Join-Path $repo 'labs-engine\scripts'
$out     = Join-Path $repo 'dist\xdrive3k.zip'

$files = @(
    'xDrive3K.py',
    'xdrive3k_cv.py',
    'xui.py',
    'labs_input_bridge.py',
    'labs_frame_bridge.py'
)

# Stage everything in a temp directory so Compress-Archive captures relative
# paths (`userdata/fonts/...`) instead of absolute ones.
$stage = Join-Path $env:TEMP ("xdrive3k-pack-" + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $stage | Out-Null
try {
    foreach ($f in $files) {
        $src = Join-Path $srcRoot $f
        if (-not (Test-Path $src)) { throw "missing source: $src" }
        Copy-Item $src (Join-Path $stage $f)
    }

    $fontsSrc = Join-Path $srcRoot 'userdata\fonts'
    if (Test-Path $fontsSrc) {
        $fontsDst = Join-Path $stage 'userdata\fonts'
        New-Item -ItemType Directory -Force -Path $fontsDst | Out-Null
        Copy-Item (Join-Path $fontsSrc '*.ttf') $fontsDst
    }

    if (Test-Path $out) { Remove-Item $out -Force }
    New-Item -ItemType Directory -Force -Path (Split-Path $out) | Out-Null
    Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $out -CompressionLevel Optimal

    $size = (Get-Item $out).Length
    "[xdrive3k-pack] wrote $out  ({0:N1} KB)" -f ($size / 1KB)
}
finally {
    Remove-Item $stage -Recurse -Force -ErrorAction SilentlyContinue
}
