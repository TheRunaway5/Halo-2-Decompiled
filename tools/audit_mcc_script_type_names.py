"""Authenticate the bounded MCC script type-name catalogue.

The original type-completion wrapper supplies [4,62), excluding four reserved
syntax types. Its unchanged collector indexes the supplied name-pointer table
at eight-byte strides and visits the exact bounded range. Read every literal
name including the four reserved entries, and validate all builtin/global type
references. This does not qualify parser callbacks, value widths or execution.
Run through run_test.py; retain bounded metadata only.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import xml.etree.ElementTree as ET

from audit_workshop_compatibility import CATALOG, MCC_PIN, Pe64


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--registry', type=Path, required=True)
    parser.add_argument('--globals', dest='globals_', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.report.resolve().is_relative_to(owner)
    pe = Pe64(Path.home() / '.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll')
    assert sha(pe.data) == MCC_PIN
    wrapper = [(0x1806e2be0, 'ba04000000'), (0x1806e2be5, '488d0db4997100'),
        (0x1806e2bec, '448d423a'), (0x1806e2bf0, 'e94bfbffff')]
    collector = [(0x1806e2744, '66413bd0'), (0x1806e2748, '0f8d97000000'),
        (0x1806e2753, '66442bc2'), (0x1806e2757, '480fbfc2'), (0x1806e2767, '410fb7f8'),
        (0x1806e276b, '488d1cc1'), (0x1806e277d, '488b33'),
        (0x1806e27cc, '4883c308'), (0x1806e27d0, '4883ef01'), (0x1806e27d4, '759e')]
    for address, raw in wrapper + collector:
        assert pe.read(address, len(raw) // 2).hex() == raw
    first = pe.word(0x1806e2be0 + 1, 4)
    end = first + pe.word(0x1806e2bec + 3, 1)
    table = 0x1806e2bec + int.from_bytes(pe.read(0x1806e2be5 + 3, 4), 'little', signed=True)
    tail = 0x1806e2bf5 + int.from_bytes(pe.read(0x1806e2bf0 + 1, 4), 'little', signed=True)
    assert first == 4 and end == 62 and table == 0x180dfc5a0 and tail == 0x1806e2740
    original = {int(row.get('opcode'), 16): dict(row.attrib) for row in ET.parse(CATALOG).findall('.//valueTypes/type')}
    assert set(original) == set(range(end))
    rows = []
    for index in range(end):
        pointer = pe.word(table + index * 8)
        name = pe.string(pointer)
        assert name and len(name) < 256
        candidate = original[index]
        rows.append(dict(index=index, original_MCC_display_name=name, name_pointer=hex(pointer),
            reserved_syntax=index < first, original_catalogue_name=candidate['name'],
            original_catalogue_size=int(candidate['size']),
            canonical_name_comparison=name == candidate['name'],
            original_size_automatically_admitted_to_MCC=False))
    assert len({row['original_MCC_display_name'] for row in rows}) == end
    registry, globals_ = json.loads(args.registry.read_bytes()), json.loads(args.globals_.read_bytes())
    assert registry['mcc_sha256'] == globals_['mcc_sha256'] == MCC_PIN and globals_['passed']
    assert len(registry['rows']) == registry['declaration_count'] == 918
    assert len(globals_['rows']) == globals_['declaration_count'] == 1086
    references = {row['return_type'] for row in registry['rows']}
    references.update(kind for row in registry['rows'] for kind in row['arguments'])
    references.update(row['mcc_type'] for row in globals_['rows'])
    assert references <= set(range(end))
    report = dict(scope=__doc__, passed=True, mcc_sha256=MCC_PIN, table=hex(table),
        catalogue_sha256=sha(CATALOG.read_bytes()), registry_sha256=sha(args.registry.read_bytes()),
        globals_registry_sha256=sha(args.globals_.read_bytes()),
        helper_sha256=sha(Path(__file__).read_bytes()), type_name_count=end,
        reserved_syntax_types=first, completion_range=[first, end],
        wrapper_instructions=[[hex(a), raw] for a, raw in wrapper],
        collector_range_instructions=[[hex(a), raw] for a, raw in collector],
        declaration_type_IDs_checked=sorted(references), rows=rows,
        parser_callbacks_qualified=False, MCC_storage_widths_qualified=False,
        actual_engine_type_consumers_qualified=False, production_admission_changed=False,
        owned_scratch=str(owner), retained_large_artifacts=[])
    raw = (json.dumps(report, indent=2) + '\n').encode()
    assert len(raw) < 1024 * 1024
    args.report.write_bytes(raw)
    print(json.dumps(dict(passed=True, type_names=end, reserved=first,
                         builtin_declarations_checked=918, external_global_declarations_checked=1086)))


if __name__ == '__main__':
    main()
