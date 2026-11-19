# Test record and verification plan

Last updated: 2026-11-19. This distinguishes checks actually run from future or manual verification.

## Current checkpoint on Windows

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

Import, approve, enable, deploy, and launch the compiled M1 test build through SWMM. Use BOTW 1.9.0 and scratch
saves in the documented test area. In the game's lobby UI, host a session, join from a second instance, and
admit a third instance after the first join. Verify that each instance displays exactly one visible placeholder
per admitted remote peer, including peers already connected when it joined. Verify that an incompatible client
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
