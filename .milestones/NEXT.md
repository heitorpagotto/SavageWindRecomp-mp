# Resume guide and next work

Start with the [native multiplayer handoff](NATIVE_MULTIPLAYER_HANDOFF.md) and the
[test record](TESTING.md). M1 is partial.

1. Use scratch settings and saves to finish any remaining Windows gameplay checks from the acceptance matrix.
   Regenerate and rebuild whenever the guest sites or patches change.
2. Build and run natively on Linux/Vulkan. Test Windows-to-Linux and Linux-to-Windows hosting. Verify direct ICE
   and forced TURN across two external networks, and record the selected candidate types.
3. Update the [test record](TESTING.md), [milestone ledger](MILESTONES.md), [handoff](NATIVE_MULTIPLAYER_HANDOFF.md),
   and [state](state.json). Begin M2 after M1 acceptance. Define wire identity and resync before forwarding
   gameplay state for reusable peer slots.
