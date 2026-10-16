#!/usr/bin/env python3
"""Compare typed native records2/3 against frozen original numeric results.

No original program/core, executable or opaque state bank is loaded at runtime.
"""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
from collections import Counter

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--probe', type=Path, required=True)
parser.add_argument('--vectors', type=Path, required=True)
parser.add_argument('--report', type=Path, required=True)
args = parser.parse_args()
raw = args.vectors.read_bytes()
fixtures = json.loads(raw)
assert fixtures['schema'] == 'H2_TU15_TWO_PRODUCT_ACCUMULATOR_NUMERIC_VECTORS_V1'
frames = fixtures['frames']
assert len(frames) == fixtures['frame_count'] == 40
assert len({row['case'] for row in frames}) == fixtures['case_count'] == 16
assert len({(row['case'], row['record'], row['frame']) for row in frames}) == 40
keys = {'flags', 'gains', 'captured_status_operand', 'exit_arithmetic_saturation',
        'output', 'arithmetic'}
tiers = Counter()
records = Counter()
rows = []
for row in frames:
    words = row['input_words']
    assert row['record'] in (2, 3) and len(words) == 104
    assert all(type(word) is int and 0 <= word <= 0xffffff for word in words)
    assert words[1:5] == [0, 0, 0, 0]
    expected = row['expected']
    assert set(expected) == keys
    assert len(expected['output']) == len(expected['arithmetic']) == 32
    assert all(len(sample) == 4 for sample in expected['arithmetic'])
    result = subprocess.run([str(args.probe.resolve())],
                            input=' '.join(map(str, words)) + '\n',
                            text=True, capture_output=True, timeout=10)
    assert result.returncode == 0 and not result.stderr, (row['case'], row['record'], row['frame'], result.stderr)
    actual = json.loads(result.stdout)
    for key in keys:
        assert actual[key] == expected[key], (row['case'], row['record'], row['frame'], key)
    assert len(actual['native_addresses']) == 6
    assert all(address > 0xffffffff for address in actual['native_addresses'])
    assert not actual['exit_arithmetic_saturation']
    tiers[row['oracle_tier']] += 1
    records[row['record']] += 1
    rows.append({'case': row['case'], 'frame': row['frame'], 'record': row['record'],
                 'oracle_tier': row['oracle_tier'], 'passed': True,
                 'arithmetic_checkpoints': 128})
assert records == {2: 20, 3: 20}
report = {'passed': True, 'frames': len(rows), 'cases': fixtures['case_count'],
          'oracle_tiers': dict(tiers), 'record_calls': dict(records),
          'fields_exact': sorted(keys), 'arithmetic_checkpoints_per_call': 128,
          'real_typed_allocations_above_4GiB': True,
          'runtime_original_program_or_core_loaded': False,
          'source_status_producer_authenticated': False,
          'whole_DSP_ready': False, 'initializer45_complete': False,
          'fixture_sha256': hashlib.sha256(raw).hexdigest(),
          'probe_sha256': hashlib.sha256(args.probe.read_bytes()).hexdigest(),
          'rows': rows}
args.report.parent.mkdir(parents=True, exist_ok=True)
args.report.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({key: value for key, value in report.items() if key != 'rows'}))
