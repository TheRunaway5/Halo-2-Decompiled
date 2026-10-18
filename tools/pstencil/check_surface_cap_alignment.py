"""Check matched surface/near-cap vertices exported from an actual GPU frame.

The pinned surface triangles come from the visibly stippled Prophet collar and
hand in prophet-surface-v1. Find near caps by their actual posed positions:
animation can change the number/order of preceding light-facing triangles.
This checks the transform invariant, not whole-frame parity.
Run after export_surface_vertex_pairs.py; pass its output directory.
"""
import argparse
import json
from pathlib import Path
import struct


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('vertices', type=Path)
parser.add_argument('--output', type=Path)
args = parser.parse_args()
events = {e['event']: e for e in json.loads((args.vertices / 'vertices.json').read_text())['events']}
failures = 0
report = []
volume = events[1448]
volume_input = (args.vertices / '1448-input-0.bin').read_bytes()
stride = volume['vertexBindings'][0]['byteStride']
caps = {}
for triangle in range(len(volume_input) // stride // 3):
    points = [struct.unpack_from('4f', volume_input, n * stride)
              for n in range(triangle * 3, triangle * 3 + 3)]
    if all(p[3] == 0 for p in points):
        caps.setdefault(frozenset(p[:3] for p in points), []).append(triangle)
for name, surface, triangle in [('collar', 860, 221), ('hand', 830, 452)]:
    inputs = (args.vertices / f'{surface}-input-0.bin').read_bytes()
    points = frozenset(struct.unpack_from('3f', inputs, n * events[surface]['vertexBindings'][0]['byteStride'])
                       for n in range(triangle * 3, triangle * 3 + 3))
    matches = caps.get(points, [])
    assert matches, 'no near cap matches the actual posed surface triangle'
    for cap in matches:
        positions = {}
        for event, primitive in [(surface, triangle), (1448, cap)]:
            row = events[event]
            inputs = (args.vertices / f'{event}-input-0.bin').read_bytes()
            outputs = (args.vertices / f'{event}-vsout.bin').read_bytes()
            positions[event] = {
                struct.unpack_from('3f', inputs, n * row['vertexBindings'][0]['byteStride']):
                struct.unpack_from('4f', outputs, n * row['mesh']['vertexByteStride'])
                for n in range(primitive * 3, primitive * 3 + 3)
            }
        assert positions[surface].keys() == positions[1448].keys(), 'paired posed geometry differs'
        different = sum(positions[surface][p] != positions[1448][p] for p in positions[surface])
        failures += different
        report.append(dict(surface=name, event=surface, triangle=triangle,
                           volume_event=1448, cap=cap, matching_posed_vertices=3,
                           different_clip_positions=different))
        print(f'{name}: matching posed vertices=3, different clip positions={different}')
if args.output:
    args.output.write_text(json.dumps(dict(pairs=report, different_clip_positions=failures), indent=2) + '\n')
if failures:
    raise SystemExit(f'FAIL: {failures} near-cap vertices do not coincide with their visible surfaces')
print('PASS: captured surface and near-cap transforms agree exactly')
