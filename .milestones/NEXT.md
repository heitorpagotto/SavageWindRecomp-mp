# Resume guide and next work

Start with the [M2 acceptance follow-up](HANDOFF.md#m2-acceptance-follow-up) for the matching build
and the [test record](TESTING.md#m2-acceptance-follow-up) for the current evidence.

1. Finish the extended eight-game session. The local Windows fixture exercise covered seven remote
   proxies on one renderer, including late join, changing actions and health, and measured 60/90/120 FPS.
   A separate two-game session passed movement, facing and health checks under UDP loss and jitter.
2. Build and inspect the AppImage on Linux with Podman. Windows ZIP and two native Windows shrine
   round trips already passed. See the [milestone ledger](MILESTONES.md#m2--remote-link-presence)
   before marking M2 complete.
3. The owner will test mixed Windows/Linux travel and visibility manually and report any issues. This is
   independent of M2 acceptance. Meet at the same waypoint on matching builds and record both logs if
   either peer disappears within the supported range.
4. After M2, start [M2.5](MILESTONES.md#m25--time-weather-and-full-link-synchronization) with host time,
   weather, equipment and animation. See the [appearance probe](M2_POSITION_HANDOFF.md#resume) and the
   [dependency findings](DIRECT_MULTIPLAYER_HANDOFF.md#security-findings) when preparing distribution.
