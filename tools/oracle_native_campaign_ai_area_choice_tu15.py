#!/usr/bin/env python3
"""Observe unchanged authored area selection and its real generic dependencies.

Only retained actors, authored areas, frame data and FP policy are inputs.
No iterator state, selection outcome, instruction or dependency result is set.
"""
import argparse
import copy
import gzip
import json
import os
from pathlib import Path
import struct
import subprocess

from oracle_smg_firing_1_1 import ROOT, Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
import oracle_native_campaign_ai_area_tu15 as areas
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry

ENTRIES = (0x1bbbd0, 0x1bbec0, 0x20d180, 0x20d2d0, 0x218780, 0x145070, 0x219030)
OUTPUT = areas.BASE + 0x12000
FRAMES, OBJECT_POOL, OBJECT_ROWS, BODY, CANDIDATES = (areas.BASE + n for n in
    (0x14000, 0x15000, 0x16000, 0x18000, 0x20000))


def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def inputs(image):
    rows = []
    originals = areas.inputs()
    def make(name, path='default', count=4, mask=0, entry=0x1bbbd0, mode=0,
             frame=0xffff, state='valid', positions=None, facing=(1., 0., 0.),
             current=(0, 0), changes=(), timeline=()):
        seed = next(r for r in originals if r['entry'] == 0x1c5870 and
                    r['name'].startswith(path + '_'))
        row = copy.deepcopy(seed)
        raw = bytearray.fromhex(next(b['before'] for b in row['banks'] if b['address'] == areas.BASE))
        raw.extend(bytes(0x30000-len(raw)))
        def put(address, value, width=4):
            assert areas.BASE <= address and address + width <= areas.BASE + len(raw)
            raw[address-areas.BASE:address-areas.BASE+width] = (value & ((1 << (8*width))-1)).to_bytes(width, 'little')
        put(areas.ACTORS+0x26c, 0xffffffff if mode == 0 else 0)
        for n, value in enumerate(facing):
            put(areas.ACTORS+0x290+n*4, bits(value))
        for n in range(3):
            put(areas.ACTORS+0x238+n*4, bits(0.))
        put(areas.ZONES+0x30, count)
        put(areas.ZONES+0x34, CANDIDATES)
        for n in range(40):
            point = CANDIDATES+n*0x88
            put(point+0x20, mode)
            xyz = positions[n % len(positions)] if positions else (float(n+1), float(n % 3), 0.)
            for axis, value in enumerate(xyz):
                put(point+0x24+axis*4, bits(value))
            put(point+0x30, frame, 2)
        put(FRAMES, 0x198b0000 if state != 'missing_object' else 0xffffffff)
        put(FRAMES+6, 0xffff if state == 'negative_node' else (1 if state == 'node_bound' else 0), 2)
        put(FRAMES+0xa, 2, 1)
        put(FRAMES+0xb, 1, 1)
        put(OBJECT_POOL+0x44, OBJECT_ROWS)
        put(OBJECT_ROWS+8, BODY)
        put(BODY+0x114, 52, 2)
        put(BODY+0x116, 0x200, 2)
        for offset, value in ((0, 1.), (4, 1.), (0x14, 1.), (0x24, 1.), (0x28, 1.)):
            put(BODY+0x200+offset, bits(value))
        output = OUTPUT+0xc if entry == 0x1bbec0 else OUTPUT
        put(output, mask)
        put(output+4, current[0], 2)
        put(output+6, current[1], 2)
        for address, value, width in changes:
            put(address, value, width)
        n = len(rows)
        gp = [0x5a010000+n*31+k*7 for k in range(8)]
        gp[4] = geometry.SP+n%4*4
        depth = (0, 1, 3, 6)[n % 4]
        banks = [b for b in row['banks'] if b['address'] not in (areas.BASE, seed['gp'][4]+4)]
        banks.append(dict(address=areas.BASE, before=raw.hex()))
        for address, value in ((0x505e78, FRAMES), (0x4ecbd0, OBJECT_POOL)):
            banks.append(dict(address=address, before=struct.pack('<I', value).hex()))
        for address, size in ((0x44aac8, 4), (0x46a0ec, 4), (0x46a0d0, 4),
                              (0x46a328, 8), (0x47d6c0, 4), (0x44acb0, 36)):
            banks.append(dict(address=address, before=image.read(address, size).hex()))
        row.update(id=n, name=name, entry=entry, gp=gp, actor=0xe3610000,
            selection=OUTPUT, matrix=0xe3610000, source=OUTPUT,
            banks=sorted(banks, key=lambda b:b['address']), active_mmx=False,
            before_fp=[0x7f | ((0, 2, 3)[n % 3] << 8) | ((n % 4) << 10),
                       ((n % 8) << 11) | ((n*0x100) & 0x4700) | (n*3 % 64),
                       0x1f80 | ((n % 4) << 13) | (0, 0x40, 0x8000, 0x8040)[n % 4]],
            live_x87=[struct.pack('<QH', 0x8000000000000000+(k << 58),
                         0x3fff+k % 3 | (0x8000 if k % 2 else 0)).hex() for k in range(depth)],
            input_timeline=list(timeline))
        rows.append(row)
    for path in ('default', 'order', 'pairs'):
        for mode in (0, 1):
            for mask in (0, 1, 3, 0xffffffff):
                make(path+'_'+str(mode)+'_'+hex(mask), path=path, mode=mode, mask=mask)
    for count in (0, 1, 2, 3, 31, 32, 33, 40):
        make('count_'+str(count), count=count)
    for entry in (0x1bbbd0, 0x1bbec0):
        for mask in (0, 1, 3, 0xffffffff):
            make('entry_'+hex(entry)+'_'+hex(mask), entry=entry, mask=mask)
    for frame in (0xffff, 0):
        for state in ('valid', 'missing_object', 'negative_node', 'node_bound'):
            make('frame_'+str(frame)+'_'+state, frame=frame, state=state)
    for positions in (((0., 0., 0.),), ((1.e-6, 0., 0.),), ((1., 0., 0.),),
                      ((-1., 0., 0.), (2., 0., 0.)), ((100., 0., 0.), (1., 1., 0.)),
                      ((float('nan'), 0., 0.), (1., 0., 0.))):
        make('geometry_'+str(positions), positions=positions)
    for current in ((0, 0), (0, 2), (1, 1), (0xffff, 0xffff)):
        make('reset_'+str(current), mask=0xffffffff, current=current)
    def publication(address, occurrence, target, value):
        return dict(on_read=dict(address=address, occurrence=occurrence),
                    writes=[dict(address=target, bytes=struct.pack('<I', value).hex())])
    make('fresh_selection_mask', timeline=(publication(OUTPUT, 1, OUTPUT, 2),))
    make('fresh_facing', timeline=(publication(areas.ACTORS+0x298, 1,
        areas.ACTORS+0x290, bits(-1.)),))
    make('fresh_sentinel', mask=0xffffffff, timeline=(publication(0x47d6c0, 1,
        0x47d6c0, 0x12345678),))
    make('fault_seed_timeline', timeline=(publication(OUTPUT, 1, OUTPUT, 2),))
    for n, options in enumerate((dict(), dict(frame=0), dict(mask=0xffffffff),
                                 dict(entry=0x1bbec0))):
        make('fault_seed_'+str(n), **options)
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--verify', type=Path)
    parser.add_argument('--complete-only', action='store_true')
    parser.add_argument('--callers', action='store_true')
    parser.add_argument('--transport', action='store_true',
        help='Observe real callback argument backing, return reads and interrupted caller packets')
    args = parser.parse_args()
    args.callers = args.callers or args.transport
    assert not args.output.exists()
    assert common.sha(NEW_DEFAULT) == NEW_PIN
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    image = Xbe(NEW_DEFAULT)
    common.ENTRIES = ENTRIES + ((0x1b0cd0,) if args.callers else ())
    metadata = common.metadata(image)
    code = _Code(image, '.text')
    # Capstone omits the read flag for TEST [selection_mask], bit. Both
    # compare/test memory operands are genuine source reads and never stores.
    for site in metadata['sites']:
        if code.at(site['instruction']).mnemonic in ('cmp', 'test'):
            for operand in site['operands']:
                operand.update(read=True, write=False)
        if args.transport and code.at(site['instruction']).mnemonic == 'ret':
            site['operands'].append(dict(base=4, index=-1, scale=1,
                displacement=0, bytes=4, read=True, write=False))
    owned = areas.prepare(image, metadata, scratch)
    command = ['c++', '-std=c++20', '-O2', str(ROOT/'tools/observe_native_campaign_ai_area_choice_tu15.cpp'), '-o', str(owned/'observer')]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    declared = inputs(image)
    if args.callers:
        wrappers = [r for r in declared if r['entry'] == 0x1bbec0]
        wrappers += [r for r in declared if r['name'] in
                     ('count_0', 'count_1', 'frame_0_valid', 'frame_0_missing_object',
                      'geometry_((0.0, 0.0, 0.0),)')]
        declared = wrappers
        for n, row in enumerate(declared):
            row.update(id=n, entry=0x1b0d1c, caller_call=True, stop_address=0x1b0d1e)
            row['gp'][0] = 0x1bbec0
            if args.transport:
                # Retain the real caller-owned argument extent. The actual
                # CALL still produces its return word; callee_entry captures
                # that genuine frame before the first callback instruction.
                frame=struct.pack('<III',0,row['matrix'],row['source'])
                row['banks'].append(dict(address=row['gp'][4]-4, before=frame.hex()))
                row['banks'].sort(key=lambda b:b['address'])
    complete = geometry.physical(owned, declared, 'complete')
    prefixes = []
    if not args.complete_only and (not args.callers or args.transport):
        seeds = ([r for r in complete if r['name'] in
                  ('fault_seed_3', 'count_0', 'frame_0_valid')]
                 if args.transport else [r for r in complete if r['name'].startswith('fault_seed')])
        for seed in seeds:
            for phase, events in (('before_read', seed['reads']), ('before_write', seed['writes']), ('after_write', seed['writes'])):
                for ordinal in range(len(events)):
                    row = copy.deepcopy(seed)
                    row.update(id=len(complete)+len(prefixes), name=seed['name']+'_'+phase+'_'+str(ordinal), fault=dict(phase=phase, ordinal=ordinal))
                    prefixes.append(row)
    observed = complete + geometry.physical(owned, prefixes, 'prefix') if prefixes else complete
    actual_faults = []
    if not args.complete_only and not args.callers:
        for address in (0x5020c8, 0x4ecc20, 0x505e78):
            row = copy.deepcopy(next(r for r in complete if r['name'] == 'frame_0_valid'))
            row.update(id=len(observed)+len(actual_faults), name='actual_missing_'+hex(address), expected_fault=True)
            for bank in row['banks']:
                if bank['address'] == address:
                    bank['before'] = struct.pack('<I', 0xdead0000).hex()
            actual_faults.append(row)
        row = copy.deepcopy(next(r for r in complete if r['entry'] == 0x1bbec0))
        row.update(id=len(observed)+len(actual_faults), name='actual_missing_selection',
                   expected_fault=True, selection=0xdead0000, source=0xdead0000)
        actual_faults.append(row)
        observed += geometry.physical(owned, actual_faults, 'actual-faults')
    reached = set().union(*(set(r['executed_sites']) for r in complete))
    roots = {s['instruction'] for s in metadata['sites'] if 0x1bbbd0 <= s['instruction'] < 0x1bbee0}
    fixture = dict(image_sha256=NEW_PIN, backend='physical-unchanged-i386-offline',
        owners=metadata['owners'], instruction_patches=[], supplied_original_outcomes=False, cases=observed)
    data = gzip.compress(json.dumps(fixture, separators=(',', ':')).encode(), mtime=0)
    if args.verify:
        assert data == args.verify.read_bytes(), 'Original area-choice repeat differs'
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    report = dict(passed=True, original_sha256=NEW_PIN, cases=len(observed), complete_calls=len(complete),
        fault_prefixes=len(prefixes), actual_faults=len(actual_faults), root_sites=len(roots), reached_root_sites=len(roots & reached),
        unreached_root_sites=[hex(p) for p in sorted(roots-reached)],
        reached_closure_sites=len(reached), fixture=str(args.output), fixture_bytes=len(data),
        actual_caller_packets=args.callers,
        actual_argument_and_return_transport=args.transport,
        fixture_sha256=common.sha(args.output), independent_repeat_identical=True if args.verify else None,
        native_implementation_qualified=False, actual_world_route_qualified=False,
        source_sha256={str(p.relative_to(ROOT)):common.sha(p) for p in
            (Path(__file__), ROOT/'tools/observe_native_campaign_ai_area_choice_tu15.cpp')},
        owned_scratch=str(scratch), retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('cases', 'complete_calls', 'root_sites', 'reached_root_sites', 'unreached_root_sites')}))


if __name__ == '__main__':
    main()
