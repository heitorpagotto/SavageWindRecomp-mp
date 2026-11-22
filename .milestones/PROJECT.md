# Project charter

## Product goal

Build native multiplayer into the Savage Wind fork for a complete cooperative playthrough of the main campaign
with up to eight players total. Each player controls their own Link and sees the others in their actual equipment.

Required final behavior

- The Multiplayer tab in F1 Settings and the Multiplayer page in the game's System > Options expose the same
  enable/disable setting and Host/Join/Leave controls.
- Hosting produces an invite code that other players can use to join over the internet.
- Up to eight players can share a session.
- Remote Links show position, movement and actions, armour, weapons, shield, and bow.
- Players can damage each other. Whether PvP is always on or a session option remains undecided.
- Enemy health and death are shared for cooperative combat.
- Main-story progression and story quests are shared.
- Chests grant loot to every eligible player.
- The fork builds and runs on every supported Savage Wind platform. The maintained targets are Windows and
  Linux/Vulkan, and players can join the same lobby across compatible builds.
- With multiplayer disabled, ordinary single-player play remains available. Disabling during a session ends
  local participation and clears multiplayer resources and placeholders.

The project owner manually updates the fork as new Savage Wind releases arrive. Keep the multiplayer core separate
from upstream-facing build, UI, settings, lifecycle, and guest-hook adapters so those updates stay focused.

## Experience constraints

- Do not add new art, models, textures, fonts, sounds, or other assets.
- Reuse the player's existing Link models and equipment variants at runtime.
- Do not redesign core gameplay. Multiplayer synchronization should preserve the original feel.
- Keep private player inventory, rupees, durability, and local settings private unless a deliberate cooperative
  transaction changes them.
- Preserve the user's game files, settings, saves, and unrelated worktree changes.
- The host's save is the campaign authority for protocol v1. Clients must receive a backup before shared persistent
  state is applied.

## Legal and repository constraints

The project inherits Savage Wind's hard rules:

- Never commit or distribute Nintendo files, game assets, keys, firmware, extracted files, generated game code,
  disassembly, or decompilation output.
- Never copy code, addresses, structures, or documentation from decompilation projects.
- Use synthetic fixtures for automated tests.
- Game-specific findings must come from the developer's own legal copy and must be expressed as version-aware
  patterns/sites, not committed proprietary data.
- Follow upstream's `test` branch convention for upstream contributions. The owner selects and integrates each new Savage Wind release into the multiplayer fork.
- Fix patch generators and site discovery instead of editing `generated/`.
- Primary and only supported game target here is 1.9.0.

## Reference projects

MilkBarStudios/PointCrow's Wii U multiplayer work is a behavioral reference: multiple visible players, equipment
replication, combat cooperation, and shared progress. Nothing is copied from it. Its code, assets, addresses, and
decompilation information must not enter this repository.

## Terminology

- **P2P transport:** gameplay packets travel between player machines rather than through a dedicated simulation
  server.
- **Host authority:** one peer orders persistent and contested gameplay state. P2P does not mean every peer has equal
  authority.
- **Rendezvous service:** a small internet service maps invite codes to connection offers/ICE candidates. It does not
  simulate gameplay.
- **TURN relay:** encrypted packet relay used only when NAT/firewall rules prevent a direct peer route.
- **Remote Link:** a proxy representation of another player, not a replacement for the game's singleton local Link.
- **Phase:** a compatible world/area/story instance in which players may see and affect one another.

## Explicit non-goals for early milestones

- Host migration is not part of protocol v1. The session ends safely when the host leaves.
- Matchmaking, public server browsers, competitive anti-cheat, and dedicated servers are not early requirements.
- DLC campaign synchronization comes after the base main campaign is validated.
- No claim of a playable multiplayer build should be made until at least lobby transport and remote Link rendering work in game.
