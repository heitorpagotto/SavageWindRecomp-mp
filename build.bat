@echo off
setlocal
pushd "%~dp0" || exit /b 1

if "%~1"=="" goto menu
if /i "%~1"=="linux" goto linux
if /i "%~1"=="windows" goto windows
if /i "%~1"=="--help" goto help
if /i "%~1"=="-h" goto help
goto invalid

:menu
echo   1  Linux ^(AppImage^)
echo   2  Windows
choice /C 12 /N /M "Platform [1/2]: "
if errorlevel 3 goto invalid
if errorlevel 2 goto windows
if errorlevel 1 goto linux
goto invalid

:linux
where wsl.exe >nul 2>&1
if errorlevel 1 (
    echo Linux AppImage builds from Windows require WSL with an x86-64 Linux distro and Podman. >&2
    goto failed
)
if "%~1"=="" (
    wsl.exe --exec bash ./build.sh linux
) else (
    wsl.exe --exec bash ./build.sh %*
)
goto done

:windows
set "MSYSTEM=UCRT64"
set "CHERE_INVOKING=1"
set "build_bash=C:\msys64\usr\bin\bash.exe"
if not exist "%build_bash%" (
    powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools\setup_windows.ps1
    if errorlevel 1 goto failed
    set "SAVAGEWIND_WINDOWS_SETUP_DONE=1"
)
if not exist "%build_bash%" (
    echo MSYS2 Bash was not found at "%build_bash%". >&2
    goto failed
)
if "%~1"=="" (
    "%build_bash%" -l ./build.sh windows
) else (
    "%build_bash%" -l ./build.sh %*
)
goto done

:help
echo Usage: build.bat [linux ^| windows [--game GAME_FOLDER ^| --app-only]]
echo With no arguments, choose a platform from the menu.
echo Linux uses the default WSL distro. Windows sets up and uses MSYS2 UCRT64.
popd
exit /b 0

:invalid
echo Usage: build.bat [linux ^| windows [--game GAME_FOLDER ^| --app-only]] >&2
popd
exit /b 2

:failed
popd
exit /b 1

:done
set "build_result=%errorlevel%"
popd
exit /b %build_result%
