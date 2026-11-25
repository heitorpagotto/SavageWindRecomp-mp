# Test record and verification plan

Reviewed 2026-11-24. The prototype and native build results below have separate verification scopes.

## M2.5 fall-damage recovery

- The Tah Muhl Shrine probe in `build/m25-hard-landing-an2p_5i_/` moved Link upward
  using the existing `player.pos` control cell and let the native fall resolve.
  The ordinary teleport command protects the player from fall damage for four seconds
  after arrival. Direct drops of 6 through 18 m kept 12 quarter-hearts and entered Land.
  The 21 m drop took four quarter-hearts and entered Fall, LandDamage, LandDamageStand
  and Wait.
- The verification run in `build/m25-hard-landing-xsbgdcyi/` used protocol 4, source ID
  `950097e37b41370878be23e6338b9dc9a1b99845` and gamelib `13365c313591b27e`.
  One remote Link played Fall, Land and Idle for each 6 m drop. Each 21 m drop played
  Fall, HardLand, HardLandRecover and Idle at 30, 60, 90 and 120 measured FPS on both
  Windows Vulkan clients. The sender lost four quarter-hearts per hard landing and the
  receiver's own health stayed at 12. The scratch runner restored the sender's
  health before each new cap. Receiver captures show impact and stand-up.
- The host used an RTX 4050 Laptop GPU (610.88) and the client used AMD Radeon Graphics
  (25.10.30.02), both at 960x540 and 50% render scale with a 1400 MiB VRAM budget
  and Vulkan heap copy off.
- The local resource check verified all 96 proxy states and all fifteen gear visual packs
  against installed 1.9.0 data. The synthetic action check distinguished ordinary
  landing from hard landing and recovery. Windows link, core, direct invite and WebRTC
  loopback passed.

## M2.5 hit reactions and knockdown recovery

- The Windows Vulkan scratch games used BOTW 1.9.0, gamelib `13365c313591b27e`, protocol 4
  and source ID `2c0271b4f42fc881ad5245655c8f9af28c171725` for the initial playback checks. The
  host used an RTX 4050 Laptop GPU (610.88) and the client used AMD Radeon Graphics
  (25.10.30.02). Both used 960x540 at 50% render scale, a 1400 MiB VRAM budget and Vulkan heap copy off.
- In `build/m25-reactions-enemy-kabvjotp/`, an unguarded Soh Kofi Shrine hit changed
  the host's native state to DamageM and dropped health from 12 to 2 quarter-hearts.
  The same remote Link selected DamageMedium and returned to Idle at 60 FPS.
- In `build/m25-reactions-bomb-r6byfsh3/`, a close Bomb Arrow blast at Tah Muhl Shrine
  entered native DamageL and the receiver selected Knockdown. The fatal hit continued
  into DeadRagdoll.
- The survivable armored replay in `build/m25-reactions-armored-1_mpvqet/` used source
  ID `1f30f098d12fb08eaeffd3767dff0476627fd914` and measured 30 FPS on both games.
  Health fell from 12 to 1 quarter-hearts. Native DamageL, SlippingDown,
  SlippingDownLand, DamageLStand and Wait reached the same remote Link as Knockdown,
  Tumble, TumbleLand, GetUp and Idle, in that order. The capture shows the remote
  knockdown. The stand-up selection was verified in the controller trace.
- The local resource check verified all 94 proxy states and fifteen gear visual packs.
  A synthetic action check covered light and medium hits, knockdown and stand-up
  selection while the bow's upper-body state still reported aim. Windows link, core,
  direct invite and WebRTC loopback checks passed.

## M2.5 grounded dodges

- The Windows Vulkan scratch games in Tah Muhl Shrine used BOTW 1.9.0, gamelib
  `13365c313591b27e`, protocol 4 and source ID `4fc921cb3b83f486928a8a0305dc9f77521f4245`.
  The host used an RTX 4050 Laptop GPU (610.88) and the client used AMD Radeon Graphics
  (25.10.30.02). Both used 960x540, 50% render scale, a 1400 MiB VRAM budget and Vulkan heap copy off.
- The receiving controller in `build/m25-dodge-live-35hivj8x/` selected the intended
  neutral, left, right, forward and backflip takeoff and landing clips at 30, 60, 90 and
  120 measured FPS. The trace recorded each pair once per dodge on one Link proxy.
  Leave retired the proxy and same-slot rejoin restored it.
- The closer 1280x720 capture at 100% render scale in `build/m25-dodge-profile-5zffty2m/`
  showed the remote Link airborne during the side hop and inverted during the backflip.
  The host measured 30 FPS and the client measured about 28 FPS at that cap. The stowed
  axe and shield can briefly separate from the body during the backflip.
- The local resource check verified all 88 proxy states against installed clips and all
  fifteen gear visual packs. The Windows app linked and the multiplayer core, direct
  invite and WebRTC loopback checks passed.

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

## M2.5 weapon families

- Two Windows Vulkan games in Tah Muhl Shrine (Dungeon000) used BOTW 1.9.0,
  gamelib `13365c313591b27e`, and source ID `d8f1cbe57e11d234264704b43eeae942dceb6140`.
  Both used scratch saves and settings, 960x540, 50% render scale, a 1400 MiB VRAM budget
  and Vulkan heap copy off. The host used an RTX 4050 Laptop GPU (610.88) and the client
  used AMD Radeon Graphics (25.10.30.02). The native observation is in
  `build/m25-weapons-jtk2dnpd/`. The receiving-game traces and captures are in
  `build/m25-weapon-live-pcclk2vu/`.
- At 30 FPS, native inventory swaps from axe to sword, spear and claymore retired only
  the prior weapon on the same remote Link. The receiver selected draw, drawn idle,
  stow, each normal chain hit, charge preparation, charged release and recovery to
  drawn idle for all three families. Spear charge also reached its final thrust.
  The drawn weapons are visible in the client captures. Draw and ready passed at
  60 FPS for sword, 90 FPS for spear and 120 FPS for claymore. Both games reached
  each measured target. Leave retired the weapon and same-slot rejoin restored it.
- The Windows app linked. Core, direct invite and WebRTC loopback tests passed. The
  local resource check verified the three passive packs, preserved stock ActorInfo
  records and found all 70 proxy clips in the installed animation archive. Linux
  gameplay validation is deferred to the owner.

## M2.5 animation clip progress probe

- Two Windows 1.9.0 scratch games used gamelib `13365c313591b27e`, source ID
  `c3c71f0f65dcf665b4f3ba417fd05ef3dc8ddf7f` and direct WebRTC at the 30 FPS cap near Oman Au.
  Both used 960x540 at 50% render scale, a 1400 MiB VRAM budget and Vulkan heap copy off.
  The host used an RTX 4050 Laptop GPU (610.88) and the client AMD Radeon Graphics (25.10.30.02).
- The read-only probe found matching frame counters on the native player and remote proxy.
  They advanced by about one frame per update and saturated at frame 10 for the standing
  jump. Both counters reset for the next activation. Walking also had advancing and wrapping
  counters. The native Move blend and the proxy's standalone walk clip advanced at different
  rates. Evidence is in `build/m25-traversal-527z85m4/`.
- The seek probe compared three received jumps. The first was a control. On the other two,
  the instrumented proxy wrote frame 7 into one candidate counter at about frame 2.
  The next update advanced to about frame 3 and overwrote the write in both cases. The three
  jumps returned to idle. The counters are observations and an in-place seek path remains open.
  Evidence is in `build/m25-traversal-esf2zqlh/`.
- The playback-start probe varied the second float argument to the existing play call over
  three received jumps. The default advanced from about frame 1 on the first observed update.
  Passing 0.5 advanced from about frame 6 on the first update and continued at about one
  frame per update. Passing 2.0 clamped to frame 10. The test confirms a normalized starting
  position for this clip. Evidence is in `build/m25-traversal-ctb9u26x/`.

## M2.5 lock-on swimming

- Windows UCRT64 runtime objects and app link, multiplayer core, eight-Link demo, eight-peer
  invitation and WebRTC loopback passed. Both changed runtime sources cross-compiled for Linux
  x86-64. Windows and Linux Make targets reported source ID
  `c3c71f0f65dcf665b4f3ba417fd05ef3dc8ddf7f`. The local pack check verified 52 entries and
  four new installed clip references. A scratch test with Clang undefined-behavior traps covered
  rotated headings, diagonal hysteresis, stopped movement, corrections and stale samples.
- A native ZL trace near Oman Au Shrine found SwimAtnMove and a fixed heading through all four
  input directions. Projected displacement matched the input directions. The trace is in
  `build/m25-traversal-z149o7t2/`. Tracking a selected moving target remains unverified.
- Two Windows 1.9.0 peers with gamelib `13365c313591b27e` used direct WebRTC with the same
  GPUs, drivers and graphics settings as the free-swim checkpoint below. At 30 FPS, the
  receiving controller played forward, backward, left and right strokes and returned to
  SwimWait between each. Waterline captures show the remote swimmer.
- Backward, forward and right strokes passed at 60, 90 and 120 FPS caps, with host/client
  measurements of about 60/60, 78/86 and 84/90 FPS. Releasing ZL restored free swimming.
  The receiver selected SwimFast and returned to SwimWait on the original remote Link.
  Gameplay logs and captures are in `build/m25-traversal-y9scfe2u/`.

## M2.5 swim stroke selection

- Windows UCRT64 runtime objects and app link passed. Multiplayer core, eight-Link demo,
  eight-peer direct invite and WebRTC loopback passed. The two changed runtime sources
  cross-compiled for Linux x86-64. Both Make targets reported source ID
  `d3d88b9e192aa7046ce55f95371214d6d65bf712`. A local pack check verified 48 entries and
  the five new references to the installed animation archive. A scratch check with Clang
  undefined-behavior traps exercised wrapped turns, hysteresis, stale samples and state changes.
- A native trace found about 0.27 m/s during a light forward stroke and about 1.9 m/s at
  full forward input, both under SwimMove. Left and right turns kept the same native state.
  Trace evidence is in `build/m25-traversal-_aef6ukk/`.
- Two Windows 1.9.0 games with gamelib `13365c313591b27e` used direct WebRTC near Oman Au Shrine.
  Both measured 30 FPS at 960x540, 50% render scale, with a 1400 MiB VRAM budget and Vulkan
  heap copy off. The host used RTX 4050 Laptop GPU (610.88) and the client used AMD Radeon
  Graphics (25.10.30.02).
- One remote Link played the six free-swim clips and returned to treading between strokes.
  The fast left and right captures show the proxy at the waterline. Native dash played on
  that actor and reached the shore, where the receiver selected Idle. The original route
  timed out waiting for another SwimWait after dash. The logs confirmed the shore exit and
  the route now expects Idle. Captures and logs are in `build/m25-traversal-xv6ytet5/`.
- The higher-cap route repeated fast forward swimming and return to treading on one remote
  Link at the 60, 90 and 120 FPS caps. Host/client measurements were about 60/60, 90/90,
  and 88/92 FPS. Evidence is in `build/m25-traversal-dbstd7g1/`.

## M2.5 glide banking

- Windows UCRT64 runtime objects and app link passed. Multiplayer core, eight-Link demo,
  eight-peer direct invite and WebRTC loopback passed. The presence source cross-compiled for
  Linux x86-64. Both Make targets reported source ID `495498ca1eebc9f5628d0537b71a2a21abb07c21`.
  A scratch check with Clang undefined-behavior traps covered wrapped heading, neutral and
  clamped bank, stale samples, position corrections and matching Link/canopy transforms.
- A local native trace during both steering directions found an upright player root and
  heading changes of about 20 degrees per second. Evidence is in `build/m25-traversal-ef96m6n9/`.
- Two Windows 1.9.0 games with gamelib `13365c313591b27e` used direct WebRTC near Oman Au Shrine.
  Both used 960x540, 50% render scale, a 1400 MiB VRAM budget and Vulkan heap copy off. The host used
  RTX 4050 Laptop GPU (610.88) and the client used AMD Radeon Graphics (25.10.30.02).
- At the 30 FPS cap the receiver applied about 14 degrees of tilt in both directions and returned
  to level between turns. Matrix traces confirmed matching yaw and roll and a canopy offset of 2 m
  along Link's tilted up axis. Both actor indices stayed fixed through the turns. Native midair
  cancel retired the canopy and played Fall and Idle. A right turn at the 60 FPS cap tilted both
  actors by about 14 degrees, and natural landing retired the canopy. The games measured about
  30/29 and 60/58 FPS. Evidence is in `build/m25-traversal-9_yjcvba/`.
- A client left during the follow-up flight. Leave removed both actors and same-slot rejoin
  restored them while the host was still airborne. Right turns at the 90 and 120 FPS caps
  tilted both actors by about 16 and 17 degrees respectively. The host/client measurements
  were about 85/70 and 95/95 FPS. Both landings retired the canopy and returned Link to idle.
  The camera from the pond missed the airborne actors. The matrix and lifecycle traces are in
  `build/m25-traversal-8coyqutq/`.
- The lower launch and camera on dry ground captured the airborne Link and canopy in both
  bank directions. The right turn landed on dry ground. The left turn reached the pond,
  retired the canopy and selected SwimWait. The original script expected Idle after that
  turn. The logs confirmed SwimWait and the route now expects it. Captures and logs are in
  `build/m25-traversal-o_2o73v0/`.

## M2.5 directional climbing jumps

- Windows UCRT64 runtime objects and app link passed. Multiplayer core, eight-Link demo,
  eight-peer direct invite and WebRTC loopback passed. Both changed runtime sources cross-compiled
  for Linux x86-64. Windows and Linux Make targets reported source ID
  `f6877769f778e5ad6dd4d41b96751c75d6caddc8`. A local pack check found 43 entries and all five
  climbing jump references in the installed animation archive. A scratch sampling check covered
  the five sectors, the upward fallback and keeping the chosen action through later movement.
- Two Windows 1.9.0 games with gamelib `13365c313591b27e` played over direct WebRTC at about
  30 FPS near Oman Au Shrine. Both used 960x540, 50% render scale, a 1400 MiB VRAM budget
  and Vulkan heap copy off. The host used RTX 4050 Laptop GPU (610.88) and the client used
  AMD Radeon Graphics (25.10.30.02).
- The host jumped right, up-left, up, left and up-right with native inputs. The receiving
  controller played the five corresponding clips exactly once each on the same remote Link.
  The up-right capture shows both Links and the remote jump against the wall. Logs and
  captures are in `build/m25-traversal-d0ac70tc/`.
- The initial higher-cap route timed out when teleporting directly from the wall after
  changing the cap. The repeat set up each climb at 30 FPS, changed the cap after attaching
  to the wall, and dropped back to the ground between trials. The same remote Link played
  right, up-left and up at the 60, 90 and 120 FPS caps. The host and client measured about
  60/57, 84/80 and 98/94 FPS respectively. Logs and captures are in
  `build/m25-traversal-u32enh6k/`.

## M2.5 gliding

- Windows UCRT64 runtime objects and app link passed. Multiplayer core, eight-Link demo,
  eight-peer direct invitation and WebRTC loopback passed. Linux x86-64 cross-compilation passed for
  the changed runtime, archive and RomFS sources. Windows and Linux reported source ID
  `c4f6bceacb6af307ab84ace438d5218871469310`.
- A synthetic archive test rejected a cut metadata prefix, an out-of-range member and a missing
  member. A local resource check built the animated canopy alias and preserved all stock actor records
  with the five aliases present. The save converter passed its archive fixtures, including the
  truncated Bootup pack case. Its standalone Saves tab and non-ASCII folder checks were skipped.
- Two Windows 1.9.0 games with gamelib `13365c313591b27e` played over direct WebRTC near Oman Au Shrine
  at approximately 30 FPS. Both used 960×540, 50% render scale, 1400 MiB VRAM budget and Vulkan heap
  copy off. The host used an RTX 4050 Laptop GPU (610.88) and the client AMD Radeon Graphics
  (25.10.30.02).
- The host opened the paraglider during a fall. The client selected the deploy and glide clips and
  rendered the canopy above the remote Link. Midair cancel retired the canopy and the proxy
  selected Fall, Land and Idle. A second native glide ended on dry ground with no cancel input,
  retired the canopy and returned the proxy to idle. Evidence is in `build/m25-traversal-7fn4xuou/`.
- Leave during flight retired the remote Link and canopy. Rejoining in the same slot during the
  same flight restored the canopy. Landing retired it again. Evidence is in
  `build/m25-traversal-iwc0gye7/`.
- A closer side capture at the 60 FPS cap shows the remote canopy open above Link's hands.
  The host measured 58.5 FPS and the client 60.0 FPS. The canopy retired after normal landing.
  Captures are in `build/m25-traversal-g182s5tq/`.

## M2.5 directional wall climbing

- Windows UCRT64 runtime objects and app link passed. The multiplayer core, eight-Link demo,
  eight-peer direct invitation and WebRTC loopback tests passed. Linux x86-64 cross-compilation
  passed for the changed sources. A local resource check verified all 33 animation entries,
  including eight movement directions and both new wall transitions.
- Two Windows games on 1.9.0 used gamelib `13365c313591b27e`, source ID
  `2fb96f88f354fd5544af2c78f21540288cfe8f50` and direct WebRTC near Oman Au Shrine, with the
  same GPUs, drivers and graphics settings as the swimming and climbing checkpoint.
- At the 30 FPS target the receiving controller selected wall approach, all eight requested
  directions, wall hold and climbing jump. The native and remote captures show lateral climbing
  on the same wall. The client also selected right at approximately 60 FPS, left at 90 FPS,
  and up at the 120 FPS target with 99.3 FPS on the host and 98.3 FPS on the client.
  The proxy returned to idle after the host let go and landed. Evidence is in
  `build/m25-traversal-97uxmfan/`.
- A separate 30 FPS ascent reached the ledge. The local player entered ClimbEd and the
  receiving controller played ClimbMantle. A repeat captured the proxy pulling up and standing
  on top of the wall after the host released the stick. Traces and captures are in
  `build/m25-traversal-n4gg3i19/` and `build/m25-traversal-p0jg4z5w/`.

## M2.5 swimming and climbing

- Windows UCRT64 runtime objects and app link, multiplayer core, eight-Link demo, eight-peer
  WebRTC loopback and direct invitation tests passed. A local resource check verified all 24
  animation pack entries and the references for the six traversal clips. Linux x86-64 runtime
  objects cross-compiled. Both Make targets reported source ID `b6cc1385595f980904c71878c84b5b2e44ce7bb8`.
- Two Windows 1.9.0 games using gamelib `13365c313591b27e` held a direct WebRTC lobby
  near Oman Au Shrine. Settings were 960×540, 50% render scale, 1400 MiB VRAM budget and
  Vulkan heap copy off. Both games reached the 30 FPS target after shader warmup. The host
  had an RTX 4050 Laptop GPU (610.88) and the client had AMD Radeon Graphics (25.10.30.02).
- The host entered the pond, swam, stopped and dashed. The receiving controller selected
  SwimMove, SwimWait and SwimDash. At the masonry wall it selected ClimbMove, ClimbWait and
  ClimbJump, then returned to Idle after the host let go and landed. The same Link remained
  active throughout. Initial action traces are in ignored `build/m25-traversal-iiwud2hw/`.
- Close-up comparisons exposed a submerged proxy on the first build. The native matrix and
  PlayerInfo position agreed for all sampled swim actions. The lift comparison in
  `build/m25-traversal-rpjly6_x/` tested the treading stance at several heights before choosing
  a 1.65 m model-only correction.
- The corrected build repeated the six-state route at approximately 30 FPS on both games.
  The client captures show the remote Link at the surface while treading, swimming and
  dashing, and against the masonry during a climb. The final logs and captures are in
  `build/m25-traversal-i9v68ip5/`.

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

## M2.5 passive axe and animation

- Windows UCRT64 runtime objects and app linked. Multiplayer core, eight-peer WebRTC loopback, and
  eight-peer direct invite tests passed. The mock lobby checked forged equipment ownership, revised
  identity and pose relay, late join, unequip, and equipment cleanup on slot reuse. The changed C
  sources cross-compiled for Linux x86-64. A local resource check verified both alias configurations
  preserve the stock actor records, the axe uses the passive profile and dummy physics, and the Link pack
  contains all ten animation entries.
- Two Windows games used BOTW 1.9.0, gamelib `13365c313591b27e`, and direct WebRTC loopback in Tah Muhl
  Shrine. Settings were 960×540 at 50% render scale, 1400 MiB VRAM budget, Vulkan heap copy off, and
  30 FPS target and measured. The host ran on the NVIDIA RTX 4050 Laptop GPU (610.88) and the client
  on AMD Radeon Graphics (25.10.30.02). Both games used scratch saves and settings.
- The client saw the host's axe move from about 1.43 m stowed to 0.87 m drawn on revision 1.
  Client captures show both placements with the client's own axe still stowed. The passive axe
  capture had no Take prompt. The remote animation controller reported AxeDraw, AxeSwing, AxeFinish,
  AxeSpin, Jump and Land. This checks the selected states. Exact clip timing and drawn idle pose remain open.
- The host used the native inventory to remove and equip the axe. The client capture shows the remote
  axe gone with the remote Link and the client's own axe still present. Re-equip restored the remote
  visual on revision 3. A native shrine exit retired both remote axes. Re-entry restored both in the
  same lobby. Client Leave and same-slot rejoin retired and restored both visuals again.
- Scratch evidence is in `build/m25-revision-wire-eqcu56sr/`, with client captures in `client/build/`
  and the controller and lifecycle traces in each `run.log`. Linux gameplay, a second rigid slot, and
  skinned equipment remain open.

## M2.5 first slice

- Windows UCRT64 runtime objects and app link passed. The multiplayer core, WebRTC loopback, direct
  eight-peer relay, native-clock fixture, 52 game-site checks, and 59 layout checks passed. The UI-text
  check found no missing entries. Linux x86-64 cross-compilation passed for the changed runtime and UI sources.
- The mock lobby check covered the initial environment on late join, an outdated roster epoch, a client
  attempting to send environment state, host withdrawal, and one admission notice per join or rejoin.
- Regeneration resolved 189 sites for BOTW 1.9.0 and produced Windows x86-64-v3 gamelib
  `13365c313591b27e`. The current application source ID is `7e16c3fb15753cf38ba9ed5e7c543212bc4b4306`
  on Windows and Linux. The build reused 1,058 objects whose regenerated translation units were byte-identical.
- The two-game Windows WebRTC run with that library used the same GPU and display settings below.
  The client started about 13 in-game hours behind. It adopted the untouched host's clear weather,
  advanced with the host's clock, and showed rain after the control probe requested it on the host.
  Both rain captures display 06:05 PM and matching rain indicators. Client and host opt-out each returned
  control to the client's local clock and weather. Re-enabling synchronization converged again. The
  rejoin around midnight recovered immediately. The host captured one notice per admission above the
  game with F1 closed. The existing-client notification is covered by the mock lobby test.
  Scratch evidence is in `build/m25-live-wop3o6bw/`, including `host/build/frames/window_01170.png`.
- To repeat the two-game check, use the ignored `build/m25_build_probe.py` and
  `build/m25_run_probe.py`. The scratch executable reuses the production build and adds a local
  file-backed clipboard, setting toggles, and trace messages. The production app and local launch script
  are in `build/m25-build/`. Use scratch saves and settings for control commands.
- A two-game Windows shrine round trip kept both players in the lobby. The client followed the host
  while both were in the shrine. After a native elevator exit, the client and host kept different clock
  values. The client converged again after returning to the shrine. The probe is
  `build/m25_run_travel.py`. The final build's logs are in `build/m25-travel-bwm4nvpt/`.
- The initial two-game Windows run used BOTW 1.9.0 on the Plateau, a 30 FPS target and measured 30 FPS,
  960x540 at 50% render scale, 1400 MiB VRAM budget, and Vulkan heap copy off. The host used the NVIDIA
  RTX 4050 Laptop GPU (610.88) and the client used AMD Radeon Graphics (25.10.30.02). With the old
  gamelib the clocks converged from 18:00 vs 05:00, progressed together, and converged after a rejoin
  around midnight. Forced clear, rain, and cloudy followed the host. Client opt-out restored local
  control. The scratch logs are under ignored `build/m25-live-3p_nfe1g/`.
- The client keeps its native clock speed and progression gate. A client before the paraglider can still
  hit the native 11:00 cap between samples when the host is past it. Verify mixed progression before
  claiming full clock fidelity. Native Linux and mixed-platform environment checks remain open.
- Disconnect notices passed in two 30 FPS Windows games using gamelib `13365c313591b27e` and source ID
  `538ba47fbd6d1889fc1fff8728b9714a85a6cc8f`. Client Leave produced a named notice on the host and a
  local notice on the client. After rejoin, host Leave produced a local notice on the host and both
  notices on the client. The client Leave screenshots with F1 closed are under
  `build/m25-disconnect-unbcilnp/`, in each player's `build/frames/window_01365.png`.
  The mock lobby also checked the departed name on a surviving client after same-slot rejoin.
  The multiplayer, WebRTC, direct and UI-text checks passed, and the changed sources cross-compiled for Linux.

## M2.5 equipment probe

Two Windows scratch games in Tah Muhl Shrine used BOTW 1.9.0, gamelib `13365c313591b27e`, direct
WebRTC loopback, 30 FPS target, 960x540, 50% render scale, 1400 MiB VRAM and Vulkan heap copy off.
The GPUs and drivers match the environment test. Both native stock axe actors activated and both
renderers applied equipment transforms from the other game. Only the host drew their axe and only
the client's remote visual moved from the stowed to drawn placement. The client still showed its
own axe stowed. The stock actor exposed Take, and the fixture does not cover equip changes or actor
retirement. See the [equipment handoff](HANDOFF.md#m25-equipment-probe) for scripts and evidence.

## M1 developer build

Use MSYS2 UCRT64 on Windows or the project Linux/Vulkan environment. Install Git, CMake, Ninja, and OpenSSL
development files. The dependency script builds libdatachannel v0.24.6 with libjuice. Custom flags use
MP_RTC_CFLAGS and MP_RTC_LIBS.

~~~sh
CC=clang CXX=clang++ sh tools/build_multiplayer_deps.sh
export MP_RTC_PREFIX="$PWD/build/multiplayer-deps"
export PATH="$MP_RTC_PREFIX/bin:$PATH"
export LD_LIBRARY_PATH="$MP_RTC_PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
make B=build/m1 -j8
make B=build/m1 test-multiplayer test-multiplayer-webrtc test-multiplayer-direct
python3 tools/i18n_check.py
~~~

The historical HTTPS fixture uses libcurl 7.85 or newer and runs via make test-multiplayer-rendezvous.
The game executable does not use that fixture.

This recipe covers source builds. Release package dependency staging and license notices are tracked under
[M6](MILESTONES.md#m6--release-hardening).

### Direct invite and reply

The Windows standalone test exchanged the actual invite and reply messages for seven clients in one lobby.
It rejected malformed invites, old replies after a new invite, and replay of a consumed reply. It verified
invite timeout, a full roster, START, and cleanup when the host left. The Windows application and both menus
compiled. Two Windows processes also exercised the production runtime's Host, Join, Copy, Paste reply, and disable
actions with an isolated clipboard fixture. The direct transport objects cross-compiled for Linux x86-64.
Native menu interaction, gameplay across two external networks, and Linux/Vulkan execution remain pending. The older
gameplay evidence below used HTTPS signaling.

The staged Windows build used OpenSSL 4.1.0. See the [security findings](DIRECT_MULTIPLAYER_HANDOFF.md#security-findings)
for the applicable DTLS advisory, the configured DTLS minimum, and the checks still open.

### Direct lobby field reports

- Windows host to Linux client returned the incompatible-build error. The inspected Windows executable
  embedded the fingerprint of the current multiplayer sources combined with the previous Git tree.
  Rebuilding the same sources from commit `007973d` computed a different fingerprint with the old formula.
  The fingerprint now uses source contents with normalized line endings. Make produced identical IDs for
  Windows and for a Linux target with Git metadata hidden. Rebuild both platforms from the updated
  multiplayer sources for the next test.
- The owner reported a Linux-to-Linux join that succeeded with the friend hosting and stalled with the owner
  hosting. In the stalled direction the old host UI could show **Connected** as soon as it accepted the
  pasted answer. Both menus now show progress until a peer is admitted. The client switches from the reply
  prompt to connecting at the lobby handshake. The cause of the failed connection remains unverified.
- The updated Windows executable linked. The core, WebRTC loopback, and eight-peer direct tests passed.
  The direct test checked that the accepted reply's connection was assigned to the expected peer.
  The UI text check found zero missing or unused entries. Native Linux and external-network retests
  of these changes are pending.
- For the next run, use a fresh invite in each host direction. Confirm both logs show the same multiplayer
  source ID and BOTW build ID, then check whether both rosters contain both players. For a failure, collect
  the `[multiplayer]` log lines from both machines, including whether the host applied the reply and whether
  the client sent the lobby handshake. Keep the clipboard codes private.

The owner confirmed the updated Windows and Linux builds both joined a friend hosting on Linux.
The reverse host direction still failed. The owner accepted M1 and approved starting M2 while
investigating the firewall or direct connection separately.

At the M2 checkpoint `7991b79`, the owner confirmed a Windows client joined the Linux host after
launching the matching Windows build. The original Windows executable had a stale multiplayer
source ID. The [session handoff](HANDOFF.md#windows-and-linux-lobby-compatibility) records the IDs,
the verified launcher, and the outstanding host-direction report.

### Native build and standalone checks

- MSYS2 UCRT64 with Clang 23.1.5 built the Windows executable and Vulkan runtime objects against libdatachannel
  0.24.6 and libcurl 8.23.0. The regenerated BOTW 1.9.0 library matched ID `6bb7f77965924e25` at x86-64-v3.
- The multiplayer runtime, settings, and both UI adapters cross-compiled to Linux x86-64 and aarch64 objects.
  Native Linux/Vulkan execution and mixed-platform tests are pending.
- The Windows synthetic core and demo passed, with 560 snapshot deliveries and one stale duplicate rejected.
  Eight Windows WebRTC peers passed the loopback lobby and HTTPS rendezvous tests, including untrusted TLS rejection.
- The game-site and layout suites passed 50 and 59 checks. The translation check found zero missing or unused entries.

### Native Windows gameplay with the HTTPS prototype

- With separate scratch settings and saves, a fresh game booted with multiplayer off. The F1 tab appeared with
  its toggle off and showed Host and Join after enabling it. System > Options opened the Multiplayer page,
  enabled multiplayer, hosted and displayed a 24-character invite. On that same run F1 reflected the enabled
  preference. The INI recorded both on and off changes.
- On the Plateau, the Windows renderer showed one `Npc_TripMaster_00` after the first peer joined the
  certificate-verified loopback lobby, then two after a second peer joined. A deliberately incompatible source
  identity was rejected and did not acquire a slot. The renderer used a 30 FPS target, 960×540, 50% render scale,
  a 1400 MiB VRAM budget, and the NVIDIA RTX 4050 Laptop GPU with driver 616.92.
- The first headless peer disconnected and its actor was retired. A late-joining Windows game reused its
  slot and rendered the host and remaining peer from its initial roster. The host rendered two actors again.
  The late client used the same frame, resolution, and memory settings on AMD Radeon driver 25.10.30.02.
  Its native Join action used the test keyboard response for the invite.
- Disabling multiplayer through the host's System > Options page disconnected the other peers and retired
  both actors on each renderer. Screenshots showed the Plateau clear on both games after resuming play.
- A separate run hosted from F1 and admitted one peer. An in-place game reboot ran the reset hooks, returned to
  the title sequence, and dropped the observing peer to an empty roster. A fresh process loaded the saved
  enabled setting and displayed the offline Host and Join controls.
- The scratch evidence is under the ignored `build/m1-smoke/` directory when available.

### Prototype transport and UI

- Windows checks used Zig 0.15.2, libdatachannel 0.24.6, and libcurl 8.23.0. Eight encrypted WebRTC loopback
  peers passed roster, rejoin, capacity and build rejection, START, state and event delivery, and disconnect.
- Eight Windows peers passed HTTPS offer/answer and DTLS/SCTP. An untrusted TLS certificate was rejected.
  A controlled local libjuice TURN test dropped direct UDP traffic and observed a selected relay candidate.
- Two live Windows instances hosted and joined from the F1 overlay. Host reboot cleared the client roster.
  A deliberately incompatible app build was rejected with a visible Brazilian Portuguese message.
- Linux x86-64 and aarch64 runtime, UI, and test sources cross-compiled. Native Linux/Vulkan gameplay,
  mixed-platform execution, and two external networks remain pending.

## M2 visibility follow-up

- Field report on `7991b79`: the Windows host and Linux client joined but saw neither proxy. In the reverse
  setup a Linux client initially saw the Linux host. A Windows client joined at a different location. After
  the Linux client used Slate fast travel, it saw neither. Locations, logs, and the lobby state after the
  warp were unavailable, so the platform-specific cause is open.
- The follow-up Windows app and configuration launcher linked with Clang 23.1.5. The core, eight-peer direct,
  eight-peer WebRTC and place tests passed. The changed runtime units cross-compiled for Linux x86-64.
  Both Make targets produced source ID `3344f886df6a2e0a8a90d4dedddc1cc95f37380e`. The Windows target
  matched the installed 1.9.0 gamelib ID `e640fd858dc0d033` at x86-64-v3.
- In two Windows 1.9.0 Vulkan instances connected through the direct lobby, the position-control probe kept
  both proxies active at about 30 m. It withdrew both at about 145 m and restored both on return without a
  rejoin. The return screenshot showed both Links. This run used the position control endpoint. Evidence is in ignored
  `build/m2-travel-live-2c_8l727/`.
- The final Windows animation probe reported the native Idle, Walk, Run, Dash and Jump states playing on the
  receiver. The Run and Dash input tests used full left-stick travel without and with the sprint button.
  The alias resource check also passed for the synthetic and installed actor tables and the five clip records.
  Logs and captures are in ignored `build/m2-travel-live-v0ojcsbv/`. Both runs targeted 30 FPS at 960×540,
  50 percent render scale and a 1400 MiB VRAM budget on NVIDIA RTX 4050 Laptop (616.92) and AMD Radeon
  Graphics (25.10.30.02), with Vulkan heap copy off.
- The final build also passed a native System > Load test with the same Windows configuration. One client
  loaded a scratch autosave while both were in the lobby. Both sides retired their original proxies and
  reported fresh native actors when the client returned to the overworld. Both retained the original lobby.
  Evidence is in ignored `build/m2-travel-live-v_njtp_n/`.
- On 2026-11-23, two Windows scratch clients connected over direct WebRTC with version
  `0.6.0-alpha.6-0.1.0-mp`, source ID `3344f886df6a2e0a8a90d4dedddc1cc95f37380e` and BOTW 1.9.0
  gamelib `e640fd858dc0d033`. The probe used isolated clipboard automation and copies of a save with
  Slate travel unlocked. The client used the native Slate map to travel to Great Plateau Tower, then
  back to the Shrine of Resurrection. The two-player lobby remained admitted through both loads.
  At about 92 m separation after returning, both logs reported actors repeatedly becoming inactive
  and respawning. After the position-control probe put the two Links about 4 m apart outdoors, each
  visibly rendered the other in the original lobby. Sustained visibility near 92 m remains open.
  Both ran at a 30 FPS target, 960×540, 50 percent render scale, 1400 MiB VRAM budget and heap copy off
  on NVIDIA RTX 4050 Laptop (616.92) and AMD Radeon Graphics (25.10.30.02). Logs and captures are in
  ignored `build/m2-slate-kcjjyehi/`. The native mixed-platform retest remains open.

## M2 release version check

- Windows and Linux Make targets print `0.6.0-alpha.6-0.1.0-mp` via `print-version` and report the same
  multiplayer source ID as the preceding visibility build. The macOS Make target prints `0.6.0-alpha.6`.
  The Windows ZIP dry run uses the combined label for the filename and manifest. The Linux launcher
  translation unit cross-compiled for x86-64, and the AppImage packaging script passed its shell syntax check.
- The Windows app linked and displayed the combined label in Configuration and in the startup log with
  BOTW 1.9.0 and x86-64-v3 gamelib `e640fd858dc0d033`. The Windows multiplayer core, eight-peer WebRTC,
  eight-peer direct and updater core tests passed. The translation check reported zero missing or unused strings.
  The Windows capture and logs are in ignored `build/m2-version-smoke-dotx5361/`. Native Linux execution and
  full package creation are pending.

## M1 Windows presence check

The test area is outside the Shrine of Resurrection, near the `plateau` test warp. The placeholders are static
`Obj_RockGray_A_M_01` rocks placed above the grass in front of the default camera. Each instance uses scratch settings
and a separate scratch save. The installed game files are inputs to generation and launch.

- The native Windows scratch probe completed the async request, took the suspended actor, retained its indexed
  identity, and queued native deletion. Screenshots showed the rock in the world and then gone while the local
  player remained active. The run used BOTW 1.9.0, 1280x720, a 60 FPS target and NVIDIA RTX 4050 Laptop GPU, driver 616.92.
- The rebuilt session core passed the same-name slot reuse regression. The observing client processed PEER_LEFT and
  PEER_JOINED in one poll. Both the observer and host reported a new generation, and the late client had the complete
  remote roster from WELCOME.
- The Windows core, demo,
  eight-peer WebRTC loopback, HTTPS rendezvous, and controlled TURN tests passed. The UI text check found no missing
  or unused entries. The presence source cross-compiled for Linux x86-64 and aarch64.
- The synthetic game-site suite passed 50 checks, including a missing or mismatched signature disabling the complete
  presence group and preservation of the replaced guest stack stores. The layout suite passed 59 checks.
- The Windows app and regenerated guest library matched ID `6bb7f77965924e25` at the x86-64-v3 target. Two game
  instances of the prototype each reported one actor, then each reported two after a third
  client was admitted. In the single-GPU attempt, the first client exited with `vkWaitSemaphores: -4 (device lost)`.
  The split-GPU retry used the NVIDIA RTX 4050 Laptop GPU (616.92) and AMD Radeon (25.10.30.02). The workspace restart
  interrupted that run after the late client reached three admitted peers and before it logged its placeholders.
- A follow-up used one live game instance and two headless Windows peers with the same WebRTC and HTTPS transport.
  The live game joined late and reported both earlier peers from WELCOME. A mismatched peer was rejected, the
  departing peer's actor disappeared, and a same-name replacement produced one actor in the reused slot.
  Screenshots confirmed one rock after disconnect and two after rejoin. Reboot with a live connection returned
  the game to the title and removed the client from the host's roster. The live renderer used 1280x720 at 50%
  render scale, a 60 FPS target, a 1400 MiB VRAM budget, and the NVIDIA adapter with Vulkan heap copy off.
- In a separate live run, selecting Leave lobby removed the host's placeholder while the local player continued
  in the world.
- A split-GPU retry at 960x540, 50% render scale and a 30 FPS target reached two ready actors in all three games.
  The host and first client screenshots showed both rocks, then one each after the late client disconnected.
  A final host screenshot showed no placeholders after the first client disconnected. The late client's
  screenshot was still black with the loading indicator even after the draw count rose and both actors logged ready.
  Evidence is in `swmm-three-renderers-fiyqz87u` under the Windows scratch root.
- A follow-up at the same settings waited until the Plateau was visible before taking each screenshot. The host
  and late client used the RTX 4050 Laptop GPU (616.92), and the first client used AMD Radeon (25.10.30.02).
  All three games ran BOTW 1.9.0 with gamelib `6bb7f77965924e25`, scratch saves, a 1400 MiB VRAM budget, and
  Vulkan heap copy off. Each `build/two-peers.png` under the `host`, `first`, and `late` directories in
  `swmm-three-renderers-fjiu6mal` shows two rocks, one per admitted remote peer. The late client's screenshot
  shows both earlier peers during the fade-in. The surviving worlds showed one rock after the late client
  disconnected, and the host showed zero after the first client disconnected. The runner exited successfully.
- The multiplayer core also passed Clang's undefined-behavior trap check.

### Prototype evidence on the Windows development machine

When available, the ignored `build/mp-work/m1-runtime-scratch` directory contains the earlier logs and
screenshots. The old runners target the prototype activation path. Use the native Settings toggle when testing
the current executable. Wait until the Plateau renders before counting actors. A readiness log can arrive
during the loading screen.

## M2 acceptance follow-up

The owner accepted M2. Remaining manual checks are tracked under [M7](MILESTONES.md#m7--deferred-manual-validation).

- Source ID `e00ea2478251cec97481ac59c2a297346ec1b855` and BOTW 1.9.0 gamelib
  `e640fd858dc0d033`. Windows runtime and app linked. The core, direct WebRTC, transport loopback,
  and place checks passed. The extended synthetic test verifies that reordered position, facing,
  shrine area, action and health converge and that a lost withdrawal expires.
- Two Windows Vulkan games at 30 FPS held both native proxies for 30 seconds at about 48 m.
  In the boundary check, both persisted at about 58 m, withdrew once at 65 m, stayed withdrawn
  at 57 m, and returned at 48 m within the original lobby. Both used 960×540, 50% render scale,
  heap copy off and a 1400 MiB budget. GPUs were NVIDIA RTX 4050 Laptop (616.92) and AMD Radeon
  Graphics (25.10.30.02). See `build/m2-accept-range-80pd8pxf/` and `build/m2-accept-range-m9sytrxx/`.
- One Windows renderer in a shrine kept all seven remote proxies active during a 60-second hold
  after START. Six fixture peers joined after START. The fixtures varied sample intervals and omitted
  every third sample. The renderer received changing health and played all five clips. It kept the
  seven actors at measured 60, 90 and 120 FPS for about ten seconds each, then retired them on
  disconnect. This used RTX 4050 Laptop (616.92), 640×360 at 50% scale and the same heap settings.
  Evidence is in `build/m2-accept-capacity-qpenukgh/`. A session with eight actual game clients remains open.
- Four full Windows Vulkan games on BOTW 1.9.0 and the same source ID held the Shrine of Resurrection
  lobby for 90 seconds. The local probe supplied isolated clipboards and extra log markers. Two clients
  joined after START. Each game logged three distinct active remote
  actors throughout the hold at 30.0 measured FPS. Three client screenshots each show all four Links.
  Leaving cleared all three proxies on the departing client and only that client's proxy on the remaining
  games. Settings were 640×360, 50% render scale, heap copy off and a 650 MiB VRAM cap, with two games
  each on RTX 4050 Laptop (610.88) and AMD Radeon Graphics (25.10.30.02). Data, settings and saves were
  isolated per process. Each renderer used about 4 GiB of Windows commit. The 32 GiB host had 1.89 GiB
  of physical memory and 4.28 GiB of commit available before the fifth launch, so the harness held at four.
  Logs and screenshots are in `build/m2-accept-eight-6yslv0ug/`. Eight full games still require more hardware.
  Separate attempts to use the existing Windows sparse heap option failed host-memory import on both adapters.
  This kept the soak on the default heap path.
- Ubuntu 22.04 x86-64 in a local VM built the pinned libdatachannel with Clang 21.1.8 in rootless
  Podman. The multiplayer core, eight-peer direct invite and eight-peer WebRTC loopback tests passed.
  Linux and Windows Make targets reported the same release label and the same source ID shown above.
  See `build/m2-linux-vm/linux-tests.log`.
- The source-only Linux package run built the generator helper and SDL, then reached FFmpeg compilation.
  The VM was shut down at the owner's request before an AppImage was produced. The owner will verify
  the AppImage manually. See `build/m2-linux-vm/linux-build.log`.
- The source-only Windows release built with the project's pinned LGPL FFmpeg and produced
  `build/dist/SavageWind-0.6.0-alpha.6-0.1.0-mp-windows.zip`. ZIP integrity and the manifest passed.
  The included libdatachannel, FFmpeg, and OpenSSL DLLs resolved from the staged package with the
  UCRT64 and custom dependency paths removed from PATH. The archive excluded generated game
  libraries, extracted content, and saves. The Linux Make target reported the identical label.
- Two 30 FPS Windows peers completed two native shrine round trips in the same lobby. The client
  took the elevator out and back twice. Both proxies retired on each exit and reactivated on each
  return. The test used scratch saves with the shrine gate unlocked before Continue. Evidence is
  in `build/m2-accept-roundtrip-oa3qggwr/`.
- Two 30 FPS Windows peers passed a local direct WebRTC run with a test-only libjuice build.
  After admission, it dropped every fifth outbound UDP datagram in both processes and scheduled
  variable delays of 0–45 ms on the others. Both directions converged on movement, facing and
  walk/jump/idle while keeping the same proxy actor. The host reported the client's 12 → 9 → 12
  quarter-hearts while its own health stayed at 12. Evidence is in `build/m2-accept-udp-zuvr0y3n/`.
  The production app and Windows ZIP use the normal dependency.

## M2 shrine identity and native retry

- The Windows UCRT64 app and runtime objects built with Clang 23.1.5. The core, eight-peer direct
  and eight-peer WebRTC suites passed. The place test passed with the locally generated name table,
  and a synthetic test passed without it. Both changed runtime units cross-compiled for Linux x86-64.
  Source ID `cfca0ce8eae2beffbe4b682fc9d176d656f66d85` uses the existing x86-64-v3 BOTW 1.9.0
  gamelib `e640fd858dc0d033`. Windows and Linux Make targets reported the same multiplayer source ID.
- Two Windows 1.9.0 scratch instances joined over direct WebRTC after starting in the same shrine.
  Both activated a native remote Link under area ID 2. A second pair started in different shrines
  near the same local coordinates. They stayed admitted under area IDs 2 and 3 with no proxy spawned
  on either side during the eight-second check. Evidence is in ignored `build/m2-area-live-w93wah52/`
  and `build/m2-area-live-ng6llt5x/`. These runs used the test warp at Continue.
- The earlier Windows build retained both proxies at about 30 and 60 m on the open Plateau. At about
  92 m it logged four native actor removals per peer in an eight-second window without a scene change.
  With the revised retry policy, the native actor still went inactive at 92 m and each side recorded
  a single removal with no repeat spawn in the observation window. Both sides recovered at about
  4 m in the same lobby. See `build/m2-area-live-123f2cxy/` and `build/m2-area-live-lweyuimp/`.
  The range tests used the position control endpoint. A scratch write watch then traced the flag
  change to the native delete routine on both peers before multiplayer retirement. The native condition
  that triggered it remains open. See `build/m2-area-live-xb_xb_z1/`.
- Captures on both Windows instances showed local and remote Links in the same shrine. The client used
  the native elevator to return to the overworld while the host remained inside. The client went through
  areas 2, 0 (loading), and 1. Both proxies retired while both peers remained in the lobby. Evidence is in
  ignored `build/m2-area-live-8sad4vr6/`.
- In a separate native entry run, the client joined from the main field while the host was inside that shrine.
  The client activated the outside terminal and took the elevator. Both activated a remote Link after
  the client entered area 2, without another lobby admission. Evidence is in `build/m2-area-live-78oyxryd/`.
- These Windows tests targeted 30 FPS, 960×540 at 50 percent render scale and a 1400 MiB VRAM budget,
  using Vulkan with heap copy off on NVIDIA RTX 4050 Laptop (616.92) and AMD Radeon Graphics
  (25.10.30.02). Repeated shrine round trips, mixed-platform gameplay and longer sessions remain open.

## M2 Plateau position check

- BOTW 1.9.0, Windows Vulkan, 30 FPS target, 960x540 at 50% render scale, 1400 MiB VRAM budget,
  and Vulkan heap copy off. The host used an RTX 4050 Laptop GPU on driver 616.92 and the client
  used AMD Radeon Graphics on driver 25.10.30.02. Each instance had a separate scratch save and settings.
- Two live instances exchanged direct WebRTC invites and replies using an isolated clipboard. Both
  rendered a remote Link in the bind pose. Walking and jumping moved the active proxy without
  respawning. Client leave removed both proxies. A fresh invite reused the slot and host reboot
  removed the client's proxy. Evidence is under the ignored `build/m2-live-nv1jx2av/` directory.
- The follow-up compared the local players' and proxies' native matrices before and after turns.
  Facing converged in both directions on the original actor indices. The client could open and
  close inventory while the host remained in play. At a 60 FPS target, leaving and returning to the
  Plateau test bounds withdrew and restored both proxies. The two-instance run reached about
  65 FPS at a 90 FPS target and 83 FPS at a 120 FPS target. Evidence is under
  `build/m2-facing-8wcx5jxf/`.
- The Windows app and regenerated library `a1c9d2072a418844` built at x86-64-v3. The 50 game-site
  checks, 59 layout checks, multiplayer core and eight-peer WebRTC/direct tests passed. Linux
  x86-64 runtime, presence, asset and RomFS objects cross-compiled.

## M2 Plateau animation check

- Regenerated the 1.9.0 x86-64-v3 library `34aa482dae306a0d` and built the Windows runtime.
  The 52 game-site checks, 59 layout checks and Linux x86-64 cross-compilation of the proxy
  sources passed. The eight-peer direct test relayed action IDs. The synthetic lobby test dropped
  one update and reordered two more, then checked that the newest action reached the observer.
- Two Windows Vulkan games at a 30 FPS target, 960x540 at 50% render scale, and a 1400 MiB VRAM
  budget played idle, walk and jump on both receivers using direct WebRTC. Each proxy kept its
  active actor through the action changes. Inventory was independent. Changing to a 60 FPS target
  and crossing the test boundary removed and restored the proxies. Leave, late same-slot rejoin
  after START, and in-place host reboot cleanup passed. The GPUs were the RTX 4050 Laptop GPU
  (616.92) and AMD Radeon Graphics (25.10.30.02). The 90 and 120 FPS target runs exercised action
  changes below the requested rates. Evidence is under `build/m2-anim-live-m1saptwh/`.
- A single Windows renderer showed all seven remote Links for 30 seconds after START. The seven
  senders were headless WebRTC fixtures. The RTX 4050 renderer kept the same seven proxies
  through 60, 90 and 120 FPS targets, measuring about 53, 71 and 68 FPS during those checks.
  All proxies retired when the fixtures disconnected. Evidence is under
  `build/m2-anim-capacity-uzyz5rty/`.

## M2 Plateau health check

- The Windows app and regenerated BOTW 1.9.0 x86-64-v3 gamelib `e640fd858dc0d033` built.
  The multiplayer core, eight-peer WebRTC and snapshot relay tests passed. The affected sources
  cross-compiled for Linux x86-64. The UI text check found zero missing and zero unused entries.
- Two 30 FPS Windows scratch games exchanged fresh health samples over direct WebRTC. A controlled
  write to the client's native life in its scratch process changed the host's roster from 12 to
  9 quarter-hearts and then back to 12. The host's native life stayed at 12. The run also repeated
  the live walk and jump checks. Evidence is under `build/m2-health-live-rq7bfy54/`.
- An F1 window capture shows 3 hearts for the local Link and 2.25 for the headless WebRTC sender.
  The capture is `build/m2-health-menu-g9g2yn3l/host/build/frames/window_01780.png`.

## Synthetic checks

The core historically printed `multiplayer core: ok` under GCC 16.2.1 and passed Clang's undefined-behavior
trap check on Windows. The demo admitted eight peers, accepted 560 snapshot deliveries, and rejected one stale
duplicate. GCC `-fanalyzer` reported no diagnostics on the protocol, session, and demo sources. These checks
use synthetic inputs.

```sh
make test-multiplayer
make multiplayer-demo
rtk git diff --check
```

## Verification required for runtime changes

Follow `AGENTS.md` and `CONTRIBUTING.md`. In particular:

- Use `make runtime-objs` for backend/UI compilation without game linking.
- For game sites or patches, run `tools/venv/bin/python tools/test_game_sites.py`, regenerate from the user's own
  files, rebuild, and check the loaded gamelib ID.
- Run `python3 tools/i18n_check.py` for player-facing text.
- Use `tools/linux/test_run.sh` with scratch settings/saves for Linux gameplay tests.
- Never commit a save, extracted file, generated game code, key, firmware, game asset/font, or disassembly output.

## Network test matrix for M1+

Record the exact build, game version, OS, transport version, and selected ICE candidate types for each run.

| Case | Minimum expected result |
|---|---|
| Same LAN | Direct encrypted connection; correct eight-peer roster |
| Different home networks | ICE/STUN direct path where available |
| Symmetric/restrictive NAT | Direct route or a connection failure that returns to the lobby controls |
| Packet loss/jitter/reordering | State remains bounded; reliable transactions converge |
| Duplicate/replayed packet | Rejected without duplicate world mutation |
| Incompatible version | Join rejected before guest/game mutation |
| Host exits | All clients leave cleanly; no host migration in v1 |
| Late join | Full authoritative snapshot before normal play |
| Runtime reboot | Threads, queues, sockets, and guest references reset |

## Native M1 acceptance

1. Build on Windows and Linux/Vulkan with the same supported 1.9.0 game target and compatible multiplayer
   source. Verify the matching local gamelib ID. Use separate scratch settings and saves for each instance.
2. Open F1 Settings and System > Options in the game. Confirm both expose Multiplayer, reflect the same
   enable state, and control the same lobby. Exercise Host, Join, and Leave through both surfaces.
3. On clean installs, host a lobby and copy its invite to a private chat. Paste the invite on the joining
   machine and send the reply to the host. Paste the reply on the host and confirm admission. Repeat in both
   menus without a signaling deployment. Reissue an invite and verify that its previous reply is rejected.
4. Exercise malformed clipboard text, cancellation, timeout, and rapid disable and re-enable. Confirm a new
   invite leaves admitted players connected and that disabling clears the local session.
5. Use two external networks and each OS in turn as host. Verify the selected direct candidates, an eight-peer
   roster, and localized incompatible-build rejection.
6. Count the NPC placeholders on the Plateau after the world is rendered. Each instance must show
   one per admitted remote peer. Test late join, rejection, slot reuse, disconnect, Leave, disable during a
   session, and reboot. Check that the affected placeholders disappear and scratch saves remain usable.
7. Boot and play in single-player with multiplayer disabled. Confirm the setting is shared across the two
   surfaces, persists across a normal restart, and can enable hosting again.

Record the build, game version, platform, GPU/driver, FPS, settings, and observed results.

## Gameplay test matrix for later milestones

- 1, 2, 4, and 8 peers.
- 30, 60, 90, and 120 FPS where timing behavior is relevant.
- Windows and Linux/Vulkan with GPU/driver/settings recorded.
- Same area, different area, loading screen, warp, interior, cutscene, death, reconnect, and clean restart.
- Cooperative enemy damage, simultaneous lethal hits, PvP, chest contention, inventory full, quest transaction replay,
  and late join after progress.

Record which results came from synthetic tests, builds, network tests, or observed gameplay.
