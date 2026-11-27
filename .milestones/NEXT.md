# Next multiplayer work

Continue M2.5 from the [current checkpoint](HANDOFF.md). The next animation checks are native light
and directional hits and the remaining water and ledge transitions, followed by
broader phase and rate sampling.

## Remaining animation work

The table follows the owner's work order. Windows validation can run during
Appearance. The owner will validate Linux manually and report issues.

| Stage | Work still open |
| --- | --- |
| 1. Weapons | Moving and airborne throw animations, exact blends, broader projectile coverage, and special weapon effects. [Item and melee checkpoints](handoff/equipment.md#m25-held-materials-and-item-releases) |
| 2. Shield and bow | Airborne aim needs layered playback. Broaden shield coverage to surf turns and slope variants. Extend attachment checks, including the brief offset seen in backflips. [Shield and bow checkpoints](handoff/shield-bow.md), [backflip evidence](testing/traversal.md#m25-grounded-dodges) |
| 3. Traversal and reactions | Check native light and directional hits, moving-target swimming, swim and glide blends, hand placement, and water/ledge transitions. [Traversal checkpoints](handoff/traversal.md) |
| 4. Playback | Extend phase and rate beyond standing jumps. Check late join and recovery under real packet loss. Add layers and transition blends where the proxy rig supports them. [Playback checkpoints](handoff/playback.md) |
| 5. Appearance | Broaden dye and masking coverage beyond the tested outfits. Check helmet variants, cloth, DLC, attachment alignment across clips, and travel. [Appearance checkpoints](handoff/appearance.md) |

## Other open work

- For environment sync, verify mixed game progression and native Linux and mixed-platform
  behavior.
  [Environment notes](handoff/environment.md), [environment tests](testing/environment.md)
- Verify player indicators on Linux. The Windows Slate fast-travel result is in the
  [player indicator test record](testing/m2.md#m25-player-names-and-map-markers).
  [M2.5 acceptance criteria](MILESTONES.md#m25--time-weather-and-full-link-synchronization)
- M7 tracks the deferred M2 manual tests.
  [Acceptance criteria](MILESTONES.md#m7--deferred-manual-validation), [M2 evidence](testing/m2.md#m2-acceptance-follow-up)
- M3 covers authoritative enemy positions, health, targeting, attacks, and reactions to every player,
  plus networked damage and PvP. [M3 acceptance criteria](MILESTONES.md#m3--combat-enemies-and-pvp)
- For distribution and the unresolved host direction, see the
  [dependency findings](DIRECT_MULTIPLAYER_HANDOFF.md#security-findings) and
  [build compatibility notes](handoff/builds.md).
