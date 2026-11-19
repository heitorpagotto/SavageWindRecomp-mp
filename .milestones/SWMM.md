# Savage Wind Mod Manager findings

## Sources inspected

Research snapshot date: 2026-11-18.

- SWMM repository: `https://github.com/ustruct/SWMM`
- Inspected SWMM commit: `b81a3afe413060667c649433ec32ead54ef471d9`
- SWMM release observed: 0.1.2.
- SWMM uses a fork of UKMM.
- Inspected UKMM `swmm` branch commit: `d264dbbaf62e8ad474bc26fe14a90b538111ff10`

Temporary inspection clones were placed under `/tmp` and are not project dependencies.

## What current SWMM does

- Imports folders and archives including ZIP, 7z, RAR, and BNP.
- Recognizes UKMM packaged mods (`meta.yml` + `manifest.yml`), BNP metadata (`info.json`), Cemu `rules.txt`, and raw
  RomFS layouts.
- A raw `romfs/` folder is staged as Switch/little-endian content.
- Unknown resources can be retained as binary data by UKMM.
- Merges enabled mods into a profile and deploys a RomFS overlay.
- Launches Savage Wind with `SAVAGEWIND_ROMFS_OVERLAY` pointing at that overlay.
- On Windows, its own proxy `pdh.dll` loader sets the overlay for standalone launches and implements SWMM motion/camera
  behavior.
- On Linux, it persists/exports the overlay environment and updates applicable Steam shortcuts.

## What current SWMM does not do

- It exposes no general third-party native plugin ABI.
- It does not enumerate and load arbitrary multiplayer DLLs/shared objects.
- Its import path treats common executable/native extensions as non-RomFS metadata at the top level.
- There is no native-code permission prompt or approved plugin-path handoff.

Therefore a normal SWMM content package cannot safely make the current C multiplayer core run inside Savage Wind.

## Development package

`make multiplayer-swmm-package` builds a raw archive SWMM can recognize from source inspection:

- `info.json` provides name, description, version, Switch platform, and empty options.
- `romfs/System/SavageWind/Multiplayer/package.swmp` is an added binary/text resource under a recognized `System`
  RomFS root.

The package proves deterministic packaging and gives the future bridge a stable discovery marker. It has not been
manually installed through the SWMM GUI in this environment. Do not record the GUI lifecycle as verified until a
person actually drags it into SWMM, enables it, deploys, launches, disables, and uninstalls it.

## Required safe integration

Preferred flow:

1. SWMM recognizes a native component in an installed mod.
2. It presents a clear executable-code permission warning.
3. On approval, it deploys platform-appropriate signed/hashed binaries outside ordinary RomFS data.
4. It passes an explicit allowlist of approved absolute plugin paths to Savage Wind.
5. Savage Wind loads only those paths through a small versioned ABI.
6. Disabling/uninstalling the profile removes the allowlist and deployed plugin.

Alternative: Savage Wind verifies an embedded publisher signature and loads only approved native modules. This is more
centralized and needs a key/release process. It should not be improvised without maintainers.

Unsafe approach to reject: scan `SAVAGEWIND_ROMFS_OVERLAY` and execute anything with a DLL/shared-object extension.
That would silently convert an ordinary downloaded content mod into arbitrary native code execution.

## Cross-repository implication

Full one-click SWMM support likely requires a focused change in the separate GPL-3.0 SWMM repository plus the plugin
ABI/loader in Savage Wind or this fork. Do not copy SWMM source into this MIT repository. Define an ABI/manifest and
implement each side in its own repository/license.

