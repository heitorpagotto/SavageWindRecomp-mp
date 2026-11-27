# Movement and animation playback

## M2.5 running and traversal playback

- Stowed equipment follows the sender's sampled transforms while the proxy
  plays a standalone gait. At 30 FPS, one native run advanced 1.22 animation
  frames per update while the remote clip advanced 0.97. The sprint
  difference was larger. The sender now includes the phase when the active run
  or sprint track is 24 frames. Playback interpolates across the loop boundary
  and re-aligns the body and skinned armor when drift exceeds two animation
  frames. This uses the existing normalized start argument. It still makes
  discrete corrections and uses the existing protocol 11 snapshot fields.
- The swim dash had 5.5 m of authored forward motion and the upward wall jump
  had 2.7 m of vertical motion. Applying these on top of networked positions
  displaced the body from its independently placed shield. The proxy archive
  now removes planar travel from the swim dash, planar travel and yaw from the
  four swim turns, and translation from the wall step and jump clips. Swim
  height and limb motion are retained. Native Link uses the installed archive.
- See the [test record](../testing/playback.md#m25-running-and-traversal-playback)
  for measurements, capture paths, and the remaining coverage.

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
  [test record](../testing/playback.md#m25-remote-movement-and-standing-jump-playback) for coverage.

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
  See the [test record](../testing/playback.md#m25-animation-clip-progress-probe).
- See [remaining animation work](../NEXT.md#remaining-animation-work) for playback, traversal,
  weapon families, shield, bow and appearance coverage.
