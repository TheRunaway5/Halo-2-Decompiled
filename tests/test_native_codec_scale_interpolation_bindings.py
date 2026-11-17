import importlib.util
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def module(name):
    spec = importlib.util.spec_from_file_location(
        '_scale_interpolation_test_' + name, ROOT / 'tools' / f'{name}.py')
    value = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(value)
    return value


class ScaleInterpolationBindings(unittest.TestCase):
    def test_actual_specializer_authenticates_original_format(self):
        binding = module('native_codec_scale_interpolation_replacements')
        specializer = module('specialize_native_operands')
        raw = 'if(s.services.instruction){s.services.instruction(0x296620u);}'
        literal = 's.instruction_literal(0x296620u);'
        for name in binding.UNITS:
            source = (ROOT / name).read_text()
            self.assertEqual(binding.installed_entries(source), ['296620'])
            prepared, receipt = specializer.specialize(source)
            self.assertEqual(receipt['native_codec_scale_interpolation_entries'], ['296620'])
            self.assertEqual(binding.installed_entries(prepared), ['296620'])
            self.assertEqual(specializer.specialize(prepared)[0], prepared)
            self.assertEqual(source.count(raw), 1)
            self.assertEqual(prepared.count(literal), 1)
            for changed in (source.replace(binding.RAW_PREFIX, binding.PREPARED_PREFIX, 1),
                            source.replace(raw, literal, 1),
                            prepared.replace(binding.PREPARED_PREFIX, binding.RAW_PREFIX, 1),
                            prepared.replace(literal, raw, 1)):
                with self.assertRaisesRegex(ValueError, '[Ii]nterpolation'):
                    specializer.specialize(changed)

    def test_exact_tags_and_continuation_adjacency(self):
        binding = module('native_codec_scale_interpolation_replacements')
        for name in binding.UNITS:
            source = (ROOT / name).read_text()
            plain = binding.remove_codec_scale_interpolation(source)
            bound = binding.replace_codec_scale_interpolation(plain)
            self.assertEqual(binding.remove_codec_scale_interpolation(bound), plain)
            self.assertEqual(binding.replace_codec_scale_interpolation(bound), bound)
            self.assertEqual(binding.installed_entries(bound), ['296620'])
            self.assertEqual(bound.count(binding.INCLUDE + binding.ANCHOR), 1)
            # The exact anchor keeps earlier and trailing include relations.
            self.assertEqual(bound[:bound.index(binding.ANCHOR)].replace(binding.INCLUDE, ''),
                             plain[:plain.index(binding.ANCHOR)])
            body = binding.BODY.search(bound)[0]
            with self.assertRaises(ValueError):
                binding.installed_entries(bound.replace(binding.RAW_PREFIX, binding.PREPARED_PREFIX, 1))
            moved = binding.INCLUDE + bound.replace(binding.INCLUDE, '', 1)
            with self.assertRaisesRegex(ValueError, 'include placement drift'):
                binding.installed_entries(moved)
            for pc, label in binding.LABELS.items():
                changed = body.replace(label, '', 1)
                at = changed.index('{') + 1 + len(binding.RAW_PREFIX)
                changed = changed[:at] + '\n' + label + changed[at:]
                with self.assertRaises(ValueError, msg=hex(pc)):
                    binding.installed_entries(bound.replace(body, changed, 1))

    def test_preserves_translation_interpolation_anchor(self):
        binding = module('native_codec_scale_interpolation_replacements')
        translation = module('native_codec_translation_interpolation_replacements')
        source = (ROOT / binding.UNITS[0]).read_text()
        bound = binding.replace_codec_scale_interpolation(source)
        self.assertEqual(translation.installed_entries(bound), ['2962f0'])
        self.assertEqual(translation.replace_codec_translation_interpolation(
            translation.remove_codec_translation_interpolation(bound)), bound)

    def test_complete_live_state_wrapper_is_authenticated(self):
        binding = module('native_codec_scale_interpolation_replacements')
        marker = '\n};\nusing WorldState=LiveState;'
        override = '\nvoid instruction_literal(std::uint32_t){gp[6]^=1u;}'
        for name in binding.UNITS:
            source = (ROOT / name).read_text()
            plain = binding.remove_codec_scale_interpolation(source)
            bound = binding.replace_codec_scale_interpolation(plain)
            for text in (plain, bound):
                self.assertEqual(text.count(marker), 1)
                changed = text.replace(marker, override + marker, 1)
                with self.assertRaisesRegex(ValueError, 'body/resolver/dispatcher/class drift'):
                    binding.authenticate(changed)
                with self.assertRaises(ValueError):
                    binding.installed_entries(changed)

    def test_isolated_copied_pure_siblings(self):
        names = ('native_codec_scale_interpolation_replacements',
                 'native_model_marker_lookup_replacements', 'native_packed_rotation_replacements',
                 'native_packed_rotation_source_inputs', 'native_halfword_bit_query_replacements')
        parent = os.environ.get('HALO2_TEST_SCRATCH')
        with tempfile.TemporaryDirectory(prefix='scale-interpolation-bindings-', dir=parent) as directory:
            target = Path(directory)
            for name in names:
                shutil.copy2(ROOT / 'tools' / f'{name}.py', target / f'{name}.py')
            script = '''import importlib.util,json,sys
from pathlib import Path
path=Path(sys.argv[1]);before=set(sys.modules)
spec=importlib.util.spec_from_file_location('isolated_scale_interpolation',path)
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
for name in sys.argv[2:]:
    source=Path(name).read_text();plain=module.remove_codec_scale_interpolation(source)
    bound=module.replace_codec_scale_interpolation(plain)
    assert module.remove_codec_scale_interpolation(bound)==plain
    assert module.replace_codec_scale_interpolation(bound)==bound
    assert module.installed_entries(bound)==['296620']
assert not any(name.startswith(('native_','_codec_','_packed_rotation_'))for name in set(sys.modules)-before)
print(json.dumps({'passed':True,'owners':len(sys.argv)-2}))
'''
            binding = module('native_codec_scale_interpolation_replacements')
            command = [sys.executable, '-I', '-B', '-c', script,
                       str(target / (names[0] + '.py')),
                       *[str(ROOT / name) for name in binding.UNITS]]
            done = subprocess.run(command, cwd=target, capture_output=True, text=True, timeout=120)
            self.assertEqual(done.returncode, 0, done.stderr)
            self.assertEqual(json.loads(done.stdout), {'passed': True, 'owners': 1})


if __name__ == '__main__':
    unittest.main()
