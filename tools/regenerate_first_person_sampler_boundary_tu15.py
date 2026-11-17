#!/usr/bin/env python3
"""Re-emit 169C46's 16A479 -> 16A4DD and 168D55's -> 169C46 calls as required
boundaries.

Only those CALL emissions in
src/gameplay/objects/source_sandbox_special_tu15_generated.cpp changes; every
other generated body stays byte-for-byte. A later full regeneration through
tools/generate_sandbox_special_tu15.py must run this script again. The live
World services the sites (PhysicsLiveWorldTu15::Impl::node_rebuild_site): the
native first-person sampler (track nodebuild's first_person_animation) and the
native first-person update (first_person_update, which calls the sampler
natively) by default, the translated 16A4DD/169C46 under
HALO2_REFERENCE_NODE_REBUILD_TU15=1 or HALO2_REFERENCE_NODE_BIRTH_TU15=1. The
map-pack image keeps the direct calls.
"""
import hashlib, json, struct, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
XBE = Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
SITES = ((0x16a479, 0x16a4dd), (0x168d55, 0x169c46))


def read_va(data, va, size):
    base = struct.unpack_from('<I', data, 0x104)[0]
    count = struct.unpack_from('<I', data, 0x11c)[0]
    headers = struct.unpack_from('<I', data, 0x120)[0] - base
    for n in range(count):
        _, address, virtual_size, raw, raw_size = struct.unpack_from('<IIIII', data, headers + n * 56)
        if address <= va < address + virtual_size and va - address + size <= raw_size:
            offset = raw + va - address
            return data[offset:offset + size]
    raise SystemExit(f'{va:#x} is outside the authenticated image')


def main():
    xbe = Path(sys.argv[1]) if len(sys.argv) > 1 else XBE
    data = xbe.read_bytes()
    if hashlib.sha256(data).hexdigest() != PIN:
        raise SystemExit('Selected executable is not the authenticated English TU1.5 image')
    calls = []
    for site, target in SITES:
        code = read_va(data, site, 5)
        if code[0] != 0xe8 or site + 5 + struct.unpack_from('<i', code, 1)[0] != target:
            raise SystemExit(f'{site:#x} is not CALL {target:#x}')
        calls.append(dict(call_pc=f'{site:08x}', call_bytes=code.hex(), target=f'{target:08x}'))
    path = ROOT / 'src/gameplay/objects/source_sandbox_special_tu15_generated.cpp'
    text = path.read_text()
    original = text
    for site, target in SITES:
        push = f's.push({site:#x}u,selected({site + 5:#x}u,s.image));'
        old = f'{push}run_{target:x}(s);'
        new = (f'{push}if(s.image==SourcePhysicsContactImage::TitleUpdate15)s.required({site:#x}u,{target:#x}u,false);'
               f'else run_{target:x}(s);')
        if text.count(old) + text.count(new) != 1:
            raise SystemExit(f'{site:#x} emission is not unique in {path.name}')
        text = text.replace(old, new)
    path.write_text(text)
    report = dict(original_sha256=PIN, calls=calls, path=str(path.relative_to(ROOT)),
                  prior_generated_cpp_sha256=hashlib.sha256(original.encode()).hexdigest(),
                  generated_cpp_sha256=hashlib.sha256(text.encode()).hexdigest())
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
