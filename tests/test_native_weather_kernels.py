import hashlib
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from generate_native_weather_kernels import generated_sources


class NativeWeatherKernels(unittest.TestCase):
    def test_production_has_no_runtime_shader_instruction_bank(self):
        for name in ('point', 'line', 'plate'):
            path = ROOT / f'src/rendering/weather/source_weather_{name}_render.cpp'
            text = path.read_text()
            self.assertNotIn('execute_vertex_program(', text, str(path))
            self.assertNotIn('decode_vertex_program(', text, str(path))
            self.assertNotIn('VertexInstruction', text, str(path))
        for name in ('point.vert', 'line.vert', 'point.frag', 'line.frag', 'plate.frag'):
            text = (ROOT / f'shaders/source_weather_{name}').read_text()
            self.assertNotIn('instructions[', text, name)
            self.assertNotIn('program[', text, name)
            self.assertNotIn('"combiner.glsl"', text, name)
        text = (ROOT / 'shaders/source_weather_point_program.glsl').read_text()
        self.assertNotIn('"vertex_program.glsl"', text)
        text = (ROOT / 'shaders/source_shadow_filter.frag').read_text()
        self.assertNotIn('program[', text)
        self.assertNotIn('"combiner.glsl"', text)
        self.assertNotIn('"texture_program.glsl"', text)

    def test_checked_in_sources_are_the_exact_offline_lowering(self):
        for name, value in generated_sources().items():
            self.assertEqual((ROOT / 'shaders/native-programs/weather' / name).read_text(), value, name)

    def test_native_fragments_cannot_decode_program_controls(self):
        for name, value in generated_sources().items():
            if name.endswith('fragment.glsl'):
                self.assertNotIn('material.program', value)
                self.assertNotIn('word(', value)
                self.assertNotIn('operation', value)

    def test_complete_original_vertex_identities(self):
        metadata = json.loads(generated_sources()['programs.json'])
        self.assertEqual(metadata['vertex_programs']['point_vs22']['sha256'],
                         '3f9e4e6030626339db80d3bb5bf9e4103936041f9bdcfd75626f18ca2ff1a1ee')
        self.assertEqual(metadata['vertex_programs']['plate_vs21']['sha256'],
                         'c57b3f2509d664bba750f1b99fd09351efd0ddc4b9c5d2288e7fd2154896799a')
        self.assertEqual(len(metadata['plate_control_variants']), 16)
        self.assertEqual(len({tuple(v) for v in metadata['plate_control_variants']}), 16)


if __name__ == '__main__':
    unittest.main()
