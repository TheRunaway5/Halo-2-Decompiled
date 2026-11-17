#!/usr/bin/env python3
"""Compare the private actual3131b0 sampling frame to the original i386 owner."""
import argparse
import hashlib
import json
import os
import platform
import random
import re
import struct
import subprocess
from pathlib import Path

from generated_routine_ownership import function_bodies
import native_solver_hull_minimum_replacements as binding
from research_mp_sandbox_update_functions import Xbe
from validate_native_tu15_prt_helpers import PIN, physical_original

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'src/physics/physics_live_support_tu15_generated.cpp'
BODY_PIN = '0f800ab67d9e63dac43f5f72584b2050f2a3afdc12325e8b319e2a0e5eb455ac'
ENTRY = 0x3131b0
RET = 0x313289
IO_DEPTH = {
    0x3131b0: 0, 0x3131b1: 0, 0x3131b7: 0, 0x3131bd: 0,
    0x3131c9: 1, 0x3131d5: 1, 0x3131e0: 1, 0x3131e3: 2, 0x3131f5: 0,
    0x3131f9: 1, 0x3131ff: 2, 0x313214: 0, 0x313218: 1, 0x31321e: 2,
    0x313233: 0, 0x313237: 1, 0x31323d: 2, 0x313252: 0, 0x31325f: 1,
    0x313265: 1, 0x31326b: 2, 0x31327c: 0, 0x313287: 0, 0x313288: 0, RET: 0,
}
EXPLICIT_READS = {0x3131b1,0x3131b7,0x3131bd,0x3131e0,0x3131f5,0x3131f9,0x313214,
                  0x313218,0x313233,0x313237,0x313252,0x313265,0x31327c}
STACK_READS = {0x31325f,0x313287,0x313288,RET}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def actual_body():
    source = SOURCE.read_text()
    if any(marker in source for marker in (binding.INCLUDE, binding.OWNER_MARKER, binding.RESUME_MARKER)):
        if binding.installed_entries(source) != ['3131b0']:
            raise ValueError('Complete minimum source overlay is not authenticated')
    body = next(body for sig, body in function_bodies(source) if sig.symbol == '3131b0')
    return binding.canonical_body(body)


def inputs(image):
    rng = random.Random(ENTRY)
    base, stack, polytope = 0x61000000, 0x61004000, 0x61001000
    counts = (0,1,2,3,4,5,6,7,8,9,10,12,13,17,0xffffffff,0x80000002)
    edges = (0,0x80000000,1,0x80000001,0x7fffff,0x800000,0x7f7fffff,
             0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc23456,
             0x7f812345,0xff823456,0x3f800000,0xbf800000)
    live = ('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f')
    rows = []
    for i in range(96):
        bank = bytearray(rng.randbytes(0x6000))
        gp = [rng.getrandbits(32)for _ in range(8)]
        gp[1], gp[4] = polytope, stack+(i&15)
        facets = gp[4]-4-0x10-((i%4)+1)*0x50 if i%3==2 else base+0x2000
        count = counts[i%len(counts)]
        struct.pack_into('<II',bank,polytope+0x1b0-base,facets,count)
        for n in range(19):
            pattern = (i//len(counts))%6
            if pattern<3:
                score = struct.unpack('<I',struct.pack('<f',(16.0-n,n-7.0,2.0)[pattern]))[0]
            else:
                score = edges[(i+3*n)%len(edges)] if pattern<5 else rng.getrandbits(32)
            struct.pack_into('<I',bank,facets+0x10+n*0x50-base,score)
        returned, seed1, seed2 = 0x63000000,rng.getrandbits(32),rng.getrandbits(32)
        struct.pack_into('<3I',bank,gp[4]-base,returned,seed1,seed2)
        cw = (0x7f,0x27f,0x37f)[i%3]|((i%4)<<10)
        sw = 0x4100|(((i//3)%8)<<11)|(0x200 if i&1 else 0)
        mxcsr = 0x1f80|((i%4)<<13)|(0x8040 if i&4 else 0)
        rows.append(dict(id=i,name=f'minimum-{i}',entry=ENTRY,gp=gp,
                         eflags=0x202|(rng.getrandbits(12)&0xcd5),return_pc=returned,
                         matrix=seed1,source=seed2,before_fp=[cw,sw,mxcsr],
                         live_x87=[live[j%4]for j in range((0,1,4,6)[i%4])],
                         xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],
                         max_steps=800,banks=[dict(address=base,before=bank.hex())]))
    return rows


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report', required=True, type=Path)
    p.add_argument('--xbe', required=True, type=Path)
    p.add_argument('--compiler', default='c++')
    p.add_argument('--sanitizers', action='store_true')
    p.add_argument('--mutations', action='store_true')
    a = p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        p.error('Run through tools/run_test.py')
    if platform.system() != 'Linux' or platform.machine().lower() not in ('x86_64', 'amd64', 'i386', 'i686') or not a.xbe.is_file():
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    paths = [SOURCE, ROOT / 'include/halo2/native_tu15_solver_hull_minimum_kernel.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_minimum_frame.hpp',
             ROOT / 'tests/native_tu15_solver_hull_minimum_frame_tests.cpp',
             ROOT / 'tools/validate_native_tu15_prt_helpers.py',
             ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp',
             ROOT / 'tools/native_solver_hull_minimum_replacements.py', Path(__file__)]
    # This childless raw proof reads the exact Primary routine and checks its
    # complete body pin before and after, independent of sibling owner edits.
    pins = {str(path.relative_to(ROOT)): sha(path) for path in paths if path != SOURCE}
    report = dict(passed=False, production_bound=False, scratch_owner=str(scratch), source_sha256=pins,
                  retained_large_artifacts=[])
    try:
        if sha(a.xbe) != PIN:
            raise ValueError('Wrong TU1.5 original image')
        body = actual_body()
        if hashlib.sha256(body.encode()).hexdigest() != BODY_PIN:
            raise ValueError('Complete actual3131b0 body drift')
        image = Xbe(a.xbe)
        if image.read(RET, 1).hex() != 'c3':
            raise ValueError('Actual minimum return-word opcode drift')
        rows = inputs(image)
        data = scratch / 'inputs.json'
        data.write_text(json.dumps(rows, separators=(',', ':')))
        report['input_sha256'] = sha(data)
        traces = [dict(entry=pc, arguments=0, gp=True, xmm=True, x87_delta=delta)
                  for pc, delta in IO_DEPTH.items()]
        observed, proof = physical_original(image, rows, scratch, entries=(ENTRY,),
            regions=((0x61000000, 0x6000),), trace_calls=traces, trace_extended=True)
        report['original'] = proof
        command = [a.compiler, '-std=c++20', '-Wno-ignored-attributes', '-O2', '-fno-fast-math',
                   '-ffp-contract=off', '-frounding-math', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'),
                   '-I' + str(ROOT / 'tests'), str(paths[3]), '-o', str(scratch / 'minimum')]
        if a.sanitizers:
            command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        report['command'] = command
        built = subprocess.run(command, capture_output=True, text=True)
        if built.returncode:
            raise ValueError('Private minimum compile failed: ' + built.stderr[-8000:])
        result = scratch / 'native.json'
        with data.open('rb') as inp, result.open('wb') as out:
            done = subprocess.run([command[-1]], stdin=inp, stdout=out, stderr=subprocess.PIPE)
        if done.returncode:
            raise ValueError('Private minimum stopped: ' + done.stderr.decode()[-8000:])
        actual = json.loads(result.read_text())
        visited, packets, reads, writes = set(), 0, 0, 0
        expected_rows = []
        for case, got in zip(observed['cases'], actual, strict=True):
            row = rows[case['id']]
            if not case['completed'] or not case['live_x87_unchanged']:
                raise ValueError('Actual minimum did not preserve its caller x87 stack')
            # Add the original POP/RET implicit reads at the exact
            # observed instruction using its actual pre-instruction ESP.
            memory_reads = iter(case['reads'])
            complete_reads = []
            for packet in case['calls']:
                pc = packet['entry']
                if pc in STACK_READS:
                    complete_reads.append(dict(instruction=pc,address=packet['gp'][4],bytes=4))
                elif pc in EXPLICIT_READS:
                    original_read = next(memory_reads)
                    if original_read['instruction'] != pc:
                        raise ValueError('Wrong ordered original minimum read')
                    complete_reads.append(original_read)
            if next(memory_reads,None) is not None:
                raise ValueError('Unclassified original minimum read')
            want = dict(
                abi=dict(entry=RET, gp=case['boundary_gp'], eflags=case['boundary_eflags'],
                         xmm=case['boundary_xmm'], fp=case['after_fp'], live_x87=row['live_x87']),
                packets=[{k: v for k, v in packet.items() if k != 'arguments'} for packet in case['calls']],
                banks=sorted(case['banks'], key=lambda bank: bank['address']),
                reads=complete_reads, writes=case['writes'])
            if want != got:
                field = next(k for k in want if want[k] != got.get(k))
                expected, received = want[field], got.get(field)
                if field in ('packets', 'reads', 'writes'):
                    first = next((n for n, (x, y) in enumerate(zip(expected, received)) if x != y),
                                 min(len(expected), len(received)))
                    expected, received = expected[first:first+1], received[first:first+1]
                if field == 'banks':
                    expected = [hashlib.sha256(bytes.fromhex(b['after'])).hexdigest() for b in expected]
                    received = [hashlib.sha256(bytes.fromhex(b['after'])).hexdigest() for b in received]
                report['first_difference'] = dict(case=row['name'], field=field, expected=expected, actual=received)
                raise ValueError('Complete unchanged original minimum differs: ' + row['name'] + ' ' + field)
            expected_rows.append(want)
            visited.update(case['executed_sites'])
            packets += len(case['calls'])
            reads += len(want['reads'])
            writes += len(case['writes'])
        sites = {int(value, 16) for value in re.findall(r'services.instruction\(0x([0-9a-f]+)u\)', body)}
        if len(sites) != 76 or not sites <= visited:
            raise ValueError('Missing authored original hull minimum sites')
        if a.mutations:
            relative = 'physics/runtime/native_tu15_solver_hull_minimum_frame.hpp'
            text = (ROOT / 'src' / relative).read_text()
            cases = (
                ('inverted_ordered_preference', 'return !s_.flags.pf;', 'return s_.flags.pf;'),
                ('omitted_visible_candidate_spill', 'Base::store(pc,address,pending.bits,[&]{fp_.commit(pending);},true);',
                 'fp_.commit(pending);'),
                ('late_batch_shift_flags', 'shift_batch_count();push(0x3131d5u,g[7]);',
                 'push(0x3131d5u,g[7]);shift_batch_count();'),
            )
            report['compiled_defects'] = {}
            for name, old, new in cases:
                if text.count(old) != 1:
                    raise ValueError('Missing genuine minimum defect seam: ' + name)
                owned = scratch / name
                header = owned / relative
                header.parent.mkdir(parents=True)
                header.write_text(text.replace(old, new, 1))
                trial = command.copy()
                trial.insert(trial.index('-I' + str(ROOT / 'include')), '-I' + str(owned))
                trial[-1] = str(owned / 'minimum')
                built = subprocess.run(trial, capture_output=True, text=True, timeout=600)
                if built.returncode:
                    raise ValueError('Minimum defect compile failed ' + name + ': ' + built.stderr[-5000:])
                with data.open('rb') as inp:
                    done = subprocess.run([trial[-1]], stdin=inp, capture_output=True, text=True, timeout=600)
                if done.returncode:
                    raise ValueError('Minimum defect stopped before comparison ' + name + ': ' + done.stderr[-5000:])
                different = json.loads(done.stdout)
                first = next((n for n, (x, y) in enumerate(zip(expected_rows, different, strict=True)) if x != y), None)
                if first is None:
                    raise ValueError('Original i386 minimum did not reject ' + name)
                field = next(k for k in expected_rows[first] if expected_rows[first][k] != different[first][k])
                report['compiled_defects'][name] = dict(rejected=True, first_case=rows[first]['name'], first_field=field)
        report.update(passed=True, body_sha256=BODY_PIN, original_sites=len(sites), complete_original_calls=len(rows),
                      complete_parent_pre_io_packets=packets, architectural_reads=reads, complete_stores=writes,
                      hostile_provider_comparisons=len(rows), original_text_unmodified=True,
                      source_or_hardware_fault_domains_qualified=False)
    except Exception as error:
        report['error'] = repr(error)
    try:
        after = actual_body()
        report['sources_unchanged'] = all(sha(ROOT / name) == value for name, value in pins.items()) and (
            hashlib.sha256(after.encode()).hexdigest() == BODY_PIN)
    except (ValueError, OSError, StopIteration):
        report['sources_unchanged'] = False
    if not report['sources_unchanged']:
        report['passed'] = False
        report['source_drift'] = True
    a.report.parent.mkdir(parents=True, exist_ok=True)
    a.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: v for k, v in report.items() if k in ('passed', 'error', 'first_difference')}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
