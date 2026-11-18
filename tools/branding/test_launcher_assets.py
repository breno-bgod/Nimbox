#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Offline checks for NimboPlay launcher identities on all platforms."""
import pathlib
import struct
import subprocess
import sys
import tempfile
import unittest

from make_launcher_assets import SIGNATURE, unpack_rgba

ROOT = pathlib.Path(__file__).resolve().parents[2]
SOURCE = ROOT / "assets/branding/nimboplay_launcher_256.png"


class BrandingTests(unittest.TestCase):
    def test_approved_external_icon_formats(self):
        source = SOURCE.read_bytes()
        self.assertTrue(source.startswith(SIGNATURE))
        width, height, rows = unpack_rgba(source)
        self.assertEqual((width, height), (256, 256))
        self.assertEqual(len(rows), 256)
        # The external icon is rounded and keeps transparent corners.
        self.assertEqual(rows[0][3], 0)
        self.assertTrue(any(row[3:-3:4] for row in rows[32:224]))

        with tempfile.TemporaryDirectory() as tmp:
            ps5 = pathlib.Path(tmp) / "icon0.png"
            ico = pathlib.Path(tmp) / "nimboplay.ico"
            subprocess.run([sys.executable,
                            str(ROOT / "tools/branding/make_launcher_assets.py"),
                            "--source", str(SOURCE), "--ps5", str(ps5),
                            "--ico", str(ico)], check=True)
            ps5_bytes = ps5.read_bytes()
            self.assertEqual((unpack_rgba(ps5_bytes)[0:2]), (512, 512))
            self.assertEqual(ps5_bytes[25], 6)  # RGBA8 for PS5
            ico_bytes = ico.read_bytes()
            self.assertEqual(ico_bytes[:6], struct.pack("<HHH", 0, 1, 1))
            self.assertEqual(ico_bytes[22:], source)

    def test_platform_references_use_external_icon(self):
        app = (ROOT / "android/app/build.gradle.kts").read_text()
        self.assertEqual(app.count('manifestPlaceholders["appIcon"] = "@mipmap/ic_launcher"'), 2)
        self.assertEqual(app.count('manifestPlaceholders["appRoundIcon"] = "@mipmap/ic_launcher_round"'), 2)
        for size in ("mdpi", "hdpi", "xhdpi", "xxhdpi", "xxxhdpi"):
            icon = ROOT / f"android/app/src/main/res/mipmap-{size}/ic_launcher.png"
            self.assertEqual(icon.read_bytes(), SOURCE.read_bytes())
        self.assertEqual(
            (ROOT / "android/app/src/main/res/drawable-nodpi/nimboplay_launcher.png").read_bytes(),
            SOURCE.read_bytes())
        self.assertIn('ICON "nimboplay_launcher.ico"', (ROOT / "windows/psbox.rc").read_text())
        self.assertIn("Name=NimboPlay", (ROOT / "linux/psbox.desktop").read_text())
        self.assertIn("NimboPlay Cloud Gaming",
                      (ROOT / "ps5/sce_sys/param.json").read_text())
        self.assertNotEqual(SOURCE.read_bytes(),
            (ROOT / "assets/branding/nimboplay_internal.png").read_bytes())


if __name__ == "__main__":
    unittest.main()
