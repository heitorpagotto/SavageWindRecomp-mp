#!/usr/bin/env python3
"""Exercise the runtime opt-in against raw and Yaz0-compressed SWMM markers."""
import subprocess
import sys
from pathlib import Path
from tempfile import TemporaryDirectory

from package_multiplayer import SOURCE, SOURCE_MARKER


def main() -> None:
    executable = Path(sys.argv[1]).resolve(strict=True)
    payload = (SOURCE / SOURCE_MARKER).read_text(encoding="ascii").encode("ascii")
    header = b"Yaz0" + len(payload).to_bytes(4, "big") + bytes(8)
    packed = header + b"".join(b"\xff" + payload[i:i + 8] for i in range(0, len(payload), 8))

    with TemporaryDirectory() as scratch:
        overlay = Path(scratch)
        marker = overlay / Path(SOURCE_MARKER).relative_to("romfs")
        marker.parent.mkdir(parents=True)

        cases = [
            ("missing", None, False),
            ("raw", payload, True),
            ("stock SWMM encoding", packed, True),
            ("wrong protocol", payload.replace(b"protocol = 1", b"protocol = 2"), False),
            ("trailing data", payload + b"\0", False),
            ("truncated Yaz0", packed[:-1], False),
            ("oversized Yaz0 output", packed[:4] + b"\xff" * 4 + packed[8:], False),
            ("oversized file", packed + bytes(256), False),
        ]
        for label, contents, enabled in cases:
            if contents is not None:
                marker.write_bytes(contents)
            try:
                subprocess.run([executable, overlay, "enabled" if enabled else "disabled"], check=True)
            except subprocess.CalledProcessError as exc:
                raise RuntimeError(f"marker case failed: {label}") from exc

    print("multiplayer SWMM marker: raw, Yaz0, and invalid inputs ok")


if __name__ == "__main__":
    main()
