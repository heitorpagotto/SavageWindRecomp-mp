# Time, weather, and lobby notices

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
