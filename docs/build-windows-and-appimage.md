# Build Savage Wind for Windows and Linux AppImage

Use the same fork source revision on both systems. Carry over any local source edits you want in both builds.
The Windows steps produce a local x86-64 executable and its BOTW 1.9.0 game library. The Linux steps produce
an x86-64 AppImage that builds the game library on the player's machine during setup.

## Build scripts

On x86-64 Linux, including Bazzite, run this from a regular source checkout with Podman available:

```bash
bash build-linux.sh
```

The script builds in an Ubuntu container and writes the AppImage under `build/dist/`. The host needs Podman,
and the compiler and development libraries live in the container. Use a source checkout without extracted game
files or a locally generated `runtime/game_place_table.inc` when packaging.

On Windows, complete the one-time UCRT64 setup and package installation below, then run:

```bash
bash build-windows.sh --game '/c/path/to/your/BOTW-1.9.0'
```

Omit `--game` to use `extracted/`, or use `--app-only` to compile the executable without generating a game
library. Set `JOBS=4` before the command if the compiler runs out of memory. Outputs are in
`build/windows-local/`. The script stages the runtime DLLs for Explorer launches and the compiler, generator,
and build headers for **Set up game files**. To play with game files stored elsewhere, follow the launch step below.

The manual steps below give the individual build commands and include the optional Windows ZIP workflow.

## Windows x86-64 local build

1. Open PowerShell in the repository root. Run the one-time setup if you need MSYS2 UCRT64, Clang, Vulkan
   development libraries, or the Python environment in `tools/venv`.

   ```powershell
   powershell -ExecutionPolicy Bypass -File tools\setup_windows.ps1
   ```

2. Open **MSYS2 UCRT64** and change to the repository root. Install the multiplayer build dependencies and build
   the pinned libdatachannel library.

   ```bash
   pacman -S --needed git mingw-w64-ucrt-x86_64-{openssl,cmake,ninja}
   CC=clang CXX=clang++ sh tools/build_multiplayer_deps.sh
   export MP_RTC_PREFIX="$PWD/build/multiplayer-deps"
   export PATH="$MP_RTC_PREFIX/bin:$PATH"
   ```

3. Generate the game code from your own extracted BOTW 1.9.0 files. If your game folder is already
   `extracted/` in this checkout, run `make gen`. For a game folder elsewhere, set `GAME_ROOT` to
   the folder containing both `exefs/` and `romfs/` and run the generator directly:

   ```bash
   GAME_ROOT='/c/path/to/your/BOTW-1.9.0'
   ./tools/venv/Scripts/python.exe tools/gen.py all "$GAME_ROOT/exefs" generated \
     --hle runtime/hle/hle_symbols.txt --locals
   ```

4. Compile the Windows app and guest library. If the compiler is killed for lack of memory, rerun with `-j4`.
   Use the same output directory on later builds to reuse compiled objects.

   ```bash
   make B=build/windows-local -j8
   ls -l build/windows-local/{savagewind.exe,swgame.dll,swgame.dll.id}
   ```

5. Launch from the same UCRT64 shell so Windows finds the dependency DLLs. Set `GAME_ROOT="$PWD/extracted"`
   if you used `make gen`. The smoke test keeps settings and saves in a scratch data directory.

   ```bash
   mkdir -p build/windows-smoke
   SAVAGEWIND_DATA_ROOT="$(cygpath -m "$PWD/build/windows-smoke")" \
   SAVAGEWIND_NO_LAUNCHER=1 \
     ./build/windows-local/savagewind.exe "$(cygpath -m "$GAME_ROOT")"
   ```

   Check the startup log for the expected `swgame.dll` and game version 1.9.0. Open F1 Settings and look for
   Multiplayer. Test the invite and reply exchange with a friend using the same build.
   [Multiplayer setup](../mods/multiplayer/README.md)

### Optional Windows ZIP

Use the same UCRT64 shell and `MP_RTC_PREFIX`. The ZIP contains the tools for players to build their own game
library. The `dist` target recreates `build/dist/`, so move any AppImage or earlier ZIP you want to retain
before running it.

```bash
make deps-windows
make gen-helper
bash tools/fetch_zig.sh
cp "$MP_RTC_PREFIX/bin/libdatachannel.dll" build/deps-win/bin/
make dist DIST_PGO= DIST_LTO=
```

Staging the DLL under `build/deps-win/bin/` puts it on the packager's dependency search and copy paths.
`DIST_PGO=` and `DIST_LTO=` select a plain build with the installed Clang. The result is
`build/dist/SavageWind-<version>-windows.zip`. Extract it to a fresh folder and launch
`savagewind.exe --config` from PowerShell with a normal Windows PATH to check that the ZIP supplies the DLLs.
[Windows packaging reference](BUILD-windows.md#package)

## Linux x86-64 AppImage

Run these steps from an x86-64 Linux system with Podman. Start from the fork checkout containing your Windows
build's source changes. This fork adds libdatachannel and OpenSSL to the original AppImage dependencies.
Stage those libraries in the Ubuntu 22.04 build prefix before running the package script. The procedure is
for a local test AppImage. Native Linux packaging and release validation are pending.
[M1 test record](../.milestones/TESTING.md#native-build-and-standalone-checks)

1. From the repository root, build the repository's AppImage container and start a shell inside it. On
   SELinux systems, use `-v "$PWD:/src:Z"` if the container cannot access the checkout.

   ```bash
   podman build --arch amd64 -t localhost/savagewind-build:amd64 \
     -f tools/linux/Containerfile tools/linux
   podman run --rm -it --arch amd64 -v "$PWD:/src" -w /src \
     localhost/savagewind-build:amd64 bash
   ```

2. In the container shell, install the OpenSSL and zlib headers. Build libdatachannel into the same
   prefix that the packager scans for shared libraries.

   ```bash
   set -e
   apt-get update
   apt-get install -y --no-install-recommends libssl-dev zlib1g-dev

   prefix=/src/build/linux/x86_64/deps
   CC=clang CXX=clang++ sh tools/build_multiplayer_deps.sh "$prefix"
   ```

3. Stage the OpenSSL runtime libraries in the prefix. The AppImage packager follows shared-library
   dependencies by soname.

   ~~~bash
   openssl_libdir="$(pkg-config --variable=libdir openssl)"
   cp -L "$openssl_libdir/libssl.so.3" "$openssl_libdir/libcrypto.so.3" "$prefix/lib/"
   ~~~

4. In the same shell, build the generator helper and run the package script. The package script also builds
   the remaining platform libraries, the app, and the AppImage.

   ```bash
   bash tools/linux/gen_helper.sh x86_64
   bash tools/linux/package.sh x86_64
   exit
   ```

5. Back on the Linux host, run the AppImage on a machine with a Vulkan-capable driver. Replace `<version>`
   with the filename printed by the package script.

   ```bash
   image='build/dist/Savage_Wind-<version>-x86_64.AppImage'
   chmod +x "$image"
   mkdir -p build/appimage-smoke
   SAVAGEWIND_DATA_ROOT="$PWD/build/appimage-smoke" "$image" --config
   ```

   If mounting fails because FUSE is unavailable, add `--appimage-extract-and-run` before `--config`. Use
   the setup screen to point the app at your own game files. Use the Multiplayer tab to trade an invite and reply.
   [Linux runtime and platform requirements](linux.md#running-the-appimage)

## Local files and release packages

Keep the extracted game, generated C sources, game library, saves, and settings local. For redistribution,
complete the release checks for the bundled network libraries and their notices.
[Release hardening](../.milestones/MILESTONES.md#m6--release-hardening)
