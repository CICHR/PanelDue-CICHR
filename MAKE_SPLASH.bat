@echo off
setlocal
cd /d "%~dp0"
where py.exe >nul 2>&1
if errorlevel 1 (
  echo Python launcher not found. Install Python 3 first.
  pause
  exit /b 1
)
py -c "import PIL" >nul 2>&1
if errorlevel 1 (
  echo Installing Pillow...
  py -m pip install pillow
  if errorlevel 1 exit /b 1
)
py Tools\SplashScreenTool.py encode SplashScreens\SplashScreen-CICHR-480x272.png SplashScreens\SplashScreen-CICHR-480x272.bin
if errorlevel 1 exit /b 1
py Tools\SplashScreenTool.py encode SplashScreens\SplashScreen-CICHR-800x480.png SplashScreens\SplashScreen-CICHR-800x480.bin
if errorlevel 1 exit /b 1
echo.
echo Splash binaries updated.
pause
