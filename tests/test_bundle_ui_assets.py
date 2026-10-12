"""Relocation, source-selection preservation and corrupt bundle boundaries."""
import json
from pathlib import Path
import shutil
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import bundle_ui_assets as ui


def tiny_bundle(path):
    packs = {}
    for name in ui.PACKS:
        pack = path/name;pack.mkdir(parents=True)
        if name in ui.REGISTRIES:
            (pack/'fonts').mkdir();(pack/'fonts/font').write_bytes(b'font payload')
            (pack/'bitmaps').mkdir();(pack/'bitmaps/page.dds').write_bytes(b'DDS payload')
            report = dict(source='/unavailable/original/MCC',
                bitmaps=[dict(frames=[dict(dds='bitmaps/page.dds', dds_sha256=ui.sha(pack/'bitmaps/page.dds'))])],
                fonts={'font':dict(bytes=12,sha256=ui.sha(pack/'fonts/font'))})
            (pack/'manifest.json').write_text(json.dumps(report))
            summary = dict(pages=1)
        else:
            (pack/'reference').mkdir();(pack/'reference/font').write_bytes(b'Vista font')
            (pack/'reference/font_table.txt').write_bytes(b'Vista selector table')
            (pack/'button.svg').write_bytes(b'<svg/>');(pack/'atlas.dds').write_bytes(b'prompt atlas')
            row = dict(source_font='reference/font',vector='button.svg',atlas='atlas.dds')
            row.update({key+'_sha256':ui.sha(pack/value) for key,value in list(row.items())})
            report = dict(source_table='reference/font_table.txt',
                source_table_sha256=ui.sha(pack/'reference/font_table.txt'),records=[row],unique_vectors=1)
            (pack/'reconstruction.json').write_text(json.dumps(report))
            summary = {}
        summary['files'] = {p.relative_to(pack).as_posix():dict(bytes=p.stat().st_size,sha256=ui.sha(p))
                            for p in pack.rglob('*') if p.is_file()}
        packs[name] = summary
    (path/'bundle.json').write_text(json.dumps(dict(schema=ui.SCHEMA,packs=packs)))


def game_setup(root):
    (root/'assets').mkdir(parents=True)
    vista = root/'original Vista';(vista/'maps').mkdir(parents=True)
    (vista/'maps/01a_tutorial.map').write_bytes(b'original setup fixture')
    (root/'assets/asset-sources.tsv').write_text('H2_ASSET_SOURCES_V1\nvista\toriginal Vista\n')
    return vista


class BundleTests(unittest.TestCase):
    def test_relocated_source_installs_without_original_mcc(self):
        with tempfile.TemporaryDirectory() as temp:
            original=Path(temp)/'source';original.mkdir();tiny_bundle(original)
            moved=Path(temp)/'relocated source with spaces';shutil.move(original,moved)
            root=Path(temp)/'game checkout with spaces';vista=game_setup(root)
            result=ui.install(root,moved)
            self.assertTrue(result['passed'])
            self.assertFalse(original.exists())
            for pack,(registry,header) in ui.REGISTRIES.items():
                lines=(root/'assets'/registry).read_text().splitlines()
                self.assertEqual(lines[0],header)
                directory=root/lines[1].split('\t')[1]
                self.assertEqual((directory/'fonts/font').read_bytes(),b'font payload')
                self.assertIn(str(vista), (directory/'menu-ui-source.tsv').read_text())
                prompts=Path((directory/'prompt-source.tsv').read_text().splitlines()[1])
                self.assertEqual((prompts/'button.svg').read_bytes(),b'<svg/>')
            # Reusing an installed version must still validate its payloads.
            directory.joinpath('fonts/font').write_bytes(b'changed font')
            with self.assertRaises(ValueError):ui.install(root,moved)

    def test_setup_preserves_an_existing_ui_selection(self):
        with tempfile.TemporaryDirectory() as temp:
            bundle=Path(temp)/'bundle';bundle.mkdir();tiny_bundle(bundle)
            root=Path(temp)/'game';game_setup(root)
            selected=root/'assets/hud-source.tsv';selected.write_text('custom HUD selection\n')
            result=ui.install(root,bundle,only_unselected=True)
            self.assertEqual(selected.read_text(),'custom HUD selection\n')
            self.assertEqual(result['selected'],['mcc-classic-menus'])
            self.assertTrue((root/'assets/menu-source.tsv').exists())

    def test_corrupt_bundle_does_not_change_working_selections(self):
        with tempfile.TemporaryDirectory() as temp:
            bundle=Path(temp)/'bundle';bundle.mkdir();tiny_bundle(bundle)
            root=Path(temp)/'game';game_setup(root)
            for registry,_ in ui.REGISTRIES.values():(root/'assets'/registry).write_text('working selection\n')
            (bundle/'mcc-classic-menus/fonts/font').write_bytes(b'corrupt font')
            with self.assertRaises(ValueError):ui.install(root,bundle)
            for registry,_ in ui.REGISTRIES.values():
                self.assertEqual((root/'assets'/registry).read_text(),'working selection\n')
            self.assertFalse((root/'assets/bundled-ui').exists())


if __name__ == '__main__':unittest.main()
