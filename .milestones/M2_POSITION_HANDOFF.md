# M2 Plateau position, animation and health handoff

M1 was accepted on 2026-11-22 after Windows and Linux clients joined a Linux host. Hosting from the
owner's network still fails and remains a separate investigation. START stays available. The Plateau
position slice of M2 works in the open lobby and after START. The Plateau proxy now maps the local
player's idle, movement and jump states to native clips. M2 remains partial.

For the current shrine identity and range tests, see the [latest handoff](HANDOFF.md#m2-shrine-identity-and-native-actor-retry).
The [visibility follow-up](HANDOFF.md#m2-visibility-follow-up) records the earlier Slate travel tests.

## Implementation

- The guest hook samples the game-published XYZ in PlayerInfo. Facing comes from the local actor's
  matrix at +0x398 after checking that its translation matches the player's published position.
  Title and loading screens, stale samples,
  incompatible coordinates and departures from the small Plateau test area withdraw the model. The
  window thread owns transport. The host sends up to 20 full position batches per second with a reliable
  roster epoch to prevent a stale batch from moving a peer that reused a slot.
- The virtual base RomFS installs a dedicated passive actor alias from the local game data. Its model
  list selects the installed base Link model from TitleBG, and its PhysicsUser is Dummy. The metadata
  writer keeps the BYML string table and actor CRC order valid and preserves the existing records.
  The original files are read only. A developer overlay for the actor table or template and a missing
  resource fall back to the legacy NPC placeholder.
- The 1.9.0 `mp_presence_move` site resolves the native actor matrix setter. Call it on the game thread
  while holding the validated link for an owned proxy index. The proxy interpolates over one 20 Hz send
  interval and snaps large corrections. Generation changes, leave and reboot clear owned references.
- The virtual actor pack adds its own AS list and three single-clip definitions from the installed
  Player_Animation archive. The proxy has no AI or physics resource. The guest update reads layer
  zero, slot zero of local Link's native animation controller and sends a stable action ID in the
  existing snapshot field. The receiver bounds that ID and selects a local state name. It calls
  the native playback function on the retained proxy's controller when the state changes. Movement
  and facing still use the matrix setter. The optional animation sites resolve as a group and a
  signature failure leaves the position path available.
- The supported actions cover idle, locomotion with a walk clip and jumping. Facing turns use the
  existing interpolation. Default hair, armor and weapons remain open. Keep local Link's
  singleton, camera, input and inventory independent of remote actors.
- Read-only health uses PlayerInfo's native quarter-heart getter. Fresh values travel with the
  snapshots and appear beside peer names in the F1 multiplayer tab. An unknown value omits the
  count. Peer snapshots never change either player's native life.

## Verification

- Regenerated BOTW 1.9.0 game library `a1c9d2072a418844` at x86-64-v3. Windows UCRT64 runtime
  and full build passed. The game-site (50) and layout (59) checks passed. The multiplayer core,
  eight-peer WebRTC, and eight-peer direct position relay tests passed. The runtime, proxy assets,
  presence and RomFS objects cross-compiled for Linux x86-64. Native Linux and mixed-platform gameplay
  are pending. An offline alias test compared all stock metadata records with their originals.
- Two Windows scratch games using the actual direct WebRTC path both visibly rendered the base remote Link.
  The receiver's capture showed a remote walk and jump while each slot kept its original active proxy.
  Leave removed both proxies, a new invite reused the client slot successfully, and host reboot removed
  the surviving client's proxy and rebuilt the host's virtual RomFS. This used BOTW 1.9.0 with a 30 FPS
  target, 960x540 at 50 percent render scale, a 1400 MiB VRAM budget and Vulkan heap copy off, on the
  NVIDIA RTX 4050 Laptop GPU (616.92) and AMD Radeon Graphics (25.10.30.02).
- Windows scratch evidence is under ignored `build/m2-live-nv1jx2av/`. The local `build/m2-probe/run_live.py`
  runner uses an isolated clipboard and separate settings/saves. It expects the scratch build from
  `build/m2-probe/build_live.py` and the matching generated `build/m1/swgame.dll`. Production code
  has no automation hooks.
- `build/m2-probe/run_facing.py` and `build/m2-facing-8wcx5jxf/` cover turns from both clients,
  independent inventory opening, and leaving and returning to the test bounds. The two-instance
  hardware reached about 65 and 83 FPS at 90 and 120 FPS targets, respectively. Native Linux gameplay
  and sustained 90/120 FPS verification are open.
- `build/m2-probe/build_anim_live.py` and `build/m2-probe/run_anim_matrix.py` checked native animation
  in both directions and repeated the inventory, area and lifecycle checks with regenerated gamelib
  `34aa482dae306a0d`. A separate `run_anim_capacity.py` fixture rendered all seven remote Link actors
  in one game through START and disconnect. Both used the actual direct WebRTC flow. See the
  [animation test record](TESTING.md#m2-plateau-animation-check) for settings and results.
- `build/m2-probe/build_health_live.py` and `build/m2-probe/run_health_live.py` tested health
  observation with gamelib `e640fd858dc0d033`. The installed-data regeneration is in ignored
  `build/m2-health-generated/`, and the linked app and library are in `build/m2-health-build/`.
  The Windows link used a Clang response file for the generated objects because the expanded command
  exceeded the process command-line limit. The Makefile wrote the ID after linking.

## Resume

1. For M2.5, add default appearance pieces and reliable equipment revisions. Use
   the existing read-only equipped-actor query in `runtime/quick_drop_action.c` as a reference for
   equipped weapons. Resolve appearance resources from installed data and apply them to an owned
   proxy. `Armor_Default_Head` selects its head unit from the `Armor_Default` model archive in
   TitleBG. The scratch `build/m2-probe/build_hair_probe.py` experiment added a second model
   entry to the proxy pack. The proxy failed to activate. The 8 MiB instance-heap retry also
   submitted creation requests that failed before activation. Its log is under ignored
   `build/m2-hair-budget-a5ndp610/`. Start by isolating the native model load and attachment path
   with one equipped piece. Verify individual inventory and local control.
2. For M2.5, extend the action map to cover the full set of player animations. Preserve the
   indexed actor lifecycle, transform and snapshot rate.
3. Complete extended sessions with eight independently controlled game clients, sustained 90 and
   120 FPS, interiors, travel, live loss/jitter and native Windows/Linux gameplay. Review the
   [milestone ledger](MILESTONES.md) and
   [direct multiplayer handoff](DIRECT_MULTIPLAYER_HANDOFF.md) before marking M2 accepted.
