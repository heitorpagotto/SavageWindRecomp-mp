# Melee weapons and equipment groundwork

## M2.5 grounded weapon throws

- The Windows UCRT64 app linked. The multiplayer core, animation selector,
  direct invite, and WebRTC loopback tests passed. Both changed runtime
  sources also cross-compiled for Linux x86-64. The installed-resource
  check resolved all 122 proxy actions, including the 11 new throw and
  catch actions, to native skeletal clips.
- Two Windows Vulkan games near Oman Au Shrine used BOTW 1.9.0, gamelib
  `baf44255c0bc3428`, and protocol 9 with source ID
  `9b8604dd9e1a33d4ef8ece71c57ea8f8fa1ead5f`. Both used copied saves,
  960×540, 50% render scale, a 1400 MiB VRAM budget, and Vulkan heap copy
  and GPU overlap off. The host used an RTX 4050 Laptop GPU (610.88),
  and the client used AMD Radeon Graphics (25.10.30.02).
- At 30 FPS, `build/m25-throw-live-hyfz_cb2/` verified wind-up, aim and
  release for `Weapon_Spear_002` and the one-handed Boomerang
  (`Weapon_Sword_051`). The receiver removed each held visual after
  release and restored the Boomerang on its catch animation. Each throw
  kept the same remote Link actor.
- At the 60 FPS cap, `build/m25-throw-live-3ant3f54/` repeated the
  checks for the Giant Boomerang (`Weapon_Lsword_051`) and Traveler's
  Sword (`Weapon_Sword_001`). Both games ran near 60 FPS and the
  Giant Boomerang reappeared after the two-handed catch state started.
- With 90 and 120 FPS caps, `build/m25-throw-live-eycsmh75/` verified
  `Weapon_Lsword_002` and the Sea-Breeze Boomerang (`Weapon_Sword_059`),
  including the catch and held-model restoration. During the test the
  host measured about 55–85 FPS and the client about 62–87 FPS.
- To repeat the test, run the ignored `build/m25-throw-build/build_fixture.py`
  and `verify_live.py range`. Native Linux gameplay and remote projectile
  flight remain open.

## M2.5 installed weapon catalog

- Windows built successfully, and the multiplayer core, direct invite, and
  WebRTC loopback targets passed. The equipment codec checks big-endian IDs,
  wrong-slot families, invalid IDs, and rejection of the shorter protocol 7 record.
  The changed equipment sources also cross-compiled for Linux x86-64.
- The BOTW 1.9.0 installed-resource check built 54 one-handed weapons,
  41 two-handed weapons, 39 spears, 33 shields, and 27 bows. All 194 aliases
  reused the native model lists, and all 27 bows kept their own AS files.
  The check preserved every stock ActorInfo record and verified the melee
  draw, ready, first attack, and stow selectors for all 134 melee identities.
  Paraglider and all six nocked-arrow aliases also passed resource checks.
  The ignored check is `build/m25-gear-catalog-build/verify_resources.py`.
- Two Windows Vulkan games on the Plateau near Oman Au Shrine used BOTW
  1.9.0, gamelib `baf44255c0bc3428`, and protocol 8 with source ID
  `b1dd205f140bb3c7cdb27252f9e33631f1cf8729`. Both used copied saves,
  960x540, 50% render scale, a 1400 MiB VRAM budget, and Vulkan heap copy
  off. The host used an RTX 4050 Laptop GPU (610.88) and the client AMD
  Radeon Graphics (25.10.30.02).
- `build/m25-gear-catalog-live-qnf3ziby/` shows the new sword draw, ready,
  swing, and stow at 30 FPS, and the spear draw and thrust at a 60 FPS
  cap. The same replay verified Hylian Shield guard, bow aiming with an
  ordinary nocked arrow, bowstring draw, and release for the native
  `Weapon_Bow_023` animation list.
- `build/m25-gear-catalog-live-xumidlk9/` captures `Weapon_Lsword_002`
  and the shared-model `Weapon_Spear_049` on the receiving games. Both
  played draw, ready, and first attack and returned to stowed. At the
  90 and 120 FPS caps, the two games measured about 82/81 and 78/79 FPS,
  respectively. Some native stows went straight to Wait, with the
  receiving controller returning to Idle.
- The 30 FPS replay in `build/m25-gear-catalog-live-o0afdtsb/` verified
  guarding and the bow body and bowstring draw, aim, and release for both
  custom-list bows. The verification required the ordinary nocked arrow
  on `Weapon_Bow_023`. The trace for `Weapon_Bow_072` shows a brief
  SpearDraw before BowDraw while a spear was equipped.
- Leave and same-slot rejoin retired and restored the three equipped models
  on both peers in the melee and bow replays. Rebuild the local
  `build/m25-gear-catalog-build/build_fixture.py` and run `verify_live.py range`
  to repeat the replays. Native Linux gameplay and weapon-specific effects
  remain open.

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

## M2.5 equipment probe

Two Windows scratch games in Tah Muhl Shrine used BOTW 1.9.0, gamelib `13365c313591b27e`, direct
WebRTC loopback, 30 FPS target, 960x540, 50% render scale, 1400 MiB VRAM and Vulkan heap copy off.
The GPUs and drivers match the environment test. Both native stock axe actors activated and both
renderers applied equipment transforms from the other game. Only the host drew their axe and only
the client's remote visual moved from the stowed to drawn placement. The client still showed its
own axe stowed. The stock actor exposed Take, and the fixture does not cover equip changes or actor
retirement. See the [equipment handoff](../handoff/equipment.md#m25-equipment-probe) for scripts and evidence.
