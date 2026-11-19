#!/usr/bin/env python3
"""Build and inspect the raw development archive imported by Savage Wind Mod Manager."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import zipfile


ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "mods" / "multiplayer" / "swmm"
VERSION_FILE = ROOT / "mods" / "multiplayer" / "VERSION"
INFO = "info.json"
MARKER = "romfs/System/SavageWind/Multiplayer/package.swmp"
EXPECTED = {INFO, MARKER}


def project_version() -> str:
    version = VERSION_FILE.read_text(encoding="utf-8").strip()
    if not version or any(c.isspace() for c in version):
        raise ValueError("mods/multiplayer/VERSION is invalid")
    return version


def validate(info: bytes, marker: bytes) -> None:
    meta = json.loads(info.decode("utf-8"))
    version = project_version()
    if meta.get("name") != "Savage Wind Multiplayer":
        raise ValueError("SWMM info.json has the wrong name")
    if meta.get("version") != version or meta.get("platform") != "switch":
        raise ValueError("SWMM info.json version/platform does not match the project")
    text = marker.decode("utf-8")
    if not text.startswith("SWMP_PACKAGE_V1\n") or "protocol = 1\n" not in text:
        raise ValueError("SWMM package marker is invalid")


def build(output: Path) -> None:
    info = (SOURCE / INFO).read_bytes()
    marker = (SOURCE / MARKER).read_bytes()
    validate(info, marker)
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in ((INFO, info), (MARKER, marker)):
            entry = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
            entry.compress_type = zipfile.ZIP_DEFLATED
            entry.external_attr = 0o100644 << 16
            archive.writestr(entry, data)
    check(output)
    print(output)


def check(package: Path) -> None:
    with zipfile.ZipFile(package) as archive:
        names = set(archive.namelist())
        if names != EXPECTED:
            raise ValueError(f"unexpected SWMM package entries: {sorted(names)}")
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

