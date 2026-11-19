# Implemented foundation

## Status

The standalone core implements the lobby over an in-process transport. An opt-in test exercises it over encrypted
loopback sockets. Production transport, rendezvous, and runtime integration are pending.

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
payload, wrong message channel, and packet-size mismatch. An unassigned sender is accepted only for HELLO. The
lobby currently sends and accepts control packets with zero flags.

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

## Control payloads

`lobby_protocol.h` declares the payload types. Integers use network byte order. Text is a one-byte length followed
by UTF-8, with 1–31 bytes for a player name and 1–95 for a rejection. Decoding rejects embedded NUL, C0/C1
controls, malformed UTF-8, duplicate roster IDs, and trailing bytes.

| Message | Payload |
|---|---|
| HELLO | u16 protocol, two sets of three u16 version components (mod then game), 20-byte Savage Wind build ID, 32-byte main module build ID, u32 supported features, u32 required features, 16-byte connection binding, name |
| WELCOME | Assigned client ID, playing flag (0/1), u32 feature intersection, repeated ID/name entries including host and assignee |
| REJECT | Reason code byte and diagnostic text |
| PEER_JOINED / PEER_LEFT | Client ID and a name for PEER_JOINED |
| START | Empty |
| PING / PONG | u64 token, echoed in PONG |

`lobby.c` owns control dispatch. HELLO uses an unassigned sender and zero session ID and sequence. The host matches
the binding, compares build/version fields and required features, then assigns a slot. The client adopts the host's
session ID and roster on WELCOME. The playing flag lets a late joiner enter the same lobby state. A 5-second
deadline bounds the joining client and each pending host-side handshake. An incompatible envelope version is
dropped by the packet decoder and reaches the ordinary handshake timeout.

## Transport and demo

`transport.h` routes events and sends by opaque connection handle. The lobby binds each admitted connection to
one sender ID. The mock uses a single in-process rendezvous and a predictable binding. Production backends must
authenticate the link and supply their own binding. Event storage is owned by the backend until the next poll
or stop, and sends copy their input.

`mock_transport.c` supports 16 endpoints and 16 queued packets per endpoint plus separate lifecycle notifications.
Fault injection can drop, duplicate, or reorder the next state send. Reliable traffic can evict queued state and
returns failure if the queue is filled with reliable traffic. The lobby disconnects a peer when a required control
message cannot be queued. Disconnect removes queued packets for that link.

`demo.c` drives one host and seven clients through the mock transport and verifies each roster and START transition.
It then uses those sessions to check snapshot encoding and sequencing in process, including a stale duplicate.

Current deterministic result:

- 8 Links;
- 10 ticks;
- 560 accepted snapshot deliveries (`8 * 7 * 10`); and
- 1 intentionally duplicated state packet rejected as stale.

The separate `tests/test_multiplayer_webrtc.c` probe drives one host and one client over three negotiated WebRTC
data channels through the same `SwmpTransport` interface. Its trusted in-process offer/answer exchange binds
the SDP certificate fingerprints to the endpoints. The fixed invite and link binding are test inputs. The probe
verifies the channel configurations, compatible and incompatible lobby admission, START, binary delivery, and
disconnect. It depends on an externally built libdatachannel and is built only on demand.

## Draft native boundary

`native_abi_draft.h` provides a versioned C query and lifecycle tables. `test-multiplayer-native` builds
a project-owned test library and an independent host executable. The fixture covers negotiated capabilities,
initialization, poll, reset, shutdown, and unload. See [NATIVE_ABI.md](NATIVE_ABI.md) for ownership and the SWMM
approval proposal.

## SWMM package source

Version: `0.1.0-dev`.

The package builder creates exactly:

```text
info.json
romfs/System/SavageWind/Multiplayer/package.swmp
```

The marker contains package format v1, ID `org.savagewind.multiplayer`, protocol 1, and `runtime = native`. The
archive contains only the marker and its metadata. The builder normalizes source line endings and uses stored
entries, fixed timestamps and permissions. The checker also rejects duplicate and oversized entries.

Current generated artifact:

```text
build/mods/SavageWind-Multiplayer-0.1.0-dev-SWMM.zip
SHA-256 efbed5e1322bb4b8de344cd76d30cb073ff87dabfc134878b2100f6c968c2eff
```

Build artifacts are not source and should not be committed unless release policy explicitly requires them.

