"""MCC source-weapon admission and positional routing; no game execution."""
import json
import os
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import test_source_weapon_effects_mcc_tu15 as runner


class SourceWeaponMccInputs(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()
        self.cache = self.root / 'assets/imported/mcc-current'
        self.maps = self.cache / 'halo2/h2_maps_win64_dx11'
        self.maps.mkdir(parents=True)
        for name in ('01b_spacestation.map', 'lockout.map', 'mainmenu.map', 'shared.map', 'single_player_shared.map'):
            (self.maps / name).write_bytes(struct.pack('<II', 0x68656164, 13) + b'MCC input admission fixture')
        (self.maps / 'textures.dat').write_bytes(b'imported texture bank')
        fonts = self.cache / 'halo2/h2_fonts'
        fonts.mkdir()
        (fonts / 'font_table.txt').write_text('imported font table')
        (self.cache / 'provenance.json').write_text(json.dumps(dict(schema='H2_MCC_TU15_PREPARATION_V1',
            external_installation_requirement='Halo MCC only', runtime_active=False, full_tu15_parity=False)))
        (self.root / 'assets/mcc-source.tsv').write_text('H2_MCC_ASSET_SOURCE_V1\ncache\tassets/imported/mcc-current\n')

    def test_prepared_cache_is_admitted_without_complete_parity_flags(self):
        maps, receipt = runner.selected_inputs(self.root, self.cache)
        self.assertEqual(maps, self.maps)
        self.assertEqual(len(receipt['files']), 5)
        self.assertFalse(receipt['xbox_and_vista_cache_fallback_allowed'])
        self.assertFalse(receipt['import_or_conversion_performed'])

    def test_every_required_map_header_rejects_original_and_vista_versions(self):
        for name in ('01b_spacestation.map', 'lockout.map', 'mainmenu.map', 'shared.map', 'single_player_shared.map'):
            path = self.maps / name
            original = path.read_bytes()
            for version in (8, 10):
                path.write_bytes(struct.pack('<II', 0x68656164, version) + original[8:])
                with self.subTest(name=name, version=version), self.assertRaises(ValueError):
                    runner.selected_inputs(self.root)
            path.write_bytes(original)

    def test_unprepared_or_foreign_cache_is_rejected(self):
        with self.assertRaises((ValueError, OSError)):
            runner.selected_inputs(self.root, self.root / 'foreign')
        (self.cache / 'provenance.json').write_text('{"schema":"unconverted"}')
        with self.assertRaises(ValueError):
            runner.selected_inputs(self.root)

    def test_foreign_selected_cache_and_symlinked_required_bank_reject(self):
        registry = self.root / 'assets/mcc-source.tsv'
        registry.write_text('H2_MCC_ASSET_SOURCE_V1\ncache\t' + str(self.root) + '\n')
        with self.assertRaises(ValueError):
            runner.selected_inputs(self.root)
        registry.write_text('H2_MCC_ASSET_SOURCE_V1\ncache\tassets/imported/mcc-current\n')
        path = self.maps / 'shared.map'; path.rename(self.maps / 'shared-owned.map')
        path.symlink_to(self.maps / 'shared-owned.map')
        with self.assertRaises(ValueError):
            runner.selected_inputs(self.root)

    def test_invalid_registry_rows_reject(self):
        registry = self.root / 'assets/mcc-source.tsv'
        for text in ('H2_MCC_ASSET_SOURCE_V1\ncache\t\n',
                     'H2_MCC_ASSET_SOURCE_V1\ncache\tassets/imported/mcc-current\nextra\n',
                     'H2_MCC_ASSET_SOURCE_V1\ncache\tassets/imported/mcc-current\tother\n'):
            registry.write_text(text)
            with self.subTest(text=text), self.assertRaises(ValueError):
                runner.selected_inputs(self.root)

    def test_native_positional_contract_preserves_paths_with_spaces(self):
        runtime = self.root / 'a runtime/source weapon test'
        maps = self.root / 'a cache/halo2/h2_maps_win64_dx11'
        output = self.root / 'owned scratch/output'
        self.assertEqual(runner.native_command(runtime, maps, output, 'network'),
            [str(runtime), str(maps / 'lockout.map'), str(maps), '--mcc', str(output), '5', 'network'])
        self.assertEqual(runner.native_command(runtime, maps, output, 'direct')[-1], '5')
        self.assertEqual(runner.native_command(runtime, maps, output, 'charge-instant')[-2:],
                         ['authored-plasma-pistol', 'charge-instant'])

    def test_numeric_overrides_and_invalid_weapon_modes_reject(self):
        args = (Path('/runtime'), Path('/maps'), Path('/output'))
        self.assertEqual(runner.native_command(*args, 'match', '20')[-2:], ['20', 'match'])
        for mode, weapon in [('match', 'authored-plasma-pistol'), ('charge-instant', '../reference.xbe'),
                             ('charge-positive', 'authored-plasma-pistol'),
                             ('direct', '2'), ('direct', '-1'), ('direct', '21')]:
            with self.subTest(mode=mode, weapon=weapon), self.assertRaises(ValueError):
                runner.native_command(*args, mode, weapon)

    def test_empty_none_publisher_is_not_instant_charge_acceptance(self):
        publisher = dict(schema='H2_MCC_SOURCE_WEAPON_EFFECTS_V1', passed=True,
            runtime_cache_source='MCC', original_executable_runtime_input=False,
            mode='charge-instant', events=1, accepted=0, source_battery_spent=.15)
        self.assertFalse(runner.result_matches_mode(publisher, 'charge-instant'))
        observed = dict(publisher, instant_charge_required=True,
            charge_metadata=dict(equivalent_to_original=True), charge_instance=0xe6670002,
            charge_positive_allocations=1, charge_damage_returns=1, charge_returned_none=1,
            charge_retired_before_none=True, charge_player_response=True,
            charge_held=True, charge_ready=True, charge_released=True,
            charged_projectile_observed=True, actual_effect_frames=240)
        self.assertTrue(runner.result_matches_mode(observed, 'charge-instant'))
        for changes in (dict(charge_instance=0xffffffff), dict(charge_damage_returns=0),
                        dict(charge_player_response=False),
                        dict(charge_metadata=dict(equivalent_to_original=False))):
            self.assertFalse(runner.result_matches_mode(dict(observed, **changes), 'charge-instant'))

    def test_private_preparation_root_requires_its_actual_scratch_owner(self):
        scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
        self.assertEqual(runner.admission_root(Path.cwd(),self.root,scratch),self.root)
        maps,_=runner.selected_inputs(runner.admission_root(Path.cwd(),self.root,scratch))
        self.assertEqual(maps,self.maps)
        with self.assertRaises(ValueError):runner.admission_root(self.root,Path.cwd(),scratch)
        link=self.root/'root-link';link.symlink_to(self.root,target_is_directory=True)
        with self.assertRaises(ValueError):runner.admission_root(self.root,link,scratch)


if __name__ == '__main__':
    unittest.main()
