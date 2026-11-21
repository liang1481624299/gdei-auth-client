@echo off
rem ============================================================
rem  GDEI campus network auto-login - Windows launcher
rem  Double-click = open GUI; with argument = console action
rem    login.bat            GUI (default, calls portal.ps1)
rem    login.bat login      online
rem    login.bat logout     offline
rem    login.bat status     check status
rem  Prefers PowerShell 7 (pwsh); falls back to Windows PowerShell 5.1.
rem ============================================================
setlocal
set "DIR=%~dp0"

set "PS=powershell"
where pwsh >nul 2>nul
if not errorlevel 1 set "PS=pwsh"

if "%~1"=="" (
    "%PS%" -NoProfile -ExecutionPolicy Bypass -File "%DIR%portal.ps1" -Action gui
) else (
    "%PS%" -NoProfile -ExecutionPolicy Bypass -File "%DIR%portal.ps1" -Action %1
)

endlocal
