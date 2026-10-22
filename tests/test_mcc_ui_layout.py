"""MCC-native UI conversion and resumable cache upgrade ownership."""
import hashlib
import json
from pathlib import Path
import shutil
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import mcc_ui_layout as ui
from audit_rendering import FormatError


def layout_fixture(root):
    target = Path(root)/ui.SOURCE
    target.parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(Path(__file__).resolve().parents[1]/ui.SOURCE,target)


class NativeUiLayoutTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory(prefix='halo2-ui-layout-')
        self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name)/'port';self.cache=self.root/'assets/imported/mcc-test'
        self.cache.mkdir(parents=True);layout_fixture(self.root)
        self.menu=self.cache/'halo2/h2_maps_win64_dx11/mainmenu.map'
        self.menu.parent.mkdir(parents=True);self.menu.write_bytes(b'daeh\x0d\0\0\0MCC fixture')
        self.provenance=self.cache/'provenance.json'
        self.provenance.write_text(json.dumps(dict(schema='H2_MCC_TU15_PREPARATION_V1',external_installation_requirement='Halo MCC only')))
        self.asset=self.cache/ui.TARGET
    def repair(self):return ui.repair(self.root,self.cache,progress=lambda _:None)
    def test_bundled_and_compiled_profiles_match(self):
        project = Path(__file__).resolve().parents[1]
        embedded = (project/'src/ui/frontend_mcc_logical_layout_data.inc').read_text().split('R"H2UI(',1)[1].split(')H2UI"',1)[0]
        self.assertEqual(json.loads(embedded),ui.source_profile(project)[1]['layout'])
    def test_fresh_stage_converts_logical_assets_and_responsive_regions(self):
        before=self.menu.read_bytes();entry=ui.prepare(self.root,self.cache,progress=lambda _:None)
        value=json.loads(self.asset.read_bytes());self.assertEqual(value['layout'],ui.source_profile(self.root)[1]['layout'])
        self.assertTrue(value['layout']['responsive']['friends_full_surface'])
        self.assertTrue(value['layout']['responsive']['clans_full_surface'])
        self.assertEqual(entry['source_mainmenu_sha256'],hashlib.sha256(before).hexdigest())
        self.assertEqual(self.menu.read_bytes(),before)
    def test_existing_cache_upgrade_is_idempotent(self):
        self.repair();before=[(p.stat().st_ino,p.stat().st_mtime_ns,p.read_bytes()) for p in (self.asset,self.provenance,self.menu)]
        self.repair();self.assertEqual(before,[(p.stat().st_ino,p.stat().st_mtime_ns,p.read_bytes()) for p in (self.asset,self.provenance,self.menu)])
    def test_import_converts_selector_anchors_and_preserves_mcc_bungie_page(self):
        self.repair();layout=json.loads(self.asset.read_bytes())['layout']
        original=layout['original']['screens'];raw=layout['raw']['screens']
        self.assertEqual(original['207']['panes'][0]['lists'][0]['position'],[-224,69])
        self.assertTrue(all(not p['lists'] for p in original['207']['panes'][1:]))
        self.assertEqual(raw['207']['panes'][1]['lists'][0]['position'],[0,0])
        self.assertTrue(all(p['lists'][0]['position']==[-224,69] for p in original['60']['panes']))
        self.assertTrue(all(p['bitmaps'][1]['frame']==4 for p in original['186']['panes']))
        policy=layout['responsive'];browser=policy['browser_converted']['panes'][0]
        self.assertGreaterEqual(240-.4*browser['texts'][3]['bounds'][0],60)
        self.assertLess(browser['lists'][0]['visible'],policy['browser_raw']['panes'][0]['lists'][0]['visible'])
        self.assertEqual(policy['bungie_news']['page'],4)
    def test_managed_profile_updates_and_pins_provenance(self):
        self.repair();source=self.root/ui.SOURCE;data=json.loads(source.read_text());data['layout']['responsive']['revision_note']='next bundled version';source.write_text(json.dumps(data))
        _,report=self.repair();self.assertEqual(report['native_ui_layout']['sha256'],hashlib.sha256(self.asset.read_bytes()).hexdigest())
        self.assertEqual(json.loads(self.asset.read_bytes())['layout'],data['layout'])
    def test_customized_cache_asset_is_preserved(self):
        self.repair();before=self.provenance.read_bytes();self.asset.write_bytes(b'custom mod layout')
        with self.assertRaises(FormatError):self.repair()
        self.assertEqual(self.asset.read_bytes(),b'custom mod layout');self.assertEqual(self.provenance.read_bytes(),before)
    def test_symlink_target_is_rejected(self):
        self.asset.parent.mkdir(parents=True);self.asset.symlink_to(self.root/ui.SOURCE)
        with self.assertRaises(FormatError):self.repair()
    def test_duplicate_source_keys_fail_before_writing_cache(self):
        (self.root/ui.SOURCE).write_text('{"schema":"one","schema":"two"}')
        with self.assertRaises(FormatError):self.repair()
        self.assertFalse(self.asset.exists())

if __name__=='__main__':unittest.main()
