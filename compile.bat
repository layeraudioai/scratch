@echo off
REM Interactive Song Compiler
REM Batch version of compile.sh

setlocal enabledelayedexpansion

REM Get the song name from first argument
set "SONG=%~1"
if "%SONG%"=="" set "SONG=scratch"

REM Remove .c extension if present
if not "%SONG%"=="" (
    set "SONG=%SONG:.c=%"
)

cls
echo === MML Song Encoder Build System ===
echo.

echo Scanning for available audio libraries...

REM Initialize available backends array
set "AVAILABLE_COUNT=0"
set "AVAILABLE[0]=DUMMY"

REM Check for ALSA (not typically available on Windows)
REM On Windows, we'll check for common audio backends

REM Check for SDL3 via pkg-config (if available)
where pkg-config >nul 2>nul
if %errorlevel% equ 0 (
    pkg-config --exists sdl3 >nul 2>nul
    if %errorlevel% equ 0 (
        set /a AVAILABLE_COUNT+=1
        set "AVAILABLE[!AVAILABLE_COUNT!]=SDL3"
    )
)

REM Check for SDL2 via pkg-config
where pkg-config >nul 2>nul
if %errorlevel% equ 0 (
    pkg-config --exists sdl2 >nul 2>nul
    if %errorlevel% equ 0 (
        set /a AVAILABLE_COUNT+=1
        set "AVAILABLE[!AVAILABLE_COUNT!]=SDL2"
    )
)

REM Check for SDL via sdl-config
where sdl-config >nul 2>nul
if %errorlevel% equ 0 (
    sdl-config --version >nul 2>nul
    if %errorlevel% equ 0 (
        set /a AVAILABLE_COUNT+=1
        set "AVAILABLE[!AVAILABLE_COUNT!]=SDL"
    )
)

REM Windows Multimedia API is always available on Windows
set /a AVAILABLE_COUNT+=1
set "AVAILABLE[!AVAILABLE_COUNT!]=WINMM"

REM Add DUMMY backend (already at index 0)

if %AVAILABLE_COUNT% equ 0 (
    echo No audio backends detected.
    set "SELECTED=DUMMY"
) else (
    echo Available backends for '%SONG%':
    for /l %%i in (0,1,%AVAILABLE_COUNT%) do (
        echo   [%%i] !AVAILABLE[%%i]!
    )
    
    set /p "choice=Select backend [0-%AVAILABLE_COUNT%]: "
    if "!choice!"=="" set "choice=0"
    
    REM Validate input
    set "valid=0"
    for /l %%i in (0,1,%AVAILABLE_COUNT%) do (
        if "%%i"=="!choice!" (
            set "valid=1"
            set "SELECTED=!AVAILABLE[%%i]!"
        )
    )
    
    if !valid! equ 0 (
        echo Invalid selection. Using default.
        set "SELECTED=!AVAILABLE[0]!"
    )
)

REM Handle compiler prefix (e.g. for cross-compiling)
set "CC_CMD=gcc"
if not "%~2"=="" (
    set "CC_CMD=%~2cc"
)

echo Building %SONG% with %SELECTED% backend...
echo --------------------------------------

REM Change to parent directory
cd /d "%~dp0.."

REM Call make with the selected parameters
make SONG="%SONG%" AUDIO_BACKEND="%SELECTED%" CC="%CC_CMD%"

echo --------------------------------------

endlocal
