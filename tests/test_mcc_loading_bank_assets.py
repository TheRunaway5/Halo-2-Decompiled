"""All eleven MCC resident loading pages retain exact owners and mip bytes."""
import copy
import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import mcc_loading_assets as assets
from audit_rendering import FormatError
from export_vista_menu_ui import mip_shapes


def loading_bank_fixture(pack):
    """Small synthetic owners with the standard dimensions/mips and fake pixels."""
    pack = Path(pack)
    banks, resources, expected = {}, {}, []
    for index, page in enumerate(assets.ORIGINAL_PAGES):
        bank_index, suffix, ordinal, role, width, height, fmt, mips, _ = page
        payload = bytes([index + 1]) * sum(row['bytes'] for row in mip_shapes(width, height, mips, fmt))
        sha = hashlib.sha256(payload).hexdigest()
        expected.append((*page[:-1], sha))
        name = 'globals\\loading_screen\\' + suffix
        datum = 0xe5630274 + bank_index * 0x10001
        bank = banks.setdefault(name, dict(map='mainmenu.map', tag=name, datum=f'{datum:08x}', frames=[]))
        record = bytearray(168)
        struct.pack_into('<2H', record, 4, width, height)
        struct.pack_into('<2H', record, 12, fmt, 0x203)
        struct.pack_into('<H', record, 20, mips)
        struct.pack_into('<I', record, 28, datum)
        partition = 'page-' + str(index)
        path = Path('bitmap-resources') / (partition + '.bin')
        target = pack / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(payload)
        resources[partition] = dict(path=path.as_posix(), bytes=len(payload), sha256=sha)
        bank['frames'].append(dict(dimensions=[width, height, 1], type=0, creation_flags=0,
            format=fmt, flags=0x203, additional_mips=mips, record_hex=record.hex(),
            partitions=[partition, None, None]))
    (pack / 'bitmap-resources.json').write_text(json.dumps(dict(banks=list(banks.values()), resources=resources)))
    return tuple(expected)


class OriginalLoadingBankTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix='mcc-original-loading-')
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.pack = self.root / 'cache'
        self.pages = loading_bank_fixture(self.pack)
        patcher = patch.object(assets, 'ORIGINAL_PAGES', self.pages)
        patcher.start()
        self.addCleanup(patcher.stop)

    def test_all_eleven_resident_pages_and_legacy_five_inputs(self):
        result = assets.prepare(self.root, self.pack)
        original = result['original_bank_inputs']
        self.assertEqual((original['bank_count'], original['page_count']), (5, 11))
        self.assertEqual(len(result['presentation_inputs']['inputs']), 5)
        self.assertFalse(original['runtime_compositor_admitted'])
        self.assertFalse(result['external_vista_or_xbox_required'])
        for row in original['pages']:
            self.assertEqual(row['datum_u32'], int(row['datum'], 16))
            self.assertEqual(row['flags'], 0x203)
            self.assertEqual(row['resident_mips'], row['additional_mips'])
            self.assertEqual(row['resident_sha256'], row['source_payload']['sha256'])
            self.assertEqual((self.pack / row['path']).read_bytes()[128:],
                             (self.pack / row['source_payload']['path']).read_bytes())
        self.assertTrue(assets.compatible(self.root, self.pack))

    def test_missing_alternate_logo_or_ticker_cannot_pass(self):
        result = assets.prepare(self.root, self.pack)
        for suffix in ('loading_screen_logos', 'loading_screen_tickers'):
            banks = copy.deepcopy(result['banks'])
            next(bank for bank in banks if bank['tag'].endswith('\\' + suffix))['frames'].pop()
            with self.assertRaisesRegex(FormatError, 'missing required pages'):
                assets.original_bank_inputs(banks)

    def test_wrong_mips_or_payload_hash_rejected(self):
        result = assets.prepare(self.root, self.pack)
        for field, value in [('additional_mips', 0), ('width', 2)]:
            banks = copy.deepcopy(result['banks'])
            banks[-1]['frames'][0][field] = value
            with self.assertRaisesRegex(FormatError, 'accepted source identity'):
                assets.original_bank_inputs(banks)
        banks = copy.deepcopy(result['banks'])
        banks[-1]['frames'][0]['source_payload']['sha256'] = 'f' * 64
        with self.assertRaisesRegex(FormatError, 'accepted source identity'):
            assets.original_bank_inputs(banks)

    def test_full_salt_datum_and_record_flags_must_match(self):
        result = assets.prepare(self.root, self.pack)
        for change in ('datum', 'flags'):
            banks = copy.deepcopy(result['banks'])
            if change == 'datum':
                # Preserve the low word; an owner with a different salt fails.
                banks[0]['datum'] = f'{int(banks[0]["datum"],16) ^ 0x10000:08x}'
            else:
                banks[0]['frames'][0]['flags'] ^= 1
            with self.assertRaisesRegex(FormatError, 'full-salt source record'):
                assets.original_bank_inputs(banks)

    def test_changed_custom_dds_retained_on_reuse_and_preparation(self):
        result = assets.prepare(self.root, self.pack)
        path = self.pack / result['banks'][0]['frames'][0]['path']
        path.write_bytes(b'custom image bytes')
        before = path.read_bytes()
        with self.assertRaisesRegex(FormatError, 'Existing MCC loading DDS differs'):
            assets.compatible(self.root, self.pack)
        with self.assertRaisesRegex(FormatError, 'Existing MCC loading DDS differs'):
            assets.prepare(self.root, self.pack)
        self.assertEqual(path.read_bytes(), before)

    def test_old_metadata_requests_fresh_preparation_without_mutation(self):
        result = assets.prepare(self.root, self.pack)
        del result['original_bank_inputs']
        path = self.pack / assets.MANIFEST
        path.write_text(json.dumps(result))
        before = path.read_bytes()
        self.assertFalse(assets.compatible(self.root, self.pack))
        self.assertEqual(path.read_bytes(), before)


if __name__ == '__main__':
    unittest.main()
