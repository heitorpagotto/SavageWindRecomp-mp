# Agent handoff

Checkpoint 2026-11-23 on `multiplayer/windows-test`, based on `e92518d` with the M2 area changes in the worktree.
The M2 version label is `0.6.0-alpha.6-0.1.0-mp`. Read [AGENTS.md](../AGENTS.md) and
[CONTRIBUTING.md](../CONTRIBUTING.md) before making changes. Use RTK for supported shell commands and
separate scratch settings and saves for game tests. Keep generated code and game data local.

## M2 acceptance follow-up

- Mixed Windows/Linux sessions are a manual follow-up for the owner. M2 can be accepted without
  that joint-platform run. Both build targets still use the same version label and source fingerprint.
- The current range is 50 m to spawn and 60 m to retain. The earlier 100/120 m policy allowed the
  engine to delete actors before multiplayer withdrew them.
- The Windows launcher is `build/m2-area-build/Play-M2-Area.cmd`. Match source ID
  `e00ea2478251cec97481ac59c2a297346ec1b855` and game library `e640fd858dc0d033` for this checkpoint.
  The Windows ZIP recipe now collects libdatachannel from the configured dependency prefix.
  The verified Windows artifact is `build/dist/SavageWind-0.6.0-alpha.6-0.1.0-mp-windows.zip`.
  See the [test record](TESTING.md#m2-acceptance-follow-up) for range, capacity, and packaging evidence.
- The ignored `build/m2-probe/run_accept_range.py`, `run_accept_capacity.py` and
  `run_accept_roundtrip.py` exercise the latest build. Two native Windows shrine round trips passed.
  One actual renderer plus seven headless clients passed the capacity hold and measured the requested
  frame rates. Eight full clients still need additional machines or a host with more RAM.
- The ignored `build/m2-probe/run_accept_udp.py` passed a two-game run using the test-only
  libdatachannel build in `build/m2-udp-fixture/obj/`. See the test record for loss, delay,
  movement, facing, and health checks.

## M2 shrine identity and native actor retry

- The RomFS stage tracker now supplies a stable multiplayer area ID for the main field and each shrine.
  Proxies require a fresh pose in the same area. An area change retires the old proxies even if the guest
  reuses the same actor or spawn context. Both Windows instances visibly rendered a remote Link in
  the same shrine. Two instances in different shrines kept the same lobby with no remote actors.
- The earlier build reproduced the native actor loss at about 92 m on the open Plateau with no Slate travel.
  Both proxies stayed active near 30 and 60 m. The retry experiment limited repeated spawns after native
  actor loss beyond 60 m and retried sooner when the players moved at least 10 m closer. A local write watch
  caught the game's native actor deletion routine setting the flag before the multiplayer tick retired
  the slot. The native condition that triggers deletion remains open.
  [Test record](TESTING.md#m2-shrine-identity-and-native-retry)
- The historical 100/120 m retry experiment used source ID `cfca0ce8eae2beffbe4b682fc9d176d656f66d85`.
  The current Windows launcher and source ID are listed above.
- The ignored `build/m2-probe/build_area.py` and `run_area.py` use an isolated clipboard and scratch saves.
  The runner accepts `shrine`, `mixed`, `range`, or `entry`. With `SWMP_SHRINE_MAP=1`, the shrine run
  accepts input through its scratch `map-input` file. One client used the native elevator to return to
  the main field and both proxies retired. In a separate test, a client entered from outdoors and both
  proxies activated after arrival. The lobby stayed connected during both loads.
  The ignored watch probe and logs in `build/m2-area-live-xb_xb_z1/` contain the guest call chain for deletion.

## M2 visibility follow-up

- In the latest field test a Linux client joined the owner's Windows host, but neither could see the other.
  With a friend hosting on Linux, the Linux client initially saw the host, and the Windows client joined from
  farther away. After the Linux client used Slate fast travel, it no longer saw either player. The owner also
  reported that walking a short distance was enough for proxies to disappear.
- The earlier build published positions only inside a 20-by-20 m rectangle near the Shrine of Resurrection,
  with a 12 m height band. The follow-up samples the main field and spawns proxies within 100 m of the local
  player. It retains existing requests and actors up to 120 m to prevent repeated spawning near the boundary.
  Loading, a changed spawn context or local actor, and unavailable remote positions retire the corresponding
  actors. A fresh nearby pose can spawn a replacement in the same lobby.
- Two Windows scratch games stayed in the same lobby while the client used the native Slate to travel to
  Great Plateau Tower and back to the Shrine of Resurrection. The returned proxy actors repeatedly became
  inactive at about 92 m separation. At about 4 m apart outdoors, both players were visible again. Use the
  logs in `build/m2-slate-kcjjyehi/` to compare intermittent native actor activation with the first stable
  reunion. [Test record](TESTING.md#m2-visibility-follow-up)
- The earlier `build/m2-version-build/Play-M2-Version.cmd` used the local 1.9.0 gamelib with
  ID `e640fd858dc0d033`. Build the Linux app from the same source and confirm both runs show source ID
  `3344f886df6a2e0a8a90d4dedddc1cc95f37380e`. For a visibility test, confirm both names and health are
  present in F1, meet at the same overworld waypoint, and face each other. On failure, keep the `[multiplayer]`
  lines from both sides, the two locations, and the lobby roster outcome.
  See [testing](TESTING.md#m2-visibility-follow-up).

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
  See the [build compatibility guide](../docs/build-windows-and-appimage.md#multiplayer-build-compatibility).
- Local game-library IDs encode platform and CPU build flags. The matching Windows x86-64-v3 library is
  `e640fd858dc0d033`. The lobby compares the multiplayer source IDs and BOTW main build IDs.
  Use a fresh invite and reply for each join attempt.
- The latest Windows-host test admitted a Linux client. Visible proxies in that direction remain unverified. Use the
  [direct multiplayer handoff](DIRECT_MULTIPLAYER_HANDOFF.md) for the clipboard flow and transport diagnostics.
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
3. Trace mismatches with [launcher readiness and auto-setup](../runtime/ui/launcher.mm),
   [game-library lookup](../runtime/gamelib.c), [library and ID writes](../runtime/gamebuild.c), and
   [data-root selection](../runtime/paths.c). Preserve the installed files and their IDs during diagnosis.

A packaged AppImage opens Configuration by default. The **Play** button starts a ready game, and
**Game Files → Open this screen first** controls that landing page. **Set up game files** can deliberately
rebuild the library. The reported automatic version rejection still needs investigation.

## M2 remote Link presence

- Position, facing, idle, locomotion and jump are implemented for the Plateau test area. The passive
  RomFS alias uses the installed Link model and animation archive. Proxy placement and playback use
  version-aware native calls on an owned actor. The wire action IDs are idle 0, jump 1, and walk 2.
  Health is sampled as quarter-hearts and displayed in the F1 roster.
- Two Windows games verified actions in both directions, independent inventory, area withdrawal and return,
  same-slot rejoin and reboot cleanup. One renderer sustained seven remote Links driven by headless WebRTC
  fixtures through START and disconnect. The health test relayed a controlled change from 12 to 9 and back
  to 12 quarter-hearts while the host stayed at 12. The [test record](TESTING.md#m2-plateau-animation-check)
  has GPU, FPS, settings, builds and evidence paths.
- Appearance and reliable equipment revisions are planned for M2.5, after M2 is complete. The passive proxy failed to
  activate when the scratch model list included a second model for default hair, including with an 8 MiB
  instance heap. Start from one native equipped model on an owned proxy. The [M2 handoff](M2_POSITION_HANDOFF.md#resume)
  has source entry points and probe paths.
- See the [milestone ledger](MILESTONES.md#m2--remote-link-presence) for acceptance status.
  The owner-confirmed cross-platform join verifies lobby admission. Joint-platform travel and presence
  remain on the owner's manual test list.
