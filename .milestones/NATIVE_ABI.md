# Native module and SWMM proposal

Status: draft for Savage Wind and SWMM maintainer review. The only consumer of this ABI today is a standalone
fixture test. The release archive layout, approval record, and runtime loader need joint agreement.

## ABI and lifecycle

The [draft header](../mods/multiplayer/native_abi_draft.h) uses a versioned C function table and one exported
query symbol, `swmp_query_draft`. The host and module each declare the table byte size. The host checks the
returned version, size, required capabilities, and callback pointers before calling `init`. The module checks
the host table before using it. A version mismatch returns no table. Each side owns its allocations and keeps
its table alive through `shutdown`. The opaque instance belongs to the module and log messages are borrowed
for the duration of the call.

- `init` acquires module resources and leaves the lobby offline. On failure it frees partial resources and returns
  null. The host then closes the library without invoking instance callbacks.
- `poll` receives monotonic milliseconds on the owning thread and drains bounded work.
- `reset` leaves any session and releases per-boot state, including workers, sockets and queues. It is repeatable
  and leaves the instance ready for the next boot.
- `shutdown` finishes reset, quiesces module callbacks and frees the instance before the host closes the library.

In Savage Wind, call reset from the `RT_ON_RESET` lifecycle and shutdown on final process exit. Poll and UI commands
should have one owning thread. If the UI and module poll run on different threads, exchange copied commands and
status over bounded queues. A future UI capability should expose a read-only lobby snapshot and Host, Join, Leave
and Start commands. Use stable status and rejection codes so the host owns localization. Extend the ABI deliberately
before exposing any per-boot runtime access. The draft tables carry no guest addresses.

## Archive manifest and approval handoff

Propose a versioned manifest alongside SWMM metadata. Its fields identify the mod and release, native ABI,
supported Savage Wind builds, and a target list keyed by OS and architecture. Each target declares the entry library
and its bundled native dependencies as relative archive paths with byte sizes and SHA-256 digests. A release
should cover Windows x86-64 and Linux x86-64, and track Linux aarch64 alongside Savage Wind's experimental build.
Keep executables outside the deployed RomFS tree.

SWMM would validate the manifest and archive entries before staging them, reject ambiguous targets/paths, and show
an executable-code prompt for the selected target. Approval would cover the exact set of native file hashes for
that mod and target. Changing a native file invalidates that approval. The manager would stage approved files in a
mod-owned directory and pass an explicit enabled approval record to Savage Wind at launch. That record would
carry the canonical absolute entry path, approved file hashes, mod identity, ABI, and target.

Before loading, Savage Wind would check target, host build, identity, path and hashes against the approval record.
The loader needs a defined way to prevent staged-file replacement between validation and load, and a dependency
search policy scoped to the approved directory and operating-system libraries. These checks must precede the OS
load operation because native library initialization can execute code before the ABI query. A hash verifies the
bytes covered by approval. Publisher authenticity requires a separate signature and key policy if maintainers
choose to support it.

Disable and uninstall would remove the enabled approval record before the next launch. Update and removal own
only mod files. Saves, settings and unrelated overlay files stay out of that lifecycle. Validate this flow across
both repositories using the [SWMM test matrix](TESTING.md#swmm-release-verification).

## Fixture

`make test-multiplayer-native` builds a project-owned DLL or shared object and exercises absent-module startup,
version and capability rejection, initialization, polling, reset, shutdown and unload. The test loads only the
explicit fixture path passed by the recipe. The fixture has no game, network, or SWMM dependency and is kept
outside the development marker ZIP.

`make multiplayer-native-fixture-package` uses Zig to compile the fixture for Windows x86-64, Linux x86-64,
and Linux aarch64. Set `MP_FIXTURE_ZIG` to the Zig executable when it is outside PATH. The output is
`build/mods/SavageWind-Multiplayer-0.1.0-dev-native-abi-fixture.zip`. Its `native-fixture.json` lists the
module ID, version, draft ABI, entry symbol, and each binary's path, size, and SHA-256. The archive validator
checks PE/ELF target headers and recomputes the manifest. This tests the proposed structure with real binaries.
The fixture format carries no execution approval and is kept outside SWMM's import path.
