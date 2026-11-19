# Agent handoff

## Resume checkpoint

- Branch `multiplayer/windows-test`, uncommitted working tree based on `1bde6b5`. Preserve the earlier M0.5
  changes and new M1 files. M0 is complete for synthetic scope. M0.5 still needs the SWMM maintainer-approved
  native loader and archive contract. Work in this checkpoint is M1 only.
- Read [AGENTS.md](../AGENTS.md), [CONTRIBUTING.md](../CONTRIBUTING.md), and the [milestone index](README.md).
- M1 has a Windows/Linux opt-in test build, a localized lobby UI, HTTPS invite signaling, encrypted
  WebRTC channels, and a 1.9.0 Great Plateau placeholder path. The Windows SWMM GUI lifecycle passed for the
  development marker, including single-player boot after removal. [Test record](TESTING.md#m1-windows-swmm-gui)
- The user provided a supported BOTW 1.9.0 installation for local Windows work. The location and machine-local
  commands belong in the ignored `build/mp-work` folder. No separate Linux/Vulkan machine or second external
  network is available. Use fresh scratch settings and saves, and leave installation files read-only.

## What is in the checkpoint

- `mods/multiplayer/webrtc.c` is the reusable libdatachannel adapter. It negotiates three channels, derives each
  admitted link's binding from the authenticated offer exchange, cleans up provisional connections and keeps
  transport connection IDs unique across stop/start. The connection and HELLO have separate deadlines.
- `mods/multiplayer/rendezvous.c` uses libcurl multi with HTTPS certificate verification and asynchronous DNS.
  `tools/multiplayer_rendezvous.py` runs a bounded in-memory service on loopback for a TLS deployment. The
  service brokers SDP and expires invites and offers. SDP carries ICE and DTLS fingerprints, never guest data.
- `runtime/multiplayer.c` owns the lobby on the main window thread, publishes copies to the render and guest
  threads, and resets at `RT_ON_RESET`. `runtime/ui/multiplayer.mm` provides the localized tab. The test
  build validates the deployed raw/Yaz0 marker and the game version before exposing the tab.
- `runtime/multiplayer_presence.c` reconciles the guest roster from the native menu update thread. The request
  helper builds its own descriptor. The take helper clears creation suspension, and a retained actor index
  identifies the spawned rock for native deletion. A Windows scratch probe verified visible spawn and removal.
  The audited sites are in the strict `MP_PRESENCE_SITES` group.
- The stock SWMM marker enables code compiled into the test executable. See [NATIVE_ABI.md](NATIVE_ABI.md)
  for the separately loadable release module and its approval proposal.

## Pick up here

The [Windows presence record](TESTING.md#m1-windows-presence-check) contains passing screenshots from the host,
first client, and late client on the Great Plateau and records the earlier device-loss and loading-screen attempts.
[Replay commands and evidence paths](TESTING.md#replay-on-the-windows-development-machine)

1. Run the HTTPS and WebRTC suites natively on Linux, then verify Windows-to-Linux and Linux-to-Windows hosting.
2. Verify direct ICE and restrictive NAT with TURN from two external networks. A same-machine TURN server alone
   does not preclude peer-reflexive direct routes, so verify selected candidate pairs.

After each result, update [TESTING.md](TESTING.md), [MILESTONES.md](MILESTONES.md), and [state.json](state.json).
