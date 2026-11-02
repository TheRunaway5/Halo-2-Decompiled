"""Donor containment regressions; every fixture is owned temporary storage."""
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from audit_rendering import FormatError, donor_inventory
from donor_tree import DonorError, WindowsApi, admit
import prepare_mcc_gameplay as preparation


class DonorTreeSecurityTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name).resolve()
        self.source, self.outside, self.target = [self.root/name for name in ('donor', 'outside', 'target')]
        for path in (self.source/'nested', self.outside, self.target):
            path.mkdir(parents=True)
        (self.source/'nested/asset.bin').write_bytes(b'admitted')
        (self.outside/'secret.bin').write_bytes(b'outside secret')

    @unittest.skipIf(os.name == 'nt', 'POSIX directory replacement fixture')
    def test_directory_swap_cannot_redirect_inventory(self):
        original = os.scandir
        nested = self.source/'nested'
        inode = nested.stat().st_ino
        swapped = False
        def race(parent):
            nonlocal swapped
            selected = os.fstat(parent).st_ino == inode if isinstance(parent, int) else Path(parent) == nested
            if selected and not swapped:
                swapped = True
                nested.rename(self.source/'original')
                nested.symlink_to(self.outside, target_is_directory=True)
            return original(parent)
        with patch('os.scandir', side_effect=race):
            try:
                rows = donor_inventory(self.source)
            except FormatError:
                return
        self.assertTrue(swapped)
        self.assertFalse(any(path.name == 'secret.bin' for path, _, _ in rows))

    def test_file_swap_is_rejected_before_outside_bytes_are_written(self):
        files, _ = preparation.source_files(self.source)
        leaf = self.source/'nested/asset.bin'
        path_open, os_open, windows_open = Path.open, os.open, WindowsApi.open
        swapped = False
        def swap():
            nonlocal swapped
            if not swapped:
                swapped = True
                leaf.rename(self.source/'original.bin')
                leaf.write_bytes(b'outside secret')
        def path_race(path, mode='r', *args, **kwargs):
            if path == leaf and mode == 'rb':
                swap()
            return path_open(path, mode, *args, **kwargs)
        def fd_race(path, flags, *args, **kwargs):
            if (str(path) == str(leaf) or (str(path) == leaf.name and kwargs.get('dir_fd') is not None)):
                swap()
            return os_open(path, flags, *args, **kwargs)
        def windows_race(api, path, **kwargs):
            if Path(path) == leaf:
                swap()
            return windows_open(api, path, **kwargs)
        with patch.object(Path, 'open', path_race), patch('os.open', side_effect=fd_race), \
             patch.object(WindowsApi, 'open', windows_race):
            try:
                preparation.copy_verified(self.source, self.target, files, lambda _: None, [Path('nested')])
            except FormatError:
                pass
        output = self.target/'nested/asset.bin'
        self.assertTrue(swapped)
        self.assertTrue(not output.exists() or output.read_bytes() == b'admitted')

    @unittest.skipIf(os.name == 'nt', 'POSIX permission bits')
    def test_copied_files_are_private_even_with_permissive_umask(self):
        files, _ = preparation.source_files(self.source)
        previous = os.umask(0)
        try:
            preparation.copy_verified(self.source, self.target, files, lambda _: None, [Path('nested')])
        finally:
            os.umask(previous)
        self.assertEqual(stat.S_IMODE((self.target/'nested/asset.bin').stat().st_mode), 0o600)

    def test_directory_reparse_metadata_is_never_admitted(self):
        from types import SimpleNamespace
        with self.assertRaisesRegex(DonorError, 'reparse'):
            admit(SimpleNamespace(st_mode=stat.S_IFDIR | 0o700, st_file_attributes=0x410))

    @unittest.skipUnless(os.name == 'nt', 'Windows directory junction fixture')
    def test_wine_host_mount_allows_normal_source_inventory(self):
        from donor_tree import windows, DonorTree
        api = windows()
        ntdll = api.kernel.GetModuleHandleW('ntdll.dll')
        if not ntdll or not api.kernel.GetProcAddress(ntdll, b'wine_get_version'):
            self.skipTest('Wine-specific host filesystem mount fixture')
        path = Path(__file__).absolute().anchor
        handle = api.open(path, directory=True)
        try:
            if not api.wine_host_mount(handle):
                self.skipTest('This test drive has no synthetic host mount metadata')
        finally:
            api.kernel.CloseHandle(handle)
        with DonorTree(self.source) as tree:
            self.assertTrue(tree.inventory())

    @unittest.skipUnless(os.name == 'nt', 'Windows directory junction fixture')
    def test_windows_junction_is_rejected_before_inventory_or_copy(self):
        (self.source/'nested/asset.bin').unlink()
        (self.source/'nested').rmdir()
        result = subprocess.run(['cmd', '/c', 'mklink', '/J', str(self.source/'nested'), str(self.outside)],
                                capture_output=True, text=True)
        if result.returncode:
            self.skipTest('Junction creation is unavailable: '+result.stderr)
        with self.assertRaisesRegex(FormatError, 'reparse'):
            donor_inventory(self.source)
        self.assertEqual(list(self.target.iterdir()), [])

    def test_vista_directory_import_uses_the_same_admitted_file_owner(self):
        from contextlib import contextmanager
        from donor_tree import DonorTree
        import setup_assets
        maps = self.source/'maps'
        (maps/'fonts').mkdir(parents=True)
        (maps/'mainmenu.map').write_bytes(b'original menu')
        (maps/'shared.map').write_bytes(b'original shared')
        original = DonorTree.open_file
        @contextmanager
        def swapped(tree, relative, expected):
            if relative == Path('maps/mainmenu.map'):
                replacement = self.root/'replacement'
                replacement.write_bytes(b'outside secret')
                os.replace(replacement, maps/'mainmenu.map')
            with original(tree, relative, expected) as stream:
                yield stream
        target = self.root/'vista-import'
        with patch.object(DonorTree, 'open_file', swapped):
            with self.assertRaises(FormatError):
                setup_assets.extract_vista(self.source, target)
        self.assertFalse(target.exists())
