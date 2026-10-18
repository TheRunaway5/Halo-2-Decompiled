#!/usr/bin/env python3
"""Authenticate exact OG Xbox TU1.5 observer loader bytes and access extents."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
# This independent contract names source-owned extents, not native arrays.
REGIONS = (
    (0x44ea9c, 4, False, 'coordinate minimum'),
    (0x44eaac, 4, False, 'packet duration maximum'),
    (0x44eab8, 24, False, 'six acceleration caps'),
    (0x45f854, 4, False, 'collision probe scale'),
    (0x46a124, 4, False, 'interpolation coefficient twelve'),
    (0x46a128, 4, False, 'interpolation coefficient eight'),
    (0x46a2b8, 4, False, 'interpolation coefficient seven'),
    (0x46a514, 4, False, 'observer aspect'),
    (0x46aa20, 8, False, 'binary64 comparison operand'),
    (0x475288, 4, True, 'initialized frame-delta multiplier'),
    (0x475290, 24, True, 'six packet and six motion short widths'),
    (0x4f64a0, 4, True, 'loader-zero completed delta destination'),
    (0x4f64a4, 4 * 0x358, True, 'loader-zero four-view observer bank'),
    (0x4f7204, 3, True, 'loader-zero reset and policy flags'),
)
ANCHORS = ((0x171e10, 0x3e), (0x171ef0, 0x25), (0x171f20, 0x1d),
           (0x171f40, 0x3b), (0x1720e0, 0x1c), (0x172310, 0xbd),
           (0x1730b8, 0xa), (0x173223, 6))


def generate(xbe, output):
    if output.exists():
        raise ValueError('Preserve the existing original contract; select a fresh output')
    raw = xbe.read_bytes()
    if hashlib.sha256(raw).hexdigest() != PIN or raw[:4] != b'XBEH':
        raise ValueError('Expected the authenticated English OG Xbox TU1.5 XBE')
    word = lambda p: struct.unpack_from('<I', raw, p)[0]
    base, count, table = word(0x104), word(0x11c), word(0x120)
    if count > 128 or table < base or table-base+count*56 > len(raw):
        raise ValueError('Invalid authenticated XBE section table')
    sections = []
    for i in range(count):
        flags, va, virtual, offset, stored, name = struct.unpack_from('<6I', raw, table-base+i*56)
        end = raw.find(b'\0', name-base)
        if stored > virtual or offset+stored > len(raw) or name < base or end < 0:
            raise ValueError('Invalid original section extent')
        sections.append(dict(name=raw[name-base:end].decode('ascii'), flags=flags,
                             address=va, bytes=virtual, raw_offset=offset, raw_bytes=stored))

    def read(address, size):
        matches = [s for s in sections if s['address'] <= address and
                   address+size <= s['address']+s['bytes']]
        if len(matches) != 1:
            raise ValueError('Contract crosses an original virtual section boundary')
        section = matches[0]
        offset = address-section['address']
        stored = max(0, min(size, section['raw_bytes']-offset))
        start = section['raw_offset']+offset
        value = raw[start:start+stored]+bytes(size-stored)
        return section, value, stored

    regions = []
    for address, size, writable, owner in REGIONS:
        section, value, stored = read(address, size)
        if bool(section['flags'] & 1) != writable:
            raise ValueError('Original observer region write permission differs')
        if address >= 0x4f64a0 and (stored or any(value)):
            raise ValueError('Observer destination is not wholly loader-zeroed storage')
        regions.append(dict(address=address, bytes=size, access='ReadWrite' if writable else 'ReadOnly',
                            owner=owner, bytes_hex=value.hex(), sha256=hashlib.sha256(value).hexdigest(),
                            section=section, file_backed_bytes=stored, loader_zero_bytes=size-stored))
    assert len(regions) == 14 and sum(r['bytes'] for r in regions) == 3519
    assert regions[9]['bytes_hex'] == '0000803f'
    assert regions[10]['bytes_hex'] == '030003000200010001000600030003000200010001000300'
    anchors = []
    for address, size in ANCHORS:
        section, value, stored = read(address, size)
        assert section['name'] == '.text' and stored == size
        anchors.append(dict(address=address, bytes=size, bytes_hex=value.hex(),
                            sha256=hashlib.sha256(value).hexdigest()))
    report = dict(schema='H2_NATIVE_TU15_OBSERVER_LOADER_DATA_V1',
                  status='PASS_ORIGINAL_LOADER_BYTES_AND_EXTENTS', xbe_sha256=PIN,
                  generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  regions=regions, anchors=anchors, code_patches=[], service_returns=[],
                  constructors_executed=False, lifecycle_executed=False, gpu_executed=False,
                  limits='Independent file/loader contract only; no execution or completed camera claim')
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(dict(status=report['status'], regions=len(regions), bytes=3519,
                          output=str(output), sha256=hashlib.sha256(output.read_bytes()).hexdigest())))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    generate(args.xbe, args.output)


if __name__ == '__main__':
    main()
