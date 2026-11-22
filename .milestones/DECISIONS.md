# Architecture decision record

These decisions are active unless explicitly superseded. For an incompatible wire change, bump the protocol version
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

## D009 — Multiplayer is built into Savage Wind

Status: implemented in the Windows and Linux source builds on 2026-11-22.

Ship Windows and Linux/Vulkan builds of the fork with the feature compiled in. Expose one enable/disable setting,
Host, Join, and Leave in F1 Settings and the game's Options. The prototype's overlay activation gate is superseded.
Keep guest hooks in strict optional site groups and preserve single-player play with the feature disabled.

## D011 — Reuse installed game assets

Status: accepted.

Remote Links use the player's own existing Link and equipment assets at runtime. No new or extracted assets are
distributed.

## D012 — Transport backend for M1

Status: libdatachannel is selected for the native M1 source build. Release distribution review remains open.

The required properties are encrypted data channels, Windows/Linux support, ICE/STUN, optional TURN, reliable ordered
and unreliable unordered delivery, and an acceptable redistributable license.

- **libdatachannel** offers a C API over WebRTC data channels on Windows and Linux. Two reliable ordered streams
  and an unordered stream with zero retransmissions map to the existing channels. The Windows probe built v0.24.6
  with libjuice ICE and OpenSSL and exercised a two-peer lobby over DTLS/SCTP loopback. The test code uses the
  public API and the native application links the same backend. [C API](https://libdatachannel.org/pages/reference.html),
  [release](https://github.com/paullouisageneau/libdatachannel/releases/tag/v0.24.6)
- **GameNetworkingSockets** has encrypted UDP, reliable and unreliable delivery, and Windows/Linux build guidance.
  Its open-source P2P option uses an ICE dependency and custom signaling. The upstream material reviewed does
  not establish the self-hosted TURN fallback needed here. This comparison is based on source documentation.
  [Project and BSD-3-Clause license](https://github.com/ValveSoftware/GameNetworkingSockets),
  [P2P notes](https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/README_P2P.md)
- **libjuice alone** provides ICE/STUN/TURN over UDP. It would require adding DTLS and a reliable message layer;
  using it through libdatachannel avoids maintaining that combination here.
  [libjuice documentation](https://github.com/paullouisageneau/libjuice)

libdatachannel and libjuice use MPL-2.0. The tested configuration also includes usrsctp (BSD-style), plog (MIT),
and OpenSSL 3 (Apache-2.0). Review license notices and MPL covered-source obligations for the exact release
configuration before distribution. A production decision still needs Linux runtime and cross-platform tests,
forced TURN verification, an advisory review of the pinned dependencies, and release build size measurements.

The developer build uses libcurl HTTPS for the SDP exchange and libdatachannel for ICE/DTLS/SCTP. The pinned
libdatachannel WebSocket implementation disables TLS certificate verification on Windows. The service issues
a 120-bit invite and scopes each offer/answer to separate bearer credentials. The client's per-exchange nonce
provides the lobby binding. ICE gets an initial 45-second deadline before the separate five-second HELLO
deadline. Public service deployment, Linux execution, WAN, and TURN validation remain release gates.

## D013 — Connection-bound lobby admission

Status: accepted for the synthetic M1 lobby.

The transport supplies a unique opaque handle and a per-link binding. HELLO echoes the binding before the host
assigns a peer ID. Subsequent control messages must claim the ID assigned to that handle. Control messages that
change the roster or start play come from the host. A failed reliable lobby send disconnects that link so it cannot
continue with a divergent roster.

The mock binding is deterministic. The production transport must derive a binding from authenticated signaling or
its encrypted channel. Production transport selection remains open.

## D014 — Keep upstream release updates small

Status: accepted on 2026-11-22.

The owner manually integrates new Savage Wind releases. Keep the reusable network/session core independent of
upstream internals. Limit upstream changes to small build, settings, UI, lifecycle, and guest-site integration
points. Document the upstream release used and verify Windows and Linux builds and mixed-platform interoperability
after each update.

## D015 — M1 presence uses native creation and local generations

Status: implemented for BOTW 1.9.0.

The prototype used a static rock outside the Shrine of Resurrection to verify the presence lifecycle. The
native M1 build requests an NPC at the test placements. The request completes, taking the result clears
creation suspension, and an explicit native deletion call removes the active actor. Retain a native actor
index after taking the result so cleanup can resolve the owned actor.

Compare the local lobby generation and the remote slot generation when reconciling. The same display name
and peer ID can return between two guest polls. A generation change still retires the previous placeholder.
These generations stay local to the runtime. Gameplay replication will need its own wire identity and resync
contract. [Implementation](IMPLEMENTATION.md#m1-guest-presence)
