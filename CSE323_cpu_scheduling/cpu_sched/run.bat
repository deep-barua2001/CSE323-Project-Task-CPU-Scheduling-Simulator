@echo off
setlocal enabledelayedexpansion

:: 1. Setup Environment
cd /d "%~dp0"
set "EXE=sched.exe"
set "SRC=src\main.c src\common.c src\parse.c src\metrics.c src\predictor.c src\scheduler.c"

:: 2. Find Compiler (Prefer clang, then gcc)
where clang >nul 2>nul
if %ERRORLEVEL% equ 0 (
    set "CC=clang"
    goto :compile
)

where gcc >nul 2>nul
if %ERRORLEVEL% equ 0 (
    set "CC=gcc"
    goto :compile
)

:: Try to find LLVM-MinGW in LocalAppData (common for this setup)
for /d %%i in ("%LOCALAPPDATA%\Microsoft\WinGet\Packages\MartinStorsjo.LLVM-MinGW*") do (
    for /d %%j in ("%%i\*") do (
        if exist "%%j\bin\clang.exe" (
            set "CC=%%j\bin\clang.exe"
            set "PATH=%%j\bin;!PATH!"
            goto :compile
        )
    )
)

echo [ERROR] No C compiler (clang or gcc) found in PATH.
echo Please install MinGW or LLVM.
pause
exit /b 1

:compile
:: 3. Check if rebuild is needed
set "REBUILD=0"
if not exist "%EXE%" (
    set "REBUILD=1"
) else (
    del "%EXE%"
    set "REBUILD=1"
)

:: Force recompile if user wants a clean run or if exe is missing
echo [INFO] Using compiler: %CC%
echo [INFO] Compiling...
%CC% -std=c11 -Wall -Wextra -O2 -o %EXE% %SRC%
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Compilation failed.
    pause
    exit /b %ERRORLEVEL%
)

:run
:: 4. Run the simulator
echo.
echo [SUCCESS] Build complete. Starting Simulator...
echo ------------------------------------------------
%EXE%
if %ERRORLEVEL% neq 0 (
    echo.
    echo [CRITICAL] The simulator crashed or returned an error (Code: %ERRORLEVEL%).
)
echo.
echo ------------------------------------------------
echo [INFO] Simulation ended.
echo Press any key to close this window.
pause >nul
