#!/usr/bin/env bash
# Build an x86-64 AppImage in rootless Podman from a source checkout.
set -euo pipefail

if [[ ${1:-} == --help ]]; then
    printf 'Usage: bash build-linux.sh\nBuilds an x86-64 AppImage under build/dist using Podman.\n'
    exit 0
fi
if (( $# )); then
    printf 'Usage: bash build-linux.sh\n' >&2
    exit 2
fi
if [[ $(uname -s) != Linux || $(uname -m) != x86_64 ]]; then
    echo 'Run this script on an x86-64 Linux host.' >&2
    exit 1
fi
if ! command -v podman >/dev/null; then
    echo 'Install Podman on the Linux host before running this script.' >&2
    exit 1
fi

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
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
