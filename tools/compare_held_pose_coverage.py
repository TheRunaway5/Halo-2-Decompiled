#!/usr/bin/env python3
"""Exact original/native triangle coverage comparison, with no image alignment.

Inputs are RenderDoc drawcall overlays selected by original c0/c50 bytes and
native WORLD vertex words. This is a geometry test, not color/shader or live
animation acceptance. The original GL framebuffer is explicitly flipped to
top-down coordinates before selecting its declared source view rectangle.
"""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original', type=Path, required=True)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--rectangle', type=int, nargs=4, required=True,
                        metavar=('TOP', 'LEFT', 'BOTTOM', 'RIGHT'))
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    args.output.mkdir(parents=True)
    original = json.loads((args.original/'receipt.json').read_text())
    native = json.loads((args.native/'receipt.json').read_text())
    assert original['rows'] and native['rows'] and native['complete']
    assert not any(r['mixed_unrelated_geometry'] for r in native['rows'])
    hashes = {}

    def coverage(directory, rows):
        union = None
        for row in rows:
            assert row['success'] == "<Result: 'Success'>"
            path = directory/(str(row['event'])+'-coverage.png')
            hashes[str(path.resolve())] = hashlib.sha256(path.read_bytes()).hexdigest()
            pixels = np.asarray(Image.open(path).convert('RGB'))
            mask = np.any(pixels != 0, axis=2)
            assert union is None or union.shape == mask.shape
            union = mask if union is None else union | mask
        assert np.any(union), 'Empty overlays cannot establish coverage parity'
        return union

    top, left, bottom, right = args.rectangle
    reference = np.flipud(coverage(args.original, original['rows']))
    assert 0 <= top < bottom <= reference.shape[0]
    assert 0 <= left < right <= reference.shape[1]
    reference = reference[top:bottom, left:right]
    observed = coverage(args.native, native['rows'])
    assert observed.shape == reference.shape, 'No resizing or alignment is allowed'
    original_only = reference & ~observed
    native_only = observed & ~reference
    shared = reference & observed
    difference = np.zeros((*reference.shape, 3), dtype=np.uint8)
    difference[shared] = [255, 255, 255]
    difference[original_only] = [255, 0, 0]
    difference[native_only] = [0, 255, 255]
    Image.fromarray(reference.astype(np.uint8)*255).save(args.output/'original.png')
    Image.fromarray(observed.astype(np.uint8)*255).save(args.output/'native.png')
    Image.fromarray(difference).save(args.output/'coverage-comparison.png')
    mismatch = int(np.count_nonzero(original_only | native_only))
    result = dict(scope='Exact unoccluded triangle coverage for one retained render-input replay; not RGB, live animation or all-weapon acceptance',
                  coverage_passed=mismatch == 0, original_pixels=int(reference.sum()),
                  native_pixels=int(observed.sum()), differing_pixels=mismatch,
                  original_only_pixels=int(original_only.sum()), native_only_pixels=int(native_only.sum()),
                  source_rectangle=args.rectangle, original_gl_flip_y=True,
                  resizing=False, alignment=False, rgb_checked=False,
                  independent_animation_checked=False, pixel_acceptance=False,
                  input_sha256=hashes)
    (args.output/'receipt.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({k: v for k, v in result.items() if k != 'input_sha256'}))
    return int(mismatch != 0)


if __name__ == '__main__':
    raise SystemExit(main())
