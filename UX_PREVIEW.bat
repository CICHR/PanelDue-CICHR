@echo off
setlocal
cd /d "%~dp0"
if not exist "preview\index.html" (
  echo ERROR: preview\index.html not found.
  pause
  exit /b 1
)
start "PanelDue CICHR Preview" "%~dp0preview\index.html"
exit /b 0
