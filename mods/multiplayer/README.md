# Savage Wind Multiplayer

This directory contains the protocol, lobby, and transport for Savage Wind's native Windows and Linux multiplayer.
The guest integration targets BOTW 1.9.0.

## Current status

The standalone multiplayer core provides:

- a versioned, endian-independent packet envelope;
- an eight-peer session state machine;
- separate control, transient-state, and reliable-gameplay channels;
- a compact remote-player snapshot format;
- a lobby handshake with build and feature checks, host-assigned player IDs, and roster updates; and
- a deterministic mock transport with bounded queues and injectable state loss, duplication, and reordering.

Run the synthetic checks with `make test-multiplayer`. They need no game files.

For a visible end-to-end simulation, run `make CC=gcc multiplayer-demo` (or omit `CC=gcc` when Clang is installed).
It joins seven clients to a host through the mock transport, starts the lobby, then checks in-process Link snapshot
delivery and rejects a stale duplicate.

The standalone `make test-multiplayer-webrtc` target exercises the lobby over encrypted loopback sockets using
libdatachannel. `make test-multiplayer-rendezvous` also exercises HTTPS signaling.

## Build and play

Follow the platform build guide and install the [multiplayer dependencies](../../.milestones/TESTING.md#m1-developer-build).
Point both players' apps at the same trusted service with `SAVAGEWIND_MP_RENDEZVOUS=https://...`.
For local tests the repository includes [a rendezvous server](../../tools/multiplayer_rendezvous.py).

Open Multiplayer in F1 Settings or System > Options and turn on Enable multiplayer. The host selects Host lobby and
shares the invite code. Clients select Join lobby and enter that code. Leave lobby disconnects the local player.
Turning multiplayer off in either menu also ends local participation. The preference defaults to off.

## Session model

The connection is peer-to-peer over the internet, but gameplay is host-authoritative. The host is peer 0 and up to
seven clients connect directly to it. A small rendezvous service maps a short invite code to connection offers and ICE
candidates; it never simulates gameplay. Direct encrypted connections are preferred. A TURN relay is the fallback for
NATs that cannot establish a direct route, because internet P2P cannot work reliably for every home network without
one.

Host authority avoids conflicting quest flags, enemy health, chest state, and damage decisions. Clients own their
local input and predict their own Link, while the host orders persistent transactions. Protocol v1 deliberately has no
host migration: if the host leaves, the session ends without changing anyone's save.

| State | Authority | Replication rule |
|---|---|---|
| Link movement and animation | Each Link's player | Frequent unreliable snapshots; interpolate remote Links |
| Clothing and equipment | Each Link's player | Reliable update when the equipment revision changes |
| Enemy health and death | Host | Clients request hits; host publishes the accepted health transaction |
| Player-vs-player damage | Host | Opt-in session rule; validate attacker, target, distance, phase, and cooldown |
| Main-story flags | Host | Reliable ordered allowlist, never blanket-copy all game flags |
| Chests | Host | One shared open transaction plus one reward transaction per connected player |
| Inventory, rupees, durability | Each player | Private unless a specific cooperative transaction grants an item |
| Time, weather, world phase | Host | Snapshot on join and ordered changes while connected |

The host's save is the campaign authority. Before joining, every client gets a temporary backup. Persistent shared
progress is applied only at explicit safe points; disconnecting during a transaction must leave the local save valid.

## Runtime integration rules

- Use the player's existing Link models and equipment variants at runtime. Do not add or redistribute assets.
- Remote Links are proxy actors. They must not replace the game's singleton local-player object or retain guest
  pointers across a reboot.
- Discover every 1.9.0 hook through `tools/game_sites.py`, patch it through `tools/patches.py`, assert the replaced
  instructions, and access runtime addresses through `g_site`.
- Keep multiplayer hooks in strict optional groups. If one required signature is absent, the affected feature stays
  disabled and ordinary play continues.
- Queue transport events from its worker thread. Read or mutate guest state only on the game thread.
- Use a fixed multiplayer tick independent of presented or generated frames. Test at 30, 60, 90, and 120 FPS.
- Treat names, invite responses, packet sizes, enum values, actor identifiers, and flag transactions as untrusted.
- Never synchronize raw guest pointers or whole memory regions. Stable identifiers and explicit fields only.

## Campaign synchronization

Full-campaign cooperation is more than copying a save. Story events, shrines, Divine Beasts, interiors, cutscenes, and
open-world cells can put players in different world phases. Each player snapshot therefore carries an area and phase.
Players only see and interact with peers in a compatible phase. Phase transitions are host-ordered; if an event cannot
safely run for everyone, clients spectate or rejoin at the next safe checkpoint rather than forcing guest state.

Story synchronization will use an audited allowlist of flags grouped by quest transaction. Transient UI flags,
platform settings, DLC ownership, tutorial prompts, local position, and inventory bookkeeping are never part of that
allowlist. Chest rewards are explicit grants so every player receives loot without duplicating unrelated world drops.

## Milestones

See the [milestone ledger](../../.milestones/MILESTONES.md) for the current status and acceptance criteria.

The Wii U multiplayer projects are behavioral references only. No code, addresses, assets, or decompilation output is
copied from them.

