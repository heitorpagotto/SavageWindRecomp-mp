# One-time setup of a Windows machine for building Savage Wind (see docs/BUILD-windows.md).
# Installs MSYS2 (clang, lld, make, SDL3, FFmpeg, Vulkan loader/headers, shaderc) and Python 3.12, then creates
# tools/venv (lifter: unicorn, capstone, lz4). Safe to re-run.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

if (-not (Test-Path C:\msys64\usr\bin\pacman.exe)) {
    winget install --id MSYS2.MSYS2 -e --accept-source-agreements --accept-package-agreements --silent
    if (-not (Test-Path C:\msys64\usr\bin\pacman.exe)) { throw 'MSYS2 setup failed. Expected C:\msys64.' }
}
$env:MSYSTEM = 'UCRT64'
$env:CHERE_INVOKING = '1'
$bash = 'C:\msys64\usr\bin\bash.exe'
$pkgs = 'git', 'make', 'mingw-w64-ucrt-x86_64-clang', 'mingw-w64-ucrt-x86_64-lld', 'mingw-w64-ucrt-x86_64-sdl3',
        'mingw-w64-ucrt-x86_64-binutils', 'mingw-w64-ucrt-x86_64-openssl', 'mingw-w64-ucrt-x86_64-cmake',
        'mingw-w64-ucrt-x86_64-ninja', 'mingw-w64-ucrt-x86_64-curl', 'mingw-w64-ucrt-x86_64-zlib',
        'mingw-w64-ucrt-x86_64-ffmpeg', 'mingw-w64-ucrt-x86_64-vulkan-headers', 'mingw-w64-ucrt-x86_64-vulkan-loader',
        'mingw-w64-ucrt-x86_64-shaderc',
        'mingw-w64-ucrt-x86_64-spirv-tools', 'mingw-w64-ucrt-x86_64-pkgconf', 'mingw-w64-ucrt-x86_64-gdb',
        'mingw-w64-ucrt-x86_64-vulkan-memory-allocator', 'mingw-w64-ucrt-x86_64-vulkan-validation-layers',
        'mingw-w64-ucrt-x86_64-nasm'  # (make deps-windows: FFmpeg's VP9 assembly)
$packages = $pkgs -join ' '
& $bash -lc "pacman -Q $packages >/dev/null 2>&1"
if ($LASTEXITCODE -ne 0) {
    & $bash -lc 'pacman -Syuu --noconfirm'
    & $bash -lc 'pacman -Syuu --noconfirm'  # second pass after a core update
    if ($LASTEXITCODE -ne 0) { throw 'MSYS2 package update failed.' }
    & $bash -lc "pacman -S --needed --noconfirm $packages"
    if ($LASTEXITCODE -ne 0) { throw 'Installation of the Windows build packages failed.' }
}

# Python for the tools: the python.org build (prebuilt unicorn / capstone wheels; MSYS2's Python can't build them)
$python312 = $false
if (Get-Command py.exe -ErrorAction SilentlyContinue) {
    $python312 = [bool]((& py.exe -0p) -match '-(?:V:)?3\.12(?:\s|[-*])')
}
if (-not $python312) {
    winget install --id Python.Python.3.12 -e --scope user --accept-source-agreements --accept-package-agreements --silent
}
if (-not (Test-Path "$root\tools\venv\Scripts\python.exe")) {
    & py.exe -3.12 -m venv "$root\tools\venv"
    if ($LASTEXITCODE -ne 0) { throw 'Could not create the Python 3.12 build environment.' }
}
& "$root\tools\venv\Scripts\python.exe" -m pip install -q lz4 capstone unicorn
if ($LASTEXITCODE -ne 0) { throw 'Installation of the Python build dependencies failed.' }

Write-Host 'Windows build prerequisites are ready.'
