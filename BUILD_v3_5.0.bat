@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"

set "VERSION_FILE=src\Version.hpp"
set "FW_VERSION="
for /f "tokens=3" %%V in ('findstr /C:"#define VERSION_MAIN" "%VERSION_FILE%"') do set "FW_VERSION=%%~V"
if not defined FW_VERSION (
    echo ERROR: VERSION_MAIN not found in %VERSION_FILE%.
    exit /b 1
)

set "DEVICE=%~1"
if not defined DEVICE set "DEVICE=v3-5.0"
set "BATCH_MODE=%~2"
title PanelDue CICHR v%FW_VERSION% - %DEVICE%

set "BUILD_DIR=build-%DEVICE%"
set "OUT_DIR=output"
set "DEPS_CACHE=%TEMP%\PanelDueCICHRDeps"
set "BASE64_COMMIT=25b96f3dadc3763a6ce8bfd72250cbe08592accc"
set "LIBRRF_COMMIT=e2bc756bc3bd4af6337e84c50042b057a627c60a"
set "QOI_COMMIT=ef59be0ce2b160fe7e3567266895f3b55c53bfd4"
set "FINAL_BIN=%OUT_DIR%\PanelDue-CICHR-v%FW_VERSION%-%DEVICE%.bin"
set "NOLOGO_BIN=%OUT_DIR%\PanelDue-CICHR-v%FW_VERSION%-%DEVICE%-nologo.bin"
set "SPLASH=SplashScreens\SplashScreen-CICHR-800x480.bin"
if /i "%DEVICE%"=="v2-4.3" set "SPLASH=SplashScreens\SplashScreen-CICHR-480x272.bin"
if /i "%DEVICE%"=="v3-4.3" set "SPLASH=SplashScreens\SplashScreen-CICHR-480x272.bin"
set "FLASH_LIMIT=262144"

cls
echo *** PANELDUE CICHR v%FW_VERSION% ***
echo.
echo ============================================================
echo   PanelDue CICHR - %DEVICE%
echo   PanelDue CICHR v%FW_VERSION%
echo ============================================================
echo.
echo Source folder: %CD%
echo Target:        %DEVICE%
echo.

call :main
set "BUILD_RC=!ERRORLEVEL!"
if not "!BUILD_RC!"=="0" goto :failed
goto :success

:main
if not exist "CMakeLists.txt" (
    echo ERROR: CMakeLists.txt not found.
    echo Put this BAT file in the root folder of PanelDue CICHR.
    exit /b 1
)
if not exist "%SPLASH%" (
    echo ERROR: Missing %SPLASH%
    exit /b 1
)

rem ------------------------------------------------------------
rem Required Windows tools
rem ------------------------------------------------------------
echo [1/6] Checking build tools...
call :ensure_tool cmake.exe Kitware.CMake "CMake"
if errorlevel 1 exit /b 1
call :ensure_tool ninja.exe Ninja-build.Ninja "Ninja"
if errorlevel 1 exit /b 1
call :ensure_arm_gcc
if errorlevel 1 exit /b 1

call :find_on_path cmake.exe CMAKE_EXE
call :find_on_path ninja.exe NINJA_EXE
call :locate_arm_gcc
if errorlevel 1 exit /b 1

rem Validate that this is a COMPLETE C++ toolchain, not just compiler EXEs.
call :test_cpp_toolchain
if errorlevel 1 (
    echo.
    echo ERROR: The installed ARM GCC cannot compile a basic C++ file.
    echo The standard C++ headers such as cstdint are missing.
    echo Reinstall Arm GNU Toolchain and run this BAT again.
    exit /b 1
)

set "PATH=!ARM_BIN!;!PATH!"
set "ARM_PREFIX=!ARM_BIN:\=/!arm-none-eabi-"
set "NINJA_CMAKE=!NINJA_EXE:\=/!"

set "GCC_VERSION_FILE=.build_support\gcc-version.txt"
if not exist ".build_support" mkdir ".build_support"
"!ARM_GCC!" --version > "!GCC_VERSION_FILE!" 2>&1
set "GCC_VERSION="
set /p GCC_VERSION=<"!GCC_VERSION_FILE!"
del /q "!GCC_VERSION_FILE!" >nul 2>&1

echo.
echo Tools:
echo   CMake : !CMAKE_EXE!
echo   Ninja : !NINJA_EXE!
echo   ARM bin: !ARM_BIN!
echo   GCC    : !ARM_GCC!
echo   G++    : !ARM_GPP!
echo   Objcopy: !ARM_OBJCOPY!
echo   Version: !GCC_VERSION!
echo.

rem ------------------------------------------------------------
rem GCC compatibility
rem ------------------------------------------------------------
echo [2/6] Checking GCC compatibility patch...
call :patch_ecv_array
if errorlevel 1 exit /b 1

rem ------------------------------------------------------------
rem Dependencies
rem ------------------------------------------------------------
if not exist "lib\librrf\CMakeLists.txt" goto :need_deps
if not exist "lib\base64\base64.c" goto :need_deps
if not exist "lib\qoi\qoi.h" goto :need_deps
goto :deps_ready

:need_deps
echo [3/6] Missing Git submodules - downloading pinned dependencies...
call :ensure_tool git.exe Git.Git "Git"
if errorlevel 1 exit /b 1
call :find_on_path git.exe GIT_EXE
if not defined GIT_EXE (
    echo ERROR: git.exe not found after installation.
    exit /b 1
)

rem Keep dependency repositories in a short path to avoid Windows path-length limits.
echo       Dependency cache: !DEPS_CACHE!
"!GIT_EXE!" config --global core.longpaths true >nul 2>&1

call :ensure_dependency base64 https://github.com/Duet3D/base64.git !BASE64_COMMIT! base64.c
if errorlevel 1 exit /b 1
call :ensure_dependency librrf https://github.com/Duet3D/RRFLibraries.git !LIBRRF_COMMIT! CMakeLists.txt
if errorlevel 1 exit /b 1
call :ensure_dependency qoi https://github.com/Duet3D/qoi.git !QOI_COMMIT! qoi.h
if errorlevel 1 exit /b 1

:copy_deps
echo Copying libraries into the source tree...
call :copy_tree "!DEPS_CACHE!\librrf" "lib\librrf"
if errorlevel 1 exit /b 1
call :copy_tree "!DEPS_CACHE!\base64" "lib\base64"
if errorlevel 1 exit /b 1
call :copy_tree "!DEPS_CACHE!\qoi" "lib\qoi"
if errorlevel 1 exit /b 1

:deps_ready
if not exist "lib\librrf\CMakeLists.txt" (
    echo ERROR: librrf is still missing.
    exit /b 1
)

rem Validate the RRFLibraries eCv header with the selected compiler.
echo       Checking RRFLibraries GCC compatibility patch...
call :patch_ecv_array_file "lib\librrf\src\ecv_original.h"
if errorlevel 1 exit /b 1
call :verify_no_array_defines
if errorlevel 1 exit /b 1
call :test_ecv_cpp_compat
if errorlevel 1 exit /b 1

echo [3/6] Dependencies OK.

rem ------------------------------------------------------------
rem Clean CMake cache
rem ------------------------------------------------------------
echo [4/6] Cleaning build cache...
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
if exist "%BUILD_DIR%" (
    echo ERROR: Could not delete %BUILD_DIR%.
    echo Close any program that has files open in that folder and try again.
    exit /b 1
)

rem ------------------------------------------------------------
rem Configure and compile
rem ------------------------------------------------------------
echo [5/6] Configuring CMake for %DEVICE%...
"!CMAKE_EXE!" -S . -B "%BUILD_DIR%" -G Ninja -DDEVICE=%DEVICE% "-DCROSS_COMPILE=!ARM_PREFIX!" "-DCMAKE_MAKE_PROGRAM=!NINJA_CMAKE!"
if errorlevel 1 exit /b 1

echo.
echo Compiling firmware...
"!CMAKE_EXE!" --build "%BUILD_DIR%" --target paneldue.elf --parallel
if errorlevel 1 exit /b 1

if not exist "%BUILD_DIR%\paneldue.elf" (
    echo ERROR: Build finished but %BUILD_DIR%\paneldue.elf was not created.
    exit /b 1
)

rem ------------------------------------------------------------
rem Create DWC upload BIN
rem ------------------------------------------------------------
echo [6/6] Creating firmware BIN files...
if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"
if exist "%FINAL_BIN%" del /q "%FINAL_BIN%"
if exist "%NOLOGO_BIN%" del /q "%NOLOGO_BIN%"

"!ARM_OBJCOPY!" -O binary "%BUILD_DIR%\paneldue.elf" "%NOLOGO_BIN%"
if errorlevel 1 exit /b 1

copy /b /y "%NOLOGO_BIN%"+"%SPLASH%" "%FINAL_BIN%" >nul
if errorlevel 1 exit /b 1

for %%F in ("%FINAL_BIN%") do set "FINAL_SIZE=%%~zF"
echo.
echo Firmware size: !FINAL_SIZE! bytes / %FLASH_LIMIT% bytes
if !FINAL_SIZE! GTR %FLASH_LIMIT% (
    echo ERROR: Firmware is larger than the 256 KiB flash limit for this PanelDue target.
    echo Do NOT upload this file.
    exit /b 1
)


exit /b 0

rem ============================================================
rem Validate eCv headers for the selected GCC toolchain.
rem ============================================================
:patch_ecv_array
call :patch_ecv_array_file "src\ecv.h"
exit /b %ERRORLEVEL%

:patch_ecv_array_file
set "ECV_PATCH_FILE=%~1"
if not exist "%ECV_PATCH_FILE%" (
    echo ERROR: %ECV_PATCH_FILE% not found.
    exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -File "Tools\PatchEcvArrayForGcc14.ps1" -Path "%ECV_PATCH_FILE%"
if errorlevel 1 (
    echo ERROR: Could not remove active array macro from %ECV_PATCH_FILE%.
    exit /b 1
)
exit /b 0

rem ============================================================
rem Check project headers for active array macros.
rem ============================================================
:verify_no_array_defines
powershell -NoProfile -ExecutionPolicy Bypass -File "Tools\VerifyNoArrayDefines.ps1"
if errorlevel 1 (
    echo ERROR: An active #define array still exists somewhere in the source tree.
    exit /b 1
)
exit /b 0

rem ============================================================
rem Compile-test both eCv headers with the installed ARM GCC.
rem ============================================================
:test_ecv_cpp_compat
if not exist ".build_support" mkdir ".build_support"

call :test_one_ecv_header "src/ecv.h" "project"
if errorlevel 1 exit /b 1
call :test_one_ecv_header "lib/librrf/src/ecv_original.h" "librrf"
if errorlevel 1 exit /b 1

echo       GCC C++ std::array compatibility tests passed for both eCv headers.
exit /b 0

:test_one_ecv_header
set "ECV_HEADER=%~1"
set "ECV_TEST_NAME=%~2"
set "ECV_TEST_CPP=.build_support\paneldue_ecv_!ECV_TEST_NAME!_test.cpp"
set "ECV_TEST_OBJ=.build_support\paneldue_ecv_!ECV_TEST_NAME!_test.o"
set "ECV_TEST_LOG=.build_support\paneldue_ecv_!ECV_TEST_NAME!_test.log"

>"!ECV_TEST_CPP!" echo #include "!ECV_HEADER!"
>>"!ECV_TEST_CPP!" echo #ifdef array
>>"!ECV_TEST_CPP!" echo #error PanelDue build error: macro array is still defined
>>"!ECV_TEST_CPP!" echo #endif
>>"!ECV_TEST_CPP!" echo #include ^<utility^>
>>"!ECV_TEST_CPP!" echo #include ^<array^>
>>"!ECV_TEST_CPP!" echo std::array^<int, 2^> paneldue_array_test = { 1, 2 };

"!ARM_GPP!" -std=gnu++17 -mcpu=cortex-m4 -mthumb -I. -c "!ECV_TEST_CPP!" -o "!ECV_TEST_OBJ!" >"!ECV_TEST_LOG!" 2>&1
set "ECV_TEST_RC=!ERRORLEVEL!"
if not "!ECV_TEST_RC!"=="0" (
    echo.
    echo ERROR: eCv/GCC compatibility test failed for !ECV_HEADER!.
    if exist "!ECV_TEST_LOG!" type "!ECV_TEST_LOG!"
    del /q "!ECV_TEST_CPP!" >nul 2>&1
    del /q "!ECV_TEST_OBJ!" >nul 2>&1
    exit /b 1
)

del /q "!ECV_TEST_CPP!" >nul 2>&1
del /q "!ECV_TEST_OBJ!" >nul 2>&1
del /q "!ECV_TEST_LOG!" >nul 2>&1
exit /b 0

rem ============================================================
rem Validate complete C++ toolchain
rem ============================================================
:test_cpp_toolchain
if not exist ".build_support" mkdir ".build_support"
set "TC_TEST_CPP=.build_support\paneldue_toolchain_test.cpp"
set "TC_TEST_OBJ=.build_support\paneldue_toolchain_test.o"
set "TC_TEST_LOG=.build_support\paneldue_toolchain_test.log"

>"!TC_TEST_CPP!" echo #include ^<cstdint^>
>>"!TC_TEST_CPP!" echo #include ^<cstddef^>
>>"!TC_TEST_CPP!" echo std::uint32_t paneldue_toolchain_test = 0;

"!ARM_GPP!" -std=gnu++17 -mcpu=cortex-m4 -mthumb -c "!TC_TEST_CPP!" -o "!TC_TEST_OBJ!" >"!TC_TEST_LOG!" 2>&1
set "TC_TEST_RC=!ERRORLEVEL!"
if not "!TC_TEST_RC!"=="0" (
    if exist "!TC_TEST_LOG!" type "!TC_TEST_LOG!"
    del /q "!TC_TEST_CPP!" >nul 2>&1
    del /q "!TC_TEST_OBJ!" >nul 2>&1
    exit /b 1
)

del /q "!TC_TEST_CPP!" >nul 2>&1
del /q "!TC_TEST_OBJ!" >nul 2>&1
del /q "!TC_TEST_LOG!" >nul 2>&1
exit /b 0

rem ============================================================
rem Find/install Arm GNU toolchain. Prefer existing complete installation.
rem ============================================================
:ensure_arm_gcc
call :locate_arm_gcc
if not errorlevel 1 exit /b 0

echo ARM GCC not found. Installing with winget...
where winget.exe >nul 2>&1
if errorlevel 1 (
    echo ERROR: winget is unavailable.
    echo Install Arm GNU Toolchain for arm-none-eabi manually and run again.
    exit /b 1
)
winget install --id Arm.GnuArmEmbeddedToolchain --exact --accept-package-agreements --accept-source-agreements --silent
if errorlevel 1 (
    echo ERROR: Arm GNU Toolchain installation failed.
    exit /b 1
)
call :refresh_path
call :locate_arm_gcc
if errorlevel 1 (
    echo ERROR: ARM GCC was installed but could not be located.
    echo Close this window and run the BAT again.
    exit /b 1
)
exit /b 0

:locate_arm_gcc
set "ARM_GCC="
set "ARM_GPP="
set "ARM_OBJCOPY="
set "ARM_AR="
set "ARM_OBJDUMP="
set "ARM_BIN="

for /f "delims=" %%I in ('where arm-none-eabi-gcc.exe 2^>nul') do if not defined ARM_GCC set "ARM_GCC=%%I"

if not defined ARM_GCC (
    for /f "usebackq delims=" %%I in (`powershell -NoProfile -ExecutionPolicy Bypass -Command "$roots=@($env:ProgramFiles,${env:ProgramFiles(x86)},$env:LOCALAPPDATA+'\Programs'); $patterns=@('Arm GNU Toolchain*\*\bin\arm-none-eabi-gcc.exe','Arm GNU Toolchain arm-none-eabi\*\bin\arm-none-eabi-gcc.exe','GNU Arm Embedded Toolchain\*\bin\arm-none-eabi-gcc.exe','GNU Tools ARM Embedded\*\bin\arm-none-eabi-gcc.exe'); foreach($r in $roots){if($r -and (Test-Path $r)){foreach($p in $patterns){$x=Get-ChildItem -Path (Join-Path $r $p) -File -ErrorAction SilentlyContinue ^| Sort-Object FullName -Descending ^| Select-Object -First 1 -ExpandProperty FullName; if($x){$x; exit}}}}"`) do if not defined ARM_GCC set "ARM_GCC=%%I"
)

if not defined ARM_GCC exit /b 1
for %%I in ("!ARM_GCC!") do set "ARM_BIN=%%~dpI"
set "ARM_GPP=!ARM_BIN!arm-none-eabi-g++.exe"
set "ARM_OBJCOPY=!ARM_BIN!arm-none-eabi-objcopy.exe"
set "ARM_AR=!ARM_BIN!arm-none-eabi-ar.exe"
set "ARM_OBJDUMP=!ARM_BIN!arm-none-eabi-objdump.exe"
if not exist "!ARM_GPP!" exit /b 1
if not exist "!ARM_OBJCOPY!" exit /b 1
if not exist "!ARM_AR!" exit /b 1
if not exist "!ARM_OBJDUMP!" exit /b 1
exit /b 0

rem ============================================================
rem Generic helpers
rem ============================================================
:ensure_tool
where %~1 >nul 2>&1
if not errorlevel 1 exit /b 0
call :refresh_path
where %~1 >nul 2>&1
if not errorlevel 1 exit /b 0

echo Missing %~3.
where winget.exe >nul 2>&1
if errorlevel 1 (
    echo ERROR: winget is not available, so %~3 cannot be installed automatically.
    exit /b 1
)
echo Installing %~3 using winget...
winget install --id %~2 --exact --accept-package-agreements --accept-source-agreements --silent
if errorlevel 1 (
    echo ERROR: Installation of %~3 failed.
    exit /b 1
)
call :refresh_path
where %~1 >nul 2>&1
if not errorlevel 1 exit /b 0
echo ERROR: %~3 was installed but %~1 is still not on PATH.
echo Close this window and run the BAT again.
exit /b 1

:refresh_path
for /f "usebackq delims=" %%P in (`powershell -NoProfile -ExecutionPolicy Bypass -Command "$m=[Environment]::GetEnvironmentVariable('Path','Machine');$u=[Environment]::GetEnvironmentVariable('Path','User');Write-Output ($m+';'+$u)"`) do set "PATH=%%P"
exit /b 0

:find_on_path
set "%~2="
for /f "delims=" %%I in ('where %~1 2^>nul') do if not defined %~2 set "%~2=%%I"
exit /b 0

:ensure_dependency
set "DEP_NAME=%~1"
set "DEP_URL=%~2"
set "DEP_COMMIT=%~3"
set "DEP_CHECK=%~4"
set "DEP_DIR=!DEPS_CACHE!\!DEP_NAME!"

if not exist "!DEPS_CACHE!" mkdir "!DEPS_CACHE!"

rem Reuse the short-path cache only if it is the exact pinned revision.
if exist "!DEP_DIR!\!DEP_CHECK!" (
    set "DEP_HEAD="
    "!GIT_EXE!" -C "!DEP_DIR!" rev-parse HEAD > "!DEPS_CACHE!\dep-head.txt" 2>nul
    if not errorlevel 1 set /p DEP_HEAD=<"!DEPS_CACHE!\dep-head.txt"
    del /q "!DEPS_CACHE!\dep-head.txt" >nul 2>&1
    if /i "!DEP_HEAD!"=="!DEP_COMMIT!" (
        echo       OK cached !DEP_NAME! @ !DEP_COMMIT!
        exit /b 0
    )
)

if exist "!DEP_DIR!" rmdir /s /q "!DEP_DIR!"
echo       Downloading !DEP_NAME! to short path...
"!GIT_EXE!" -c core.longpaths=true clone --no-tags "!DEP_URL!" "!DEP_DIR!"
if errorlevel 1 (
    echo ERROR: Could not clone !DEP_NAME! from !DEP_URL!
    exit /b 1
)

"!GIT_EXE!" -C "!DEP_DIR!" -c core.longpaths=true checkout --detach "!DEP_COMMIT!"
if errorlevel 1 (
    echo ERROR: Could not checkout !DEP_NAME! revision !DEP_COMMIT!.
    exit /b 1
)

if not exist "!DEP_DIR!\!DEP_CHECK!" (
    echo ERROR: !DEP_NAME! downloaded, but !DEP_CHECK! is missing.
    exit /b 1
)
exit /b 0

:copy_tree
if not exist "%~1" (
    echo ERROR: Dependency source folder does not exist: %~1
    exit /b 1
)
if not exist "%~2" mkdir "%~2"
robocopy "%~1" "%~2" /E /NFL /NDL /NJH /NJS /NC /NS /XD .git >nul
set "RC=!ERRORLEVEL!"
if !RC! GEQ 8 (
    echo ERROR: robocopy failed copying %~1
    exit /b 1
)
exit /b 0

:success
echo.
echo ============================================================
echo   BUILD OK
echo ============================================================
echo.
echo Firmware files:
echo   %CD%\%FINAL_BIN%
echo   %CD%\%NOLOGO_BIN%
echo.
echo Normal use: upload the file without -nologo.
echo The -nologo file contains the same firmware without the appended splash screen.
echo If needed, run M997 S4 afterwards.
echo.
if /i "%BATCH_MODE%"=="/batch" exit /b 0
explorer "%CD%\%OUT_DIR%"
pause
exit /b 0

:failed
echo.
echo ============================================================
echo   BUILD FAILED
echo ============================================================
echo Nothing was uploaded to the printer.
echo.
echo Send the output from this window if the build stops again.
echo.
if /i "%BATCH_MODE%"=="/batch" exit /b 1
pause
exit /b 1
