# Agent handoff

Checkpoint 2026-11-26 on `multiplayer/windows-test`, based on `256ab64`, with native
armor palette selection and body visibility for remote Link.
The M2 version label is `0.6.0-alpha.6-0.1.0-mp`. Read [AGENTS.md](../AGENTS.md) and
[CONTRIBUTING.md](../CONTRIBUTING.md) before making changes. Use RTK for supported shell commands and
separate scratch settings and saves for game tests. Keep generated code and game data local.

## M2.5 animation work order

The owner set the following order.

1. Weapons
2. Shield and bow
3. Traversal and reactions
4. Playback
5. Appearance (Validation on windows can happen here, on linux I will validate manually later and tell if there are any problems)

The grounded Weapons and Shield and bow slices are verified. The movement and shield report led to
the first Playback work after the fall-damage checkpoint. The owner then asked to render hair and
equipped armor before extending animation coverage. The
[animation backlog](NEXT.md#remaining-animation-work) describes the open work in each stage.

## M2.5 hair and equipped armor

- The 1.9.0 local player has retained actor indices for the head, upper body and lower body at
  offsets `0x23f0`, `0x2400` and `0x2410`. The guest thread resolves those indices temporarily.
  It sees the on-body actor, including a different Hylian Hood variant than the inventory name.
- The installed ActorInfo table supplies per-piece model identities and upgrade aliases.
  The original protocol 6 sent three model IDs in the reliable equipment revision.
  Each receiver looks up those IDs by slot in its own installed resources.
- A separate passive actor per slot plays Link's skeletal clips on the same root and starts at
  Link's current phase. The default head provides hair. Changing a piece preserves Link, the other
  armor pieces and rigid gear. Helmet variants, cloth simulation and DLC assets need further coverage.
- The [Windows test record](TESTING.md#m25-hair-and-equipped-armor) has the verified equip, removal
  and rejoin path. A second replay verified a mixed outfit of Climber's Bandanna, Ancient Cuirass
  and Stealth Tights on the same proxy. The current launcher is listed below.

## M2.5 armor colors and base skin clipping

- Protocol 7 carries a native dye index (0 through 15) per armor slot with the reliable
  equipment revision. The sender reads the equipped actor. The receiver passes the dye
  through the native placement parameters so the model loader binds the correct palette
  textures before the piece appears. A dye change reloads only the affected armor slot.
- The receiver uses the native `HideMaterialGroup` rules. It restores Link's grouped meshes
  and reapplies coverage from the active pieces when the worn models change. Unequipped,
  pending and unavailable pieces leave their corresponding base meshes visible.
- Run `build/m25-outfit-build/Play-M25-Outfit.cmd` on both Windows peers. It uses game version
  1.9.0, x86-64-v3 gamelib `baf44255c0bc3428`, multiplayer source ID
  `4afc4c9f6fea459a9a837809966d1ef3bd521b6f`. The previous appearance launcher uses
  protocol 6.
- The Windows core, direct invite and WebRTC loopback targets passed. The site and layout
  checks passed, regeneration resolved all 195 sites, and the installed-resource check
  verified both palette names in all 123 model packs and preserved the stock ActorInfo entries.
- The Windows replays used copied saves and Vulkan at 30 FPS in Tah Muhl Shrine. Both games
  used 1280x720, 75% render scale, a 1400 MiB VRAM budget and Vulkan heap copy off. The host
  used an RTX 4050 Laptop GPU (610.88) and the client used AMD Radeon Graphics (25.10.30.02).
- The visibility probe in `build/m25-outfit-check-vi7rb00n/` shows the Hylian Trousers
  covering the base leg meshes at idle and in motion. Removing the trousers restored those
  meshes while the remote Link, tunic, hood and axe stayed active. The later color probe
  showed that seeking the palette after creation could reuse the receiver's textures for
  the same model. The current build selects the palette during the native load.
- The mixed-outfit visibility probe in `build/m25-outfit-check-8m17funu/` kept the Climber's
  Bandanna, Ancient Cuirass and Stealth Tights visible through movement, a standing jump
  and rejoin.
- The current build's replay in `build/m25-outfit-check-msabxl_r/` shows the host's yellow
  Hylian Tunic beside the client's undyed tunic, on both screens under the same shrine
  lighting. The remote yellow tunic and the body mask survived movement, a jump, and rejoin.
  Equipping the undyed copy through the host inventory then changed the remote tunic to the
  default palette while the same Link and the other pieces stayed active. Compare
  `client/build/remote-rejoin.png` and `client/build/remote-dye-changed.png`.
- Rebuild the ignored `build/m25_build_outfit_native_fixture.py` and run
  `build/m25_run_outfit_check.py` with the matching gamelib to repeat the Windows checks.
  Linux validation remains on the owner's list.

## M2.5 remote movement and standing-jump playback

- The passive proxy played locomotion clips whose Root bone included forward translation. The
  sprint clip travels about 3.7 m in one cycle. The shield and axe used the networked root, so the
  body ran ahead of them and snapped back when the clip looped. A diagnostic sprint with Idle
  showed the same position updates with the gear beside the body.
- The 1.9.0 virtual RomFS now supplies an animation archive for the proxy with the planar Root
  curves removed in walk, run, sprint and crouch locomotion. The installed archive still serves
  native Link. The proxy archive retains vertical root motion and the remaining clip data.
- Protocol 5 adds source sample time, animation activation and phase to the snapshot. Both peers
  need the matching build. The receiver interpolates the body and compatible gear poses on the
  same delayed timeline. When an equipped item's pose is temporarily unavailable it carries the
  last valid relative offset with Link. Standing jumps use the tracked native frame to start
  the matching clip at the sampled phase.
- The matching Windows launcher is `build/m25-build/Play-M25.cmd`. See the
  [test record](TESTING.md#m25-remote-movement-and-standing-jump-playback) for coverage.

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
  [test record](TESTING.md#m25-weapon-families) for the full settings and evidence.

## M2.5 shield and bow follow-up

- Added the Soldier's Shield and Soldier's Bow as passive aliases in their existing
  equipment slots. Both bow variants share the standing reload, aim and release mapping.
  The equipment codec test checks the new identities and rejects swapped slots.
- Windows linked successfully. Core, direct invite and WebRTC loopback tests passed.
  The local resource check verified all eight equipment packs and confirmed that their
  alias entries coexist with unmodified stock ActorInfo records.
- The Windows sender trace in `build/m25-guard-bow-k28m4n5x/` confirmed that native
  Soldier's Shield equip activates the remote shield and that a parry reports full-body
  GuardJust between WaitAttention samples. Added shield-gated parry, hit and break clip
  mappings from the installed controller's GuardJust, GuardFailHit and GuardBreak states.
- The same trace found that grounded bow draw shares WeaponEquipOn with melee. After a
  shot, bow stow retained WaitAttention and WaitAttentionUpper while the bow crossed
  Link's left side. Added grounded draw and stow selection using the observed relative
  attachment and controller states.
- The first two-game run verified Soldier's Bow reload, aim and release at 30 FPS.
  Native remove and re-equip kept the axe and shield active. Leave and same-slot
  rejoin retired and restored all three visuals.
- The follow-up Windows build linked with source ID `9a531870ff22c2770f397f07c67f6bb2bd50e259`
  and the existing 1.9.0 gamelib `13365c313591b27e`. Core, direct invite and WebRTC loopback
  passed again, and the local resource check found installed clips for all 75 proxy states.
  This checkpoint used `build/m25_guard_bow_live.py` and `build/m25_run_guard_bow.py` with
  `SWMP_VERIFY_GUARD_BOW=1`.
- The follow-up in `build/m25-guard-bow-5x4vetaa/` verified that the receiver selected
  ShieldGuard, played two ShieldParry activations, returned to guard after each, and returned
  to idle when the host released ZL.
- The receiver also selected BowDraw, BowReload, BowAim, BowShoot and BowStow in the
  grounded route. The client captures show the Soldier's Shield and aimed Soldier's Bow.
- The retry in `build/m25-guard-bow-4b3vfoxa/` waited for native Wait after stow, then
  verified axe draw, ready and stow with the shield and bow equipped. It also verified bow
  remove/re-equip and retirement and restoration of all three visuals on same-slot rejoin.
- The installed Equip_Shield_On and Equip_Shield_Off clips belong to the shared weapon-equip
  states. The standing ZL trace entered and left WaitAttention without either equip state.
  Separate guard raise and lower playback needs transition evidence.
- The longer Windows fall in `build/m25-guard-bow-t9gnto0b/` reached native Fall over
  upper-slot BowReload, BowCharge and BowShoot. The player was still about 3.9 m above the
  starting point while aiming after the 6 m setup. The installed Fall state references
  Bow_Jump_Upper. The current proxy gives Fall precedence.
- The layered scratch run in `build/m25-guard-bow-9g15fuvx/` requested BowAim in layer 1,
  slot 1 over Fall. The receiver continued reporting Fall in the base slot with an empty
  upper slot during Fall and Land. The follow-up in `build/m25-sword-guard-en520aka/`
  confirmed the request string immediately before the native call and reproduced the empty
  upper slot. Layered aim needs a rig or animation graph that supports simultaneous playback.
  Build the ignored fixture with `build/m25_bow_layer_probe.py`, then set
  `SWMP_PROBE_APP=build/m25-build/bow-layer-probe/savagewind.exe` and `SWMP_AIR_BOW=1`
  for `build/m25_run_guard_bow.py`.
- The same follow-up verified that the Soldier's Shield and Traveler's Sword stayed active
  together through sword draw, sustained guard, parry and the sword attack chain while ZL
  was held. The ignored runner is `build/m25_run_sword_guard.py`.
- A synthetic action check in `build/m25-guard-bow-tests/actions.c` passed. It covered
  the shield requirement for guard hit/break and separation of bow and melee draw.
- The Soh Kofi Shrine encounter in `build/m25-guard-encounter-tna9m6wi/` verified
  native GuardFailHit after an enemy struck the raised Soldier's Shield. The receiver
  played ShieldGuard, ShieldHit, and ShieldGuard on the same Link. Chaas Qeta Shrine
  reproduced the hit in `build/m25-guard-encounter-drsxy9xe/` before the host died.
  The ignored `build/m25_run_guard_encounter.py` runner accepts `SWMP_GUARD_DUNGEON`.
- The frame-rate run in `build/m25-guard-bow-ffvz5v53/` verified BowDraw, BowAim,
  BowShoot and BowStow at 30, 60, 90 and 120 measured FPS on both games. The same route
  confirmed melee separation after each aim, followed by bow remove/re-equip, Leave
  and same-slot rejoin. Set `SWMP_BOW_FPS=1` with
  `SWMP_VERIFY_GUARD_BOW=1` to repeat it.

## M2.5 held bow and shield destruction

- The standing release trace showed the bow resting about 0.60 m above and 0.30 m in
  front of Link while the native controller reported WaitAttention and WaitAttentionUpper.
  The proxy selects the installed Bow_Lockon_Wait clip during that held pose.
- A close-range Chaas Qeta test with the Wooden Shield reproduced native GuardBreak
  in `build/m25-guard-break-vzld8rms/`. The shield actor disappeared on the same sampled
  update. The receiver retired the visual and missed the reaction because the action
  required a shield identity in the current sample.
- The one-update carry in `build/m25-guard-break-zh8konic/` still missed playback. Its
  trace showed a sample with the shield gone immediately before the controller entered
  GuardBreak. The sender now accepts shield presence within two snapshot intervals
  before the reaction and retains the action until the native state changes. It publishes
  shield removal normally and clears the history on actor, area, or lobby changes.
- The Windows app linked with source ID `e9928eba4b67d29f45947018ecbd17a57d018317` and
  gamelib `13365c313591b27e`. The action check distinguishes held, side and stowed bow poses,
  and the resource check found installed clips for all 76 proxy states. Core, direct invite
  and WebRTC loopback passed.
- The Chaas Qeta replay in `build/m25-guard-break-72upt2zg/` passed at 30 FPS. The native
  GuardBreak reached the same remote Link as ShieldBreak and returned to idle. The
  Wooden Shield retired exactly once, and the axe and Link actor remained active.
  Reproduce with `SWMP_VERIFY_GUARD_BREAK=1`, `SWMP_PROBE_APP=build/m25-build/bow-ready-live/savagewind.exe`,
  and `build/m25_run_guard_break.py`.
- The previous standing guard trace shows the shield moving across Link's side during
  the Wait to WaitAttention change. The forward offset crosses the 175 mm guard threshold
  four samples after WaitAttention begins. Release already starts moving the shield
  before Wait. The received rigid pose follows that movement. Distinct body transitions
  need a playback blend across the native state edge.
- The Tah Muhl replay in `build/m25-bow-ready-y5zwclgm/` verified BowReady after release
  and before native stow at 30, 60, 90 and 120 measured FPS. The receiver held BowReady for
  at least 0.65 seconds in each check. Draw, aim, release and stow also passed at every cap.
  The same session passed axe draw and stow, bow remove/re-equip, Leave and same-slot rejoin.
  Rebuild `build/m25_bow_ready_live.py` and run `build/m25_run_bow_ready.py` to repeat it.
  The matching Windows launcher is `build/m25-build/Play-M25.cmd`.

## M2.5 shield transition probe

- The installed player animation list has a CFDefines section. It describes source
  and target states with a frame count and starting fraction. The passive Link list
  currently has resource and clip definitions. The Windows scratch fixture
  `build/m25_guard_transition_probe.py` varies the first playback float across five
  native guard raise and lower cycles to test whether the existing call can blend
  transitions without additional animation-list entries.
- The five 30 FPS cycles in `build/m25-guard-transition-zrheifj4/` compared default,
  zero, 12, 60 and one frame. With 60, the proxy was still between idle and guard
  at the mid-transition capture in each direction. With zero, the same capture
  already showed the destination pose. Both paths reached the expected clip.
  The first float can therefore supply the blend directly.
- Standing Idle and SwordReady now crossfade to and from ShieldGuard over four
  animation frames. That covers the part of the sampled native attachment motion
  after the guard threshold crossing. The same engine call handles the blend,
  with the existing action and pose fields.
- The Windows app linked with source ID `6204b14ff79e7a921c8f3aa5cf5f2459d13405da`
  and existing 1.9.0 gamelib `13365c313591b27e`. Core, direct invite and WebRTC
  loopback tests passed. The 30 FPS probe's default argument also showed a blend
  at the early capture, so the four-frame setting makes the tested guard edges explicit.
- In `build/m25-guard-blend-kpq64rvp/`, the receiver used four frames in both
  directions at 30, 60, 90 and 120 FPS targets for idle, and at 30 FPS for
  SwordReady. Client captures show the progression between stance poses at 30 FPS.
  Parry and its return to guard used the default. One Link and one shield stayed
  active throughout. Rebuild `build/m25_guard_blend_live.py` and run
  `build/m25_run_guard_blend.py` to repeat it. The matching launcher is
  `build/m25-build/Play-M25.cmd`.
- The Chaas Qeta replay in `build/m25-guard-break-03uykit4/` verified ShieldHit
  and ShieldBreak with the engine default on the same build. The destroyed
  shield retired once, and the axe and Link stayed active.
- The installed NormalArrow actor pack is in TitleBG and its model list references
  Weapon_Arrow_001. The Soldier's Bow model contains String_L_1, String_L_2,
  String_R_1 and String_R_2. Those are the starting points for the arrow and
  bowstring visual probes.

## M2.5 nocked-arrow probe

- The current Windows scratch probe walks bounded, read-only references from Link
  and the equipped bow during reload, aim, release and post-release ready. It reports
  nearby arrow actors and their matrices so a passive visual can use the native pose.
  The probe uses `build/m25_arrow_probe.py` and the isolated `build/m25_run_guard_bow.py`
  route.
- The scratch run in `build/m25-guard-bow-khwv41j6/` found NormalArrow at bow actor
  offset 0x88 during BowReload and BowAim. Its actor matrix carries the held pose.
  BowShoot exposed a different placement and BowReady had no reachable arrow.
  The installed actor info and TitleBG agree on the Weapon_Arrow_001 model. The
  first visual slice will carry the held NormalArrow pose and stop on release.
- The runtime reads the 1.9.0 bow attachment during reload and aim. A new passive
  arrow alias uses the installed TitleBG member. Snapshot protocol 2 adds the
  transient arrow identity and bounded relative pose. The renderer requires the
  corresponding bow revision and a visible bow before activating the arrow.
- The Windows app linked with source ID `3f7f9da84ecd7ba679e60c5c03469d2f71d6922a`
  and existing gamelib `13365c313591b27e`. Core, direct invite and WebRTC loopback
  tests passed. The core test covers arrow pose round-trip, bounded offsets,
  reserved bytes, a missing bow pose and peer relay. The local resource check
  verified the passive arrow pack and preserved stock actor information.
- The first receiving-game route in `build/m25-nocked-arrow-jq6s5_sg/` verified
  activation during aim and retirement on release at 30, 60, 90 and 120 measured
  FPS. A held arrow restored after same-slot rejoin and retired on native cancel.
  All 106 unique receiver pose tuples matched the sender trace.
- The 30 FPS side-view replay in `build/m25-arrow-profile-9pfm_fb9/` shows the
  Normal Arrow in the remote bow at 1280x720 and 100% render scale. The arrow
  retired on release. Rebuild `build/m25_nocked_arrow_live.py` and run
  `build/m25_run_nocked_arrow.py` or `build/m25_run_arrow_profile.py`.
  Both peers need the updated build. The matching launcher is
  `build/m25-build/Play-M25.cmd`.
- Both installed bow packs contain the same Bow.baslist and six embedded clips.
  The list loads Weapon_Bow_Common_Animation and exposes Draw, Hold, Shoot,
  Equiped, Null and SelfCameraPause.

## M2.5 bowstring animation

- The native bow controller in `build/m25-guard-bow-4padu4m4/` remained Equiped during
  early aim, then entered Draw. Shoot extended into Link's held-ready stance. The
  passive bow uses its own controller to follow those timings.
- Both supported bow aliases mount the installed bow animation list and states. Snapshot
  protocol 3 uses a formerly reserved byte for the native bow state. Both peers need a
  matching build. The snapshot stays at 96 bytes, and the renderer requires a bow pose
  from the matching equipment revision.
- The Windows app linked with source ID `856b34baf572ee17f73d0a94ac97c53b3c930d8a`
  and existing 1.9.0 gamelib `13365c313591b27e`. The launcher is
  `build/m25-build/Play-M25.cmd`. Core, direct invite and WebRTC loopback passed.
- The scratch replay in `build/m25-nocked-arrow-1pfzvcs_/` verified Soldier's Bow draw and
  release, steady aim, cancel, remove and re-equip, and restored Draw on same-slot
  rejoin during aim. Draw and Shoot entered on one bow actor across the 30, 60, 90 and
  120 FPS checks. The nocked Normal Arrow remained independent of the bow actor.
- The 30 FPS side-view replay in `build/m25-bowstring-profile-p8x7vv4h/` verified the
  Traveler's Bow. The captures compare forced Hold with replicated Draw while Link
  stayed aimed. They show the limbs and string moving into the drawn pose.
- Rebuild the ignored fixture with `build/m25_bowstring_live.py` and run
  `build/m25_run_nocked_arrow.py` with
  `SWMP_PROBE_APP=build/m25-build/bowstring-live/savagewind.exe`. The comparison uses
  `build/m25_bowstring_profile_build.py` and `build/m25_run_bowstring_profile.py`. See the
  [test record](TESTING.md#m25-bowstring-animation).

## M2.5 nocked arrow variants and rapid reload

- The five other inventory arrow types use passive aliases built from the installed TitleBG
  members. The sender resolves the attached actor's native name while nocked. Protocol 4
  accepts the expanded identity set and both peers need the matching build.
- The ActorInfo builder's bounded buffer now fits the combined alias tables. The scratch
  `build/m25_arrow_variants_resources.py` check verified all fifteen visual packs and
  preserved the stock actor records.
- The Windows app linked with source ID `a7106be30f6384fe6ab5b88bac0f791ff033dd99`, reusing
  gamelib `13365c313591b27e`. The launcher is `build/m25-build/Play-M25.cmd`. Core, direct
  invite and WebRTC loopback passed.
- Routine arrow retirement now waits one snapshot interval before a new request. The
  one-second failure backoff still applies. Four rapid Normal Arrow draws and releases
  passed at both 30 and 120 FPS. See the [test record](TESTING.md#m25-nocked-arrow-variants-and-rapid-reload).
- The native quick selector equipped Fire, Ice, Shock, Bomb, and Ancient Arrows at
  30 FPS. Each nocked visual appeared and retired on the receiver. Same-slot rejoin during
  Ancient Arrow aim restored its visual and the bow's Draw state.
- The combined replay in `build/m25-arrow-variants-tu31l6l1/` also verified bow remove
  and re-equip with the other gear active, followed by Leave and same-slot rejoin.
- Rebuild the ignored fixture with `build/m25_arrow_variants_build.py` and run
  `build/m25_run_arrow_variants.py`. Set `SWMP_RAPID_RELOAD=1` to include the cadence checks.

## M2.5 grounded dodges

- The native trace in `build/m25-dodge-sd300nm8/` confirmed Step and StepLand for neutral,
  left, right and forward dodges. The sideways and forward steps moved about 18 cm in
  the first 30 FPS update while facing stayed fixed. The native backflip used BackJump
  and BackJumpLand.
- The sender selects the shared Step direction from the first planar displacement
  in Link's facing frame and retains it through StepLand. BackJump uses its own clip
  pair. The installed backward Step clips are available if that shared state moves
  backward. The tested back input went through BackJump.
- In `build/m25-dodge-live-35hivj8x/`, two Windows games verified all five takeoff and
  landing pairs at 30, 60, 90 and 120 measured FPS. Each clip played once per dodge on
  the same proxy, and Leave and same-slot rejoin retired and restored the Link.
- The closer capture in `build/m25-dodge-profile-5zffty2m/` showed the remote side hop
  and backflip. The stowed axe and shield can briefly separate from the proxy during
  the flip. Recheck attachment alignment during the Playback and Appearance stages.
- The Windows source ID is `4fc921cb3b83f486928a8a0305dc9f77521f4245`, with protocol 4
  and gamelib `13365c313591b27e`. The launcher is `build/m25-build/Play-M25.cmd`.
  Rebuild the ignored fixture with `build/m25_dodge_live.py` and run
  `build/m25_run_dodge_live.py` or `build/m25_run_dodge_profile.py`. See the
  [test record](TESTING.md#m25-grounded-dodges).

## M2.5 hit reactions and knockdown recovery

- Unguarded Soh Kofi and Chaas Qeta enemy hits entered native DamageM. The Soh Kofi
  replay in `build/m25-reactions-enemy-kabvjotp/` verified the remote reaction and
  return to idle at 60 FPS. A close Bomb Arrow blast in Tah Muhl entered DamageL.
- The survivable replay used the upgraded Champion's Tunic in scratch saves. It exposed
  SlippingDown and SlippingDownLand between DamageL and DamageLStand. The receiver now
  has separate tumble and landing actions before the stand-up clip.
- The final run in `build/m25-reactions-armored-1_mpvqet/` verified the complete
  knockdown, tumble, landing, stand-up and idle sequence on one Link at 30 FPS.
  The Windows launcher is `build/m25-build/Play-M25.cmd`, source ID
  `1f30f098d12fb08eaeffd3767dff0476627fd914`, with gamelib `13365c313591b27e`.
- The build selects representative frontal reactions and a representative landing
  roll. The installed light hit is checked synthetically. Native light hits, exact
  direction and upper-body flinches remain open.
- Rebuild the ignored fixtures with `build/m25_reactions_live.py` or
  `build/m25_reactions_armored.py`. The routes are `build/m25_run_reaction_enemy.py`
  and `build/m25_run_reaction_armored.py`. For the armored route set
  `SWMP_PROBE_APP=build/m25-build/reactions-armored/savagewind.exe` and
  `SWMP_VERIFY_REACTION=1`. See the [test record](TESTING.md#m25-hit-reactions-and-knockdown-recovery).

## M2.5 fall-damage recovery

- The scratch drops use `cell player.pos` to raise the native player and
  let the game resolve the fall. The ordinary teleport command grants a short
  fall-damage grace period. In Tah Muhl Shrine, the 21 m drop took four quarter-hearts
  and entered LandDamage followed by LandDamageStand. A 6 m drop used Land.
- The final two-game replay in `build/m25-hard-landing-xsbgdcyi/` verified both drops
  at 30, 60, 90 and 120 measured FPS on one Link. The local receiver's health stayed
  at 12 quarter-hearts. Captures show the remote impact and recovery poses.
- The Windows launcher is `build/m25-build/Play-M25.cmd`, source ID
  `950097e37b41370878be23e6338b9dc9a1b99845`, with gamelib `13365c313591b27e`.
  Rebuild the ignored fixture with `build/m25_hard_landing_live.py`, then run
  `build/m25_run_hard_landing.py` with `SWMP_VERIFY_LANDING=1`,
  `SWMP_PROBE_APP=build/m25-build/hard-landing-live/savagewind.exe`,
  `SWMP_FALL_HEIGHTS=6,21` and `SWMP_FALL_FPS=30,60,90,120`.
  See the [test record](TESTING.md#m25-fall-damage-recovery).

## M2.5 animation clip progress probe

- The playback probe used source ID `c3c71f0f65dcf665b4f3ba417fd05ef3dc8ddf7f`
  and 1.9.0 gamelib `13365c313591b27e` on both peers.
- On Windows 1.9.0, the native player and passive proxy exposed advancing frame counters
  that reset for repeated jumps. The engine overwrote writes to the mirrored counters.
  The second float argument to the existing animation play call starts the clip at a
  normalized position. Passing 0.5 started the 10-frame jump halfway through at normal speed.
- In stage 4, start with a standing-jump timing slice. Verify the track read for the matching native clip,
  then carry its start phase and activation identity in snapshots. Use the normalized start
  argument on activation and test delayed snapshots and repeated one-shots before expanding.
- The ignored `build/m25_phase_probe.py` builds the read-only fixture. Use
  `SWMP_PROBE_APP=build/m25-build/phase-probe/savagewind.exe` and
  `SWMP_TRAVERSAL_ROUTE=build/m25_phase_route.txt` with `build/m25_run_traversal.py`.
  The seek probe uses `build/m25_phase_seek_probe.py`, the `phase-seek-probe` executable directory,
  and `build/m25_phase_seek_route.txt`. The normalized-start probe uses `build/m25_phase_rate_probe.py`
  and the `phase-rate-probe` executable directory with that same route.
  See the [test record](TESTING.md#m25-animation-clip-progress-probe).
- See [remaining animation work](NEXT.md#remaining-animation-work) for playback, traversal,
  weapon families, shield, bow and appearance coverage.

## M2.5 lock-on swimming

- The lock-on swimming checkpoint used source ID `c3c71f0f65dcf665b4f3ba417fd05ef3dc8ddf7f`.
- Native SwimAtnMove exposes targeting movement. The sender projects planar displacement
  into Link's facing frame and selects a forward, backward, left or right stroke. Treading
  and free swimming keep their existing paths.
- The ignored `build/m25_swim_lock_live.py` builds the instrumented app. Set
  `SWMP_PROBE_APP=build/m25-build/swim-lock-live/savagewind.exe` and
  `SWMP_TRAVERSAL_ROUTE=build/m25_swim_lock_verify.txt` for `build/m25_run_traversal.py`.
  See the [test record](TESTING.md#m25-lock-on-swimming).
- Target-following in the water, exact blend playback and water effects still need coverage.

## M2.5 swim stroke selection

- The free swim checkpoint used source ID `d3d88b9e192aa7046ce55f95371214d6d65bf712`.
- Native SwimMove covers both stroke speeds and both turn directions. The sender samples
  speed and wrapped heading change to select one of the six installed swimming clips.
  Treading and dash keep their dedicated clips. All swim actions use the same model lift.
- The ignored `build/m25_swim_blends_probe.py` builds the trace executable. Set
  `SWMP_PROBE_APP=build/m25-build/swim-blends-probe/savagewind.exe` and use
  `SWMP_TRAVERSAL_ROUTE=build/m25_swim_blends_route.txt` with `build/m25_run_traversal.py`.
  Use `build/m25_swim_caps_route.txt` for the higher caps. See the [test record](TESTING.md#m25-swim-stroke-selection).

## M2.5 glide banking

- The glide banking checkpoint used source ID `495498ca1eebc9f5628d0537b71a2a21abb07c21`.
- Link's native root stayed upright during steering. The sender estimates bank from heading change,
  and the receiver tilts the neutral flight clip and open canopy together. Native blends and precise
  hand attachment remain open.
- The ignored `build/m25_bank_probe.py` builds the instrumented executable. Set
  `SWMP_PROBE_APP=build/m25-build/bank-probe/savagewind.exe` and `SWMP_TRAVERSAL_ROUTE` to
  `build/m25_bank_route.txt` for the two-direction and lifecycle run, or
  `build/m25_bank_visible_route.txt` for same-slot rejoin and the higher caps. For close
  captures use `build/m25_bank_close_route.txt`. Run `build/m25_run_traversal.py`.
  See the [test record](TESTING.md#m25-glide-banking).

## M2.5 directional climbing jumps

- The climbing jump checkpoint used source ID `f6877769f778e5ad6dd4d41b96751c75d6caddc8`.
- The sender selects one installed clip for each native ClimbJump using early movement in Link's
  wall plane. The current clips cover right, up-right, up, up-left and left. Direction stays fixed
  until the native state changes. The proxy still starts the clip with its own animation timing.
- The ignored `build/m25_jump_probe.py` builds the instrumented app. Set
  `SWMP_PROBE_APP=build/m25-build/jump-probe/savagewind.exe` for `build/m25_run_traversal.py`.
  Set `SWMP_TRAVERSAL_ROUTE` to `build/m25_jump_30_route.txt` for all five clips at 30 FPS or
  `build/m25_jump_route.txt` for the higher caps.
  See the [test record](TESTING.md#m25-directional-climbing-jumps).

## M2.5 gliding

- The gliding checkpoint used source ID `c4f6bceacb6af307ab84ace438d5218871469310`.
- Native ParaEquipOn, ParashawlGlide and Fall select the deploy, glide and fall clips. A transient
  gliding bit and matching action create a passive animated canopy with a dedicated actor index.
  The stock actor pack lives inside TitleBG. The virtual alias reads only the SARC metadata and
  the requested member from the installed archive.
- This checkpoint used the neutral flight clip and a fixed canopy transform in Link's yaw frame.
  Precise hand attachment and additional glider variants remain open.
- The ignored `build/m25_glide_probe.py` builds action and lifecycle traces. Use
  `SWMP_PROBE_APP=build/m25-build/glide-probe/savagewind.exe` and
  `SWMP_TRAVERSAL_ROUTE=build/m25_glide_route.txt` with `build/m25_run_traversal.py` for native cancel
  and landing. Use `build/m25_glide_rejoin.txt` for Leave and rejoin during flight.
  See the [test record](TESTING.md#m25-gliding).

## M2.5 directional wall climbing

- The wall climb checkpoint used source ID `2fb96f88f354fd5544af2c78f21540288cfe8f50`.
- The native ClimbMove state covers all eight directions. The sender projects about 100 ms of
  displacement into the wall plane. Link's local X axis points left. The selected direction
  travels as an action ID in the existing position snapshot.
- WallDashSt and WallDash share the approach clip. ClimbEd selects a ledge mantle clip.
  This checkpoint used the upward variant for climbing jumps. The proxy plays discrete clips
  with local animation timing.
- Run the Oman Au masonry route with `SWMP_TRAVERSAL_ROUTE=build/m25_climb_route.txt` and
  `build/m25_run_traversal.py`. Use `build/m25_mantle_route.txt` for the full climb to the ledge.
  The local probe and scratch-save setup match the previous slice.
  See the [test record](TESTING.md#m25-directional-wall-climbing).

## M2.5 swimming and climbing

- The initial traversal checkpoint used source ID `b6cc1385595f980904c71878c84b5b2e44ce7bb8`.
- The native states SwimWait, SwimMove, SwimDash, ClimbWait, ClimbMove and ClimbJump select
  the basic clips. The swimming movement clip depicts a forward stroke. This checkpoint used
  the upward wall movement and jump variants.
- The native actor matrix and PlayerInfo agree on the swimmer's position. At that root the standalone
  proxy clips left Link below the waterline. The receiver raises the swimming model by 1.65 m.
  Equipment transforms and range checks continue to use the sampled root. Water effects remain open.
- The local `build/m25_run_traversal.py` runner accepts `SWMP_TRAVERSAL_ROUTE` with
  `build/m25_traversal_route.txt` for the pond and masonry wall near Oman Au Shrine. It loads
  `build/m25-build/traversal-probe/savagewind.exe` with isolated settings and saves.
  See the [test record](TESTING.md#m25-swimming-and-climbing).

## M2.5 Traveler's Bow

- The bow checkpoint used source ID `670a3194f0b5a7ae7a407ddd0b9a7353f947450c`. It added
  three gear identities per revision and 80-byte player snapshots.
- The bow uses native equipment category two and a passive actor with its own index.
  Layer 1, slot 1 exposes BowReload, BowCharge and BowShoot during standing bow use. The proxy
  selects the corresponding installed clips. The base WeaponEquipOn state is shared by axe and bow,
  so axe draw requires the axe's hand-height band.
- The ignored `build/m25_bow_probe.py` builds the trace and an isolated clipboard. The runner
  `build/m25_run_bow.py` uses scratch saves and native menus to equip the host's bow and arrows.
  Set `SWMP_BOW_FPS=1` and `SWMP_BOW_TRAVEL=1` to exercise the additional frame rates and shrine
  round trip. See the [test record](TESTING.md#m25-travelers-bow).
- The bow actor applies a rigid transform. Bowstring and arrow visuals, blended or airborne aim,
  exact clip progress and skinned equipment remain open. The
  [appearance probe](M2_POSITION_HANDOFF.md#resume) describes the multi-model attempt.

## M2.5 shield, axe idle and crouch

- The shield checkpoint used source ID `5b6399a29f68162f952aff5316c44ad03ed9216d` and
  1.9.0 gamelib `13365c313591b27e`.
- Weapon and shield identities now share a reliable equipment revision and each slot has a separate
  pose and native actor index. The Wooden Shield samples equipment getter category one, as confirmed
  in the [test record](TESTING.md#m25-shield-axe-idle-and-crouch). Updates to one slot preserve the
  other slot's actor. The two virtual gear packs use the same passive profile.
- The native `Wait` state can cover both stowed and drawn idle. The axe hand-height band gates
  `A_Pose_Lsword_Wait`, the native large-sword upper-body clip. The new map also covers
  `WeaponEquipOff`, `SquatWait` and `SquatMove`. A forward shield pose during `WaitAttention`
  selects `Shield_Guard_Wait`. Drawn idle and guarding approximate native blends.
- The ignored `build/m25_shield_probe.py` builds a fixture with an isolated clipboard and action
  traces. `build/m25_run_shield.py` drives both scratch games, including native inventory equip,
  shrine travel, same-slot rejoin, and changes from 30 through 120 FPS. The fixture stages the
  Wooden Shield in the host's scratch inventory using the app's item command.

## M2.5 first slice

- The default-on **Sync host time and weather** preference is shared by F1 and System > Options.
  The host sends time and effective weather with position batches. Clients apply fresh samples within
  the same area when gameplay is ready. The passive Link actor remains independent of this path.
- Join and disconnect notices render above gameplay for five seconds. The host and existing clients see
  a named notice when a peer joins or leaves. An admitted player also gets a local notice when their
  session ends. Late joiners receive the current roster without a burst of notices.
- The 1.9.0 `mp_weather_current` site resolves the world manager's effective-weather getter. The prior
  cheat override field returns 255 during ordinary forecasts and cannot serve as the host's sole source.
  The environment and disconnect checks used game library `13365c313591b27e`. See the weapon families
  checkpoint above for the current Windows launcher and multiplayer source ID.
  See the [M2.5 test record](TESTING.md#m25-first-slice) for Windows validation and remaining cases.
- For overhead name feasibility, `control_camera_view()` in `runtime/nvn/vulkan/device.c` currently follows
  the motion-vector path. A default-on label needs a current world view and projection in other AA modes
  too, and must use the world viewport and the active proxy's applied pose.

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
- The [test record](TESTING.md#m25-passive-axe-and-animation) lists the native unequip, re-equip, travel,
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

## M2 acceptance follow-up

- The owner accepted M2. [M7](MILESTONES.md#m7--deferred-manual-validation) tracks the eight-game soak,
  Linux AppImage and mixed Windows/Linux sessions. Both build targets use the same version label and
  source fingerprint.
- The current range is 50 m to spawn and 60 m to retain. The earlier 100/120 m policy allowed the
  engine to delete actors before multiplayer withdrew them.
- The Windows launcher is `build/m2-area-build/Play-M2-Area.cmd`. Match source ID
  `e00ea2478251cec97481ac59c2a297346ec1b855` and game library `e640fd858dc0d033` for this checkpoint.
  The Windows ZIP recipe now collects libdatachannel from the configured dependency prefix.
  The verified Windows artifact is `build/dist/SavageWind-0.6.0-alpha.6-0.1.0-mp-windows.zip`.
  See the [test record](TESTING.md#m2-acceptance-follow-up) for range, capacity, and packaging evidence.
- The ignored `build/m2-probe/run_accept_range.py`, `run_accept_capacity.py` and
  `run_accept_roundtrip.py` exercise the latest build. Two native Windows shrine round trips passed.
  One actual renderer plus seven headless clients passed the capacity hold and measured the requested
  frame rates. Eight full clients still need additional machines or a host with more RAM.
- The ignored `build/m2-probe/run_accept_udp.py` passed a two-game run using the test-only
  libdatachannel build in `build/m2-udp-fixture/obj/`. See the test record for loss, delay,
  movement, facing, and health checks.
- The subsequent full-renderer soak held four Windows games in the same shrine for 90 seconds.
  Each renderer kept the other three actors active at about 30 FPS, and Leave retired the expected actors.
  The harness stopped at four with 1.89 GiB of available physical RAM and 4.28 GiB of available commit.
  Use additional machines for the eight-game check. The local logs and screenshots are in
  `build/m2-accept-eight-6yslv0ug/`. See the [test record](TESTING.md#m2-acceptance-follow-up).
- Linux packaging was paused at the owner's request for manual validation. In the source-only Ubuntu
  22.04 x86-64 VM, rootless Podman built the toolchain, libdatachannel, the generator helper and SDL.
  The VM was shut down during FFmpeg compilation, before an AppImage was produced. The build log and
  passing Linux multiplayer test log are in ignored `build/m2-linux-vm/`.

## M2 shrine identity and native actor retry

- The RomFS stage tracker now supplies a stable multiplayer area ID for the main field and each shrine.
  Proxies require a fresh pose in the same area. An area change retires the old proxies even if the guest
  reuses the same actor or spawn context. Both Windows instances visibly rendered a remote Link in
  the same shrine. Two instances in different shrines kept the same lobby with no remote actors.
- The earlier build reproduced the native actor loss at about 92 m on the open Plateau with no Slate travel.
  Both proxies stayed active near 30 and 60 m. The retry experiment limited repeated spawns after native
  actor loss beyond 60 m and retried sooner when the players moved at least 10 m closer. A local write watch
  caught the game's native actor deletion routine setting the flag before the multiplayer tick retired
  the slot. The native condition that triggers deletion remains open.
  [Test record](TESTING.md#m2-shrine-identity-and-native-retry)
- The historical 100/120 m retry experiment used source ID `cfca0ce8eae2beffbe4b682fc9d176d656f66d85`.
  The current Windows launcher and source ID are listed above.
- The ignored `build/m2-probe/build_area.py` and `run_area.py` use an isolated clipboard and scratch saves.
  The runner accepts `shrine`, `mixed`, `range`, or `entry`. With `SWMP_SHRINE_MAP=1`, the shrine run
  accepts input through its scratch `map-input` file. One client used the native elevator to return to
  the main field and both proxies retired. In a separate test, a client entered from outdoors and both
  proxies activated after arrival. The lobby stayed connected during both loads.
  The ignored watch probe and logs in `build/m2-area-live-xb_xb_z1/` contain the guest call chain for deletion.

## M2 visibility follow-up

- In the latest field test a Linux client joined the owner's Windows host, but neither could see the other.
  With a friend hosting on Linux, the Linux client initially saw the host, and the Windows client joined from
  farther away. After the Linux client used Slate fast travel, it no longer saw either player. The owner also
  reported that walking a short distance was enough for proxies to disappear.
- The earlier build published positions only inside a 20-by-20 m rectangle near the Shrine of Resurrection,
  with a 12 m height band. The follow-up samples the main field and spawns proxies within 100 m of the local
  player. It retains existing requests and actors up to 120 m to prevent repeated spawning near the boundary.
  Loading, a changed spawn context or local actor, and unavailable remote positions retire the corresponding
  actors. A fresh nearby pose can spawn a replacement in the same lobby.
- Two Windows scratch games stayed in the same lobby while the client used the native Slate to travel to
  Great Plateau Tower and back to the Shrine of Resurrection. The returned proxy actors repeatedly became
  inactive at about 92 m separation. At about 4 m apart outdoors, both players were visible again. Use the
  logs in `build/m2-slate-kcjjyehi/` to compare intermittent native actor activation with the first stable
  reunion. [Test record](TESTING.md#m2-visibility-follow-up)
- The earlier `build/m2-version-build/Play-M2-Version.cmd` used the local 1.9.0 gamelib with
  ID `e640fd858dc0d033`. Build the Linux app from the same source and confirm both runs show source ID
  `3344f886df6a2e0a8a90d4dedddc1cc95f37380e`. For a visibility test, confirm both names and health are
  present in F1, meet at the same overworld waypoint, and face each other. On failure, keep the `[multiplayer]`
  lines from both sides, the two locations, and the lobby roster outcome.
  See [testing](TESTING.md#m2-visibility-follow-up).

## Windows and Linux lobby compatibility

- The old `build/windows-local/savagewind.exe` embedded source ID
  `a90e1242b88646f11812935c9a9f19517d723d15`. The current Linux host and M2 Windows client both reported
  source ID `970ccbd296037d6d9aec806b7c0c22dca89d1d9e` and BOTW main build ID
  `cd57b23fa4bbad65803d9788c01821ee00000000000000000000000000000000` (1.9.0).
- The working launch path is the ignored `build/m2-health-build/Play-Matching-M2.cmd`. It runs the executable
  and game library in its own folder, resolves DLLs from `build/windows-local` and
  `build/multiplayer-deps/bin`, and defaults `SAVAGEWIND_DATA_ROOT` to `build/windows-local` so it uses
  the existing data, saves, and settings. It honors an explicit data-root override. The Windows scratch
  startup log in `build/mp-matching-boot-3sky6sm5/startup.log` verified both IDs before the owner confirmed the join.
  The old executable is still present in its original folder.
- On a later rejection, compare the `[multiplayer] source ID: …, game build: …` line from each actual run.
  Windows development builds write it to the launching terminal. Windows builds without a console or redirected
  stderr write `savagewind.log` beside the executable. Linux writes to the data folder, usually
  `~/.local/share/savagewind/savagewind.log`. Use `rtk make -s print-multiplayer-id` for the source tree's
  expected ID. The Windows and Linux Make targets produced the same ID with the same source inputs.
  See the [build compatibility guide](../docs/build-windows-and-appimage.md#multiplayer-build-compatibility).
- Local game-library IDs encode platform and CPU build flags. The matching Windows x86-64-v3 library is
  `e640fd858dc0d033`. The lobby compares the multiplayer source IDs and BOTW main build IDs.
  Use a fresh invite and reply for each join attempt.
- The latest Windows-host test admitted a Linux client. Visible proxies in that direction remain unverified. Use the
  [direct multiplayer handoff](DIRECT_MULTIPLAYER_HANDOFF.md) for the clipboard flow and transport diagnostics.
  That handoff also records the OpenSSL and DTLS checks pending before distribution.

## Linux AppImage recompiles on launch

The owner double-clicks the AppImage from `build/dist/`. Their log contained
`[launcher] setting up this version by itself (Game code built for another version)`. The launcher found
`swgame.so` and could not match a readable adjacent `swgame.so.id` to the selected target. The exact cause
is open. The owner has yet to supply the two files' metadata and ID after a completed build.

1. After setup completes and the game closes, capture the files before reopening the AppImage. Use the
   folder shown in the log's `[paths] data:` line for `root`.

   ```sh
   root="${XDG_DATA_HOME:-$HOME/.local/share}/savagewind"
   ls -l "$root/extracted/swgame.so" "$root/extracted/swgame.so.id"
   cat "$root/extracted/swgame.so.id"
   ```

2. Compare that root and the selected CPU target across launches. For `7991b79` with the default x86-64
   Linux Make flags, the locally calculated IDs are `f24383c5199a514c` (v3) and `0bfc4f56bfc9a721` (v2).
   Confirm the actual AppImage build and flags before using those values to judge the installation.
3. Trace mismatches with [launcher readiness and auto-setup](../runtime/ui/launcher.mm),
   [game-library lookup](../runtime/gamelib.c), [library and ID writes](../runtime/gamebuild.c), and
   [data-root selection](../runtime/paths.c). Preserve the installed files and their IDs during diagnosis.

A packaged AppImage opens Configuration by default. The **Play** button starts a ready game, and
**Game Files → Open this screen first** controls that landing page. **Set up game files** can deliberately
rebuild the library. The reported automatic version rejection still needs investigation.

## M2 remote Link presence

- Position, facing, idle, locomotion and jump are implemented for the Plateau test area. The passive
  RomFS alias uses the installed Link model and animation archive. Proxy placement and playback use
  version-aware native calls on an owned actor. The wire action IDs are idle 0, jump 1, and walk 2.
  Health is sampled as quarter-hearts and displayed in the F1 roster.
- Two Windows games verified actions in both directions, independent inventory, area withdrawal and return,
  same-slot rejoin and reboot cleanup. One renderer sustained seven remote Links driven by headless WebRTC
  fixtures through START and disconnect. The health test relayed a controlled change from 12 to 9 and back
  to 12 quarter-hearts while the host stayed at 12. The [test record](TESTING.md#m2-plateau-animation-check)
  has GPU, FPS, settings, builds and evidence paths.
- Appearance and reliable equipment revisions are planned for M2.5, after M2 is complete. The passive proxy failed to
  activate when the scratch model list included a second model for default hair, including with an 8 MiB
  instance heap. Start from one native equipped model on an owned proxy. The [M2 handoff](M2_POSITION_HANDOFF.md#resume)
  has source entry points and probe paths.
- See the [milestone ledger](MILESTONES.md#m2--remote-link-presence) for acceptance status.
  The owner-confirmed cross-platform join verifies lobby admission. Joint-platform travel and presence
  remain on the owner's manual test list.
