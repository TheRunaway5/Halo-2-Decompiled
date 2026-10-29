"""Hostile donors stay within aggregate work budgets before publication."""
import io
import contextlib
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from audit_rendering import Cache, FormatError, ImportBudget, donor_inventory
import prepare_mcc_gameplay as preparation
import setup_assets
import test_security_tool_limits as tool_fixture

class ImportLimits(unittest.TestCase):
    def test_ui_tag_aliases_charge_before_frame_lists_expand(self):
        class Aliased:
            block = Cache.block
            magic = 0
            tags = {i:dict(group='bitm',address=1,datum=i,name='ui\\bank'+str(i)) for i in range(4)}
            def tag(self, datum, group): return self.tags[datum],0
            def u32(self, at): return {68:2,72:100}.get(at,0)
            def pointer(self, address, magic, size): return address
        with self.assertRaisesRegex(FormatError,'metadata budget'):
            setup_assets.banks(Aliased(),budget=ImportBudget(items=5))

    def test_aliases_are_charged_even_when_pixels_are_deduplicated(self):
        with tempfile.TemporaryDirectory() as temporary:
            paths, textures = tool_fixture.BitmapBudgetTests().maps(Path(temporary))
            budget = ImportBudget(items=1, retained=1024**2)
            with self.assertRaisesRegex(FormatError, 'metadata budget'):
                preparation.plan_bitmaps(paths, textures, lambda _: None, budget)

    def test_empty_directory_entries_count_before_sorting(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for i in range(4): (root / str(i)).mkdir()
            with self.assertRaisesRegex(FormatError, 'member budget'):
                donor_inventory(root, members=3)
            self.assertEqual(len(donor_inventory(root, members=4)), 4)

    def test_depth_and_symlink_admission(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root/'one'/'two').mkdir(parents=True)
            with self.assertRaisesRegex(FormatError, 'depth budget'):
                donor_inventory(root, depth=1)
            (root/'link').symlink_to(root/'one', target_is_directory=True)
            with self.assertRaisesRegex(FormatError, '[Ss]ymlink'):
                donor_inventory(root)

    def test_vista_empty_entries_reject_before_any_output(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); donor = root/'donor'; maps=donor/'maps'; maps.mkdir(parents=True)
            (maps/'mainmenu.map').touch(); (maps/'shared.map').touch(); (maps/'fonts').mkdir()
            for i in range(20): (maps/'fonts'/str(i)).touch()
            visited=[]; scandir=os.scandir
            @contextlib.contextmanager
            def counted(parent):
                with scandir(parent) as entries:
                    def rows():
                        for entry in entries:
                            if str(parent).startswith(str(maps)): visited.append(entry.name)
                            yield entry
                    yield rows()
            with patch.object(os, 'scandir', counted), patch.object(setup_assets, 'VISTA_MAX_MEMBERS', 4):
                with self.assertRaisesRegex(FormatError, 'member budget'):
                    setup_assets.extract_vista(donor, root/'output')
            self.assertLessEqual(len(visited),5,"Donor was materialized before enforcing member quota")
            self.assertFalse((root/'output').exists())

if __name__ == '__main__': unittest.main()
