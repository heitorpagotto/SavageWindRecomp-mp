# Milestone ledger

Last reviewed: 2026-11-19. Status values are `complete`, `partial`, `not started`, and `blocked`.

Completion here means the listed acceptance criteria were demonstrated. A synthetic milestone is not gameplay
verification, and a package that imports is not proof that native multiplayer runs.

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

## M0.5 — SWMM development packaging

Status: **partial**.

Goal: make the work discoverable as a separately installable mod package while defining the missing safe native-code
integration.

Delivered:

- Mod identity `org.savagewind.multiplayer`, development version `0.1.0-dev`.
- Deterministic ZIP builder and package validation target.
- Raw RomFS marker at `System/SavageWind/Multiplayer/package.swmp`.
- Source-level investigation of current SWMM and its UKMM dependency.
- Canonical ZIP metadata and line endings, strict archive validation, and validated atomic replacement.
- Draft native manifest and approval handoff for maintainer review.

Accepted so far by deterministic archive generation, structure/metadata validation, and an identical SHA-256 after a
rebuild.

Still required:

- Manual SWMM GUI import, enable, deploy, launch, disable, and uninstall test.
- A versioned Savage Wind native-plugin ABI/loader.
- Explicit native-code approval and approved-plugin-path handoff from SWMM.
- Built native modules for every supported Savage Wind target, packaged for drag-and-drop SWMM installation, and an agreed integrity policy.

The current archive is intentionally a development marker; it cannot execute the C core.

## M1 — Native bridge and internet lobby

Status: **partial (synthetic lobby, encrypted loopback probe, draft native ABI)**.

Delivered:

- Reliable control payloads for the compatibility handshake, roster changes, start, and ping/pong.
- Host-authoritative admission over opaque connection handles, including bounded UTF-8 fields and handshake deadlines.
- Deterministic in-process transport with state faults and bounded queues. The eight-peer demo joins through the transport.
- Synthetic coverage for eight players, capacity rejection, build and feature mismatch, spoofing, repeated HELLO,
  queue exhaustion, cancellation, disconnect, and late join. Late join receives lobby state. Gameplay resync is pending.
- Windows two-peer WebRTC probe with real encrypted loopback sockets, using trusted local signaling and synthetic
  build IDs. Checks admission, build rejection, all three channel modes, START, and host disconnect.
- Standalone draft native ABI fixture. Windows lifecycle passed and Linux x86-64 binaries cross-compiled.

Goal: host and join a real encrypted internet session using an invite code, with the multiplayer module loaded and
unloaded independently from core gameplay.

Planned deliverables:

- A small versioned plugin ABI between Savage Wind and the multiplayer module.
- Reset/unload-safe runtime lifecycle using `RT_ON_RESET` where applicable.
- A production transport backend supporting reliable control/event traffic and replaceable unreliable state traffic.
- Rendezvous signaling, opaque invite codes, ICE/STUN NAT traversal, and TURN relay fallback.
- Host/Join/Leave UI using existing Savage Wind UI primitives and translation tables.
- Wire compatibility and lobby lifecycle validation over the production transport, including localized errors and runtime shutdown.
- A compiled SWMM test build for visual lobby verification in a supported BOTW 1.9.0 gameplay area. On successful
  lobby admission, each game instance spawns one visible placeholder actor per remote peer. An existing idle NPC
  or static actor is sufficient. Movement, animation, and equipment synchronization remain M2 deliverables.

Acceptance criteria:

- Host receives an invite code and seven clients can join a test session.
- Joining works across two different external networks; TURN fallback is deliberately exercised.
- Incompatible builds are rejected with a useful localized error.
- Disable/uninstall leaves single-player behavior and user data unchanged.
- Rebooting or leaving releases threads, queues, sockets, and guest references.
- Authenticated, encrypted lobby traffic is verified on Windows and Linux, including a session with a host on one OS and a client on the other.
- Import, approve, enable, and launch the test build through SWMM. Host and join from inside the game. In the
  supported test area, the host sees one placeholder for each admitted client, and each client sees one for each
  other admitted peer, including the host and any peers who joined earlier. Rejected joins create no actors.
  Leaving, disconnecting, or rebooting removes the corresponding placeholders without affecting the local player
  or saves.

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

Status: **not started**.

Goal: ship a maintainable, removable, safe multiplayer mod for every supported Savage Wind platform, currently Windows and Linux/Vulkan.

Planned deliverables:

- Stable protocol/plugin/package versioning and compatibility policy.
- Reproducible native builds and SWMM-ready release archives for each supported platform.
- Security review, fuzzing, malformed-packet tests, dependency audit, and resource limits.
- Localization, diagnostics, privacy notice, and troubleshooting documentation.
- SWMM drag-and-drop installation, update, disable, and uninstall with native-code consent.

Acceptance criteria:

- On a fresh setup for each supported platform, drag and drop the compiled archive into SWMM and verify the full lifecycle from approval and launch through update, disable, and uninstall.
- Release packages contain only redistributable project-owned code and metadata.
- Long-running eight-player soak tests pass with bounded memory and queues.
- Single-player behavior is unchanged when the mod is absent or disabled.

## Dependency order

The intended order is M0 → M0.5/M1 → M2 → M3 → M4 → M5 → M6. M0.5 and M1 can overlap only after the native
plugin boundary and consent model are agreed with SWMM/Savage Wind maintainers. Story work should not precede stable
identity, resync, and transaction semantics.
