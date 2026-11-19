# Test record and verification plan

Last updated: 2026-11-19. This distinguishes checks actually run from future or manual verification.

## M0.5 SWMM 0.1.3 portable CLI

Ran these targets with GNU Make 4.4.1, Zig 0.15.2, Python 3.12, and the official
[SWMM 0.1.3 Windows portable release](https://github.com/ustruct/SWMM/releases/tag/v0.1.3). The portable
release ZIP matched the publisher's SHA-256 list.

```sh
make test-multiplayer-swmm
make test-multiplayer-swmm-cli SWMM_EXE=/path/to/portable/swmm.exe
make multiplayer-native-fixture-package
make test-multiplayer-native
```

- The old bare `romfs/` marker archive failed CLI conversion with “No content or DLC folder found in source.”
  With the Switch title directory in the ZIP, stock SWMM installed, merged, deployed and uninstalled it.
  The smoke test used a synthetic language pack and compared the deployed marker after Yaz0 decoding.
  It also checked removal from the overlay and installed mod store, and preservation of the synthetic base files.
- Repeated content builds matched SHA-256
  `47b9dada32a2d2b22fc0fe943a3b4dc6ad86df5ed10a37a0eb3fa49811b003a1`.
- The fixture bundle contains Windows x86-64, Linux x86-64, and Linux aarch64 libraries. The packaged Windows
  DLL exercised the standalone native lifecycle. Both Linux libraries were cross-compiled and validated by
  header and hash, with execution pending. Repacking the same binaries produced identical bytes. A modified
  DLL and a mislabeled ELF were rejected. Bundle SHA-256 for this build was
  `a6f45f2e94da8ae1601b1b3f6c0f65ee9c06ed61ab3b989bb6a19fc43bf2f250`.

The CLI check does not exercise drag-and-drop, game launch, the GUI update/disable workflow, or native approval.
Production approval and loading are still pending in SWMM and Savage Wind.

## M1 developer build

From an MSYS2 UCRT64 or Linux/Vulkan build environment, install libdatachannel v0.24.6 with libjuice and
TLS-enabled libcurl 7.85 or newer with asynchronous DNS. Use a separate build directory.

```sh
make B=build/m1 MP_M1_TEST=1 -j8
make B=build/m1 test-multiplayer test-multiplayer-marker test-multiplayer-webrtc test-multiplayer-rendezvous
make B=build/m1 multiplayer-swmm-package
python3 tools/i18n_check.py
```

Use `MP_RTC_CFLAGS`, `MP_RTC_LIBS`, `MP_CURL_CFLAGS`, and `MP_CURL_LIBS` for custom dependency paths.
Make Windows DLL dependencies available on PATH. The rendezvous test uses Python 3.10+ and the OpenSSL
command-line tool for a disposable certificate. Set `SWMP_TEST_TURN` to a TURN URI to exercise relay candidates.

For a developer session, run `python3 tools/multiplayer_rendezvous.py` on the same machine as a trusted HTTPS
reverse proxy and point both games to its HTTPS origin with `SAVAGEWIND_MP_RENDEZVOUS`. For local TLS, pass
`--certificate` and `--key` and set `SAVAGEWIND_MP_CA_BUNDLE` to the trusted CA. Use
`SAVAGEWIND_MP_STUN=stun:stun.example:3478` with your direct ICE test service. For TURN, set
`SAVAGEWIND_MP_TURN=turn:user:password@turn.example:3478`. `SAVAGEWIND_MP_RELAY_ONLY=1` advertises relay
candidates. Use a network where peer-reflexive direct ICE is unavailable to force the selected route through TURN.
Keep deployment access bounded at the reverse proxy.

In stock SWMM, import and enable the development ZIP and deploy to a scratch overlay. Set the custom
executable in SWMM. Open Multiplayer in the F1 or controller overlay. Host lobby displays an invite, and
Join lobby consumes it. Use separate data roots, settings and saves for each instance. Check the loaded game
library's ID and use 1.9.0. The [M1 presence test](#m1-swmm-in-game-presence-test) tracks actor verification.

### Current M1 evidence

- Windows, Zig 0.15.2, libdatachannel 0.24.6, libcurl 8.23.0. Eight encrypted loopback endpoints passed
  roster, rejoin, capacity and build rejection, START, channel payload, receive overflow, and disconnect.
- Eight Windows peers passed local HTTPS offer/answer and DTLS/SCTP. An untrusted certificate failed before
  invitation. A controlled check passed with a local libjuice TURN process and a test-only UDP filter that
  dropped direct traffic. Each link reported a relay candidate at one endpoint. Unfiltered loopback can
  select peer-reflexive direct paths.
- Raw and Yaz0 marker checks, invalid marker rejection, and 13 translation tables passed.
- The Windows M1 app linked with the portable Zig toolchain and an ignored PE export shim. The supplied
  BOTW 1.9.0 installation regenerated and booted with the matching game library ID. Windows runtime ran
  on an RTX 4050 Laptop GPU, driver 32.0.16.1692, 1280x720, a 60 FPS target, and scratch saves and settings.
- Stock SWMM 0.1.3 portable CLI deployed and removed the marker in an isolated profile. An actual CLI-deployed
  marker gated the app's Multiplayer tab. Two Windows game instances on the same machine used Host and Join
  in the overlay against a local HTTPS service and both reported two admitted peers. Rebooting the host
  ran the reset hooks and the other instance returned to zero peers with a host disconnect.
- A second Windows client built with a deliberately different multiplayer compatibility ID was rejected
  by the live game host. The test captured the Brazilian Portuguese incompatibility message in the menu.
- With an empty overlay, the same Windows executable presented the normal title and Settings without the
  Multiplayer tab. A separate scratch run reached the Great Plateau and exercised a native quick drop.
  The native debugger captured the actor name and transient request at a lower creation helper.
  The [presence check](#m1-windows-presence-check) followed the request to its create and delete calls.
- Changed runtime, UI and network test sources cross-compiled for Linux x86-64 and aarch64. Native Linux/Vulkan,
  mixed-platform execution, and external networks remain open.

## M1 Windows SWMM GUI

Tested stock SWMM 0.1.3 portable with the development marker ZIP (SHA-256
`47b9dada32a2d2b22fc0fe943a3b4dc6ad86df5ed10a37a0eb3fa49811b003a1`) and BOTW 1.9.0. The test app loaded
the matching x86-64-v3 gamelib with ID `6bb7f77965924e25`. It ran at 1280x720, 50% render scale and a 60 FPS
target on an NVIDIA GeForce RTX 4050 Laptop GPU (616.92), with a 1400 MiB VRAM budget and Vulkan heap copy off.
The manager, overlay, executable, extracted data, settings, and saves were in a scratch profile.

1. Dropped the ZIP from Explorer onto SWMM. It imported as Savage Wind Multiplayer v0.1.0-dev and defaulted
   to enabled. Apply and Deploy put the expected marker in the overlay, verified after Yaz0 decoding.
2. SWMM Play booted the game and the in-game Settings showed Multiplayer. The overlay used the test build's
   marker gate and displayed the prompt to configure an HTTPS rendezvous server.
3. Disabled the mod in SWMM, applied and deployed. The deployed marker disappeared while the package stayed
   installed. SWMM Play loaded the scratch save on the Plateau with Multiplayer absent.
4. Uninstalled the mod in SWMM and applied from the Mod menu. The empty Home page hid Deploy, so the stock
   portable CLI finished the pending deployment. The installed package and marker were absent. The overlay
   retained SWMM's generated language pack and resource size table.
5. Booted the same executable against the post-uninstall overlay. Settings matched the pre-import single-player
   tab set, including the absence of Multiplayer. The NVIDIA run reached the Plateau, then logged a Vulkan device
   loss. The AMD Radeon retry (25.10.30.02, 1800 MiB VRAM budget, otherwise the same settings) visibly resumed the
   scratch save on the Plateau and exited successfully. SHA-256 comparisons of six scratch settings and save
   files matched across disable and across uninstall and deployment.

Local evidence is under `build/mp-work/m1-runtime-scratch/swmm-gui-ri7z_s6t`. See `06-zip-drop.png`,
`08-deployed.png`, `enabled-menu.png`, `disabled-menu.png`, `14-uninstalled.png`,
and `uninstalled-retry-world.png`. The Windows core, demo, raw/Yaz0 marker, ZIP
validator, SWMM CLI, native fixture, WebRTC loopback, and HTTPS loopback targets passed again from fresh test
binaries. Game sites passed 50 checks, layout passed 59, and the UI text check found no missing or unused entries.

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
- The Windows core, demo, package validator, raw/Yaz0 marker, standalone DLL lifecycle, SWMM portable CLI,
  eight-peer WebRTC loopback, HTTPS rendezvous, and controlled TURN tests passed. The UI text check found no missing
  or unused entries. The presence source cross-compiled for Linux x86-64 and aarch64.
- The synthetic game-site suite passed 50 checks, including a missing or mismatched signature disabling the complete
  presence group and preservation of the replaced guest stack stores. The layout suite passed 59 checks.
- The Windows app and regenerated guest library matched ID `6bb7f77965924e25` at the x86-64-v3 target. Two game
  instances using the stock SWMM CLI-deployed marker each reported one actor, then each reported two after a third
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
  in the world. The rebuilt app also reached Settings with the Multiplayer tab absent when started with an empty overlay.
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

### Replay on the Windows development machine

The ignored `build/mp-work` folder contains the matching test app and guest library under `m1-make`, the headless
peer fixture `m1-resume-peer.exe`, the installation pointer in `m1-local-state.json`, and the scratch inputs.
Check the loaded library's adjacent ID against the Makefile with the same build overrides. The Python runners
use `openssl` on PATH to create a loopback TLS certificate. Run each scenario from the repository root.

| Command | Evidence to check |
| --- | --- |
| `python build/mp-work/m1-resume-headless-presence.py` | Both peers ready after WELCOME, build rejection, one rock after disconnect, two after same-slot rejoin, client removed from the host roster on reboot. |
| `python build/mp-work/m1-resume-leave.py` | Leave lobby returns the game to zero peers and `after-leave.png` shows the Plateau without placeholders. |
| `powershell -NoProfile -File build/mp-work/m1-resume-marker-free.ps1` | The app boots using an empty overlay and the Settings capture omits Multiplayer. |
| `python build/mp-work/m1-swmm-three-renderers-visible.py` | Three rendered worlds using the CLI-deployed marker. Check two rocks in each `two-peers.png`, one in each surviving world after the late client disconnects, and none in the host after the final disconnect. |

Each runner writes under `build/mp-work/m1-runtime-scratch`. In `resume-headless-presence-slk518cv/late`, see
`presence.log`, `build/after-disconnect.png`, and `build/after-rejoin.png`. The first readiness log can appear
before the loading screen clears, so wait for the world to render before counting rocks. The Leave lobby run is
`resume-leave-presence-mfr63be2`. The marker-free run is `resume-marker-free`.

## Previous M1 checkpoint on Windows

Ran the test targets with GNU Make 4.4.1 and Zig 0.15.2's Clang driver. The toolchain, binaries, and caches
were kept in the repository build directory.

- `test-multiplayer` reported `multiplayer core: ok`. A direct build also passed with
  `-fsanitize=undefined -fsanitize-trap=undefined`.
- `multiplayer-demo` admitted eight Links over the mock, verified 560 snapshot deliveries, and rejected a stale
  duplicate.
- `test-multiplayer-swmm` validated the ZIP and passed three package tests covering line-ending parity, extra or
  duplicate entries, entry size, and marker identity. Two archive builds produced SHA-256
  `efbed5e1322bb4b8de344cd76d30cb073ff87dabfc134878b2100f6c968c2eff`.
- `test-multiplayer-native` passed absent-module startup and DLL load, ABI and capability checks, poll, reset,
  shutdown, and unload. The fixture and its host also cross-compiled for Linux x86-64 with
  `zig cc -target x86_64-linux-gnu.2.28`. Linux execution remains pending.
- `test-multiplayer-webrtc` passed with libdatachannel v0.24.6, bundled libjuice/usrsctp, and OpenSSL 3.6.5.
  The two endpoints selected loopback ICE candidates, completed DTLS/SCTP, joined and started the lobby, exchanged
  binary messages over state and event streams, rejected an incompatible build in a second session, and observed
  the client leaving when the host closed its control stream. Five repeated Windows runs also passed. The Linux
  x86-64 probe source compiled to an object without linking or executing.

All lobby checks used synthetic build IDs for the BOTW 1.9.0 target. The WebRTC test supplied SDP and the probe
binding in process. External-network signaling, STUN/TURN, Linux and mixed-platform lobby execution, the SWMM
GUI lifecycle, the game host, and gameplay remain unverified.

## Build the optional WebRTC probe

Use libdatachannel v0.24.6 with libjuice and OpenSSL. The build requires a C++ toolchain and the dependency's
checked-out submodules. A shared build can be produced with
`-DUSE_NICE=OFF -DNO_MEDIA=ON -DNO_WEBSOCKET=ON -DNO_TESTS=ON -DNO_EXAMPLES=ON`. Follow the upstream
[build instructions](https://github.com/paullouisageneau/libdatachannel/blob/v0.24.6/BUILDING.md). For an installed
shared build on the compiler and runtime search paths, run `make test-multiplayer-webrtc`. Use `MP_RTC_CFLAGS`
for a custom header prefix and `MP_RTC_LIBS` for a custom import/library path.

The Windows check used a static libdatachannel build with OpenSSL DLL imports. It passed
`-DRTC_STATIC -DRTC_ENABLE_MEDIA=0 -DRTC_ENABLE_WEBSOCKET=0` and the libdatachannel include path through
`MP_RTC_CFLAGS`. `MP_RTC_LIBS` supplied the Zig C++ runtime, libdatachannel, libjuice, usrsctp, OpenSSL
import libraries, and `-lws2_32 -liphlpapi -lbcrypt -lcrypt32`. The OpenSSL DLL directory was on PATH.

## Bootstrap checks (historical)

The following ran from the repository root with GCC 16.2.1 because `clang` was unavailable:

```sh
make CC=gcc test-multiplayer
```

Result: `multiplayer core: ok`. This covers protocol/header and snapshot round trips, malformed packet rejection,
session membership, capacity, sequence filtering (including wrap behavior), wrong-session/sender rejection, and client
reset when the host leaves.

```sh
make CC=gcc multiplayer-demo
```

Result:

```text
Lobby ready: 8/8 Links; host authority at peer 0
PASS: 8 Links, 560 snapshot deliveries, 1 stale duplicate rejected
```

This is an in-process simulation: eight sessions, ten ticks, every sender delivered to seven other peers. It does not
use a socket or game runtime.

```sh
make multiplayer-swmm-package
make test-multiplayer-swmm
make test-multiplayer-native
```

Package creation and archive validation passed. Artifact at the time of testing:

```text
build/mods/SavageWind-Multiplayer-0.1.0-dev-SWMM.zip
SHA-256 731f2548d8e9c691defe0437a310d621a02e09a1eab2a83a73b3e991475db867
```

Rebuilding produced the same digest. GCC's `-fanalyzer` was also run against `protocol.c`, `session.c`, and `demo.c`
with no diagnostics. `git diff --check` and a trailing-whitespace scan passed for the implementation state preceding
this handoff; run them again after every edit.

## Bootstrap verification gaps (historical)

- `clang` was not installed. The project normally uses Clang.
- ASan/UBSan executables could not link because `libasan.so.8.0.0` and `libubsan.so.1.0.0` were missing. This is an
  environment gap, not a passed sanitizer run.
- No full Savage Wind build was claimed because the multiplayer files are not linked into the runtime.
- No extracted-game regeneration (`make gen`) was needed or performed.
- Gameplay, remote Link rendering, network/NAT traversal, and campaign behavior were outside the bootstrap checks.
- The archive has not been exercised through SWMM's GUI lifecycle.
- Windows has not been tested.

## Reproduce current checks

```sh
make CC=gcc test-multiplayer
make CC=gcc multiplayer-demo
make multiplayer-swmm-package
make test-multiplayer-swmm
python3 -m json.tool .milestones/state.json >/dev/null
rg -n '[[:blank:]]+$' .milestones mods tests tools Makefile
git diff --check
```

If `clang` is available, omit `CC=gcc` and test the default build path too. Once sanitizer runtimes are installed,
add an instrumented compile/run rather than assuming GCC analysis is equivalent.

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

## SWMM release verification

Record OS/architecture, Savage Wind and SWMM versions, native ABI version, archive hash, and the observed result
for each supported platform. Windows and Linux/Vulkan are the current targets.

| Check | Evidence required |
|---|---|
| Build and package | Produce the native module and archive from the documented recipe, then validate the target, metadata, and contents. |
| Fresh install | Drag the archive into SWMM, approve, enable, deploy, and launch. Confirm the multiplayer module loads. |
| Update and removal | Update from a prior build, then disable and uninstall. Confirm the module is unavailable while disabled or removed and user data is preserved. |
| Invalid or denied module | Decline native-code approval and try the wrong platform or ABI. Confirm the module stays unloaded and Savage Wind can start. |
| Cross-platform lobby | Verify that the Windows and Linux packages interoperate on compatible builds. Test each OS as host. |

These are pending release checks. Historical package validation used the development marker archive.

## M1 SWMM in-game presence test

Import and enable the development marker ZIP in the SWMM GUI, deploy it, and launch the configured M1 test
executable. The [Windows GUI run](#m1-windows-swmm-gui) verified the package lifecycle. Use BOTW 1.9.0 and
scratch saves in the [Plateau test area](#m1-windows-presence-check). In the game's
lobby UI, host a session, join from a second instance, and admit a third instance. Verify that each instance shows
exactly one visible placeholder per admitted remote peer, including peers already connected when it joined.
Verify that an incompatible client
creates no placeholder. Disconnect one client and check that its placeholder disappears from the remaining
instances. Leave or reboot and check that the local session's placeholders are cleared and the local player
and scratch saves remain usable. Record the build, platform, area, actor used, and observed counts.

## Gameplay test matrix for later milestones

- 1, 2, 4, and 8 peers.
- 30, 60, 90, and 120 FPS where timing behavior is relevant.
- Windows and Linux/Vulkan with GPU/driver/settings recorded.
- Same area, different area, loading screen, warp, interior, cutscene, death, reconnect, and clean restart.
- Cooperative enemy damage, simultaneous lethal hits, PvP, chest contention, inventory full, quest transaction replay,
  and late join after progress.

Keep synthetic, build, network, and verified-gameplay results in separate sections of future reports.
