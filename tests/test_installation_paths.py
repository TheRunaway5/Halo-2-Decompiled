"""Exercise immutable resource readers with independently owned mutable data."""
import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from installation_paths import ENVIRONMENT, SCHEMA, resource_root
import mcc_movies
import mcc_ui_layout
import mcc_onboarding
from test_mcc_movies import bundle_fixture
from test_mcc_ui_layout import layout_fixture
from native_variant_test_support import variant_fixture
from native_variant_assets import LAYOUTS
from custom_ui_test_support import custom_ui_fixture, custom_ui_menu_fixture, empty_custom_map_fixture


class InstallationOwnershipTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix='installation-owners-')
        self.addCleanup(temporary.cleanup)
        self.work = Path(temporary.name).resolve()
        self.data = self.work / 'User data Ω'
        self.resources = self.work / 'Read-only app Δ/Contents/Resources'
        self.resources.mkdir(parents=True)
        binding = json.dumps(dict(schema=SCHEMA, data=str(self.data), resources=str(self.resources)))
        context = patch.dict(os.environ, {ENVIRONMENT: binding})
        context.start(); self.addCleanup(context.stop)

    def snapshot(self):
        return {str(p.relative_to(self.resources)): hashlib.sha256(p.read_bytes()).hexdigest()
                for p in self.resources.rglob('*') if p.is_file()}

    def test_binding_is_read_only_exact_and_idempotent(self):
        self.assertEqual(resource_root(self.data), self.resources)
        self.assertFalse(self.data.exists())
        self.assertEqual(resource_root(self.resources), self.resources)
        unrelated = self.work / 'Other project'
        self.assertEqual(resource_root(unrelated), unrelated)
        self.assertFalse(unrelated.exists())

    def test_source_checkout_has_no_binding_requirement(self):
        with patch.dict(os.environ):
            os.environ.pop(ENVIRONMENT)
            self.assertEqual(resource_root(self.data), self.data)
        with patch.dict(os.environ, {ENVIRONMENT: json.dumps(dict(schema=SCHEMA,
            data=str(self.resources), resources=str(self.resources)))}):
            self.assertEqual(resource_root(self.resources), self.resources)

    def test_invalid_or_overlapping_bindings_fail(self):
        for value in ('{', 'null', json.dumps(dict(schema=SCHEMA, data='relative', resources=str(self.resources))),
            json.dumps(dict(schema=SCHEMA, data=str(self.resources / 'data'), resources=str(self.resources))),
            json.dumps(dict(schema=SCHEMA, data=str(self.work), resources=str(self.resources)))):
            with self.subTest(value=value), patch.dict(os.environ, {ENVIRONMENT: value}):
                root = self.data
                if value.startswith('{') and value != '{':
                    parsed = json.loads(value)
                    root = Path(parsed['data'])
                with self.assertRaises(ValueError):
                    resource_root(root)

    def test_movies_read_app_resources_and_publish_in_data_cache(self):
        expected = bundle_fixture(self.resources)
        cache = self.data / 'assets/imported/mcc-fixture'
        cache.mkdir(parents=True)
        before = self.snapshot()
        result = mcc_movies.prepare(self.data, cache, progress=lambda _: None)
        self.assertTrue(result['complete'])
        for name in expected['files']:
            self.assertEqual((cache / mcc_movies.BANK / name).read_bytes(),
                             (self.resources / 'resources/movies' / name).read_bytes())
        self.assertEqual(self.snapshot(), before)
        self.assertFalse((self.data / 'resources').exists())

    def test_ui_profile_ignores_mutable_shadow_resource(self):
        layout_fixture(self.resources)
        shadow = self.data / mcc_ui_layout.SOURCE
        shadow.parent.mkdir(parents=True)
        shadow.write_text('not an authorized UI profile')
        before = self.snapshot()
        self.assertEqual(mcc_ui_layout.source_profile(self.data), mcc_ui_layout.source_profile(self.resources))
        self.assertEqual(self.snapshot(), before)
        self.assertEqual(shadow.read_text(), 'not an authorized UI profile')

    def test_mcc_selection_registry_stays_with_mutable_cache(self):
        variant_fixture(self.resources)
        custom_ui_fixture(self.resources)
        empty_custom_map_fixture(self.resources)
        cache = self.data / 'assets/imported/mcc-fixture'
        for name in mcc_onboarding.REQUIRED:
            path = cache / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('synthetic cache selection fixture')
        (cache / 'provenance.json').write_text(json.dumps(dict(schema='H2_MCC_TU15_PREPARATION_V1',
            external_installation_requirement='Halo MCC only', runtime_active=False, full_tu15_parity=False)))
        custom_ui_menu_fixture(cache, self.resources)
        decoy = self.resources / mcc_onboarding.REGISTRY
        decoy.parent.mkdir(parents=True)
        decoy.write_text('immutable resource-side decoy')
        before = self.snapshot()
        mcc_onboarding.select(self.data, cache, verify=False, progress=lambda _: None)
        selected, proof = mcc_onboarding.selected(self.data)
        self.assertEqual(selected, cache)
        self.assertFalse(proof['runtime_active'])
        self.assertFalse(proof['full_tu15_parity'])
        self.assertTrue((self.data / mcc_onboarding.REGISTRY).is_file())
        self.assertTrue(proof['custom_ui_assets']['complete'])
        self.assertTrue((cache / 'presentation/custom-ranks/manifest.json').is_file())
        self.assertEqual(self.snapshot(), before)
        for name in ('data/tags/variants/bomb-defusal-v1.json', LAYOUTS):
            with self.subTest(asset=name):
                self.assertEqual((cache / name).read_bytes(), (self.resources / name).read_bytes())
                self.assertEqual((self.data / name).read_bytes(), (self.resources / name).read_bytes())


if __name__ == '__main__':
    unittest.main()
