"""Read-only H2EK source ownership, exact decode and optional staging lifecycle."""
import hashlib
import json
import os
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from audit_rendering import FormatError
import h2ek_loading_ambience as ambience


def dds(width, height, fourcc, fill):
    header = bytearray(128)
    header[:4] = b'DDS '
    struct.pack_into('<5I', header, 4, 124, 0x1007, height, width, 0)
    struct.pack_into('<2I', header, 76, 32, 4)
    header[84:88] = fourcc.encode()
    struct.pack_into('<I', header, 108, 0x1000)
    blocks = ((width + 3) // 4) * ((height + 3) // 4) * (8 if fourcc == 'DXT1' else 16)
    return bytes(header) + bytes([fill]) * blocks


def texture_fixture(kit):
    """Six contiguous DDS extents in the original slot order, like loading_dx11.bin."""
    shapes = ((0, 8, 8, 'DXT1'), (1, 16, 4, 'DXT3'), (3, 4, 4, 'DXT3'), (4, 4, 4, 'DXT3'),
              (5, 16, 4, 'DXT1'), (11, 4, 4, 'DXT1'))
    data = bytearray(); images = []
    for slot, width, height, fourcc in shapes:
        image = dds(width, height, fourcc, slot)
        images.append(dict(slot=slot, path=f'loading-{slot:02d}.dds', offset=len(data), bytes=len(image),
            width=width, height=height, fourcc=fourcc, sha256=hashlib.sha256(image).hexdigest()))
        data += image
    kit.mkdir(parents=True, exist_ok=True)
    (kit / ambience.TEXTURE_SOURCE).write_bytes(data)
    return dict(file=ambience.TEXTURE_SOURCE, bytes=len(data), sha256=hashlib.sha256(data).hexdigest(), images=images)


def fixture(root, kit):
    bank = bytes(36)
    data = bytearray(1024)
    data[:2] = b'MZ'
    struct.pack_into('<I', data, 0x3c, 0x80)
    data[0x80:0x84] = b'PE\0\0'
    struct.pack_into('<H', data, 0x86, 1)
    struct.pack_into('<H', data, 0x94, 96)
    struct.pack_into('<H', data, 0x98, 0x10b)
    struct.pack_into('<8s6I2HI', data, 0xf8, b'.rdata\0\0', 512, 0x1000, 512, 512, 0, 0, 0, 0, 0x40000040)
    data[544:544 + len(bank)] = bank
    kit.mkdir(parents=True, exist_ok=True)
    executable = kit / 'guerilla.exe'
    executable.write_bytes(data)
    executable.chmod(0o644)
    policy = dict(schema=ambience.POLICY_SCHEMA, steam_app_id=1613450,
        audio=dict(bytes=len(bank), sha256=hashlib.sha256(bank).hexdigest(),
            decoded_pcm_sha256=hashlib.sha256(bytes(128)).hexdigest(), decoded_frames=64,
            sample_rate=22050, channels=1, playback_frequencies=[21738,16290,29008,10876]),
        sources=[dict(file=executable.name, bytes=len(data), sha256=hashlib.sha256(data).hexdigest(),
            offset=544, section=dict(name='.rdata', virtual_address=0x1000, virtual_size=512,
                raw_offset=512, raw_bytes=512, characteristics_hex='0x40000040'))],
        textures=texture_fixture(kit))
    path = root / ambience.POLICY
    path.parent.mkdir(parents=True)
    path.write_text(json.dumps(policy))
    return policy


class LoadingAmbience(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='h2ek-ambience-', dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name) / 'port'
        self.kit = Path(self.temp.name) / 'kit'
        self.policy = fixture(self.root, self.kit)
        self.cache = self.root / 'assets/imported/cache'
        self.cache.mkdir(parents=True)
        self.provenance = self.cache / 'provenance.json'
        self.provenance.write_text(json.dumps(dict(schema='H2_MCC_TU15_PREPARATION_V1',
            external_installation_requirement='Halo MCC only',
            native_loading_presentation=dict(profile='mcc-logo-glow-mcc-track-oneshot-v1'))))
        self.environment = patch.dict(os.environ, HALO2_H2EK_ROOT=str(self.kit))
        self.environment.start(); self.addCleanup(self.environment.stop)

    def test_source_policy_inventory_contains_no_audio_or_pe(self):
        self.assertEqual(set(ambience.inventory(self.root)), {ambience.POLICY})
        self.assertGreater(ambience.required_bytes(self.root), 36)
        self.assertFalse(any(name.endswith('.exe') for name in ambience.inventory(self.root)))

    def test_prepare_exact_read_only_data_and_repair_missing_defaults(self):
        executable = self.kit / 'guerilla.exe'
        before = (executable.read_bytes(), executable.stat().st_mtime_ns)
        with patch('subprocess.run', side_effect=AssertionError('PE execution attempted')), \
             patch('os.system', side_effect=AssertionError('PE execution attempted')):
            entry = ambience.prepare(self.root, self.cache, progress=lambda _: None)
        self.assertTrue(ambience.verify(self.cache, entry))
        self.assertTrue(entry['runtime_activation'])
        metadata = json.loads((self.cache / ambience.BANK / 'manifest.json').read_bytes())
        self.assertTrue(metadata['decoded_pcm_verified'])
        self.assertFalse(metadata['pe_executed'])
        self.assertEqual((self.cache / ambience.BANK / ambience.TONE).read_bytes(), bytes(36))
        self.assertEqual([row['slot'] for row in metadata['textures']], list(ambience.TEXTURE_SLOTS))
        for row in self.policy['textures']['images']:
            staged = (self.cache / ambience.BANK / row['path']).read_bytes()
            self.assertEqual(hashlib.sha256(staged).hexdigest(), row['sha256'])
            self.assertEqual(metadata['files'][row['path']], dict(bytes=row['bytes'], sha256=row['sha256']))
        _, report = ambience.repair(self.root, self.cache, progress=lambda _: None)
        self.assertEqual(report['native_loading_presentation']['profile'], 'mcc-logo-glow-mcc-track-oneshot-v1')
        path = self.cache / ambience.BANK / ambience.TONE
        path.unlink()
        ambience.repair(self.root, self.cache, progress=lambda _: None)
        self.assertEqual(path.read_bytes(), bytes(36))
        self.assertEqual(before, (executable.read_bytes(), executable.stat().st_mtime_ns))
        self.assertFalse(list(self.cache.rglob('.h2ek-loading-*')))

    def test_cached_complete_bank_survives_missing_kit_and_policy(self):
        ambience.repair(self.root, self.cache, progress=lambda _: None)
        before = self.provenance.read_bytes()
        (self.kit / 'guerilla.exe').unlink()
        (self.root / ambience.POLICY).unlink()
        with patch.object(ambience, 'discover', side_effect=AssertionError('unneeded source discovery')):
            ambience.repair(self.root, self.cache, progress=lambda _: None)
        self.assertEqual(before, self.provenance.read_bytes())

    def test_texture_source_mismatch_publishes_nothing(self):
        source = self.kit / ambience.TEXTURE_SOURCE
        for change in ('missing', 'bytes', 'header'):
            with self.subTest(change=change):
                original = source.read_bytes()
                if change == 'missing': source.unlink()
                if change == 'bytes': source.write_bytes(original[:-1] + b'x')
                if change == 'header':
                    policy = json.loads(json.dumps(self.policy))
                    policy['textures']['images'][0]['width'] = 4
                    policy['textures']['images'][0]['bytes'] = ambience._dds_bytes(policy['textures']['images'][0])
                    (self.root / ambience.POLICY).write_text(json.dumps(policy))
                with self.assertRaises(FormatError):
                    ambience.prepare(self.root, self.cache, progress=lambda _: None)
                self.assertFalse((self.cache / ambience.BANK).exists())
                source.write_bytes(original)
                (self.root / ambience.POLICY).write_text(json.dumps(self.policy))

    def test_legacy_ambience_bank_is_preserved_and_upgraded(self):
        legacy = self.cache / ambience.LEGACY_BANK
        legacy.mkdir(parents=True)
        (legacy / ambience.TONE).write_bytes(bytes(36))
        (legacy / 'manifest.json').write_text('{}')
        files = {ambience.LEGACY_BANK + '/' + name: dict(bytes=(legacy / name).stat().st_size,
                 sha256=hashlib.sha256((legacy / name).read_bytes()).hexdigest()) for name in (ambience.TONE, 'manifest.json')}
        old = dict(schema=ambience.LEGACY_SCHEMA, complete=True, available=True, files=files, runtime_activation=False)
        self.assertFalse(ambience.verify(self.cache, old))
        report = json.loads(self.provenance.read_text())
        self.provenance.write_text(json.dumps({**report, 'h2ek_loading_ambience': old}))
        _, updated = ambience.repair(self.root, self.cache, progress=lambda _: None)
        entry = updated['h2ek_loading_ambience']
        self.assertEqual(entry['schema'], ambience.SCHEMA)
        self.assertEqual(entry['legacy_reference'], old)
        self.assertTrue(ambience.verify(self.cache, entry))
        self.assertEqual((legacy / ambience.TONE).read_bytes(), bytes(36))

    def test_absent_policy_or_kit_is_explicitly_unavailable_and_never_activates(self):
        with patch.dict(os.environ, {}, clear=True), patch.object(ambience, 'discover', return_value=None):
            entry = ambience.prepare(self.root, self.cache, progress=lambda _: None)
            self.assertFalse(entry['complete'])
            self.assertTrue(entry['policy_available'])
            self.assertFalse(ambience.verify(self.cache, entry))
            (self.root / ambience.POLICY).unlink()
            with patch.object(ambience, 'discover', side_effect=AssertionError('host discovery for absent policy')):
                entry = ambience.prepare(self.root, self.cache, progress=lambda _: None)
            self.assertFalse(entry['policy_available'])
        self.assertFalse((self.cache / ambience.BANK).exists())
        self.assertEqual(ambience.inventory(self.root), {})
        self.assertEqual(ambience.required_bytes(self.root), 0)

    def test_source_hash_wrong_offset_section_and_pcm_mismatch_publish_nothing(self):
        path = self.root / ambience.POLICY
        for field in ('source_sha', 'offset', 'section', 'raw_sha', 'pcm_sha'):
            with self.subTest(field=field):
                policy = json.loads(json.dumps(self.policy))
                if field == 'source_sha': policy['sources'][0]['sha256'] = '0' * 64
                if field == 'offset': policy['sources'][0]['offset'] = 500
                if field == 'section': policy['sources'][0]['section']['name'] = '.text'
                if field == 'raw_sha': policy['audio']['sha256'] = '0' * 64
                if field == 'pcm_sha': policy['audio']['decoded_pcm_sha256'] = '0' * 64
                path.write_text(json.dumps(policy))
                with self.assertRaises(FormatError):
                    ambience.prepare(self.root, self.cache, progress=lambda _: None)
                self.assertFalse((self.cache / ambience.BANK).exists())

    def test_custom_bank_and_provenance_are_preserved(self):
        ambience.repair(self.root, self.cache, progress=lambda _: None)
        path = self.cache / ambience.BANK / ambience.TONE
        path.write_bytes(b'custom user bank')
        before = self.provenance.read_bytes()
        with self.assertRaisesRegex(FormatError, 'Customized'):
            ambience.repair(self.root, self.cache, progress=lambda _: None)
        self.assertEqual(path.read_bytes(), b'custom user bank')
        self.assertEqual(before, self.provenance.read_bytes())

    def test_adpcm_matches_native_predictor_and63_nibble_domain(self):
        raw = bytearray(36)
        struct.pack_into('<h', raw, 0, -1234)
        pcm = ambience.decode_mono_adpcm(raw)
        self.assertEqual(pcm, struct.pack('<h', -1234) * 64)
        raw[-1] |= 0xf0
        self.assertEqual(ambience.decode_mono_adpcm(raw), pcm)
        raw[2] = 89
        with self.assertRaisesRegex(FormatError, 'step index'):
            ambience.decode_mono_adpcm(raw)

    def test_explicit_root_errors_and_external_steam_manifest_discovery(self):
        with self.assertRaisesRegex(FormatError, 'HALO2_H2EK_ROOT'):
            ambience.discover(environment={'HALO2_H2EK_ROOT': str(self.root / 'missing')})
        home = Path(self.temp.name) / 'home'
        steam = home / '.local/share/Steam/steamapps'
        steam.mkdir(parents=True)
        library = Path(self.temp.name) / 'external'
        apps = library / 'steamapps';apps.mkdir(parents=True)
        (steam / 'libraryfolders.vdf').write_text('"1" {"path" "' + str(library) + '"}')
        kit = apps / 'common/Custom Editing Kit';kit.mkdir(parents=True)
        (kit / 'guerilla.exe').write_bytes(b'source discovery fixture only')
        (apps / 'appmanifest_1613450.acf').write_text('"installdir" "Custom Editing Kit"')
        self.assertEqual(ambience.discover(home=home, environment={}), kit)


if __name__ == '__main__':
    unittest.main()
