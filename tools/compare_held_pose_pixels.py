#!/usr/bin/env python3
"""Compare retained held-pose RGB pixels without tolerance or alignment.

The mask must come from a separately verified geometry-coverage comparison.
This measures a render-input replay; it does not certify independent live
animation, all weapon poses, physical Xbox output, or background pixels.
"""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image


def compare(original, native, mask):
    if original.shape != native.shape or original.shape[:2] != mask.shape:
        raise ValueError('Pixel dimensions differ; resizing and alignment are forbidden')
    if original.ndim != 3 or original.shape[2] != 3:
        raise ValueError('Expected RGB pixels')
    if mask.dtype != np.bool_ or not mask.any():
        raise ValueError('An empty or nonbinary coverage mask cannot establish parity')
    delta = native.astype(np.int16) - original.astype(np.int16)
    differences = np.any(delta != 0, axis=2) & mask
    values = np.abs(delta[mask])
    return differences, dict(compared_pixels=int(mask.sum()),
        exact_pixels=int((mask & ~differences).sum()),
        differing_pixels=int(differences.sum()),
        maximum_channel_difference=int(values.max()),
        mean_absolute_channel_difference=float(values.mean()),
        rgb_component_passed=not differences.any())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original', type=Path, required=True)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--coverage', type=Path, required=True)
    parser.add_argument('--native-geometry',type=Path,required=True,
                        help='Actual same-capture GPU geometry receipt; prior coverage cannot qualify another screenshot')
    parser.add_argument('--rectangle', type=int, nargs=4, required=True,
                        metavar=('TOP', 'LEFT', 'BOTTOM', 'RIGHT'))
    parser.add_argument('--original-gl-flip-y', action='store_true')
    parser.add_argument('--native-rectangle', type=int, nargs=4,
                        metavar=('TOP', 'LEFT', 'BOTTOM', 'RIGHT'),
                        help='Declared native viewport in a complete framebuffer')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    geometry = json.loads((args.coverage/'receipt.json').read_text())
    native_geometry=json.loads(args.native_geometry.read_text())
    assert native_geometry['complete'] and native_geometry['rows'], 'Actual GPU geometry is incomplete'
    assert not any(r['mixed_unrelated_geometry'] for r in native_geometry['rows'])
    assert geometry['native_geometry_receipt_sha256']==hashlib.sha256(args.native_geometry.read_bytes()).hexdigest(), 'Coverage belongs to another GPU geometry capture'
    assert Path(native_geometry['capture']).resolve().parent==args.native.resolve().parent, 'RGB screenshot belongs to another capture'
    assert geometry['coverage_passed'] and geometry['differing_pixels'] == 0
    assert geometry['source_rectangle'] == args.rectangle
    assert geometry['original_gl_flip_y'] == args.original_gl_flip_y
    assert geometry['native_rectangle']==args.native_rectangle
    assert not geometry['resizing'] and not geometry['alignment']
    original = np.asarray(Image.open(args.original).convert('RGB'))
    if args.original_gl_flip_y:
        original = np.flipud(original)
    top, left, bottom, right = args.rectangle
    assert 0 <= top < bottom <= original.shape[0]
    assert 0 <= left < right <= original.shape[1]
    original = original[top:bottom, left:right]
    native = np.asarray(Image.open(args.native).convert('RGB'))
    if args.native_rectangle:
        ntop,nleft,nbottom,nright=args.native_rectangle
        assert 0 <= ntop < nbottom <= native.shape[0]
        assert 0 <= nleft < nright <= native.shape[1]
        native=native[ntop:nbottom,nleft:nright]
    raw_mask = np.asarray(Image.open(args.coverage/'original.png').convert('L'))
    assert np.all((raw_mask == 0) | (raw_mask == 255)), 'Coverage mask is not binary'
    mask = raw_mask == 255
    differences, result = compare(original, native, mask)
    assert result['compared_pixels'] == geometry['original_pixels']
    args.output.mkdir(parents=True)
    for name, pixels in [('original', original), ('native', native)]:
        isolated = np.zeros_like(pixels)
        isolated[mask] = pixels[mask]
        Image.fromarray(isolated).save(args.output/(name+'.png'))
    difference_image = np.zeros_like(native)
    difference_image[mask & ~differences] = [255, 255, 255]
    difference_image[differences] = [255, 0, 0]
    Image.fromarray(difference_image).save(args.output/'rgb-differences.png')
    inputs = [args.original, args.native, args.native_geometry,args.coverage/'receipt.json', args.coverage/'original.png']
    result.update(scope='Strict RGB comparison for one retained render-input replay',
        source_rectangle=args.rectangle, original_gl_flip_y=args.original_gl_flip_y,
        native_rectangle=args.native_rectangle,
        tolerance=0, resizing=False, alignment=False, color_adjustment=False,
        independent_animation_checked=False, physical_xbox_capture=False,
        pixel_acceptance=False,
        input_sha256={str(p.resolve()): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs})
    (args.output/'receipt.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({k: v for k, v in result.items() if k != 'input_sha256'}))
    return int(not result['rgb_component_passed'])


if __name__ == '__main__':
    raise SystemExit(main())
