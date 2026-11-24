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
frame counters and a normalized clip start argument for the Playback stage. The
[weapon family checkpoint](HANDOFF.md#m25-weapon-families) covers standing sword, spear, and
large-sword actions. The [shield and bow checkpoint](HANDOFF.md#m25-shield-and-bow-follow-up)
records the grounded clips, native guard-hit check, and passive rig's upper-slot limitation.
Continue under the [owner's work order](HANDOFF.md#m25-animation-work-order). The
[appearance probe](M2_POSITION_HANDOFF.md#resume) records the separate skinned-model constraint.
The [dependency findings](DIRECT_MULTIPLAYER_HANDOFF.md#security-findings) apply to distribution.

## Remaining animation work

| Stage | Work still open |
| --- | --- |
| 1. Weapons | Standing draw, idle, stow, chains, charge and recovery verified for representative sword, spear and large-sword weapons. Moving and airborne variants and exact blends remain open. |
| 2. Shield and bow | Soldier's variants, sustained guard, standing raise/lower blends, parry, guard hit/break, one-handed sword with shield, and grounded bow draw, ready and stow are verified. Normal Arrow follows the native nocked pose. Bowstring animation, additional arrow types and rapid reload cadence remain open. Airborne aim needs layers. See the [shield and bow handoff](HANDOFF.md#m25-shield-and-bow-follow-up) and [nocked arrow](HANDOFF.md#m25-nocked-arrow-probe). |
| 3. Traversal and reactions | Verify moving-target swimming, native swim and glide blends, hand placement, and transitions involving water and ledges. Audit dodges, hit reactions, knockdown and recovery against the native controller. |
| 4. Playback | Start with standing-jump phase and repeated activations. Extend phase and rate sampling across verified clips and check late join, rejoin and recovery after dropped snapshots. Add layered playback and transition blends where the proxy rig supports them. |
| 5. Appearance | Verify a skinned-model path for hair and armor and check equipment attachments across supported clips. Windows animation validation can happen here, including area travel, late join and reconnect. The owner will validate Linux manually later and report any problems. |

Networked damage and enemy ownership remain under [M3](MILESTONES.md#m3--combat-enemies-and-pvp).

The owner can run [M7 manual validation](MILESTONES.md#m7--deferred-manual-validation) separately. See the
[handoff](HANDOFF.md#m2-acceptance-follow-up) for the matching build and the
[test record](TESTING.md#m2-acceptance-follow-up) for the current evidence.
