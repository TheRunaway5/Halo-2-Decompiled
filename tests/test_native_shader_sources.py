"""Build the preserved port shaders without a game installation or references."""
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class NativeShaderSourcesTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='halo2-shaders-only-')
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        shutil.copytree(ROOT / 'shaders/native-programs', self.root / 'sources')
        tools = self.root / 'tools'
        tools.mkdir()
        for name in ('compile_menu_world_vertex_catalog.py', 'generate_particle_graphics_program.py'):
            shutil.copyfile(ROOT / 'tools' / name, tools / name)

    def run_tool(self, tool, *args, passed=True):
        result = subprocess.run([sys.executable, str(self.root / 'tools' / tool), *map(str, args)],
                                cwd=self.root, capture_output=True, text=True)
        self.assertEqual(result.returncode == 0, passed, result.stdout + result.stderr)
        return result

    def menu(self, passed=True):
        compiler, validator = shutil.which('glslangValidator'), shutil.which('spirv-val')
        if not compiler or not validator:
            self.skipTest('native shader compiler/validator unavailable')
        return self.run_tool('compile_menu_world_vertex_catalog.py',
             '--sources', self.root / 'sources/menu/sources.json', '--compiler', compiler, '--validator', validator,
             '--header', self.root / 'build/catalog.hpp', '--manifest', self.root / 'build/compiled.json', passed=passed)

    def particle(self, classification, passed=True, variant='standard'):
        return self.run_tool('generate_particle_graphics_program.py',
             '--preserved-sources', self.root / 'sources/particles.json', '--classification', classification,
             '--variant', variant,
             '--output', self.root / f'build/particle-{classification}-{variant}.glsl', passed=passed)

    def test_all_existing_programs_compile_from_shipped_glsl_only(self):
        self.menu()
        report = json.loads((self.root / 'build/compiled.json').read_bytes())
        source = json.loads((self.root / 'sources/menu/sources.json').read_bytes())
        self.assertFalse(report['original_asset_files_read'])
        self.assertFalse(report['runtime_compilation'])
        self.assertEqual(len(report['entries']), len(source['entries']))
        self.assertEqual({row['class_index'] for row in report['entries']}, set(range(7)))
        for row, original in zip(report['entries'], source['entries']):
            self.assertEqual(row['program_sha256'], original['program_sha256'])
            self.assertEqual(row['source_sha256'], original['source_sha256'])
        self.assertNotIn('_program{{', (self.root / 'build/catalog.hpp').read_text())
        for entry in json.loads((self.root / 'sources/particles.json').read_bytes())['entries']:
            classification = entry['classification']
            variant = entry.get('variant', 'standard')
            self.particle(classification, variant=variant)
            self.assertEqual((self.root / f'build/particle-{classification}-{variant}.glsl').read_bytes(),
                             (self.root / 'sources' / entry['source']).read_bytes())
            self.assertIn(entry['program_fnv64'], (self.root / f'build/particle-{classification}-{variant}.hpp').read_text())
        self.assertFalse((self.root / 'sources/menu/compiled').exists())

    def test_distortion_has_a_distinct_identity_and_compiles_without_assets(self):
        self.particle(7, variant='distortion')
        self.particle(7)
        self.assertNotEqual((self.root / 'build/particle-7-distortion.glsl').read_bytes(),
                            (self.root / 'build/particle-7-standard.glsl').read_bytes())
        self.assertIn('particle_distortion_program_hash', (self.root / 'build/particle-7-distortion.hpp').read_text())
        compiler, validator = shutil.which('glslangValidator'), shutil.which('spirv-val')
        if not compiler or not validator:
            self.skipTest('native shader compiler/validator unavailable')
        shutil.copyfile(self.root / 'build/particle-7-distortion.glsl', self.root / 'build/particle_distortion_program.glsl')
        for name in ('particle.vert', 'particle_vertex_output.glsl'):
            shutil.copyfile(ROOT / 'shaders' / name, self.root / 'build' / name)
        output = self.root / 'build/distortion.spv'
        subprocess.run([compiler, '-V', '-DPARTICLE_DISTORTION=1', '-I' + str(self.root / 'build'),
                        str(self.root / 'build/particle.vert'), '-o', str(output)], check=True, capture_output=True)
        subprocess.run([validator, str(output)], check=True, capture_output=True)
        manifest = self.root / 'sources/particles.json'
        data = json.loads(manifest.read_bytes())
        data['entries'] = [e for e in data['entries'] if e.get('variant') != 'distortion']
        manifest.write_text(json.dumps(data))
        self.assertIn('missing or ambiguous', self.particle(7, variant='distortion', passed=False).stderr)

    def test_modified_glsl_is_rejected_before_compilation(self):
        menu = json.loads((self.root / 'sources/menu/sources.json').read_bytes())
        with (self.root / 'sources/menu' / menu['entries'][0]['source']).open('a') as stream:
            stream.write('\n// changed\n')
        self.assertIn('GLSL differs', self.menu(passed=False).stderr)
        particle = json.loads((self.root / 'sources/particles.json').read_bytes())['entries'][0]
        with (self.root / 'sources' / particle['source']).open('a') as stream:
            stream.write('\n// changed\n')
        self.assertIn('GLSL identity differs', self.particle(particle['classification'], passed=False).stderr)

    def test_source_paths_cannot_escape_the_shipped_directory(self):
        for path, run in ((self.root / 'sources/menu/sources.json', self.menu),
                          (self.root / 'sources/particles.json', lambda **kw: self.particle(7, **kw))):
            manifest = json.loads(path.read_bytes())
            manifest['entries'][0]['source'] = '../outside.glsl'
            path.write_text(json.dumps(manifest))
            self.assertIn('escapes', run(passed=False).stderr)


if __name__ == '__main__':
    unittest.main()
