; installer.nsi — NSIS script for Labs Engine distribution.
;
; Invoked by package.ps1 with:
;   makensis /DSRCDIR=<staged-portable-dir> /DOUTDIR=<dist-dir> installer.nsi
;
; Produces LabsEngineSetup.exe in OUTDIR.
;
; Tested with NSIS 3.09+.

!ifndef SRCDIR
  !error "SRCDIR must be passed via /DSRCDIR=... (path to LabsEngine-portable)"
!endif
!ifndef OUTDIR
  !error "OUTDIR must be passed via /DOUTDIR=..."
!endif

!include "MUI2.nsh"
!include "LogicLib.nsh"

Name         "Labs Engine"
OutFile      "${OUTDIR}\LabsEngineSetup.exe"
InstallDir   "$PROGRAMFILES64\Labs Engine"
InstallDirRegKey HKLM "Software\Labs Engine" "InstallDir"
RequestExecutionLevel admin
Unicode      true
SetCompressor /SOLID lzma

; ── UI ───────────────────────────────────────────────────────────────────────
!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\LabsEngine.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch Labs Engine"
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

; ── Install ──────────────────────────────────────────────────────────────────
Section "Labs Engine (required)" SEC_CORE
  SectionIn RO
  SetOutPath "$INSTDIR"
  File /r "${SRCDIR}\*.*"

  WriteRegStr HKLM "Software\Labs Engine" "InstallDir" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LabsEngine" \
    "DisplayName"     "Labs Engine"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LabsEngine" \
    "DisplayIcon"     "$INSTDIR\LabsEngine.exe,0"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LabsEngine" \
    "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LabsEngine" \
    "Publisher"       "TM Labs"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LabsEngine" \
    "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LabsEngine" \
    "NoRepair" 1
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  CreateDirectory "$SMPROGRAMS\Labs Engine"
  CreateShortcut  "$SMPROGRAMS\Labs Engine\Labs Engine.lnk"  "$INSTDIR\LabsEngine.exe"
  CreateShortcut  "$SMPROGRAMS\Labs Engine\Uninstall.lnk"    "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Desktop shortcut" SEC_DESKTOP
  CreateShortcut "$DESKTOP\Labs Engine.lnk" "$INSTDIR\LabsEngine.exe"
SectionEnd

Section "Install ViGEmBus driver (Xbox mode support)" SEC_VIGEM
  ; ViGEmBus is a one-time kernel driver install. Silent + no-restart flags
  ; are honored by the 1.22.0 installer; user sees a brief UAC prompt.
  IfFileExists "$INSTDIR\third-party\ViGEmBus\ViGEmBus_Setup.exe" 0 +2
    ExecWait '"$INSTDIR\third-party\ViGEmBus\ViGEmBus_Setup.exe" /S'
SectionEnd

; ── Uninstall ────────────────────────────────────────────────────────────────
Section "Uninstall"
  Delete "$DESKTOP\Labs Engine.lnk"
  Delete "$SMPROGRAMS\Labs Engine\Labs Engine.lnk"
  Delete "$SMPROGRAMS\Labs Engine\Uninstall.lnk"
  RMDir  "$SMPROGRAMS\Labs Engine"

  RMDir /r "$INSTDIR"

  DeleteRegKey HKLM "Software\Labs Engine"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LabsEngine"
SectionEnd
