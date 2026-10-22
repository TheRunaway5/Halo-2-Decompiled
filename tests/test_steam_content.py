"""Selection boundaries against observed Valve metadata and importer owners."""
import copy
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import steam_content as steam


class SteamContentTests(unittest.TestCase):
    def setUp(self):
        self.info = json.loads((ROOT / 'tests/macos/steam/public-appinfo-20261020.json').read_text())
        self.depots = steam.depot_plan(self.info)

    def manifests(self):
        rules = steam.profile()
        rows = {p['depot_id']: dict(depot_id=p['depot_id'], manifest_id=p['manifest_id'], files=[])
                for p in self.depots['depots']}
        for name in rules['required_files']:
            depot = 976731 if name in rules['shared_files'] else 976735
            rows[depot]['files'].append(dict(name=name, bytes=23, compressed_bytes=17, sha1='ab' * 20))
        return list(rows.values())

    def test_live_valve_relationships_select_shared_and_halo2_only(self):
        self.assertEqual([p['depot_id'] for p in self.depots['depots']], [976731, 976735, 976736, 976737])
        self.assertEqual(self.depots['halo2_dlc_id'], 1064270)
        self.assertEqual(self.depots['halo2_multiplayer_dlc_id'], 1097223)
        self.assertEqual(self.depots['platform'], 'windows')
        self.assertFalse(self.depots['entitlement_verified'])
        self.assertEqual(self.depots['depots'][1]['manifest_id'], '3171334642139802443')

    def test_changed_branch_manifest_is_bound_from_live_metadata(self):
        self.info['apps']['976730']['depots']['976735']['manifests']['public']['gid'] = '18446744073709551614'
        plan = steam.depot_plan(self.info)
        self.assertEqual(plan['depots'][1]['manifest_id'], '18446744073709551614')
        self.assertFalse(plan['entitlement_verified'])

    def test_changed_depot_owner_or_new_required_depot_fails_closed(self):
        depots = self.info['apps']['976730']['depots']
        original = copy.deepcopy(depots['976735'])
        for change in ['wrong-owner', 'missing-manifest', 'new-depot']:
            with self.subTest(change=change):
                changed = copy.deepcopy(self.info)
                target = changed['apps']['976730']['depots']
                if change == 'wrong-owner': target['976735']['dlcappid'] = '1064271'
                if change == 'missing-manifest': del target['976735']['manifests']
                if change == 'new-depot': target['976799'] = original
                with self.assertRaises(steam.SteamContentError): steam.depot_plan(changed)

    def test_unsafe_and_ambiguous_paths_are_rejected(self):
        for name in ['/halo2/file', '../halo2/file', 'halo2/../file', 'halo2//file',
                     r'C:\halo2\file', 'halo2/file:stream', 'halo2/file.', 'halo2/\x00file', 'halo2/./file']:
            with self.subTest(name=name), self.assertRaises(steam.SteamContentError):
                steam.selected_file(name)
        self.assertTrue(steam.selected_file(r'halo2\h2_maps_win64_dx11\lockout.map'))

    def test_shared_pak_is_required_and_other_titles_are_excluded(self):
        self.assertTrue(steam.selected_file('MCC/Content/Paks/MCC-WindowsNoEditor.pak'))
        for name in ['halo3/maps/guardian.map', 'groundhog/maps/lockout.map', 'MCC/Binaries/Win64/MCC.exe',
                     'MCC/Content/Movies/intro.bk2', 'Engine/Extras/Redist/install.exe']:
            with self.subTest(name=name): self.assertFalse(steam.selected_file(name))
        plan = steam.file_plan(self.depots, self.manifests())
        self.assertEqual(plan['bytes'], 7 * 23)
        self.assertEqual(plan['download_bytes'], 7 * 17)
        self.assertFalse(plan['entitlement_verified'])
        missing = self.manifests(); missing[0]['files'].clear()
        with self.assertRaisesRegex(steam.SteamContentError, 'MCC-WindowsNoEditor.pak'):
            steam.file_plan(self.depots, missing)

    def test_download_rules_match_existing_importer_exclusions(self):
        from prepare_mcc_gameplay import EXCLUDED_SUFFIXES, excluded
        self.assertEqual(set(steam.profile()['excluded_suffixes']), EXCLUDED_SUFFIXES)
        for name in ['h2_maps_win64_dx11/mainmenu.map', 'h2_fonts/font_table.txt',
                     'prebuild/shaders_2_cache/cache.bin', 'prebuild/shaders_2_cache',
                     *['some/file' + suffix for suffix in EXCLUDED_SUFFIXES]]:
            with self.subTest(name=name):
                self.assertEqual(steam.selected_file('halo2/' + name), not excluded(Path(name)))

    def test_manifest_must_match_every_selected_depot_and_reject_aliases(self):
        for change in ['old-manifest', 'missing-depot', 'case-alias', 'linked', 'wrong-owner', 'bad-sha']:
            rows = self.manifests()
            if change == 'old-manifest': rows[1]['manifest_id'] = '42'
            if change == 'missing-depot': rows.pop()
            if change == 'case-alias': rows[1]['files'].append(dict(rows[1]['files'][0], name=rows[1]['files'][0]['name'].upper()))
            if change == 'linked': rows[1]['files'][0]['symlink'] = True
            if change == 'wrong-owner': rows[0]['files'].append(rows[1]['files'].pop())
            if change == 'bad-sha': rows[1]['files'][0]['sha1'] = 'not-a-file-hash'
            with self.subTest(change=change), self.assertRaises(steam.SteamContentError):
                steam.file_plan(self.depots, rows)

    def test_shared_parent_directories_do_not_alias_selected_files(self):
        rows = self.manifests()
        for row in rows: row['files'].insert(0, dict(name='halo2', directory=True))
        self.assertEqual(len(steam.file_plan(self.depots, rows)['files']), 7)

    def test_invalid_ids_and_sizes_never_become_download_estimates(self):
        for value in [-1, True, '1e10', '18446744073709551616', '0']:
            info = copy.deepcopy(self.info)
            info['apps']['976730']['depots']['976735']['manifests']['public']['gid'] = value
            with self.subTest(value=value), self.assertRaises(steam.SteamContentError): steam.depot_plan(info)


if __name__ == '__main__':
    unittest.main()
