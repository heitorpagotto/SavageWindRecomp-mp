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
if ! pkg-config --exists sdl3 libavformat libcurl openssl; then
    echo 'Install the UCRT64 SDL3, FFmpeg, curl and OpenSSL development packages.' >&2
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

printf 'Windows executable: %s/savagewind.exe\n' "$build_dir"
printf 'Local runtime DLLs staged next to savagewind.exe.\n'
