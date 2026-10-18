#!/usr/bin/env python3
"""Compare the selected May2003 normalization cube with actual current cache output.

This verifies the complete decoded image, not a GPU frame or the complete MRT
pipeline. The current image/metadata are produced by audit_visor_inputs.cpp.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--current', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise RuntimeError('Refusing to replace retained evidence')
    source = ROOT / 'analysis/pstencil/2003-presentation/extract_resident_environment.py'
    spec = importlib.util.spec_from_file_location('resident', source)
    resident = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(resident)
    digest = hashlib.sha256(resident.DATA).hexdigest()
    if digest != '1c40d851a19dd9077b1a5cc8d84319e00150678dbfed5d6713e7e786973cc4b1':
        raise RuntimeError('Primary May2003 resident cache differs')
    globals_ = next(t for t in resident.TAGS.values() if t['group'] == 'matg')
    root = globals_['payload_file_offset']
    if resident.u(root + 0x134) != 1:
        raise RuntimeError('Original rasterizer globals row missing')
    rasterizer = resident.u(root + 0x138) - resident.MAGIC
    # Original144CD4 publishes this row;962B9..962CA resolves override1 at+1C.
    datum = resident.u(rasterizer + 0x1c)
    bitmap = resident.TAGS[datum]
    if bitmap['group'] != 'bitm' or bitmap['name'] != 'rasterizer\\vector_normalization':
        raise RuntimeError('Actual original normalization binding differs')
    payload = bitmap['payload_file_offset']
    descriptor = resident.u(payload + 0x64) - resident.MAGIC
    u16 = lambda at: int.from_bytes(resident.DATA[at:at + 2], 'little')
    if (resident.u(descriptor), u16(descriptor + 4), u16(descriptor + 6),
        u16(descriptor + 10), u16(descriptor + 12), u16(descriptor + 20)) != (
            0x6269746d, 64, 64, 2, 10, 6):
        raise RuntimeError('Original normalization image declaration differs')
    if not u16(descriptor + 14) & 8:
        raise RuntimeError('Expected original Morton image storage')
    offset, extent = resident.u(descriptor + 0x1c), resident.u(descriptor + 0x2c)
    if extent != 131328 or offset + extent > len(resident.DATA):
        raise RuntimeError('Original padded cubemap extent differs')
    raw = resident.DATA[offset:offset + extent]
    decoded = bytearray()
    mips = []
    for face in range(6):
        start = 0
        for level in range(7):
            width = max(1, 64 >> level)
            mips.append(dict(width=width, height=width, depth=1,
                             offset=len(decoded), size=width * width * 4))
            for y in range(width):
                for x in range(width):
                    morton = sum((((x >> bit) & 1) << (2 * bit)) |
                                 (((y >> bit) & 1) << (2 * bit + 1)) for bit in range(6))
                    at = face * (extent // 6) + start + morton * 4
                    decoded.extend((raw[at + 2], raw[at + 1], raw[at], 255))
            start += width * width * 4
    current = (args.current / 'current-normalization.rgba').read_bytes()
    metadata = json.loads((args.current / 'inputs.json').read_text())
    if len(metadata['materials']) != 2:
        raise RuntimeError('Current armor/visor material pair missing')
    for material in metadata['materials']:
        selected = material['normalization']
        image = selected['image']
        if selected['name'] != bitmap['name'] or selected['frame'] != 0 or (
            image['width'], image['height'], image['layers'], image['format'], image['bytes']) != (
                64, 64, 6, 11, len(decoded)) or image['mips'] != mips:
            raise RuntimeError('Current selected image/mip declaration differs')
    if current != decoded:
        raise RuntimeError('Current normalization pixels differ from the original image')
    result = dict(scope=__doc__, original_cache=str(resident.CACHE), original_cache_sha256=digest,
                  original_bitmap=bitmap, descriptor_file_offset=descriptor,
                  pixel_file_offset=offset, padded_source_bytes=extent,
                  current_directory=str(args.current), decoded_bytes=len(decoded),
                  sha256=hashlib.sha256(decoded).hexdigest(), faces=6, mips_per_face=7,
                  differences=0, full_presentation_verified=False)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k: result[k] for k in ('decoded_bytes', 'faces', 'mips_per_face', 'differences', 'sha256')}))


if __name__ == '__main__':
    main()
