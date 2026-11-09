"""Exercise bundle completeness and rebuild publication with owned tiny inputs."""
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
import custom_asset_cache as bundle
import rebuild_asset_cache
from audit_rendering import FormatError
from custom_ui_test_support import custom_ui_fixture, custom_ui_menu_fixture, empty_custom_map_fixture
from native_variant_test_support import variant_fixture
from test_controller_prompt_assets import controller_prompt_fixture


class BundleTests(unittest.TestCase):
    def setUp(self):
        self.owner = tempfile.TemporaryDirectory(prefix='custom-cache-', dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(self.owner.cleanup)
        self.root = Path(self.owner.name)/'port'
        self.cache = self.root/'assets/imported/mcc-fixture'
        self.cache.mkdir(parents=True)
        custom_ui_fixture(self.root)
        custom_ui_menu_fixture(self.cache, self.root)
        empty_custom_map_fixture(self.root)
        variant_fixture(self.root)
        controller_prompt_fixture(self.root)
        from controller_prompt_assets import prepare as prompts
        from custom_ui_assets import prepare as ranks
        from native_variant_assets import install as variants
        self.report = dict(controller_prompts=prompts(self.root, self.cache, progress=lambda _:None),
                           custom_ui_assets=ranks(self.root, self.cache, progress=lambda _:None))
        variants(self.root, self.cache)

    def test_one_receipt_requires_all_modes_both_prompt_families_and_all_celestial_slots(self):
        entry = bundle.publish(self.root, self.cache, self.report)
        self.assertEqual(bundle.verify(self.root, self.cache, self.report), entry)
        self.assertEqual(entry['game_modes'], ['fiesta-slayer-v1','infection-slayer-v1','bomb-defusal-v1'])
        self.assertFalse(entry['policy']['mcc_button_fallback'])
        self.assertEqual(entry['celestial_slots'], list(range(43,50)))
        for group in ('presentation/prompts-xbox/', 'presentation/prompts-playstation/',
                      'presentation/custom-ranks/', 'data/tags/variants/'):
            name = next(name for name in entry['files'] if name.startswith(group))
            path = self.cache/name
            raw = path.read_bytes()
            path.unlink()
            with self.assertRaisesRegex(FormatError, 'missing or changed'):
                bundle.verify(self.root, self.cache, self.report)
            path.write_bytes(raw)
        self.assertFalse(list(self.cache.glob('.custom-cache-*')))

    def test_missing_or_outdated_receipt_never_allows_partial_mcc_fallback(self):
        with self.assertRaisesRegex(FormatError, 'missing'):
            bundle.verify(self.root, self.cache, self.report)
        bundle.publish(self.root, self.cache, self.report)
        self.report['custom_ui_assets'] = {}
        with self.assertRaisesRegex(FormatError, 'celestial ranks'):
            bundle.verify(self.root, self.cache, self.report)

    def test_tampered_bundle_and_changed_sources_require_rebuild(self):
        bundle.publish(self.root, self.cache, self.report)
        path = self.cache/bundle.MANIFEST
        entry = json.loads(path.read_bytes())
        entry['policy']['mcc_button_fallback'] = True
        path.write_text(json.dumps(entry))
        with self.assertRaisesRegex(FormatError, 'outdated'):
            bundle.verify(self.root, self.cache, self.report)

        bundle.publish(self.root, self.cache, self.report)
        mode = self.root/'data/tags/variants/fiesta-slayer-v1.json'
        value = json.loads(mode.read_bytes())
        value['description'] = 'New authored default'
        mode.write_text(json.dumps(value))
        with self.assertRaisesRegex(FormatError, 'outdated'):
            bundle.verify(self.root, self.cache, self.report)

    def test_catalog_cannot_silently_drop_a_required_map(self):
        from custom_asset_contract_test_support import contract_fixture
        contract_fixture(self.root, ['deserted'])
        with self.assertRaisesRegex(FormatError, 'contract differs'):
            bundle.publish(self.root, self.cache, self.report)

    def test_rebuild_always_bypasses_cache_reuse_and_selects_after_preparation(self):
        import mcc_onboarding
        with patch('frontend_runtime_selection.selected_frontend_owner', return_value=None), \
             patch.object(mcc_onboarding, 'selected', return_value=None), \
             patch.object(mcc_onboarding, 'setup', return_value=(self.cache,self.report)) as setup, \
             patch.object(mcc_onboarding, 'select', return_value=(self.cache,self.report)) as select:
            rebuild_asset_cache.rebuild(self.root, mcc_root=self.root/'MCC', progress=lambda _:None)
        self.assertTrue(setup.call_args.kwargs['force_rebuild'])
        self.assertFalse(setup.call_args.kwargs['select_cache'])
        select.assert_called_once()

    def test_stale_existing_source_folder_falls_back_to_mcc_detection(self):
        import mcc_onboarding
        from test_mcc_gameplay_preparation import source as source_fixture
        stale=self.root/'old-MCC-location'
        stale.mkdir()
        donor=source_fixture(self.root/'actual-MCC')
        with patch('frontend_runtime_selection.selected_frontend_owner',return_value=None), \
             patch.object(mcc_onboarding,'selected',return_value=(self.cache,{'source':str(stale)})), \
             patch.object(mcc_onboarding,'discover',return_value=donor) as discover, \
             patch.object(mcc_onboarding,'prepare',return_value=(self.cache,self.report)), \
             patch.object(mcc_onboarding,'select',return_value=(self.cache,self.report)):
            rebuild_asset_cache.rebuild(self.root,progress=lambda _:None)
        discover.assert_called_once()

    def test_saved_source_is_normalized_and_manual_selection_takes_precedence(self):
        import mcc_onboarding
        from test_mcc_gameplay_preparation import source as source_fixture
        donor=source_fixture(self.root/'actual-MCC')
        manual=self.root/'manual-MCC'
        for supplied,expected in ((None,donor),(manual,manual)):
            with self.subTest(supplied=supplied), \
                 patch('frontend_runtime_selection.selected_frontend_owner',return_value=None), \
                 patch.object(mcc_onboarding,'selected',return_value=(self.cache,{'source':str(donor.parent)})), \
                 patch.object(mcc_onboarding,'setup',return_value=(self.cache,self.report)) as setup, \
                 patch.object(mcc_onboarding,'select',return_value=(self.cache,self.report)):
                rebuild_asset_cache.rebuild(self.root,mcc_root=supplied,progress=lambda _:None)
            self.assertEqual(setup.call_args.args[1],expected)

    def test_rebuild_uses_one_runtime_for_presentation_and_startup_validation(self):
        import mcc_onboarding
        owner={'runtime':self.root/'selected-engine'}
        with patch('frontend_runtime_selection.selected_frontend_owner',return_value=owner), \
             patch('frontend_runtime_selection.check_owner_unchanged') as unchanged, \
             patch.object(mcc_onboarding,'selected',return_value=None), \
             patch.object(mcc_onboarding,'setup',return_value=(self.cache,self.report)) as setup, \
             patch.object(mcc_onboarding,'select',return_value=(self.cache,self.report)), \
             patch('qualify_frontend_runtime.qualify') as qualify:
            rebuild_asset_cache.rebuild(self.root,progress=lambda _:None)
        self.assertEqual(setup.call_args.kwargs['runtime'],owner['runtime'])
        self.assertEqual(qualify.call_args.args[1],owner['runtime'])
        self.assertEqual(qualify.call_args.kwargs['executable_owner'],owner)
        unchanged.assert_called_once_with(owner)

    def test_missing_runtime_proof_prepares_assets_without_granting_startup_admission(self):
        import mcc_onboarding
        runtime=self.root/'halo2_native'
        runtime.write_bytes(b'unqualified executable fixture')
        with patch('frontend_runtime_selection.selected_frontend_owner') as owner, \
             patch.object(mcc_onboarding,'selected',return_value=None), \
             patch.object(mcc_onboarding,'setup',return_value=(self.cache,self.report)), \
             patch.object(mcc_onboarding,'select',return_value=(self.cache,self.report)), \
             patch('qualify_frontend_runtime.qualify') as qualify:
            rebuild_asset_cache.rebuild(self.root,runtime=runtime,progress=lambda _:None)
        owner.assert_not_called()
        qualify.assert_not_called()
        self.assertFalse((self.cache/'frontend-runtime-capability.json').exists())

    def test_failed_native_startup_preserves_selection_and_cleans_new_cache_and_captures(self):
        import mcc_onboarding
        registry = self.root/mcc_onboarding.REGISTRY
        registry.write_text('original selection')
        scratch = []
        def fail(root,runtime,cache,output,temporary,**options):
            self.assertEqual(registry.read_text(), 'original selection')
            self.assertTrue(options['cache_scoped'])
            scratch.append(temporary)
            (temporary/'frame.ppm').write_bytes(b'capture')
            raise RuntimeError('native title capture failed')
        with patch('frontend_runtime_selection.selected_frontend_owner', return_value={'runtime':self.root/'engine'}), \
             patch.object(mcc_onboarding, 'selected', return_value=None), \
             patch.object(mcc_onboarding, 'setup', return_value=(self.cache,self.report)), \
             patch.object(mcc_onboarding, 'select') as select, \
             patch('qualify_frontend_runtime.qualify', side_effect=fail):
            with self.assertRaisesRegex(RuntimeError, 'capture failed'):
                rebuild_asset_cache.rebuild(self.root, progress=lambda _:None)
        select.assert_not_called()
        self.assertEqual(registry.read_text(), 'original selection')
        self.assertFalse(self.cache.exists())
        self.assertTrue(scratch and all(not path.exists() for path in scratch))


if __name__ == '__main__':
    unittest.main()
