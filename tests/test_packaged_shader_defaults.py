"""Pure relocation guards; ELF fixtures are synthetic data-owner containers."""
import hashlib
import os
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from packaged_shader_defaults import (native_runtime_wrapper, readonly_literal_location,
                                     relative_compiled_default, relocate_compiled_shader_default,
                                     shader_inventory)
from packaged_shader_test_support import elf_with_readonly_literal


class PackagedShaderDefaultTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="halo2-packaged-shader-default-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()
        self.directory = self.root / "actual-compiled-bank"
        self.directory.mkdir()

    def test_exact_readonly_literal_only_and_both_padding_lengths(self):
        forms = set()
        for length in range(4):
            directory = self.root / ("shader-bank" + "x" * length)
            directory.mkdir()
            original = os.fsencode(directory)
            before = elf_with_readonly_literal(original)
            after, receipt = relocate_compiled_shader_default(before, directory)
            at = receipt["offset"]
            self.assertEqual(before[:at], after[:at])
            self.assertEqual(before[at+len(original):], after[at+len(original):])
            self.assertEqual(len(before), len(after))
            self.assertEqual(before[0x80:0x82], after[0x80:0x82], "Executable bytes changed")
            self.assertEqual(receipt['input_sha256'], hashlib.sha256(before).hexdigest())
            self.assertEqual(receipt['output_sha256'], hashlib.sha256(after).hexdigest())
            self.assertEqual(readonly_literal_location(after, receipt['compiled_default'].encode())['section'], '.rodata')
            relative_compiled_default(receipt['compiled_default'])
            forms.add('bin//' in receipt['compiled_default'])
        self.assertEqual(forms, {True, False})

    def test_writable_executable_unallocated_or_duplicate_literal_rejects(self):
        original = os.fsencode(self.directory)
        for flags in (0, 3, 6, 7):
            with self.subTest(flags=flags), self.assertRaises(ValueError):
                relocate_compiled_shader_default(elf_with_readonly_literal(original, flags=flags), self.directory)
        with self.assertRaises(ValueError):
            relocate_compiled_shader_default(elf_with_readonly_literal(original, extra=original+b'\0'), self.directory)

    def test_local_owner_keeps_known_length_binary_neighbors_and_existing_bank(self):
        target = self.root / "local"
        target.mkdir()
        (self.directory / "preserved.spv").write_bytes(b"original bank")
        (target / "preserved.spv").write_bytes(b"same shader")
        original = os.fsencode(self.directory)
        before = elf_with_readonly_literal(original, terminated=False)
        after, receipt = relocate_compiled_shader_default(before, self.directory, target_directory=target)
        start, size = receipt["offset"], receipt["bytes"]
        self.assertEqual(before[:start], after[:start])
        self.assertEqual(before[start+size:], after[start+size:])
        self.assertEqual(len(before), len(after))
        self.assertEqual(Path(receipt["compiled_default"]).resolve(), target)
        self.assertEqual(receipt["directory"], str(target))
        self.assertEqual(len(receipt["compiled_default"].encode()), len(original))
        readonly_literal_location(after, receipt["compiled_default"].encode())
        self.assertEqual((self.directory / "preserved.spv").read_bytes(), b"original bank")
        self.assertEqual((target / "preserved.spv").read_bytes(), b"same shader")

    def test_local_owner_rejects_alias_missing_relative_or_oversized_path(self):
        target = self.root / "local"
        target.mkdir()
        alias = self.root / "alias"
        alias.symlink_to(target, target_is_directory=True)
        long = self.root / ("long" * 20)
        long.mkdir()
        payload = elf_with_readonly_literal(os.fsencode(self.directory))
        for path in (alias, self.root / "missing", Path("relative"), long):
            with self.subTest(path=path), self.assertRaises(ValueError):
                relocate_compiled_shader_default(payload, self.directory, target_directory=path)
        for flags in (0, 3, 6, 7):
            with self.subTest(flags=flags), self.assertRaises(ValueError):
                relocate_compiled_shader_default(elf_with_readonly_literal(os.fsencode(self.directory), flags=flags), self.directory, target_directory=target)

    def test_actual_known_length_constant_needs_no_terminator_and_keeps_binary_neighbor(self):
        original = os.fsencode(self.directory)
        before = elf_with_readonly_literal(original, terminated=False)
        after, receipt = relocate_compiled_shader_default(before, self.directory)
        end = receipt['offset'] + receipt['bytes']
        self.assertEqual(before[end:end+4], b'\x02\0\0\0')
        self.assertEqual(after[end:end+4], before[end:end+4])

    def test_invalid_truncated_or_wrong_literal_rejects_without_any_write(self):
        original = os.fsencode(self.directory)
        good = elf_with_readonly_literal(original)
        for data in (b'not ELF', good[:63], good[:-1], good.replace(b'\x7fELF', b'BAD!', 1),
                     elf_with_readonly_literal(original+b'-wrong')):
            with self.subTest(bytes=len(data)), self.assertRaises(ValueError):
                relocate_compiled_shader_default(data, self.directory)
        self.assertEqual(good, elf_with_readonly_literal(original))

    def test_relative_default_domain_cannot_escape_or_choose_other_banks(self):
        for value in ('/bin/shaders', '../bin/shaders', './bin/../shaders', 'other/shaders',
                      'bin/shaders/extra', 'bin///shaders', 'bin\\shaders', '././bin/shaders\0'):
            with self.subTest(value=value), self.assertRaises(ValueError):
                relative_compiled_default(value)

    def test_complete97_inventory_includes_nested_and_source_companions(self):
        names = ['world.vert.spv', 'world.frag.spv', 'hud.frag.spv', 'pstencil/volume.vert.spv',
                 'particle_program.hpp'] + [f'fixture-{i}.spv' for i in range(92)]
        for name in names:
            path = self.directory / name
            path.parent.mkdir(exist_ok=True)
            path.write_bytes(name.encode())
        inventory = shader_inventory(self.directory)
        self.assertEqual(len(inventory), 97)
        self.assertIn('pstencil/volume.vert.spv', inventory)
        self.assertIn('particle_program.hpp', inventory)
        (self.directory / 'unowned.spv').write_bytes(b'extra')
        with self.assertRaises(ValueError):
            shader_inventory(self.directory)
        (self.directory / 'unowned.spv').unlink()
        (self.directory / 'hud.frag.spv').unlink()
        (self.directory / 'hud.frag.spv').symlink_to(self.directory / 'world.frag.spv')
        with self.assertRaises(ValueError):
            shader_inventory(self.directory)

    def test_canonical_wrapper_sets_exact_project_cwd_before_payload(self):
        wrapper = native_runtime_wrapper()
        self.assertIn(b'cd "$root"\nexec "$root/lib/ld-linux-x86-64.so.2"', wrapper)
        self.assertTrue(wrapper.endswith(b'"$root/bin/halo2_native.real" "$@"\n'))
        self.assertNotIn(b'--mp-capture-shaders', wrapper)


if __name__ == '__main__':
    unittest.main()
