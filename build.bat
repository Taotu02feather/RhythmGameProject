@echo off
REM ============================================
REM  Open Rhythm Engine - Build Script (Windows)
REM ============================================
REM
REM  Prerequisites:
REM    - CMake 3.16+ installed and in PATH
REM    - Visual Studio 2019/2022 with C++ desktop workload
REM    - vcpkg with SDL2 and SDL2_mixer installed
REM
REM  vcpkg path is hardcoded; change VCPKG_ROOT if needed.
REM ============================================

setlocal enabledelayedexpansion

echo ============================================
echo  Open Rhythm Engine - Build
echo ============================================
echo.

REM ---------- vcpkg root ----------
set "VCPKG_ROOT=D:\VS\vcpkg\vcpkg"
set "VCPKG_TOOLCHAIN=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"

REM Check CMake
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake not found. Please install CMake 3.16+ and add it to PATH.
    exit /b 1
)
echo [OK] CMake found.

REM Check vcpkg toolchain file
if not exist "%VCPKG_TOOLCHAIN%" (
    echo [ERROR] vcpkg toolchain not found at:
    echo         %VCPKG_TOOLCHAIN%
    echo Please set VCPKG_ROOT to your vcpkg installation directory.
    exit /b 1
)
echo [OK] vcpkg toolchain found.

REM Check that SDL2 and SDL2_mixer are installed via vcpkg
"%VCPKG_ROOT%\vcpkg.exe" list sdl2 >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [WARN] SDL2 may not be installed in vcpkg.
    echo        Installing: "%VCPKG_ROOT%\vcpkg.exe" install sdl2 sdl2-mixer --triplet x64-windows
    "%VCPKG_ROOT%\vcpkg.exe" install sdl2 sdl2-mixer --triplet x64-windows
    if %ERRORLEVEL% NEQ 0 (
        echo [ERROR] Failed to install SDL2 packages.
        exit /b 1
    )
)
echo [OK] SDL2 packages ready.

REM ---------- Create / clean build directory ----------
if not exist "build" (
    mkdir build
) else (
    echo [INFO] Cleaning stale CMake cache...
    if exist "build\CMakeCache.txt" del /q "build\CMakeCache.txt"
    if exist "build\CMakeFiles" rmdir /s /q "build\CMakeFiles"
)
cd build

REM ---------- Configure ----------
echo.
echo [1/2] Configuring with CMake...
echo        Toolchain: %VCPKG_TOOLCHAIN%

REM Try VS 2026, 2022, 2019, then NMake
set CMAKE_ERR=1

REM -- Generator 1: Visual Studio 2026 (preview) --
cmake .. ^
    -G "Visual Studio 18 2026" ^
    -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN%"
set CMAKE_ERR=%ERRORLEVEL%
if %CMAKE_ERR% EQU 0 goto :configure_ok
echo [INFO] VS 2026 not found ^(error %CMAKE_ERR%^), trying VS 2022...

REM -- Generator 2: Visual Studio 2022 --
cmake .. ^
    -G "Visual Studio 17 2022" ^
    -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN%"
set CMAKE_ERR=%ERRORLEVEL%
if %CMAKE_ERR% EQU 0 goto :configure_ok
echo [INFO] VS 2022 not found ^(error %CMAKE_ERR%^), trying VS 2019...

REM -- Generator 3: Visual Studio 2019 --
cmake .. ^
    -G "Visual Studio 16 2019" ^
    -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN%"
set CMAKE_ERR=%ERRORLEVEL%
if %CMAKE_ERR% EQU 0 goto :configure_ok
echo [INFO] VS 2019 not found ^(error %CMAKE_ERR%^), trying NMake...

REM -- Generator 4: NMake Makefiles --
cmake .. ^
    -G "NMake Makefiles" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN%"
set CMAKE_ERR=%ERRORLEVEL%
if %CMAKE_ERR% EQU 0 goto :configure_ok

echo [ERROR] CMake configuration failed ^(error %CMAKE_ERR%^).
echo        No supported build system found.  See output above for details.
cd ..
exit /b 1

:configure_ok
echo [OK] CMake configuration succeeded.

REM ---------- Build ----------
echo.
echo [2/2] Building Release...
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed.
    cd ..
    exit /b 1
)

echo.
echo ============================================
echo  Build successful!
echo  Output: bin\OpenRhythmEngine.exe
echo ============================================
echo.
echo To run the engine:
echo   bin\OpenRhythmEngine.exe
echo.

cd ..
endlocal