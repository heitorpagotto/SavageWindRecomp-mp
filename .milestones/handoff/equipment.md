# Melee weapons and equipment groundwork

## M2.5 held materials and item releases

- Sample the five world CarryBox indices in the material manager. Each
  instance keeps its model and interpolated world transform through
  carry, placement, and toss. The inventory preview uses its own actors.
- The remote Link and his armor use native material hold clips and blend
  them with walk or run playback. The native `GrabPut` state uses
  a 30-frame track for the far release.
- Normal inventory discard uses the completed native spawn request.
  Equipped discard and weapon throws track the released equipment
  actor. Registered aliases reuse installed model lists with passive
  actor behavior. Shared item pickup and combat effects remain future work.
- Protocol 10 carries up to 12 item instances per owner in pose snapshots.
  The receiver culls items by their own positions and retires them
  when the owner leaves or either side changes areas.
  [Test evidence](../testing/equipment.md#m25-held-materials-and-item-releases)

## M2.5 grounded weapon throws

- Sword, large-sword, and spear throws use their native wind-up, held aim,
  and release clips. Native `WeaponThrowCharge` uses a 36-frame aim track.
  The sender retains the starting melee family through `WeaponThrow`
  because the held actor detaches before the release finishes.
- One-handed and Giant Boomerangs catch in `WeaponThrowCatch` on layer 1,
  slot 1 over `WaitAttention`. The receiver selects the catch clip by
  melee family. Normal equipment revisions remove the held model at
  release and restore it on catch. Revisions and the snapshot format
  still use protocol 9.
- This covers Link and the held model during grounded throws. Projectile
  flight, impacts, and moving or airborne variants remain open.
  [Test evidence](../testing/equipment.md#m25-grounded-weapon-throws)

## M2.5 installed weapon catalog

- The previous build supported only four melee models, two shields, and two
  bows. The runtime now builds passive aliases from the installed ActorInfo and
  actor packs for the five numbered equipment families. It resolves aliases
  by the native family and variant number on each peer.
- Protocol 8 sends a 16-bit identity per equipped slot. Existing position
  snapshots still carry each item's relative transform and equipment revision.
  Both peers need the matching build.
- Melee variants reuse their one-handed, two-handed, or spear action map.
  Each item uses its own model list, and bows keep their native animation
  sets, including the two bows with custom lists. The three numbered actor
  records without models have no passive visual.
  [Verification](../testing/equipment.md#m25-installed-weapon-catalog)

## M2.5 weapon families

- Resume review confirmed that the previous checkpoint recognizes the Woodcutter's Axe
  as its only melee identity and gates the large-sword idle and draw clips on the axe's
  measured attachment height. The installed 1.9.0 data has the Sword, Spear, and Lsword
  `001` actor packs. The first weapon slice probes these three alongside the axe before
  widening the action map.
- Added stable identities and passive alias entries for Traveler's Sword, Traveler's Spear
  and Traveler's Claymore. The three share the existing weapon slot, revision and pose layout.
  The equipment codec test covers each identity and rejects it in the shield slot.
- Windows core, direct invite and WebRTC loopback targets passed. The local
  `build/m25_weapon_resources.py` check verified that all alias entries can coexist, stock
  ActorInfo records are preserved, the three new packs use the passive profile, and all
  70 proxy states reference installed clips.
- The proxy action map now separates one-handed sword, spear, and large-sword clips. The
  axe and claymore share the large-sword actions. The sword and spear have separate draw,
  ready, stow and attack-chain clips.
- The native Windows trace in `build/m25-weapons-jtk2dnpd/` captured all grounded chain
  steps and charged releases. The installed controller uses the same spear clip for hits
  two through four. The proxy gives each hit a distinct action so each can start playback.
  Charge preparation and release are separate actions for all three weapon families.
  The spear rests about 0.43 m above Link's root when stowed and 0.58–0.73 m when drawn.
  The sword and large weapons use a higher stowed attachment.
- The Windows app linked with source ID `d8f1cbe57e11d234264704b43eeae942dceb6140`
  and uses the existing 1.9.0 gamelib `13365c313591b27e`. The updated core test passed
  a simulated lobby weapon revision change with the shield and bow preserved. Direct invite
  and WebRTC loopback tests passed again.
- The receiving-game run in `build/m25-weapon-live-pcclk2vu/` confirmed native sword
  equip replaced the remote axe on the same Link. The client selected draw, ready, stow,
  all four normal attacks, charge and return to ready at 30 FPS. Draw and ready also
  passed at 60 FPS.
- The same run confirmed spear equip replaced the remote sword. All five thrusts,
  charged rush and finish, draw, ready and stow passed at 30 FPS. Draw and ready also
  passed at 90 FPS. A client capture shows the drawn spear on the remote Link.
- Claymore equip replaced the spear and reused the large-sword draw, ready, stow,
  normal chain and charged spin. The client selected every expected action at 30 FPS,
  and draw and ready passed at 120 FPS. The client capture shows the drawn claymore.
- Client Leave retired the claymore. A same-slot rejoin restored it from the current
  equipment record. All three weapon swaps preserved the original Link proxy. The launcher
  was `build/m25-build/Play-M25.cmd`. The scratch run used the ignored
  `build/m25_weapon_live.py` and `build/m25_verify_weapons.py` against that checkpoint. See the
  [test record](../testing/equipment.md#m25-weapon-families) for the full settings and evidence.

## M2.5 passive axe and animation

- The initial axe checks used the local 1.9.0 gamelib `13365c313591b27e` and source ID
  `76b08c288fd4ca87a334826f624e3a5b03ed711e`.
- The Woodcutter's Axe uses a passive local alias and its own actor index for each remote Link. A
  reliable equipment revision on the control channel gates the relative transform in 20 Hz snapshots.
  The native weapon getter only samples the local player. The host replays current records to newcomers
  after WELCOME and clears the record on departure before the slot can be reused.
- The proxy plays landing, axe draw, first swing, finishing swing and spin from installed clips. The native
  `Wait` state can cover both stowed and drawn idle. This first slice used generic idle for it, so the
  drawn axe can render beside a hand in a neutral pose. Find a native draw-state signal before adding a
  drawn idle animation. Clip phase and layered native blends are also open.
- The [test record](../testing/equipment.md#m25-passive-axe-and-animation) lists the native unequip, re-equip, travel,
  Leave and same-slot rejoin checks. The ignored `build/m25_revision_probe.py` builds the isolated clipboard
  and animation trace fixture. `build/m25_run_revision.py` drives the game over local control using scratch
  saves. At the inventory pause the runner reads `inventory-input` inside the new scratch directory.
  Each line starts with `host` or `client` followed by a control command, or `sleep` with seconds.

## M2.5 equipment probe

- The 1.9.0 `g_site.player_equipment` getter exposed the equipped Woodcutter's Axe in category 0 in both
  Windows games. The equipped actor's matrix at `+0x398` follows its placement. At the same idle
  position, the host's sampled height above Link changed from about 1.43 m stowed to 0.83 m drawn and
  its basis changed as well. The scope is a rigid weapon. Hair and armor still need a skinned-model path.
- A separate local actor created through the existing presence request, taken into an owned actor index
  and moved through the matrix setter rendered beside the remote Link. This was a stock weapon actor
  in scratch only. The drawn capture shows a Take prompt, so production needs a passive visual actor
  with its own indexed lifecycle.
- The ignored `build/m25_equipment_probe.py`, `build/m25_build_probe.py --equipment`, and
  `build/m25_run_equipment.py` build and run the scratch probe. It uses one known installed weapon
  and expresses its transform in Link's yaw frame. Its temporary encoding borrows unused position
  fields. Keep that experiment out of the production wire format. The local placement run is in
  `build/m25-equipment-md8xh7xs/`. The direct WebRTC run is in `build/m25-equipment-wire-e2yv1mv9/`.
  Only the host drew their axe. The client applied the remote change from 1.43 m to 0.83 m while
  its own axe stayed stowed. The client captures `build/equipment-stowed.png` and
  `build/equipment-drawn.png` show the pose change. The scratch actor also lacks per-slot cleanup.
- The probe informed the passive alias, independent actor index and separate identity and transform
  messages in the current slice.
