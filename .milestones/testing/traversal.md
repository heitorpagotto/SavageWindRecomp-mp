# Traversal, dodges, and reactions

## M2.5 fast travel and narrow climbs

- The Windows app and matching game library built, and the multiplayer
  core, animation selector, direct invite, and WebRTC loopback tests passed.
  The site and layout fixtures passed. A local resource check resolved all
  14 new clips and verified the proxy's ELink user in ActorInfo.
- Two Windows Vulkan peers played at 30 FPS on BOTW 1.9.0 with copied
  saves, 960x540 at 50% scale, a 1400 MiB VRAM budget, and heap copy off. The
  host used an RTX 4050 Laptop GPU (610.88) and the client used AMD Radeon
  Graphics (25.10.30.02).
- At Great Plateau Tower the client used the Slate to travel from the
  platform back to its waypoint. The host played WarpDepart and spawned
  the native departure particles before retiring the proxy during loading.
  The proxy reappeared at the waypoint with WarpArrive, the arrival beam,
  and the dissolve, then returned to Idle in the same lobby. The run used
  protocol 11 and source ID `7a3f089202feeea00d81f3dba0a6ef38bfbd0f31`.
  See `build/m25-sync-warp-re9_tumn/`.
- The Gerudo Canyon ladder replay covered mounting, alternating upward
  steps, holding, descending, and dismounting. The tree north of the
  Shrine of Resurrection replayed PoleMount and PoleHold on the receiving
  controller. The final warp build also captured the hold from the
  receiving peer in `build/m25-sync-warp-re9_tumn/client/build/pole-hold.png`.
  Ladder traces and captures are in `build/m25-sync-ladder-zimy8996/`.

## M2.5 wall release

- The Windows app linked. The standalone multiplayer core, direct invite,
  and WebRTC loopback tests passed. A local resource check resolved all 97
  proxy clip references and confirmed that the wall release takes precedence
  over a bow aim sample.
- Two Windows Vulkan games on the Plateau used BOTW 1.9.0, protocol 7, gamelib
  `baf44255c0bc3428`, and source ID `dd4b88437469808ab8c6368989b86c93e50ef3f2`.
  Both used copied saves, 960x540 at 50% scale, a 1400 MiB VRAM budget, and
  Vulkan heap copy off. The host used an RTX 4050 Laptop GPU (610.88) and
  the client AMD Radeon Graphics (25.10.30.02).
- The host pressed B while climbing. The receiving controller played
  ClimbRelease, Fall, Land, and Idle in order on the same proxy at measured
  30, 60, and 90 FPS. At the 120 FPS cap, the host measured about 100 FPS and
  the client about 103 FPS. The client capture at 30 FPS shows the wall release pose.
- Logs and captures are under `build/m25-wall-release-check-pewsa8js/`.
  The local replay scripts are `build/m25-wall-release-build/build_fixture.py`
  and `build/m25-wall-release-build/verify_release.py`.

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
