# Agent handoff

## Resume checkpoint

- Branch `multiplayer/windows-test`, checkpoint `11ed467` (`m1 partially implemented`).
- M0 is complete for synthetic scope. M0.5 is partial with a deterministic SWMM marker archive. M1 is partial
  with a transport-driven synthetic lobby.
- Product requirements include every supported Savage Wind platform (currently Windows and Linux/Vulkan),
  interoperability between platform builds, and compiled archives for SWMM drag-and-drop installation.
- Read [AGENTS.md](../AGENTS.md), [CONTRIBUTING.md](../CONTRIBUTING.md), and the [milestone index](README.md).
  Check the branch and working tree before editing. Preserve local changes and player files.

## What is in the checkpoint

- `mods/multiplayer/lobby_protocol.h` and `protocol.c` define and encode the reliable control payloads.
- `lobby.c` handles admission, roster, handshake deadlines, start, and disconnect. The transport supplies the link
  identity through `transport.h`.
- `mock_transport.c`, `tests/test_multiplayer.c`, and `demo.c` provide the in-process harness. The demo drives the
  lobby over the mock, then checks snapshot deliveries using the resulting sessions.
- `tools/package_multiplayer.py` builds and validates the marker archive. See [SWMM.md](SWMM.md) for the native
  build and packaging contract still to be agreed with the maintainers.

## Verification carried forward

Windows standalone tests passed with the Zig/Clang driver, including undefined-behavior trap mode. The demo
admitted eight Links, verified 560 snapshot deliveries, and rejected one stale duplicate. The development archive
passed Python validation. See [TESTING.md](TESTING.md) for commands, historical results, and pending platform checks.

Linux verification of the M1 lobby, mixed-platform internet sessions, the runtime module lifecycle, SWMM GUI
installation, and gameplay are pending. This handoff update changes documentation only.

## Pick up here

1. Re-run the standalone checks from [NEXT.md](NEXT.md). Run the checkpoint on Linux if a Linux environment is
   available, and record the toolchain and results.
2. Start the M1 transport selection spike in [NEXT.md](NEXT.md). Compare buildable Windows/Linux candidates
   against the existing transport contract. Confirm licensing, encryption, ICE/STUN/TURN, and channel semantics.
   Record what was actually built or exercised and the reasoning behind any choice in [DECISIONS.md](DECISIONS.md).
3. Develop the native ABI and SWMM manifest/approval proposal alongside that spike. Platform selection, loading, and
   consent need agreement with the Savage Wind and SWMM maintainers before implementing the native loader.
4. Before calling packaging complete, compile the native module for each platform, produce the SWMM archive, and
   verify drag-and-drop import through uninstall using the matrix in [TESTING.md](TESTING.md#swmm-release-verification).
5. Update this handoff and the linked records at the next full or partial milestone. Record the checkpoint, evidence,
   outstanding decisions, and the next independent task. See the [maintenance rule](README.md#maintenance-rule).
