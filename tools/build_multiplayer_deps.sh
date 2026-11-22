#!/bin/sh
# Build the data-channel library with libjuice ICE and OpenSSL for a native source build.
set -eu
prefix=${1:-"$PWD/build/multiplayer-deps"}
src="$prefix/src"
revision=6b1e2e620f1e37f0eafeee702eaea0043cb305fd  # libdatachannel v0.24.6
mkdir -p "$prefix"
if [ ! -d "$src/.git" ]; then
    git clone --depth 1 --branch v0.24.6 https://github.com/paullouisageneau/libdatachannel.git "$src"
fi
[ "$(git -C "$src" rev-parse HEAD)" = "$revision" ] || {
    echo "Unexpected libdatachannel revision in $src" >&2
    exit 1
}
git -C "$src" submodule update --init --depth 1 deps/libjuice deps/plog deps/usrsctp
cmake -S "$src" -B "$prefix/obj" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix" \
      -DCMAKE_INSTALL_LIBDIR=lib -DNO_MEDIA=ON -DNO_WEBSOCKET=ON -DNO_EXAMPLES=ON -DNO_TESTS=ON
cmake --build "$prefix/obj" --parallel "${JOBS:-8}"
cmake --install "$prefix/obj"
