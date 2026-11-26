# Savage Wind Multiplayer milestones

Native eight-player co-op for BOTW Switch 1.9.0 on Windows and Linux/Vulkan.

Start with the [current handoff](HANDOFF.md). Open [next work](NEXT.md) when
choosing a task, then use the row for that task to find earlier investigations
or test evidence.

| Topic | Implementation checkpoints | Verification evidence |
| --- | --- | --- |
| Armor, dyes, and body visibility | [Appearance](handoff/appearance.md) | [Appearance tests](testing/appearance.md) |
| Movement, jitter, and clip phase | [Playback](handoff/playback.md) | [Playback tests](testing/playback.md) |
| Melee weapons and passive gear | [Equipment](handoff/equipment.md) | [Equipment tests](testing/equipment.md) |
| Shields, bows, and arrows | [Shield and bow](handoff/shield-bow.md) | [Shield and bow tests](testing/shield-bow.md) |
| Swimming, climbing, gliding, dodges, and hits | [Traversal and reactions](handoff/traversal.md) | [Traversal tests](testing/traversal.md) |
| Host clock, weather, and join/leave notices | [Environment](handoff/environment.md) | [Environment tests](testing/environment.md) |
| Presence, range, shrines, and travel | [M2 checkpoints](handoff/m2.md), [Plateau research](M2_POSITION_HANDOFF.md) | [M2 tests](testing/m2.md) |
| Lobby, invites, and platform builds | [Direct invites](DIRECT_MULTIPLAYER_HANDOFF.md), [Build compatibility](handoff/builds.md) | [Build recipes and test matrix](TESTING.md) |

- For scope and acceptance criteria, see the [charter](PROJECT.md) and [milestone ledger](MILESTONES.md).
- For design and source locations, see [architecture](ARCHITECTURE.md), [decisions](DECISIONS.md),
  the [repository map](REPOSITORY.md), and the [implementation reference](IMPLEMENTATION.md).
- For structured status, see [state.json](state.json). Earlier records include the
  [native migration](NATIVE_MULTIPLAYER_HANDOFF.md) and [November 25 state snapshot](history/state-2026-11-25.json).

At a checkpoint, refresh the handoff and save reusable findings and evidence
in the relevant topic files. Update next work, the ledger, and structured
status when their facts change.
Read [AGENTS.md](../AGENTS.md) and [CONTRIBUTING.md](../CONTRIBUTING.md) before making changes.
