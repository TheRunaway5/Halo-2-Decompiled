"""Small hostile fixtures exercise aggregate import limits and cleanup."""
import io
import shutil
import struct
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import disc
import setup_assets as assets
from archive_io import copy_exact, stream_command


class AssetLimitsTests(unittest.TestCase):
    def test_disc_aliased_extents_rejected_before_output(self):
        data = bytearray(36 * disc.SECTOR)
        data[65536:65556] = disc.MAGIC
        data[65536+disc.SECTOR-20:65536+disc.SECTOR] = disc.MAGIC
        struct.pack_into('<II', data, 65556, 33, 64)
        for offset, right, name in ((0, 8, b'a'), (32, 0, b'b')):
            at = 33 * disc.SECTOR + offset
            struct.pack_into('<HHIIBB', data, at, 0, right, 0, 40000, 0x20, 1)
            data[at+14:at+15] = name
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); source = root / 'disc.iso'; source.write_bytes(data)
            with disc.Disc(source) as reader, self.assertRaisesRegex(disc.FormatError, 'aggregate output'):
                reader.extract(root / 'output')
            self.assertFalse((root / 'output').exists())
            for limit in ('MAX_ENTRIES', 'MAX_PATH_BYTES'):
                with self.subTest(limit=limit), patch.object(disc, limit, 1):
                    with disc.Disc(source) as reader, self.assertRaisesRegex(disc.FormatError, 'budget exceeded'):
                        reader.extract(root / 'output')
                    self.assertFalse((root / 'output').exists())

    def test_zip_aggregate_and_count_fail_before_publication(self):
        for limit in ('VISTA_MAX_OUTPUT', 'VISTA_MAX_MEMBERS'):
            with self.subTest(limit=limit), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary); source = root / 'installer.zip'
                with zipfile.ZipFile(source, 'w') as archive:
                    archive.writestr('maps/mainmenu.map', b'abcd')
                    archive.writestr('maps/shared.map', b'efgh')
                with patch.object(assets, limit, 1), self.assertRaises(assets.FormatError):
                    assets.extract_vista(source, root / 'output')
                self.assertEqual({p.name for p in root.iterdir()}, {'installer.zip'})

    def test_zip_ratio_rejected_and_private_staging_removed(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); source = root / 'installer.zip'
            with zipfile.ZipFile(source, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
                archive.writestr('loading.bin', b'x' * 10000)
            with patch.object(assets, 'VISTA_MAX_RATIO', 2), self.assertRaises(assets.FormatError):
                assets.extract_vista(source, root / 'output', loading_only=True)
            self.assertEqual({p.name for p in root.iterdir()}, {'installer.zip'})

    def test_zip_inventory_is_bounded_before_zipfile_allocates_rows(self):
        with tempfile.TemporaryDirectory() as temporary:
            source = Path(temporary) / 'installer.zip'
            with zipfile.ZipFile(source, 'w') as archive:
                archive.writestr('loading.bin', b'a')
            raw = bytearray(source.read_bytes())
            at = raw.rfind(b'PK\x05\x06')
            struct.pack_into('<HH', raw, at + 8, 5000, 5000)
            source.write_bytes(raw)
            with self.assertRaisesRegex(assets.FormatError, 'inventory budget'):
                assets.extract_vista(source, Path(temporary) / 'output', loading_only=True)

    def test_actual_member_output_cannot_exceed_declaration(self):
        with self.assertRaisesRegex(ValueError, 'exceeds declared'):
            copy_exact(io.BytesIO(b'overflow'), io.BytesIO(), 2)

    def test_nested_bundles_cannot_repeat_extraction_budgets(self):
        raw = b'payload'
        for _ in range(3):
            stream = io.BytesIO()
            with zipfile.ZipFile(stream, 'w') as archive:
                archive.writestr('h2pc_data.bin', raw)
            raw = stream.getvalue()
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); source = root / 'installer.zip'; source.write_bytes(raw)
            with self.assertRaisesRegex(assets.FormatError, 'depth budget'):
                assets.extract_vista(source, root / 'output', progress=lambda _: None)
            self.assertEqual({p.name for p in root.iterdir()}, {'installer.zip'})

    def test_subprocess_stdout_and_stderr_are_bounded(self):
        for program in ('import sys; sys.stdout.buffer.write(b"x"*2000000)',
                        'import sys; sys.stderr.buffer.write(b"x"*2000000)'):
            with self.assertRaisesRegex(ValueError, 'budget exceeded'):
                stream_command([sys.executable, '-c', program], lambda chunk: None, output_limit=100)

    @unittest.skipUnless(shutil.which('7z') or shutil.which('7zz'), '7-Zip unavailable')
    def test_7zip_aggregate_preflight_and_cleanup(self):
        import subprocess
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); source = root / 'source'; source.mkdir()
            (source / 'loading.bin').write_bytes(b'four')
            archive = root / 'installer.7z'
            subprocess.run([shutil.which('7z') or shutil.which('7zz'), 'a', str(archive), 'loading.bin'],
                           cwd=source, check=True, stdout=subprocess.DEVNULL)
            with patch.object(assets, 'VISTA_MAX_OUTPUT', 1), self.assertRaises(assets.FormatError):
                assets.extract_vista(archive, root / 'output', loading_only=True)
            self.assertFalse((root / 'output').exists())
            self.assertFalse(list(root.glob('halo2-vista-*')))
