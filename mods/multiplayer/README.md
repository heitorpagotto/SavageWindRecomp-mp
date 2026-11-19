# Savage Wind Multiplayer

This directory is the independently removable multiplayer mod. It contains no game assets, generated game code,
keys, saves, or extracted files. The only supported game target is Savage Wind's supported BOTW 1.9.0 build.

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

`make multiplayer-swmm-package` creates the current Savage Wind Mod Manager development archive. See
[`SWMM.md`](SWMM.md) for what can be tested now and the native-plugin bridge still required.

Nothing under this directory is linked into the Savage Wind runtime yet. Offline Savage Wind therefore behaves and
builds exactly as before. The first runtime integration will be a small plugin host boundary; removing the multiplayer
library will remove the feature. Until that boundary exists, this is developer scaffolding rather than an installable
release.

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
- Keep multiplayer hooks in strict optional groups. If one required signature is absent, multiplayer stays disabled
  and ordinary play continues.
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

1. **Foundation (this milestone):** protocol, session core, transport contract, and synthetic tests.
2. **Lobby:** approved native-module bridge for SWMM, Host/Join/Leave UI, short invite codes, encrypted ICE transport,
   compatibility handshake, and eight-instance localhost tests.
3. **Remote Links:** verified spawn/despawn hooks, position/action interpolation, current armour and weapons, map/phase
   visibility, reboot-safe cleanup.
4. **Combat:** stable actor identities, shared enemy health/death, player damage, optional PvP, late-join resync.
5. **Co-op world:** main-story transaction allowlist, host-controlled phase changes, per-player chest rewards, save
   backup and disconnect recovery.
6. **Campaign validation:** Great Plateau through Dark Beast Ganon, then Divine Beasts, shrines and DLC; record player
   count, FPS, latency/loss, GPU/driver, settings, save result, and observed desyncs.

The Wii U multiplayer projects are behavioral references only. No code, addresses, assets, or decompilation output is
copied from them.

