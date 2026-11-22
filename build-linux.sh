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

curl_version=8.23.0
archive="curl-$curl_version.tar.xz"
work=/src/build/linux/x86_64/curl-build
if [[ ! -f "$prefix/lib/libcurl.so" ]] ||
   [[ $(sh "$prefix/bin/curl-config" --version 2>/dev/null) != "libcurl $curl_version" ]]; then
    mkdir -p "$work"
    (
        cd "$work"
        if [[ ! -f $archive ]]; then
            curl -fsSL "https://github.com/curl/curl/releases/download/curl-${curl_version//./_}/$archive" \
                -o "$archive.part"
            mv "$archive.part" "$archive"
        fi
        printf '%s  %s\n' 8dadd7d2536a314f982ad05a651b713f426bf7d02cc2c76cffb9796f2218e6c5 "$archive" \
            | sha256sum -c -
        tar -xf "$archive"
        cd "curl-$curl_version"
        CC=clang ./configure --prefix="$prefix" --libdir="$prefix/lib" \
            --enable-shared --disable-static --with-openssl --enable-threaded-resolver \
            --with-zlib --without-libpsl --without-brotli --without-zstd --without-libidn2 \
            --disable-ldap --disable-ldaps
        make -j"$(nproc)"
        make install
    )
fi

# package.sh resolves the shared libraries by soname from this prefix.
openssl_libdir=$(pkg-config --variable=libdir openssl)
cp -L "$openssl_libdir/libssl.so.3" "$openssl_libdir/libcrypto.so.3" "$prefix/lib/"
curl_info=$(LD_LIBRARY_PATH="$prefix/lib" "$prefix/bin/curl" --version)
grep -Eq '^Protocols:.*\bhttps\b' <<<"$curl_info"
grep -Eq '^Features:.*\bAsynchDNS\b' <<<"$curl_info"

bash tools/linux/gen_helper.sh x86_64
bash tools/linux/package.sh x86_64
BUILD
