# Savage Wind Multiplayer milestone index

Savage Wind Multiplayer is a native, compile-in feature for the Windows and Linux/Vulkan builds of this fork.
The product target is an eight-player, host-authoritative cooperative campaign on BOTW Switch 1.9.0.
The project owner will manually bring in new Savage Wind releases. Keep the multiplayer core isolated and the
integration points small so each update can be built and verified on both platforms.

M0 is complete for synthetic scope. M1 is partial. The existing prototype has an encrypted lobby, HTTPS invite
signaling, and a Plateau placeholder path verified on Windows. The native enable/disable setting, both Settings
surfaces, native Linux and mixed-platform execution, and external-network acceptance are pending.

## Read in this order

1. [Native multiplayer handoff](NATIVE_MULTIPLAYER_HANDOFF.md) for the direction change, checkpoint, and first task.
2. [Project charter](PROJECT.md) and [milestone ledger](MILESTONES.md) for goals and acceptance criteria.
3. [Architecture](ARCHITECTURE.md) and [decisions](DECISIONS.md) for boundaries and rationale.
4. [Repository map](REPOSITORY.md) and [implementation notes](IMPLEMENTATION.md) for the current source.
5. [Testing](TESTING.md), [next work](NEXT.md), and [state](state.json) for evidence and remaining work.

At each checkpoint, update the handoff, milestone status, verification record, implementation notes, and state
snapshot where relevant. Read [AGENTS.md](../AGENTS.md) and [CONTRIBUTING.md](../CONTRIBUTING.md) before editing code.
