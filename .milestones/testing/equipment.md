# Melee weapons and equipment groundwork

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
