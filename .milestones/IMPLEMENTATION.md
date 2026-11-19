# Implemented foundation

## Status

This code is a developer foundation. It is not linked into Savage Wind and does not open a network socket. The
in-process demo uses the production packet/session code but a simulated packet bus.

## Protocol constants

Defined in `mods/multiplayer/protocol.h`:

- `SWMP_PROTOCOL_VERSION = 1`
- `SWMP_MAX_PEERS = 8`
- `SWMP_HOST_PEER = 0`
- `SWMP_UNASSIGNED_PEER = 255`
- `SWMP_PACKET_HEADER_SIZE = 32`
- `SWMP_PLAYER_SNAPSHOT_SIZE = 44`
- `SWMP_MAX_PAYLOAD_SIZE = 65536`

Wire integers are big-endian/network byte order.

## Packet header layout

| Offset | Size | Field |
|---:|---:|---|
| 0 | 4 | ASCII magic `SWMP` |
| 4 | 2 | protocol version |
| 6 | 1 | message type |
| 7 | 1 | flags |
| 8 | 4 | payload size |
| 12 | 8 | session ID |
| 20 | 1 | sender peer ID |
| 21 | 1 | channel |
| 22 | 2 | reserved, must be zero |
| 24 | 4 | sequence number |
| 28 | 4 | fixed simulation tick |

Packet flags: request bit 0, response bit 1, critical bit 2. Request and response cannot both be set. Unknown flag bits
are rejected.

## Channels and messages

| Channel | Value | Required messages |
|---|---:|---|
| Control | 0 | HELLO 1, WELCOME 2, REJECT 3, PEER_JOINED 4, PEER_LEFT 5, START 6, PING 7, PONG 8 |
| State | 1 | PLAYER_SNAPSHOT 16 |
| Event | 2 | EQUIPMENT 17, DAMAGE_REQUEST 32, DAMAGE_APPLIED 33, ACTOR_HEALTH 34, FLAG_TRANSACTION 48, CHEST_REWARD 49, WORLD_PHASE 50, RESYNC_REQUEST 64, RESYNC_SNAPSHOT 65 |

The decoder rejects bad magic, unsupported version, unknown type/flags/channel/peer, nonzero reserved bits, oversized
payload, wrong message channel, and packet-size mismatch. An unassigned sender is accepted only for HELLO.

## Player snapshot layout

| Offset | Size | Field |
|---:|---:|---|
| 0 | 1 | peer ID |
| 1 | 1 | grounded/gliding/crouching/guarding/dead flags |
| 2 | 2 | action ID |
| 4 | 4 | area ID |
| 8 | 4 | phase ID |
| 12 | 12 | X/Y/Z position in millimetres, signed 32-bit each |
| 24 | 6 | three signed 16-bit turn fractions |
| 30 | 6 | X/Y/Z velocity in centimetres/second, signed 16-bit each |
| 36 | 2 | health in quarter units |
| 38 | 2 | stamina in quarter units |
| 40 | 4 | equipment revision |

Equipment payloads are declared but not implemented. The revision lets a receiver request/expect a reliable equipment
update when appearance changes.

## Session state

Roles: none, host, client.

Session states: offline, connecting, lobby, playing.

Peer states: empty, connecting, ready.

Implemented behavior:

- Host initializes as peer 0 in lobby.
- Client initializes connecting and becomes ready after WELCOME assigns ID 1..7.
- Host allocates the first free client slot.
- Clients can register other remote peers after PEER_JOINED.
- Host and client can enter playing state.
- Each receiver tracks last sequence independently per peer and channel.
- Duplicate and older sequences are dropped; 32-bit wrap comparison is supported.
- Packets from the wrong session, unknown peer, or the local peer are dropped.
- Removing the host resets a client to offline. Host migration is absent.
- Late peer addition is allowed while the host is already playing; full resync is not implemented.

Public API is in `mods/multiplayer/session.h`. Core calls are expected to remain on one multiplayer/game thread; the
struct is not internally synchronized.

## Transport interface

`mods/multiplayer/transport.h` defines an unimplemented backend contract:

- start host with a rendezvous URL;
- start join with rendezvous URL and invite code;
- poll invite/connection/disconnection/packet/error events;
- send to a peer on a selected channel; and
- stop.

The event's packet buffer is owned by the backend until the next poll. This ownership rule may need revisiting once a
real threaded backend and queue exist.

## Demo

`mods/multiplayer/demo.c` creates one host and seven clients, completes their in-memory lobby membership, starts the
session, encodes/decodes ten snapshots per Link, delivers every Link to every other Link, and verifies a duplicate is
dropped.

Current deterministic result:

- 8 Links;
- 10 ticks;
- 560 accepted snapshot deliveries (`8 * 7 * 10`); and
- 1 intentionally duplicated state packet rejected as stale.

It does not exercise sockets, encryption, NAT traversal, signaling, loss, jitter, or the game runtime.

## SWMM package source

Version: `0.1.0-dev`.

The package builder creates exactly:

```text
info.json
romfs/System/SavageWind/Multiplayer/package.swmp
```

The marker contains package format v1, ID `org.savagewind.multiplayer`, protocol 1, and `runtime = native`. It is data,
not executable code. Zip timestamps and permissions are fixed, so rebuilding is byte-for-byte reproducible.

Current generated artifact:

```text
build/mods/SavageWind-Multiplayer-0.1.0-dev-SWMM.zip
SHA-256 731f2548d8e9c691defe0437a310d621a02e09a1eab2a83a73b3e991475db867
```

Build artifacts are not source and should not be committed unless release policy explicitly requires them.

