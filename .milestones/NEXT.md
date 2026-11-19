# Resume guide and next work

M1 has a synthetic lobby, an encrypted WebRTC loopback probe, and a draft native lifecycle fixture.
Production signing/approval, integration, and internet transport remain pending.

## Start here

Read [HANDOFF.md](HANDOFF.md) for the resume checkpoint and work order. Check the worktree from the repository root:

```sh
rtk git status --short --branch
rtk git diff -- Makefile mods/multiplayer tests/test_multiplayer.c tools/package_multiplayer.py .milestones
make CC=gcc test-multiplayer
make CC=gcc multiplayer-demo
make test-multiplayer-swmm
make test-multiplayer-native
```

Then read `CONTRIBUTING.md`, this directory in `README.md` order, and the nearest source around any intended hook.
Preserve unrelated work and do not edit generated sources.

## Continue M1 after synthetic verification

Use the control codecs, lobby, deterministic mock tests, and WebRTC probe as the baseline. Build and run the
probe on Linux using [TESTING.md](TESTING.md). The production adapter
needs to supply authenticated per-connection bindings, reliable ordered control and event channels, and unreliable
state delivery through `transport.h`. Keep the mock for repeatable failure tests.

Before dispatching gameplay state through the lobby, define resync and a membership generation or equivalent
identity so an in-flight state packet from a departed peer cannot be applied after its peer slot is reused.
Bump the protocol version for incompatible changes once artifacts may be used outside the repository.

## Native module ABI design spike

The [draft ABI](NATIVE_ABI.md) and standalone fixture cover the basic lifecycle. Production integration needs
maintainer agreement because it changes Savage Wind's executable-code boundary.

- Review the small C ABI and agree on the UI action/status extension, host build identity and capabilities.
- The host owns guest/runtime API tables; modules must not retain guest pointers across reset.
- Load only explicit approved absolute paths. Never scan the RomFS overlay for libraries.
- Validate manifest ID, ABI, platform, architecture, hash/signature policy, and duplicates before loading.
- Reuse the project-owned fixture and prove the lifecycle in the actual game host once agreement is in place.
- Keep Windows/Linux loaders behind one platform-neutral interface.

The SWMM handoff likely needs implementation in the separate SWMM repository. Do not copy its GPL source into
this MIT repository. Use the [packaging contract](SWMM.md#release-packaging-contract) to agree on manifest,
platform targeting, and what the player imports.

## Transport selection spike

libdatachannel is the leading candidate based on the [Windows loopback spike](DECISIONS.md#d012--transport-backend-not-yet-chosen).
Before selection, verify:

- C-compatible API, buildable on all supported Savage Wind platforms (currently Windows and Linux), with cross-platform interoperability;
- ICE/STUN/TURN and encrypted data channels;
- reliable ordered and unreliable/unordered delivery modes;
- binary size, linking, license, update cadence, and security history;
- cancellation/background-thread behavior and deterministic mocking; and
- practical rendezvous deployment, authentication, and abuse/rate limits.

Record the final choice in `DECISIONS.md`. Prototype expiring invite records separately from the local probe.
Scope signaling authorization and the link binding to a specific offer/answer exchange. Verify direct and forced
relay paths with the network matrix before making a production claim.

## UI integration after lifecycle exists

Use existing patterns around `runtime/ui/game_options.mm` and the translation tables. Minimum states are Host, Join,
Leave, creating/connecting, invite-code display/entry, roster, compatibility rejection, timeout, and cancellation.
Do not add art assets. Check player-facing text with `python3 tools/i18n_check.py`.

For the [M1 SWMM test build](TESTING.md#m1-swmm-in-game-presence-test), find a safe spawn/despawn path for an
existing idle or static actor in one supported 1.9.0 area. Reconcile placeholders from the admitted roster on
the game thread and clear their handles on leave and `RT_ON_RESET`. Keep the actor independent of local player
state. Use version-aware optional sites and the player's installed assets.

## Runtime integration rules

- Target 1.9.0 first; use version-aware patterns in `tools/game_sites.py`.
- Assert replaced instructions in `tools/patches.py` and use `g_site` for runtime addresses.
- Reset per-boot state with `RT_ON_RESET`; never retain guest pointers across reboots.
- Only the game thread touches guest state. Workers exchange bounded copied messages through queues.
- Use a fixed multiplayer tick; distinguish game updates, presented frames, and generated frames.
- Remote Links are proxies, never additional local-player singletons.
- Stable actor/transaction IDs go on the wire; pointers and transient addresses never do.

## Story and persistence prerequisites

Before M4, write and review the shared-story allowlist, transaction/deduplication rules, resync behavior, chest reward
policy for absent/late/disconnected/inventory-full peers, and save backup/rollback behavior. Never synchronize the whole
flag store or let an unvalidated client event write a persistent flag or grant inventory.

## Stop conditions requiring direction

Pause before:

- choosing a native-code consent model without Savage Wind and SWMM maintainer agreement;
- adding a dependency with unclear license or redistribution terms;
- risking Nintendo data, keys, saves, settings, or extracted files;
- changing core gameplay rules beyond multiplayer synchronization; or
- publishing, opening/closing/merging PRs, or releasing without explicit authorization.
