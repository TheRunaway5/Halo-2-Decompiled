import importlib.util
import json
from pathlib import Path
import unittest
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('bootstrap', ROOT/'tools/generate_mcc_native_hud_bootstrap.py')
gen = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gen)


class GeneratorTests(unittest.TestCase):
    def setUp(self):
        self.raw = (ROOT/'tools/data/mcc-native-hud-bootstrap.json').read_bytes()
    def test_frozen(self):
        self.assertEqual(gen.generate(self.raw), (ROOT/'src/mcc_native_hud_bootstrap.inc').read_text())
    def test_changed_domains(self):
        for mutate in (lambda d: d['maps'].append(d['maps'][0]),
                       lambda d: d['owners'][0].update(name='screen_alias'),
                       lambda d: d['records'][1]['references'][0][1].__setitem__(1, 'unknown'),
                       lambda d: d['excluded'].update(rasterizer=False)):
            d = json.loads(self.raw); mutate(d)
            with self.assertRaises(ValueError):
                gen.generate(json.dumps(d).encode())
    def test_malformed_fixups(self):
        for field, target in ((241, 10), (8, 156)):
            d = json.loads(self.raw); d['records'][2]['pointers'] = [[field, target]]
            with self.assertRaises(ValueError):
                gen.generate(json.dumps(d).encode())


if __name__ == '__main__':
    unittest.main()
