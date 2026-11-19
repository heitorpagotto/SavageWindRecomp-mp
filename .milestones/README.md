# Savage Wind Multiplayer handoff

Last updated: 2026-11-18 (America/Sao_Paulo)

This directory is the durable project memory for the Savage Wind peer-to-peer multiplayer mod. It is written so a
new developer or agent can resume work without conversation history. Treat statements here as a snapshot: verify the
branch, source, and tests before changing code, then update these files when a decision or milestone changes.

## Current truth

- Branch: `multiplayer/bootstrap`.
- Base: upstream Savage Wind `test` commit `f488af7`.
- Target: BOTW Switch 1.9.0 through Savage Wind; Windows and Linux are maintained.
- Product target: host plus seven clients, internet P2P transport, host-authoritative cooperative campaign.
- Implemented: protocol framing, player snapshot codec, eight-peer session state, transport interface, synthetic
  tests, an eight-Link in-process demo, and a deterministic SWMM development package.
- Not implemented: actual sockets/WebRTC, rendezvous service, invite-code exchange, native module loading, in-game
  Host/Join UI, remote Link actors, combat synchronization, quest synchronization, or chest rewards.
- The current SWMM archive is a discovery marker only. It does not run multiplayer inside Savage Wind.
- No Nintendo files, extracted data, generated game code, saves, assets, or decompilation output were added.
- Workspace changes are not committed. Run `git status --short --branch` before doing anything else.

## Read in this order

1. [PROJECT.md](PROJECT.md) — product goal, scope, legal and gameplay constraints.
2. [REPOSITORY.md](REPOSITORY.md) — branch, upstream rules, relevant Savage Wind integration points.
3. [DECISIONS.md](DECISIONS.md) — decisions already made and their rationale.
4. [ARCHITECTURE.md](ARCHITECTURE.md) — network, authority, persistence, threading, and security design.
5. [IMPLEMENTATION.md](IMPLEMENTATION.md) — exact code and wire formats already present.
6. [SWMM.md](SWMM.md) — Mod Manager research, package layout, and native-plugin limitation.
7. [MILESTONES.md](MILESTONES.md) — all milestones and acceptance criteria.
8. [TESTING.md](TESTING.md) — commands actually run, results, gaps, and future test matrix.
9. [NEXT.md](NEXT.md) — recommended next work in executable order.
10. [state.json](state.json) — compact machine-readable snapshot.

## Quick verification

From the repository root:

```sh
make CC=gcc test-multiplayer
make CC=gcc multiplayer-demo
make multiplayer-swmm-package
make test-multiplayer-swmm
git diff --check
```

Expected demo summary:

```text
Lobby ready: 8/8 Links; host authority at peer 0
PASS: 8 Links, 560 snapshot deliveries, 1 stale duplicate rejected
```

The local environment used for this snapshot had GCC 16.2.1 but no `clang`. The normal project compiler remains
Clang; `CC=gcc` was only a local test override.

## Maintenance rule

When work changes the project, update at least:

- `state.json` for status, version, test, or branch changes;
- `MILESTONES.md` for completed acceptance criteria;
- `IMPLEMENTATION.md` for new APIs, packets, files, or hooks;
- `TESTING.md` with commands actually run and what was not verified; and
- `DECISIONS.md` when choosing a transport, plugin ABI, authority rule, persistence rule, or incompatible protocol
  behavior.

