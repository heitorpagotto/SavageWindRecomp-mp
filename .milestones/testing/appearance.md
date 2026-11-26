# Hair and armor

## M2.5 armor colors and base skin clipping

- The Windows launcher was `build/m25-outfit-build/Play-M25-Outfit.cmd` on both peers. It uses game version
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

## M2.5 hair and equipped armor

- The first Windows probe in `build/m25-appearance-probe-30dsokke/` displayed the default hair
  and a Champion's Tunic as separate passive actors on Link. Both used the same Jump and Idle
  clips as the proxy. The read-only trace saw the native upper body change from default to
  Hylian Tunic after equipping it in the inventory.
- The production replay in `build/m25-appearance-live-vcsd56ri/` used protocol 6, source ID
  `51eff56c586d03a3f1d38dd0a5b0f500d34f6458`, and 1.9.0 gamelib `13365c313591b27e`.
  Two Windows games in Tah Muhl Shrine used Vulkan at 1280x720, 75% render scale and a 1400 MiB
  VRAM budget with heap copy off. The host used an RTX 4050 Laptop GPU (610.88) and the client
  AMD Radeon Graphics (25.10.30.02).
- Captures show default hair and each part of the Hylian set. Native inventory equips produced
  only the corresponding remote appearance change. The receiving inventory remained independent.
  The proxy and its three pieces each entered Jump once at 30, 60, 90 and 120 FPS targets.
  Leave and same-slot rejoin restored the three equipped pieces. Removing the hood restored
  hair while the tunic and trousers stayed active.
- The capture comparison shows differences from the native outfit's material color and body mesh
  masking on protocol 6. The trousers render with the boots, and the proxy's bare-leg mesh can show through.
  The reference captures are `host/build/local-set.png` and `client/build/remote-set.png` under
  `build/m25-appearance-live-vcsd56ri/`. The protocol 7 follow-up above covers color and masking.
  DLC and Linux remain open.
- A 30 FPS Windows replay on the same build equipped the Ancient Cuirass, Climber's Bandanna
  and Stealth Tights from the native inventory. Each appeared on the same remote Link, and
  all three pieces entered Jump with the body at the same sampled phase.
  See `build/m25-appearance-mixed-5u3mn8rj/client/build/remote-mixed-outfit.png` and the logs
  from `build/m25_run_appearance_mixed.py`.
- The installed-data check built 123 model packs from 420 native armor records. The runtime mounted
  116 whose model resources were available. Upgrades can share the same model. Every original
  ActorInfo entry survived the merge. The ignored probes are `build/m25_appearance_assets_test.py`,
  `build/m25_appearance_live.py` and `build/m25_run_appearance_live.py`.
