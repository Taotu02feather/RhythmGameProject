@echo off
REM ============================================
REM  Open Rhythm Engine - Build Script (Windows)
REM ============================================
REM
REM  Prerequisites:
REM    - CMake 3.16+ installed and in PATH
REM    - SDL2 and SDL2_mixer development libraries
REM    - Visual Studio 2019/2022 or MinGW-w64
REM
REM  SDL2 Setup (choose one):
REM    Option A - vcpkg (recommended):
REM      vcpkg install sdl2 sdl2-mixer
REM
REM    Option B - Manual download:
REM      Download SDL2-devel and SDL2_mixer-devel from
REM      https://github.com/libsdl-org/SDL/releases
REM      Extract to External/SDL2 and External/SDL2_mixer
REM
REM ============================================

setlocal enabledelayedexpansion

echo ============================================
echo  Open Rhythm Engine - Build
echo ============================================

REM Check for CMake
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake not found. Please install CMake 3.16+ and add it to PATH.
    exit /b 1
)

REM Create build directory
if not exist "build" mkdir build
cd build

REM Configure with CMake
echo.
echo [1/2] Configuring with CMake...
cmake .. -G "Visual Studio 17 2022" -A x64 2>nul
if %ERRORLEVEL% NEQ 0 (
    REM Try VS 2019
    cmake .. -G "Visual Studio 16 2019" -A x64 2>nul
)
if %ERRORLEVEL% NEQ 0 (
    REM Try MinGW
    cmake .. -G "MinGW Makefiles" 2>nul
)
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configuration failed.
    echo Please ensure SDL2 and SDL2_mixer development libraries are installed.
    echo Try: vcpkg install sdl2 sdl2-mixer
    exit /b 1
)

REM Build
echo.
echo [2/2] Building...
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo.
echo ============================================
echo  Build successful!
echo  Output: build\bin\Release\OpenRhythmEngine.exe
echo ============================================
echo.
echo To run the engine:
echo   build\bin\Release\OpenRhythmEngine.exe
echo.

cd ..
endlocal