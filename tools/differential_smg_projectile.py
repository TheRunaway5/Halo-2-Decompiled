#!/usr/bin/env python3
"""Compare bounded native stock SMG launch math with original instructions.

This does not establish live muzzle transforms, map activation or impacts.
"""
import argparse
import ctypes as c
import hashlib
import json
from pathlib import Path
import struct
from differential_weapon_error import Vector

ROOT = Path(__file__).resolve().parents[1]
XBE_PIN = 'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
MAP_PIN = '14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9'

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library', type=Path, default=ROOT/'build-multiplayer/libhalo2_weapon_kernels.so')
    parser.add_argument('--report', type=Path, default=ROOT/'analysis/multiplayer/map-pack/smg-allocation-first-motion-1.1-oracle.json')
    parser.add_argument('--output', type=Path, default=ROOT/'analysis/multiplayer/integration/smg-projectile-differential.json')
    args = parser.parse_args()
    report = json.loads(args.report.read_text())
    if report['xbe_sha256'] != XBE_PIN or report['cache_sha256'] != MAP_PIN:
        raise ValueError('Original source report inputs differ')
    if digest(ROOT/'tools/oracle_smg_projectile_1_1.py') != report['oracle_tool_sha256']:
        raise ValueError('Original oracle tool differs from its report')
    native = c.CDLL(str(args.library.resolve())).halo2_smg_projectile_velocity
    native.argtypes = [c.POINTER(Vector)]*3 + [c.c_float]*2 + [c.POINTER(Vector)]*2
    native.restype = None
    count = 0
    for row in report['cases']:
        requests = [e for e in row['events'] if e.get('entry') == '000b8a30']
        if len(requests) != 1:
            raise ValueError('Expected one original request per case')
        raw = bytes.fromhex(requests[0]['request_descriptor_hex'])
        obj = bytes.fromhex(row['after_creation_object_hex'])
        forward = Vector.from_buffer_copy(raw[0x28:0x34])
        aim = Vector(*row['supplied_aim'])
        owner = Vector(*row['supplied_owner_velocity'])
        scale = struct.unpack_from('<f', obj, 0x16c)[0]
        # Read the actual authored launch speed from the pinned local cache.
        data = (ROOT/'assets/disc/maps/lockout.map').read_bytes()
        if hashlib.sha256(data).hexdigest() != MAP_PIN:
            raise ValueError('Local source cache differs')
        speed = struct.unpack_from('<f', data, 31385132 + 0x17c)[0]
        inherited, velocity = Vector(), Vector()
        native(c.byref(aim), c.byref(forward), c.byref(owner), speed, scale,
               c.byref(inherited), c.byref(velocity))
        for name, actual, expected in (('inherited', bytes(inherited), raw[0x40:0x4c]),
                                       ('created', bytes(velocity), obj[0x88:0x94])):
            if actual != expected:
                raise AssertionError((count, name, actual.hex(), expected.hex()))
        count += 1
    args.output.write_text(json.dumps(dict(passed=True, cases=count,
        native_library_sha256=digest(args.library), original_report_sha256=digest(args.report),
        tool_sha256=digest(Path(__file__)), xbe_sha256=XBE_PIN, cache_sha256=MAP_PIN,
        scope='Bit-exact pre-deviation owner projection, descriptor inherited velocity and separate stock projectile constructor arithmetic. No live muzzle/world/impact parity.'), indent=2)+'\n')
    print(f'Matched {count} original SMG owner-projection/launch cases')

if __name__ == '__main__':
    main()
