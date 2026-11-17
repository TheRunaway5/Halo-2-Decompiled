#!/usr/bin/env python3
"""Physical unchanged-i386 route reconstruction and smoothing reference.

Enters 27AAA0 and 27A640 over independently constructed path-search rows,
navigation links, framed points and caller storage. Unported callees stop at
their real entries; no original instruction or callee outcome is replaced.
Continuation cases are separately labelled component entries with declared
caller frames. The original executable, observer and raw banks are private
HALO2_TEST_SCRATCH artifacts; the compressed fixture is reference evidence.
"""
import argparse
import copy
import gzip
import json
import os
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
import oracle_native_campaign_ai_area_perform_tu15 as reference
import oracle_native_campaign_ai_path_search_tu15 as search
import oracle_native_campaign_ai_route_obstacles_tu15 as obstacles

ENTRIES = (0x27a640, 0x27aaa0, 0x27c410, 0x30bd0, 0x218c40, 0x218b20,
           0x218780, 0x219030, 0x145070)
PREFIX_ENTRIES = ENTRIES
ENTRIES += (0x2cbee0,0x2cceb0,0x2cc5c0,0x2cc790,0x2cc8f0,0x2b3e0,
            0x2029e0,0x276090,0x2762a0,0x2761f0,0x2769c0,0x2768f0,0x2cbaf0,
            0x202970,0x186e30,0x27a310,0x32ccd8,0x33210c,0x32ccf5,0x332099,0x33212f,0x332122)
ENTRIES = tuple(dict.fromkeys(ENTRIES + obstacles.ENTRIES + (0x2ce030,0x2cbe30,0x2cbcf0,0x218960,0x145200,0x218fe0,0x2cdeb0,0x2cd780,0x2cdbc0,0x2cd870,0x2cd500,0x2cd3d0,0x2cd440,0x2cdb40,0x2cbc10,0x2caac0,0x2cba80,0x279100)))
OWNED = (0x27a640, 0x27aaa0)
STOP_ENTRIES = (0x27a110, 0x1b0e10)
BASE, W, WORLD = search.BASE, search.W, search.WORLD
OUT = BASE + 0x28000
GOAL, POINTS, LINKS, FLAG, COUNT = OUT, OUT + 0x200, OUT + 0x1200, OUT + 0x1800, OUT + 0x1804
POOL, ACTORS, CLOCK = OUT + 0x1900, OUT + 0x1a00, OUT + 0x2300
ACTOR, NONE, SP, STOP = 0xe3610000, 0xffffffff, reference.SP, reference.STOP
FP_PROFILES = search.FP_PROFILES
f = search.f


def put(data, at, value, size=4):
    data[at:at + size] = (value & ((1 << (8 * size)) - 1)).to_bytes(size, 'little')


def point(data, at, xyz, frame=0xffff, upper=0x1234):
    for k, value in enumerate(xyz):
        put(data, at + k * 4, f(value))
    put(data, at + 12, (upper << 16) | frame)


def scene(length=3, *, frame=0xffff, start_frame=0xffff, portal=0, kind=-1,
          edge_mode='valid', position=1.5, size=4.0):
    """A retained search chain: row parent/depth are independent inputs.

    This is not a synthetic result from 27B2B0: direct 27AAA0 cases specify
    that owner's preexisting workspace contract, as the original callers do.
    """
    world, _ = search.grid(4, 2, size=size, portal=portal)
    world.link(0, 2, words=(0, 0, 0, 1))
    ws = search.Workspace(nodes=max(length, 1), heap=0)
    ws.raw.extend(bytes(12));ws.put(0x140d0,NONE);ws.put(8,0);ws.put(12,NONE)
    ws.point(0x14, (0.0, 0.0, 0.0), start_frame, upper=0x5678)
    ws.goal(max(length - 1, 0), (position * length, 0.5, 0.25), frame)
    ws.raw[0x120b6:0x140b6] = b'\xff' * 0x2000
    for n in range(length):
        at = 0xb0 + n * 0x44
        ws.put(at, 0x8000 | n, 2)
        ws.put(at + 2, n - 1, 2)
        ws.put(at + 8, n)
        ws.put(at + 0x10, kind if n else -1, 2)
        edge = n % len(world.edges)
        if edge_mode == 'negative':
            edge = NONE
        elif edge_mode == 'outside':
            edge = len(world.edges)
        elif edge_mode == 'ffff':
            edge = 0xffff
        ws.put(at + 0x14, 0 if kind != -1 else edge)
        ws.point(at + 0x18, (position * n, 0.5, 0.25), frame, upper=0x7777)
        ws.put(at + 0x3e, n, 2)
        ws.put(0x120b6 + ((n & 0x1ff) << 4), n, 2)
    # 27AAA0 reads NAV+8 as the edge bound; World.pack deliberately keeps
    # that field separate from the search graph's sector/link counts.
    world.put(search.NAV + 8, len(world.edges))
    world.put(search.DEVROOT + 0xc4, 1)
    world.put(search.DEVROOT + 0xc8, search.NAV)
    out = bytearray([0xa5] * 0x2600)
    point(out, 4, (position * length, 0.5, 0.25), frame, upper=0x1357)
    put(out, POOL - OUT + 0x44, ACTORS)
    put(out, ACTORS - OUT + 0x478, 0, 1)
    put(out, ACTORS - OUT + 0x18, 0)
    put(out, ACTORS - OUT + 0x26c, NONE)
    for offset in (0x290,0x294,0x298):put(out, ACTORS-OUT+offset,0)
    # Coherent empty spatial cluster for newly admitted native smoothing.
    put(out,0x2400,NONE);put(out,0x2420+0x8c,0)
    world.put(search.DEVROOT+0xa0,OUT+0x2420)
    world.put(search.BODY+0x28,0);world.put(search.BODY+0x2c,0)
    world.put(search.BODY+0xaa,0,1)
    put(out, CLOCK - OUT + 2, 30, 2)
    return ws, world, out


def make(image, rows, name, entry, ws, world, out, args, *, regs=None, fp=None, extra=()):
    spatial=bytearray(0x100);put(spatial,0xb0,OUT+0x2400);put(spatial,0xcc,5)
    traversal=bytearray(0x100);put(traversal,0xe4,8)
    constants = ((0x4eab00,spatial),(0x4f3c00,traversal),
                 (0x46a1f8, image.read(0x46a1f8, 4)), (0x46a8c8, image.read(0x46a8c8, 4)),
                 (0x5020c8, struct.pack('<I', POOL)), (0x51df54, struct.pack('<I', CLOCK)),
                 *((at,image.read(at,size)) for at,size in ((0x46a8b0,8),(0x46a6e4,4),
                    (0x4502c4,4),(0x46a6a0,8),(0x46a698,8),(0x46a430,8),(0x46a2c0,8),(0x46a838,4),
                    (0x44923c,4))), (0x474cdc,struct.pack('<I',0x44abec)), (0x48d4f4,struct.pack('<I',1)))
    search.make_case(image, rows, name, entry, ws, world, fp=fp,
                     extra=((OUT, out),) + constants + tuple(extra), max_steps=100000)
    row = rows[-1]
    # Shared semantic geometry consumes the original read-only numeric page.
    # Keep one coherent bank, including collector speed thresholds.
    row['banks']=[b for b in row['banks'] if not 0x46a000<=b['address']<0x46b000]
    row['banks'].append(dict(address=0x46a000,before=image.read(0x46a000,0x1000).hex()))
    row['banks'].sort(key=lambda b:b['address'])
    row['matrix'], row['source'], row['stack_arguments'] = args[0], args[1], list(args[2:])
    row['gp'][3 if entry == 0x27aaa0 else 0] = W
    for k, value in (regs or {}).items():
        row['gp'][k] = value
    row['stop_before_entries'] = list(STOP_ENTRIES)
    row['pending_extended_sites'] = [0x27aa26, 0x27aa5f, 0x27a8d1]
    return row


def reconstruction_cases(image, rows):
    for length in (1, 2, 3, 11, 40, 45):
        for capacity in (0, 1, 2, 10, 40):
            ws, world, out = scene(length)
            make(image, rows, f'reconstruct_n{length}_cap{capacity}', 0x27aaa0, ws, world, out,
                 (ACTOR, GOAL, length - 1, capacity, POINTS, FLAG, COUNT, LINKS, 10))
    for length in (1, 3, 12):
        for limit in (0, 1, 2, 10):
            for portal in (0, 0x80):
                ws, world, out = scene(length, portal=portal)
                make(image, rows, f'links_n{length}_limit{limit}_portal{portal}', 0x27aaa0, ws, world, out,
                     (NONE, GOAL, length - 1, 40, POINTS, FLAG, COUNT, LINKS, limit))
    for kind in (1, 2, 5, 6):
        for mode in (0, 1):
            ws, world, out = scene(kind=kind)
            world.links[0]['words'] = (0, 0, 0, mode)
            make(image, rows, f'kind{kind}_worda{mode}', 0x27aaa0, ws, world, out,
                 (ACTOR, GOAL, 2, 40, POINTS, FLAG, COUNT, LINKS, 10))
    for frame, start in ((0xffff, 0xffff), (0, 0), (0, 0xffff), (0xffff, 0), (3, 0), (1, 0), (2, 1)):
        for position in (-8.0, 0.0, 0.25, 2.0, 20.0):
            ws, world, out = scene(frame=frame, start_frame=start, position=position)
            make(image, rows, f'framed_{frame:x}_{start:x}_x{position}', 0x27aaa0, ws, world, out,
                 (NONE, GOAL, 2, 40, POINTS, FLAG, COUNT, LINKS, 10))
    for mode in ('negative', 'outside', 'ffff'):
        ws, world, out = scene(edge_mode=mode)
        make(image, rows, f'edge_{mode}', 0x27aaa0, ws, world, out,
             (ACTOR, GOAL, 2, 40, POINTS, FLAG, COUNT, LINKS, 10))
    for length in (1, 2, 3, 11):
        for position in (40.0, 40.0001, 1000.0):
            ws, world, out = scene(length)
            ws.point(0x14, (-position, 0.0, 0.0), 0xffff, upper=0x5678)
            make(image, rows, f'distance_truncation_n{length}_position{position}', 0x27aaa0, ws, world, out,
                 (NONE, GOAL, length - 1, 40, POINTS, FLAG, COUNT, LINKS, 10))
    for flag, count in ((0, 0), (0, COUNT), (FLAG, 0)):
        ws, world, out = scene()
        make(image, rows, f'optional_{bool(flag)}_{bool(count)}', 0x27aaa0, ws, world, out,
             (ACTOR, GOAL, 2, 40, POINTS, flag, count, LINKS, 10))
    ws, world, out = scene(0)
    ws.put(0x6c, 0)
    make(image, rows, 'reconstruct_no_node', 0x27aaa0, ws, world, out,
         (NONE, GOAL, NONE, 40, POINTS, FLAG, COUNT, LINKS, 10))
    for limit, portal in ((0, 0), (10, 0x80)):
        ws, world, out = scene(1, portal=portal)
        ws.put(0xb0 + 0x3e, 0xffff, 2)
        make(image, rows, f'depth_ffff_limit{limit}_portal{portal}', 0x27aaa0, ws, world, out,
             (NONE, GOAL, 0, 40, POINTS, FLAG, COUNT, LINKS, limit))
    for size in (0.0, 0.125, 1.0, 16.0):
        for profile in FP_PROFILES:
            ws, world, out = scene(size=size, position=2.25)
            make(image, rows, f'geometry_size{size}_fp{profile}', 0x27aaa0, ws, world, out,
                 (ACTOR, GOAL, 2, 40, POINTS, FLAG, COUNT, LINKS, 10), fp=profile)


def route_cases(image, rows):
    for actor in (NONE, ACTOR):
        for available in (0, 1):
            for found in (False, True):
                for distance in (0.0, 0.25, 1.0, float('nan')):
                    ws, world, out = scene()
                    ws.put(0x54, available, 1)
                    if not found:
                        ws.raw[0x120b6:0x140b6] = b'\xff' * 0x2000
                    ws.put(0x90, 1, 2)
                    ws.putf(0x94, distance)
                    ws.point(0x9c, (2.0, 1.0, 0.0), 0xffff)
                    make(image, rows, f'route_actor{actor:x}_available{available}_found{found}_d{distance}',
                         0x27a640, ws, world, out, (actor, GOAL))
    for kind in (1, 2, 5, 6):
        ws, world, out = scene(kind=kind)
        make(image, rows, f'route_kind{kind}', 0x27a640, ws, world, out, (ACTOR, GOAL))


def paired_link_cases(image, rows):
    """Coherent authored link records, separate from historical node-kind inputs.

    The original ten named-boundary cases intentionally retain their existing
    kind2 link bytes. These cases provide actual paired kind1/6 link records.
    """
    for kind in (1, 6):
        for frame in (0xffff, 0):
            for profile in FP_PROFILES:
                for entry in (0x27aaa0, 0x27a640):
                    ws, world, out = scene(length=2, kind=kind, frame=0xffff, start_frame=frame)
                    ws.point(0xb0+0x18, (0.0, 0.5, 0.25), frame)
                    link=world.links[0]
                    link.update(kind=kind,
                        words=tuple(world.vertex(*p) for p in ((1.,0.,.25),(1.,4.,.25),(4.,0.,.25),(4.,4.,.25))),
                        target=1,frames=(frame,0xffff))
                    args=(ACTOR,GOAL,1,40,POINTS,FLAG,COUNT,LINKS,10) if entry==0x27aaa0 else (ACTOR,GOAL)
                    row=make(image,rows,f'paired_kind{kind}_frame{frame:x}_{entry:x}_fp{profile}',entry,ws,world,out,args,fp=profile)
                    row.update(compare_reads=False,retained_special_link=True)


def continuation_cases(image, rows):
    """Independent component inputs after the two currently open callees.

    The saved frame is input storage, never an injected callee outcome in
    a whole original call. These cases cannot qualify 2CBEE0 or 2CE030.
    """
    def run(entry, name, *, result=1, actor=NONE, actor_copy=False, count=2,
            complete=1, link_count=3, retry=0, kind=2, object_=NONE,
            frame=0xffff, start=0xffff, policy=0, shift=1, fp=None):
        ws, world, out = scene(frame=frame, start_frame=start)
        ws.put(0xac, policy, 1)
        put(out, 0, 0, 1)
        put(out, 0xa8, retry, 2)
        put(out, 0xaa, shift, 2)
        put(out, ACTORS - OUT + 0x478, int(actor_copy), 1)
        # Genuine 27A640 frame layout, including saved nonvolatile words
        # and the original caller return/arguments above the local frame.
        low = SP - (len(rows) % 4) * 4 - 0x5bc
        raw = bytearray(search.history(low, 0x5cc, len(rows)))
        for k, value in enumerate((0x71511111, 0x71522222, 0x71533333, 0x71544444)):
            put(raw, k * 4, value)
        put(raw, 0x12, complete, 1)
        put(raw, 0x13, result, 1)
        put(raw, 0x14, min(count, 4))
        put(raw, 0x18, ACTORS if actor != NONE else 0)
        put(raw, 0x1c, object_)
        put(raw, 0x20, kind)
        put(raw, 0x24, count)
        put(raw, 0x28, link_count)
        for n in range(10):
            put(raw, 0x2c + n * 8, 0x62000000 + n)
            put(raw, 0x30 + n * 8, 0x73000000 + n)
        for base in (0x7c, 0xec):
            for n in range(4):
                put(raw, base + n * 28, n, 2)
                put(raw, base + n * 28 + 4, 0x51000000 + n)
                point(raw, base + n * 28 + 12, (n + 1.25, -0.5 * n, 0.25), frame)
        for offset, value in ((0x5bc, STOP), (0x5c0, actor), (0x5c4, OUT)):
            put(raw, offset, value)
        row = make(image, rows, name, entry, ws, world, out, (0x71522222, 0x71533333, 0x71544444),
                   regs={0: 0x61100000 | result, 3: W, 4: low, 5: OUT, 6: actor, 7: NONE}, fp=fp,
                   extra=((low, raw),))
        row['return_pc'] = 0x71511111
        row['component_entry'] = True
        row['declared_caller_frame'] = low
        row['implicit_saved_words'] = [low, low + 4, low + 8, low + 12]
    for count in (0, 1, 2, 4, 5):
        for complete in (0, 1):
            for links in (-1, 0, 1, 2, 3, 10):
                run(0x27a7a7, f'component_copy_count{count}_complete{complete}_links{links}',
                    actor=ACTOR, actor_copy=True, count=count, complete=complete, link_count=links)
    for policy in (0, 1):
        for retry in (0, 1, 2, 3, 31, 32, 33):
            for shift in (0, 1, 3):
                run(0x27a7a7, f'component_smooth_policy{policy}_retry{retry}_shift{shift}',
                    policy=policy, retry=retry, shift=shift)
    for result in (0, 1):
        for retry in (0, 1, 32, 33):
            for count in (0, 1, 4):
                run(0x27a887, f'component_tail_result{result}_retry{retry}_count{count}',
                    result=result, retry=retry, count=count)
    for kind in (1, 2, 5, 6):
        for actor in (NONE, ACTOR):
            run(0x27a887, f'component_notify_kind{kind}_actor{actor:x}', kind=kind, actor=actor, object_=0x12340002)
    for frame, start in ((0xffff, 0), (0, 0xffff), (1, 0), (2, 1), (3, 0)):
        for count in (0, 4):
            for profile in FP_PROFILES:
                run(0x27a887, f'component_distance_frame{frame:x}_{start:x}_count{count}_fp{profile}',
                    frame=frame, start=start, count=count, fp=profile)
    # Independent arithmetic boundaries: x86 masks a shift count by 31,
    # and retry 31 increments to 32 without taking the >32 reset.
    run(0x27a7a7, 'component_retry_shift16', policy=0, retry=1, shift=16)
    run(0x27a887, 'component_retry_increment32', result=0, retry=31, count=2)


def inputs(image):
    rows = []
    reconstruction_cases(image, rows)
    route_cases(image, rows)
    for original in list(rows):
        if original['name'] not in ('reconstruct_n3_cap40', 'reconstruct_n11_cap40', 'kind2_worda1',
                                    'links_n3_limit0_portal0', 'links_n3_limit10_portal128',
                                    'depth_ffff_limit0_portal0', 'depth_ffff_limit10_portal128',
                                    'route_actorffffffff_available0_foundTrue_d0.0',
                                    'route_actorffffffff_available1_foundTrue_d0.0'):
            continue
        row = copy.deepcopy(original)
        row['id'], row['name'] = len(rows), 'declared_frame_' + original['name']
        size = 0xf0 if row['entry'] == 0x27aaa0 else 0x5bc
        low = row['gp'][4] - size
        row['banks'].append(dict(address=low, before=search.history(low, size + 0x28, row['id']).hex()))
        row['declared_caller_frame'] = low
        row['implicit_saved_words'] = [low + n * 4 for n in range(3 if row['entry'] == 0x27aaa0 else 4)]
        rows.append(row)
    continuation_cases(image, rows)
    paired_link_cases(image, rows)
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    parser.add_argument('--verify', type=Path)
    parser.add_argument('--select', help='Diagnostic only: case-name regular expression')
    args = parser.parse_args()
    assert not args.output.exists()
    assert reference.sha(NEW_DEFAULT) == NEW_PIN
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    image = Xbe(NEW_DEFAULT)
    meta = reference.metadata(image, ENTRIES)
    for site in meta['sites']:
        if site['bytes'].startswith('0f17'):site['operands'][0].update(read=False,write=True)
    owned = reference.prepare(image, meta, scratch)
    selected = inputs(image)
    if args.select:
        import re
        selected = [r for r in selected if re.search(args.select, r['name'])]
        for n, r in enumerate(selected):
            r['id'] = n
    rows = search.physical(owned, selected, 'complete')
    for row in rows:
        assert row['completed'], row['name']
        if 0x2cabe0 in row['executed_sites']:
            row.update(compare_reads=False,semantic_child_stores=True)
        if 0x279100 in row['executed_sites']:
            row.update(compare_reads=False,retained_special_link=True)
    # Retain the already-qualified parent/helper fault-site cohort. New
    # segment children have separate whole-call and focused observer proofs;
    # their interrupted extended quantities are not implied by this refresh.
    from generate_physics_live_support_tu15 import complete_body
    from oracle_smg_firing_1_1 import _Code
    code = _Code(image, '.text')
    prefix_sites = set().union(*(set(complete_body(code,e).code) for e in PREFIX_ENTRIES))
    link_sites = set(complete_body(code,0x279100).code)
    indexed = {row['name']: row for row in rows}
    prefixes = []
    for prefix in search.select_prefixes(rows):
        source_name = prefix['name'].rsplit('_' + prefix['fault']['phase'] + '_',1)[0]
        original = indexed[source_name]
        events = original['reads' if prefix['fault']['phase']=='before_read' else 'writes']
        if prefix['fault']['phase']=='before_read' and not original.get('compare_reads',True):
            # Keep the established parent read-prefix proof before entering
            # the new retained-DATA child; do not claim its read sequence.
            first_link_read=next((i for i,event in enumerate(original['reads']) if event['instruction'] in link_sites),-1)
            if original.get('retained_special_link') and prefix['fault']['ordinal']<first_link_read:
                prefix['compare_reads']=True
            else:continue
        if events[prefix['fault']['ordinal']]['instruction'] in prefix_sites:
            prefixes.append(prefix)
    for n, prefix in enumerate(prefixes):
        prefix['id'] = len(rows) + n
    rows += search.physical(owned, prefixes, 'prefix')
    reached = set().union(*(set(row['executed_sites']) for row in rows))
    from generate_physics_live_support_tu15 import complete_body
    from oracle_smg_firing_1_1 import _Code
    code = _Code(image, '.text')
    coverage, unreached = {}, {}
    for entry in ENTRIES:
        body = set(complete_body(code, entry).code)
        coverage[hex(entry)] = dict(reached=len(body & reached), total=len(body))
        if entry in OWNED:
            unreached[hex(entry)] = sorted(hex(s) for s in body - reached)
    stops = {}
    for row in rows:
        if 'fault' not in row and row.get('stopped_at', STOP) != STOP:
            stops[hex(row['stopped_at'])] = stops.get(hex(row['stopped_at']), 0) + 1
    movs = sorted(s['instruction'] for s in meta['sites'] if s['bytes'] in ('a4', 'a5', 'f3a4', 'f3a5'))
    fixture = dict(image_sha256=NEW_PIN, backend='physical-unchanged-i386-offline', owners=meta['owners'],
                   instruction_patches=[], supplied_original_outcomes=False, stop_entries=list(STOP_ENTRIES),
                   movs_sites=movs, bank_encoding='sparse-runs-v1', write_encoding='rep-runs-v1',
                   cases=reference.compact(rows))
    data = gzip.compress(json.dumps(fixture, separators=(',', ':')).encode(), mtime=0)
    if args.verify:
        assert data == args.verify.read_bytes(), 'Original route repeat differs'
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    whole = [r for r in rows if 'fault' not in r]
    report = dict(passed=not args.select, selection=args.select, original_sha256=NEW_PIN, cases=len(rows),
                  whole=len(whole), prefixes=len(rows) - len(whole),
                  by_entry={hex(e): sum(r['entry'] == e for r in whole) for e in (*OWNED, 0x27a7a7, 0x27a887)},
                  component_cases=sum(bool(r.get('component_entry')) for r in whole), named_refusals=stops,
                  reads=sum(len(r['reads']) for r in rows), stores=sum(len(r['writes']) for r in rows),
                  coverage=coverage, unreached_owned_sites=unreached,
                  original_instructions=sum(r['original_instructions'] for r in rows),
                  fixture=str(args.output), fixture_bytes=len(data), fixture_sha256=reference.sha(args.output),
                  independent_repeat_identical=True if args.verify else None,
                  source_sha256={str(p.relative_to(reference.ROOT)): reference.sha(p) for p in
                                 (Path(__file__), Path(reference.__file__), Path(search.__file__),
                                  reference.ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp')},
                  owned_scratch=str(scratch), retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: v for k, v in report.items() if k not in ('coverage', 'unreached_owned_sites')}))


if __name__ == '__main__':
    main()
