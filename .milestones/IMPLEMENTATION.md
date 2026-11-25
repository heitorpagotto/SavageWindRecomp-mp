# Implemented foundation

## Status

Windows and Linux source builds link the WebRTC lobby and the direct invite and reply exchange. F1 and the
game's Options share the default-off `multiplayer` INI setting and a copied lobby view. Hosting and joining
require a supported 1.9.0 gamelib with the presence and actor lifecycle sites.

## Protocol constants

Defined in `mods/multiplayer/protocol.h`:

- `SWMP_PROTOCOL_VERSION = 4`
- `SWMP_MAX_PEERS = 8`
- `SWMP_HOST_PEER = 0`
- `SWMP_UNASSIGNED_PEER = 255`
- `SWMP_PACKET_HEADER_SIZE = 32`
- `SWMP_PLAYER_SNAPSHOT_SIZE = 96`
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
| Control | 0 | HELLO 1, WELCOME 2, REJECT 3, PEER_JOINED 4, PEER_LEFT 5, START 6, PING 7, PONG 8, EQUIPMENT 17 |
| State | 1 | PLAYER_SNAPSHOT 16 |
| Event | 2 | DAMAGE_REQUEST 32, DAMAGE_APPLIED 33, ACTOR_HEALTH 34, FLAG_TRANSACTION 48, CHEST_REWARD 49, WORLD_PHASE 50, RESYNC_REQUEST 64, RESYNC_SNAPSHOT 65 |

The decoder rejects bad magic, unsupported version, unknown type/flags/channel/peer, nonzero reserved bits, oversized
payload, wrong message channel, and packet-size mismatch. An unassigned sender is accepted only for HELLO. The
lobby currently sends and accepts control packets with zero flags.

## Player snapshot layout

| Offset | Size | Field |
|---:|---:|---|
| 0 | 1 | peer ID |
| 1 | 1 | grounded/gliding/crouching/guarding/dead/weapon pose/shield pose/bow pose flags |
| 2 | 2 | action ID |
| 4 | 4 | area ID |
| 8 | 4 | phase ID |
| 12 | 12 | X/Y/Z position in millimetres, signed 32-bit each |
| 24 | 6 | three signed 16-bit turn fractions |
| 30 | 6 | X/Y/Z velocity in centimetres/second, signed 16-bit each |
| 36 | 2 | health in quarter units |
| 38 | 2 | stamina in quarter units |
| 40 | 4 | equipment revision |
| 44 | 6 | weapon X/Y/Z offset relative to Link's yaw, signed 16-bit millimetres |
| 50 | 6 | weapon pitch/yaw/roll relative to Link's yaw, signed 16-bit turn fractions |
| 56 | 6 | shield X/Y/Z offset relative to Link's yaw, signed 16-bit millimetres |
| 62 | 6 | shield pitch/yaw/roll relative to Link's yaw, signed 16-bit turn fractions |
| 68 | 6 | bow X/Y/Z offset relative to Link's yaw, signed 16-bit millimetres |
| 74 | 6 | bow pitch/yaw/roll relative to Link's yaw, signed 16-bit turn fractions |
| 80 | 1 | nocked arrow identity, zero for absent or 1–6 for Normal, Fire, Ice, Shock, Bomb, Ancient |
| 81 | 1 | bow animation state, zero for unavailable or 1–6 for Hold, Equiped, Draw, Shoot, Null, SelfCameraPause |
| 82 | 2 | reserved, must be zero |
| 84 | 6 | nocked arrow X/Y/Z offset relative to Link's yaw, signed 16-bit millimetres |
| 90 | 6 | nocked arrow pitch/yaw/roll relative to Link's yaw, signed 16-bit turn fractions |

Each gear-pose bit requires a nonzero revision and limits each offset axis to ±3000 mm. The receiver pairs the
pose with the reliable record for the same peer and revision. The nocked arrow and bow animation state require a
bow pose. The arrow shares the offset bound. The direct invite text format is independent of the packet version.
Lobby traffic uses protocol 4 and admission requires matching source fingerprints.

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
The virtual RomFS builds an alias using the installed model and supplies idle, jump, walk, sprint and
run clips from Player_Animation. The action field uses 0 through 4 in that order. Unknown action
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

## M2.5 passive gear and expanded actions

The 1.9.0 guest thread uses the native equipment getter to sample the Woodcutter's Axe and Traveler's
Sword, Spear and Claymore in category zero, Wooden and Soldier's Shields in category one, and
Traveler's and Soldier's Bows in category two. The window thread advances the equipment
revision when any identity changes. EQUIPMENT carries a one-byte peer, a four-byte revision, and three
identity bytes in weapon, shield and bow order. The host accepts updates for the connection's own peer,
relays them, and sends cached revisions after WELCOME. Identity and membership
share the ordered control channel. Peer removal clears the cached identity and position before slot reuse.

The virtual RomFS gives each gear variant a passive prop profile with dummy physics and a model list from
installed resources. A matching gear pose and an active Link proxy let the guest thread request one visual
actor per slot and peer. Each visual has its own native request and index, follows the sampled matrix
relative to the Link's interpolated pose, and retires on unequip or loss of presence. A revision change
only replaces the visuals whose identities changed. Sampling uses read-only access to local equipment.

Landing uses action ID 5. IDs 6–11 select large-sword draw, first swing, finishing swing, spin, drawn idle
and stow for the axe and claymore. IDs 49–69 add the sword and spear actions and the charge follow-ups
for all three families. Native CutS3 and CutS4 reuse the same spear clip as CutS2, with separate action
IDs to start each thrust independently. The measured hand-height band for each supported weapon detects
drawn idle and gates the shared WeaponEquipOn state. The receiver plays a representative clip for
each native action. IDs 12 and 13 add
crouch idle and movement. ID 14 selects shield guard when the native WaitAttention state accompanies
a forward shield pose. IDs 15–17 use the local upper-body state at layer 1, slot 1 to select reload,
aim and release for both bows. IDs 70–72 map native parry, guard hit and guard break when a supported
shield is equipped. Native shield destruction can precede GuardBreak by an update. The sender
uses shield presence in the previous two snapshot intervals to identify the start of the reaction,
keeps the action for the duration of GuardBreak, and publishes shield removal normally.
IDs 73 and 74 cover grounded bow draw and stow. The sender uses the bow's position beside Link
to distinguish the shared WeaponEquipOn state and the stow transition through WaitAttentionUpper.
ID 75 selects the held-bow idle when the bow sits low and in front of Link in WaitAttentionUpper.
During BowReload or BowAim, the 1.9.0 sender samples the attached arrow actor's identity
and matrix from the equipped bow. It uses the same yaw-relative format as gear. Each of the
six inventory arrow types has a passive alias backed by its installed TitleBG pack member.
One indexed arrow visual per peer follows the current bow revision during reload or aim
and retires when that state ends. Routine arrow retirement allows a new request after one
snapshot interval. Actor request failures and peer retirement retain the one-second backoff.
Both passive bows use the installed bow animation list and six state definitions. The sender
samples the equipped bow's own controller. The receiver selects that state on the passive bow
when it activates or the state changes, using the matching equipment revision. The bow retains
Draw through aim and completes Shoot while Link changes stance. The transform and animation
travel in the same position snapshot.
The receiver plays one clip per action change. Standing ShieldGuard transitions to and from Idle
or SwordReady blend over four native animation frames. The first playback float controls the
blend, and the second controls the starting phase. Parry and hit reactions use the engine default.
Exact clip
progress, blends and repeated activations of the same state remain open.

Action IDs 76–85 pair takeoff and landing clips for the native Step and StepLand states.
They cover neutral, left, right, forward and backward motion. The sender captures Link's
facing at takeoff and projects the first planar displacement onto that frame. It locks
the chosen direction for that dodge and reuses it for landing. A stationary takeoff
selects neutral after one snapshot interval. The native BackJump and BackJumpLand states
have dedicated backflip clips at IDs 86 and 87. These actions use the existing snapshot.

Action IDs 88–91 select frontal clips for the native DamageS, DamageM, DamageL and
DamageLStand full-body states. They cover light and medium hits, knockdown and getting
up. IDs 92–93 cover SlippingDown and SlippingDownLand, which can occur between a
knockdown and stand-up. IDs 94–95 select the hard landing and stand-up clips for
LandDamage and LandDamageStand. The ordinary Land state still maps to its own landing
clip. Full-body reactions take precedence over the bow's upper-body state. The sender
continues to sample native health independently. Damage authority remains in M3.

Action IDs 18–23 select treading water, forward swimming, swim dash, hanging on a wall, climbing
and climbing jumps from the native full-body states. During native ClimbMove, the sender projects
the sampled displacement onto the wall plane in Link's yaw frame. The nearest of eight directions
selects the movement clip. ID 22 is the upward clip and IDs 24–30 cover the other seven directions.
IDs 31 and 32 cover the wall approach and pulling over a ledge. ID 23 remains the upward climbing
jump. IDs 36–39 add right, up-right, up-left and left. During native ClimbJump, the sender waits
for at least 6 cm of displacement in the wall plane or falls back to the upward clip after about
100 ms. It keeps the selected action for that jump so turning, release and landing motion do
not restart the clip. The new action values fit in the existing snapshot.
Applying the standalone swim clips at the sampled root left the model submerged. The receiver lifts
the Link model for all swim clips while gear and range continue to use the sampled root.

Native SwimMove blends straight and turning strokes at different speeds. ID 19 remains the slow
straight stroke. IDs 40–44 add the fast straight stroke, slow left and right curves, and fast left
and right curves. The sender measures planar speed and wrapped heading change over about 100 ms.
It enters the fast tier at 1.30 m/s and leaves it below 1.15 m/s. Turns enter at 30 degrees/s and
leave below 16 degrees/s in the current direction. Native SwimWait and SwimDash retain their
dedicated clips. The receiver uses one representative clip per action and the same 1.65 m model
lift for every swim action.

Native SwimAtnMove covers movement while targeting. IDs 45–48 select forward, back, left and
right from the swim lock-on clips. The sender projects recent planar displacement onto Link's
current forward and right axes, then selects the dominant axis and current sign. It keeps the
previous axis near diagonals. Stationary swimmers and rejected motion samples use SwimWait.
These actions share the existing snapshot and waterline lift.

Action IDs 33–35 select paraglider deployment, neutral glide and falling. The existing gliding flag
is set only during native ParashawlGlide. The receiver requires this flag and the glide action to
request a separate canopy visual, so the equipment revision and wire layout stay unchanged.
The virtual actor pack reuses the installed canopy model and its Open animation. The receiver
retires that actor when flight ends or when the remote Link retires.

During flight, the sender estimates bank from the change in Link's native root heading over
at least one snapshot interval. The native root stays upright. The estimate wraps heading at
the signed turn boundary, rejects stale or discontinuous motion, and is bounded to 18 degrees.
It travels in the existing snapshot roll field. The receiver uses roll during active gliding only
and interpolates it with the root pose. Link keeps the neutral flight clip. The canopy shares
Link's yaw and roll and sits 2 m along the tilted local up axis. Rigid equipment keeps its own
native pose in Link's yaw frame.

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

The build normalizes line endings before hashing the source for the network identity. The platform-specific
gamelib ID is used locally to validate the guest library.

## M2.5 environment and lobby notices

The host can prefix a complete position batch with a TimeMgr clock sample and a weather type. The receiver
validates both fields, requires a host pose and uses that pose's area ID. This shares sequencing and the
reliable roster epoch with position, so a late join or packet loss recovers on the next full batch. Clients
send plain position snapshots and accept environment samples only from the authenticated host link.

On the guest update, a client with the default-on setting applies a fresh sample in the same area, outside
loads and events. It updates both native WM_Time buffers as needed and lets TimeMgr advance between samples.
The 1.9.0 weather getter returns the effective weather for the active climate. The client feeds that type
into the existing world manager weather override and the local weather setting resumes when sync stops.
Both F1 and System > Options use the persisted setting.

The window thread queues the admitted name on successful WELCOME send or receipt of PEER_JOINED. The
render thread shows the notice for five seconds, including with the menu closed. A joiner's initial roster
populates through WELCOME without notices for the already present peers or for the joiner.
The host and existing clients use the departed peer's roster name when a connection closes or PEER_LEFT
arrives. A transition from an admitted session to offline also posts a local notice. Disabling multiplayer
clears the session immediately and keeps the disconnect notice visible until it expires.
