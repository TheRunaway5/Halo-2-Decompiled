"""MCC source/argument rejection controls, not a cutscene rendering fixture."""
import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from audit_rendering import FormatError
from mcc_onboarding import REQUIRED, PROFILE
from preview_opening_cinematic import INPUTS, MAP_DIRECTORY, MAP_NAME, opening_command, opening_inputs


class OpeningInputsTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='opening-mcc-inputs-')
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.cache = self.root / 'assets/imported/mcc-fixture'
        for name in REQUIRED:
            path = self.cache / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b'fixture')
        self.rows = {}
        for name in INPUTS:
            path = self.cache / MAP_DIRECTORY / name
            raw = bytearray(896)
            struct.pack_into('<II', raw, 0, 0x68656164, 13)
            path.write_bytes(raw if name.endswith('.map') else b'MCC-resource-fixture')
            self.rows['h2_maps_win64_dx11/' + name] = dict(
                sha256=hashlib.sha256(path.read_bytes()).hexdigest(), bytes=path.stat().st_size)
        self.provenance = dict(schema=PROFILE, external_installation_requirement='Halo MCC only',
                               source_files=self.rows)
        self.conversion = dict(maps=[])
        self.save()
        (self.root / 'assets/mcc-source.tsv').write_text(
            'H2_MCC_ASSET_SOURCE_V1\ncache\tassets/imported/mcc-fixture\n')

    def save(self):
        (self.cache / 'provenance.json').write_text(json.dumps(self.provenance))
        (self.cache / 'tag-conversion.json').write_text(json.dumps(self.conversion))

    def test_selected_mcc_campaign_arguments(self):
        inputs = opening_inputs(self.root)
        runtime = self.root / 'runtime'
        runtime.touch()
        command = opening_command(runtime, inputs, hidden=True, frames=3)
        self.assertEqual(inputs['source'], 'Halo MCC only')
        self.assertEqual(len(inputs['inputs']), len(INPUTS))
        self.assertEqual(command[command.index('--map') + 1], str(self.cache / MAP_DIRECTORY / MAP_NAME))
        self.assertEqual(command[command.index('--shared-maps') + 1], str(self.cache / MAP_DIRECTORY))
        self.assertIn('--campaign', command)
        for forbidden in ('--gameplay-xbe', '--mcc-preview', '--hud-ui-assets', '--pause-ui-assets'):
            self.assertNotIn(forbidden, command)

    def test_no_selection_has_no_fallback(self):
        (self.root / 'assets/mcc-source.tsv').unlink()
        with self.assertRaisesRegex(FormatError, 'Select a prepared MCC'):
            opening_inputs(self.root)

    def test_foreign_registry_owner_rejected(self):
        for name in ('disc', 'vista'):
            with self.subTest(name=name):
                (self.root / 'assets/mcc-source.tsv').write_text(
                    'H2_MCC_ASSET_SOURCE_V1\ncache\tassets/' + name + '\n')
                with self.assertRaises(FormatError):
                    opening_inputs(self.root)

    def test_relabelled_iso_or_vista_header_rejected(self):
        path = self.cache / MAP_DIRECTORY / MAP_NAME
        raw = bytearray(path.read_bytes())
        struct.pack_into('<I', raw, 4, 8)
        path.write_bytes(raw)
        # Even a matching manifest cannot relabel a v8 cache as MCC.
        self.rows['h2_maps_win64_dx11/' + MAP_NAME]['sha256'] = hashlib.sha256(raw).hexdigest()
        self.save()
        with self.assertRaisesRegex(FormatError, 'version-13'):
            opening_inputs(self.root)

    def test_changed_map_shared_and_audio_rejected(self):
        for name in (MAP_NAME, 'shared.map', 'textures.dat', 'sounds_en.dat'):
            with self.subTest(name=name):
                path = self.cache / MAP_DIRECTORY / name
                raw = path.read_bytes()
                try:
                    path.write_bytes(raw + b'changed')
                    with self.assertRaisesRegex(FormatError, 'differs from prepared MCC'):
                        opening_inputs(self.root)
                finally:
                    path.write_bytes(raw)

    def test_unselected_symlink_rejected(self):
        path = self.cache / MAP_DIRECTORY / MAP_NAME
        outside = self.root / 'other.map'
        path.rename(outside)
        path.symlink_to(outside)
        with self.assertRaisesRegex(FormatError, 'escaped'):
            opening_inputs(self.root)

    def test_converted_map_uses_output_digest(self):
        path = self.cache / MAP_DIRECTORY / MAP_NAME
        path.write_bytes(path.read_bytes() + b'converted')
        self.conversion['maps'] = [dict(map=MAP_NAME, output_sha256=hashlib.sha256(path.read_bytes()).hexdigest())]
        self.save()
        self.assertEqual(opening_inputs(self.root)['map'], str(path))

    def test_broken_correction_chain_rejected(self):
        self.provenance['output_changes'] = [dict(path=MAP_DIRECTORY + '/' + MAP_NAME,
                                                donor_sha256='0' * 64, converted_sha256='1' * 64)]
        self.save()
        with self.assertRaisesRegex(FormatError, 'provenance chain'):
            opening_inputs(self.root)


if __name__ == '__main__':
    unittest.main()
