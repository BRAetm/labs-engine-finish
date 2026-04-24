# package.ps1 - stage the full Labs Engine distribution.
#
# Outputs under dist/:
#   dist/LabsEngine-portable/           ready-to-run folder
#   dist/LabsEngine-portable.zip        portable folder zipped
#   dist/LabsEngineSetup.exe            NSIS installer (if makensis available)
#
# Usage: powershell -ExecutionPolicy Bypass -File app/scripts/package.ps1
param(
    [switch]$NoZip,
    [switch]$NoInstaller
)

$ErrorActionPreference = 'Stop'

$root     = (Resolve-Path "$PSScriptRoot\..\..").Path
$bin      = Join-Path $root "app\build\msvc-release\bin"
$dist     = Join-Path $root "dist"
$portable = Join-Path $dist "LabsEngine-portable"

if (!(Test-Path $bin)) {
    throw "Build output missing: $bin. Run build.ps1 -Release first."
}

Write-Host "[package] staging to $portable"
Remove-Item -Recurse -Force $dist -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $portable | Out-Null

# 1. Core binaries.
Write-Host "[package] copying binaries"
Copy-Item -Recurse -Force "$bin\*" $portable

# 2. Default scripts.
$scriptsDst = Join-Path $portable "scripts"
$cvDst      = Join-Path $portable "cv-scripts"
New-Item -ItemType Directory -Force -Path $scriptsDst | Out-Null
New-Item -ItemType Directory -Force -Path $cvDst      | Out-Null

Write-Host "[package] copying scripts"
Get-ChildItem "$root\labs-engine\scripts" -Recurse -File |
    Where-Object {
        $_.FullName -notmatch '\\__pycache__\\' -and
        $_.FullName -notmatch '\\userdata\\' -and
        $_.FullName -notmatch '\\_nba2k_main\\'
    } |
    ForEach-Object {
        $rel = $_.FullName.Substring("$root\labs-engine\scripts\".Length)
        $dst = Join-Path $scriptsDst $rel
        New-Item -ItemType Directory -Force -Path (Split-Path $dst) | Out-Null
        Copy-Item $_.FullName $dst -Force
    }

Get-ChildItem "$root\labs-engine\cv-scripts" -Recurse -File |
    Where-Object { $_.FullName -notmatch '\\__pycache__\\' } |
    ForEach-Object {
        $rel = $_.FullName.Substring("$root\labs-engine\cv-scripts\".Length)
        $dst = Join-Path $cvDst $rel
        New-Item -ItemType Directory -Force -Path (Split-Path $dst) | Out-Null
        Copy-Item $_.FullName $dst -Force
    }

# 3. Bundled third-party installers.
$tpDst = Join-Path $portable "third-party\ViGEmBus"
New-Item -ItemType Directory -Force -Path $tpDst | Out-Null
$vigemSrc = Join-Path $root "app\third-party\ViGEmBus\ViGEmBus_Setup.exe"
if (Test-Path $vigemSrc) { Copy-Item $vigemSrc $tpDst -Force }

# 4. README / first-run notes.
$readme = @"
Labs Engine -- PS5 Remote Play + shot-timing hub
=================================================

QUICK START
-----------
1. If you have not installed ViGEmBus before, run:
      third-party\ViGEmBus\ViGEmBus_Setup.exe
   (one-time kernel driver required for Xbox-mode virtual pad output)

2. Launch LabsEngine.exe.

3. First launch: pick your PS5 (auto-discovered on your LAN) and pair it
   with the 8-digit code shown on the PS5.

4. Click Start to begin streaming.

5. Scripts live in the left rail. Labs2K.py is the shot-timing engine.
   Click a script, then click Run.

TROUBLESHOOTING
---------------
- Controller not responding on PS5 after clicking Start:
    Plug DualSense in via USB. Bluetooth also works but USB is the
    most reliable path for the input override bridge.

- Labs2K fps stuck below 20:
    Open Labs2K, enable "Ultra-lite mode (weak CPU / laptop)" in the
    Features panel, click STOP then START again.

- Xbox mode virtual pad missing:
    Install ViGEmBus (step 1 above), then relaunch.

Feedback and issues: your distribution channel goes here.
"@
Set-Content -Path (Join-Path $portable "README.txt") -Value $readme -Encoding UTF8

# 5. Portable zip.
if (-not $NoZip) {
    $zipPath = Join-Path $dist "LabsEngine-portable.zip"
    Write-Host "[package] zipping to $zipPath"
    Compress-Archive -Path "$portable\*" -DestinationPath $zipPath -Force
    $zipMB = "{0:N1}" -f ((Get-Item $zipPath).Length / 1MB)
    Write-Host "[package] zip size: $zipMB MB"
}

# 6. NSIS installer.
if (-not $NoInstaller) {
    $nsisExe = $null
    foreach ($p in @(
        "C:\Program Files (x86)\NSIS\makensis.exe",
        "C:\Program Files\NSIS\makensis.exe"
    )) {
        if (Test-Path $p) { $nsisExe = $p; break }
    }
    if (-not $nsisExe) {
        $cmd = Get-Command makensis -ErrorAction SilentlyContinue
        if ($cmd) { $nsisExe = $cmd.Source }
    }
    if ($nsisExe) {
        $nsi = Join-Path $root "app\scripts\installer.nsi"
        if (Test-Path $nsi) {
            Write-Host "[package] compiling installer with $nsisExe"
            & $nsisExe "/DSRCDIR=$portable" "/DOUTDIR=$dist" $nsi
            if ($LASTEXITCODE -eq 0) {
                Write-Host "[package] installer -> $dist\LabsEngineSetup.exe"
            } else {
                Write-Warning "[package] NSIS returned $LASTEXITCODE -- installer NOT built"
            }
        }
    } else {
        Write-Host "[package] NSIS not found -- skipping installer build (portable zip still produced)"
        Write-Host "[package] to enable: install NSIS from https://nsis.sourceforge.io/ and re-run"
    }
}

Write-Host ""
Write-Host "[package] done -> $dist"
