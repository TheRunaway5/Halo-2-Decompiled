#!/usr/bin/env python3
"""Compare native single-SMG phases with pinned original instruction traces.

The original suite uses explicit service fixtures and unsuccessful projectile
allocation. This checks scheduling, ammo, every barrel byte and request/RNG,
not successful projectile physics or whole-game multiplayer parity.
"""
import argparse
import ctypes as c
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from differential_weapon_error import Definition as Error, Vector

XBE_PIN = 'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
MAP_PIN = '14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9'

class Definition(c.Structure):
    _fields_ = [('error', Error)] + [(k, c.c_float) for k in
        ('rate', 'rate_rise', 'rate_fall', 'effect_decay')] + [
        ('preparation_effect', c.c_bool)] + [(k, c.c_int16) for k in
        ('dispatch_interval', 'capacity', 'reload_amount', 'full_reload_ticks',
         'empty_reload_ticks', 'ready_ticks', 'single_own_frames')]

class Barrel(c.Structure):
    _fields_ = [('counter', c.c_int8), ('mode', c.c_uint8), ('ticks', c.c_int16)] + [
        (k, c.c_uint16) for k in ('flags', 'shot_count', 'effect_mask')] + [
        (k, c.c_int16) for k in ('effect_index', 'effect_remaining', 'dispatch_counter')] + [
        (k, c.c_float) for k in ('rate', 'effect_a', 'effect_b', 'error',
        'angle_change', 'subshot', 'recovery')] + [('subshot_count', c.c_int16),
        ('marker_count', c.c_uint8), ('reserved', c.c_uint8), ('effect_handle', c.c_uint32)]

class State(c.Structure):
    _fields_ = [('barrel', Barrel)] + [(k, c.c_int16) for k in
        ('loaded', 'reserve', 'magazine_mode', 'magazine_ticks', 'transfer_ticks',
         'blocking_ticks')] + [(k, c.c_uint16) for k in ('weapon_flags', 'input_flags')] + [
        ('trigger_flags', c.c_uint32), ('trigger_ticks', c.c_int16),
        ('trigger_state', c.c_uint8), ('hand_timers', c.c_uint8 * 2),
        ('weapon_state', c.c_uint32)] + [(k, c.c_bool) for k in ('held', 'reload_empty', 'marker')] + [
        ('second_trigger_state', c.c_uint8), ('own_flags', c.c_uint8), ('own_events', c.c_uint16),
        ('magazine_other_ticks', c.c_int16), ('trigger_effect_handle', c.c_uint32),
        ('own_manager', c.c_bool), ('attached', c.c_bool), ('hand_timer_registered', c.c_bool)]

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def snapshot(s, seed):
    b = s.barrel
    return dict(mode=b.mode, counter=b.counter, ticks=b.ticks & 65535,
        flags=b.flags, shot_count=b.shot_count, loaded=s.loaded, reserve=s.reserve,
        magazine_mode=s.magazine_mode, magazine_ticks=s.magazine_ticks & 65535,
        reload_transfer_ticks=s.transfer_ticks & 65535, trigger_state=s.trigger_state,
        trigger_ticks=s.trigger_ticks & 65535, trigger_flags=f'{s.trigger_flags:08x}',
        weapon_flags=f'{s.weapon_flags:04x}', input_flags=f'{s.input_flags:04x}',
        weapon_state=s.weapon_state, hand_timers=list(s.hand_timers), seed=f'{seed.value:08x}',
        barrel_state_hex=bytes(b).hex())

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library', type=Path, default=ROOT / 'build-multiplayer/libhalo2_weapon_kernels.so')
    parser.add_argument('--suite', type=Path, default=ROOT / 'analysis/multiplayer/map-pack/smg-firing-suite/suite.json')
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/multiplayer/integration/smg-differential.json')
    parser.add_argument('--export-fixtures', type=Path,
                        help='Export original postpass states for the actual match test')
    args = parser.parse_args()
    suite = json.loads(args.suite.read_text())
    if suite['xbe_sha256'] != XBE_PIN or suite['cache_sha256'] != MAP_PIN:
        raise ValueError('Original suite input hashes differ')
    map_path = ROOT / 'assets/disc/maps/lockout.map'
    if digest(map_path) != MAP_PIN:
        raise ValueError('Local original map differs')
    image = Xbe(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    if hashlib.sha256(image.data).hexdigest() != XBE_PIN:
        raise ValueError('Local original executable differs')
    data = map_path.read_bytes()
    sample = json.loads((ROOT / suite['cases'][0]['report']).read_text())
    barrel = sample['authored']['fields']['2d0']['file_offset']
    magazine = sample['authored']['fields']['2c0']['file_offset']
    f = lambda offset: struct.unpack_from('<f', data, barrel + offset)[0]
    short = lambda offset: struct.unpack_from('<h', data, magazine + offset)[0]
    # Actual SMG clips selected by the original action lookup, not mag+10's
    # nominal seconds. Both original frame-event blocks must be empty here.
    timing = []
    for va in (0x8021dd18, 0x8021dcac):
        at = va - 0x7e42c800
        if struct.unpack_from('<I', data, at + 0x4c)[0]:
            raise ValueError('Bounded reload has unported animation events')
        timing.append(struct.unpack_from('<h', data, at + 0x14)[0])
    definition = Definition(Error(*(f(x) for x in
        (0xdc, 0xe0, 0x50, 0x54, 0xc4, 0xc8, 0x70, 0x74, 0x78, 0x58, 0x5c, 0x60))),
        f(4), f(0xd4), f(0xd8), f(0xcc), f(0xa0) > 0,
        struct.unpack_from('<h', data, barrel + 0x2e)[0], short(0xa), short(0x14), *timing)
    table = (Vector * 1026).from_buffer_copy(image.read(0x447e40, 1026 * 12))
    native = c.CDLL(str(args.library.resolve()))
    input_ = native.halo2_smg_input
    input_.argtypes = [c.POINTER(State), c.c_int]; input_.restype = None
    reload = native.halo2_smg_reload
    reload.argtypes = [c.POINTER(State), c.POINTER(Definition)]; reload.restype = c.c_int
    update = native.halo2_smg_update
    update.argtypes = [c.POINTER(State), c.POINTER(Definition), c.c_float]; update.restype = c.c_int
    dispatch = native.halo2_smg_dispatch
    dispatch.argtypes = [c.POINTER(State), c.POINTER(Definition), c.POINTER(Vector),
                         c.POINTER(c.c_uint32), c.POINTER(Vector)]
    dispatch.restype = c.c_int
    dt = struct.unpack('<f', struct.pack('<I', 0x3d088889))[0]
    total_ticks = total_requests = 0
    reports = []
    fixture_lines = ['# Original update1.1 source traces; ' + XBE_PIN,
                     '# Lockout input cache SHA256 ' + MAP_PIN,
                     '# case tick held reload loaded reserve magmode magticks transfer hand0 weaponflags triggerflags barrel52hex']
    for case in suite['cases']:
        path = ROOT / case['report']
        if digest(path) != case['sha256']:
            raise ValueError('Original case changed: ' + str(path))
        report = json.loads(path.read_text())
        s = State()
        s.attached = True # Original bare fixture explicitly supplies object+14 parent.
        s.hand_timer_registered = True # Exact WI10000 is registered at first-person+10.
        s.barrel.counter = 127; s.barrel.effect_handle = 0xffffffff
        s.loaded = report['initial_state']['loaded']
        s.reserve = report['initial_state']['reserve']
        seed = c.c_uint32(0x12345678)
        requests = {e['tick']: e for e in report['events'] if 'request_forward' in e}
        deviations = {e['tick']: e['random_result'] for e in report['events'] if 'random_result' in e}
        for row in report['rows']:
            tick = row['tick']
            input_(c.byref(s), row['held'])
            if tick == report['reload_command_tick']:
                if not reload(c.byref(s), c.byref(definition)):
                    raise AssertionError((case['name'], tick, 'reload command rejected'))
            def compare(phase):
                actual = snapshot(s, seed)
                for key, value in actual.items():
                    if row[phase][key] != value:
                        raise AssertionError((case['name'], case['mode'], tick, phase,
                                              key, value, row[phase][key]))
            compare('before')
            update(c.byref(s), c.byref(definition), dt)
            compare('after')
            markers = [e['original_marker'] for e in report['events']
                       if e.get('tick') == tick and 'original_marker' in e]
            if markers and s.marker != bool(markers[0]):
                raise AssertionError((case['name'], tick, 'original marker', markers, s.marker))
            aim = Vector(*report['supplied_aim'])
            emitted = dispatch(c.byref(s), c.byref(definition), c.byref(aim), c.byref(seed), table)
            if bool(emitted) != (tick in requests):
                raise AssertionError((case['name'], tick, 'request presence'))
            if emitted:
                actual = struct.pack('<3f', aim.x, aim.y, aim.z).hex()
                expected = deviations[tick]
                expected = ''.join(struct.pack('<I', int(k, 16)).hex() for k in expected['bits'])
                if actual != expected:
                    raise AssertionError((case['name'], tick, 'request direction', actual, expected))
                total_requests += 1
            compare('post_dispatch')
            before_second = bytes(s), seed.value
            if dispatch(c.byref(s), c.byref(definition), c.byref(aim), c.byref(seed), table) or before_second != (bytes(s), seed.value):
                raise AssertionError((case['name'], tick, 'duplicate second dispatch'))
            total_ticks += 1
            if case['mode'] == 5 and case['name'] not in ('empty_initial', 'empty_no_reserve'):
                expected = row['post_dispatch']
                fixture_lines.append(' '.join(map(str, (case['name'], tick, int(row['held']),
                    int(tick == report['reload_command_tick']), expected['loaded'], expected['reserve'],
                    expected['magazine_mode'], expected['magazine_ticks'], expected['reload_transfer_ticks'],
                    expected['hand_timers'][0], expected['weapon_flags'], expected['trigger_flags'],
                    expected['barrel_state_hex']))))
        reports.append(dict(name=case['name'], mode=case['mode'], ticks=len(report['rows']),
                            requests=len(requests), original_report=case['report'], sha256=case['sha256']))
    result = dict(passed=True, native_library_sha256=digest(args.library),
        original_suite_sha256=digest(args.suite), tool_sha256=digest(Path(__file__)),
        xbe_sha256=XBE_PIN, cache_sha256=MAP_PIN, cases=reports,
        ticks=total_ticks, requests=total_requests,
        scope='All52 barrel bytes, input/ammo/trigger/hand state, request directions and shared RNG across three phases. Original service boundaries include failed projectile allocation; no successful world/projectile parity claim.')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    if args.export_fixtures:
        args.export_fixtures.write_text('\n'.join(fixture_lines) + '\n')
    print(f'Passed {len(reports)} original cases, {total_ticks} ticks, {total_requests} requests')

if __name__ == '__main__':
    main()
