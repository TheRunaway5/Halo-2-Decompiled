"""Preserve the actual chosen Vista match loader across MCC asset imports."""
from pathlib import Path
import shutil
import struct
import sys
import tempfile
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
from audit_rendering import FormatError
import preserve_vista_loading as loading
ROOT=Path(__file__).resolve().parents[1]
try:
    SOURCE=loading.selected(ROOT)
    loading.validate(SOURCE)
except (OSError,ValueError):
    SOURCE=None


class LoadingSelectionTests(unittest.TestCase):
    def test_missing_and_invalid_explicit_source_do_not_fall_back(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            with self.assertRaises(FormatError):loading.selected(root)
            (root/"assets").mkdir()
            (root/"assets/loading-source.tsv").write_text("incorrect\nvista\tother\n")
            with self.assertRaisesRegex(FormatError,"Invalid selected"):
                loading.selected(root)

    @unittest.skipUnless(SOURCE,"owned measured Vista bank unavailable")
    def test_mcc_import_owns_complete_original_bank_and_independent_selection(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);pack=root/"assets/imported/mcc-owned";pack.mkdir(parents=True)
            (root/"assets/asset-sources.tsv").write_bytes(b"existing game selection")
            report=loading.preserve(pack,SOURCE)
            self.assertTrue(report["complete"])
            self.assertEqual(len(report["files"]),14)
            self.assertEqual(loading.validate(pack/"loading"),loading.validate(SOURCE))
            loading.activate(root,pack/"loading")
            self.assertEqual(loading.selected(root),pack/"loading")
            self.assertEqual((root/"assets/asset-sources.tsv").read_bytes(),b"existing game selection")
            self.assertFalse(list(pack.glob(".loading-staging-*")))

    @unittest.skipUnless(SOURCE,"owned measured Vista bank unavailable")
    def test_changed_dds_masks_and_pixels_fail_before_selection(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);pack=root/"pack";pack.mkdir()
            loading.preserve(pack,SOURCE)
            path=pack/"loading/loading-00.dds";original=path.read_bytes()
            for offset in (92,128):
                data=bytearray(original);data[offset]^=1;path.write_bytes(data)
                with self.assertRaises(FormatError):loading.activate(root,pack/"loading")
                self.assertFalse((root/"assets/loading-source.tsv").exists())
                path.write_bytes(original)
            # Copying a mismatched old bank must not replace it in place.
            data=bytearray(original);data[128]^=1;path.write_bytes(data)
            with self.assertRaises(FormatError):loading.preserve(pack,SOURCE)
            self.assertEqual(path.read_bytes(),data)


if __name__=="__main__":unittest.main()
