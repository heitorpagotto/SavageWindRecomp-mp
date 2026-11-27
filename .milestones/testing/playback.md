# Movement and animation playback

## M2.5 running and traversal playback

- The Windows UCRT64 app linked. Multiplayer core, animation selector,
  direct invite, and WebRTC loopback passed. A local archive check verified
  all 26 selected 1.9.0 clips, compared the resulting bytes with the expected
  root-channel edits, and rejected a truncated archive. A scratch playback
  fixture checked phase interpolation across the loop and held samples
  at buffer underrun and teleport boundaries. The matching gamelib ID
  was `be47daf941612328`.
- Two Windows 1.9.0 Vulkan peers used copied saves, direct WebRTC,
  960x540 at 50% render scale, a 1400 MiB VRAM budget, and heap copy off.
  The host used an RTX 4050 Laptop GPU (610.88) and the client used AMD
  Radeon Graphics (25.10.30.02). The baseline at 30 FPS recorded a native
  run advancing about 1.22 animation frames per update and the proxy
  advancing about 0.97. See `build/m25-playback-before-run-s484awee/`.
- At 30 FPS near Oman Au Shrine, `build/m25-playback-final-all-ncyyv0dz/`
  recorded fast forward swimming, slow and fast turns in both directions,
  swim dash, and upward and rightward wall jumps on the receiving Link.
  The swim dash advanced about 4 m with zero backwards steps along the
  traced forward axis. The climb-jump trace reproduced a small dip already
  present in the sender's position near the top of the jump. Receiver
  captures `swim-dash.png` and `climb-jump.png` are in `client/build/`.
  In the same run, the median phase error between corrections was 0.92
  animation frames across 39 receiving updates with a valid gait phase.
- The matching source ID in the handoff played Run and Dash at 30, 60, 90,
  and 120 FPS caps on the same receiving proxy. At the 30 and 60 caps both
  peers reached the cap. Median logged host/client FPS at the 90 and 120
  caps were 81/78 and 76/70. Across 220 receiving updates with a valid
  gait phase between corrections, the median error was 1.00 animation
  frame and the 95th percentile was 1.92. See
  `build/m25-playback-caps-wnvfi2p8/`.
- Playback tests used local peers. Packet loss, mixed platforms, and visual
  smoothness of the discrete gait corrections over longer runs remain open.

## M2.5 remote movement and standing-jump playback

- The Windows Vulkan pair used 1.9.0 gamelib `13365c313591b27e` in Tah Muhl Shrine.
  Both used 960x540 at 50% render scale, a 1400 MiB VRAM budget and Vulkan heap copy off.
  The host used an RTX 4050 Laptop GPU (610.88) and the client AMD Radeon Graphics (25.10.30.02).
- The baseline in `build/m25-motion-before-60pw37kg/` recorded eight held position updates
  in sixteen consecutive 30 FPS mid-sprint steps, followed by movement steps as large as
  0.84 m. The network trace itself had no backwards steps. The separate side view in
  `build/m25-motion-model-3wxkvrdi/` shows the body passing the shield and axe during a
  sprint despite agreement between the actor and equipment matrices. The
  `build/m25-motion-idle-uk4r9qkc/` capture with the same movement applied over Idle and
  the `build/m25-motion-root-ttjxvfes/` clip-curve experiment isolated animation root translation.
- The final run in `build/m25-motion-final-3aoo7r_x/` used source ID
  `0724ac48f6da300ff33bfefb442ce2117dd955fc` and protocol 5. Sprint captures show the
  Wooden Shield and axe beside the running proxy at 30, 60, 90 and 120 measured FPS.
  The receiver recorded zero held or backwards steps in 155 mid-sprint updates across the
  four caps. At 30 FPS, the standard deviation of velocity between updates fell from
  10.30 to 0.06 m/s. This compares the same 4.8–9.5 m window along the run.
- Three standing jumps at each cap produced twelve native jump activations and twelve
  remote starts. The receiver called playback at normalized phases between 0.0157 and 0.1378.
  Standing shield guard still played, and Leave and same-slot rejoin retired and restored the
  proxy and shield. The test used copied saves, scratch settings and an isolated clipboard.
- The 30 FPS replay in `build/m25-motion-udp-rdowc22g/` used the existing test transport to
  drop every fifth application datagram and delay the others by up to 45 ms. It recorded zero
  held or backwards mid-sprint steps, three jump starts and shield guard. After Leave, the
  rejoin timed out before the client sent a new lobby handshake.
- The local resource check resolved all 96 clips and confirmed that only the four
  locomotion curves changed. A truncated archive was rejected. A synthetic playback check
  covered duplicate and older samples, a dropped sample, buffer underrun, timestamp wrap,
  gear angle wrap, repeated activation and same-area teleport.
- Windows app link, multiplayer core, eight-peer direct invite and WebRTC loopback passed.
  The ignored `build/m25_motion_probe.py` and `build/m25_run_motion.py` reproduce the
  instrumented route with `SWMP_MOTION_VARIANT=motion-final`.

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
