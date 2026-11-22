# M2 position and remote Link handoff

M1 was accepted on 2026-11-22 after Windows and Linux clients joined a Linux host. Hosting from the
owner's network still fails and remains a separate investigation. START stays available. The Plateau
position slice of M2 works in the open lobby and after START. M2 remains partial.

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
- The proxy renders in the base model's bind pose. Default hair and other appearance pieces,
  native animation, health and equipment are later steps. Keep local Link's singleton, camera,
  input and inventory independent of remote actors.

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

## Resume

1. Work out the default idle pose and animated turns, walks and jumps using the installed resources
   and a separately owned model. The local PauseMenuPlayer pack shows an animation list that selects
   Player_Animation and its normal wait clip. It is a reference for the native animation data path.
2. Keep the transform and creation path stable while adding appearance pieces, animation, health and
   reliable equipment revisions. Verify individual inventory and local control.
3. Complete the extended session and frame-rate matrix, eight visible world peers, interiors, travel,
   loss/jitter and native Windows/Linux gameplay. Review the [milestone ledger](MILESTONES.md) and
   [direct multiplayer handoff](DIRECT_MULTIPLAYER_HANDOFF.md) before marking M2 accepted.
