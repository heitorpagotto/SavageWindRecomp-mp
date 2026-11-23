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
libdatachannel. `make test-multiplayer-direct` runs the invite and reply flow with eight peers.

## Build and play

Follow the platform build guide and install the [multiplayer dependencies](../../.milestones/TESTING.md#m1-developer-build).

Both players open Multiplayer in F1 Settings or System > Options and turn on Enable multiplayer.

1. **Host:** Select **Host lobby**, then **Copy invite code**. Send the whole message to your friend in a private chat.
2. **Joining player:** Copy the invite from chat and select **Paste invite and join**. When **Copy reply**
   appears, use it and send the whole reply back to the host.
3. **Host:** Copy the reply from chat and select **Paste reply**. Use **New invite** for each additional player
   or if an invite expires.

Each invite is single-use and expires after five minutes. Send both messages only to the intended player because
they contain connection details. If chat truncates a message, send it as a text file and copy the full line.
Leave lobby or turn multiplayer off to disconnect. Multiplayer defaults to off.

The host and existing clients see a notice when someone joins or disconnects. A player also sees a
notice when their own session ends. **Sync host time and weather** is on by default.
Clients follow the host's clock and effective weather while in the same supported area. Turn it off to use
local time and weather.

Players can connect across different networks when ICE finds a direct route. The app uses the public STUN
endpoint from the WebRTC documentation to discover addresses. Game traffic flows between peers. Restrictive
routers or firewalls can prevent a direct connection. For development, `SAVAGEWIND_MP_STUN` overrides the
endpoint, and an empty value disables STUN. [WebRTC connection setup](https://webrtc.org/getting-started/peer-connections)

## Session model

The connection is peer-to-peer and gameplay is host-authoritative. The host is peer 0 and up to seven clients
connect directly over encrypted WebRTC. The invite and reply carry the ICE candidates and DTLS fingerprints.

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
| Time and weather | Host | Latest sample in the position batch |
| World phase | Host | Planned reliable phase updates |

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

