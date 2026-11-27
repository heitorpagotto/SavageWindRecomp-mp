# Traversal, dodges, and reactions

## M2.5 wall release

- Earlier wall-climbing traces showed native `WallOff` while the remote Link
  switched to Idle midair. The action map now plays the installed left-foot
  release clip before the usual Fall and Land states.
- The action is appended after the existing action IDs and uses the existing
  snapshot format. Both peers need the matching source fingerprint.
- Release uses a representative clip. Foot selection, tired releases, and
  attachment alignment during the transition remain open.
  [Windows replay](../testing/traversal.md#m25-wall-release)

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
  [test record](../testing/traversal.md#m25-grounded-dodges).

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
  `SWMP_VERIFY_REACTION=1`. See the [test record](../testing/traversal.md#m25-hit-reactions-and-knockdown-recovery).

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
  See the [test record](../testing/traversal.md#m25-fall-damage-recovery).

## M2.5 lock-on swimming

- The lock-on swimming checkpoint used source ID `c3c71f0f65dcf665b4f3ba417fd05ef3dc8ddf7f`.
- Native SwimAtnMove exposes targeting movement. The sender projects planar displacement
  into Link's facing frame and selects a forward, backward, left or right stroke. Treading
  and free swimming keep their existing paths.
- The ignored `build/m25_swim_lock_live.py` builds the instrumented app. Set
  `SWMP_PROBE_APP=build/m25-build/swim-lock-live/savagewind.exe` and
  `SWMP_TRAVERSAL_ROUTE=build/m25_swim_lock_verify.txt` for `build/m25_run_traversal.py`.
  See the [test record](../testing/traversal.md#m25-lock-on-swimming).
- Target-following in the water, exact blend playback and water effects still need coverage.

## M2.5 swim stroke selection

- The free swim checkpoint used source ID `d3d88b9e192aa7046ce55f95371214d6d65bf712`.
- Native SwimMove covers both stroke speeds and both turn directions. The sender samples
  speed and wrapped heading change to select one of the six installed swimming clips.
  Treading and dash keep their dedicated clips. All swim actions use the same model lift.
- The ignored `build/m25_swim_blends_probe.py` builds the trace executable. Set
  `SWMP_PROBE_APP=build/m25-build/swim-blends-probe/savagewind.exe` and use
  `SWMP_TRAVERSAL_ROUTE=build/m25_swim_blends_route.txt` with `build/m25_run_traversal.py`.
  Use `build/m25_swim_caps_route.txt` for the higher caps. See the [test record](../testing/traversal.md#m25-swim-stroke-selection).

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
  See the [test record](../testing/traversal.md#m25-glide-banking).

## M2.5 directional climbing jumps

- The climbing jump checkpoint used source ID `f6877769f778e5ad6dd4d41b96751c75d6caddc8`.
- The sender selects one installed clip for each native ClimbJump using early movement in Link's
  wall plane. The current clips cover right, up-right, up, up-left and left. Direction stays fixed
  until the native state changes. The proxy still starts the clip with its own animation timing.
- The ignored `build/m25_jump_probe.py` builds the instrumented app. Set
  `SWMP_PROBE_APP=build/m25-build/jump-probe/savagewind.exe` for `build/m25_run_traversal.py`.
  Set `SWMP_TRAVERSAL_ROUTE` to `build/m25_jump_30_route.txt` for all five clips at 30 FPS or
  `build/m25_jump_route.txt` for the higher caps.
  See the [test record](../testing/traversal.md#m25-directional-climbing-jumps).

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
  See the [test record](../testing/traversal.md#m25-gliding).

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
  See the [test record](../testing/traversal.md#m25-directional-wall-climbing).

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
  See the [test record](../testing/traversal.md#m25-swimming-and-climbing).

## M2.5 fast travel and narrow climbs

- Ladder states select the installed mount, wait, climb, descent, and exit
  clips. The upward state advances between alternating foot clips when the
  native animation frame wraps. A narrow tree or pole top uses
  `ClimbTreeEdSharp` for the mount and `ClimbTreeTopSharp` for the hold.
- Fast travel out uses the native `Warp` clip. Fast travel in uses the
  `Demo005_1` event because the skeletal state is a standing pose. The
  receiver applies the replicated native dissolve to Link and attached gear
  and plays the corresponding ELink effect on the proxy. Origin observers
  finish buffered departure samples while the traveling peer loads.
- The proxy's ActorInfo entry uses Link's installed ELink user. Setting
  the ActorLink entry alone left the proxy's XLink component unallocated.
- [Settings and evidence](../testing/traversal.md#m25-fast-travel-and-narrow-climbs)
