#!/usr/bin/env python3
"""Physical unchanged-i386 segment reduction over independent route rows.

No original instructions or child outcomes are patched. The executable,
observer and raw banks are private HALO2_TEST_SCRATCH artifacts. Whole-entry
cases declare only the original ABI, navigation inputs and caller storage.
"""
import argparse
import gzip
import json
import os
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_area_perform_tu15 as reference
import oracle_native_campaign_ai_path_search_tu15 as search
import oracle_native_campaign_ai_route_tu15 as route
import oracle_native_campaign_ai_route_traversal_tu15 as traversal

OWNED = (0x2cbee0, 0x2cceb0, 0x2cc5c0, 0x2cc790, 0x2cc8f0, 0x2b3e0)
ENTRIES = tuple(dict.fromkeys(OWNED + traversal.ENTRIES + (0x218780, 0x219030, 0x145070,
    0x32ccd8, 0x33210c, 0x32ccf5, 0x332099, 0x33212f, 0x332122)))
STOP_ENTRIES = ()
DATA = search.BASE + 0x28000
ROWS, OUTPUT, COUNT, FLAG = DATA, DATA + 0x1000, DATA + 0x2000, DATA + 0x2004


def inputs(image):
    rows = []

    def make(name, count=3, kind=2, nav_count=1, nav=True, zero=False, frame=0xffff,
             start_frame=0xffff, position=1.0, profile=None, declared=False, flags=0x10,
             portal=0, start=(0.,0.,0.), target_y=0.):
        world, _ = search.grid(4, 2, size=4.0, flags=flags, portal=portal)
        world.put(search.NAV + 8, len(world.edges))
        world.put(search.DEVROOT + 0xc4, nav_count)
        world.put(search.DEVROOT + 0xc8, search.NAV if nav else 0)
        ws = search.Workspace(nodes=0, heap=0)
        ws.point(0x14, start, start_frame, upper=0x6a27)
        data = bytearray([0xa5] * 0x2010)
        for n in range(max(count, 1)):
            at = n * 28
            route.put(data, at, kind, 2)
            route.put(data, at + 2, 0x200 + n, 2)
            route.put(data, at + 4, n)
            route.put(data, at + 8, 0x100 + n, 2)
            route.point(data, at + 12, (0. if zero else (n + 1) * position, target_y, 0.), frame, upper=0x9c12)
        extra = ((DATA, data), (0x46a8b0, image.read(0x46a8b0, 8)),
                 (0x46a6e4, image.read(0x46a6e4, 4)),
                 *((at,image.read(at,size)) for at,size in ((0x4502c4,4),(0x46a6a0,8),
                    (0x46a698,8),(0x46a430,8),(0x46a2c0,8),(0x46a838,4))),
                 (0x474cdc,struct.pack('<I',DATA+0x1800)), (0x44923c,image.read(0x44923c,4)),
                 (0x48d4f4,struct.pack('<I',1)))
        route.point(data,0x1800,(1.,0.,0.))
        search.make_case(image, rows, name, 0x2cbee0, ws, world, fp=profile, extra=extra)
        row = rows[-1]
        row.update(matrix=search.W, source=count & 0xffffffff,
                   stack_arguments=[ROWS, COUNT, OUTPUT, FLAG],
                   stop_before_entries=list(STOP_ENTRIES))
        if declared:
            bp = row['gp'][4] - 4
            low = (bp & ~7) - 0x118
            # CALL's arguments and all private owner frames remain outside
            # this declared frame; the entire owned 2CBEE0 frame is checked.
            row['banks'].append(dict(address=low, before=search.history(low, bp + 4 - low, row['id']).hex()))
            row['implicit_saved_words'] = [low, low + 4, low + 8, bp]
            row['declared_caller_frame'] = low

    for profile in search.FP_PROFILES:
        for count in (-1, 0, 1, 2, 3, 4, 6):
            for declared in (False, True):
                make(f'kind_count{count}_fp{profile}_frame{declared}', count=count,
                     profile=profile, declared=declared)
        for count in (0, 1, -1):
            make(f'absent_count{count}_fp{profile}', nav_count=count, profile=profile, declared=True)
        make(f'absent_pointer_fp{profile}', nav=False, profile=profile, declared=True)
        for count in (2, 3, 6):
            make(f'zero_count{count}_fp{profile}', count=count, kind=-1, zero=True,
                 profile=profile, declared=True)
        for position in (.01, 1., 4., 10., 100.):
            make(f'distance_{position}_fp{profile}', position=position, profile=profile, declared=True)
    for frame, start in ((0xffff, 0), (0, 0xffff), (0, 0), (1, 0), (2, 1), (3, 0)):
        make(f'frames_{frame}_{start}', frame=frame, start_frame=start, declared=True)
    for profile in search.FP_PROFILES:
        for flags in (1, 0x10, 0x11):
            for position in (.125, 1., 3., 5., 10.):
                make(f'composed_flags{flags}_position{position}_fp{profile}', kind=-1,
                     position=position, profile=profile, flags=flags, declared=True)
        for position in (1.,3.,5.,10.,20.):
            for target_y in (1.,3.,6.,10.):
                make(f'corner_path_position{position}_y{target_y}_fp{profile}',kind=-1,
                     position=position,profile=profile,flags=0x11,portal=0x40,
                     start=(1.,1.,0.),target_y=target_y,declared=True)
    for profile in search.FP_PROFILES:
        for side in (0, 1):
            for width in (0., .125, .5, 4.):
                for position in ((2., 2.), (-1., 2.), (5., 2.)):
                    world, _ = search.grid(2, 2, size=4.)
                    ws = search.Workspace(nodes=0, heap=0)
                    data = bytearray([0xa5] * 0x2010)
                    route.point(data, 0, (*position, 0.))
                    search.make_case(image, rows, f'corner_s{side}_r{width}_p{position}_fp{profile}',
                                     0x2cceb0, ws, world, fp=profile, extra=((DATA, data),))
                    row = rows[-1]
                    row.update(matrix=search.NAV, source=DATA,
                               stack_arguments=[search.f(width), 0, side, OUTPUT],
                               stop_before_entries=list(STOP_ENTRIES))
                    low = row['gp'][4] - 0x38
                    row['banks'].append(dict(address=low, before=search.history(low, 0x54, row['id']).hex()))
                    row['implicit_saved_words'] = [low + n * 4 for n in range(4)]
                    row['declared_caller_frame'] = low
    for profile in search.FP_PROFILES:
        for side in (0,1):
            for mode in ('linked','device_open','device_closed'):
                for portal in (0x40,0x80):
                    world,_=search.grid(2,2,size=4.,flags=1 if mode=='linked' else 3,portal=portal,links=True)
                    if mode!='linked':
                        for sector in range(len(world.sectors)):
                            world.link(sector,7,raw4=2,raw8=0xffffffff)
                        world.put(search.DEVTABLE+2*8+4,8,1)
                        world.put(search.DEVTABLE+2*8+5,3,1)
                        world.put(search.DEVBITS+33,8 if mode=='device_open' else 0)
                    ws=search.Workspace(nodes=0,heap=0);data=bytearray([0xa5]*0x2010)
                    route.point(data,0,(2.,2.,0.))
                    search.make_case(image,rows,f'corner_link_s{side}_{mode}_portal{portal}_fp{profile}',0x2cceb0,ws,world,fp=profile,extra=((DATA,data),))
                    row=rows[-1];row.update(matrix=search.NAV,source=DATA,
                        stack_arguments=[search.f(.25),0,side,OUTPUT],stop_before_entries=[])
                    low=row['gp'][4]-0x38
                    row['banks'].append(dict(address=low,before=search.history(low,0x54,row['id']).hex()))
                    row['implicit_saved_words']=[low+n*4 for n in range(4)]
                    row['declared_caller_frame']=low
    for profile in search.FP_PROFILES:
        for entry in (0x2cc5c0, 0x2cc790, 0x2cc8f0):
            for radius in (0., .25, 1., 3.):
                for shape in (0, 1, 2, 3, 4):
                    world, _ = search.grid(2, 2, size=4.)
                    ws = search.Workspace(nodes=0, heap=0)
                    data=bytearray([0xa5]*0x2010)
                    layouts=( ((0.,0.),(2.,0.),(0.,1.),(0.,-1.),(-1.,0.)),
                              ((0.,0.),(0.,0.),(0.,0.),(0.,0.),(0.,0.)),
                              ((-2.,1.),(3.,2.),(0.,2.),(0.,-2.),(-1.,1.)),
                              ((0.,0.),(4.,0.),(1.,1.),(2.,1.),(-1.,0.)),
                              ((3.,2.),(-2.,1.),(0.,-2.),(0.,2.),(1.,1.)))
                    for n,xy in enumerate(layouts[shape]):route.point(data,n*0x40,(*xy,0.))
                    route.point(data,0x1800,(1.,0.,0.))
                    extra=((DATA,data),(0x474cdc,struct.pack('<I',DATA+0x1800)),
                           (0x44923c,image.read(0x44923c,4)),(0x48d4f4,struct.pack('<I',1)))
                    search.make_case(image,rows,f'helper_{entry:x}_radius{radius}_shape{shape}_fp{profile}',entry,ws,world,fp=profile,extra=extra)
                    row=rows[-1];row['gp'][0]=DATA;row['gp'][1]=DATA+0x40;row['gp'][2]=OUTPUT
                    row['xmm'][6][0]=row['xmm'][7][0]=search.f(radius)
                    row.update(matrix=shape%2 if entry==0x2cc5c0 else DATA+0x100,source=0,stack_arguments=[],stop_before_entries=[])
                    size=0x18 if entry==0x2cc5c0 else 0xc if entry==0x2cc790 else 0x38
                    if entry==0x2cc8f0:row['gp'][3]=DATA+0x80;row['gp'][6]=DATA+0x40;row['gp'][7]=DATA+0xc0
                    low=row['gp'][4]-size
                    row['banks'].append(dict(address=low,before=search.history(low,size+0xc,row['id']).hex()))
                    row['implicit_saved_words']=[] if entry==0x2cc5c0 else [low]
                    row['declared_caller_frame']=low
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    parser.add_argument('--select')
    args = parser.parse_args()
    assert not args.output.exists()
    assert reference.sha(NEW_DEFAULT) == NEW_PIN
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    image = Xbe(NEW_DEFAULT)
    meta = reference.metadata(image, ENTRIES)
    owned = reference.prepare(image, meta, scratch)
    selected = inputs(image)
    if args.select:
        import re
        selected = [r for r in selected if re.search(args.select, r['name'])]
        for n, row in enumerate(selected):
            row['id'] = n
    rows = search.physical(owned, selected, 'complete')
    for row in rows:
        assert row['completed'], row['name']
    reached = set().union(*(set(row['executed_sites']) for row in rows))
    code = _Code(image, '.text')
    coverage = {hex(e): dict(reached=len(set(complete_body(code, e).code) & reached),
                            total=len(complete_body(code, e).code)) for e in ENTRIES}
    fixture = dict(image_sha256=NEW_PIN, backend='physical-unchanged-i386-offline', owners=meta['owners'],
                   instruction_patches=[], supplied_original_outcomes=False,
                   stop_entries=list(STOP_ENTRIES), bank_encoding='sparse-runs-v1',
                   write_encoding='rep-runs-v1', cases=reference.compact(rows))
    data = gzip.compress(json.dumps(fixture, separators=(',', ':')).encode(), mtime=0)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    report = dict(original_sha256=NEW_PIN, cases=len(rows), coverage=coverage,
                  reads=sum(len(r['reads']) for r in rows), stores=sum(len(r['writes']) for r in rows),
                  named_refusals={hex(e): sum(r.get('stopped_at') == e for r in rows) for e in STOP_ENTRIES},
                  fixture=str(args.output), fixture_bytes=len(data), fixture_sha256=reference.sha(args.output),
                  source_sha256={str(p.relative_to(reference.ROOT)): reference.sha(p) for p in
                                 (Path(__file__), Path(reference.__file__), Path(search.__file__))},
                  owned_scratch=str(scratch), retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
