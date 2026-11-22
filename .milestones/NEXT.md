# Resume guide and next work

Start with the [native multiplayer handoff](NATIVE_MULTIPLAYER_HANDOFF.md). The prototype's Windows
presence and encrypted loopback evidence is in [TESTING.md](TESTING.md). M1 is still partial.

1. Trace `MP_M1_TEST`, the boot gate, shared runtime commands, M1 presence hooks, and both Settings implementations.
   Reuse the lobby, transport, and actor lifecycle code. Remove the prototype activation and packaging dependencies
   from the multiplayer build and keep the platform and guest integration narrowly scoped.
2. Make the feature part of the native Windows and Linux builds, add one enable/disable setting, and connect the
   F1 Multiplayer tab and in-game Multiplayer page to the same actions and state. Use existing settings and
   translation conventions. Test event handling across the UI, window, and guest thread boundaries.
3. Regenerate and rebuild if guest sites or patches change. Re-run Windows gameplay with scratch settings and
   saves. Exercise enable/disable, Host, Join, Leave, late join, rejection, reboot, and visible actor cleanup.
4. Build and run natively on Linux/Vulkan. Test Windows-to-Linux and Linux-to-Windows hosting. Verify direct ICE
   and forced TURN across two external networks, and record the selected candidate types.
5. Update the [test record](TESTING.md), [milestone ledger](MILESTONES.md), [handoff](NATIVE_MULTIPLAYER_HANDOFF.md),
   and [state](state.json). Begin M2 after M1 acceptance. Define wire identity and resync before forwarding
   gameplay state for reusable peer slots.
