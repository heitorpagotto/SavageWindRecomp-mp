# Current multiplayer handoff

## M2.5 grounded weapon throws

Checkpoint 2026-11-26 on `multiplayer/windows-test`, based on `1a334d2`.
M2 is accepted. M2.5 remains in progress.

- **Implemented** native wind-up, aim and release for all three melee
  families, plus one- and two-handed boomerang catches. Held models
  follow the normal equipment revisions. Both peers need the matching
  protocol 9 build.
  [Implementation notes](handoff/equipment.md#m25-grounded-weapon-throws)
- **Verified** all three melee families in two-peer Windows gameplay at
  30 and 60 FPS, including disappearance and reattachment for both
  one-handed and Giant Boomerangs.
  [Settings and evidence](testing/equipment.md#m25-grounded-weapon-throws)
- **Resume** with native light and directional reactions or the remaining water and ledge transitions.
  [Next work](NEXT.md)

## Matching Windows build

Use the local `build/m25-throw-build/Play-M25-Throw.cmd` on both peers when available.
It uses BOTW 1.9.0, x86-64-v3 gamelib `baf44255c0bc3428`, multiplayer source ID
`9b8604dd9e1a33d4ef8ece71c57ea8f8fa1ead5f`, and release label `0.6.0-alpha.6-0.1.0-mp`.
See the [build guide](../docs/build-windows-and-appimage.md#multiplayer-build-compatibility)
for source matching and use copied saves and separate settings for tests.

The owner will validate Linux manually. [M7](MILESTONES.md#m7--deferred-manual-validation)
tracks the eight-game soak, Linux AppImage, and mixed-platform visibility and travel.
See the [topic index](README.md) for earlier checkpoints.
