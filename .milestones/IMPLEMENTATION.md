# Implemented foundation

## Status

Windows and Linux source builds link the WebRTC lobby and the direct invite and reply exchange. F1 and the
game's Options share the default-off `multiplayer` INI setting and a copied lobby view. Hosting and joining
require a supported 1.9.0 gamelib with the presence and actor lifecycle sites.

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
- Each admitted remote slot has a local generation. Reusing the same slot advances its generation even if both
  roster events arrive in a single poll and the display name is unchanged.

Public API is in `mods/multiplayer/session.h`. Core calls are expected to remain on one multiplayer/game thread; the
struct is not internally synchronized.

## M1 guest presence

The window thread publishes the admitted remote roster after each lobby poll. `swmp_m1_roster()` copies it under the
same mutex as the render view. The snapshot combines the local lobby generation with each remote slot's generation.
The lobby generation changes across local leave and rejoin, even when the host's session ID remains the same.
Membership versions are local, with no wire schema change.

The native menu update hook runs on the guest thread. `MP_PRESENCE_SITES` in
[`tools/game_sites.py`](../tools/game_sites.py) requires every signature. The generated hook preserves the native
frame stores. The runtime reconciles the roster when the spawn context and the local player's position in the
Plateau test area are available. It requests `Npc_TripMaster_00` at up to seven fixed placements and excludes
the local peer. The test NPC uses its ordinary idle and interaction behavior. The
[presence code](../runtime/multiplayer_presence.c) owns each slot's lifecycle.

1. Submit an asynchronous native request using the local actor name and transform. The game owns the request
   until completion, so its storage stays at a fixed guest address for the boot.
2. Take the completed result to clear creation suspension. Retain the actor's native index and resolve it into
   a temporary ordinary link when checking whether the actor is active or requesting deletion.
3. Cancel pending requests or queue native deletion for retained actors when a peer leaves, a slot is reused,
   the lobby generation changes, or the spawn context or test position becomes unavailable. Release the
   temporary link, index, and request as applicable. At reboot, discard the per-boot state after guest threads stop.

The slot gives creation and activation up to ten seconds. Retirement puts the slot on a one-second cooldown before
another request can reuse that placement. The `placeholder removed` log marks retirement on the runtime side.
Native deletion finishes asynchronously, so use a rendered frame to confirm the actor disappeared.

## M2 remote Link proxy

The receiver combines the admitted roster and 20 Hz position batches to drive a passive Link actor.
The virtual RomFS builds an alias using the installed model and supplies idle, jump, walk, run and
sprint clips from Player_Animation. The action field uses 0 through 4 in that order. Unknown action
IDs use idle. The guest thread samples local Link's native
full-body state and selects a clip on each proxy's own animation controller while it holds the
actor link. Playback starts on activation and on action changes. The native clock advances the
clip while the interpolator applies position and facing. The interpolator blends over one send
interval, snaps across corrections above 3 m, and holds the latest pose until the next sample or
expiry. A new slot generation starts with a fresh controller and the latest snapshot.

The visibility follow-up accepts fresh positions on the main field and in individual shrine scenes.
The RomFS stage tracker maps the field to area ID 1 and a shrine to 2 plus its pack number. IDs are
independent of the generated display-name table. Title, loading and unsupported scenes withdraw the pose.
Peers must share the same area ID. A changed local area, actor or spawn context retires owned requests and
proxies independently of lobby membership. The spawn radius is 50 m in three dimensions, with retirement
beyond 60 m. This keeps proxy lifetime within the distance sustained in the Windows range probe.
The native Move state is mapped to walk, run and sprint using horizontal speed sampled over roughly
100 ms. Each action uses its own installed clip.

The separate native life observation reads current quarter-hearts from PlayerInfo. The window
thread copies fresh health to the F1 roster. Zero life uses the snapshot's dead flag to distinguish
it from missing health. Damage authority and death handling belong to M3.

## M2 multiplayer release version

Windows and Linux append `-<multiplayer version>-mp` to the upstream `SAVAGEWIND_VERSION` in the runtime,
Configuration and package names. Bump [the multiplayer version file](../mods/multiplayer/VERSION) for a mod
release. `make -s print-version` prints the combined label. The lobby's protocol, compatibility fields and
source fingerprint continue to govern admission. The multiplayer fork is updated manually.

## Control payloads

`lobby_protocol.h` declares the payload types. Integers use network byte order. Text is a one-byte length followed
by UTF-8, with 1–31 bytes for a player name and 1–95 for a rejection. Decoding rejects embedded NUL, C0/C1
controls, malformed UTF-8, duplicate roster IDs, and trailing bytes.

| Message | Payload |
|---|---|
| HELLO | u16 protocol, two sets of three u16 version components (multiplayer then game), 20-byte Savage Wind build ID, 32-byte main module build ID, u32 supported features, u32 required features, 16-byte connection binding, name |
| WELCOME | Assigned client ID, playing flag (0/1), u32 feature intersection, repeated ID/name entries including host and assignee |
| REJECT | Reason code byte and diagnostic text |
| PEER_JOINED / PEER_LEFT | Client ID and a name for PEER_JOINED |
| START | Empty |
| PING / PONG | u64 token, echoed in PONG |

`lobby.c` owns control dispatch. HELLO uses an unassigned sender and zero session ID and sequence. The host matches
the binding, compares build/version fields and required features, then assigns a slot. The client adopts the host's
session ID and roster on WELCOME. The playing flag lets a late joiner enter the same lobby state. A five-second deadline bounds HELLO/WELCOME.
WebRTC allows 45 seconds before that for SDP signaling and ICE. An incompatible envelope version is
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

`webrtc.c` implements the three negotiated streams for `SwmpTransport`. It allocates a fresh connection handle
per link, bounds sends and pending connections, and closes the whole link on a channel failure.
`tests/test_multiplayer_webrtc.c` tests seven clients using trusted in-process SDP exchange.

`rendezvous.c` uses one nonblocking libcurl multi request at a time. The loopback Python service exposes
`/v1/room`, `/v1/join`, `/v1/offer`, and `/v1/answer`. A room uses a 90-second lease and a join a 55-second lease.
The host and each prospective client get separate random bearer credentials. The offer includes a 16-byte
per-exchange binding. The HTTPS service authenticates the exchange before WebRTC validates the DTLS fingerprints.
`test-multiplayer-rendezvous` exercises both components with a short-lived TLS certificate.

The native build uses direct.c to exchange versioned, bounded invite and reply messages on the clipboard.
The host generates a fresh binding and offer for each player. It accepts only a matching reply while that offer
is live. Preparing a new invite retires the previous outstanding offer. The earlier HTTPS design remains a
standalone transport fixture.

## Native migration

The compatibility check combines guest main-module identity with a platform-neutral source fingerprint. The
render thread submits a copied action and receives a copied view. The window thread owns the transport.
Leave, exit, and `RT_ON_RESET` release per-session resources.

Both menus send Host, Join, Start, and Leave through the same runtime. The window thread handles clipboard reads
and writes for both menus. Disable clears the published roster and pending command under the view mutex.
The window thread destroys the transport before processing another action, including after a rapid off/on toggle.
Guest support checks are independent of the persisted preference.

The wire version and fields are unchanged. The build normalizes line endings before hashing the source for the
network identity. The platform-specific gamelib ID is used locally to validate the guest library.
