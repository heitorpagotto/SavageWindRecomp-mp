# Current multiplayer handoff

## M2.5 armor and appearance

The 2026-11-26 checkpoint is commit `097b11a` on `multiplayer/windows-test`.
M2 is accepted. M2.5 remains in progress.

- **Implemented** per-slot dye indices in protocol 7 and native body masking from equipped armor.
  Dye changes reload the affected slot. [Implementation notes](handoff/appearance.md#m25-armor-colors-and-base-skin-clipping)
- **Verified** in two Windows Vulkan games at 30 FPS. A yellow and undyed copy of the same tunic
  appeared together. Movement, jump, rejoin, dye changes, and restoration of leg meshes after
  unequipping trousers were checked. [Settings and evidence](testing/appearance.md#m25-armor-colors-and-base-skin-clipping)
- **Resume** with native light and directional reactions or movement transitions at water and ledges.
  Extend clip phase and rate beyond standing jumps as playback work resumes. [Next work](NEXT.md)

## Matching Windows build

Use the local `build/m25-outfit-build/Play-M25-Outfit.cmd` on both peers when available.
It uses BOTW 1.9.0, x86-64-v3 gamelib `baf44255c0bc3428`, multiplayer source ID
`4afc4c9f6fea459a9a837809966d1ef3bd521b6f`, and release label `0.6.0-alpha.6-0.1.0-mp`.
Older appearance builds use protocol 6. See the [build guide](../docs/build-windows-and-appimage.md#multiplayer-build-compatibility)
for source matching and use copied saves and separate settings for tests.

The owner will validate Linux manually. [M7](MILESTONES.md#m7--deferred-manual-validation)
tracks the eight-game soak, Linux AppImage, and mixed-platform visibility and travel.
See the [topic index](README.md) for earlier checkpoints.
