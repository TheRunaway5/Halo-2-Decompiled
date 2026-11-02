import json
import os
from pathlib import Path
import struct
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import steam_workshop as workshop


class WorkshopTests(unittest.TestCase):
    def setUp(self):
        self.owner = tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(self.owner.cleanup)
        self.root = Path(self.owner.name).resolve()
        self.home = self.root / 'home'
        self.steam = self.home / '.local/share/Steam'
        self.library = self.root / 'External Steam Library'
        self.content = self.library / 'steamapps/workshop/content/976730'
        self.write(self.steam / 'steamapps/libraryfolders.vdf',
                   '"libraryfolders" { "0" { "path" "' + str(self.library) + '" } }')
        self.options = dict(home=self.home, environment={})

    def write(self, path, value):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(value if isinstance(value, bytes) else value.encode())

    def metadata(self, path, value):
        self.write(path, json.dumps(value))

    def mod(self, identifier='3360515088', engine='Halo2'):
        item = self.content / identifier
        self.metadata(item / 'ModInfo.json', dict(Engine=engine,
            ModIdentifier=dict(HostedModIds=dict(SteamWorkshopId=int(identifier))),
            Title=dict(Neutral='E3 2003'), InheritSharedFiles='FromMCC',
            GameModContents=dict(HasCampaign=True)))
        self.metadata(item / 'CampaignInfo.json', dict(CampaignMaps=['campaign/earthcity_e3.json']))
        self.metadata(item / 'campaign/earthcity_e3.json', dict(ScenarioFile='earthcity_e3',
                      Title=dict(Neutral='EARTH CITY'), Description=dict(Neutral='Clear the LZ.')))
        self.write(item / 'maps/earthcity_e3.map', struct.pack('<II', 0x68656164, 13) + bytes(888))
        return item

    def scan(self):
        with patch('steam_client._windows_libraries', return_value=[]):
            return workshop.scan(**self.options)

    def test_external_libraries_and_non_halo2_filter(self):
        item = self.mod()
        self.mod('1234', engine='Halo3')
        result = self.scan()
        self.assertEqual(len(result['mods']), 1)
        self.assertEqual(result['mods'][0]['directory'], str(item))
        self.assertEqual(result['mods'][0]['missions'][0]['scenario'], 'earthcity_e3')
        self.assertEqual(result['roots'], [str(self.content)])

    def test_override_and_steam_symlink_deduplicate(self):
        self.mod()
        (self.home / '.steam').mkdir(parents=True)
        (self.home / '.steam/steam').symlink_to(self.steam, target_is_directory=True)
        self.options['environment'] = dict(HALO2_WORKSHOP_ROOT=str(self.content))
        self.assertEqual(len(self.scan()['mods']), 1)

    def test_metadata_traversal_is_reported_without_hiding_other_mods(self):
        item = self.mod()
        self.metadata(item / 'CampaignInfo.json', dict(CampaignMaps=['../outside.json']))
        self.mod('1234')
        result = self.scan()
        self.assertEqual(len(result['mods']), 2)
        broken = next(row for row in result['mods'] if row['id'] == '3360515088')
        self.assertIn('relative content path', broken['error'])
        self.assertEqual(broken['missions'], [])

    def test_missing_and_wrong_engine_maps_remain_unlaunchable(self):
        item = self.mod()
        (item / 'maps/earthcity_e3.map').unlink()
        self.assertIn('absent', self.scan()['mods'][0]['error'])
        self.write(item / 'maps/earthcity_e3.map', bytes(896))
        self.assertIn('not an MCC Halo 2', self.scan()['mods'][0]['error'])

    def test_symlink_payload_cannot_escape_subscription(self):
        item = self.mod()
        payload = item / 'maps/earthcity_e3.map'
        outside = self.root / 'outside.map'
        payload.rename(outside)
        payload.symlink_to(outside)
        self.assertIn('symlink', self.scan()['mods'][0]['error'])

    def test_duplicate_json_keys_and_item_identity(self):
        item = self.mod()
        self.write(item / 'CampaignInfo.json', '{"CampaignMaps": [], "CampaignMaps": []}')
        self.assertIn('duplicate metadata key', self.scan()['mods'][0]['error'])
        info = json.loads((item / 'ModInfo.json').read_text())
        info['ModIdentifier']['HostedModIds']['SteamWorkshopId'] = 1
        self.metadata(item / 'ModInfo.json', info)
        result = self.scan()
        self.assertEqual(result['mods'], [])
        self.assertIn('identity differs', result['errors'][0])

    def test_bom_and_multiplayer_metadata(self):
        item = self.mod()
        info = json.loads((item / 'ModInfo.json').read_text())
        info['GameModContents']['HasCampaign'] = False
        self.write(item / 'ModInfo.json', b'\xef\xbb\xbf' + json.dumps(info).encode())
        self.metadata(item / 'multiplayer/earthcity_e3.json', dict(ScenarioFile='earthcity_e3'))
        mission = self.scan()['mods'][0]['missions'][0]
        self.assertFalse(mission['campaign'])

    def test_failed_preparation_cleans_its_stage_and_preserves_sources(self):
        item = self.mod()
        before = workshop.pin(item / 'maps/earthcity_e3.map')
        base = self.root / 'base'
        base.mkdir()
        output = self.root / 'prepared/e3'
        with self.assertRaises(Exception):
            workshop.prepare(item, 'earthcity_e3', base, output)
        self.assertFalse(output.exists())
        self.assertEqual(list(output.parent.glob('.workshop-*')), [])
        self.assertEqual(workshop.pin(item / 'maps/earthcity_e3.map'), before)

    def test_existing_prepared_content_is_never_replaced(self):
        item = self.mod()
        output = self.root / 'prepared'
        output.mkdir()
        self.write(output / 'keep.txt', 'existing content')
        with self.assertRaisesRegex(workshop.FormatError, 'output already exists'):
            workshop.prepare(item, 'earthcity_e3', self.root, output)
        self.assertEqual((output / 'keep.txt').read_text(), 'existing content')

    def test_declared_filename_can_retain_a_different_internal_scenario(self):
        item=self.mod()
        # Mod metadata owns the selected file. Low-spec cache variants may
        # legitimately retain the full-quality cache's internal scenario path.
        source=item/'maps/earthcity_e3.map'
        cache=SimpleNamespace(index=0x10,data=bytearray(source.read_bytes()),
            tags={1:dict(group='scnr',name='scenarios\\solo\\shared_internal',address=1,offset=0x100)},
            u32=lambda at:1,u16=lambda at:0,close=lambda:None)
        base=self.root/'base';base.mkdir()
        with patch('mcc_content.MccCache',return_value=cache):
            # Stop at the next independent ownership boundary. The real E3
            # low-spec case separately exercises complete preparation/reuse.
            with self.assertRaisesRegex(workshop.FormatError,'prepared MCC resource is absent: shared.map'):
                workshop.prepare(item,'earthcity_e3',base,self.root/'prepared/alias')

    def test_local_and_inherited_resource_banks_keep_distinct_owners(self):
        item = self.mod()
        info = json.loads((item / 'ModInfo.json').read_text())
        info['GameModContents']['HasSharedFiles'] = True
        self.metadata(item / 'ModInfo.json', info)
        nested = item / 'halo2/h2_maps_win64_dx11'
        for name in ('shared.map', 'single_player_shared.map', 'mainmenu.map', *workshop.RESOURCE_BANKS):
            self.write(nested / name, b'inherited ' + name.encode())
        self.write(item / 'maps/textures.dat', b'local textures')
        mod = workshop.read_mod(item)
        selected = workshop.resource_inputs(item, self.root / 'absent-base', mod)
        self.assertEqual(selected['maps']['shared.map'], nested / 'shared.map')
        self.assertEqual(selected['local']['textures.dat'], nested / 'textures.dat')
        self.assertEqual(selected['inherited']['textures.dat'], nested / 'textures.dat')
        self.assertEqual(selected['local']['sounds_en.dat'], nested / 'sounds_en.dat')
        # Embedded maps retain their own decoded owner when no local or
        # nested bank is shipped. They never borrow another mod's DAT files.
        (nested / 'textures.dat').unlink()
        selected = workshop.resource_inputs(item, self.root / 'absent-base', mod)
        self.assertEqual(selected['local']['textures.dat'], item / 'maps/textures.dat')
        self.assertEqual(selected['inherited']['textures.dat'], nested / 'shared.map')
        (item / 'maps/textures.dat').unlink()
        selected = workshop.resource_inputs(item, self.root / 'absent-base', mod)
        self.assertIsNone(selected['local']['textures.dat'])
        self.assertEqual(selected['inherited']['textures.dat'], nested / 'shared.map')

    def test_linked_resource_bank_is_rejected(self):
        item = self.mod()
        base = self.root / 'base/halo2/h2_maps_win64_dx11'
        for name in ('shared.map', 'single_player_shared.map', 'mainmenu.map'):
            self.write(base / name, b'base')
        outside = self.root / 'outside.dat'
        self.write(outside, b'outside')
        (item / 'maps/textures.dat').symlink_to(outside)
        with self.assertRaisesRegex(workshop.FormatError, 'symlink'):
            workshop.resource_inputs(item, base.parent.parent, workshop.read_mod(item))

    def test_malformed_nested_metadata_does_not_abort_discovery(self):
        item = self.mod()
        info = json.loads((item / 'ModInfo.json').read_text())
        info['ModIdentifier'] = []
        self.metadata(item / 'ModInfo.json', info)
        self.mod('1234')
        result = self.scan()
        self.assertEqual([row['id'] for row in result['mods']], ['1234'])
        self.assertIn('identity metadata', result['errors'][0])

    def test_windows_authored_directory_case_is_resolved_unambiguously(self):
        item = self.mod()
        (item / 'maps').rename(item / 'Maps')
        result = self.scan()['mods'][0]
        self.assertEqual(result['error'], '')
        self.assertEqual(result['missions'][0]['scenario'], 'earthcity_e3')

    def test_mixed_campaign_and_multiplayer_package_keeps_both(self):
        item = self.mod()
        info = json.loads((item / 'ModInfo.json').read_text())
        info['GameModContents']['MultiplayerMaps'] = ['multiplayer/zanzibar.json']
        self.metadata(item / 'ModInfo.json', info)
        self.metadata(item / 'multiplayer/zanzibar.json', dict(ScenarioFile='zanzibar', Title=dict(Neutral='Demo Multiplayer')))
        self.write(item / 'maps/zanzibar.map', struct.pack('<II', 0x68656164, 13) + bytes(888))
        result = self.scan()['mods'][0]
        self.assertEqual([(row['scenario'], row['campaign']) for row in result['missions']],
                         [('earthcity_e3', True), ('zanzibar', False)])


if __name__ == '__main__':
    unittest.main()
