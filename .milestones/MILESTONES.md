# Milestone ledger

Last reviewed 2026-11-23. Status values are `complete`, `partial`, `not started`, and `blocked`.
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

Status **accepted by the project owner**. The owner confirmed Windows and Linux could join when the friend
hosted and approved starting M2. Hosting from the owner's network still needs diagnosis. See the
[test record](TESTING.md) for the verification scope and open cases.
The [direct multiplayer handoff](DIRECT_MULTIPLAYER_HANDOFF.md#security-findings) records the open transport security findings.

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
- Movement and basic animation synchronization begin in M2.

## M2 — Remote Link presence

Status **complete**. The owner accepted M2 and moved the remaining manual validation to
[M7](#m7--deferred-manual-validation). The main field supports nearby remote Link proxies driven by peer position,
facing and native idle, walk, run, sprint and jump clips. The F1 multiplayer roster displays reported
quarter-heart health. Windows Slate travel preserved the lobby and nearby presence recovered after reunion.
Two Windows instances rendered proxies in the same shrine and withheld them across different shrines.
Two native shrine round trips retired and restored both proxies while the original lobby stayed connected.
The Windows range test verified 50 m spawn and 60 m retention. A single renderer kept seven fixture-driven
proxies active through measured 30, 60, 90, and 120 FPS. Four full Windows games held mutual presence
for 90 seconds.

Goal: every peer sees the other seven players as Links using existing game assets.

Planned deliverables:

- Version-aware game sites/patches for player observation and remote proxy lifecycle.
- Stable network identity for remote actors; no guest pointer crosses the network or survives a reboot.
- Snapshot interpolation/extrapolation, teleport/area transition handling, animation mapping, and visibility rules.
- Join-in-progress resync and peer removal cleanup.
- An independently maintained multiplayer release number, starting at `0.1.0`. Append `-<multiplayer version>-mp`
  to the full Savage Wind version, for example `0.6.0-alpha.6-0.1.0-mp`.

Acceptance criteria:

- Movement, facing, common actions, and health converge correctly.
- Loss and jitter do not let remote proxies corrupt guest state or become the local-player singleton.
- Plateau/interior/travel transitions and late join are tested at 30/60/90/120 FPS where relevant.
- Windows and Linux Make targets report the same combined release label. The lobby checks protocol and build
  compatibility independently of the release label.

## M2.5 — Time, weather, and full Link synchronization

Status **in progress**. The first slice implements host time and weather sync and admission notifications.
The equipment slices add passive weapon, shield and bow slots and reliable identities. New native clips cover
landing, standing draw, attacks, charge and stow for three melee families, shield guard blends and reactions,
crouch, and grounded bow draw, reload, aim, release and stow with native bowstring playback
and passive nocked visuals for all six inventory arrow types,
slow and fast swimming with turns, directional targeting strokes, directional wall climbing and jumps,
wall approach and mantle, grounded lock-on dodges and backflips with landing clips, hit,
knockdown and fall-damage recovery clips, and gliding with a passive canopy that banks with
Link during turns. Grounded locomotion and equipment share buffered playback, and standing jumps
start at the sampled animation phase. Default hair and equipped armor use skinned models that follow
the remote proxy. Material tint and body mesh masking remain open.
The rest of the equipment and animation catalog and overhead names are open. See the
[playback handoff](HANDOFF.md#m25-remote-movement-and-standing-jump-playback) and
[animation backlog](NEXT.md#remaining-animation-work).

Goal: clients share the host's time of day and weather, and every player sees the other Links' equipped
gear and animations.

Planned deliverables:

- Host-authoritative time of day and weather, including clock progression and weather transitions. Expose
  one multiplayer setting to enable or disable both, enabled by default.
- Show `[UserName] connected to the game` to the host and existing peers when the host admits a player.
- Show a named disconnect notice to other players and a local notice when the player's own session ends.
- If feasible, display player names above their remote Links. Add a multiplayer setting to hide the names,
  with name display enabled by default.
- Reliable appearance and equipment updates keyed by `equipment_revision`, covering every armor slot,
  weapons, shields, and bows, including equip, unequip, drawn, and stowed states.
- Full remote animation playback, covering traversal, locomotion, equipment use, and combat animations.
- Initial state and resync for late join, reconnect, and travel.

Acceptance criteria:

- With the default setting enabled, Windows and Linux clients converge on the host's time of day and observe
  consistent weather transitions when sharing an area, including after joining late, reconnecting, or
  traveling. Disabling the setting leaves time and weather under local game control.
- The join notification uses the admitted player's in-game name and appears once per successful admission.
- If overhead names ship, verify that they follow their remote Links and can be hidden with the setting.
- Each peer sees other Links' armor, weapons, shields, and bows update as players change equipment. Their
  remote animations match the actions they perform, including combat animations.
- Changes in a peer's gear or animation leave the receiving player's inventory and controls independent.

## M3 — Combat, enemies, and PvP

Status: **not started**.

Goal: peers cooperate against enemies and damage one another without duplicate authoritative damage.

Planned deliverables:

- Stable actor identity mapping for networked enemies.
- Client damage requests with host validation and idempotent authoritative results.
- Shared enemy health/death and appropriate status/knockback replication.
- PvP rules that preserve the base combat feel and avoid feedback loops.
- Ownership/rate/range checks and reconciliation after prediction disagreement.
- Show remote players' locations as markers on the in-game map screen.

Acceptance criteria:

- Multiple peers damaging one enemy observe one convergent health/death outcome.
- Replayed, duplicated, forged, stale, and out-of-range requests are rejected.
- PvP works in both directions with consistent health and death state.
- High latency, packet loss, and simultaneous-hit cases are reproducible in tests.
- Map markers follow each player's reported location while it belongs to the displayed map and disappear
  when that player leaves the session.

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

## M7 — Deferred manual validation

Status **not started**. The owner can run these checks independently of M2.5 and later implementation.

- **Eight-game soak.** With matching builds in the same supported area, confirm that all eight full game
  instances hold seven remote Links each for a sustained session. Include a late join after START and
  check actor cleanup after one peer leaves. Record per-client logs, screenshots and measured FPS.
- **Linux AppImage.** Build and launch the package, verify its bundled dependencies, and confirm that the
  artifact name and running app report the same combined release label as the matching Windows build.
- **Mixed Windows/Linux gameplay.** Test both host directions with matching builds. Confirm nearby remote
  Links are visible in the same supported area and reappear after peers reunite following Slate travel.
  Record both logs and player locations if either peer fails to reappear within the supported range.

See the [test record](TESTING.md#m2-acceptance-follow-up) for the four-game soak, Windows ZIP, and Linux
build and loopback results.

## Dependency order

The implementation order is M0 → M1 → M2 → M2.5 → M3 → M4 → M5 → M6. Story work follows stable actor
identity, resync, and transaction semantics. M7 tracks independent manual validation.
