# Hair and armor

## M2.5 hair and equipped armor

- The 1.9.0 local player has retained actor indices for the head, upper body and lower body at
  offsets `0x23f0`, `0x2400` and `0x2410`. The guest thread resolves those indices temporarily.
  It sees the on-body actor, including a different Hylian Hood variant than the inventory name.
- The installed ActorInfo table supplies per-piece model identities and upgrade aliases.
  The original protocol 6 sent three model IDs in the reliable equipment revision.
  Each receiver looks up those IDs by slot in its own installed resources.
- A separate passive actor per slot plays Link's skeletal clips on the same root and starts at
  Link's current phase. The default head provides hair. Changing a piece preserves Link, the other
  armor pieces and rigid gear. Helmet variants, cloth simulation and DLC assets need further coverage.
- The [Windows test record](../testing/appearance.md#m25-hair-and-equipped-armor) has the verified equip, removal
  and rejoin path. A second replay verified a mixed outfit of Climber's Bandanna, Ancient Cuirass
  and Stealth Tights on the same proxy.

## M2.5 armor colors and base skin clipping

- Protocol 7 carries a native dye index (0 through 15) per armor slot with the reliable
  equipment revision. The sender reads the equipped actor. The receiver passes the dye
  through the native placement parameters so the model loader binds the correct palette
  textures before the piece appears. A dye change reloads only the affected armor slot.
- The receiver uses the native `HideMaterialGroup` rules. It restores Link's grouped meshes
  and reapplies coverage from the active pieces when the worn models change. Unequipped,
  pending and unavailable pieces leave their corresponding base meshes visible.
- The Windows replay verified representative dyes and masking on protocol 7.
  See the [test evidence](../testing/appearance.md#m25-armor-colors-and-base-skin-clipping)
  for the matching launcher, settings, and captures.
