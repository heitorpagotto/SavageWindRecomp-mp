# Test record and verification plan

Reviewed 2026-11-22. The prototype and native build results below have separate verification scopes.

## M1 developer build

Use MSYS2 UCRT64 on Windows or the project Linux/Vulkan environment. Install Git, CMake, Ninja, and OpenSSL
development files. The dependency script builds libdatachannel v0.24.6 with libjuice. Custom flags use
MP_RTC_CFLAGS and MP_RTC_LIBS.

~~~sh
CC=clang CXX=clang++ sh tools/build_multiplayer_deps.sh
export MP_RTC_PREFIX="$PWD/build/multiplayer-deps"
export PATH="$MP_RTC_PREFIX/bin:$PATH"
export LD_LIBRARY_PATH="$MP_RTC_PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
make B=build/m1 -j8
make B=build/m1 test-multiplayer test-multiplayer-webrtc test-multiplayer-direct
python3 tools/i18n_check.py
~~~

The historical HTTPS fixture uses libcurl 7.85 or newer and runs via make test-multiplayer-rendezvous.
The game executable does not use that fixture.

This recipe covers source builds. Release package dependency staging and license notices are tracked under
[M6](MILESTONES.md#m6--release-hardening).

### Direct invite and reply

The Windows standalone test exchanged the actual invite and reply messages for seven clients in one lobby.
It rejected malformed invites, old replies after a new invite, and replay of a consumed reply. It verified
invite timeout, a full roster, START, and cleanup when the host left. The Windows application and both menus
compiled. Two Windows processes also exercised the production runtime's Host, Join, Copy, Paste reply, and disable
actions with an isolated clipboard fixture. The direct transport objects cross-compiled for Linux x86-64.
Native menu interaction, gameplay across two external networks, and Linux/Vulkan execution remain pending. The older
gameplay evidence below used HTTPS signaling.

The staged Windows build used OpenSSL 4.1.0. See the [security findings](DIRECT_MULTIPLAYER_HANDOFF.md#security-findings)
for the applicable DTLS advisory, the configured DTLS minimum, and the checks still open.

### Direct lobby field reports

- Windows host to Linux client returned the incompatible-build error. The inspected Windows executable
  embedded the fingerprint of the current multiplayer sources combined with the previous Git tree.
  Rebuilding the same sources from commit `007973d` computed a different fingerprint with the old formula.
  The fingerprint now uses source contents with normalized line endings. Make produced identical IDs for
  Windows and for a Linux target with Git metadata hidden. Rebuild both platforms from the updated
  multiplayer sources for the next test.
- The owner reported a Linux-to-Linux join that succeeded with the friend hosting and stalled with the owner
  hosting. In the stalled direction the old host UI could show **Connected** as soon as it accepted the
  pasted answer. Both menus now show progress until a peer is admitted. The client switches from the reply
  prompt to connecting at the lobby handshake. The cause of the failed connection remains unverified.
- The updated Windows executable linked. The core, WebRTC loopback, and eight-peer direct tests passed.
  The direct test checked that the accepted reply's connection was assigned to the expected peer.
  The UI text check found zero missing or unused entries. Native Linux and external-network retests
  of these changes are pending.
- For the next run, use a fresh invite in each host direction. Confirm both logs show the same multiplayer
  source ID and BOTW build ID, then check whether both rosters contain both players. For a failure, collect
  the `[multiplayer]` log lines from both machines, including whether the host applied the reply and whether
  the client sent the lobby handshake. Keep the clipboard codes private.

The owner confirmed the updated Windows and Linux builds both joined a friend hosting on Linux.
The reverse host direction still failed. The owner accepted M1 and approved starting M2 while
investigating the firewall or direct connection separately.

At the M2 checkpoint `7991b79`, the owner confirmed a Windows client joined the Linux host after
launching the matching Windows build. The original Windows executable had a stale multiplayer
source ID. The [session handoff](HANDOFF.md#windows-and-linux-lobby-compatibility) records the IDs,
the verified launcher, and the outstanding host-direction report.

### Native build and standalone checks

- MSYS2 UCRT64 with Clang 23.1.5 built the Windows executable and Vulkan runtime objects against libdatachannel
  0.24.6 and libcurl 8.23.0. The regenerated BOTW 1.9.0 library matched ID `6bb7f77965924e25` at x86-64-v3.
- The multiplayer runtime, settings, and both UI adapters cross-compiled to Linux x86-64 and aarch64 objects.
  Native Linux/Vulkan execution and mixed-platform tests are pending.
- The Windows synthetic core and demo passed, with 560 snapshot deliveries and one stale duplicate rejected.
  Eight Windows WebRTC peers passed the loopback lobby and HTTPS rendezvous tests, including untrusted TLS rejection.
- The game-site and layout suites passed 50 and 59 checks. The translation check found zero missing or unused entries.

### Native Windows gameplay with the HTTPS prototype

- With separate scratch settings and saves, a fresh game booted with multiplayer off. The F1 tab appeared with
  its toggle off and showed Host and Join after enabling it. System > Options opened the Multiplayer page,
  enabled multiplayer, hosted and displayed a 24-character invite. On that same run F1 reflected the enabled
  preference. The INI recorded both on and off changes.
- On the Plateau, the Windows renderer showed one `Npc_TripMaster_00` after the first peer joined the
  certificate-verified loopback lobby, then two after a second peer joined. A deliberately incompatible source
  identity was rejected and did not acquire a slot. The renderer used a 30 FPS target, 960×540, 50% render scale,
  a 1400 MiB VRAM budget, and the NVIDIA RTX 4050 Laptop GPU with driver 616.92.
- The first headless peer disconnected and its actor was retired. A late-joining Windows game reused its
  slot and rendered the host and remaining peer from its initial roster. The host rendered two actors again.
  The late client used the same frame, resolution, and memory settings on AMD Radeon driver 25.10.30.02.
  Its native Join action used the test keyboard response for the invite.
- Disabling multiplayer through the host's System > Options page disconnected the other peers and retired
  both actors on each renderer. Screenshots showed the Plateau clear on both games after resuming play.
- A separate run hosted from F1 and admitted one peer. An in-place game reboot ran the reset hooks, returned to
  the title sequence, and dropped the observing peer to an empty roster. A fresh process loaded the saved
  enabled setting and displayed the offline Host and Join controls.
- The scratch evidence is under the ignored `build/m1-smoke/` directory when available.

### Prototype transport and UI

- Windows checks used Zig 0.15.2, libdatachannel 0.24.6, and libcurl 8.23.0. Eight encrypted WebRTC loopback
  peers passed roster, rejoin, capacity and build rejection, START, state and event delivery, and disconnect.
- Eight Windows peers passed HTTPS offer/answer and DTLS/SCTP. An untrusted TLS certificate was rejected.
  A controlled local libjuice TURN test dropped direct UDP traffic and observed a selected relay candidate.
- Two live Windows instances hosted and joined from the F1 overlay. Host reboot cleared the client roster.
  A deliberately incompatible app build was rejected with a visible Brazilian Portuguese message.
- Linux x86-64 and aarch64 runtime, UI, and test sources cross-compiled. Native Linux/Vulkan gameplay,
  mixed-platform execution, and two external networks remain pending.

## M2 visibility follow-up

- Field report on `7991b79`: the Windows host and Linux client joined but saw neither proxy. In the reverse
  setup a Linux client initially saw the Linux host. A Windows client joined at a different location. After
  the Linux client used Slate fast travel, it saw neither. Locations, logs, and the lobby state after the
  warp were unavailable, so the platform-specific cause is open.
- The follow-up Windows app and configuration launcher linked with Clang 23.1.5. The core, eight-peer direct,
  eight-peer WebRTC and place tests passed. The changed runtime units cross-compiled for Linux x86-64.
  Both Make targets produced source ID `3344f886df6a2e0a8a90d4dedddc1cc95f37380e`. The Windows target
  matched the installed 1.9.0 gamelib ID `e640fd858dc0d033` at x86-64-v3.
- In two Windows 1.9.0 Vulkan instances connected through the direct lobby, the position-control probe kept
  both proxies active at about 30 m. It withdrew both at about 145 m and restored both on return without a
  rejoin. The return screenshot showed both Links. This run used the position control endpoint. Evidence is in ignored
  `build/m2-travel-live-2c_8l727/`.
- The final Windows animation probe reported the native Idle, Walk, Run, Dash and Jump states playing on the
  receiver. The Run and Dash input tests used full left-stick travel without and with the sprint button.
  The alias resource check also passed for the synthetic and installed actor tables and the five clip records.
  Logs and captures are in ignored `build/m2-travel-live-v0ojcsbv/`. Both runs targeted 30 FPS at 960×540,
  50 percent render scale and a 1400 MiB VRAM budget on NVIDIA RTX 4050 Laptop (616.92) and AMD Radeon
  Graphics (25.10.30.02), with Vulkan heap copy off.
- The final build also passed a native System > Load test with the same Windows configuration. One client
  loaded a scratch autosave while both were in the lobby. Both sides retired their original proxies and
  reported fresh native actors when the client returned to the overworld. Both retained the original lobby.
  Evidence is in ignored `build/m2-travel-live-v_njtp_n/`.
- On 2026-11-23, two Windows scratch clients connected over direct WebRTC with version
  `0.6.0-alpha.6-0.1.0-mp`, source ID `3344f886df6a2e0a8a90d4dedddc1cc95f37380e` and BOTW 1.9.0
  gamelib `e640fd858dc0d033`. The probe used isolated clipboard automation and copies of a save with
  Slate travel unlocked. The client used the native Slate map to travel to Great Plateau Tower, then
  back to the Shrine of Resurrection. The two-player lobby remained admitted through both loads.
  At about 92 m separation after returning, both logs reported actors repeatedly becoming inactive
  and respawning. After the position-control probe put the two Links about 4 m apart outdoors, each
  visibly rendered the other in the original lobby. Sustained visibility near 92 m remains open.
  Both ran at a 30 FPS target, 960×540, 50 percent render scale, 1400 MiB VRAM budget and heap copy off
  on NVIDIA RTX 4050 Laptop (616.92) and AMD Radeon Graphics (25.10.30.02). Logs and captures are in
  ignored `build/m2-slate-kcjjyehi/`. The native mixed-platform retest remains open.

## M2 release version check

- Windows and Linux Make targets print `0.6.0-alpha.6-0.1.0-mp` via `print-version` and report the same
  multiplayer source ID as the preceding visibility build. The macOS Make target prints `0.6.0-alpha.6`.
  The Windows ZIP dry run uses the combined label for the filename and manifest. The Linux launcher
  translation unit cross-compiled for x86-64, and the AppImage packaging script passed its shell syntax check.
- The Windows app linked and displayed the combined label in Configuration and in the startup log with
  BOTW 1.9.0 and x86-64-v3 gamelib `e640fd858dc0d033`. The Windows multiplayer core, eight-peer WebRTC,
  eight-peer direct and updater core tests passed. The translation check reported zero missing or unused strings.
  The Windows capture and logs are in ignored `build/m2-version-smoke-dotx5361/`. Native Linux execution and
  full package creation are pending.

## M1 Windows presence check

The test area is outside the Shrine of Resurrection, near the `plateau` test warp. The placeholders are static
`Obj_RockGray_A_M_01` rocks placed above the grass in front of the default camera. Each instance uses scratch settings
and a separate scratch save. The installed game files are inputs to generation and launch.

- The native Windows scratch probe completed the async request, took the suspended actor, retained its indexed
  identity, and queued native deletion. Screenshots showed the rock in the world and then gone while the local
  player remained active. The run used BOTW 1.9.0, 1280x720, a 60 FPS target and NVIDIA RTX 4050 Laptop GPU, driver 616.92.
- The rebuilt session core passed the same-name slot reuse regression. The observing client processed PEER_LEFT and
  PEER_JOINED in one poll. Both the observer and host reported a new generation, and the late client had the complete
  remote roster from WELCOME.
- The Windows core, demo,
  eight-peer WebRTC loopback, HTTPS rendezvous, and controlled TURN tests passed. The UI text check found no missing
  or unused entries. The presence source cross-compiled for Linux x86-64 and aarch64.
- The synthetic game-site suite passed 50 checks, including a missing or mismatched signature disabling the complete
  presence group and preservation of the replaced guest stack stores. The layout suite passed 59 checks.
- The Windows app and regenerated guest library matched ID `6bb7f77965924e25` at the x86-64-v3 target. Two game
  instances of the prototype each reported one actor, then each reported two after a third
  client was admitted. In the single-GPU attempt, the first client exited with `vkWaitSemaphores: -4 (device lost)`.
  The split-GPU retry used the NVIDIA RTX 4050 Laptop GPU (616.92) and AMD Radeon (25.10.30.02). The workspace restart
  interrupted that run after the late client reached three admitted peers and before it logged its placeholders.
- A follow-up used one live game instance and two headless Windows peers with the same WebRTC and HTTPS transport.
  The live game joined late and reported both earlier peers from WELCOME. A mismatched peer was rejected, the
  departing peer's actor disappeared, and a same-name replacement produced one actor in the reused slot.
  Screenshots confirmed one rock after disconnect and two after rejoin. Reboot with a live connection returned
  the game to the title and removed the client from the host's roster. The live renderer used 1280x720 at 50%
  render scale, a 60 FPS target, a 1400 MiB VRAM budget, and the NVIDIA adapter with Vulkan heap copy off.
- In a separate live run, selecting Leave lobby removed the host's placeholder while the local player continued
  in the world.
- A split-GPU retry at 960x540, 50% render scale and a 30 FPS target reached two ready actors in all three games.
  The host and first client screenshots showed both rocks, then one each after the late client disconnected.
  A final host screenshot showed no placeholders after the first client disconnected. The late client's
  screenshot was still black with the loading indicator even after the draw count rose and both actors logged ready.
  Evidence is in `swmm-three-renderers-fiyqz87u` under the Windows scratch root.
- A follow-up at the same settings waited until the Plateau was visible before taking each screenshot. The host
  and late client used the RTX 4050 Laptop GPU (616.92), and the first client used AMD Radeon (25.10.30.02).
  All three games ran BOTW 1.9.0 with gamelib `6bb7f77965924e25`, scratch saves, a 1400 MiB VRAM budget, and
  Vulkan heap copy off. Each `build/two-peers.png` under the `host`, `first`, and `late` directories in
  `swmm-three-renderers-fjiu6mal` shows two rocks, one per admitted remote peer. The late client's screenshot
  shows both earlier peers during the fade-in. The surviving worlds showed one rock after the late client
  disconnected, and the host showed zero after the first client disconnected. The runner exited successfully.
- The multiplayer core also passed Clang's undefined-behavior trap check.

### Prototype evidence on the Windows development machine

When available, the ignored `build/mp-work/m1-runtime-scratch` directory contains the earlier logs and
screenshots. The old runners target the prototype activation path. Use the native Settings toggle when testing
the current executable. Wait until the Plateau renders before counting actors. A readiness log can arrive
during the loading screen.

## M2 Plateau position check

- BOTW 1.9.0, Windows Vulkan, 30 FPS target, 960x540 at 50% render scale, 1400 MiB VRAM budget,
  and Vulkan heap copy off. The host used an RTX 4050 Laptop GPU on driver 616.92 and the client
  used AMD Radeon Graphics on driver 25.10.30.02. Each instance had a separate scratch save and settings.
- Two live instances exchanged direct WebRTC invites and replies using an isolated clipboard. Both
  rendered a remote Link in the bind pose. Walking and jumping moved the active proxy without
  respawning. Client leave removed both proxies. A fresh invite reused the slot and host reboot
  removed the client's proxy. Evidence is under the ignored `build/m2-live-nv1jx2av/` directory.
- The follow-up compared the local players' and proxies' native matrices before and after turns.
  Facing converged in both directions on the original actor indices. The client could open and
  close inventory while the host remained in play. At a 60 FPS target, leaving and returning to the
  Plateau test bounds withdrew and restored both proxies. The two-instance run reached about
  65 FPS at a 90 FPS target and 83 FPS at a 120 FPS target. Evidence is under
  `build/m2-facing-8wcx5jxf/`.
- The Windows app and regenerated library `a1c9d2072a418844` built at x86-64-v3. The 50 game-site
  checks, 59 layout checks, multiplayer core and eight-peer WebRTC/direct tests passed. Linux
  x86-64 runtime, presence, asset and RomFS objects cross-compiled.

## M2 Plateau animation check

- Regenerated the 1.9.0 x86-64-v3 library `34aa482dae306a0d` and built the Windows runtime.
  The 52 game-site checks, 59 layout checks and Linux x86-64 cross-compilation of the proxy
  sources passed. The eight-peer direct test relayed action IDs. The synthetic lobby test dropped
  one update and reordered two more, then checked that the newest action reached the observer.
- Two Windows Vulkan games at a 30 FPS target, 960x540 at 50% render scale, and a 1400 MiB VRAM
  budget played idle, walk and jump on both receivers using direct WebRTC. Each proxy kept its
  active actor through the action changes. Inventory was independent. Changing to a 60 FPS target
  and crossing the test boundary removed and restored the proxies. Leave, late same-slot rejoin
  after START, and in-place host reboot cleanup passed. The GPUs were the RTX 4050 Laptop GPU
  (616.92) and AMD Radeon Graphics (25.10.30.02). The 90 and 120 FPS target runs exercised action
  changes below the requested rates. Evidence is under `build/m2-anim-live-m1saptwh/`.
- A single Windows renderer showed all seven remote Links for 30 seconds after START. The seven
  senders were headless WebRTC fixtures. The RTX 4050 renderer kept the same seven proxies
  through 60, 90 and 120 FPS targets, measuring about 53, 71 and 68 FPS during those checks.
  All proxies retired when the fixtures disconnected. Evidence is under
  `build/m2-anim-capacity-uzyz5rty/`.

## M2 Plateau health check

- The Windows app and regenerated BOTW 1.9.0 x86-64-v3 gamelib `e640fd858dc0d033` built.
  The multiplayer core, eight-peer WebRTC and snapshot relay tests passed. The affected sources
  cross-compiled for Linux x86-64. The UI text check found zero missing and zero unused entries.
- Two 30 FPS Windows scratch games exchanged fresh health samples over direct WebRTC. A controlled
  write to the client's native life in its scratch process changed the host's roster from 12 to
  9 quarter-hearts and then back to 12. The host's native life stayed at 12. The run also repeated
  the live walk and jump checks. Evidence is under `build/m2-health-live-rq7bfy54/`.
- An F1 window capture shows 3 hearts for the local Link and 2.25 for the headless WebRTC sender.
  The capture is `build/m2-health-menu-g9g2yn3l/host/build/frames/window_01780.png`.

## Synthetic checks

The core historically printed `multiplayer core: ok` under GCC 16.2.1 and passed Clang's undefined-behavior
trap check on Windows. The demo admitted eight peers, accepted 560 snapshot deliveries, and rejected one stale
duplicate. GCC `-fanalyzer` reported no diagnostics on the protocol, session, and demo sources. These checks
use synthetic inputs.

```sh
make test-multiplayer
make multiplayer-demo
rtk git diff --check
```

## Verification required for runtime changes

Follow `AGENTS.md` and `CONTRIBUTING.md`. In particular:

- Use `make runtime-objs` for backend/UI compilation without game linking.
- For game sites or patches, run `tools/venv/bin/python tools/test_game_sites.py`, regenerate from the user's own
  files, rebuild, and check the loaded gamelib ID.
- Run `python3 tools/i18n_check.py` for player-facing text.
- Use `tools/linux/test_run.sh` with scratch settings/saves for Linux gameplay tests.
- Never commit a save, extracted file, generated game code, key, firmware, game asset/font, or disassembly output.

## Network test matrix for M1+

Record the exact build, game version, OS, transport version, and selected ICE candidate types for each run.

| Case | Minimum expected result |
|---|---|
| Same LAN | Direct encrypted connection; correct eight-peer roster |
| Different home networks | ICE/STUN direct path where available |
| Symmetric/restrictive NAT | Direct route or a connection failure that returns to the lobby controls |
| Packet loss/jitter/reordering | State remains bounded; reliable transactions converge |
| Duplicate/replayed packet | Rejected without duplicate world mutation |
| Incompatible version | Join rejected before guest/game mutation |
| Host exits | All clients leave cleanly; no host migration in v1 |
| Late join | Full authoritative snapshot before normal play |
| Runtime reboot | Threads, queues, sockets, and guest references reset |

## Native M1 acceptance

1. Build on Windows and Linux/Vulkan with the same supported 1.9.0 game target and compatible multiplayer
   source. Verify the matching local gamelib ID. Use separate scratch settings and saves for each instance.
2. Open F1 Settings and System > Options in the game. Confirm both expose Multiplayer, reflect the same
   enable state, and control the same lobby. Exercise Host, Join, and Leave through both surfaces.
3. On clean installs, host a lobby and copy its invite to a private chat. Paste the invite on the joining
   machine and send the reply to the host. Paste the reply on the host and confirm admission. Repeat in both
   menus without a signaling deployment. Reissue an invite and verify that its previous reply is rejected.
4. Exercise malformed clipboard text, cancellation, timeout, and rapid disable and re-enable. Confirm a new
   invite leaves admitted players connected and that disabling clears the local session.
5. Use two external networks and each OS in turn as host. Verify the selected direct candidates, an eight-peer
   roster, and localized incompatible-build rejection.
6. Count the NPC placeholders on the Plateau after the world is rendered. Each instance must show
   one per admitted remote peer. Test late join, rejection, slot reuse, disconnect, Leave, disable during a
   session, and reboot. Check that the affected placeholders disappear and scratch saves remain usable.
7. Boot and play in single-player with multiplayer disabled. Confirm the setting is shared across the two
   surfaces, persists across a normal restart, and can enable hosting again.

Record the build, game version, platform, GPU/driver, FPS, settings, and observed results.

## Gameplay test matrix for later milestones

- 1, 2, 4, and 8 peers.
- 30, 60, 90, and 120 FPS where timing behavior is relevant.
- Windows and Linux/Vulkan with GPU/driver/settings recorded.
- Same area, different area, loading screen, warp, interior, cutscene, death, reconnect, and clean restart.
- Cooperative enemy damage, simultaneous lethal hits, PvP, chest contention, inventory full, quest transaction replay,
  and late join after progress.

Record which results came from synthetic tests, builds, network tests, or observed gameplay.
