# Multiplayer architecture

## Topology

```text
                         invite code / signaling
                    +-------------------------------+
                    | small rendezvous service      |
                    | offers + ICE candidates only  |
                    +---------------+---------------+
                                    |
          direct encrypted P2P      |      direct encrypted P2P
      +-----------------------------+-----------------------------+
      |                             |                             |
  client 1  <--------------------> host 0 <--------------------> client 7
                                     |
                              gameplay authority
```

The host has at most seven direct client connections. Clients do not require direct connections to one another. TURN
may carry encrypted packets when a direct path cannot be established; it does not gain gameplay authority.

## Session lifecycle

Planned full lifecycle:

1. Host creates random session identity and asks rendezvous service for a short invite code.
2. Client enters code. Service exchanges connection offers and ICE candidates.
3. Encrypted transport connects.
4. Client sends `HELLO` with protocol/mod/game compatibility and display name.
5. Host rejects mismatch/full sessions or replies `WELCOME` with assigned peer ID and session rules.
6. `PEER_JOINED` makes the lobby membership consistent.
7. Host sends authoritative resync snapshot for late join.
8. Host sends `START`; peers enter playing state.
9. On disconnect, host sends `PEER_LEFT`; if the host leaves, clients stop the session safely.

Only the basic in-memory session transitions exist today. HELLO/WELCOME payload codecs and compatibility checking are
not implemented.

## Compatibility handshake fields still required

- Wire protocol version.
- Multiplayer mod semantic version.
- Savage Wind source/build identifier.
- Game version and main-module build ID; primary target must be 1.9.0.
- Feature-bit set and required/optional feature distinction.
- Display name after UTF-8 validation and length limit.
- Session nonce/transport identity binding to prevent replay across sessions.
- Host rules: PvP, player limit, late join, persistence policy, DLC expectations.

Never accept a peer based only on the short invite code. The encrypted transport identity and session nonce must bind
the handshake to the rendezvous exchange.

## Authority and replication

| State | Authority | Transport behavior |
|---|---|---|
| Local Link input/prediction | that player | local, with snapshots to host |
| Link position/action | originating player; host relays | frequent state channel |
| Armour/weapons/shield/bow | originating player | reliable event on revision change |
| Player health/PvP result | host | request then authoritative event |
| Enemy health/death | host | request then authoritative event |
| Story quest progress | host | reliable allowlisted transaction |
| Chest opened | host | reliable shared transaction |
| Chest loot | host creates one grant per player | reliable per-player transaction |
| Inventory/rupees/durability | each player | private except explicit grant/cost |
| Time/weather/world phase | host | snapshot on join plus reliable changes |

## Remote Links

Remote Links must be proxy actors, not additional local-player singletons. Expected approach:

- discover a safe actor-spawn path for 1.9.0 through `tools/game_sites.py`;
- spawn/despawn proxies only on the game thread;
- interpolate the two newest snapshots at a fixed delay;
- hide a proxy when area/phase is incompatible;
- apply existing equipment variants by stable equipment identity;
- prevent proxy actors from owning camera, UI, save, input, or singleton player state; and
- release every proxy and guest pointer on reboot via `RT_ON_RESET`.

This is a research area. Do not assume spawning the normal Link actor multiple times is safe.

## World identity

Enemy addresses differ between machines. Static world actors need a canonical identity based on audited placement
data such as map/area, actor type/name hash, placement identity, and world phase. Dynamic actors need host-assigned IDs
scoped to a session/phase. Identity collisions and actor unload/reload behavior require synthetic and gameplay tests.

## Story and phase synchronization

Do not copy a whole save or every game-data flag. Build an audited allowlist grouped into atomic quest transactions.
Exclude transient UI state, tutorial prompts, platform options, DLC ownership, local position, physics state, and
inventory bookkeeping.

Interiors, shrines, Divine Beasts, cutscenes, and overworld cells can place peers in incompatible phases. Snapshots
carry `area_id` and `phase_id`. When an event cannot safely run for every peer, non-authoritative peers should spectate
or rejoin at a known checkpoint instead of forcing arbitrary guest memory.

## Persistence and saves

- Back up every client's save before applying shared persistent progress.
- Host save is authoritative in protocol v1.
- Commit ordered transactions only at known safe points.
- Make transactions idempotent with IDs so reconnect/resend cannot duplicate rewards.
- Chest rewards must be explicit grants; do not duplicate arbitrary world drops.
- Disconnect during a transaction must leave each local save valid.
- Never test persistence against the user's only save; use scratch copies and the documented save-directory override.

## Threading

- Transport worker thread owns network polling and encryption callbacks.
- It pushes bounded, validated events into a queue.
- The game thread drains events and is the only thread that reads/writes guest state.
- The game thread produces outgoing snapshots into another bounded queue.
- Queue overflow drops replaceable state snapshots first, never reliable persistent events.
- Shutdown and in-place reboot stop transport workers and discard every guest pointer/handle.

## Timing

Use a fixed multiplayer simulation tick independent of:

- game update frequency;
- presented frames; and
- frame-generated display frames.

Snapshots should be rate-limited and interpolated. Validate at 30, 60, 90, and 120 game FPS, with frame generation
both off and on where relevant. Never let render frequency multiply network traffic or damage transactions.

## Security boundaries

- Transport encryption is mandatory.
- Validate packet length, version, enum values, channel, sender, session, sequence, names, actor IDs, and transaction
  fields before use.
- Rate-limit handshake, damage, resync, and persistent-event requests.
- The host validates distance, phase, target existence, cooldown, and plausible damage before applying PvP/enemy hits.
- No raw memory write API is exposed to peers.
- Native plugin execution requires explicit user approval; a content overlay alone is insufficient consent.

