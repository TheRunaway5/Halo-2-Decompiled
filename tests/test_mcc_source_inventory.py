"""Copy real tiny donor inventories without losing source-change protection.

No installed MCC files are needed. These controls exercise the same bounded
enumeration, saved stat rows and verified-copy seam used by first-run import.
"""
import hashlib
from contextlib import contextmanager
import os
from pathlib import Path
import sys
import tempfile
import threading
import shutil
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from audit_rendering import FormatError, donor_inventory
import prepare_mcc_gameplay as preparation


class MutatingReader:
    def __init__(self, stream, mutation):
        self.stream, self.mutation, self.changed = stream, mutation, False

    def __getattr__(self, name):
        return getattr(self.stream, name)

    def read(self, *arguments):
        value = self.stream.read(*arguments)
        if not self.changed:
            self.changed = True
            self.mutation()
        return value

    def __enter__(self):
        return self

    def __exit__(self, *arguments):
        return self.stream.__exit__(*arguments)


class MccSourceInventoryTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(
            prefix='halo2-source-inventory-', dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()
        self.source = self.root / 'MCC café-日本 donor' / 'halo2'
        self.source.mkdir(parents=True)
        self.target = self.root / 'private copy'
        self.messages = []

    def write(self, relative, data):
        path = self.source / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        return path

    def copy(self, files=None):
        if files is None:
            files, omitted = preparation.source_files(self.source)
            self.assertEqual(omitted, [])
        return preparation.copy_verified(self.source, self.target, files,
                                         self.messages.append)

    def test_unchanged_game_cfg_copies_from_saved_real_inventory(self):
        data = b'[settings]\r\nplayer=fixture\r\n'
        self.write('game.cfg', data)
        files, omitted = preparation.source_files(self.source)
        self.assertEqual(omitted, [])
        records = self.copy(files)
        self.assertEqual((self.target / 'game.cfg').read_bytes(), data)
        self.assertEqual(records, {'game.cfg': {
            'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}})

    def test_enumerated_regular_file_identity_matches_fresh_nofollow_stat(self):
        self.write('game.cfg', b'settings')
        self.write('data/My Mod/日本 café.bin', b'owned fixture')
        checked = 0
        for relative, saved, directory in donor_inventory(self.source):
            if directory:
                continue
            current = (self.source / relative).lstat()
            self.assertNotEqual(current.st_ino, 0, 'Fixture filesystem has no file identity')
            self.assertEqual((saved.st_dev, saved.st_ino, saved.st_size, saved.st_mtime_ns),
                             (current.st_dev, current.st_ino, current.st_size, current.st_mtime_ns),
                             relative.as_posix())
            checked += 1
        self.assertEqual(checked, 2)

    def test_unicode_nested_assets_keep_authored_names_bytes_and_hashes(self):
        expected = {'game.cfg': b'settings',
                    'h2_maps_win64_dx11/mainmenu.map': b'small synthetic map bytes',
                    'data/My Mod/日本 café.bin': bytes(range(32))}
        for relative, data in expected.items():
            self.write(relative, data)
        empty = Path('tags/My Mod/Empty 日本')
        (self.source / empty).mkdir(parents=True)
        records = self.copy()
        self.assertEqual(set(records), set(expected))
        self.assertTrue((self.target / empty).is_dir())
        for relative, data in expected.items():
            self.assertEqual((self.target / relative).read_bytes(), data)
            self.assertEqual((self.source / relative).read_bytes(), data)
            self.assertEqual(records[relative], {
                'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()})

    def test_same_bytes_restored_mtime_replacement_after_inventory_is_rejected(self):
        data = b'unchanged bytes must not hide a replaced source owner'
        path = self.write('game.cfg', data)
        files, _ = preparation.source_files(self.source)
        original = path.lstat()
        replacement = self.root / 'replacement.cfg'
        replacement.write_bytes(data)
        os.utime(replacement, ns=(original.st_atime_ns, original.st_mtime_ns))
        os.replace(replacement, path)
        current = path.lstat()
        self.assertEqual((current.st_size, current.st_mtime_ns),
                         (original.st_size, original.st_mtime_ns))
        self.assertNotEqual(current.st_ino, original.st_ino)
        with self.assertRaisesRegex(FormatError, 'MCC source changed during import: game.cfg'):
            self.copy(files)
        self.assertFalse((self.target / 'game.cfg').exists())
        self.assertEqual(path.read_bytes(), data)

    def test_modification_while_copying_is_rejected(self):
        path = self.write('game.cfg', b'a' * 128)
        files, _ = preparation.source_files(self.source)
        original_open = preparation.DonorTree.open_file
        mutated = []

        def mutation():
            path.write_bytes(b'b' * 129)
            mutated.append(True)

        @contextmanager
        def opened(tree, relative, expected):
            with original_open(tree, relative, expected) as stream:
                yield MutatingReader(stream, mutation) if relative == Path('game.cfg') else stream

        with patch.object(preparation.DonorTree, 'open_file', opened):
            with self.assertRaisesRegex(FormatError, 'MCC source changed during copy: game.cfg'):
                self.copy(files)
        self.assertEqual(mutated, [True])
        self.assertEqual(path.read_bytes(), b'b' * 129)

    def test_symlink_donor_is_still_rejected(self):
        self.write('game.cfg', b'settings')
        try:
            (self.source / 'redirect').symlink_to(self.root, target_is_directory=True)
        except (OSError, NotImplementedError) as error:
            self.skipTest('Creating fixture symlinks is unavailable: ' + str(error))
        with self.assertRaisesRegex(FormatError, '[Ss]ymlink'):
            preparation.source_files(self.source)

    def test_parallel_copies_overlap_and_retain_exact_bytes_and_order(self):
        for name in ('a.bin', 'b.bin'):
            self.write(name, name.encode() * 4096)
        files, _ = preparation.source_files(self.source)
        original = preparation.DonorTree.open_file
        barrier = threading.Barrier(2)
        readers = set()

        @contextmanager
        def opened(tree, relative, expected):
            with original(tree, relative, expected) as stream:
                readers.add(threading.get_ident())
                barrier.wait(timeout=3)
                yield stream

        with patch.object(preparation.DonorTree, 'open_file', opened):
            records = preparation.copy_verified(self.source, self.target, files,
                                                 self.messages.append, workers=2)
        self.assertEqual(len(readers), 2)
        self.assertEqual(list(records), ['a.bin', 'b.bin'])
        for name, row in records.items():
            self.assertEqual((self.target/name).read_bytes(), (self.source/name).read_bytes())
            self.assertEqual(row['sha256'], hashlib.sha256((self.source/name).read_bytes()).hexdigest())
        updates = [value for value in self.messages if hasattr(value, 'percent')]
        self.assertEqual(updates[0].percent, 0)
        self.assertEqual(updates[-1].percent, 100)
        self.assertEqual([v.percent for v in updates], sorted(v.percent for v in updates))

    def test_parallel_readback_failure_cannot_report_completion(self):
        for name in ('a.bin', 'b.bin'):
            self.write(name, b'original source bytes' * 4096)
        files, _ = preparation.source_files(self.source)
        original = Path.open

        def opened(candidate, *args, **kwargs):
            if candidate == self.target/'a.bin' and args and args[0] == 'rb':
                candidate.write_bytes(b'bad destination bytes')
            return original(candidate, *args, **kwargs)

        with patch.object(Path, 'open', opened), \
                self.assertRaisesRegex(FormatError, 'copied asset hash differs'):
            preparation.copy_verified(self.source, self.target, files, self.messages.append, workers=2)
        self.assertFalse(any(getattr(value, 'percent', None) == 100 for value in self.messages))
        self.assertFalse(any(t.name.startswith('mcc-copy') for t in threading.enumerate()))

    def test_cancelled_parallel_copy_retires_writers_before_owner_cleanup(self):
        for name in ('a.bin', 'b.bin'):
            self.write(name, b'x' * (2 * 1024**2))
        files, _ = preparation.source_files(self.source)

        def stop(update):
            if getattr(update, 'percent', 0) > 0:
                raise SystemExit(143)

        with self.assertRaises(SystemExit):
            preparation.copy_verified(self.source, self.target, files, stop, workers=2)
        self.assertFalse(any(t.name.startswith('mcc-copy') for t in threading.enumerate()))
        shutil.rmtree(self.target)
        self.assertFalse(self.target.exists())

    def test_inventory_member_depth_and_byte_budgets_remain_enforced(self):
        self.write('game.cfg', b'a' * 32)
        (self.source / 'empty').mkdir()
        with self.assertRaisesRegex(FormatError, 'member budget'):
            donor_inventory(self.source, members=1)
        with self.assertRaisesRegex(FormatError, 'byte budget'):
            donor_inventory(self.source, byte_limit=31)
        (self.source / 'one/two').mkdir(parents=True)
        with self.assertRaisesRegex(FormatError, 'depth budget'):
            donor_inventory(self.source, depth=1)
        self.assertFalse(self.target.exists())


if __name__ == '__main__':
    unittest.main()
