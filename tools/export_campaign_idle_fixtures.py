#!/usr/bin/env python3
"""Retain direct original initialized-controller/clock outputs as test data."""
import argparse
import hashlib
import json
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--reference', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
raw = a.reference.read_bytes()
r = json.loads(raw)
assert r['xbe_sha256'] == '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
assert r['map_sha256'] == '5093e2f8ef7f0df41c11c6621bac5785a36f78a1cff3b8b2bc6c14b561c02a28'
assert not r['patched_instructions'] and not r['intercepted_callees'] and not r['supplied_runtime_owner_results']
assert len(r['cases']) == 8
for row in r['cases']:
    assert row['constructed']['complete'] and row['bound']['complete'] and len(row['steps']) == 180
    assert all(x['updated']['complete'] and x['transition']['complete'] for x in row['steps'])
fixture = {k: r[k] for k in ['scope', 'xbe_sha256', 'map_sha256', 'tool_sha256',
    'admitted_game_clock_hex', 'resource_phase', 'update_scope']}
fixture['reference_output_sha256'] = hashlib.sha256(raw).hexdigest()
fixture['cases'] = [{k: row[k] for k in ['name', 'object', 'model', 'graph',
    'caller_buffer_fill', 'bound', 'node_count', 'clip_metadata_6c', 'steps']} for row in r['cases']]
output = (json.dumps(fixture, indent=2) + '\n').encode()
if a.output.exists() and a.output.read_bytes() != output:
    raise ValueError('Refusing to replace different original fixture evidence')
a.output.parent.mkdir(parents=True, exist_ok=True)
a.output.write_bytes(output)
print(hashlib.sha256(output).hexdigest(), len(output), 'fixture bytes')
