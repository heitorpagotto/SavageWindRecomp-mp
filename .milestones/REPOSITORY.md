# Repository and upstream context

## Snapshot

- M1 working branch: `multiplayer/windows-test`.
- Base ref: `upstream/test`
- Base commit: `f488af7` (`No usable GPU: the error names each graphics card and what its driver lacks`)
- Fork remote: `origin` -> `https://github.com/heitorpagotto/SavageWindRecomp-mp.git`
- The bootstrap commit is `2b9a27c`. The lobby work started from `11c1af1` on `origin/multiplayer/bootstrap`.
- The M1 synthetic lobby is committed at `11ed467`. Inspect the current branch and preserve any local edits.

To refresh the upstream development branch without adding a remote:

```sh
git fetch https://github.com/Revivedx/SavageWindRecomp.git test:refs/remotes/upstream/test
```

Inspect and rebase carefully; preserve the user's changes. Never reset or overwrite the worktree to update it.

## Savage Wind architecture relevant to multiplayer

- `tools/`: NSO loading, layout, lifting, code generation, version-aware game sites, and instruction patches.
- `runtime/`: host OS replacement, HLE, platform code, settings, save handling, input, and UI.
- `runtime/nvn/`: graphics and Vulkan backend. Multiplayer currently needs no Vulkan change.
- `runtime/ui/game_options.mm`: Savage Wind's pages inside the game's own Options screen. It already supports button
  rows and page actions, so it is the likely Host/Join UI integration point after the module boundary exists.
- `tools/game_sites.py`: version-aware discovery of guest addresses. New actor, damage, flag, chest, and event hooks
  belong here.
- `tools/patches.py`: generated hooks. Every replaced instruction must be asserted.
- `runtime/game_sites.h`: names runtime-visible guest addresses. Runtime code must use `g_site`, not hard-coded
  addresses.
- `runtime/gamelib.c` and `runtime/gamelib.h`: load the locally generated game library and its site table.
- `runtime/hle.h`: `RT_ON_RESET` registration. Every per-boot multiplayer guest pointer, actor handle, or guest-side
  cache must be discarded through reset handling.
- `runtime/main.c`: boot/reboot lifecycle. The app can tear down the guest and boot again inside one process.
- `runtime/services/fs.c`: reads `SAVAGEWIND_ROMFS_OVERLAY`; this is how current SWMM content mods reach the game.
- `runtime/settings.c` and `runtime/settings.h`: persisted options. Player-facing changes require translation updates.
- `runtime/ui/i18n/`: translation tables. Run `python3 tools/i18n_check.py` after adding UI text.
- `Makefile`: runtime sources are mostly discovered by wildcard. The multiplayer core is intentionally outside
  `runtime/` and currently only has dedicated test/demo targets.

## Required implementation conventions

- Follow nearby indentation, naming, and error handling. C style reference: `settings_load` in `runtime/settings.c`.
- Add strict optional site groups: if every signature required for a feature is not found, leave that feature off.
- Do not retain guest pointers across reboot.
- Keep network I/O off the game thread, but only read/write guest state on the game thread.
- Multiplayer simulation uses its own fixed tick. Do not use presented-frame or generated-frame time by accident.
- Test 30/60/90/120 FPS when gameplay integration begins.
- Preserve guest FP/SIMD semantics and calling conventions.

## Current multiplayer files

- `mods/multiplayer/protocol.[ch]`: packet and player-snapshot codecs.
- `mods/multiplayer/session.[ch]`: host/client and peer bookkeeping.
- `mods/multiplayer/transport.h`: connection-scoped event and send interface.
- `mods/multiplayer/lobby_protocol.h`, `lobby.[ch]`: lobby payloads and control dispatch.
- `mods/multiplayer/mock_transport.[ch]`: bounded in-process transport with state faults.
- `mods/multiplayer/demo.c`: eight-player mock lobby and snapshot demonstration.
- `mods/multiplayer/README.md`: architecture and status visible beside the code.
- `mods/multiplayer/SWMM.md`: current manager integration notes.
- `mods/multiplayer/VERSION`: `0.1.0-dev`.
- `mods/multiplayer/swmm/`: raw SWMM development archive source.
- `tests/test_multiplayer.c`: synthetic protocol/session tests.
- `tests/test_multiplayer_webrtc.c`: opt-in encrypted loopback probe using externally built libdatachannel.
- `mods/multiplayer/native_abi_draft.h`, `tests/multiplayer_native_fixture.c`, and
  `tests/test_multiplayer_native.c`: draft ABI and standalone fixture.
- `tools/package_multiplayer.py`: deterministic package builder/checker.
- `Makefile`: `test-multiplayer`, `multiplayer-demo`, `multiplayer-swmm-package`, and
  `test-multiplayer-swmm`, plus standalone `test-multiplayer-native` and `test-multiplayer-webrtc` targets.

## Bootstrap build environment (historical)

- GCC: 16.2.1.
- Clang: absent in this environment, although Clang remains the project's normal compiler.
- ASan/UBSan runtime libraries were absent, so sanitizer linking could not run.
- GCC `-fanalyzer` was used successfully on the protocol, session, and demo sources.
- No generated game sources or extracted game files were needed for current tests.

