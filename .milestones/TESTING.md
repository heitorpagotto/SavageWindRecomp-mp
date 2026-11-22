# Test record and verification plan

Reviewed 2026-11-22. The prototype and native build results below have separate verification scopes.

## M1 developer build

Use MSYS2 UCRT64 on Windows or the project Linux/Vulkan environment. Install Git, CMake, Ninja, OpenSSL development
files, and libcurl 7.85 or newer with TLS and asynchronous DNS. The dependency script builds libdatachannel v0.24.6
with libjuice. A system libdatachannel can also be used. Custom flags use `MP_RTC_CFLAGS`, `MP_RTC_LIBS`,
`MP_CURL_CFLAGS`, and `MP_CURL_LIBS`.

```sh
CC=clang CXX=clang++ sh tools/build_multiplayer_deps.sh
export MP_RTC_PREFIX="$PWD/build/multiplayer-deps"
export PATH="$MP_RTC_PREFIX/bin:$PATH"   # Windows DLL lookup
export LD_LIBRARY_PATH="$MP_RTC_PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"   # Linux shared library lookup
make B=build/m1 -j8
make B=build/m1 test-multiplayer test-multiplayer-webrtc test-multiplayer-rendezvous
python3 tools/i18n_check.py
```

For local tests, run `tools/multiplayer_rendezvous.py` with a trusted HTTPS reverse proxy or its certificate
and key options. Set `SAVAGEWIND_MP_RENDEZVOUS` on both game instances and `SAVAGEWIND_MP_CA_BUNDLE` for a
local CA. Use `SAVAGEWIND_MP_STUN` and `SAVAGEWIND_MP_TURN` to point to the test ICE services.
`SWMP_TEST_TURN` configures the standalone relay test. `SAVAGEWIND_MP_RELAY_ONLY=1` advertises relay
candidates. Verify the selected route because loopback can still select a peer-reflexive path.

This recipe covers source builds. Release package dependency staging and license notices are tracked under
[M6](MILESTONES.md#m6--release-hardening).

### Native build and standalone checks

- MSYS2 UCRT64 with Clang 23.1.5 built the Windows executable and Vulkan runtime objects against libdatachannel
  0.24.6 and libcurl 8.23.0. The regenerated BOTW 1.9.0 library matched ID `6bb7f77965924e25` at x86-64-v3.
- The multiplayer runtime, settings, and both UI adapters cross-compiled to Linux x86-64 and aarch64 objects.
  Native Linux/Vulkan execution and mixed-platform tests are pending.
- The Windows synthetic core and demo passed, with 560 snapshot deliveries and one stale duplicate rejected.
  Eight Windows WebRTC peers passed the loopback lobby and HTTPS rendezvous tests, including untrusted TLS rejection.
- The game-site and layout suites passed 50 and 59 checks. The translation check found zero missing or unused entries.

### Native Windows gameplay

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

Record the exact build, game version, OS, transport version, and signaling/relay deployment for each run.

| Case | Minimum expected result |
|---|---|
| Same LAN | Direct encrypted connection; correct eight-peer roster |
| Different home networks | ICE/STUN direct path where available |
| Forced relay | TURN path connects and reports relay use |
| Symmetric/restrictive NAT | Relay fallback or actionable error |
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
3. Invite clients on the other OS, with each OS taking a turn as host. Verify an eight-peer roster and localized
   incompatible-build rejection. Use two external networks for direct ICE and a controlled forced TURN run,
   and record the selected candidates.
4. Count the NPC placeholders on the Plateau after the world is rendered. Each instance must show
   one per admitted remote peer. Test late join, rejection, slot reuse, disconnect, Leave, disable during a
   session, and reboot. Check that the affected placeholders disappear and scratch saves remain usable.
5. Boot and play in single-player with multiplayer disabled. Confirm the setting is shared across the two
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
