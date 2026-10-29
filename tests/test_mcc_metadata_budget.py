"""Transient preflight metadata must not consume the conversion's live-memory scope."""
import hashlib
import io
import json
import os
from pathlib import Path
import sys
import struct
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
sys.path.insert(0, str(Path(__file__).resolve().parent))
from audit_rendering import FormatError, ImportBudget
import prepare_mcc_gameplay as preparation
from test_mcc_gameplay_preparation import cache_bytes


class MetadataBudgetTests(unittest.TestCase):
    def prepare_resource(self, *, items=100, retained=3600, partition=False):
        temporary = tempfile.TemporaryDirectory(prefix='halo2-metadata-budget-',
                                                 dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(temporary.cleanup)
        root = Path(temporary.name)
        maps, output = root / 'maps', root / 'output'
        maps.mkdir(); output.mkdir()
        source = maps / 'shared.map'
        raw = cache_bytes()
        if partition:
            struct.pack_into('<I', raw, 2384 + 56, 8)
            struct.pack_into('<I', raw, 2384 + 80, 16)
        source.write_bytes(raw)
        source_sha = hashlib.sha256(source.read_bytes()).hexdigest()
        (maps / 'textures.dat').write_bytes(
            b'padding!' + struct.pack('<ii', 1, -8) + b'pixels01' if partition else
            b'no referenced texture partitions')
        self.output = output

        class LimitedBudget(ImportBudget):
            def __init__(self, *args, **kwargs):
                kwargs.setdefault('items', items)
                kwargs.setdefault('retained', retained)
                super().__init__(*args, **kwargs)

        original_cache = preparation.MccCache

        def read_cache(path, budget=None):
            # The unrelated authored scenario projection runs before either
            # metadata scope. Supply another real copy of this tiny cache for
            # that initial read, while the actual map list stays resource-only.
            return original_cache(source if path.name == 'mainmenu.map' else path, budget)

        with patch.object(preparation, 'ImportBudget', LimitedBudget), \
                patch.object(preparation, 'MccCache', read_cache), \
                patch.object(preparation, 'plan_catalog'), \
                patch.object(preparation, 'apply_catalog_plan', return_value={
                    'changed_store_blocks': 0, 'stores': []}):
            summary = preparation.prepare_maps(maps, output, lambda _: None)
        self.assertEqual(hashlib.sha256(source.read_bytes()).hexdigest(), source_sha)
        return summary, output

    def test_released_preflight_data_does_not_spend_the_conversion_memory_limit(self):
        # The real parser spends 777B of conservative preflight estimates.
        # Conversion needs 3328B; each lifetime fits a 3600B limit independently.
        # Combining their historical allocations incorrectly rejects this map.
        summary, output = self.prepare_resource()
        self.assertEqual(summary['maps'], 1)
        self.assertEqual(summary['bitmap_frames'], 1)
        tags = json.loads((output / 'tag-inventory.json').read_text())
        bitmaps = json.loads((output / 'bitmap-resources.json').read_text())
        self.assertEqual(tags['schema'], 'H2_MCC_TAG_INVENTORY_V1')
        self.assertEqual(tags['maps'][0]['map'], 'shared.map')
        self.assertEqual(len(tags['maps'][0]['tags']), 1)
        self.assertEqual(bitmaps['schema'], 'H2_MCC_BITMAP_RESOURCES_V1')
        self.assertEqual(len(bitmaps['banks'][0]['frames']), 1)

    def test_global_reference_work_is_not_refilled_between_memory_scopes(self):
        # Four preflight references and six conversion references exceed seven,
        # even though either phase would fit if the work counter were reset.
        with self.assertRaisesRegex(FormatError, 'metadata budget'):
            self.prepare_resource(items=7, retained=1024 ** 2)

    def test_one_phase_still_cannot_exceed_its_live_metadata_limit(self):
        with self.assertRaisesRegex(FormatError, 'metadata budget'):
            self.prepare_resource(retained=3000)

    def test_partition_plan_remains_charged_during_conversion(self):
        # The one 512B plan entry remains live after preflight. Conversion's
        # 3328B estimate fits 3600B alone, but not alongside this retained plan.
        with self.assertRaisesRegex(FormatError, 'metadata budget'):
            self.prepare_resource(partition=True)
        self.assertEqual(list((self.output / 'bitmap-resources').glob('*.bin')), [])

    def test_plan_and_conversion_fit_without_resetting_or_raising_the_cap(self):
        summary, output = self.prepare_resource(partition=True, retained=4500)
        self.assertEqual((summary['bitmap_partitions'], summary['decoded_bitmap_bytes']), (1, 8))
        manifest = json.loads((output / 'bitmap-resources.json').read_text())
        resource = next(iter(manifest['resources'].values()))
        self.assertEqual((output / resource['path']).read_bytes(), b'pixels01')

    def test_phase_rejects_invalid_carry_without_changing_remaining_allowance(self):
        for carried in (-1, 1025, 1.5, '512'):
            with self.subTest(carried=carried):
                budget = ImportBudget(items=3, retained=1024)
                budget.charge(1, 256)
                with self.assertRaisesRegex(FormatError, 'metadata budget'):
                    budget.begin_retained_phase(carried_bytes=carried)
                self.assertEqual((budget.items, budget.retained), (2, 768))

    def test_deduplicated_pixel_aliases_still_spend_global_reference_work(self):
        with tempfile.TemporaryDirectory(prefix='halo2-metadata-alias-',
                                         dir=os.environ.get('HALO2_TEST_SCRATCH')) as temporary:
            root = Path(temporary)
            paths = []
            for i, pointer in enumerate((8, 0x80000008)):
                raw = cache_bytes()
                struct.pack_into('<I', raw, 2384 + 56, pointer)
                struct.pack_into('<I', raw, 2384 + 80, 16)
                path = root / f'{i}.map'; path.write_bytes(raw); paths.append(path)
            textures = io.BytesIO(b'padding!' + struct.pack('<ii', 1, -8) + b'pixels01')
            with self.assertRaisesRegex(FormatError, 'metadata budget'):
                preparation.plan_bitmaps(paths, textures, lambda _: None,
                                         ImportBudget(items=8, retained=1024 ** 2))
            budget = ImportBudget(items=9, retained=1024 ** 2)
            plan, total = preparation.plan_bitmaps(paths, textures, lambda _: None, budget)
            self.assertEqual((len(plan), total, budget.items), (1, 8, 0))


if __name__ == '__main__':
    unittest.main()
