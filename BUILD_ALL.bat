@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"
set "VERSION_FILE=src\Version.hpp"
set "FW_VERSION="
for /f "tokens=3" %%V in ('findstr /C:"#define VERSION_MAIN" "%VERSION_FILE%"') do set "FW_VERSION=%%~V"
if not defined FW_VERSION (
  echo ERROR: VERSION_MAIN not found in %VERSION_FILE%.
  pause
  exit /b 1
)
title PanelDue CICHR v%FW_VERSION% - All displays
cls

echo ============================================================
echo   PanelDue CICHR v%FW_VERSION% - ALL SUPPORTED TARGETS
echo ============================================================
echo.

set "TARGETS=v2-4.3 v2-5.0 v2-7.0 v2-7.0c v3-4.3 v3-5.0 v3-7.0 v3-7.0c 5.0i 7.0i"

if not exist "BUILD_v3_5.0.bat" (
  echo ERROR: Multi-target builder not found.
  pause
  exit /b 1
)

if exist "output" rmdir /s /q "output"
mkdir "output"

for %%D in (%TARGETS%) do (
  echo.
  echo ============================================================
  echo   BUILDING %%D
  echo ============================================================
  call "BUILD_v3_5.0.bat" "%%D" /batch
  if errorlevel 1 goto :failed
)


echo.
echo ============================================================
echo   ALL BUILDS OK
echo ============================================================
echo.
echo Firmware files are in:
echo   %CD%\output\
echo.
explorer "%CD%\output"
pause
exit /b 0

:failed
echo.
echo ============================================================
echo   BUILD FAILED
echo ============================================================
echo.
echo The failed target is shown above.
echo Completed targets remain in the output folder.
pause
exit /b 1
