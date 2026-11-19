#!/usr/bin/env python3
"""Exercise the marker archive's reproducibility and validation."""
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import warnings
import zipfile

import package_multiplayer as package


class MarkerPackageTest(unittest.TestCase):
    def setUp(self):
        scratch = tempfile.TemporaryDirectory()
        self.addCleanup(scratch.cleanup)
        self.scratch = Path(scratch.name)
        self.info = (package.SOURCE / package.INFO).read_bytes()
        self.marker = package.MARKER_TEXT.encode("ascii")

    def make_zip(self, entries):
        path = self.scratch / "invalid.zip"
        with zipfile.ZipFile(path, "w") as archive:
            for name, data in entries:
                archive.writestr(name, data)
        return path

    def test_marker_package_ignores_checkout_line_endings(self):
        output = self.scratch / "original.zip"
        package.build(output)
        source = self.scratch / "source"
        for name, data in ((package.INFO, self.info), (package.MARKER, self.marker)):
            path = source / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data.replace(b"\r\n", b"\n").replace(b"\n", b"\r\n"))
        rebuilt = self.scratch / "rebuilt.zip"
        with patch.object(package, "SOURCE", source):
            package.build(rebuilt)
        self.assertEqual(output.read_bytes(), rebuilt.read_bytes())
        with zipfile.ZipFile(output) as archive:
            for entry in archive.infolist():
                self.assertEqual(entry.create_system, 3)
                self.assertEqual(entry.compress_type, zipfile.ZIP_STORED)

    def test_rejects_duplicate_and_extra_entries(self):
        entries = [(package.INFO, self.info), (package.MARKER, self.marker)]
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", UserWarning)
            with self.assertRaisesRegex(ValueError, "unexpected SWMM package entries"):
                package.check(self.make_zip(entries + entries[:1]))
        with self.assertRaisesRegex(ValueError, "unexpected SWMM package entries"):
            package.check(self.make_zip(entries + [("extra", b"")]))

    def test_rejects_oversized_entry_and_foreign_marker(self):
        with self.assertRaisesRegex(ValueError, "entry is too large"):
            package.check(self.make_zip([(package.INFO, bytes(package.MAX_ENTRY_SIZE + 1)),
                                         (package.MARKER, self.marker)]))
        with self.assertRaisesRegex(ValueError, "marker is invalid"):
            package.check(self.make_zip([(package.INFO, self.info),
                                         (package.MARKER, self.marker.replace(b"savagewind", b"foreign"))]))


if __name__ == "__main__":
    unittest.main()
