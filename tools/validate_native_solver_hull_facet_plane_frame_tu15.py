#!/usr/bin/env python3
"""Compare the private actual314a30 facet-plane frame to the original i386 owner."""
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
from research_mp_sandbox_update_functions import Xbe
from validate_native_tu15_prt_helpers import PIN, physical_original

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'src/physics/physics_live_support_tu15_generated.cpp'
BODY_PIN = '709b82327b9db90dcc38e86ec52e3f3c5122042bab49218b0108f3b2639cbd61'
ENTRY = 0x314a30
RET = 0x314b1a
EXPLICIT_READS = {0x314a39,0x314a3c,0x314a3f,0x314a42,0x314a45,0x314a48,0x314aa4,0x314ab9,0x314acb,0x314aec,0x314b10}
STACK_READS = {0x314b19,RET}
VECTOR_STORES = {0x314a95,0x314ae0,0x314ae9}
STORES = VECTOR_STORES|{0x314a30,0x314a9e,0x314ab1,0x314ac3,0x314b0c,0x314b14}
IO = sorted(EXPLICIT_READS|STACK_READS|STORES|{0x314a9a})


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def actual_body():
    return next(body for sig, body in function_bodies(SOURCE.read_text()) if sig.symbol == '314a30')


def inputs():
    rng = random.Random(ENTRY)
    base,stack,facet=0x61000000,0x61005000,0x61001000
    live=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f')
    patterns=(
        ((0,0,0,0),(0x3f800000,0,0,0),(0,0x3f800000,0,0)),
        ((0x40000000,0x3f800000,0x80000000,0),(0x3f800000,0x40000000,0,0),(0,0x3f800000,0x40000000,0)),
        ((0,0,0,0),(0,0,0,0),(0,0,0,0)),
        ((0,0,0,0),(1,0,0,0),(0,0x80000001,0,0)),
        ((0x7f7fffff,0x40000000,0,0),(0xff7fffff,0x40000000,0,0),(0,0x7f7fffff,0,0)),
        ((0x7fc12345,0x40000000,0,0xffc00001),(0x3f800000,0xffc23456,0,0),(0x7f812345,0,0x3f800000,0x7fc30002)),
        ((0x7f800000,0,0,0),(0,0xff800000,0,0),(0,0,0x3f800000,0)),
        ((0x3f800001,0x3f000007,0xbe800007,0x40400000),(0x3f000001,0xbf800009,0x4000000f,0),
         (0x3f000017,0x3e800001,0xc0000001,0x80000000)),
    )
    rows=[]
    for i in range(64):
        bank=bytearray(rng.randbytes(0x6000));gp=[rng.getrandbits(32)for _ in range(8)]
        gp[1],gp[4]=facet,stack+(i&15)
        points=[base+0x2400,base+0x2410,base+0x2420]
        alias=(i//8)%4
        if alias==1:points[0]=facet
        if alias==2:points[0]=((gp[4]-4)&~15)-16
        if alias==3:points[2]=facet
        for point,values in zip(points,patterns[i%8],strict=True):struct.pack_into('<4I',bank,point-base,*values)
        for offset,point in zip((0x14,0x24,0x34),points,strict=True):struct.pack_into('<I',bank,facet+offset-base,point)
        returned,seed1,seed2=0x63000000,rng.getrandbits(32),rng.getrandbits(32)
        struct.pack_into('<3I',bank,gp[4]-base,returned,seed1,seed2)
        cw=(0x7f,0x27f,0x37f)[i%3]|((i%4)<<10)
        sw=0x4100|(((i//3)%8)<<11)|(0x200 if i&1 else 0)
        mxcsr=0x1f80|((i%4)<<13)|(0x8040 if i&4 else 0)
        rows.append(dict(id=i,name=f'facet-plane-{i}',entry=ENTRY,gp=gp,
                         eflags=0x202|(rng.getrandbits(12)&0xcd5),return_pc=returned,matrix=seed1,source=seed2,
                         before_fp=[cw,sw,mxcsr],live_x87=[live[j%4]for j in range((0,1,4,6,7,8)[i%6])],
                         xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],max_steps=100,
                         banks=[dict(address=base,before=bank.hex())]))
    return rows


def split_writes(writes):
    result=[]
    for event in writes:
        if event['instruction'] not in VECTOR_STORES:
            result.append(event)
            continue
        if len(event['written'])!=32 or len(event['before'])!=32:
            raise ValueError('Expected one unchanged original 16-byte facet-plane store')
        for offset in (0,8):
            result.append(dict(event,address=event['address']+offset,
                               before=event['before'][offset*2:offset*2+16],written=event['written'][offset*2:offset*2+16]))
    return result


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
    paths = [SOURCE, ROOT / 'include/halo2/native_tu15_solver_hull_facet_plane_kernel.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_facet_plane_frame.hpp',
             ROOT / 'tests/native_tu15_solver_hull_facet_plane_frame_tests.cpp',
             ROOT / 'tools/validate_native_tu15_prt_helpers.py',
             ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp',
             ROOT / 'src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp',
             ROOT / 'src/physics/runtime/native_tu15_raw_x87_scope.hpp', Path(__file__)]
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
            raise ValueError('Complete actual314a30 body drift')
        image = Xbe(a.xbe)
        if image.read(RET, 1).hex() != 'c3':
            raise ValueError('Actual facet_plane return-word opcode drift')
        rows = inputs()
        data = scratch / 'inputs.json'
        data.write_text(json.dumps(rows, separators=(',', ':')))
        report['input_sha256'] = sha(data)
        traces = [dict(entry=pc, arguments=0, gp=True, xmm=True, x87_delta=0) for pc in IO]
        observed, proof = physical_original(image, rows, scratch, entries=(ENTRY,),
            regions=((0x61000000, 0x6000),), trace_calls=traces, trace_extended=True)
        report['original'] = proof
        command = [a.compiler, '-std=c++20', '-Wno-ignored-attributes', '-O2', '-fno-fast-math',
                   '-ffp-contract=off', '-frounding-math', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'),
                   '-I' + str(ROOT / 'tests'), str(paths[3]), '-o', str(scratch / 'facet_plane')]
        if a.sanitizers:
            command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        report['command'] = command
        built = subprocess.run(command, capture_output=True, text=True)
        if built.returncode:
            raise ValueError('Private facet_plane compile failed: ' + built.stderr[-8000:])
        result = scratch / 'native.json'
        with data.open('rb') as inp, result.open('wb') as out:
            done = subprocess.run([command[-1]], stdin=inp, stdout=out, stderr=subprocess.PIPE)
        if done.returncode:
            raise ValueError('Private facet_plane stopped: ' + done.stderr.decode()[-8000:])
        actual = json.loads(result.read_text())
        visited, packets, reads, writes = set(), 0, 0, 0
        expected_rows = []
        for case, got in zip(observed['cases'], actual, strict=True):
            row = rows[case['id']]
            if not case['completed'] or not case['live_x87_unchanged']:
                raise ValueError('Actual facet_plane did not preserve its caller x87 stack')
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
                        raise ValueError('Wrong ordered original facet_plane read')
                    complete_reads.append(original_read)
            if next(memory_reads,None) is not None:
                raise ValueError('Unclassified original facet_plane read')
            want = dict(
                abi=dict(entry=RET, gp=case['boundary_gp'], eflags=case['boundary_eflags'],
                         xmm=case['boundary_xmm'], fp=case['after_fp'], live_x87=row['live_x87']),
                packets=[{k: v for k, v in packet.items() if k != 'arguments'} for packet in case['calls']],
                banks=sorted(case['banks'], key=lambda bank: bank['address']),
                reads=complete_reads, writes=split_writes(case['writes']))
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
                raise ValueError('Complete unchanged original facet_plane differs: ' + row['name'] + ' ' + field)
            expected_rows.append(want)
            visited.update(case['executed_sites'])
            packets += len(case['calls'])
            reads += len(want['reads'])
            writes += len(case['writes'])
        sites = {int(value, 16) for value in re.findall(r'services.instruction\(0x([0-9a-f]+)u\)', body)}
        if len(sites) != 66 or not sites <= visited:
            raise ValueError('Missing authored original hull facet_plane sites')
        if a.mutations:
            relative = 'physics/runtime/native_tu15_solver_hull_facet_plane_frame.hpp'
            text = (ROOT / 'src' / relative).read_text()
            cases = (
                ('wrong_normal_orientation','x[1]=sub(mul(x[1],x[2]),x[3]);','x[1]=sub(x[3],mul(x[1],x[2]));'),
                ('wrong_reciprocal_estimate_destination','"+x"(x[2]):"x"(x[0])','"+x"(x[0]):"x"(x[2])'),
                ('dropped_unnormalized_publication','io_.store_vector(0x314ae0u,g[1],x[1]);',''),
            )
            report['compiled_defects'] = {}
            for name, old, new in cases:
                if text.count(old) != 1:
                    raise ValueError('Missing genuine facet_plane defect seam: ' + name)
                owned = scratch / name
                header = owned / relative
                header.parent.mkdir(parents=True)
                header.write_text(text.replace(old, new, 1))
                trial = command.copy()
                trial.insert(trial.index('-I' + str(ROOT / 'include')), '-I' + str(owned))
                trial[-1] = str(owned / 'facet_plane')
                built = subprocess.run(trial, capture_output=True, text=True, timeout=600)
                if built.returncode:
                    raise ValueError('Facet plane defect compile failed ' + name + ': ' + built.stderr[-5000:])
                with data.open('rb') as inp:
                    done = subprocess.run([trial[-1]], stdin=inp, capture_output=True, text=True, timeout=600)
                if done.returncode:
                    raise ValueError('Facet plane defect stopped before comparison ' + name + ': ' + done.stderr[-5000:])
                different = json.loads(done.stdout)
                first = next((n for n, (x, y) in enumerate(zip(expected_rows, different, strict=True)) if x != y), None)
                if first is None:
                    raise ValueError('Original i386 facet_plane did not reject ' + name)
                field = next(k for k in expected_rows[first] if expected_rows[first][k] != different[first][k])
                report['compiled_defects'][name] = dict(rejected=True, first_case=rows[first]['name'], first_field=field)
        report.update(passed=True, body_sha256=BODY_PIN, original_sites=len(sites), complete_original_calls=len(rows),
                      complete_parent_pre_io_packets=packets, architectural_reads=reads, complete_stores=writes,
                      hostile_provider_comparisons=len(rows), original_text_unmodified=True,
                      physical_vector_store_protocol='Architectural16-byte writes compared with declared ordered8+8 provider events',
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
