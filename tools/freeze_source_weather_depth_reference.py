#!/usr/bin/env python3
"""Retain the original snow depth integers for an explicit native D24 upload.

This is an offline RenderDoc export conversion, not a live depth producer.
The captured raw GL rows are retained. UI PickPixel coordinates are top-origin;
their vertical mapping and packed depth lane are checked against actual samples.
"""
import argparse
import array
import hashlib
import json
from pathlib import Path
import struct
import sys


def sha(data):
    return hashlib.sha256(data).hexdigest()


def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('reference', type=Path)
    args = parser.parse_args()
    root = args.reference.resolve()
    metadata_data = (root / 'surfaces.json').read_bytes()
    metadata = json.loads(metadata_data)
    depth = next(x for x in metadata['resources'] if x['id'] == metadata['depth'])
    if depth['format'] != 'D24S8' or depth['mips'] != 1:
        raise ValueError('reference is not the captured single-level D24S8 surface')
    width, height = depth['width'], depth['height']
    before = (root / 'before-scene-depth.raw').read_bytes()
    after = (root / 'after-scene-depth.raw').read_bytes()
    if before != after or len(before) != width * height * 4:
        raise ValueError('depth contents changed or dimensions differ')
    words = array.array('I')
    words.frombytes(before)
    if sys.byteorder != 'little':
        words.byteswap()
    samples = metadata['before_depth_samples']
    if len(samples) < 9:
        raise ValueError('actual depth sample controls are incomplete')
    reciprocal = f32(1.0 / 16777215.0)
    controls = []
    for sample in samples:
        x, y = sample['xy']
        if not (0 <= x < width and 0 <= y < height):
            raise ValueError('sample is outside the captured surface')
        original = words[(height - 1 - y) * width + x]
        integer = original >> 8
        actual = f32(sample['float'][0])
        converted = f32(f32(integer) * reciprocal)
        if struct.pack('<f', converted) != struct.pack('<f', actual):
            raise ValueError('packed depth/row mapping disagrees with actual PickPixel')
        controls.append({'ui_xy': [x, y], 'raw_row': height - 1 - y,
                         'source_word': original, 'depth_integer': integer,
                         'picked_float_word': struct.unpack('<I', struct.pack('<f', actual))[0]})
    # Native depth-aspect upload has the same 24 depth bits in a low-24 lane,
    # with its padding byte zero. There is no float-depth approximation.
    native = array.array('I', (word >> 8 for word in words))
    if sys.byteorder != 'little':
        native.byteswap()
    native_data = native.tobytes()
    output = root / 'before-depth-d24-low24.bin'
    output.write_bytes(native_data)
    record = {'event': metadata['event'], 'width': width, 'height': height,
              'original_depth_format': 'D24S8',
              'source_packing': 'little-endian word: depth bits31..8, stencil bits7..0',
              'native_packing': 'little-endian word: unchanged depth integer bits23..0, zero padding',
              'raw_rows_preserved': True, 'float_depth_substitution': False,
              'before_after_depth_identical': True, 'actual_sample_controls': controls,
              'surface_metadata_sha256': sha(metadata_data),
              'original_depth_sha256': sha(before), 'native_depth_sha256': sha(native_data),
              'native_file': output.name,
              'scope': 'Original xemu software-GL readback; native GPU format/readback validation remains separate.'}
    (root / 'depth-upload.json').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps({'event': record['event'], 'pixels': width * height,
                      'sample_controls_exact': len(controls),
                      'native_depth_sha256': record['native_depth_sha256']}))


if __name__ == '__main__':
    main()
