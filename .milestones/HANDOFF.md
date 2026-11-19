# Agent handoff

## Resume checkpoint

- Branch `multiplayer/windows-test`, working-tree checkpoint based on `5b41d90` (`adjusted milestones`).
  The synthetic lobby baseline is `11ed467`.
- M0 is complete for synthetic scope. M0.5 is partial with a validated deterministic SWMM marker archive and
  a native package proposal. M1 is partial with the synthetic lobby, a Windows encrypted loopback probe,
  and a standalone draft ABI lifecycle fixture.
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
- `tests/test_multiplayer_webrtc.c` proves that libdatachannel's three stream modes can carry the lobby and
  test messages over encrypted loopback sockets. See [D012](DECISIONS.md#d012--transport-backend-not-yet-chosen).
- `mods/multiplayer/native_abi_draft.h` and `tests/test_multiplayer_native.c` exercise the proposed native
  boundary outside the game. [NATIVE_ABI.md](NATIVE_ABI.md) describes the draft manifest and approval handoff.

## Verification

Windows passed the synthetic checks, eight-Link demo, native fixture lifecycle, WebRTC loopback, and marker ZIP
validation. The Linux x86-64 native fixture and its test host cross-compiled. See [TESTING.md](TESTING.md) for
toolchain details and the scope of each result.

Linux runtime tests, mixed-platform internet sessions, forced TURN, the in-game module lifecycle, SWMM GUI
installation, and gameplay are pending.

## Pick up here

1. Run the standalone, native fixture and WebRTC targets on Linux. Record host architecture and toolchain.
2. Prototype authenticated rendezvous and bounded transport lifecycle, then exercise direct and forced TURN paths
   across separate networks and between Windows and Linux. Decide whether to adopt libdatachannel after these checks.
3. Review the [native ABI and SWMM proposal](NATIVE_ABI.md) with both maintainers. Agree on platform selection,
   runtime loading and native-code consent before implementing either production side.
4. Before calling packaging complete, compile the native module for each platform, produce the SWMM archive, and
   verify drag-and-drop import through uninstall using the matrix in [TESTING.md](TESTING.md#swmm-release-verification).
5. Update this handoff and the linked records at the next full or partial milestone. Record the checkpoint, evidence,
   outstanding decisions, and the next independent task. See the [maintenance rule](README.md#maintenance-rule).
