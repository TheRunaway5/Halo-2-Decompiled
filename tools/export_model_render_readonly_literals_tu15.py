#!/usr/bin/env python3
"""Export exact missing fixed model-render .rdata inputs; use run_test.py.

The native census selects missing operand extents, never mathematical results.
Indexed accesses and gaps between referenced extents are not exported.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

ROOT = Path(__file__).absolute().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inventory', type=Path, required=True)
    parser.add_argument('--census', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--manifest', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    assert args.output.resolve().is_relative_to(scratch)
    inventory = json.loads(args.inventory.read_text())
    census = json.loads(args.census.read_text())
    assert inventory['original_xbe_sha256'] == NEW_PIN
    assert sha(ROOT / inventory['module']) == inventory['module_sha256']
    assert census['cold_factory_passed'] and not census['conflicts'] and not census['fragmented']
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    operands = {(row['address'], row['bytes']): row for row in inventory['absolute_readonly_operands']}
    selected = []
    spans = []
    for missing in sorted(census['missing'], key=lambda row: row['address']):
        address, size = missing['address'], missing['bytes']
        row = operands[(address, size)]
        assert row['section'] == '.rdata' and address % 4 == 0 and size in (4, 8)
        assert image.read(address, size).hex() == row['original_hex']
        selected.append(row)
        if spans and address <= spans[-1][1]:
            spans[-1][1] = max(spans[-1][1], address + size)
        else:
            spans.append([address, address + size])
    lines = ['// Exact fixed .rdata inputs of the authenticated model-render module.',
             '// TU1.5 SHA-256: ' + NEW_PIN,
             '// No indexed extent, intervening gap, source instruction or result is supplied.']
    exports = []
    for address, end in spans:
        raw = image.read(address, end - address)
        for byte in range(address, end):
            assert any(row['address'] <= byte < row['address'] + row['bytes'] for row in selected)
        words = struct.unpack('<' + 'I' * (len(raw) // 4), raw)
        lines += [f'share_immutable_words(memory,0x{address:x}u,std::array{{',
                  '    ' + ','.join(f'0x{word:08x}u' for word in words) + '},',
                  '    "native TU1.5 authored model-render fixed readonly operands");']
        exports.append(dict(address=address, bytes=len(raw), original_hex=raw.hex()))
    args.output.write_text('\n'.join(lines) + '\n')
    report = dict(original_xbe_sha256=NEW_PIN, module=inventory['module'],
                  module_sha256=inventory['module_sha256'],
                  inventory_sha256=sha(args.inventory), census_sha256=sha(args.census),
                  missing_operands=len(selected), exported_ranges=exports,
                  exported_bytes=sum(row['bytes'] for row in exports),
                  output_sha256=sha(args.output), exporter_sha256=sha(Path(__file__)),
                  indexed_operands_exported=False, source_instructions_changed=False,
                  supplied_source_results=False, scratch=str(scratch), large_artifacts_retained=[])
    args.manifest.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(operands=len(selected), ranges=len(exports), bytes=report['exported_bytes'])), flush=True)


if __name__ == '__main__':
    main()
