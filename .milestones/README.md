# Savage Wind Multiplayer milestone index

Savage Wind Multiplayer is a native, compile-in feature for the Windows and Linux/Vulkan builds of this fork.
The product target is an eight-player, host-authoritative cooperative campaign on BOTW Switch 1.9.0.
The project owner will manually bring in new Savage Wind releases. Keep the multiplayer core isolated and the
integration points small so each update can be built and verified on both platforms.

M0 is complete for synthetic scope. The owner accepted M1 after joining a Linux host from Windows and Linux.
M2 is complete. It implements native remote Link animation and area-aware presence on the main field and
in shrines. Windows travel and a four-game soak passed. [M7](MILESTONES.md#m7--deferred-manual-validation)
tracks the deferred eight-game soak, Linux AppImage check, and mixed-platform visibility and travel.
[M2.5](MILESTONES.md#m25--time-weather-and-full-link-synchronization) follows M2 and covers host time and
weather and full player equipment and animation sync.
See the [session handoff](HANDOFF.md), [M2 handoff](M2_POSITION_HANDOFF.md), and [test record](TESTING.md)
for the current results.

## Read in this order

1. [Session handoff](HANDOFF.md) for the current checkpoint, matching Windows launch path, and open reports.
   See the [direct multiplayer handoff](DIRECT_MULTIPLAYER_HANDOFF.md) for the invite flow and security findings
   and the [native multiplayer handoff](NATIVE_MULTIPLAYER_HANDOFF.md) for the migration history.
2. [Project charter](PROJECT.md) and [milestone ledger](MILESTONES.md) for goals and acceptance criteria.
3. [Architecture](ARCHITECTURE.md) and [decisions](DECISIONS.md) for boundaries and rationale.
4. [Repository map](REPOSITORY.md) and [implementation notes](IMPLEMENTATION.md) for the current source.
5. [Testing](TESTING.md), [next work](NEXT.md), and [state](state.json) for evidence and remaining work.

At each checkpoint, update the handoff, milestone status, verification record, implementation notes, and state
snapshot where relevant. Read [AGENTS.md](../AGENTS.md) and [CONTRIBUTING.md](../CONTRIBUTING.md) before editing code.
