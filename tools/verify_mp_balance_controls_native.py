#!/usr/bin/env python3
"""Differential native test against original executed admission/reference controls."""
import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ORIGINAL = ROOT / 'analysis/multiplayer/title-update-1.5/combat-postprocess-controls/measured/report.json'
ORIGINAL_SHA = 'ee0d942c92d4b5fb6e35b2219a35c9bb8fa2b05442ed3d3761d0c9091df6e07b'

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--update-xbe', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert sha(ORIGINAL) == ORIGINAL_SHA, 'original branch-control evidence changed'
    source = json.loads(ORIGINAL.read_text())
    raw = (ROOT / 'assets/disc/maps/lockout.map').read_bytes()
    assert hashlib.sha256(raw).hexdigest() == source['original_cache_sha256']
    images = {'disc_1_1': ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',
              'owned_1_5': args.update_xbe}
    args.output.mkdir(parents=True, exist_ok=True)
    inputs, output = args.output / 'branch-input.map', args.output / 'branch-native-output.map'
    cases = []
    for row in source['cases']:
        xbe = images[row['image_label']]
        assert sha(xbe) == row['xbe_sha256']
        declared = bytearray(raw)
        for operand in row['explicit_control_operand_changes']:
            at = int(operand['file_offset'], 16)
            before = bytes.fromhex(operand['authored_before_hex'])
            after = bytes.fromhex(operand['explicit_control_after_hex'])
            assert declared[at:at + len(before)] == before
            declared[at:at + len(after)] = after
        assert hashlib.sha256(declared).hexdigest() == row['fixture_input_cache_sha256']
        inputs.write_bytes(declared)
        command = [str(args.native.resolve()), str(inputs.resolve()), str(xbe),
                   '-' if row['returned'] else '--bounds-stop', str(output)]
        result = subprocess.run(command, capture_output=True, text=True, check=True)
        actual = json.loads(result.stdout)
        expected_stores = []
        for store in row['ordered_metadata_stores']:
            expected_stores.append([int(store['instruction'], 16), int(store['file_offset'], 16),
                                    int.from_bytes(bytes.fromhex(store['before_hex']), 'little'),
                                    int.from_bytes(bytes.fromhex(store['after_hex']), 'little')])
        assert actual['stores'] == expected_stores, (row['case'], row['image_label'], 'ordered stores')
        if row['returned']:
            index = struct.unpack_from('<I', declared, 16)[0]
            header_magic = struct.unpack_from('<I', declared, index)[0] - (index + 32)
            table = struct.unpack_from('<I', declared, index + 8)[0] - header_magic
            magic = struct.unpack_from('<I', declared, table + 8)[0] - (index + struct.unpack_from('<I', declared, 20)[0])
            scenario_index = struct.unpack_from('<I', declared, index + 12)[0] & 0xffff
            scenario = struct.unpack_from('<I', declared, table + scenario_index * 16 + 8)[0] - magic
            assert actual['admitted'] == (struct.unpack_from('<H', declared, scenario + 0x10)[0] == 1)
        else:
            assert actual['returned'] is False and actual['host_bounds_rejected'] is True
        assert sha(output) == row['complete_cache_output_sha256'], (row['case'], row['image_label'], 'whole cache')
        cases.append({'case': row['case'], 'image': row['image_label'], 'ordered_stores': len(expected_stores),
                      'original_returned': row['returned'], 'native_host_bounds_rejected': not row['returned'],
                      'complete_output_sha256': sha(output)})
    report = {'tool_sha256': sha(__file__), 'native_binary_sha256': sha(args.native),
              'original_report_sha256': ORIGINAL_SHA, 'variants': source['variants'], 'cases': cases,
              'all_match': True, 'scope': 'Explicit disposable original metadata operands; native ordered writes/full outputs match original normal returns and pre-fault prefixes. Native host bounds rejection intentionally replaces guest invalid access, not with a successful source return.'}
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    inputs.unlink()
    output.unlink()
    print(json.dumps({'all_match': True, 'cases': len(cases), 'variants': source['variants'],
                      'host_bounds_rejections': sum(not c['original_returned'] for c in cases),
                      'report_sha256': sha(args.output / 'report.json')}))

if __name__ == '__main__':
    main()
