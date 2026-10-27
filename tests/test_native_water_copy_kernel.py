"""The production water copy must execute the complete native combiner owner."""
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from generate_native_water_copy_kernel import literal_program, outputs, OUTPUT, MANIFEST, OWNER


class NativeWaterCopy(unittest.TestCase):
    def test_generated_source_and_all_program_words_are_current(self):
        for name, data in outputs().items():
            self.assertEqual((ROOT / name).read_bytes(), data, name)
        manifest = json.loads((ROOT / MANIFEST).read_bytes())
        actual, owner_hash = literal_program((ROOT / OWNER).read_text())
        self.assertEqual(len(actual), 60)
        self.assertEqual(actual, manifest['program_words'])
        self.assertEqual(owner_hash, manifest['source_owner_body_sha256'])
        self.assertFalse(manifest['runtime_instruction_interpreter'])
        wrapper = (ROOT / 'shaders/water_copy.frag').read_text()
        self.assertIn('#include "native-programs/world/water-copy-combiner.glsl"', wrapper)
        self.assertNotIn('#include "combiner.glsl"', wrapper)
        source = (ROOT / OUTPUT).read_text()
        for forbidden in ('word(', 'switch(', 'for(', 'while('):
            self.assertNotIn(forbidden, source)
        self.assertIn('runCombiners(samples,vec4(0),vec4(0))', wrapper)

    def test_nonliteral_or_ambiguous_source_owners_reject(self):
        text = (ROOT / OWNER).read_text()
        for changed in (text.replace('w[0]=0x18201920;', 'w[0]=0x18201920;w[0]=0;'),
                        text.replace('w[0]=0x18201920;', 'w[60]=0;'),
                        text.replace('w[0]=0x18201920;', 'w[0]=4294967296;'),
                        text.replace('w[0]=0x18201920;', 'w[0]=unproved();'),
                        text.replace('return out;', 'unproved(out);return out;', 1),
                        text + text):
            with self.subTest(changed=changed[:70]), self.assertRaises(ValueError):
                literal_program(changed)
        changed, _ = literal_program(text.replace('w[0]=0x18201920;', 'w[0]=0x18201921;'))
        self.assertNotEqual(changed, json.loads((ROOT / MANIFEST).read_bytes())['program_words'])

    def test_optimized_production_shader_reads_no_program_word(self):
        compiler, optimizer, validator = (shutil.which(x) for x in ('glslangValidator', 'spirv-opt', 'spirv-val'))
        if not all((compiler, optimizer, validator)):
            self.skipTest('Native Vulkan build tools unavailable')
        with tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH')) as folder:
            raw, output = Path(folder) / 'raw.spv', Path(folder) / 'native.spv'
            subprocess.run([compiler, '-V', '--target-env', 'vulkan1.3',
                            '-I' + str(ROOT / 'shaders'), str(ROOT / 'shaders/water_copy.frag'),
                            '-o', str(raw)], check=True, capture_output=True)
            subprocess.run([optimizer, '--target-env=vulkan1.3', '-O', str(raw), '-o', str(output)],
                           check=True, capture_output=True)
            subprocess.run([validator, '--target-env', 'vulkan1.3', str(output)], check=True, capture_output=True)
            data = output.read_bytes();words = struct.unpack('<' + 'I' * (len(data) // 4), data)
            ops, at = [], 5
            while at < len(words):
                count, opcode = words[at] >> 16, words[at] & 65535
                self.assertGreater(count, 0)
                ops.append((opcode, words[at + 1:at + count]));at += count
            constants, decorations = {}, {}
            for opcode, args in ops:
                if opcode == 43 and len(args) == 3:
                    constants[args[1]] = args[2]
                if opcode == 71 and len(args) >= 3:
                    decorations[(args[0], args[1])] = args[2:]
            material = [variable for (variable, kind), value in decorations.items()
                        if kind == 33 and value == (2,) and decorations.get((variable, 34)) == (0,)]
            self.assertEqual(len(material), 1, 'actual water Copy set0/binding2 owner missing')
            accesses, loads = {}, 0
            for opcode, args in ops:
                self.assertNotIn(opcode, (246, 251), 'native copy retained loop or switch dispatch')
                if opcode in (65, 66):
                    result, base = args[1:3]
                    if base == material[0]:
                        accesses[result] = tuple(constants.get(x) for x in args[3:])
                    elif base in accesses:
                        accesses[result] = accesses[base] + tuple(constants.get(x) for x in args[3:])
                if opcode == 61:
                    pointer = args[2]
                    self.assertNotEqual(pointer, material[0], 'copy loaded complete program-bearing uniform')
                    if pointer in accesses:
                        path = accesses[pointer]
                        self.assertTrue(path and path[0] in (1, 2),
                                        'optimized copy read a program word instead of ordinary rectangle/taps')
                        loads += 1
            self.assertGreater(loads, 0, 'shader did not retain its real source rectangle/tap inputs')


if __name__ == '__main__':
    unittest.main()
