# Milestone ledger

Last reviewed 2026-11-22. Status values are `complete`, `partial`, `not started`, and `blocked`.
Completion requires demonstrated acceptance criteria. Synthetic, build, network, and gameplay evidence have distinct scopes.

## M0 — Protocol and session foundation

Status: **complete for synthetic scope**.

Goal: establish a testable, game-independent core before touching guest state or networking libraries.

Delivered:

- Explicit version-1 packet header and validation.
- Fixed-size player snapshot codec with endian-safe serialization.
- Eight-peer host/client session model and per-channel sequence filtering.
- Abstract transport interface with no backend dependency.
- In-process eight-Link demo and focused C tests.
- Makefile targets for the tests and demo.

Accepted by:

- `make CC=gcc test-multiplayer` prints `multiplayer core: ok`.
- `make CC=gcc multiplayer-demo` reports 8 Links, 560 deliveries, and one rejected duplicate.
- GCC static analysis finds no diagnostics in the protocol/session/demo sources.

Limit: nothing is linked into Savage Wind and no network connection is made.

## M1 — Native multiplayer build and internet lobby

Status **partial**. Native source integration and both Settings surfaces are implemented. See the
[test record](TESTING.md) for Windows loopback verification and pending native Linux and external-network acceptance.

Delivered

- Eight-peer host-authoritative lobby, compatibility handshake, roster changes, START, ping/pong, and deterministic
  coverage for capacity, build and feature mismatch, spoofing, cancellation, disconnect, and late join.
- WebRTC with reliable ordered control/event channels and an unreliable unordered state channel. The two-message
  clipboard exchange carries per-peer invites and replies. [Player flow](../mods/multiplayer/README.md#build-and-play)
- Localized F1 overlay lobby UI, reset and exit cleanup, and a copied admitted roster with slot generations.
- Default-off persisted enable setting and a native Options page with the same lobby controls.
- Strict 1.9.0 Great Plateau placeholder lifecycle. The Windows prototype visibly rendered two rocks in each of
  three live instances. The native build requests an NPC for each peer. [Test record](TESTING.md#m1-windows-presence-check)

Acceptance criteria

- Build the native multiplayer Savage Wind app on Windows and Linux/Vulkan. The Multiplayer tab is visible in
  F1 Settings and a Multiplayer page is reachable from the game's System > Options.
- Both surfaces share one enable/disable setting and session state. Enable exposes Host and Join. Disable during
  a session leaves or closes it and cleans up the lobby and placeholders. Single-player play and user data
  remain available.
- Fresh Windows and Linux/Vulkan installs can start multiplayer and exchange an invite and reply through chat
  without deploying a signaling server. Invalid, mismatched, or expired replies leave existing peers connected.
- A host can invite seven clients to the same lobby. Incompatible builds are rejected with a localized error.
- Verify authenticated, encrypted lobby traffic on Windows and Linux, with each OS hosting a peer on the other.
  Verify a direct ICE path across two external networks.
- In the supported 1.9.0 test area, each peer sees one visible NPC placeholder per admitted remote peer,
  including peers who joined earlier. Rejected joins spawn nothing. Leave, disconnect, disable,
  and reboot clear the appropriate placeholders and release session resources and guest references.
- Movement, animation, and equipment synchronization begin in M2.

## M2 — Remote Link presence

Status: **not started**.

Goal: every peer sees the other seven players as Links using existing game assets.

Planned deliverables:

- Version-aware game sites/patches for player observation and remote proxy lifecycle.
- Stable network identity for remote actors; no guest pointer crosses the network or survives a reboot.
- Snapshot interpolation/extrapolation, teleport/area transition handling, animation mapping, and visibility rules.
- Reliable equipment/appearance updates keyed by `equipment_revision`.
- Join-in-progress resync and peer removal cleanup.

Acceptance criteria:

- Eight players are visible in the same supported area for an extended session.
- Movement, facing, common actions, health, and worn equipment converge correctly.
- Loss and jitter do not let remote proxies corrupt guest state or become the local-player singleton.
- Plateau/interior/travel transitions and late join are tested at 30/60/90/120 FPS where relevant.

## M3 — Combat, enemies, and PvP

Status: **not started**.

Goal: peers cooperate against enemies and damage one another without duplicate authoritative damage.

Planned deliverables:

- Stable actor identity mapping for networked enemies.
- Client damage requests with host validation and idempotent authoritative results.
- Shared enemy health/death and appropriate status/knockback replication.
- PvP rules that preserve the base combat feel and avoid feedback loops.
- Ownership/rate/range checks and reconciliation after prediction disagreement.

Acceptance criteria:

- Multiple peers damaging one enemy observe one convergent health/death outcome.
- Replayed, duplicated, forged, stale, and out-of-range requests are rejected.
- PvP works in both directions with consistent health and death state.
- High latency, packet loss, and simultaneous-hit cases are reproducible in tests.

## M4 — Shared world, story, and chests

Status: **not started**.

Goal: players share main-story progression and receive individual rewards from shared chest interactions.

Planned deliverables:

- An allowlisted schema for multiplayer-safe story flags and world phases.
- Host-authoritative, ordered, idempotent flag transactions and resync snapshots.
- Chest-open transaction with one eligibility record and one reward per player.
- Explicit policy for inventory-full, absent, disconnected, and late-joining players.
- Durable transaction journal plus save backup/recovery behavior.

Acceptance criteria:

- A defined main-quest slice completes together and resumes consistently after reconnect/restart.
- No blanket flag-table copy occurs; private/transient flags remain local.
- Opening a chest cannot duplicate or lose grants after retransmission or reconnect.
- Eight-player simultaneous interactions converge to one world state.

## M5 — Full main-campaign playthrough

Status: **not started**.

Goal: complete the supported main campaign from a clean save with up to eight players.

Planned deliverables:

- Area, dungeon, shrine, boss, cutscene, warp, death/revival, and quest compatibility work.
- Recovery/resync tools for disconnects and divergent clients.
- A documented support matrix and known limitations.

Acceptance criteria:

- A recorded clean-save campaign run reaches the ending with shared story state.
- Rejoin, host restart, death, travel, cutscenes, and boss transitions have documented outcomes.
- Game version, FPS, GPU/driver, settings, and network conditions are recorded.
- No Nintendo data, generated game code, saves, or extracted content enters the repository or release archive.

## M6 — Release hardening

Status **not started**.

Goal: ship maintainable native multiplayer builds for every supported Savage Wind platform, currently Windows
and Linux/Vulkan.

Planned deliverables:

- Stable protocol and build compatibility policy, reproducible native builds, and a documented manual upstream
  release update procedure.
- Security review, fuzzing, malformed-packet tests, dependency audit, and resource limits.
- Localization, diagnostics, privacy notice, and troubleshooting documentation.

Acceptance criteria:

- Build and verify the native app on a fresh setup for each maintained platform. Exercise enable, Host, Join,
  Leave, disable, and single-player operation using the built-in Settings.
- Build outputs intended for distribution contain only redistributable files.
- Long-running eight-player soak tests pass with bounded memory and queues.
- On a subsequent Savage Wind release, apply the multiplayer changes and verify the two platform builds, both
  Settings surfaces, and compatible cross-platform lobbies.

## Dependency order

The intended order is M0 → M1 → M2 → M3 → M4 → M5 → M6. Story work follows stable actor identity, resync,
and transaction semantics.
