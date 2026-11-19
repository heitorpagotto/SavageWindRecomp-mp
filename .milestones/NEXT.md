# Resume guide and next work

This is the recommended executable order for a context-free developer or agent. The next product milestone is M1:
native bridge and internet lobby. Keep the first patch narrower than all of M1.

## Start here

```sh
git status --short --branch
git diff -- Makefile mods/multiplayer tests/test_multiplayer.c tools/package_multiplayer.py .milestones
make CC=gcc test-multiplayer
make CC=gcc multiplayer-demo
make test-multiplayer-swmm
```

Then read `CONTRIBUTING.md`, this directory in `README.md` order, and the nearest source around any intended hook.
Preserve unrelated work and do not edit generated sources.

## Recommended next patch: lobby protocol plus mock transport

Do this before adding a large networking dependency or touching guest game state.

1. Define payload codecs for HELLO, WELCOME, REJECT, PEER_JOINED, PEER_LEFT, START, PING, and PONG.
2. Put protocol/build/game/mod compatibility fields in HELLO and the authoritative session/roster in WELCOME.
3. Define length limits and UTF-8/error handling for names and rejection text; never trust peer lengths.
4. Implement a deterministic mock `transport.h` backend with loss, duplication, reordering, disconnect, queue limits,
   and backend-owned packet lifetime.
5. Drive lobby state through transport events instead of manually mutating sessions in the demo.
6. Test capacity, duplicate joins, timeouts, incompatibility, spoofed peer IDs, exhaustion, cancellation, disconnect,
   and late join.
7. Put packet authentication/encryption below `transport.h` if practical, and document the trust boundary.

Keep wire structs explicitly encoded. Bump the protocol version for incompatible changes once artifacts may be used
outside the repository.

## Native module ABI design spike

This needs maintainer agreement because it changes Savage Wind's executable-code boundary.

- Define a small C ABI with ABI version, init/shutdown, per-boot reset, update/poll, UI contribution, logging, and
  capability negotiation.
- The host owns guest/runtime API tables; modules must not retain guest pointers across reset.
- Load only explicit approved absolute paths. Never scan the RomFS overlay for libraries.
- Validate manifest ID, ABI, platform, architecture, hash/signature policy, and duplicates before loading.
- Make a no-op synthetic plugin fixture owned by this project; do not use Nintendo data.
- Prove load, reset, unload, disabled-mod startup, incompatible-ABI rejection, and failure cleanup.
- Keep Windows/Linux loaders behind one platform-neutral interface.

The final SWMM handoff likely belongs in the separate SWMM repository. Do not copy its GPL source into this MIT
repository.

## Transport selection spike

No transport library is selected. Evaluate maintained libraries for:

- C-compatible API and Windows/Linux support;
- ICE/STUN/TURN and encrypted data channels;
- reliable ordered and unreliable/unordered delivery modes;
- binary size, linking, license, update cadence, and security history;
- cancellation/background-thread behavior and deterministic mocking; and
- practical rendezvous deployment, authentication, and abuse/rate limits.

Record the choice in `DECISIONS.md`. An invite code identifies an opaque rendezvous record; it must not expose an IP
or contain a reusable secret. Direct peer paths still need authenticated signaling and relay fallback.

## UI integration after lifecycle exists

Use existing patterns around `runtime/ui/game_options.mm` and the translation tables. Minimum states are Host, Join,
Leave, creating/connecting, invite-code display/entry, roster, compatibility rejection, timeout, and cancellation.
Do not add art assets. Check player-facing text with `python3 tools/i18n_check.py`.

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
