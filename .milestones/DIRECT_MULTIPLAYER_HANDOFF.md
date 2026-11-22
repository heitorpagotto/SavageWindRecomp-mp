# Direct multiplayer handoff

Checkpoint 2026-11-22 on `multiplayer/windows-test`, implementation commit `007973d` (`adjusted mp`). M1 remains
partial. The owner approved direct internet play with two messages exchanged privately. One player's game hosts
the lobby. Players can join from another network when ICE finds a direct route. No separate signaling deployment
is part of this flow. The owner handles commits and pushes.

## Player flow and implementation

1. Enable multiplayer in **F1 Settings** or **System > Options**. The host selects **Host lobby**, then
   **Copy invite code**, and sends the complete message privately.
2. The joining player copies the invite, selects **Paste invite and join**, then uses **Copy reply** and sends
   the reply back privately.
3. The host copies the reply and selects **Paste reply**. **New invite** makes a separate offer for the next player
   or replaces an expired offer. Existing peers stay connected. [Player guide](../mods/multiplayer/README.md#build-and-play)

- [direct.c](../mods/multiplayer/direct.c) encodes the WebRTC offer and answer in two bounded clipboard messages.
  The host generates a random binding for each invite and accepts one matching reply. An outstanding invite
  expires after five minutes. [webrtc.c](../mods/multiplayer/webrtc.c) supports either lobby role making the offer.
- [The multiplayer runtime](../runtime/multiplayer.c) owns clipboard access, signaling, and transport on the
  window thread. The [F1 overlay](../runtime/ui/multiplayer.mm) and [native Options](../runtime/ui/game_options.mm)
  share the enable setting and runtime actions. Disable, Leave, and reboot tear down the session and clear the
  roster the guest update uses for Plateau NPCs.
- The default public STUN endpoint discovers candidate addresses. The game opens peer connections on Host or Join.
  The native direct flow uses no TURN relay. Restrictive network pairs can fail to connect. A development build can
  override STUN with `SAVAGEWIND_MP_STUN` or set it empty for loopback checks. [Runtime configuration](../runtime/multiplayer.c)
- Automatic HTTPS rendezvous was initially added to M1 and prototyped with a launcher readiness check and on-demand
  service startup. Scratch checks covered Caddy TLS routing, retry, certificate and protocol rejection, and Windows
  gameplay startup with multiplayer enabled and disabled. The launcher verified the endpoint during recompilation
  and finished game setup. The owner then chose the two-message flow to avoid deployment. The launcher setup code
  and installer were removed. The earlier HTTPS transport is still covered by `make test-multiplayer-rendezvous`.
- The [dependency build](../tools/build_multiplayer_deps.sh) pins libdatachannel v0.24.6 with libjuice v1.7.4 and
  usrsctp at `fec583d`. Media and WebSocket support are disabled. The Linux wrapper no longer builds libcurl for
  multiplayer. The Windows executable's import table has no direct libcurl entry. The original build guides and
  milestone records now describe the direct flow. [Build guide](../docs/build-windows-and-appimage.md)

## Checks completed

- The Windows app and both menus compiled and linked in MSYS2 UCRT64. The direct transport sources cross-compiled for
  Linux x86-64. The translation check reported zero missing and zero unused entries.
- `make test-multiplayer-direct` passed with eight peers on Windows. It covered invalid and stale messages,
  single-use replies, invite expiry, a full roster, Start, and host departure. The same-machine test also passed
  with the public STUN endpoint configured. That run did not establish which candidate type was selected.
- A separate two-process Windows harness exercised the production runtime's Host, Join, Copy, Paste reply, and
  disable actions with an isolated clipboard fixture. The core, WebRTC, and historical HTTPS tests also passed.
  Use the repository test target for a repeatable check. [Direct test source](../tests/test_multiplayer_direct.c)

Older Windows gameplay and TURN evidence came from the HTTPS prototype. Use the current direct build for native
menu checks, mixed-platform hosting, external-network gameplay, and NPC cleanup verification. Local toolchains and
logs may still be under the ignored `build/m1-tools/` directory. [Test record](TESTING.md)

## Security findings

- **CVE-2026-95690 applies to the OpenSSL version in the inspected Windows build.** The staged
  `build/windows-local/libcrypto-4-x64.dll` reported **4.1.0**. OpenSSL lists it as affected by excess memory
  allocation during DTLS handshakes and lists **4.1.1** as the fix target. OpenSSL rates the issue low severity.
  Practical denial of service through the game's ICE layer and connection limits remains unverified. The Linux
  AppImage's OpenSSL package revision also remains unverified. [OpenSSL CVE record](https://www.cve.org/CVERecord?id=CVE-2026-95690)
  · [Ubuntu package status](https://ubuntu.com/security/CVE-2026-95690)
- **The bundled libdatachannel sets its OpenSSL minimum to DTLS 1.0.** The IETF directs implementations to reject
  negotiation of DTLS 1.0. Actual negotiation in the local build remains unverified because OpenSSL policy and
  cipher configuration also constrain it. Requiring DTLS 1.2 explicitly is open hardening work.
  [Pinned libdatachannel configuration](https://github.com/paullouisageneau/libdatachannel/blob/6b1e2e620f1e37f0eafeee702eaea0043cb305fd/src/impl/dtlstransport.cpp#L761-L779)
  · [RFC 8996](https://www.rfc-editor.org/rfc/rfc8996.html#section-6)
- **Peer identity relies on the private invite exchange.** DTLS verifies the certificate fingerprints carried
  in the messages. Someone who can replace those messages could substitute a peer identity. The messages also
  expose candidate IP addresses to their recipients, and public STUN sees the source IP of discovery traffic.
  Exchange the invite and reply through a trusted private chat.
  [WebRTC security architecture](https://www.rfc-editor.org/rfc/rfc8827.html)
  · [WebRTC IP address handling](https://www.rfc-editor.org/rfc/rfc8828.html)

## Resume work

1. Read [AGENTS.md](../AGENTS.md) and [CONTRIBUTING.md](../CONTRIBUTING.md). Keep game data, generated code, saves,
   certificates, and real invites local. Use separate scratch saves and settings for each native test instance.
2. For the security follow-up, check the OpenSSL library actually shipped in each build, then verify a vendor
   package with the CVE fix. Investigate enforcing a DTLS 1.2 minimum through the dependency build and verify the
   negotiated version in a direct session. Both dependency changes remain open.
3. Run `make test-multiplayer-direct` in the platform build environment. Build and launch the Linux/Vulkan
   AppImage. Use matching source and BOTW 1.9.0 builds on Windows and Linux.
4. Exercise both menus and both mixed-platform host directions on two independent networks. Record the selected
   ICE candidate types and keep IP addresses local. Verify the full roster, rejection, Leave, disable, reboot, and NPC cleanup against
   [M1 acceptance](TESTING.md#native-m1-acceptance). Record the results in [TESTING.md](TESTING.md) and [state.json](state.json).
