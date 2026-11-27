# Current multiplayer handoff

## M2.5 running and traversal playback

- **Implemented** Remote run and sprint use the sender's gait phase to keep
  Link and his armor closer to the sampled motion of stowed gear. The proxy's
  swim dash, swim turns, wall steps and wall jumps now take movement from
  snapshots on the axes previously duplicated by the animation.
  [Implementation notes](handoff/playback.md#m25-running-and-traversal-playback)
- **Tested** Two Windows peers at 30 FPS on BOTW 1.9.0 played fast forward
  swimming, both turns, the swim dash, and upward and rightward wall jumps.
  Run and sprint also played at 30, 60, 90, and 120 FPS caps. The peers
  ran below the two highest caps. The Windows app and four multiplayer
  test targets passed.
  [Settings and evidence](testing/playback.md#m25-running-and-traversal-playback)

## M2.5 fast travel and narrow climbs

- **Implemented** Slate fast-travel departure and arrival, including the
  native blue particles and the dissolve on Link and attached gear. Remote
  Link also plays the one-handed pole hold and ladder mount, climb, wait,
  descent, and dismount animations.
  [Implementation notes](handoff/traversal.md#m25-fast-travel-and-narrow-climbs)
- **Tested** on two Windows peers at 30 FPS in BOTW 1.9.0. At Great Plateau
  Tower, the observer saw the traveler depart, reappear at the waypoint,
  and return to idle in the same lobby. Separate replays verified the
  one-handed hold at a conifer tip and alternating ladder steps in Gerudo
  Canyon. The Windows build and multiplayer tests passed.
  [Settings and evidence](testing/traversal.md#m25-fast-travel-and-narrow-climbs)

## M2.5 held materials and item releases

Checkpoint `e45852a` on `multiplayer/windows-test`, 2026-11-27. M2.5 is in progress.

- **Implemented** carry and release animations, up to five held material
  models, and visual playback of supported drops and throws.
  [Implementation notes](handoff/equipment.md#m25-held-materials-and-item-releases)
- **Tested** carry, far tosses, an axe drop, and spear flight on two
  Windows peers at 30 and 60 FPS. The 30 FPS runs also covered near
  drops, put-away, Boomerang drop and flight, and peer departure.
  [Settings and evidence](testing/equipment.md#m25-held-materials-and-item-releases)
- **Next** Native light and directional hit reactions, water transitions, and ledges.
  [Next work](NEXT.md)

## Matching Windows build

Both peers need matching protocol 11 builds. The local launcher is
`build/m25-playback-fix/Play-M25-Playback.cmd` for BOTW 1.9.0. It uses gamelib
`be47daf941612328` from `build/m25-warp-sync` and reports source ID
`e9d4f4d26a0d1aec984cc63a36b3e8bdd083f2c0`.
[Build compatibility](../docs/build-windows-and-appimage.md#multiplayer-build-compatibility)

Linux gameplay and mixed-platform validation are tracked in [M7](MILESTONES.md#m7--deferred-manual-validation).
Earlier checkpoints are in the [topic index](README.md).
