#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'USAGE'
Usage: bash build.sh [linux | windows [--game GAME_FOLDER | --app-only]]
With no arguments, choose a platform from the menu.
Linux: x86-64 AppImage via Podman, under build/dist/.
Windows: native UCRT64 build, under build/windows-local/.
         Default game folder: extracted/. Set JOBS to change the build job count.
USAGE
}

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

build_linux() {
    if (( $# )); then usage >&2; exit 2; fi
    case $(uname -s) in
        MSYS*|MINGW*)
            command -v wsl.exe >/dev/null || { echo 'Linux builds from Windows require WSL with Bash and Podman.' >&2; exit 1; }
            MSYS2_ARG_CONV_EXCL='*' exec wsl.exe --exec bash ./build.sh linux
            ;;
    esac
    if [[ $(uname -s) != Linux || $(uname -m) != x86_64 ]]; then
        echo 'Run this script on an x86-64 Linux host.' >&2
        exit 1
    fi
    if ! command -v podman >/dev/null; then
        echo 'Install Podman on the Linux host before running this script.' >&2
        exit 1
    fi

    if [[ -f extracted/romfs/Pack/Bootup.pack || -f runtime/game_place_table.inc ]]; then
        echo 'Use a source-only checkout for packaging. This checkout has game data used by the place table build.' >&2
        exit 1
    fi
    mkdir -p build

    base=localhost/savagewind-build:amd64
    image=localhost/savagewind-multiplayer-build:amd64
    podman build --arch amd64 -t "$base" -f tools/linux/Containerfile tools/linux
    podman build --arch amd64 -t "$image" -f - tools/linux <<'CONTAINERFILE'
FROM localhost/savagewind-build:amd64
RUN apt-get update && apt-get install -y --no-install-recommends libssl-dev zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*
CONTAINERFILE

    # Mount only the output directory writable. Disable SELinux labels for this container so
    # the host checkout keeps its existing labels, including on Bazzite.
    podman run --rm -i --arch amd64 --userns=keep-id --security-opt label=disable \
        -v "$PWD:/src:ro" -v "$PWD/build:/src/build:rw" -w /src -e HOME=/tmp \
        "$image" bash -se <<'BUILD'
set -euo pipefail
prefix=/src/build/linux/x86_64/deps
CC=clang CXX=clang++ sh tools/build_multiplayer_deps.sh "$prefix"

# package.sh resolves the shared libraries by soname from this prefix.
openssl_libdir=$(pkg-config --variable=libdir openssl)
cp -L "$openssl_libdir/libssl.so.3" "$openssl_libdir/libcrypto.so.3" "$prefix/lib/"

bash tools/linux/gen_helper.sh x86_64
bash tools/linux/package.sh x86_64
BUILD
}

windows_tools_ready() {
    [[ ${MSYSTEM:-} == UCRT64 ]] || return 1
    local tool
    for tool in clang clang++ ld.lld make git cmake ninja windres pkg-config ldd curl; do
        command -v "$tool" >/dev/null || return 1
    done
    pkg-config --exists sdl3 libavformat libavcodec libavutil libswscale openssl shaderc vulkan zlib &&
        [[ -f /ucrt64/include/vulkan/vulkan.h ]] &&
        py -3.12 --version >/dev/null 2>&1 &&
        tools/venv/Scripts/python.exe -c 'import lz4, capstone, unicorn' >/dev/null 2>&1
}

build_windows() {
    case $(uname -s) in
        MSYS*|MINGW*) ;;
        *) echo 'Build Windows on an x86-64 Windows host using build.bat or Windows Bash.' >&2; exit 1 ;;
    esac
    local mode game_root build_dir gen_dir rtc_prefix probe_dir dependencies dependency
    mode=game
    game_root=extracted
    case "${1:-}" in
        --app-only) [[ $# == 1 ]] || { usage >&2; exit 2; }; mode=app ;;
        --game) [[ $# == 2 ]] || { usage >&2; exit 2; }; game_root=$2 ;;
        '') [[ $# == 0 ]] || { usage >&2; exit 2; } ;;
        *) usage >&2; exit 2 ;;
    esac

    if [[ $(uname -m) != x86_64 ]]; then
        echo 'The Windows build requires x86-64 MSYS2 UCRT64.' >&2
        exit 1
    fi
    if [[ $mode == game && $# == 0 && ! -f $game_root/exefs/main && -t 0 ]]; then
        printf 'BOTW 1.9.0 folder (leave blank for an app-only build): '
        IFS= read -r game_root || exit 1
        game_root=${game_root#\"}
        game_root=${game_root%\"}
        if [[ -z $game_root ]]; then mode=app; fi
    fi
    if [[ $mode == game ]]; then
        game_root=$(cygpath -u "$game_root")
        if [[ ! -f $game_root/exefs/main || ! -d $game_root/romfs ]]; then
            echo "Expected exefs/main and romfs/ in $game_root. Use --game to point at your BOTW 1.9.0 folder." >&2
            exit 1
        fi
        game_root=$(cd -- "$game_root" && pwd)
    fi

    if ! windows_tools_ready; then
        if [[ ${SAVAGEWIND_WINDOWS_SETUP_DONE:-} == 1 ]]; then
            echo 'Windows build prerequisites are still unavailable after setup. See docs/BUILD-windows.md.' >&2
            exit 1
        fi
        command -v powershell.exe >/dev/null || { echo 'Windows PowerShell is required for setup.' >&2; exit 1; }
        powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$(cygpath -w tools/setup_windows.ps1)"
        export SAVAGEWIND_WINDOWS_SETUP_DONE=1
        export MSYSTEM=UCRT64 CHERE_INVOKING=1
        if [[ $mode == game ]]; then
            exec /c/msys64/usr/bin/bash.exe -l ./build.sh windows --game "$(cygpath -w "$game_root")"
        fi
        exec /c/msys64/usr/bin/bash.exe -l ./build.sh windows --app-only
    fi

    export JOBS=${JOBS:-8}
    export MP_RTC_PREFIX=build/multiplayer-deps
    rtc_prefix="$PWD/$MP_RTC_PREFIX"
    CC=clang CXX=clang++ sh tools/build_multiplayer_deps.sh "$rtc_prefix"
    export PATH="$rtc_prefix/bin:$PATH"
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
    trap "$(printf 'rm -f -- %q; rmdir -- %q' "$probe_dir/savagewind.exe" "$probe_dir")" EXIT
    cp "$build_dir/savagewind.exe" "$probe_dir/"
    dependencies=$(ldd "$probe_dir/savagewind.exe")
    if [[ $dependencies != *'SDL3.dll => '* || $dependencies == *'not found'* ]]; then
        printf 'Could not resolve Windows runtime DLLs with the UCRT64 toolchain.\n%s\n' "$dependencies" >&2
        exit 1
    fi
    while IFS= read -r dependency; do
        case "$dependency" in
            /ucrt64/bin/*|"$rtc_prefix"/bin/*)
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
}

if (( $# == 0 )); then
    printf 'Choose a build platform:\n  1  Linux (AppImage)\n  2  Windows\n'
    printf 'Platform [1/2]: '
    IFS= read -r platform || exit 1
    case $platform in
        1) set -- linux ;;
        2) set -- windows ;;
        *) usage >&2; exit 2 ;;
    esac
fi

platform=$1
shift
case ${platform,,} in
    linux) build_linux "$@" ;;
    windows) build_windows "$@" ;;
    --help|-h) usage ;;
    *) usage >&2; exit 2 ;;
esac
