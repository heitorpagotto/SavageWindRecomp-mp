# Resume guide and next work

Start with the [native multiplayer handoff](NATIVE_MULTIPLAYER_HANDOFF.md) and the
[test record](TESTING.md). M1 is partial.

1. Run [build-linux.sh](../build-linux.sh) on x86-64 Linux with Podman and validate its AppImage on Vulkan.
2. Use scratch settings and saves to finish the [M1 acceptance matrix](TESTING.md#native-m1-acceptance),
   including the full roster, explicit Leave, localized rejection, and reboot with visible NPCs.
   Regenerate and rebuild whenever the guest sites or patches change.
3. Test Windows-to-Linux and Linux-to-Windows hosting. Verify direct ICE
   and forced TURN across two external networks, and record the selected candidate types.
4. Update the [test record](TESTING.md), [milestone ledger](MILESTONES.md), [handoff](NATIVE_MULTIPLAYER_HANDOFF.md),
   and [state](state.json). Begin M2 after M1 acceptance. Define wire identity and resync before forwarding
   gameplay state for reusable peer slots.
