"""Exact grouped codec row-copy overlays and isolated pure-helper packaging."""
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
    spec = importlib.util.spec_from_file_location('_codec_copy_test_'+name, ROOT/'tools'/f'{name}.py')
    value = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(value)
    return value


class CodecCopyBindings(unittest.TestCase):
    def test_installed_group_and_exact_prepared_retag(self):
        binding = module('native_codec_copy_replacements')
        specializer = module('specialize_native_operands')
        for name, group in zip(binding.UNITS,binding.ENTRY_GROUPS,strict=True):
            source = (ROOT/name).read_text()
            expected = [f'{entry:x}' for entry in group]
            self.assertEqual(binding.installed_entries(source),expected)
            prepared,receipt = specializer.specialize(source)
            self.assertEqual(receipt['native_codec_copy_entries'],expected)
            self.assertEqual(binding.installed_entries(prepared),expected)
            self.assertEqual(specializer.specialize(prepared)[0],prepared)
            self.assertEqual(binding.remove_codec_copy(prepared),
                             specializer.specialize(binding.remove_codec_copy(source))[0])
            for entry in group:
                raw = binding._callback(entry,False)
                literal = binding._callback(entry,True)
                for original,changed in (
                    (source,source.replace(binding.PREFIXES[entry][False],binding.PREFIXES[entry][True],1)),
                    (prepared,prepared.replace(binding.PREFIXES[entry][True],binding.PREFIXES[entry][False],1)),
                    (source,source.replace(raw,literal,1)),
                    (prepared,prepared.replace(literal,raw,1)),
                ):
                    self.assertNotEqual(changed,original)
                    with self.assertRaises(ValueError):
                        specializer.specialize(changed)

    def test_exact_group_reversal_labels_and_header_position(self):
        binding = module('native_codec_copy_replacements')
        for name,group in zip(binding.UNITS,binding.ENTRY_GROUPS,strict=True):
            source = (ROOT/name).read_text()
            plain = binding.remove_codec_copy(source)
            bound = binding.replace_codec_copy(plain)
            self.assertEqual(binding.remove_codec_copy(bound),plain)
            self.assertEqual(binding.replace_codec_copy(bound),bound)
            self.assertEqual(bound.count(binding.ANCHOR+binding.INCLUDE),1)
            for entry in group:
                body = binding.BODY[entry].search(bound)[0]
                for pc,label in binding.LABELS[entry].items():
                    changed = body.replace(label,'',1)
                    at = changed.index('{')+1+len(binding.PREFIXES[entry][False])
                    changed = changed[:at]+'\n'+label+changed[at:]
                    with self.assertRaises(ValueError,msg=hex(pc)):
                        binding.installed_entries(bound.replace(body,changed,1))
            moved = binding.INCLUDE+bound.replace(binding.INCLUDE,'',1)
            self.assertNotEqual(moved,bound)
            with self.assertRaises(ValueError):
                binding.installed_entries(moved)

    def test_outside_owner_bridge_and_continuation_refuse_before_install(self):
        binding = module('native_codec_copy_replacements')
        specializer = module('specialize_native_operands')
        rogue = ('static void rogue_copy_owner(WorldState&s){'
                 '(void)halo2::native_codec_copy_owner_detail::bridge<'
                 'halo2::native_codec_copy_owner_detail::Entry::quaternion,false>(s);}\n')
        for name in binding.UNITS:
            source = (ROOT/name).read_text()
            plain = binding.remove_codec_copy(source)
            bound = binding.replace_codec_copy(plain)
            for text in (plain,bound):
                anchor = binding.BODY[binding.QUATERNION].search(text).start()
                for addition in (rogue,'static void rogue_copy_label(){native_codec_copy_resume_dead:;}\n'):
                    changed = text[:anchor]+addition+text[anchor:]
                    for action in (binding.authenticate,binding.replace_codec_copy,binding.installed_entries,
                                   specializer.specialize):
                        with self.assertRaises(ValueError):
                            action(changed)

    def test_complete_state_wrapper_and_mixed_group_refuse(self):
        binding = module('native_codec_copy_replacements')
        marker = '\n};\nusing WorldState=LiveState;'
        override = '\nvoid instruction_literal(std::uint32_t){gp[6]^=1u;}'
        for name,group in zip(binding.UNITS,binding.ENTRY_GROUPS,strict=True):
            source = (ROOT/name).read_text()
            plain = binding.remove_codec_copy(source)
            bound = binding.replace_codec_copy(plain)
            for text in (plain,bound):
                self.assertEqual(text.count(marker),1)
                with self.assertRaises(ValueError):
                    binding.authenticate(text.replace(marker,override+marker,1))
            entry = group[-1]
            with self.assertRaises(ValueError):
                binding.installed_entries(bound.replace(binding.PREFIXES[entry][False],
                                                         binding.PREFIXES[entry][True],1))

    def test_pure_helper_loads_in_isolation_without_repository_imports(self):
        binding = module('native_codec_copy_replacements')
        parent = os.environ.get('HALO2_TEST_SCRATCH')
        with tempfile.TemporaryDirectory(prefix='codec-copy-bindings-',dir=parent) as folder:
            target = Path(folder)
            shutil.copy2(ROOT/'tools/native_codec_copy_replacements.py',target/'binding.py')
            script = '''import importlib.util,json,sys
from pathlib import Path
before=set(sys.modules)
spec=importlib.util.spec_from_file_location('isolated_codec_copy',sys.argv[1])
helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
for name,group in zip(sys.argv[2:],helper.ENTRY_GROUPS,strict=True):
    text=Path(name).read_text();plain=helper.remove_codec_copy(text)
    bound=helper.replace_codec_copy(plain)
    assert helper.remove_codec_copy(bound)==plain
    assert helper.replace_codec_copy(bound)==bound
    assert helper.installed_entries(bound)==[f'{entry:x}'for entry in group]
assert not any(name.startswith(('native_','_codec_'))for name in set(sys.modules)-before)
print(json.dumps({'passed':True,'owners':len(sys.argv)-2}))
'''
            done = subprocess.run([sys.executable,'-I','-B','-c',script,str(target/'binding.py'),
                                   *[str(ROOT/name) for name in binding.UNITS]],
                                  cwd=target,capture_output=True,text=True,timeout=120)
            self.assertEqual(done.returncode,0,done.stderr)
            self.assertEqual(json.loads(done.stdout),{'passed':True,'owners':2})


if __name__=='__main__':
    unittest.main()
