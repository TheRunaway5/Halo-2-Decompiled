"""Fresh-install loading exports, checked against original decoder payloads."""
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import prepare_vista_loading as loading


class LoadingPreparationTests(unittest.TestCase):
    def test_unknown_bank_and_executable_are_rejected(self):
        with self.assertRaisesRegex(ValueError, "loading.bin SHA-256"):
            loading.decode(b"unverified input", 0)
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory)
            (source / "loading.bin").write_bytes(b"unverified input")
            (source / "halo2.exe").write_bytes(b"unverified executable")
            output = source / "export"
            with self.assertRaisesRegex(ValueError, "executable SHA-256"):
                loading.prepare(source, output, lambda _: None)
            self.assertFalse(output.exists())

    def test_optional_acceleration_preserves_standard_library_result(self):
        pixels = bytes((i * 53 + i // 11) % 256 for i in range(13 * 7 * 4))
        self.assertEqual(loading.resize(pixels, 13, 7, 32, 16),
                         loading.resize(pixels, 13, 7, 32, 16, accelerated=False))

    @unittest.skipUnless((ROOT / "Halo 2 Vista installer/loading.bin").is_file(), "owned loading bank unavailable")
    def test_all_twelve_payloads_match_actual_original_pool_readbacks(self):
        bank = (ROOT / "Halo 2 Vista installer/loading.bin").read_bytes()
        reference = ROOT / "analysis/frontend/vista-session-flow-audit/native-loading-presentation-original/d3dx-readback-20261011"
        for index in range(12):
            pixels, _, _ = loading.decode(bank, index)
            self.assertEqual(loading.digest(pixels), loading.PAYLOAD_SHA256[index])
            for pool in range(2):
                path = reference / f"texture-{index:02d}-pool{pool}.bgra"
                if path.is_file():
                    self.assertEqual(pixels, path.read_bytes())
        # Exercise the dependency-free path on an actual resized RGBA image.
        pixels, _, _ = loading.decode(bank, 7, accelerated=False)
        self.assertEqual(loading.digest(pixels), loading.PAYLOAD_SHA256[7])

    @unittest.skipUnless((ROOT / "Halo 2 Vista installer/halo2.exe").is_file(), "owned Vista executable unavailable")
    def test_complete_owned_export_contains_every_runtime_dependency(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            report = loading.prepare(ROOT / "Halo 2 Vista installer", output, lambda _: None)
            rows = (output / "loading-textures.tsv").read_text().splitlines()
            self.assertEqual(rows[0], "H2V_LOADING_TEXTURES_V1")
            self.assertEqual(len(rows), 13)
            for line in rows[1:]:
                index, sw, sh, w, h, fmt, name = line.split("\t")
                data = (output / name).read_bytes()
                self.assertEqual(len(data), 128 + int(w) * int(h) * 4)
                self.assertEqual(loading.digest(data[128:]), loading.PAYLOAD_SHA256[int(index)])
            self.assertEqual(loading.digest((output / "loading-tone.xbox-adpcm").read_bytes()), loading.BANK_SHA)
            self.assertEqual(json.loads((output / "source.json").read_text()), report)


if __name__ == "__main__":
    unittest.main()
