# Test record and verification plan

Last updated: 2026-11-19. This distinguishes checks actually run from future or manual verification.

## M1 synthetic checks on Windows

Compiled the standalone test and demo sources directly with Zig 0.15.2's Clang driver using
`-std=c11 -O1 -g -Wall -Wextra -Werror -Imods/multiplayer`. The executables and compiler cache went into scratch
space. GNU Make was unavailable in this workspace.

- The test executable reported `multiplayer core: ok`. It covers framing and snapshots, control payload sizes and
  UTF-8, lobby compatibility and capacity, sender-to-link binding, replay and timeout handling, cancellation,
  disconnect and late join, and mock queue limits and faults.
- The same test passed with Clang undefined-behavior checks enabled in trap mode.
- The demo admitted eight Links over the mock transport. The snapshot check reported 560 deliveries and one
  rejected duplicate.
- Ran `python tools/package_multiplayer.py <scratch ZIP>` and then `python tools/package_multiplayer.py --check <scratch ZIP>`.
  Both passed. The Windows archive SHA-256 was `ec79a0835bb42da566f160b8239d7be07ad8d49230a9252143f8649aa5cc7730`.

These checks used synthetic build IDs for the BOTW 1.9.0 target. No game runtime, Linux lobby run, network path,
or SWMM GUI lifecycle was exercised in M1.

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

## Gameplay test matrix for later milestones

- 1, 2, 4, and 8 peers.
- 30, 60, 90, and 120 FPS where timing behavior is relevant.
- Windows and Linux/Vulkan with GPU/driver/settings recorded.
- Same area, different area, loading screen, warp, interior, cutscene, death, reconnect, and clean restart.
- Cooperative enemy damage, simultaneous lethal hits, PvP, chest contention, inventory full, quest transaction replay,
  and late join after progress.

Keep synthetic, build, network, and verified-gameplay results in separate sections of future reports.
