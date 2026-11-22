# Native multiplayer handoff

Direction recorded 2026-11-22. Working branch `multiplayer/windows-test`, source checkpoint `9fb424b`.
This checkpoint updates the milestone records. The source still implements the prototype described below.

## Owner's direction

The owner's research found that SWMM appears unable to run the arbitrary native code this project needs.
Breathalyzer is not yet a multiplatform mod loader, and its future is uncertain. Drop both approaches and any
other mod-loader integration. Milestone 0.5 and all mod-loader steps in M1 are retired.

Compile multiplayer directly into this Savage Wind fork for Windows and Linux/Vulkan. Provide a Multiplayer
tab in F1 Settings and a Multiplayer page in the game's System > Options. Both surfaces must let players
enable or disable multiplayer and host or join the same session. Keep Leave and orderly session cleanup.

The owner will manually update the fork when new Savage Wind releases come out. Keep the reusable multiplayer
code modular, with small build, settings, UI, lifecycle, and version-aware guest integration points so each
update is straightforward to apply and verify.

M1 still delivers working Windows and Linux builds, an internet lobby, and a visible NPC standing in for each
admitted remote peer in a supported gameplay area. Real remote Links, movement, animation, and equipment are
M2 work. The longer-term goal remains an eight-player, host-authoritative cooperative campaign with shared
combat, story progress, and chest rewards. [Milestone ledger](MILESTONES.md)

## What is already in the branch

- The protocol, session, lobby, mock transport, WebRTC adapter, and HTTPS rendezvous client live in
  `mods/multiplayer/`. The signaling service is `tools/multiplayer_rendezvous.py`.
- The Makefile's `MP_M1_TEST=1` builds a prototype with code already linked into the executable. Its boot
  gate in `runtime/multiplayer.c` checks an overlay marker and the loaded 1.9.0 game. `runtime/ui/menu.mm`
  exposes the tab through the F1 overlay and `runtime/ui/multiplayer.mm` draws it. The new settings toggle
  and the page in `runtime/ui/game_options.mm` still need implementation.
- `runtime/multiplayer.c` owns lobby and transport state on the window thread, receives UI commands, and
  publishes copies of the session and admitted roster. `runtime/main.c` and `runtime/nvn/vulkan/device.c`
  provide the prototype boot, polling, and shutdown call sites.
- `runtime/multiplayer_presence.c` uses strict `MP_PRESENCE_SITES` with a guest-thread menu update hook to
  create and remove static Plateau rocks. It tracks local lobby and slot generations for replacement and cleanup.
  Reuse the lifecycle for the NPC test. Windows prototype runs visibly showed two rocks in each of three game
  instances and exercised cleanup. [Test record](TESTING.md#m1-windows-presence-check)
- Windows encrypted loopback with eight peers, HTTPS certificate rejection, and a controlled local TURN
  route passed under the prototype. Linux cross-compilation covered runtime/UI/test objects. Native Linux
  execution, mixed-platform sessions, external-network direct and relay tests, and the new settings flow are pending.

## Resume here

1. Read [AGENTS.md](../AGENTS.md), [CONTRIBUTING.md](../CONTRIBUTING.md), and the [repository map](REPOSITORY.md).
   Inspect both UI paths and trace the Makefile's prototype activation through boot, window polling, reset,
   and guest actor retirement.
2. Replace multiplayer's marker gate and test-build-only activation with built-in Windows/Linux integration and
   a shared setting. Connect both Settings surfaces to the existing lobby through thread-safe commands and copied
   views. On disable, stop the connection and publish an empty roster so the guest thread retires its actors.
   Keep version and site support checks and leave generic upstream RomFS overlay support alone.
3. Remove the multiplayer-only marker, packaging, and dynamic module fixture paths during the source migration.
   The prototype code, Makefile targets, tests, and documentation under `mods/multiplayer/` still contain these
   remnants. Update those docs and their old milestone links as part of that cleanup. Reuse the existing session,
   transport, and tested guest lifecycle code.
4. Run the [native M1 acceptance checks](TESTING.md#native-m1-acceptance) on Windows and Linux/Vulkan, including
   live NPC visibility, both mixed-platform host directions, and external direct ICE and TURN. Use separate scratch
   settings and saves for each game instance. The prior Windows test used a local 1.9.0 installation. Confirm local
   Linux/Vulkan and two-network test inputs before claiming those results.
5. Record the results in [TESTING.md](TESTING.md), [MILESTONES.md](MILESTONES.md), and [state.json](state.json).
   Record the upstream release and rerun the relevant checks whenever the owner brings in an update.
