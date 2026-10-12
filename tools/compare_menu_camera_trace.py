#!/usr/bin/env python3
"""Compare a native authored menu replay with the pinned original research trace.

The reference executes original camera/director/observer instructions with an
externally supplied script schedule. This does not compare retail framebuffers.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import struct


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('native', type=Path)
    parser.add_argument('reference', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    with args.native.open() as source:
        native = list(csv.DictReader(source))
    with args.reference.open() as source:
        reference = {int(row['tick']): row for row in csv.DictReader(source)}
    ticks = [int(row['tick']) for row in native]
    if ticks != list(range(1, 2991)):
        raise ValueError('expected every authored replay tick 1..2990')
    position_mismatches = 0
    maximum_forward_error = 0.0
    for row in native:
        original = reference[int(row['tick'])]
        for axis in ('x', 'y', 'z'):
            position_mismatches += (
                struct.pack('<f', float(row[axis])) !=
                struct.pack('<f', float(original[axis])))
            maximum_forward_error = max(
                maximum_forward_error,
                abs(float(row['forward_' + axis]) - float(original['f' + axis])))
    crossings = sum(int(row['carrier_crossing']) for row in native)
    result = {
        'updates': len(native), 'position_axis_comparisons': len(native) * 3,
        'position_bit_mismatches': position_mismatches,
        'maximum_forward_component_error': maximum_forward_error,
        'carrier_geometry_crossings': crossings,
        'reference_sha256': hashlib.sha256(args.reference.read_bytes()).hexdigest(),
        'scope': 'authored native VM schedule versus original camera/director/observer '
                 'instruction fixture with supplied script schedule and no-BSP location; '
                 'position bits and bounded orientation, not framebuffer parity',
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))
    return int(bool(position_mismatches or crossings or maximum_forward_error > .0001))


if __name__ == '__main__':
    raise SystemExit(main())
