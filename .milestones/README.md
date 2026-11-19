# Savage Wind Multiplayer handoff

Last updated: 2026-11-19 (America/Sao_Paulo)

This directory is the durable project memory for the Savage Wind peer-to-peer multiplayer mod. It is written so a
new developer or agent can resume work without conversation history. Treat statements here as a snapshot: verify the
branch, source, and tests before changing code, then update these files when a decision or milestone changes.

## Current truth

- M1 working branch: `multiplayer/windows-test`. Bootstrap branch: `multiplayer/bootstrap`.
- Resume checkpoint: working tree based on `1bde6b5` on `multiplayer/windows-test`. Historical upstream
  `test` baseline: `f488af7`.
- Target: BOTW Switch 1.9.0 on every supported Savage Wind platform. The current release matrix is Windows and
  Linux/Vulkan.
- Distribution: compile the module for each supported target and provide SWMM-compatible drag-and-drop archives.
- Product target: host plus seven clients, internet P2P transport, host-authoritative cooperative campaign.
- Implemented: protocol and sessions, a mock transport and demo, the development marker and native ABI fixtures,
  the marker-gated Windows/Linux lobby UI, an eight-peer WebRTC adapter, HTTPS rendezvous, and the Great Plateau
  placeholder lifecycle.
- Windows presence checks showed two placeholders each in three simultaneously rendered worlds.
  See the [test record](TESTING.md#m1-windows-presence-check) for the observations and local replay commands.
- M1 work remaining: native Linux and mixed-platform execution, and the two-network direct/relay acceptance test.
- The SWMM marker passed install, deploy, and uninstall through stock SWMM 0.1.3's Windows CLI with synthetic
  data and through its GUI with a scratch 1.9.0 test build. A separate draft ABI fixture archive contains compiled
  Windows and Linux binaries for maintainer review.
- No Nintendo files, extracted data, generated game code, saves, assets, or decompilation output were added.
- Bootstrap is committed at `2b9a27c` and the synthetic M1 lobby at `11ed467`. Verify the branch and local edits before resuming.

## Read in this order

Start with [HANDOFF.md](HANDOFF.md) for the current checkpoint and the first independent task. Then read:

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
11. [NATIVE_ABI.md](NATIVE_ABI.md) — draft C ABI, native manifest, and approval handoff for maintainer review.

## Quick verification

From the repository root:

```sh
make CC=gcc test-multiplayer
make CC=gcc multiplayer-demo
make multiplayer-swmm-package
make test-multiplayer-swmm
make test-multiplayer-native
rtk git diff --check
```

Expected demo summary:

```text
Lobby ready: 8/8 Links; host authority at peer 0
PASS: 8 Links, 560 snapshot deliveries, 1 stale duplicate rejected
```

Clang is the normal project compiler. Use `CC=gcc` for GCC or see [TESTING.md](TESTING.md) for the Windows
standalone verification and the opt-in WebRTC test.

## Maintenance rule

At every milestone or partial milestone checkpoint, update [HANDOFF.md](HANDOFF.md) with the resume commit or
working-tree status, what shipped, the verification results and gaps, any decisions waiting on maintainers, and the
first task the next agent can execute. Update the relevant records in the same change:

- `state.json` for status, version, test, or branch changes;
- `MILESTONES.md` for completed acceptance criteria;
- `IMPLEMENTATION.md` for new APIs, packets, files, or hooks;
- `TESTING.md` with commands actually run and what was not verified; and
- `DECISIONS.md` when choosing a transport, plugin ABI, authority rule, persistence rule, or incompatible protocol
  behavior.

