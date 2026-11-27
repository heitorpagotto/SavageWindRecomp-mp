# Shields, bows, and arrows

## M2.5 bow draw and shield model playback

- The baseline Windows trace in `build/m25-bow-shield-probe-1m4oec3b/`
  reproduced the false SpearDraw at the start of a 20-frame bow draw.
  The stowed spear crossed the old hand-height threshold while the bow was
  still on Link's back.
- The Windows build, animation selector regression, multiplayer core,
  direct invite, and WebRTC loopback checks passed. Changed sources
  cross-compiled for Linux x86-64. The installed-resource check covered
  guard, hit, and surfing selectors for all 33 shields, the complete
  animation lists for the four animated shields, and all 111 body clip
  references. [Implementation notes](../handoff/shield-bow.md#m25-bow-draw-and-shield-model-playback)
- Two Windows Vulkan games on the Plateau used BOTW 1.9.0, gamelib
  `baf44255c0bc3428`, protocol 9, and source ID
  `3165585c9f0b927300f035404b4ae74ac0e1d650`. Both used scratch saves,
  960x540 at 50% scale, a 1400 MiB VRAM budget, and Vulkan heap copy off.
  The host used an RTX 4050 Laptop GPU (610.88) and the client AMD Radeon
  Graphics (25.10.30.02).
- `build/m25-bow-shield-live-kp4u2aj6/` verified `Weapon_Shield_038`
  and `Weapon_Shield_014` at measured 30 FPS with GPU overlap off. Both
  shields played their own Off, WeaponEquipOn, On, and WeaponEquipOff states
  while Link guarded and parried. Each custom bow drew, aimed, and released
  without a melee draw on the receiver. A subsequent spear draw still played.
  Rejoin restored the equipped shields and Off state on both peers.
- `build/m25-bow-shield-live-gzn5p2xc/` repeated the same checks for
  `Weapon_Shield_013` and `Weapon_Shield_015`. The 90 and 120 FPS caps
  measured about 76/84 and 64/62 FPS on the host/client at the sample points.
  An earlier 60 FPS trial logged a Vulkan device timeout after verifying
  shield playback. The 30 FPS replay completed.
- The local scripts are `build/m25-bow-shield-build/build_fixture.py`,
  `verify_live.py`, and `verify_resources.py`.

## M2.5 raised-shield movement, reactions, and surfing

- The Windows app, multiplayer core, direct invite, and WebRTC loopback
  checks passed. The installed-resource check resolved all 110 clips,
  verified the eight in-place locomotion curves, and checked the guard and
  surf selectors even with no shield model ID.
- The Soh Kofi Shrine replay in `build/m25-shield-verified-2dv3wmq7/`
  relayed native `GuardFailHit` as ShieldHit and returned to ShieldGuard
  on the same Link and Soldier's Shield at 30 FPS.
- The Plateau replay in `build/m25-shield-verified-bvyi315y/` relayed standing
  guard, parry, and a right sidestep with the shield raised, followed by return
  to guard. Host/client rates were about 30/30, 60/56, 90/84, and 97/88 at
  the 30, 60, 90, and 120 FPS caps.
- The 30 FPS Plateau surf replay in `build/m25-shield-verified-w3od6_p1/`
  played mount, airborne, landing, slide, and dismount in order. One Link and
  one Soldier's Shield stayed active while the shield pose moved under the rider.
  Shorter trials could skip the brief landing or slide between snapshots.
  Surf jump, stagger, and the other mapped variants passed the resource and
  selector checks. Their gameplay coverage remains open.
- Both Windows games used BOTW 1.9.0, protocol 7, gamelib `baf44255c0bc3428`,
  multiplayer source ID `782adcd4d14ef19d9d1f63ba1bb5bf426780426c`,
  Vulkan at 960x540 and 50% scale, a 1400 MiB VRAM budget, Vulkan heap copy
  off, and scratch saves. The host used an RTX 4050 Laptop GPU (610.88) and
  the client AMD Radeon Graphics (25.10.30.02).
- The ignored scripts are `build/m25-shield-sync-build/build_fixture.py`,
  `verify.py`, and `verify_surf.py`.

## M2.5 nocked arrow variants and rapid reload

- The Windows Vulkan scratch games in Tah Muhl Shrine used BOTW 1.9.0, gamelib
  `13365c313591b27e`, protocol 4 and source ID `a7106be30f6384fe6ab5b88bac0f791ff033dd99`.
  The host used an RTX 4050 Laptop GPU (610.88) and the client used AMD Radeon Graphics
  (25.10.30.02). Both used 960x540, 50% render scale, a 1400 MiB VRAM budget and Vulkan heap copy off.
- In `build/m25-arrow-variants-z9lpjpdt/`, four consecutive Normal Arrow draw and release
  cycles passed on the same bow at 30 and 120 measured FPS. From a visual retiring to
  the next arrow becoming visible, the receiver measured 733–766 ms at 30 FPS and
  691–718 ms at 120 FPS. Both ranges fit inside the former one-second respawn delay.
- In `build/m25-arrow-variants-34sl5juy/`, the host equipped Fire, Ice, Shock, Bomb and
  Ancient via the native quick selector at 30 FPS. The logs recorded one active remote
  arrow at a time. The client captured each nocked type during steady aim. Leaving and
  rejoining during Ancient Arrow aim restored the same type and the bow's Draw state.
- The full replay in `build/m25-arrow-variants-tu31l6l1/` repeated both cadence checks
  and all five variant checks. It measured 765–800 ms and 691–700 ms between visuals at
  30 and 120 FPS respectively. Bow removal and re-equip preserved the axe and shield,
  and a final Leave and same-slot rejoin retired and restored all three gear visuals.
- Core, direct invite and WebRTC loopback passed. Core round-tripped every new identity,
  rejected invalid IDs and relayed Ancient Arrow through the host. The resource
  check verified all fifteen passive visual packs and preserved the stock ActorInfo records.

## M2.5 bowstring animation

- Two Windows Vulkan games used BOTW 1.9.0, gamelib `13365c313591b27e`, protocol 3 and
  source ID `856b34baf572ee17f73d0a94ac97c53b3c930d8a`. The scratch Tah Muhl Shrine route
  used an RTX 4050 Laptop GPU (610.88) and AMD Radeon Graphics (25.10.30.02), 960x540 at
  50% render scale, a 1400 MiB VRAM budget and Vulkan heap copy off.
- In `build/m25-nocked-arrow-1pfzvcs_/`, the same passive Soldier's Bow entered Draw and
  Shoot at 30, 60, 90 and 120 measured FPS. The Normal Arrow activated while nocked and
  retired on release at each cap. A same-slot rejoin during aim restored Draw and the
  arrow. Native cancel retired the arrow and returned the bow to Equiped.
  Bow removal and re-equip preserved the axe and shield actors. Axe draw and stow passed.
- The 30 FPS Traveler's Bow replay in `build/m25-bowstring-profile-p8x7vv4h/` used the
  same GPUs at 1280x720 and 100% render scale. Client captures
  `bow-profile-neutral.png` and `bow-aim-remote-30.png` compare forced Hold and replicated
  Draw at steady aim. The limbs bend and the string pulls toward the nocked arrow.
  The receiver entered Shoot on release and restored the stowed bow on same-slot rejoin.
- Core, direct invite and WebRTC loopback passed. Core verified bow state round-trip,
  relay, and clearing on unequip, and rejected out-of-range states, states without a bow
  pose and nonzero reserved bytes. The local resource check verified all ten passive
  visual packs, including both bow lists and all six state definitions in each.

## M2.5 nocked Normal Arrow

- Two Windows Vulkan games in Tah Muhl Shrine used BOTW 1.9.0, gamelib
  `13365c313591b27e` and source ID `3f7f9da84ecd7ba679e60c5c03469d2f71d6922a`.
  They used 960x540 at 50% render scale with scratch saves, a 1400 MiB VRAM budget,
  Vulkan heap copy off, and RTX 4050 Laptop GPU (610.88) and AMD Radeon Graphics (25.10.30.02).
- In `build/m25-nocked-arrow-jq6s5_sg/`, the receiver activated the Normal Arrow with
  the Soldier's Bow and retired it on release at 30, 60, 90 and 120 measured FPS.
  All 106 unique receiver pose tuples appeared in the sender trace. Native cancel
  retired the visual, and same-slot rejoin while aiming restored it. Bow removal
  and re-equip preserved the axe and shield.
- The 30 FPS side-view run in `build/m25-arrow-profile-9pfm_fb9/` used 1280x720 at
  100% render scale with the same adapters and gamelib. Its client capture shows
  the Normal Arrow nocked on the remote Soldier's Bow before release.
- Core, direct invite and WebRTC loopback passed. Core verified protocol 2 packet
  encoding and peer relay, and rejected unknown arrow identity, bad offsets,
  nonzero reserved bytes and an arrow without a bow pose. Resource checks
  confirmed the passive pack and preserved stock ActorInfo. Very rapid reload
  spacing and Linux gameplay remain unverified.

## M2.5 standing shield transitions

- The Windows 1.9.0 scratch probe in `build/m25-guard-transition-zrheifj4/` compared
  the default playback argument with zero, 12, 60 and one frame. Both games used Tah
  Muhl Shrine, Vulkan at 30 FPS and the `13365c313591b27e` gamelib. The middle raise
  and lower captures showed an intermediate body pose with 60 and the settled
  destination with zero. Both values selected the same requested clips.
- The production replay in `build/m25-guard-blend-kpq64rvp/` used source ID
  `6204b14ff79e7a921c8f3aa5cf5f2459d13405da` and the same gamelib. It verified
  four-frame idle-to-guard and guard-to-idle playback at 30, 60, 90 and 120 FPS
  targets, with both games measuring about the requested caps. At 30 FPS it
  also verified four-frame SwordReady transitions in both directions, default
  parry timing, and one Link and shield across all checks. The client captures
  show the body progressing between the standing and guarding poses. The run
  used 960x540 at 50% render scale, Vulkan heap copy off, a 1400 MiB VRAM budget,
  and RTX 4050 Laptop GPU (610.88) and AMD Radeon Graphics (25.10.30.02) adapters.
  The Windows app, core, direct invite and WebRTC loopback targets passed.
- The Chaas Qeta regression in `build/m25-guard-break-03uykit4/` verified default
  ShieldHit and ShieldBreak playback at 30 FPS. Only the destroyed shield retired.

## M2.5 held bow and shield destruction

- In Chaas Qeta Shrine (Dungeon082), a close-range enemy attack destroyed the Wooden
  Shield one sampled update before GuardBreak. The first build retired the equipment
  and skipped the reaction. The revised Windows build, source ID
  `e9928eba4b67d29f45947018ecbd17a57d018317`, played ShieldBreak and returned to idle
  on the same proxy. The client retired the shield once and preserved the axe.
  Runs `build/m25-guard-break-zh8konic/` and `build/m25-guard-break-72upt2zg/` used
  BOTW 1.9.0 gamelib `13365c313591b27e`, two 30 FPS Windows Vulkan games,
  960x540 and 50% render scale, and the RTX 4050 Laptop GPU (610.88) and AMD Radeon
  Graphics (25.10.30.02) adapters.
- In Tah Muhl Shrine (Dungeon000), the same build held the Soldier's Bow in BowReady
  between release and stow. Both games reached 30, 60, 90 and 120 FPS, and the receiver
  stayed in the ready clip for at least 0.65 seconds at every cap. Draw, aim, release,
  and stow passed at each cap. Axe draw and stow, bow remove/re-equip, Leave and
  same-slot rejoin passed at the end. Evidence and 30 FPS captures are in
  `build/m25-bow-ready-y5zwclgm/`. Core, direct invite, WebRTC, and the installed clip check passed.

## M2.5 Soldier's Shield and Bow

- Two Windows Vulkan games in Tah Muhl Shrine (Dungeon000) used BOTW 1.9.0 and gamelib
  `13365c313591b27e`. Both used isolated saves and settings, 960x540, 50% render scale,
  1400 MiB VRAM budget, Vulkan heap copy off and 30 FPS measured. GPU and driver were
  RTX 4050 Laptop GPU (610.88) on the host and AMD Radeon Graphics (25.10.30.02)
  on the client.
- The variant run in `build/m25-guard-bow-k28m4n5x/` verified native Soldier's Shield
  and Soldier's Bow equip, bow reload/aim/release, bow remove and re-equip with the remote
  shield and axe intact, and retirement/restoration of all three visuals on Leave and rejoin.
- The follow-up with source ID `9a531870ff22c2770f397f07c67f6bb2bd50e259` in
  `build/m25-guard-bow-5x4vetaa/` verified sustained guard, two parries, release back to
  idle, and grounded bow draw through stow. Client captures show guard and aim with the
  Soldier's gear. A standing jump landed before aim. The retry in
  `build/m25-guard-bow-4b3vfoxa/` verified switching back to the axe after the native attention
  state ended, bow remove/re-equip, and Leave and same-slot rejoin with all three gear slots.
- Windows app build, multiplayer core, direct invite, and WebRTC loopback passed.
  Local resource inspection verified eight equipment packs, preserved stock ActorInfo
  entries and found all 75 installed proxy clips.
- Native aim, reload and release overlapped Fall during controlled 6 m and 9 m drops.
  The proxy currently played Fall throughout each airborne sequence. An instrumented attempt
  to play BowAim on layer 1, slot 1 left that slot empty. Evidence is in
  `build/m25-guard-bow-t9gnto0b/` and `build/m25-guard-bow-9g15fuvx/`. Remote airborne
  aim and Linux gameplay remain unverified.
- A separate 30 FPS run in `build/m25-sword-guard-en520aka/` verified simultaneous Traveler's
  Sword and Soldier's Shield visuals during draw, guard, parry and attacks with ZL held.
  It reproduced the empty proxy upper slot after confirming the BowAim request string.
- In Soh Kofi Shrine (Dungeon070), a native enemy hit produced GuardFailHit while the
  host held up the Soldier's Shield. The receiver went from ShieldGuard to ShieldHit and
  back. Chaas Qeta Shrine (Dungeon082) reproduced the hit before the host died. The
  GuardBreak mapping passed synthetic dispatch. Evidence is in
  `build/m25-guard-encounter-tna9m6wi/` and `build/m25-guard-encounter-drsxy9xe/`.
- In `build/m25-guard-bow-ffvz5v53/`, both Windows games reached 30, 60, 90 and
  120 measured FPS. Soldier's Bow draw, aim, release and stow passed at every cap.
  Axe draw, ready and stow passed again after returning to 30 FPS.

## M2.5 Traveler's Bow

- Windows UCRT64 runtime objects and app link passed. Multiplayer core, Clang undefined-behavior
  trap mode, the eight-Link demo, eight-peer WebRTC loopback and eight-peer direct invitation
  tests passed. The mock lobby verified all three poses in one revision, replay to a late
  joiner and bow removal while weapon and shield identities remained active. The codec rejected
  a shield identity in the bow slot, an out-of-range bow pose and a bow pose without a revision.
- Local resource checks verified that the stock actor records remained intact with Link alone,
  Link and the bow, and Link with all three gear aliases. The changed runtime and protocol
  sources cross-compiled for Linux x86-64. The Windows and Linux Make targets reported the same
  multiplayer source ID, `670a3194f0b5a7ae7a407ddd0b9a7353f947450c`.
- Two Windows 1.9.0 scratch games used gamelib `13365c313591b27e`, direct WebRTC and Tah Muhl Shrine.
  The host used an RTX 4050 Laptop GPU (610.88) and the client used AMD Radeon Graphics (25.10.30.02).
  Both ran at 960×540, 50% render scale, 1400 MiB VRAM budget and Vulkan heap copy off.
- The host equipped a Wooden Shield, Traveler's Bow and arrows in the native inventory. The client
  rendered the bow stowed and aimed on the remote Link. The bow pose moved from roughly 9 cm
  behind Link to 39 cm forward during aim. The receiving controller selected reload, aim and release,
  and the existing axe draw, ready and stow clips still played when the host used the axe.
- Native bow removal and re-equip preserved both the remote axe and shield. Both games reached
  approximately 30, 60, 90 and 120 FPS at their corresponding targets. The client selected bow
  reload, aim and release at each rate without playing an axe draw during bow use.
- A native shrine exit retired all three gear visuals. Re-entry restored them in the same lobby.
  Client Leave and same-slot rejoin repeated the retirement and restoration. Captures and traces
  for the Windows gameplay checks are in ignored `build/m25-bow-3d8133ed/`.

## M2.5 shield, axe idle and crouch

- Windows UCRT64 runtime objects and app linked. The core, eight-peer WebRTC loopback and eight-peer
  direct invite tests passed with the two-slot wire layout. The mock lobby covered two poses in one
  revision, removal of a single slot, late join, and rejection of an identity in the wrong slot.
  Local resource checks preserved every stock actor record with Link alone, with either gear alias,
  and with both. The changed sources cross-compiled for Linux x86-64.
- Two Windows games on BOTW 1.9.0 used gamelib `13365c313591b27e`, direct WebRTC and scratch saves
  in Tah Muhl Shrine. The host used an RTX 4050 Laptop GPU (610.88) and the client used AMD Radeon
  Graphics (25.10.30.02). Both ran at 960×540, 50% render scale, 1400 MiB VRAM budget and Vulkan
  heap copy off. The main run measured 30 FPS.
- The host added a Wooden Shield to its scratch inventory using the existing item command, then equipped
  it from the native inventory. The native getter reported the shield in category one. The client saw
  the shield on the remote Link's back and in front when raised. Native Remove and Equip retired and
  restored that shield while both axes remained active. The client had no shield equipped.
- Both visuals retired during a native shrine exit and returned after re-entry in the same lobby.
  Client Leave and same-slot rejoin repeated cleanup and restored the host's equipment.
- Two draw/stow cycles selected AxeDraw, AxeReady, AxeStow and Idle on the receiving controller.
  The client also selected CrouchIdle and CrouchMove.
  Crouch and return to idle repeated at 60, 90 and 120 FPS targets. Measured rates were approximately
  60.0, 90.0 and 120.0 FPS on both games.
- The lifecycle and frame-rate run used source ID `0a537a8acd76d9e9aa33cdc22a4656d2bf2c7d1e`.
  Captures and traces are under ignored `build/m25-shield-5ohv11y3/`.
- The guard follow-up used current source ID `5b6399a29f68162f952aff5316c44ad03ed9216d`. The client
  selected ShieldGuard while the host held guard and returned to Idle after release. The raised
  stance is visible in `build/m25-shield-6ort3715/client/build/shield-raised.png`.
- A separate 30 FPS run on the same source ID confirmed both crouch clips. The crouched proxy is
  visible in `build/m25-shield-y216z_hl/client/build/remote-crouch.png`.
