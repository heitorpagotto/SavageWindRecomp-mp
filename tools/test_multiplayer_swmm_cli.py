#!/usr/bin/env python3
"""Exercise the marker through SWMM 0.1.3's portable CLI with a synthetic data root."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

import package_multiplayer as package
from botw_save_convert import yaz0_decompress
from test_save_convert import sarc


def exercise(executable: Path, scratch: Path) -> None:
    swmm = scratch / executable.name
    shutil.copyfile(executable, swmm)
    config = scratch / "config"
    temp = scratch / "temp"
    game = scratch / "synthetic-game"
    romfs = game / "extracted" / "romfs"
    overlay = scratch / "overlay"
    for directory in (config, temp, romfs / "Pack"):
        directory.mkdir(parents=True)

    # SWMM adds a title-screen text override during merge. An empty inner message archive
    # lets it exercise that path without importing any game files.
    language_pack = romfs / "Pack" / "Bootup_USen.pack"
    bootup = sarc({"Message/Msg_USen.product.ssarc": sarc({})})
    language_pack.write_bytes(bootup)
    settings = {
        "system_7z": False,
        "storage_dir": str(scratch / "data"),
        "savage_wind": {"executable": str(game / "savagewind.exe")},
        "switch_config": {
            "language": "USen",
            "profile": "Default",
            "dump": {
                "bin_type": "Nintendo",
                "endian": "Switch",
                "source": {
                    "type": "Unpacked",
                    "host_path": str(romfs),
                    "content_dir": str(romfs),
                    "update_dir": None,
                    "aoc_dir": None,
                },
            },
            "deploy_config": {
                "output": str(overlay), "method": "Copy", "auto": False, "layout": "SavageWind",
            },
        },
    }
    (config / "settings.yml").write_text(json.dumps(settings), encoding="utf-8")
    archive = scratch / "multiplayer-marker.zip"
    package.build(archive)

    env = {**os.environ, "TMPDIR": str(temp), "TMP": str(temp), "TEMP": str(temp),
           "SAVAGEWIND_DATA_ROOT": str(game)}
    marker = overlay / package.SOURCE_MARKER.removeprefix("romfs/")
    expected_marker = package.MARKER_TEXT.encode("ascii")
    for command in (["install", str(archive)], ["uninstall", "0"]):
        result = subprocess.run([str(swmm), "--portable", "--deploy", *command],
                                cwd=scratch, env=env, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, text=True, errors="replace", timeout=60)
        print(result.stdout, end="")
        result.check_returncode()
        if command[0] == "install":
            deployed = marker.read_bytes()
            # SWMM compresses resource extensions beginning with "s".
            if deployed.startswith(b"Yaz0") and int.from_bytes(deployed[4:8], "big") != len(expected_marker):
                raise ValueError("SWMM deployed a marker with an unexpected decompressed size")
            if yaz0_decompress(deployed) != expected_marker:
                raise ValueError("SWMM deployed a different marker")
        elif marker.exists() or any((scratch / "data" / "mods").glob("*.zip")):
            raise ValueError("SWMM left the marker deployed or the mod installed")
    if language_pack.read_bytes() != bootup or [p for p in romfs.rglob("*") if p.is_file()] != [language_pack]:
        raise ValueError("SWMM altered the synthetic base data")
    print("SWMM portable CLI: install, deploy, and uninstall ok")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("swmm", type=Path, help="executable from the official portable SWMM archive")
    args = parser.parse_args()
    executable = args.swmm.resolve(strict=True)
    if not executable.is_file():
        parser.error("pass the portable SWMM executable path")
    scratch_root = package.ROOT / "build"
    scratch_root.mkdir(exist_ok=True)
    # Windows can retain an executable handle briefly after its process exits.
    with tempfile.TemporaryDirectory(prefix="swmm-marker-test-", dir=scratch_root,
                                     ignore_cleanup_errors=os.name == "nt") as scratch:
        exercise(executable, Path(scratch))


if __name__ == "__main__":
    main()
