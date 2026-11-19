# Savage Wind Mod Manager integration

The target manager is [SWMM](https://github.com/ustruct/SWMM). Its current 0.1.x releases import ordinary BOTW
content mods, merge them, and deploy a RomFS overlay selected through `SAVAGEWIND_ROMFS_OVERLAY`.

`make multiplayer-swmm-package` creates a deterministic raw archive that can be dragged into SWMM. It contains only
metadata and `System/SavageWind/Multiplayer/package.swmp`, a development discovery marker. It contains no game data,
generated code, assets, save files, or executable code. `make test-multiplayer-swmm` validates its exact contents.

This archive lets us test the manager-facing lifecycle now: install, add to a profile, enable/disable, deploy, and
uninstall. It does not make the current protocol demo run inside the game. SWMM 0.1.x has no third-party native-plugin
contract; its native loader handles the RomFS overlay, motion input, and camera sensitivity itself.

Multiplayer ultimately needs native code, so production integration requires one of these explicit contracts:

1. SWMM adds a native-plugin permission and passes approved plugin paths to Savage Wind; or
2. Savage Wind gains a signed/approved native-plugin ABI which SWMM can manage.

The first is the preferred user experience because SWMM can show a clear warning before enabling executable code and
can remove it cleanly with the profile. We must not silently load arbitrary DLLs or shared objects found in a content
overlay: that would turn any downloaded data mod into native code execution.

The package path and marker are stable development contracts. Once the native bridge exists, the archive will gain
Windows and Linux binaries without changing how users install it: drag the archive into SWMM, enable it, and press
Play.

