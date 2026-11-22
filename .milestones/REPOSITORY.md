# Repository and upstream context

## Snapshot

- Working branch `multiplayer/windows-test`, source checkpoint `9fb424b`.
- Historical upstream baseline `f488af7`. The bootstrap is `2b9a27c` and the synthetic lobby is `11ed467`.
- Fork remote is `origin`. Identify the current upstream release and inspect the branch before updating it.

## Manual release updates

The project owner selects and brings in new Savage Wind releases. Record the chosen upstream tag or commit.
Preserve local edits, game data, settings, and saves. Review conflicts around the Makefile, settings, UI,
runtime lifecycle, and guest sites, then rebuild and verify both platforms. Follow upstream's `test` branch
conventions when preparing an upstream contribution. [Contribution guide](../CONTRIBUTING.md)

## Savage Wind architecture relevant to multiplayer

- `tools/`: NSO loading, layout, lifting, code generation, version-aware game sites, and instruction patches.
- `runtime/`: host OS replacement, HLE, platform code, settings, save handling, input, and UI.
- `runtime/nvn/`: graphics and Vulkan backend. Multiplayer currently needs no Vulkan change.
- `runtime/ui/game_options.mm`: Savage Wind's pages inside the game's own Options screen. It already supports button
  rows and page actions for the Multiplayer page. Its hooks run on the guest thread.
- `tools/game_sites.py`: version-aware discovery of guest addresses. New actor, damage, flag, chest, and event hooks
  belong here.
- `tools/patches.py`: generated hooks. Every replaced instruction must be asserted.
- `runtime/game_sites.h`: names runtime-visible guest addresses. Runtime code must use `g_site`, not hard-coded
  addresses.
- `runtime/gamelib.c` and `runtime/gamelib.h`: load the locally generated game library and its site table.
- `runtime/hle.h`: `RT_ON_RESET` registration. Every per-boot multiplayer guest pointer, actor handle, or guest-side
  cache must be discarded through reset handling.
- `runtime/main.c`: boot/reboot lifecycle. The app can tear down the guest and boot again inside one process.
- `runtime/ui/menu.mm`: F1 Settings, Multiplayer tab registration, and software keyboard.
- `runtime/settings.c` and `runtime/settings.h`: persisted options. Player-facing changes require translation updates.
- `runtime/ui/i18n/`: translation tables. Run `python3 tools/i18n_check.py` after adding UI text.
- `runtime/multiplayer.c` and `runtime/ui/multiplayer.mm`: M1 window-thread ownership and the
  localized multiplayer tab. The window thread publishes the admitted roster for the guest update.
- `runtime/multiplayer_presence.c`: the 1.9.0 guest menu update hook owns native creation, indexed deletion,
  and per-boot placeholder state.
- `Makefile` adds the native multiplayer objects to Windows and Linux builds. The standalone networking and
  session tests run independently of the game. `tools/build_multiplayer_deps.sh` builds the pinned RTC dependency.

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
- `mods/multiplayer/webrtc.[ch]` provides bounded WebRTC channels and link ownership via libdatachannel.
- `mods/multiplayer/rendezvous.[ch]` and `tools/multiplayer_rendezvous.py` implement the HTTPS offer/answer exchange.
- `tests/test_multiplayer.c`, `tests/test_multiplayer_webrtc.c`, and `tests/test_multiplayer_rendezvous.c`
  cover the synthetic core, encrypted lobby, and HTTPS signaling.
- The Makefile exposes `test-multiplayer`, `multiplayer-demo`, `test-multiplayer-webrtc`, and
  `test-multiplayer-rendezvous`.

## Bootstrap build environment (historical)

- GCC: 16.2.1.
- Clang: absent in this environment, although Clang remains the project's normal compiler.
- ASan/UBSan runtime libraries were absent, so sanitizer linking could not run.
- GCC `-fanalyzer` was used successfully on the protocol, session, and demo sources.
- No generated game sources or extracted game files were needed for current tests.
