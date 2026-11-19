# Architecture decision record

These decisions are active unless explicitly superseded. If one changes incompatibly, bump the wire protocol version
and document migration behavior.

## D001 — Eight peers, host is peer 0

Status: accepted.

`SWMP_MAX_PEERS` is 8. Peer 0 is always the host. Joining clients receive IDs 1 through 7. This keeps payloads compact
and matches the product requirement of about eight total players.

## D002 — Host-authoritative star topology

Status: accepted.

Each client connects directly to the host. The host orders persistent transactions and contested combat state. A full
mesh would create 28 pairwise connections at eight players and still would not solve conflict authority. The host may
forward another player's state while preserving the original `sender_peer`.

## D003 — P2P gameplay, service-assisted connection setup

Status: accepted.

Invite codes require a rendezvous/signaling service. ICE/STUN attempts a direct encrypted route. TURN is an optional
fallback when direct NAT traversal fails. The service does not run game simulation, so the design remains P2P from a
gameplay perspective.

## D004 — Three transport channels

Status: accepted and implemented in protocol v1.

- Control: reliable and ordered for handshake/lobby messages.
- State: unreliable and unordered for frequent Link snapshots; newest sequence wins.
- Event: reliable and ordered for gameplay transactions.

Message types are validated against their required channel.

## D005 — Explicit serialization, no native structs on the wire

Status: accepted and implemented.

Every integer is written in network byte order. Headers and snapshots have fixed byte layouts. No padding, raw guest
memory, native pointers, or platform-dependent floating-point structs are transmitted.

## D006 — Stable identifiers, never guest pointers

Status: accepted.

Remote actors and world state require stable identifiers derived from audited placement identity or host-assigned
dynamic IDs. Guest addresses vary by process and must never cross the network or survive reboot.

## D007 — Host owns persistent world transactions

Status: accepted.

The host owns enemy health/death, story-flag transactions, chest-open state, world phase, time, and weather. Clients
own their local Link input/prediction and private inventory except for explicit grants. Story sync uses an allowlist;
never mirror all game flags.

## D008 — Host migration excluded from v1

Status: accepted.

If peer 0 leaves, clients return to offline state. Persistent transactions must be committed at safe points so this
does not corrupt saves. Host migration can be designed after campaign synchronization is stable.

## D009 — Multiplayer code remains optional and removable

Status: accepted.

The core lives under `mods/multiplayer/` and is not linked into the runtime today. Future hooks must be strict optional
groups. Disabled/absent multiplayer must leave ordinary Savage Wind unchanged.

## D010 — Native mods require explicit consent

Status: accepted.

Do not scan a RomFS overlay and silently execute DLLs/shared objects. Data mods are not an implicit native-code trust
decision. SWMM should show a permission warning and pass explicitly approved plugin paths, or Savage Wind should
verify an approved/signed plugin contract.

## D011 — Reuse installed game assets

Status: accepted.

Remote Links use the player's own existing Link and equipment assets at runtime. No new or extracted assets are
distributed.

## D012 — Transport backend not yet chosen

Status: open.

The required properties are encrypted data channels, Windows/Linux support, ICE/STUN, optional TURN, reliable ordered
and unreliable unordered delivery, and an acceptable redistributable license. WebRTC/libdatachannel is a candidate,
not a committed dependency. Perform a dependency/build/licensing spike before choosing.

