# Resume guide and next work

Start with the [M2 position and remote Link handoff](M2_POSITION_HANDOFF.md). The project owner accepted
M1 after joining a Linux host from both Windows and Linux and approved starting M2.

1. Continue M2 with native idle, walk and jump animation for the Plateau proxy. Build on the existing
   actor alias, position and facing updates. Verify the gameplay matrix before expanding to equipment
   and other areas.
2. Continue the failed host-direction investigation separately. Capture the multiplayer log lines for
   the accepted reply, WebRTC connection and lobby handshake from both players. Use fresh invites.
   See the [test record](TESTING.md#direct-lobby-field-reports) and [direct handoff](DIRECT_MULTIPLAYER_HANDOFF.md).
3. Follow up on the [OpenSSL and DTLS findings](DIRECT_MULTIPLAYER_HANDOFF.md#security-findings)
   when preparing builds for distribution. Verify the bundled OpenSSL package and negotiated DTLS version.
