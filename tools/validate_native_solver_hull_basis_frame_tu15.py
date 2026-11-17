#!/usr/bin/env python3
"""Private full actual313790 original-hardware/callee differential."""
import argparse
import hashlib
import json
import os
import platform
import re
import struct
import subprocess
from pathlib import Path

from generated_routine_ownership import function_bodies
from research_mp_sandbox_update_functions import Xbe
from validate_native_solver_hull_edge_frame_tu15 import inputs as edge_inputs
from validate_native_tu15_prt_helpers import PIN, physical_original

ROOT = Path(__file__).resolve().parents[1]
PRIMARY = ROOT / 'src/physics/physics_live_support_tu15_generated.cpp'
BODY_PIN = '929233cfbc4586134b4d9b0602b4b3738cfa21228f6ac73afc265dedf268f59b'
ENTRY = 0x313790
CHILDREN = {0x313470: 3, 0x313390: 2, 0x313290: 1, 0x1f6610: 2, 0x2e8d20: 2, 0x306790: 2}
DEPTH = {0x3137b9: 1, 0x3137bf: 2, 0x3137c0: 2, 0x3137c4: 2, 0x3137d4: 3,
         0x3137ee: 1, 0x313808: 1, 0x31381c: 1, 0x313824: 1, 0x313828: 1, 0x31394d: 1}
VECTOR_STORES = {0x3137ac, 0x313870, 0x3138d5, 0x3138e8, 0x313927,
                 0x313494, 0x313539, 0x31353c, 0x313540,
                 0x31341d, 0x313461, 0x3132b8, 0x3132ca, 0x3132e1,
                 0x2e8d2c, 0x30679d, 0x3067bf}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source():
    body = next(body for sig, body in function_bodies(PRIMARY.read_text()) if sig.symbol == '313790')
    if hashlib.sha256(body.encode()).hexdigest() != BODY_PIN:
        raise ValueError('Complete 148-site hull basis body drift')
    sites, io, explicit, pops = set(), [], set(), set()
    for row in body.splitlines():
        marker = re.search(r'services.instruction\(0x([0-9a-f]+)u\)', row)
        if marker is None:
            continue
        pc = int(marker[1], 16)
        sites.add(pc)
        if re.search(r'\b(?:r|v)\(s.memory', row):
            explicit.add(pc)
        if 's.pop()' in row:
            pops.add(pc)
        if pc in explicit | pops or 's.push(' in row or re.search(r'\b(?:w|vw)\(s.memory', row):
            io.append(pc)
    if len(sites) != 148 or not DEPTH.keys() <= set(io):
        raise ValueError('Complete actual hull basis control graph drift')
    return sites, io, explicit, pops


def inputs(image, smoke=False):
    rows = edge_inputs(image)[:96]
    for row in rows:
        base = 0x61000000
        data = bytearray.fromhex(row['banks'][0]['before'])
        context = row['gp'][1] - base
        struct.pack_into('<I', data, context + 0x48, base + 0x7000)
        struct.pack_into('<I', data, context + 0x50, base + 0x8000)
        struct.pack_into('<I', data, 0x7080, 0)
        data[0x8000:0x8010] = data[0x2100:0x2110]
        data[0x8010:0x8020] = data[0x2000:0x2010]
        row['banks'][0]['before'] = data.hex()
        row['banks'].append(dict(address=0x46a000, before=image.read(0x46a000, 0x1000).hex()))
        row.update(entry=ENTRY, name='basis-' + str(row['id']), max_steps=5500,
                   live_x87=row['live_x87'][:5])
    return rows[:12] if smoke else rows


def capture(case, row, io, explicit, pops):
    if not case['completed'] or not case['live_x87_unchanged']:
        raise ValueError('Unchanged original basis did not return with live caller x87')
    second = 0x313981 in case['executed_sites']
    edge_calls = (0x313499, 0x3133ab, 0x313425, 0x3134a9, 0x3133ab, 0x313425)
    callsites = iter((0x31392c, *edge_calls, 0x313944, *edge_calls, 0x313977 if second else 0x313962))
    packets, children, reads, writes = [], [], [], []
    original_reads = iter(event for event in case['reads'] if event['instruction'] in io)
    for event in case['calls']:
        pc = event['entry']
        abi = {k: v for k, v in event.items() if k != 'arguments'}
        if pc in io:
            packets.append(abi)
            if pc in pops:
                reads.append(dict(instruction=pc, address=event['gp'][4], bytes=4))
            if pc in explicit:
                read = next(original_reads)
                if read['instruction'] != pc:
                    raise ValueError('Changed actual ordered basis read')
                reads.append(read)
        else:
            callsite = next(callsites)
            width = 3 if callsite in (0x3133ab, 0x313425) else 5
            children.append(dict(target=pc, site=callsite, return_pc=callsite+width,
                                 arguments=event['arguments'], abi=abi))
    if next(callsites, None) is not None or next(original_reads, None) is not None:
        raise ValueError('Incomplete actual basis descendant or read stream')
    for event in case['writes']:
        if event['instruction'] not in VECTOR_STORES:
            writes.append(event)
            continue
        old, new = bytes.fromhex(event['before']), bytes.fromhex(event['written'])
        if len(old) != 16 or len(new) != 16:
            raise ValueError('Actual basis MOVAPS store width drift')
        writes.extend(dict(instruction=event['instruction'], address=event['address']+off,
                           before=old[off:off+8].hex(), written=new[off:off+8].hex()) for off in (0, 8))
    stop = 0x313981 if second else 0x31396c
    return dict(abi=dict(entry=stop, gp=case['boundary_gp'], eflags=case['boundary_eflags'],
                         xmm=case['boundary_xmm'], fp=case['after_fp'], live_x87=row['live_x87']),
                packets=packets, children=children, reads=reads, writes=writes,
                banks=sorted(case['banks'], key=lambda value: value['address']))


def difference(want, got):
    key = next(k for k in want if want[k] != got.get(k))
    old, new = want[key], got.get(key)
    if key == 'banks':
        def hashes(banks):
            return {hex(b['address']): hashlib.sha256(bytes.fromhex(b['after'])).hexdigest() for b in banks}
        old, new = hashes(old), hashes(new)
    elif isinstance(old, list):
        first = next((n for n, (x, y) in enumerate(zip(old, new)) if x != y), min(len(old), len(new)))
        old, new = old[first:first+1], new[first:first+1]
    return dict(field=key, expected=old, actual=new)


def reject_defects(scratch, command, rows, expected):
    relative = 'physics/runtime/native_tu15_solver_hull_basis_frame.hpp'
    source = (ROOT/'src'/relative).read_text()
    variants = (
        ('wrong_first_axis_rank', 'return s_.flags.pf;}','return !s_.flags.pf;}'),
        ('exact_reciprocal_replaces_estimate',
         'asm volatile("rsqrtss %1,%0":"+x"(a):"x"(b):"memory");return a;',
         'return _mm_move_ss(a,_mm_div_ss(_mm_set_ss(1.f),_mm_sqrt_ss(b)));'),
        ('reversed_edge_choice', 'return !s_.flags.zf;', 'return s_.flags.zf;'),
    )
    data = scratch/'defect-inputs.json'
    data.write_text(json.dumps(rows[:12], separators=(',', ':')))
    rejected = {}
    for name, before, after in variants:
        if source.count(before) != 1:
            raise ValueError('Missing authentic hull basis mutation seam '+name)
        overlay = scratch/name
        path = overlay/relative
        path.parent.mkdir(parents=True)
        path.write_text(source.replace(before, after, 1))
        trial = command.copy()
        trial.insert(trial.index('-I'+str(ROOT/'include')), '-I'+str(overlay))
        trial[-1] = str(overlay/'basis')
        built = subprocess.run(trial, capture_output=True, text=True, timeout=600)
        if built.returncode:
            raise ValueError('Hull basis defect compile failed '+name+': '+built.stderr[-6000:])
        with data.open('rb') as inp:
            replay = subprocess.run([trial[-1], '--skip-hostile'], stdin=inp, capture_output=True, text=True, timeout=600)
        if replay.returncode:
            raise ValueError('Hull basis defect did not finish the actual child path '+name+': '+replay.stderr[-6000:])
        observed = json.loads(replay.stdout)
        first = next((i for i,(want,got) in enumerate(zip(expected[:12],observed,strict=True)) if want!=got),None)
        if first is None:
            raise ValueError('Unchanged original hull basis did not reject '+name)
        rejected[name] = dict(rejected=True, case=rows[first]['name'], **difference(expected[first],observed[first]))
    return rejected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--sanitizers', action='store_true')
    parser.add_argument('--smoke', action='store_true')
    parser.add_argument('--mutations', action='store_true')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    if platform.system() != 'Linux' or platform.machine().lower() not in ('x86_64', 'amd64', 'i386', 'i686') or not args.xbe.is_file():
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    test = ROOT / 'tests/native_tu15_solver_hull_basis_frame_tests.cpp'
    initial = [test, Path(__file__), ROOT / 'tools/validate_native_solver_hull_edge_frame_tu15.py',
               ROOT / 'tools/validate_native_tu15_prt_helpers.py', ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp']
    seen = {p.resolve() for p in initial}
    while initial:
        current = initial.pop()
        for name in re.findall(r'^#include "([^"\n]+)"', current.read_text(), re.M):
            target = next((p.resolve() for p in (current.parent/name, ROOT/'include'/name, ROOT/'src'/name) if p.is_file()), None)
            if target is not None and target not in seen:
                seen.add(target)
                initial.append(target)
    pins = {str(path.relative_to(ROOT)): sha(path) for path in sorted(seen)}
    report = dict(passed=False, production_bound=False, scratch_owner=str(scratch), source_sha256=pins,
                  body_sha256=BODY_PIN, retained_large_artifacts=[])
    try:
        if sha(args.xbe) != PIN:
            raise ValueError('Wrong authenticated original image')
        sites, io, explicit, pops = source()
        image = Xbe(args.xbe)
        if [image.read(pc, 4).hex() for pc in (0x313875, 0x3138ed)] != ['f30f52d8', 'f30f52c8']:
            raise ValueError('Actual basis estimate opcodes drifted')
        rows = inputs(image, args.smoke)
        data = scratch / 'inputs.json'
        data.write_text(json.dumps(rows, separators=(',', ':')))
        report['input_sha256'] = sha(data)
        command = [args.compiler, '-std=c++20', '-Wno-ignored-attributes', '-O2', '-fno-fast-math',
                   '-ffp-contract=off', '-frounding-math', '-I'+str(ROOT/'include'), '-I'+str(ROOT/'src'),
                   '-I'+str(ROOT/'tests'), str(test), '-o', str(scratch/'basis')]
        if args.sanitizers:
            command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        report['command'] = command
        compiled = subprocess.run(command, capture_output=True, text=True, timeout=600)
        if compiled.returncode:
            raise ValueError('Private hull basis compile failed: ' + compiled.stderr[-8000:])
        traces = [dict(entry=pc, arguments=0, gp=True, xmm=True, x87_delta=DEPTH.get(pc, 0)) for pc in io]
        traces.extend(dict(entry=pc, arguments=arity, gp=True, xmm=True) for pc, arity in CHILDREN.items())
        observed, report['original'] = physical_original(image, rows, scratch, entries=(ENTRY, *CHILDREN),
            regions=((0x61000000, 0x10000), (0x44a000, 0x1000), (0x461000, 0x2000), (0x46a000, 0x1000)),
            trace_calls=traces, trace_extended=True)
        output = scratch / 'native.json'
        with data.open('rb') as inp, output.open('wb') as out:
            result = subprocess.run([command[-1]], stdin=inp, stdout=out, stderr=subprocess.PIPE, timeout=600)
        if result.returncode:
            raise ValueError('Private hull basis replay failed: '+result.stderr.decode()[-8000:])
        visited, parents, child_entries, reads, writes = set(), 0, 0, 0, 0
        expected = []
        for case, got in zip(observed['cases'], json.loads(output.read_text()), strict=True):
            row = rows[case['id']]
            want = capture(case, row, io, explicit, pops)
            if want != got:
                report['first_difference'] = dict(case=row['name'], **difference(want, got))
                raise ValueError('Complete unchanged original hull basis differs ' + row['name'])
            visited.update(case['executed_sites'])
            parents += len(want['packets'])
            child_entries += len(want['children'])
            reads += len(want['reads'])
            writes += len(want['writes'])
            expected.append(want)
        report.update(original_sites=len(sites), executed_parent_sites=len(sites&visited),
                      missing_parent_sites=[hex(pc) for pc in sorted(sites-visited)], complete_calls=len(rows),
                      parent_pre_io_packets=parents, genuine_child_entry_packets=child_entries, parent_reads=reads,
                      ordered_parent_and_child_stores=writes, hostile_equivalences=len(rows),
                      supplied_child_outcomes=False, original_text_unmodified=True)
        if not args.smoke and sites - visited:
            raise ValueError('Missing original hull basis parent coverage')
        if args.mutations:
            report['compiled_defects'] = reject_defects(scratch, command, rows, expected)
        report['passed'] = True
    except Exception as error:
        report['error'] = repr(error)
    try:
        source()
        report['sources_unchanged'] = all(sha(ROOT/name) == value for name, value in pins.items())
    except (ValueError, OSError):
        report['sources_unchanged'] = False
    if not report['sources_unchanged']:
        report.update(passed=False, source_drift=True)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('passed', 'error') if k in report}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
