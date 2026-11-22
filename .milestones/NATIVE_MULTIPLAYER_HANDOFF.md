# Native multiplayer handoff

Direction recorded 2026-11-22. Working branch `multiplayer/windows-test`, base `ae7ff1f` with local M1 changes.

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
- Windows and Linux source builds link multiplayer. F1 and System > Options share a default-off `multiplayer`
  setting and the runtime's copied lobby state. Both surfaces offer Host, Join, Start, and Leave. Native Options
  uses the existing keyboard for invite entry. Build the transport with `tools/build_multiplayer_deps.sh`.
- `runtime/multiplayer.c` owns lobby and transport state on the window thread, receives UI commands, and
  publishes copies of the session and admitted roster. `runtime/main.c` and `runtime/nvn/vulkan/device.c`
  provide the boot, polling, and shutdown call sites.
- `runtime/multiplayer_presence.c` uses strict `MP_PRESENCE_SITES` and the existing guest-thread menu update
  hook for `Npc_TripMaster_00` on the Plateau. Local lobby and slot generations govern replacement and cleanup.
  Prototype rock evidence and the current native test results are recorded separately. [Test record](TESTING.md)
- Native Windows builds, encrypted eight-peer loopback, and gameplay with the shared setting have passed.
  The prototype also passed a controlled local TURN test. Native Linux execution, mixed-platform sessions,
  and external-network direct and relay tests remain pending.

## Resume here

1. Read [AGENTS.md](../AGENTS.md), [CONTRIBUTING.md](../CONTRIBUTING.md), and the [test record](TESTING.md).
2. Finish the [native M1 acceptance checks](TESTING.md#native-m1-acceptance) on Windows and Linux/Vulkan. Test
   both mixed-platform host directions and external direct ICE and TURN. Use separate scratch settings and saves
   for each instance. Local loopback exercises the encrypted lobby and HTTPS broker.
3. Record the results in [TESTING.md](TESTING.md), [MILESTONES.md](MILESTONES.md), and [state.json](state.json).
   Record the upstream release and rerun the relevant checks whenever the owner brings in an update.
