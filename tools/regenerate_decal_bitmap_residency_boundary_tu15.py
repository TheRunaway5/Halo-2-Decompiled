#!/usr/bin/env python3
"""Re-emit 181DA0's two decal bitmap residency calls as required boundaries.

Only the 182B17 -> 12F800 and 182B26 -> 12F910 CALL emissions change; every
surrounding generated body stays byte-for-byte. The shared generator
(tools/generate_physics_live_support_tu15.py) owns the same rule for a later
full regeneration. The live World services both sites: an Xbox cache runs the
original callee unchanged, and an MCC World first substitutes the projected
116-byte frame row for the canonical 168-byte bitmap row.
"""
import hashlib, json, struct, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
XBE = Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
SITES = ((0x182b17, 0x12f800), (0x182b26, 0x12f910))


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
        old = f's.push({site:#x}u,{site + 5:#x}u);run_{target:x}(s);'
        new = f's.push({site:#x}u,{site + 5:#x}u);s.required({site:#x}u,{target:#x}u,false);'
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
