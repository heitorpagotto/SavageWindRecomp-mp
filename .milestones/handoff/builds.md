# Windows and Linux build compatibility

## Windows and Linux lobby compatibility

- The old `build/windows-local/savagewind.exe` embedded source ID
  `a90e1242b88646f11812935c9a9f19517d723d15`. The current Linux host and M2 Windows client both reported
  source ID `970ccbd296037d6d9aec806b7c0c22dca89d1d9e` and BOTW main build ID
  `cd57b23fa4bbad65803d9788c01821ee00000000000000000000000000000000` (1.9.0).
- The working launch path is the ignored `build/m2-health-build/Play-Matching-M2.cmd`. It runs the executable
  and game library in its own folder, resolves DLLs from `build/windows-local` and
  `build/multiplayer-deps/bin`, and defaults `SAVAGEWIND_DATA_ROOT` to `build/windows-local` so it uses
  the existing data, saves, and settings. It honors an explicit data-root override. The Windows scratch
  startup log in `build/mp-matching-boot-3sky6sm5/startup.log` verified both IDs before the owner confirmed the join.
  The old executable is still present in its original folder.
- On a later rejection, compare the `[multiplayer] source ID: …, game build: …` line from each actual run.
  Windows development builds write it to the launching terminal. Windows builds without a console or redirected
  stderr write `savagewind.log` beside the executable. Linux writes to the data folder, usually
  `~/.local/share/savagewind/savagewind.log`. Use `rtk make -s print-multiplayer-id` for the source tree's
  expected ID. The Windows and Linux Make targets produced the same ID with the same source inputs.
  See the [build compatibility guide](../../docs/build-windows-and-appimage.md#multiplayer-build-compatibility).
- Local game-library IDs encode platform and CPU build flags. The matching Windows x86-64-v3 library is
  `e640fd858dc0d033`. The lobby compares the multiplayer source IDs and BOTW main build IDs.
  Use a fresh invite and reply for each join attempt.
- The latest Windows-host test admitted a Linux client. Visible proxies in that direction remain unverified. Use the
  [direct multiplayer handoff](../DIRECT_MULTIPLAYER_HANDOFF.md) for the clipboard flow and transport diagnostics.
  That handoff also records the OpenSSL and DTLS checks pending before distribution.

## Linux AppImage recompiles on launch

The owner double-clicks the AppImage from `build/dist/`. Their log contained
`[launcher] setting up this version by itself (Game code built for another version)`. The launcher found
`swgame.so` and could not match a readable adjacent `swgame.so.id` to the selected target. The exact cause
is open. The owner has yet to supply the two files' metadata and ID after a completed build.

1. After setup completes and the game closes, capture the files before reopening the AppImage. Use the
   folder shown in the log's `[paths] data:` line for `root`.

   ```sh
   root="${XDG_DATA_HOME:-$HOME/.local/share}/savagewind"
   ls -l "$root/extracted/swgame.so" "$root/extracted/swgame.so.id"
   cat "$root/extracted/swgame.so.id"
   ```

2. Compare that root and the selected CPU target across launches. For `7991b79` with the default x86-64
   Linux Make flags, the locally calculated IDs are `f24383c5199a514c` (v3) and `0bfc4f56bfc9a721` (v2).
   Confirm the actual AppImage build and flags before using those values to judge the installation.
3. Trace mismatches with [launcher readiness and auto-setup](../../runtime/ui/launcher.mm),
   [game-library lookup](../../runtime/gamelib.c), [library and ID writes](../../runtime/gamebuild.c), and
   [data-root selection](../../runtime/paths.c). Preserve the installed files and their IDs during diagnosis.

A packaged AppImage opens Configuration by default. The **Play** button starts a ready game, and
**Game Files → Open this screen first** controls that landing page. **Set up game files** can deliberately
rebuild the library. The reported automatic version rejection still needs investigation.
