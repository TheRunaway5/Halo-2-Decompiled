"""Direct checks for the requested glare, letter spacing and trigger sizing."""
import json
from pathlib import Path
import re
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1] / 'resources/presentation'


def master(bank, selector, code):
    report = json.loads((bank / 'reconstruction.json').read_text())
    row = next(r for r in report['records'] if r['selector'] == selector and r['codepoint'] == code)
    return ET.parse(bank / row['vector']).getroot()


class Artwork(unittest.TestCase):
    def test_only_playstation_colored_side_glare_removed(self):
        for family in ('xbox', 'playstation'):
            bank = ROOT / ('prompts-' + family)
            if not bank.exists() and family == 'xbox':
                bank = ROOT / 'prompts'
            for selector in range(12):
                for code in range(0xe100, 0xe104):
                    root = master(bank, selector, code)
                    self.assertTrue(any(n.get('id') == 'top-sheen' for n in root.iter()),
                                    f'{family} selector {selector} lost its white top highlight')
                    if family == 'playstation':
                        self.assertFalse(any(n.get('id') == 'left-reflection' for n in root.iter()),
                                         f'PlayStation selector {selector} still has colored side glare')

    def test_xbox_rb_spacing(self):
        bank = ROOT / 'prompts-xbox'
        if not bank.exists():
            bank = ROOT / 'prompts'
        for selector in range(1, 12):
            lb, rb = master(bank, selector, 0xe104), master(bank, selector, 0xe105)
            if not any(n.get('id') == 'letter-B' for n in lb.iter()):
                continue # Compact legacy shoulder spheres have no LB/RB letters.
            def offset(root):
                b = next(n for n in root.iter() if n.get('id') == 'letter-B')
                return float(re.search(r'translate\(([-.0-9]+)', b.get('transform'))[1])
            self.assertAlmostEqual(offset(lb) - 6, offset(rb) - 7.7, places=4,
                                   msg='RB has less interletter space than LB')

    def test_trigger_labels_legible(self):
        for code in (0xe106, 0xe107):
            root = master(ROOT / 'prompts-playstation', 6, code)
            for name in ('l2', 'r2'):
                label = next(n for n in root.iter() if n.get('id') == 'label-' + name)
                size = float(re.search(r'scale\(([-.0-9]+)', label.get('transform'))[1])
                self.assertGreaterEqual(size, 1, 'Trigger labels are smaller than the shoulder labels')


if __name__ == '__main__':
    unittest.main()
