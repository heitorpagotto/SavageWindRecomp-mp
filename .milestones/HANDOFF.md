# Current multiplayer handoff

## M2.5 held materials and item releases

Checkpoint 2026-11-27 on `multiplayer/windows-test`, based on `1a334d2`.
M2 is accepted. M2.5 remains in progress.

- **Implemented** carrying and release animations for inventory materials,
  up to five held materials, and visual playback for supported dropped
  and thrown items. Both peers need the matching protocol 10 build.
  [Implementation notes](handoff/equipment.md#m25-held-materials-and-item-releases)
- **Verified** mixed materials while standing and walking, near and far
  releases, put-away, an inventory axe drop, weapon flight, and cleanup
  on peer departure in two Windows games.
  [Settings and evidence](testing/equipment.md#m25-held-materials-and-item-releases)
- **Resume** with native light and directional reactions or the remaining water and ledge transitions.
  [Next work](NEXT.md)

## Matching Windows build

Use the local `build/m25-items/Play-M25-Items.cmd` on both peers when available.
It uses BOTW 1.9.0, x86-64-v3 gamelib `e5c3dd1133f2953b`, multiplayer source ID
`75cde0dc3d74275165f1328d4228c6cbc2768b2e`, and release label `0.6.0-alpha.6-0.1.0-mp`.
See the [build guide](../docs/build-windows-and-appimage.md#multiplayer-build-compatibility)
for source matching and use copied saves and separate settings for tests.

The owner will validate Linux manually. [M7](MILESTONES.md#m7--deferred-manual-validation)
tracks the eight-game soak, Linux AppImage, and mixed-platform visibility and travel.
See the [topic index](README.md) for earlier checkpoints.
