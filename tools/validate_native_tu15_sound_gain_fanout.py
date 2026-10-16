#!/usr/bin/env python3
"""Compare compiled native arithmetic with frozen original numeric outputs.

No original executable, processor program/core or opaque state bank is loaded.
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
assert fixtures['schema'] == 'H2_TU15_GAIN_FANOUT_NUMERIC_VECTORS_V1'
frames = fixtures['frames']
assert len(frames) == fixtures['frame_count'] == 80
assert len({row['case'] for row in frames}) == fixtures['case_count'] == 28
assert len({(row['case'], row['frame']) for row in frames}) == 80
keys = {'flags', 'published_gain_source', 'gains', 'destination_words'}
rows = []
tiers = Counter()
for row in frames:
    words = row['input_words']
    assert len(words) == 211 and words[:9] == [0] * 9 and words[18] in (0, 1)
    assert all(type(word) is int and 0 <= word <= 0xffffff for word in words)
    assert set(row['expected']) == keys
    result = subprocess.run([str(args.probe.resolve())],
                            input=' '.join(map(str, words)) + '\n',
                            text=True, capture_output=True, timeout=10)
    assert result.returncode == 0 and not result.stderr, (row['case'], row['frame'], result.stderr)
    actual = json.loads(result.stdout)
    for key in keys:
        assert actual[key] == row['expected'][key], (row['case'], row['frame'], key)
    assert len(actual['native_addresses']) == 4
    assert all(address > 0xffffffff for address in actual['native_addresses'])
    rows.append({'case': row['case'], 'frame': row['frame'],
                 'oracle_tier': row['oracle_tier'], 'passed': True})
    tiers[row['oracle_tier']] += 1
assert sorted(tiers.values()) == [16, 64]
report = {'passed': True, 'frames': len(rows), 'cases': fixtures['case_count'],
          'oracle_tiers': dict(tiers), 'fields_exact': sorted(keys),
          'real_typed_allocations_above_4GiB': True,
          'runtime_original_program_or_core_loaded': False,
          'whole_DSP_ready': False, 'initializer45_complete': False,
          'fixture_sha256': hashlib.sha256(raw).hexdigest(),
          'probe_sha256': hashlib.sha256(args.probe.read_bytes()).hexdigest(),
          'rows': rows}
args.report.parent.mkdir(parents=True, exist_ok=True)
args.report.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({key: value for key, value in report.items() if key != 'rows'}))
