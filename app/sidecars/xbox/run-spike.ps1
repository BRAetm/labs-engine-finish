# Spike launcher.
#
# First-time setup (once per account):
#   npm run auth           # MSAL device-code login, caches tokens
#
# Then:
#   ./run-spike.ps1                              # home streaming, auto-pick console
#   ./run-spike.ps1 -Mode cloud -Title <id>      # xCloud game streaming
#   ./run-spike.ps1 -Show                        # show the hidden renderer window
#
# Env overrides (skip the token cache, paste your own):
#   $env:LABS_XCLOUD_TOKEN, LABS_XCLOUD_HOST, LABS_XCLOUD_MSAL, LABS_XCLOUD_CONSOLE

param(
    [switch]$Show,
    [string]$Mode = 'home',
    [string]$Console = '',
    [string]$Title = ''
)

$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot

if ($Show)    { $env:LABS_XCLOUD_SHOW_WINDOW = "1" }
if ($Mode)    { $env:LABS_XCLOUD_MODE    = $Mode }
if ($Console) { $env:LABS_XCLOUD_CONSOLE = $Console }
if ($Title)   { $env:LABS_XCLOUD_TITLE   = $Title }

if (-not (Test-Path "./.xbox.tokens.json") -and -not $env:LABS_XCLOUD_TOKEN) {
    Write-Host "No token cache found. Run `npm run auth` first to sign in, or set LABS_XCLOUD_TOKEN + LABS_XCLOUD_HOST."
    exit 1
}

npx electron . --enable-logging
