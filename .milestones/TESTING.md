# Test record and verification plan

Last updated: 2026-11-18. This distinguishes checks actually run from future or manual verification.

## Checks actually run

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

## Checks unavailable or deliberately not claimed

- `clang` was not installed. The project normally uses Clang.
- ASan/UBSan executables could not link because `libasan.so.8.0.0` and `libubsan.so.1.0.0` were missing. This is an
  environment gap, not a passed sanitizer run.
- No full Savage Wind build was claimed because the multiplayer files are not linked into the runtime.
- No extracted-game regeneration (`make gen`) was needed or performed.
- No gameplay, remote Link rendering, network/NAT traversal, or campaign behavior exists yet.
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

## Gameplay test matrix for later milestones

- 1, 2, 4, and 8 peers.
- 30, 60, 90, and 120 FPS where timing behavior is relevant.
- Windows and Linux/Vulkan with GPU/driver/settings recorded.
- Same area, different area, loading screen, warp, interior, cutscene, death, reconnect, and clean restart.
- Cooperative enemy damage, simultaneous lethal hits, PvP, chest contention, inventory full, quest transaction replay,
  and late join after progress.

Keep synthetic, build, network, and verified-gameplay results in separate sections of future reports.
