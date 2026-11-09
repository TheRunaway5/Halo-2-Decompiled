"""Inventory the complete authenticated MCC external-global registry.

The original external-name lookup supplies its signed16-bit bound and pointer
table. Every record is resolved through that table; a partial inline formula
is not used. Original TU1.5 catalogue/type/backing matches are candidates only,
not engine effect or mod admission. Run through run_test.py; retain small JSON.
"""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import xml.etree.ElementTree as ET

from audit_workshop_compatibility import CATALOG, MCC_PIN, Pe64
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.report.resolve().is_relative_to(owner)
    pe = Pe64(Path.home() / '.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll')
    image = Xbe(NEW_DEFAULT)
    assert sha(image.data) == NEW_PIN and sha(pe.data) == MCC_PIN
    guards = [(0x1806e2d84, '33ed'), (0x1806e2d89, '663b2d60154500'),
        (0x1806e2d90, '0fb7dd'), (0x1806e2d93, '7d2e'), (0x1806e2d95, '488d3dc4377200'),
        (0x1806e2da0, '480fbfd3'), (0x1806e2da7, '488b14d7'), (0x1806e2dab, '488b12'),
        (0x1806e2dae, 'e84db6fdff'), (0x1806e2db5, '745b'), (0x1806e2db7, '66ffc3'),
        (0x1806e2dba, '663b1d2f154500'), (0x1806e2dc1, '7cdd'),
        (0x1806e2e12, 'b80080ffff'), (0x1806e2e17, '660bc3')]
    for address, encoded in guards:
        assert pe.read(address, len(encoded) // 2).hex() == encoded
    count_address = 0x1806e2d90 + int.from_bytes(pe.read(0x1806e2d89 + 3, 4), 'little', signed=True)
    second_count = 0x1806e2dc1 + int.from_bytes(pe.read(0x1806e2dba + 3, 4), 'little', signed=True)
    table = 0x1806e2d9c + int.from_bytes(pe.read(0x1806e2d95 + 3, 4), 'little', signed=True)
    assert count_address == second_count == 0x180b342f0 and table == 0x180e06560
    count = pe.word(count_address, 2)
    assert 0 < count < 0x8000
    profile_path = root / 'tools/mcc-script-layout.json'
    profile = json.loads(profile_path.read_bytes())
    assert sha(CATALOG.read_bytes()) == profile['xbox_catalogue_sha256']
    catalog = ET.parse(CATALOG)
    types = {row.get('name'): int(row.get('opcode'), 16) for row in catalog.findall('.//valueTypes/type')}
    original = {}
    for declaration in catalog.findall('.//globals/global'):
        index = int(declaration.get('opcode'), 16)
        address = int.from_bytes(image.read(0x47fb88 + index * 4, 4), 'little')
        raw = image.read(address, 8)
        kind = int.from_bytes(raw[:2], 'little')
        assert kind == types[declaration.get('type')]
        original.setdefault(declaration.get('name'), []).append(dict(xbox_index=index,
            xbox_descriptor=hex(address), xbox_type=kind, xbox_backing=hex(int.from_bytes(raw[4:8], 'little')),
            xbox_record_hex=raw.hex()))
    admitted = {row['mcc_index']: row for row in profile['external_globals']}
    assert len(admitted) == len(profile['external_globals']) and set(admitted) <= set(range(count))
    rows, flags, kinds, deviations = [], Counter(), Counter(), []
    for index in range(count):
        address = pe.word(table + index * 8)
        raw = pe.read(address, 24)
        name = pe.string(pe.word(address))
        assert name and len(name) <= 4096
        kind, flag = pe.word(address + 8, 2), pe.word(address + 10, 2)
        backing = pe.word(address + 16)
        row = dict(mcc_index=index, name=name, mcc_descriptor=hex(address), mcc_type=kind,
            flags=flag, padding_hex=raw[12:16].hex(), mcc_backing=hex(backing),
            profiled=index in admitted, xbox_candidates=original.get(name, []))
        if index in admitted:
            expected = admitted[index]
            assert all(row[key] == expected[key] for key in ('name', 'mcc_type'))
            if 'mcc_backing' in expected:
                assert row['mcc_backing'] == expected['mcc_backing']
            assert any(candidate['xbox_index'] == expected['xbox_index'] and
                       candidate['xbox_type'] == expected['xbox_type'] and
                       ('xbox_backing' not in expected or candidate['xbox_backing'] == expected['xbox_backing'])
                       for candidate in row['xbox_candidates'])
        if address != 0x180e086f0 + index * 24:
            deviations.append(dict(index=index, actual_descriptor=hex(address)))
        rows.append(row)
        flags[flag] += 1
        kinds[kind] += 1
    report = dict(scope=__doc__, passed=True, mcc_sha256=MCC_PIN, original_TU15_sha256=NEW_PIN,
        profile_sha256=sha(profile_path.read_bytes()), catalogue_sha256=sha(CATALOG.read_bytes()),
        helper_sha256=sha(Path(__file__).read_bytes()), count_address=hex(count_address),
        pointer_table=hex(table), bound_instructions=[[hex(a), encoded] for a, encoded in guards],
        declaration_count=count, distinct_names=len({row['name'] for row in rows}),
        profiled_declarations=len(admitted), unprofiled_declarations=count - len(admitted),
        declarations_with_original_name_candidates=sum(bool(row['xbox_candidates']) for row in rows),
        declarations_with_original_name_and_type_candidates=sum(any(v['xbox_type'] == row['mcc_type']
            for v in row['xbox_candidates']) for row in rows),
        null_MCC_backing_declarations=sum(row['mcc_backing'] == '0x0' for row in rows),
        flag_counts=dict(flags), type_counts=dict(kinds), inline_formula_exceptions=deviations, rows=rows,
        value_type_registry_inventoried=False, engine_global_effects_qualified=False,
        production_admission_changed=False, owned_scratch=str(owner), retained_large_artifacts=[])
    raw = (json.dumps(report, indent=2) + '\n').encode()
    assert len(raw) < 1024 * 1024
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_bytes(raw)
    print(json.dumps({key: report[key] for key in ('passed', 'declaration_count', 'profiled_declarations',
        'unprofiled_declarations', 'null_MCC_backing_declarations', 'declarations_with_original_name_and_type_candidates')}))


if __name__ == '__main__':
    main()
