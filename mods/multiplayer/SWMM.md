# Savage Wind Mod Manager integration

## Build and install the ZIP

Run the command for your platform from the repository root.

**Windows (PowerShell)**

```powershell
rtk bypass python tools/package_multiplayer.py build/mods/SavageWind-Multiplayer-0.1.0-dev-SWMM.zip
```

**Linux**

```sh
rtk bypass python3 tools/package_multiplayer.py build/mods/SavageWind-Multiplayer-0.1.0-dev-SWMM.zip
```

Both commands produce the same platform-independent marker ZIP without source changes. To use it in-game,
build the multiplayer test executable with `MP_M1_TEST=1` for your platform and set up BOTW 1.9.0.
[Test executable build guide](../../.milestones/TESTING.md#m1-developer-build)

1. In **SWMM → Settings**, set **Savage Wind executable** to your test build. Use `savagewind.exe` on Windows
   or the Linux/Vulkan `savagewind` executable on Linux.
2. Open `build/mods` in your file manager and drag **SavageWind-Multiplayer-0.1.0-dev-SWMM.zip** onto SWMM's **Home** tab.
3. Make sure **Savage Wind Multiplayer** is enabled, then click **Apply**, **Deploy**, and **Play**.
4. In the game, press **F1** and open **Multiplayer**. Host and Join require a configured HTTPS rendezvous server.
   [Rendezvous setup](../../.milestones/TESTING.md#m1-developer-build)

The multiplayer sources have been cross-compiled for Linux. Native Linux gameplay, the Linux SWMM workflow,
and mixed-platform lobbies are still pending verification. [M1 test record](../../.milestones/TESTING.md#current-m1-evidence)

## Development package

The target manager is [SWMM](https://github.com/ustruct/SWMM). Its current 0.1.x releases import ordinary BOTW
content mods, merge them, and deploy a RomFS overlay selected through `SAVAGEWIND_ROMFS_OVERLAY`.

`make multiplayer-swmm-package` creates a deterministic archive with metadata and a discovery marker under the
Switch title's RomFS directory. `make test-multiplayer-swmm` validates its exact contents.
`make test-multiplayer-swmm-cli SWMM_EXE=/path/to/swmm` exercises the official portable manager with an isolated
synthetic profile. Windows SWMM 0.1.3 passed install, merge, deploy, and uninstall.

The opt-in `MP_M1_TEST=1` build reads the deployed marker to show the Multiplayer tab. Windows SWMM 0.1.3's GUI
passed drag-and-drop, enable, deploy, launch, disable, and uninstall with scratch BOTW 1.9.0 data.
SWMM's built-in loader handles the RomFS overlay, motion input, and camera sensitivity. Third-party multiplayer
module approval and loading require the production integration below.
[Windows GUI test record](../../.milestones/TESTING.md#m1-windows-swmm-gui)

Multiplayer ultimately needs native code, so production integration requires one of these explicit contracts:

1. SWMM adds a native-plugin permission and passes approved plugin paths to Savage Wind; or
2. Savage Wind gains a signed/approved native-plugin ABI which SWMM can manage.

The first is the preferred user experience because SWMM can show a clear warning before enabling executable code and
can remove it cleanly with the profile. We must not silently load arbitrary DLLs or shared objects found in a content
overlay: that would turn any downloaded data mod into native code execution.

The [draft manifest and approval handoff](../../.milestones/NATIVE_ABI.md) proposes how SWMM could select and approve
the compiled module for each platform. `make multiplayer-native-fixture-package` creates a separate draft ABI
fixture ZIP for maintainers. For the GUI, use the marker archive.

