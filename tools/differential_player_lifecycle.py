#!/usr/bin/env python3
"""Compare native history expiration and ordinary eyes to original execution.

This leaves history capture, current-unit ownership and world pose unresolved.
"""
import argparse
import ctypes as c
import hashlib
import json
from pathlib import Path
import struct
from differential_weapon_error import Vector

ROOT = Path(__file__).resolve().parents[1]

class History(c.Structure):
    _fields_ = [('active', c.c_uint8), ('position', Vector),
                ('initial_gate', c.c_int16), ('remaining', c.c_int16)]

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--kernels', type=Path, default=ROOT/'build-multiplayer/libhalo2_kernels.so')
    parser.add_argument('--weapons', type=Path, default=ROOT/'build-multiplayer/libhalo2_weapon_kernels.so')
    parser.add_argument('--output', type=Path, default=ROOT/'analysis/multiplayer/integration/player-lifecycle-differential.json')
    args = parser.parse_args()
    history_path = ROOT/'analysis/multiplayer/integration/spawn-source/lifecycle-report.json'
    eye_path = ROOT/'analysis/multiplayer/map-pack/smg-muzzle-platform-1.1-oracle.json'
    history = json.loads(history_path.read_text())
    eyes = json.loads(eye_path.read_text())
    if not history['passed']:
        raise ValueError('Original lifecycle oracle failed')
    for data, tool in ((history, 'oracle_mp_spawn_lifecycle.py'),
                       (eyes, 'oracle_smg_muzzle_1_1.py')):
        if data['oracle_tool_sha256'] != digest(ROOT/'tools'/tool):
            raise ValueError('Original oracle differs from report')
    for source in history['authorities'].values():
        if digest(ROOT/source['path']) != source['sha256']:
            raise ValueError('Original executable differs')
    tick = c.CDLL(str(args.weapons.resolve())).halo2_tick_spawn_history
    tick.argtypes = [c.POINTER(History)]
    tick.restype = None
    eye = c.CDLL(str(args.kernels.resolve())).halo2_ordinary_unit_eye
    eye.argtypes = [c.POINTER(Vector)]+[c.c_float]*4+[c.POINTER(Vector)]
    eye.restype = None
    ticks = 0
    for rows in history['rows'].values():
        for row in rows['ticks']:
            before, after = row['before'], row['after']
            actual = History(before['active'], Vector(*before['position']),
                             before['initial_gate'], before['remaining'])
            tick(c.byref(actual))
            expected = History(after['active'], Vector(*after['position']),
                               after['initial_gate'], after['remaining'])
            if bytes(actual) != bytes(expected):
                raise AssertionError(('history', row, bytes(actual).hex()))
            ticks += 1
    heights = [struct.unpack('<f',bytes.fromhex(v)[::-1])[0]
               for v in eyes['authored']['eye_height_bits']]
    eye_cases = 0
    for row in eyes['cases']:
        position = Vector(*row['supplied_unit_position'])
        actual = Vector()
        eye(c.byref(position), *heights, row['supplied_crouch_fraction'],
            row['supplied_unit_scale'], c.byref(actual))
        outputs = [e['output']['bits'] for e in row['events']
                   if e.get('original_muzzle_entry') == '000cc320']
        if not outputs:
            raise ValueError('Original eye output missing')
        for output in outputs:
            expected = b''.join(bytes.fromhex(v)[::-1] for v in output)
            if bytes(actual) != expected:
                raise AssertionError(('eye', row['mode'], bytes(actual).hex(), expected.hex()))
            eye_cases += 1
    args.output.write_text(json.dumps(dict(passed=True, history_cases=ticks,
        eye_cases=eye_cases, tool_sha256=digest(Path(__file__)),
        original_reports={str(p.relative_to(ROOT)):digest(p) for p in (history_path, eye_path)},
        native_libraries={str(p.relative_to(ROOT)):digest(p) for p in (args.kernels,args.weapons)},
        scope='Exact history ticker and ordinary unit-eye arithmetic. Live history capture, ownership, animation, attachment modes and complete lifecycle remain separate.'), indent=2)+'\n')
    print(f'Matched {ticks} original history ticks and {eye_cases} original eye outputs')

if __name__ == '__main__':
    main()
