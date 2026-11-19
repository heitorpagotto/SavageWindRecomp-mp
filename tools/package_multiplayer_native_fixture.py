#!/usr/bin/env python3
"""Package the project-owned native ABI fixture for testing the draft manifest."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

from package_multiplayer import ROOT, project_version, write_archive


MANIFEST = "native-fixture.json"
TARGETS = {
    "windows-x86_64": "native/windows-x86_64/multiplayer_native_fixture.dll",
    "linux-x86_64": "native/linux-x86_64/multiplayer_native_fixture.so",
    "linux-aarch64": "native/linux-aarch64/multiplayer_native_fixture.so",
}
MAX_BINARY_SIZE = 16 * 1024 * 1024


def check_binary(target: str, data: bytes) -> None:
    if not 64 <= len(data) <= MAX_BINARY_SIZE:
        raise ValueError(f"Invalid fixture size for {target}")
    if target == "windows-x86_64":
        pe, = struct.unpack_from("<I", data, 0x3c)
        if data.startswith(b"MZ") and 64 <= pe <= len(data) - 26 and data[pe:pe + 4] == b"PE\0\0":
            machine, = struct.unpack_from("<H", data, pe + 4)
            flags, optional_magic = struct.unpack_from("<HH", data, pe + 22)
            if machine == 0x8664 and flags & 0x2000 and optional_magic == 0x20B:
                return
    else:
        machine = 62 if target == "linux-x86_64" else 183
        if data.startswith(b"\x7fELF\x02\x01\x01") and struct.unpack_from("<HH", data, 16) == (3, machine):
            return
    raise ValueError(f"Fixture binary format or architecture does not match {target}")


def manifest(entries: dict[str, bytes]) -> bytes:
    targets = {}
    for target, path in TARGETS.items():
        data = entries[path]
        check_binary(target, data)
        targets[target] = {"path": path, "size": len(data), "sha256": hashlib.sha256(data).hexdigest()}
    document = {
        "format": 1,
        "kind": "abi-fixture",
        "mod_id": "org.savagewind.multiplayer",
        "mod_version": project_version(),
        "abi": "draft-1",
        "entry_symbol": "swmp_query_draft",
        "targets": targets,
    }
    return (json.dumps(document, indent=2, sort_keys=True) + "\n").encode("utf-8")


def license_bytes() -> bytes:
    return (ROOT / "LICENSE").read_text(encoding="utf-8").encode("utf-8")


def check(output: Path) -> None:
    with zipfile.ZipFile(output) as archive:
        names = archive.namelist()
        expected = {MANIFEST, "LICENSE", *TARGETS.values()}
        if len(names) != len(expected) or set(names) != expected:
            raise ValueError("Unexpected entries in native fixture archive")
        for entry in archive.infolist():
            limit = MAX_BINARY_SIZE if entry.filename in TARGETS.values() else 16 * 1024
            if entry.file_size > limit:
                raise ValueError(f"Oversized fixture entry {entry.filename}")
        entries = {path: archive.read(path) for path in TARGETS.values()}
        if archive.read(MANIFEST) != manifest(entries):
            raise ValueError("Native fixture manifest does not match the archive")
        if archive.read("LICENSE") != license_bytes():
            raise ValueError("Native fixture license does not match the project")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--check", action="store_true")
    mode.add_argument("--build", nargs=3, type=Path, metavar=("WINDOWS_X64", "LINUX_X64", "LINUX_ARM64"))
    args = parser.parse_args()
    if args.check:
        check(args.archive)
        print(f"{args.archive}: ok")
        return

    entries = {}
    for target, binary in zip(TARGETS, args.build):
        with binary.open("rb") as source:
            data = source.read(MAX_BINARY_SIZE + 1)
        entries[TARGETS[target]] = data
    write_archive(args.archive, {MANIFEST: manifest(entries), "LICENSE": license_bytes(), **entries}, check)


if __name__ == "__main__":
    main()
