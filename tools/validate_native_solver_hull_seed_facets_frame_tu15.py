#!/usr/bin/env python3
"""Compare the private actual314d80 tetrahedron seed facets frame to the original i386 owner."""
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
import validate_native_solver_hull_facet_plane_frame_tu15 as plane
from research_mp_sandbox_update_functions import Xbe
from validate_native_tu15_prt_helpers import PIN, physical_original

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'src/physics/physics_live_support_tu15_generated.cpp'
BODY_PIN = 'aea4f9f90a0cf403578341309ffd367b0525925dcf6992033df9fdeace7e43b0'
ENTRY = 0x314d80
RET = 0x314ec3
REGIONS = ((0x41d000,0x1000),(0x46a000,0x1000),(0x61000000,0x8000))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def actual_body():
    return next(body for sig, body in function_bodies(SOURCE.read_text()) if sig.symbol == '314d80')


def authored_io(body):
    reads,stack,stores=set(),set(),set()
    for line in body.splitlines():
        site=re.search(r'services.instruction\(0x([0-9a-f]+)u\)',line)
        if not site:
            continue
        pc=int(site[1],16)
        if re.search(r'\b(?:r|v)\(s.memory',line):reads.add(pc)
        if 's.pop()' in line:stack.add(pc)
        if re.search(r'\b(?:w|vw)\(s.memory|s.push\(',line):stores.add(pc)
    return reads,stack,stores



def inputs(image):
    rng=random.Random(ENTRY)
    base=0x61000000
    context,stack,facet,edge=(base+offset for offset in (0x1000,0x7000,0x3500,0x4200))
    constants=[dict(address=address,before=image.read(address,size).hex())for address,size in REGIONS[:2]]
    live=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f')
    rows=[]
    for i in range(48):
        bank=bytearray(rng.randbytes(0x8000));gp=[rng.getrandbits(32)for _ in range(8)]
        gp[1],gp[4]=context,stack+(i&15)
        struct.pack_into('<2I',bank,context+0x1b0-base,facet,0xcafe0000|i)
        struct.pack_into('<I',bank,context+0x2100-base,edge)
        points=[(0.,0.,0.,0.),(1.,0.,0.,0.),(0.,1.,0.,0.),(0.,0.,1.,0.)]
        if i&1:points[2],points[3]=points[3],points[2]
        if i%6==2:points[0]=(1.,1.,1.,-0.0)
        if i%6==3:points=[(x*.25,y*.5,z*2.,w)for x,y,z,w in points]
        if i%6==4:points[3]=(0.,0.,0.,0.)
        for n,point in enumerate(points):
            struct.pack_into('<4f',bank,edge-base+32*n,*point)
        if i%6==5:
            struct.pack_into('<I',bank,edge-base+12,0x7fc12345)
            struct.pack_into('<I',bank,edge-base+44,0xffc23456)
        returned=0x63000000
        seed_a,seed_b=rng.getrandbits(32),rng.getrandbits(32)
        struct.pack_into('<3I',bank,gp[4]-base,returned,seed_a,seed_b)
        cw=(0x7f,0x27f,0x37f)[i%3]|((i%4)<<10)
        sw=0x4100|(((i//3)%8)<<11)|(0x200 if i&1 else 0)
        mxcsr=0x1f80|((i%4)<<13)|(0x8040 if i&4 else 0)
        rows.append(dict(id=i,name=f'seed-tetrahedron-{i}',entry=ENTRY,gp=gp,
                         eflags=0x202|(rng.getrandbits(12)&0xcd5),return_pc=returned,matrix=seed_a,source=seed_b,
                         before_fp=[cw,sw,mxcsr],live_x87=[live[j%4]for j in range((0,1,4,6,7,2)[i%6])],
                         xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],max_steps=2100,
                         banks=constants+[dict(address=base,before=bank.hex())]))
    return rows


def child_body():
    result=plane.actual_body()
    if hashlib.sha256(result.encode()).hexdigest()!=plane.BODY_PIN:
        raise ValueError('Complete genuine seed facet Plane child drift')
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
    paths = [SOURCE, ROOT / 'include/halo2/native_tu15_solver_hull_seed_facets_kernel.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_seed_facets_frame.hpp',
             ROOT / 'tests/native_tu15_solver_hull_seed_facets_frame_tests.cpp',
             ROOT / 'tools/validate_native_tu15_prt_helpers.py',
             ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp',
             ROOT / 'src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp',
             ROOT / 'src/physics/runtime/native_tu15_raw_x87_scope.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_facet_plane_frame.hpp',
             ROOT / 'tools/validate_native_solver_hull_facet_plane_frame_tu15.py',Path(__file__)]
    # Pin both complete actual bodies before and after, independent of unrelated Primary additions.
    pins = {str(path.relative_to(ROOT)): sha(path) for path in paths if path != SOURCE}
    report = dict(passed=False, production_bound=False, scratch_owner=str(scratch), source_sha256=pins,
                  retained_large_artifacts=[])
    try:
        if sha(a.xbe) != PIN:
            raise ValueError('Wrong TU1.5 original image')
        body = actual_body()
        if hashlib.sha256(body.encode()).hexdigest() != BODY_PIN:
            raise ValueError('Complete actual314d80 body drift')
        image = Xbe(a.xbe)
        if image.read(RET,1).hex()!='c3':
            raise ValueError('Actual seed_facets return-word opcode drift')
        rows = inputs(image)
        data = scratch / 'inputs.json'
        data.write_text(json.dumps(rows, separators=(',', ':')))
        report['input_sha256'] = sha(data)
        child_body()
        reads_set,stack_set,stores=authored_io(body)
        explicit=reads_set|plane.EXPLICIT_READS
        stack_reads=stack_set|plane.STACK_READS
        io=sorted(reads_set|stack_set|stores|set(plane.IO))
        traces=[dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=int(pc==0x314e74))for pc in io]
        observed, proof = physical_original(image, rows, scratch, entries=(ENTRY,plane.ENTRY),
            regions=REGIONS, trace_calls=traces, trace_extended=True)
        report['original'] = proof
        command = [a.compiler, '-std=c++20', '-Wno-ignored-attributes', '-O2', '-fno-fast-math',
                   '-ffp-contract=off', '-frounding-math', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'),
                   '-I' + str(ROOT / 'tests'), str(paths[3]), '-o', str(scratch / 'seed_facets')]
        if a.sanitizers:
            command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        report['command'] = command
        built = subprocess.run(command, capture_output=True, text=True)
        if built.returncode:
            raise ValueError('Private seed_facets compile failed: ' + built.stderr[-8000:])
        result = scratch / 'native.json'
        with data.open('rb') as inp, result.open('wb') as out:
            done = subprocess.run([command[-1]], stdin=inp, stdout=out, stderr=subprocess.PIPE)
        if done.returncode:
            raise ValueError('Private seed_facets stopped: ' + done.stderr.decode()[-8000:])
        actual = json.loads(result.read_text())
        visited, packets, reads, writes = set(), 0, 0, 0
        expected_rows = []
        for case, got in zip(observed['cases'], actual, strict=True):
            row = rows[case['id']]
            if not case['completed'] or not case['live_x87_unchanged']:
                raise ValueError('Actual seed_facets did not preserve its caller x87 stack')
            # Add the original POP/RET implicit reads at the exact
            # observed instruction using its actual pre-instruction ESP.
            memory_reads = iter(case['reads'])
            complete_reads = []
            for packet in case['calls']:
                pc = packet['entry']
                if pc in stack_reads:
                    complete_reads.append(dict(instruction=pc,address=packet['gp'][4],bytes=4))
                elif pc in explicit:
                    original_read = next(memory_reads)
                    if original_read['instruction'] != pc:
                        raise ValueError('Wrong ordered original seed_facets read')
                    complete_reads.append(original_read)
            if next(memory_reads,None) is not None:
                raise ValueError('Unclassified original seed_facets read')
            want = dict(
                abi=dict(entry=RET, gp=case['boundary_gp'], eflags=case['boundary_eflags'],
                         xmm=case['boundary_xmm'], fp=case['after_fp'], live_x87=row['live_x87']),
                packets=[{k: v for k, v in packet.items() if k != 'arguments'} for packet in case['calls']],
                banks=sorted(case['banks'], key=lambda bank: bank['address']),
                reads=complete_reads, writes=plane.split_writes(case['writes']),
                plane_entries=sum(item['entry']==plane.ENTRY for item in case['calls']))
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
                raise ValueError('Complete unchanged original seed_facets differs: ' + row['name'] + ' ' + field)
            expected_rows.append(want)
            visited.update(case['executed_sites'])
            packets += len(case['calls'])
            reads += len(want['reads'])
            writes += len(case['writes'])
        sites = {int(value, 16) for value in re.findall(r'services.instruction\(0x([0-9a-f]+)u\)', body)}
        missing=sorted(sites-visited)
        report['missing_sites']=[hex(pc)for pc in missing]
        if len(sites) != 97 or missing:
            raise ValueError('Missing authored original hull seed_facets sites')
        if a.mutations:
            relative = 'physics/runtime/native_tu15_solver_hull_seed_facets_frame.hpp'
            text = (ROOT / 'src' / relative).read_text()
            cases = (
                ('wrong_link_record_stride','s_.flags.plus(g[2],12u,4u)','s_.flags.plus(g[2],0u,4u)'),
                ('omitted_first_winding_write','io_.store(0x314e81u,g[1]+0xb4u,4u,g[2]);',''),
                ('wrong_winding_compare','s_.flags.logic((g[0]>>8u)&0x41u,1u);','s_.flags.logic((g[0]>>8u)&1u,1u);'),
            )
            report['compiled_defects'] = {}
            for name, old, new in cases:
                if text.count(old) != 1:
                    raise ValueError('Missing genuine seed_facets defect seam: ' + name)
                owned = scratch / name
                header = owned / relative
                header.parent.mkdir(parents=True)
                header.write_text(text.replace(old, new, 1))
                trial = command.copy()
                trial.insert(trial.index('-I' + str(ROOT / 'include')), '-I' + str(owned))
                trial[-1] = str(owned / 'seed_facets')
                built = subprocess.run(trial, capture_output=True, text=True, timeout=600)
                if built.returncode:
                    raise ValueError('Seed facets defect compile failed ' + name + ': ' + built.stderr[-5000:])
                with data.open('rb') as inp:
                    done = subprocess.run([trial[-1]], stdin=inp, capture_output=True, text=True, timeout=600)
                if done.returncode:
                    raise ValueError('Seed facets defect stopped before comparison ' + name + ': ' + done.stderr[-5000:])
                different = json.loads(done.stdout)
                first = next((n for n, (x, y) in enumerate(zip(expected_rows, different, strict=True)) if x != y), None)
                if first is None:
                    raise ValueError('Original i386 seed_facets did not reject ' + name)
                field = next(k for k in expected_rows[first] if expected_rows[first][k] != different[first][k])
                report['compiled_defects'][name] = dict(rejected=True, first_case=rows[first]['name'], first_field=field)
        report.update(passed=True, body_sha256=BODY_PIN, original_sites=len(sites), executed_original_sites=len(sites&visited), complete_original_calls=len(rows),
                      genuine_plane_entries=sum(row['plane_entries']for row in expected_rows),
                      complete_parent_pre_io_packets=packets, architectural_reads=reads, complete_stores=writes,
                      hostile_provider_comparisons=len(rows), original_text_unmodified=True,
                      source_or_hardware_fault_domains_qualified=False)
    except Exception as error:
        report['error'] = repr(error)
    try:
        after = actual_body()
        report['sources_unchanged'] = all(sha(ROOT / name) == value for name, value in pins.items()) and (
            hashlib.sha256(after.encode()).hexdigest() == BODY_PIN)
        child_body()
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
