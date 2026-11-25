# Resume guide and next work

M2 is complete. The first [M2.5](MILESTONES.md#m25--time-weather-and-full-link-synchronization) slice
adds host time and weather sync and admission notifications. See the [handoff](HANDOFF.md#m25-first-slice)
for verification and remaining environment cases. The [shield and animation checkpoint](HANDOFF.md#m25-shield-axe-idle-and-crouch)
covers independent axe and shield slots and the new axe idle, stow, guard and crouch clips. The
[bow checkpoint](HANDOFF.md#m25-travelers-bow) covers the third rigid gear slot and its standing actions.
The [traversal checkpoint](HANDOFF.md#m25-swimming-and-climbing) covers swimming and upward wall climbing.
The [wall climbing checkpoint](HANDOFF.md#m25-directional-wall-climbing) covers directional climbing,
wall approach and mantling. The [gliding checkpoint](HANDOFF.md#m25-gliding) covers deployment,
neutral flight, falling and landing. The [climbing jump checkpoint](HANDOFF.md#m25-directional-climbing-jumps)
covers lateral, upward and upward-diagonal jumps. The [banking checkpoint](HANDOFF.md#m25-glide-banking)
covers coordinated tilt of Link and the canopy while steering. The
[swim checkpoint](HANDOFF.md#m25-swim-stroke-selection) covers slow and fast strokes during free
swimming. The [lock-on checkpoint](HANDOFF.md#m25-lock-on-swimming) covers directional strokes
when targeting. The [clip progress probe](HANDOFF.md#m25-animation-clip-progress-probe) found readable
frame counters and a normalized clip start argument. The
[playback checkpoint](HANDOFF.md#m25-remote-movement-and-standing-jump-playback)
uses those for standing jumps and addresses movement jitter and sprinting gear separation. The
[weapon family checkpoint](HANDOFF.md#m25-weapon-families) covers standing sword, spear, and
large-sword actions. The [shield and bow checkpoint](HANDOFF.md#m25-shield-and-bow-follow-up)
records the grounded clips, native guard-hit check, and passive rig's upper-slot limitation.
The [bowstring checkpoint](HANDOFF.md#m25-bowstring-animation) records native bow playback and
the protocol 3 snapshot field. The [arrow checkpoint](HANDOFF.md#m25-nocked-arrow-variants-and-rapid-reload)
adds all six inventory arrow types and verifies rapid reload. The [grounded dodge checkpoint](HANDOFF.md#m25-grounded-dodges)
verifies neutral, forward and sideways steps and the backflip. The
[reactions checkpoint](HANDOFF.md#m25-hit-reactions-and-knockdown-recovery) covers native medium hits,
knockdowns, tumbling and standing back up. The [fall-damage checkpoint](HANDOFF.md#m25-fall-damage-recovery)
covers native hard landings and their recovery. The
[appearance checkpoint](HANDOFF.md#m25-hair-and-equipped-armor) covers default hair and the
equipped head, upper and lower models.
Continue under the [owner's work order](HANDOFF.md#m25-animation-work-order).
The [dependency findings](DIRECT_MULTIPLAYER_HANDOFF.md#security-findings) apply to distribution.

## Remaining animation work

| Stage | Work still open |
| --- | --- |
| 1. Weapons | Standing draw, idle, stow, chains, charge and recovery verified for representative sword, spear and large-sword weapons. Moving and airborne variants and exact blends remain open. |
| 2. Shield and bow | Soldier's variants, standing guard and blends, parry, hit and break, and one-handed sword with shield are verified. Grounded bow playback includes bowstring draw and release, all six nocked inventory arrow types, and rapid reload. Airborne aim needs layers. See the [shield and bow handoff](HANDOFF.md#m25-shield-and-bow-follow-up) and [arrow checkpoint](HANDOFF.md#m25-nocked-arrow-variants-and-rapid-reload). |
| 3. Traversal and reactions | Grounded dodges, medium hits, knockdown recovery and fall-damage recovery have Windows coverage. Verify light and directional hits, moving-target swimming, native swim and glide blends, hand placement, and transitions involving water and ledges. |
| 4. Playback | Standing jumps carry phase and activation IDs. Extend phase and rate sampling across verified clips and check late join and recovery under real packet loss. Add layered playback and transition blends where the proxy rig supports them. |
| 5. Appearance | Default hair and skinned armor models follow the proxy. Match native material tint and body mesh masking, extend attachment checks across clips, and verify DLC assets and area travel. The owner will validate Linux manually later. |

The next appearance work is to fix armor colors and base skin clipping. [Handoff](HANDOFF.md#m25-armor-colors-and-base-skin-clipping)

Networked damage and enemy ownership remain under [M3](MILESTONES.md#m3--combat-enemies-and-pvp).

The owner can run [M7 manual validation](MILESTONES.md#m7--deferred-manual-validation) separately. See the
[handoff](HANDOFF.md#m2-acceptance-follow-up) for the matching build and the
[test record](TESTING.md#m2-acceptance-follow-up) for the current evidence.
