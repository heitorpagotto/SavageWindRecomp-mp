# Native multiplayer handoff

The native migration was recorded on 2026-11-22 when `multiplayer/windows-test` was at `1ff0af6`. That commit
contains the M1 integration and build guide. The two build scripts and their guide updates were still in the
working tree at that checkpoint. The owner asked to handle commits and pushes themselves.

This records the earlier native migration. See the [direct multiplayer handoff](DIRECT_MULTIPLAYER_HANDOFF.md)
for the current commit, verified direct invite behavior, and open security work.

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

- The protocol, session, lobby, WebRTC adapter, and direct invite and reply exchange live in
  `mods/multiplayer/`. The HTTPS prototype remains a standalone test.
- Windows and Linux source builds link multiplayer. F1 and System > Options share a default-off `multiplayer`
  setting and the runtime's copied lobby state. Both surfaces offer Host, Join, Start, and Leave and share
  clipboard actions for invites and replies. The migration removed the test-build gate, marker package, and
  dynamic-module fixtures. Build the transport with `tools/build_multiplayer_deps.sh`.
- `runtime/multiplayer.c` owns lobby and transport state on the window thread, receives UI commands, and
  publishes copies of the session and admitted roster. `runtime/main.c` and `runtime/nvn/vulkan/device.c`
  provide the boot, polling, and shutdown call sites.
- `runtime/multiplayer_presence.c` uses strict `MP_PRESENCE_SITES` and the existing guest-thread menu update
  hook for `Npc_TripMaster_00` on the Plateau. Local lobby and slot generations govern replacement and cleanup.
  Prototype rock evidence and the current native test results are recorded separately. [Test record](TESTING.md)
- Native Windows builds, encrypted eight-peer loopback, and gameplay with the shared setting have passed.
  The prototype also passed a controlled local TURN test. Native Linux execution, mixed-platform sessions,
  and external-network direct tests remain pending.

## Direct internet invites

The owner confirmed the two-message flow. The host copies an invite, the joining player pastes it and copies
a reply, and the host pastes that reply to connect. Start with Host lobby, then use New invite for each additional
player. The messages carry each peer's ICE candidates and DTLS fingerprint. The host can issue a new invite without
interrupting admitted peers. The window thread owns gathering and clipboard access for both menus.

The app uses a public STUN endpoint for address discovery. Gameplay flows between peers when a direct route exists.
The M1 build uses direct paths, so restrictive network pairs can fail to connect. This supersedes the automatic
HTTPS deployment and launcher setup plan. [Player flow](../mods/multiplayer/README.md#build-and-play)

## Build scripts

- [build-windows.sh](../build-windows.sh) runs in MSYS2 UCRT64. With no arguments it uses `extracted/`.
  With `--game` it reads the supplied BOTW 1.9.0 folder. It builds the app and local guest library under
  `build/windows-local/`. `--app-only` builds the app and configuration executables and passed in UCRT64.
  The script's full game-library path still needs an end-to-end run. The earlier manual Windows build generated,
  linked, and loaded gamelib `6bb7f77965924e25`. Use `JOBS=4` if a build runs out of memory.
- [build-linux.sh](../build-linux.sh) targets x86-64 Linux, including Bazzite. It uses rootless Podman with
  `--userns=keep-id`, mounts the checkout read-only and `build/` writable, and stages pinned libdatachannel,
  and OpenSSL in the Ubuntu 22.04 dependency prefix. It calls the existing helper and AppImage
  packager and writes to `build/dist/`. Use a regular source-only checkout so the build can read the Git
  tree identity and so packaging excludes local game-derived data. Both scripts and the Linux container
  shell body passed Bash syntax checks. No Linux or Podman runner was available for an end-to-end AppImage run.
- The [build guide](../docs/build-windows-and-appimage.md) has prerequisites, the manual equivalents, and the
  optional Windows ZIP steps. The stock Windows `make dist` recreates `build/dist/` and needs libdatachannel
  staged for DLL collection. Release dependency staging and license notices are tracked under M6.

## Windows scratch evidence

When available, `build/m1-tools/` holds the portable UCRT64 toolchain, the build/test logs, and local
automation. `build/m1/` holds the earlier matching app and gamelib and `build/windows-local/` holds the
wrapper's app-only test output. Treat the older binaries as evidence. Rebuild both platforms from the same
current source when testing interoperability because admission compares a platform-neutral source fingerprint
as well as the game's main-module build ID.

The window thread owns network I/O. Turning multiplayer off clears the published roster and queued action
immediately, then tears down transport before accepting another action. The guest update retires actors from
that roster. Test slot reuse with a late join because the same peer ID can represent a new membership.

`build/m1-smoke/` contains local scratch saves, settings, logs, and screenshots. Useful comparisons are
`plateau/plateau-two.png` and `client/client-two.png` before disable, followed by
`plateau/plateau-disabled-world.png` and `client/client-after-host-disabled.png`. The `lifecycle/` run
records a peer dropping on reboot and `restart/` shows the persisted enabled preference. The local Python
runners under `build/m1-tools/` used the owner's extracted game path and a short-lived loopback certificate.
Keep the scratch directories, certificates, invites, game data, and screenshots local.

## Resume here

1. Read [AGENTS.md](../AGENTS.md), [CONTRIBUTING.md](../CONTRIBUTING.md), and the [test record](TESTING.md).
2. Run `bash build-linux.sh` on x86-64 Linux with Podman, preferably Bazzite. Resolve any container or
   dependency staging failures. Launch the resulting AppImage, complete game setup with local 1.9.0 files,
   and exercise both Settings surfaces on Linux/Vulkan.
3. Complete the [native M1 acceptance checks](TESTING.md#native-m1-acceptance). Test both mixed-platform host
   directions and record selected direct ICE candidates across two external networks.
   In the current native build, test a full in-game roster, localized rejection, explicit Leave, and reboot
   with visible NPCs. The eight-peer Windows loopback test covered admission, and the live gameplay test
   rendered up to two NPCs per game. Use separate scratch settings and saves for each instance.
4. Record the results in [TESTING.md](TESTING.md), [MILESTONES.md](MILESTONES.md), and [state.json](state.json).
   Record the upstream release and rerun the relevant checks whenever the owner brings in an update.
