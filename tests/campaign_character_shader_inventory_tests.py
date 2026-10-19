"""Original opening override identity, independent of ordinary model materials."""
from pathlib import Path
import sys, unittest

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from audit_campaign_character_shaders import inventory

class CharacterOverrideInventory(unittest.TestCase):
    def test_shared_render_model_keeps_regret_hologram_override(self):
        report=inventory(ROOT/'assets/disc/maps/00a_introduction.map')
        self.assertEqual(report['cache_sha256'],
            '5093e2f8ef7f0df41c11c6621bac5785a36f78a1cff3b8b2bc6c14b561c02a28')
        units={u['datum']:u for u in report['units']}
        regret,mercy=units[0xe86f06fa],units[0xe66304ee]
        self.assertEqual(regret['model'],mercy['model'])
        # Literal untouched hlmt E87006FB +F4. It is not a material in the
        # shared MODE E66504F0 bank, so de-duplicating MODE must not omit it.
        self.assertEqual(regret.get('model_definition'),0xe87006fb)
        self.assertEqual(regret.get('shader_override'),0xe87106fc,
            'Regret hologram override is absent from the character shader audit')
        self.assertNotIn(0xe87106fc,report['models'][regret['model']]['materials'])
        self.assertIn('e87106fc',report['shaders'])
        shader=report['shaders']['e87106fc']
        template=report['templates'][shader['postprocess'][0]['template']]
        self.assertEqual(template['postprocess'][0]['lods'][0]['available_layers'],1<<21)

if __name__=='__main__':unittest.main()
