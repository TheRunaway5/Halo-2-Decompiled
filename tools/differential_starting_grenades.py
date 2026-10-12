#!/usr/bin/env python3
"""Check source grenade initialization across authored/variant/cap combinations."""
import argparse
import ctypes as c
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
XBE_PIN = 'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
MAP_PIN = '14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9'
def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library', type=Path, default=ROOT/'build-multiplayer/libhalo2_weapon_kernels.so')
    parser.add_argument('--report', type=Path, default=ROOT/'analysis/multiplayer/integration/inventory-source/grenades.json')
    parser.add_argument('--output', type=Path, default=ROOT/'analysis/multiplayer/integration/starting-grenades-differential.json')
    args = parser.parse_args()
    report = json.loads(args.report.read_text())
    if report['xbe_sha256'] != XBE_PIN or report['cache_sha256'] != MAP_PIN:
        raise ValueError('Source grenade inputs differ')
    native = c.CDLL(str(args.library.resolve())).halo2_starting_grenades
    native.argtypes = [c.c_uint32, c.c_int, c.c_int, c.c_int, c.POINTER(c.c_uint8)]
    native.restype = None
    for row in report['result']:
        counts = (c.c_uint8*2)()
        native(row['supplied_scenario_flags'], row['starting_grenades'],
               row['supplied_maximum_one'], row['supplied_weapon_set'], counts)
        if list(counts) != row['counts']:
            raise AssertionError((row, list(counts)))
    args.output.write_text(json.dumps(dict(passed=True, cases=len(report['result']),
        native_library_sha256=digest(args.library), original_report_sha256=digest(args.report),
        tool_sha256=digest(Path(__file__)), xbe_sha256=XBE_PIN, cache_sha256=MAP_PIN,
        scope='Original starting grenade counts only; selection, throwing and cap-owner lifecycle are separate.'), indent=2)+'\n')
    print(f"Matched {len(report['result'])} original starting-grenade cases")
if __name__ == '__main__': main()
