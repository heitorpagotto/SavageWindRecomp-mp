# Resume guide and next work

M1 implements an opt-in Windows/Linux test-build lobby, the reusable WebRTC adapter, HTTPS rendezvous, and
the Great Plateau placeholder lifecycle.
See [HANDOFF.md](HANDOFF.md) for the working tree, local Windows build inputs and first outstanding task.

## Windows verification

The [Windows SWMM GUI lifecycle](TESTING.md#m1-windows-swmm-gui) passed for the development marker, including
single-player boot after removal. The [Windows presence test](TESTING.md#m1-windows-presence-check) verified
two visible placeholders each in the host, first client, and late client on the Great Plateau. Use the replay
commands when rechecking a new build.

## Finish M1 network acceptance

- Run the HTTPS and WebRTC tests on native Linux and repeat with Windows-to-Linux and Linux-to-Windows.
- Run a controlled shared HTTPS origin and TURN server for two external networks. Test both direct ICE and the
  selected relay path, record candidate types and confirm cleanup.
- Keep the development test executable separate from the ordinary Savage Wind build. The cross-repository
  release-module approval contract remains in [NATIVE_ABI.md](NATIVE_ABI.md).

Gameplay state replication begins after M1. Define the membership generation and resync protocol before forwarding
state from peers whose slots may be reused. Keep story transaction and save rules in [ARCHITECTURE.md](ARCHITECTURE.md).
