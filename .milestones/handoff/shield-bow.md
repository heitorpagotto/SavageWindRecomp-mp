# Shields, bows, and arrows

## M2.5 shield-surf attack and sword guard walk

- `CutShieldRide` selects `Shield_Board_Attack` on the body and armor, including when the cut is sampled
  in a secondary slot over the surfing pose. The phase follows the 53-frame clip, including the native
  branch cut short at frame 38. The shield and attached equipment keep their sampled native transforms.
- Raised-shield `MoveAttention` with a drawn one-handed sword selects an in-place `Nml_LockonBattle_Walk`
  gait from the existing facing-relative direction sampler. The proxy blends `Sword_Guard_Wait` onto
  the gait with the native `ArmLR` mask. The same pose plays at rest and the shield-only mapping covers
  movement with the sword stowed.
  [Verification](../testing/traversal.md#m25-exhaustion-whistle-wall-slip-and-shield-actions)

## M2.5 bow draw and shield model playback

- The stowed spear can briefly move into the drawn-height range before the
  bow reaches Link's hand. Shared `WeaponEquipOn` now uses the native base
  clip length to distinguish shield (16), bow (20), and melee (32) draws.
- Each of the 33 installed shields uses the common stand, guard, hit, and
  surf actions. The four shields with their own clips keep their native AS
  resources in the passive alias and follow the sender's model animation.
- Protocol 9 includes the shield model animation in the player snapshot and
  requires a matching shield pose and equipment revision.
  [Verification](../testing/shield-bow.md#m25-bow-draw-and-shield-model-playback)

## M2.5 raised-shield movement, reactions, and surfing

- Standing guard and parry were already synced. Native `MoveAttention` fell back
  to Idle while moving with the shield raised. The sender now selects one of
  four in-place lock-on walk clips from displacement in Link's facing frame.
- Guard detection uses the native shield pose even for shields outside the
  passive model catalog. Parry, hit, and break use their native action states,
  so the body reaction can play when the model is unavailable or already broken.
- Shield surfing now has mount, slide, jump, airborne, land, dismount, stumble,
  and failed-start mappings. Supported shields follow their sampled native
  transforms under Link's feet. Surf turns and slope-specific variants use
  representative clips.
  [Windows verification](../testing/shield-bow.md#m25-raised-shield-movement-reactions-and-surfing)

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
  [test record](../testing/shield-bow.md#m25-bowstring-animation).

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
  passed at both 30 and 120 FPS. See the [test record](../testing/shield-bow.md#m25-nocked-arrow-variants-and-rapid-reload).
- The native quick selector equipped Fire, Ice, Shock, Bomb, and Ancient Arrows at
  30 FPS. Each nocked visual appeared and retired on the receiver. Same-slot rejoin during
  Ancient Arrow aim restored its visual and the bow's Draw state.
- The combined replay in `build/m25-arrow-variants-tu31l6l1/` also verified bow remove
  and re-equip with the other gear active, followed by Leave and same-slot rejoin.
- Rebuild the ignored fixture with `build/m25_arrow_variants_build.py` and run
  `build/m25_run_arrow_variants.py`. Set `SWMP_RAPID_RELOAD=1` to include the cadence checks.

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
  round trip. See the [test record](../testing/shield-bow.md#m25-travelers-bow).
- The bow actor applies a rigid transform. Bowstring and arrow visuals, blended or airborne aim,
  exact clip progress and skinned equipment remain open. The
  [appearance probe](../M2_POSITION_HANDOFF.md#resume) describes the multi-model attempt.

## M2.5 shield, axe idle and crouch

- The shield checkpoint used source ID `5b6399a29f68162f952aff5316c44ad03ed9216d` and
  1.9.0 gamelib `13365c313591b27e`.
- Weapon and shield identities now share a reliable equipment revision and each slot has a separate
  pose and native actor index. The Wooden Shield samples equipment getter category one, as confirmed
  in the [test record](../testing/shield-bow.md#m25-shield-axe-idle-and-crouch). Updates to one slot preserve the
  other slot's actor. The two virtual gear packs use the same passive profile.
- The native `Wait` state can cover both stowed and drawn idle. The axe hand-height band gates
  `A_Pose_Lsword_Wait`, the native large-sword upper-body clip. The new map also covers
  `WeaponEquipOff`, `SquatWait` and `SquatMove`. A forward shield pose during `WaitAttention`
  selects `Shield_Guard_Wait`. Drawn idle and guarding approximate native blends.
- The ignored `build/m25_shield_probe.py` builds a fixture with an isolated clipboard and action
  traces. `build/m25_run_shield.py` drives both scratch games, including native inventory equip,
  shrine travel, same-slot rejoin, and changes from 30 through 120 FPS. The fixture stages the
  Wooden Shield in the host's scratch inventory using the app's item command.
