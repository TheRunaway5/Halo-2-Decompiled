#!/usr/bin/env python3
"""Re-emit the light natives' other translated call sites in the physics live
support unit as required boundaries:

- C0F60's C100F, C3750's C3810 and C20D0's C2135 -> C4D30 (light reconnect);
- C2260's C2353 -> C2CB0 (light update).

Only those CALL emissions in src/physics/physics_live_support_tu15_generated.cpp
change; every other generated body stays byte-for-byte. The sites are also in
tools/generate_physics_live_support_tu15.py's required-boundary list, so a full
regeneration emits the same calls. The live World services them
(PhysicsLiveWorldTu15::Impl::light_caller): placement's light_reconnect and
attachments' light_update (see docs/native_object_factory_tracks/lights.md),
with the translated callees under HALO2_REFERENCE_LIGHT_CALLERS_TU15=1.
"""
import hashlib, json, struct, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
XBE = Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
SITES = ((0xc100f, 0xc4d30), (0xc3810, 0xc4d30), (0xc2135, 0xc4d30), (0xc2353, 0xc2cb0))


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
    path = ROOT / 'src/physics/physics_live_support_tu15_generated.cpp'
    text = path.read_text()
    original = text
    for site, target in SITES:
        push = f's.push({site:#x}u,{site + 5:#x}u);'
        old = f'{push}run_{target:x}(s);'
        new = f'{push}s.required({site:#x}u,{target:#x}u,false);'
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
