# Savage Wind Multiplayer handoff

Last updated: 2026-11-19 (America/Sao_Paulo)

This directory is the durable project memory for the Savage Wind peer-to-peer multiplayer mod. It is written so a
new developer or agent can resume work without conversation history. Treat statements here as a snapshot: verify the
branch, source, and tests before changing code, then update these files when a decision or milestone changes.

## Current truth

- M1 working branch: `multiplayer/windows-test`. Bootstrap branch: `multiplayer/bootstrap`.
- Base: upstream Savage Wind `test` commit `f488af7`.
- Target: BOTW Switch 1.9.0 through Savage Wind; Windows and Linux are maintained.
- Product target: host plus seven clients, internet P2P transport, host-authoritative cooperative campaign.
- Implemented: protocol framing, player snapshot codec, eight-peer sessions, a transport-driven lobby with
  compatibility checks, a deterministic mock transport, an eight-Link demo, and SWMM development packaging.
- Production work remaining: encrypted P2P connections, rendezvous and invite exchange, the native bridge, Host/Join
  UI, remote Link actors, and gameplay synchronization.
- The current SWMM archive is a discovery marker only. It does not run multiplayer inside Savage Wind.
- No Nintendo files, extracted data, generated game code, saves, assets, or decompilation output were added.
- Bootstrap is committed at `2b9a27c`. The M1 lobby changes are in the working tree. Check `git status --short --branch` before editing.

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

Clang is the normal project compiler. Use `CC=gcc` for GCC or see [TESTING.md](TESTING.md) for the Windows
standalone verification.

## Maintenance rule

When work changes the project, update at least:

- `state.json` for status, version, test, or branch changes;
- `MILESTONES.md` for completed acceptance criteria;
- `IMPLEMENTATION.md` for new APIs, packets, files, or hooks;
- `TESTING.md` with commands actually run and what was not verified; and
- `DECISIONS.md` when choosing a transport, plugin ABI, authority rule, persistence rule, or incompatible protocol
  behavior.

