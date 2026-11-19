#!/usr/bin/env python3
"""Build and inspect the raw development archive imported by Savage Wind Mod Manager."""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import tempfile
import zipfile


ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "mods" / "multiplayer" / "swmm"
VERSION_FILE = ROOT / "mods" / "multiplayer" / "VERSION"
INFO = "info.json"
MARKER = "romfs/System/SavageWind/Multiplayer/package.swmp"
EXPECTED = {INFO, MARKER}
MAX_ENTRY_SIZE = 16 * 1024
MARKER_TEXT = "SWMP_PACKAGE_V1\nid = org.savagewind.multiplayer\nprotocol = 1\nruntime = native\n"


def project_version() -> str:
    version = VERSION_FILE.read_text(encoding="utf-8").strip()
    if not version or any(c.isspace() for c in version):
        raise ValueError("mods/multiplayer/VERSION is invalid")
    return version


def validate(info: bytes, marker: bytes) -> None:
    if max(len(info), len(marker)) > MAX_ENTRY_SIZE:
        raise ValueError("SWMM package entry is too large")
    meta = json.loads(info.decode("utf-8"))
    version = project_version()
    if not isinstance(meta, dict) or meta.get("name") != "Savage Wind Multiplayer":
        raise ValueError("SWMM info.json has the wrong name")
    if meta.get("version") != version or meta.get("platform") != "switch":
        raise ValueError("SWMM info.json version/platform does not match the project")
    if marker != MARKER_TEXT.encode("ascii"):
        raise ValueError("SWMM package marker is invalid")


def build(output: Path) -> None:
    info = (SOURCE / INFO).read_text(encoding="utf-8").encode("utf-8")
    marker = (SOURCE / MARKER).read_text(encoding="utf-8").encode("utf-8")
    validate(info, marker)
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=output.parent) as scratch:
        staged = Path(scratch) / output.name
        with zipfile.ZipFile(staged, "w") as archive:
            for name, data in ((INFO, info), (MARKER, marker)):
                # Stored entries avoid variations in compression between Python builds.
                entry = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
                entry.create_system = 3
                entry.external_attr = 0o100644 << 16
                archive.writestr(entry, data)
        check(staged)
        os.replace(staged, output)
    print(output)


def check(package: Path) -> None:
    with zipfile.ZipFile(package) as archive:
        names = archive.namelist()
        if len(names) != len(EXPECTED) or set(names) != EXPECTED:
            raise ValueError(f"unexpected SWMM package entries: {sorted(names)}")
        if any(entry.file_size > MAX_ENTRY_SIZE for entry in archive.infolist()):
            raise ValueError("SWMM package entry is too large")
        validate(archive.read(INFO), archive.read(MARKER))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("package", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    if args.check:
        check(args.package)
        print(f"{args.package}: ok")
    else:
        build(args.package)


if __name__ == "__main__":
    main()

