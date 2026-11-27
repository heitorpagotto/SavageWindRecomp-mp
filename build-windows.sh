#!/usr/bin/env bash
# Build the Windows app and optionally generate and link a local game library.
set -euo pipefail

usage() {
    cat <<'USAGE'
Usage: bash build-windows.sh [--game GAME_FOLDER | --app-only]
Run from MSYS2 UCRT64. With no arguments, use this checkout's extracted/ folder.
GAME_FOLDER must contain exefs/ and romfs/. Outputs go to build/windows-local/.
Set JOBS to change the parallel build job count (default 8).
USAGE
}

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
mode=game
game_root=extracted
case "${1:-}" in
    --help|-h) usage; exit 0 ;;
    --app-only) [[ $# == 1 ]] || { usage >&2; exit 2; }; mode=app ;;
    --game) [[ $# == 2 ]] || { usage >&2; exit 2; }; game_root=$2 ;;
    '') ;;
    *) usage >&2; exit 2 ;;
esac

if [[ ${MSYSTEM:-} != UCRT64 || $(uname -m) != x86_64 ]]; then
    echo 'Open the MSYS2 UCRT64 terminal and run the script there.' >&2
    exit 1
fi
for tool in clang clang++ make git cmake ninja windres pkg-config ldd; do
    if ! command -v "$tool" >/dev/null; then
        echo "Missing $tool. See docs/BUILD-windows.md for the Windows build prerequisites." >&2
        exit 1
    fi
done
if ! pkg-config --exists sdl3 libavformat openssl; then
    echo 'Install the UCRT64 SDL3, FFmpeg and OpenSSL development packages.' >&2
    exit 1
fi
if ! py -3.12 --version >/dev/null 2>&1; then
    echo 'Run tools/setup_windows.ps1 from PowerShell to install Python 3.12 for the setup tools.' >&2
    exit 1
fi
if [[ $mode == game ]]; then
    if [[ ! -f $game_root/exefs/main || ! -d $game_root/romfs ]]; then
        echo "Expected exefs/main and romfs/ in $game_root. Use --game to point at your BOTW 1.9.0 folder." >&2
        exit 1
    fi
    if [[ ! -x tools/venv/Scripts/python.exe ]]; then
        echo 'Run tools/setup_windows.ps1 from PowerShell to create the generator Python environment.' >&2
        exit 1
    fi
    game_root=$(cd -- "$game_root" && pwd)
fi

export JOBS=${JOBS:-8}
export MP_RTC_PREFIX="$PWD/build/multiplayer-deps"
CC=clang CXX=clang++ sh tools/build_multiplayer_deps.sh "$MP_RTC_PREFIX"
export PATH="$MP_RTC_PREFIX/bin:$PATH"
build_dir=build/windows-local
if [[ $mode == game ]]; then
    gen_dir="$build_dir/generated"
    tools/venv/Scripts/python.exe tools/gen.py all "$game_root/exefs" "$gen_dir" \
        --hle runtime/hle/hle_symbols.txt --locals
    make -j"$JOBS" B="$build_dir" GEN="$gen_dir"
    printf 'Game library: %s/swgame.dll\n' "$build_dir"
else
    make -j"$JOBS" B="$build_dir" GEN="$build_dir/generated" \
        "$build_dir/savagewind.exe" "$build_dir/savagewind-config.exe"
fi
make B="$build_dir" stage-ui-fonts

# Inspect the new executable away from previously staged DLLs so a rebuild picks up toolchain updates.
probe_dir=$(mktemp -d "$build_dir/.dll-probe.XXXXXX")
trap 'rm -f -- "$probe_dir/savagewind.exe"; rmdir -- "$probe_dir"' EXIT
cp "$build_dir/savagewind.exe" "$probe_dir/"
dependencies=$(ldd "$probe_dir/savagewind.exe")
if [[ $dependencies != *'SDL3.dll => '* || $dependencies == *'not found'* ]]; then
    printf 'Could not resolve Windows runtime DLLs with the UCRT64 toolchain.\n%s\n' "$dependencies" >&2
    exit 1
fi
while IFS= read -r dependency; do
    case "$dependency" in
        /ucrt64/bin/*|"$MP_RTC_PREFIX"/bin/*)
            [[ ${dependency##*/} == vulkan-1.dll ]] || cp -f -- "$dependency" "$build_dir/"
            ;;
    esac
done < <(sed -n 's/^.* => \(.*\) (0x[[:xdigit:]]*)$/\1/p' <<<"$dependencies")

# The setup screen builds the game library locally when it opens a player's extracted files.
PYINSTALLER_CONFIG_DIR="$PWD/build/pyinstaller-cache" make gen-helper
ZIG_TARGET=x86_64-windows bash tools/fetch_zig.sh
mkdir -p "$build_dir"/{swgen,zig,gamelib-src}
cp -R build/helper/swgen/. "$build_dir/swgen/"
cp -R build/zig/. "$build_dir/zig/"
cp runtime/cpu.h runtime/simd.h runtime/simd_fp.h runtime/simd_softfma.h runtime/gamelib.h \
    tools/gamelib_entry.c runtime/hle/hle_symbols.txt "$build_dir/gamelib-src/"

printf 'Windows executable: %s/savagewind.exe\n' "$build_dir"
printf 'Runtime DLLs, UI fonts and game setup tools staged next to savagewind.exe.\n'
