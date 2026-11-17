#!/usr/bin/env python3
"""Compare the private actual314cb0 facet-ring construction frame to the original i386 owner."""
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
import validate_native_solver_hull_clear_frame_tu15 as clear
import validate_native_solver_hull_facet_plane_frame_tu15 as plane
from research_mp_sandbox_update_functions import Xbe
from validate_native_tu15_prt_helpers import PIN, physical_original

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'src/physics/physics_live_support_tu15_generated.cpp'
BODY_PIN = 'db500d7108e70c9325320d23a97da4795d63910556c4f18e0ff99b9baad0ea70'
ENTRY = 0x314cb0
RET = 0x314d7b
REGION = (0x61000000,0x8000)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def actual_body():
    return next(body for sig, body in function_bodies(SOURCE.read_text()) if sig.symbol == '314cb0')


def authored_io(body):
    reads,stack,stores=set(),set(),set()
    for line in body.splitlines():
        site=re.search(r'services.instruction\(0x([0-9a-f]+)u\)',line)
        if not site:
            continue
        pc=int(site[1],16)
        if re.search(r'\br\(s.memory',line):reads.add(pc)
        if 's.pop()' in line:stack.add(pc)
        if re.search(r'\bw\(s.memory|s.push\(',line):stores.add(pc)
    return reads,stack,stores


def inputs():
    rng=random.Random(ENTRY)
    base,stack,context=0x61000000,0x61007000,0x61001000
    descriptor,table,facet=base+0x3400,base+0x3500,base+0x4000
    nodes=[base+0x2400+n*0x40 for n in range(4)]
    holders=[base+0x2800+n*4 for n in range(4)]
    second=[base+0x2900+n*0x40 for n in range(4)]
    first=[base+0x2b00+n*0x40 for n in range(4)]
    third,third_holder=base+0x2c00,base+0x2d00
    live=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f')
    cases=[(count,chain,clear_count)for count in (0xffffffff,0,1,2,3)for chain in (0,1,2,4)for clear_count in (0,3)]
    rows=[]
    for i,(count,chain,clear_count) in enumerate(cases):
        bank=bytearray(rng.randbytes(0x8000));gp=[rng.getrandbits(32)for _ in range(8)]
        gp[1],gp[4]=context,stack+(i&15)
        struct.pack_into('<2I',bank,descriptor-base,table,count)
        struct.pack_into('<3I',bank,table-base,*(facet+n*0x50 for n in range(3)))
        struct.pack_into('<2I',bank,context+0x1b0-base,facet,clear_count)
        struct.pack_into('<2I',bank,context+0x2100-base,base+0x5000,2 if clear_count else 0)
        for n,(node,holder,a,b) in enumerate(zip(nodes,holders,first,second,strict=True)):
            following=nodes[n+1] if n+1<chain else (nodes[0] if chain==4 and count==1 else 0)
            struct.pack_into('<3I',bank,node-base,a,holder,0)
            struct.pack_into('<I',bank,holder-base,b)
            struct.pack_into('<4f',bank,a-base,float(n),1.0,-float(n),0.0)
            struct.pack_into('<4fI',bank,b-base,0.0,float(n),1.0,0.0,following)
        struct.pack_into('<4f',bank,third-base,1.0,0.0,1.0,0.0)
        struct.pack_into('<I',bank,third_holder-base,third)
        returned=0x63000000
        active=nodes[0] if chain else 0
        arg2=rng.getrandbits(32)
        struct.pack_into('<5I',bank,gp[4]-base,returned,descriptor,arg2,active,third_holder)
        cw=(0x7f,0x27f,0x37f)[i%3]|((i%4)<<10)
        sw=0x4100|(((i//3)%8)<<11)|(0x200 if i&1 else 0)
        mxcsr=0x1f80|((i%4)<<13)|(0x8040 if i&4 else 0)
        rows.append(dict(id=i,name=f'facet-ring-{i}',entry=ENTRY,gp=gp,
                         eflags=0x202|(rng.getrandbits(12)&0xcd5),return_pc=returned,matrix=descriptor,source=arg2,
                         before_fp=[cw,sw,mxcsr],live_x87=[live[j%4]for j in range((0,1,4,6,7,8)[i%6])],
                         xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],max_steps=800,
                         banks=[dict(address=base,before=bank.hex())]))
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
    paths = [SOURCE, ROOT / 'include/halo2/native_tu15_solver_hull_facet_ring_kernel.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_facet_ring_frame.hpp',
             ROOT / 'tests/native_tu15_solver_hull_facet_ring_frame_tests.cpp',
             ROOT / 'tools/validate_native_tu15_prt_helpers.py',
             ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp',
             ROOT / 'src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp',
             ROOT / 'src/physics/runtime/native_tu15_raw_x87_scope.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_facet_plane_frame.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_clear_frame.hpp',
             ROOT / 'tools/validate_native_solver_hull_facet_plane_frame_tu15.py',
             ROOT / 'tools/validate_native_solver_hull_clear_frame_tu15.py', Path(__file__)]
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
            raise ValueError('Complete actual314cb0 body drift')
        image = Xbe(a.xbe)
        if image.read(RET, 3).hex() != 'c21000':
            raise ValueError('Actual facet_ring return-word opcode drift')
        rows = inputs()
        data = scratch / 'inputs.json'
        data.write_text(json.dumps(rows, separators=(',', ':')))
        report['input_sha256'] = sha(data)
        child_bodies=(plane.actual_body(),clear.actual_body())
        if tuple(hashlib.sha256(child.encode()).hexdigest()for child in child_bodies)!=(plane.BODY_PIN,clear.BODY_PIN):
            raise ValueError('Complete genuine facet ring child body drift')
        reads_set,stack_set,stores=authored_io(body)
        explicit=reads_set|plane.EXPLICIT_READS|clear.EXPLICIT_READS
        stack_reads=stack_set|plane.STACK_READS|clear.STACK_READS
        io=sorted(reads_set|stack_set|stores|set(plane.IO)|set(clear.IO))
        traces=[dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=0)for pc in io]
        observed, proof = physical_original(image, rows, scratch, entries=(ENTRY,plane.ENTRY,clear.ENTRY),
            regions=(REGION,), trace_calls=traces, trace_extended=True)
        report['original'] = proof
        command = [a.compiler, '-std=c++20', '-Wno-ignored-attributes', '-O2', '-fno-fast-math',
                   '-ffp-contract=off', '-frounding-math', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'),
                   '-I' + str(ROOT / 'tests'), str(paths[3]), '-o', str(scratch / 'facet_ring')]
        if a.sanitizers:
            command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        report['command'] = command
        built = subprocess.run(command, capture_output=True, text=True)
        if built.returncode:
            raise ValueError('Private facet_ring compile failed: ' + built.stderr[-8000:])
        result = scratch / 'native.json'
        with data.open('rb') as inp, result.open('wb') as out:
            done = subprocess.run([command[-1]], stdin=inp, stdout=out, stderr=subprocess.PIPE)
        if done.returncode:
            raise ValueError('Private facet_ring stopped: ' + done.stderr.decode()[-8000:])
        actual = json.loads(result.read_text())
        visited, packets, reads, writes = set(), 0, 0, 0
        expected_rows = []
        for case, got in zip(observed['cases'], actual, strict=True):
            row = rows[case['id']]
            if not case['completed'] or not case['live_x87_unchanged']:
                raise ValueError('Actual facet_ring did not preserve its caller x87 stack')
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
                        raise ValueError('Wrong ordered original facet_ring read')
                    complete_reads.append(original_read)
            if next(memory_reads,None) is not None:
                raise ValueError('Unclassified original facet_ring read')
            want = dict(
                abi=dict(entry=RET, gp=case['boundary_gp'], eflags=case['boundary_eflags'],
                         xmm=case['boundary_xmm'], fp=case['after_fp'], live_x87=row['live_x87']),
                packets=[{k: v for k, v in packet.items() if k != 'arguments'} for packet in case['calls']],
                banks=sorted(case['banks'], key=lambda bank: bank['address']),
                reads=complete_reads, writes=plane.split_writes(case['writes']),
                plane_entries=sum(item['entry']==plane.ENTRY for item in case['calls']),
                clear_entries=sum(item['entry']==clear.ENTRY for item in case['calls']))
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
                raise ValueError('Complete unchanged original facet_ring differs: ' + row['name'] + ' ' + field)
            expected_rows.append(want)
            visited.update(case['executed_sites'])
            packets += len(case['calls'])
            reads += len(want['reads'])
            writes += len(case['writes'])
        sites = {int(value, 16) for value in re.findall(r'services.instruction\(0x([0-9a-f]+)u\)', body)}
        if len(sites) != 72 or not sites <= visited:
            raise ValueError('Missing authored original hull facet_ring sites')
        if a.mutations:
            relative = 'physics/runtime/native_tu15_solver_hull_facet_ring_frame.hpp'
            text = (ROOT / 'src' / relative).read_text()
            cases = (
                ('wrong_successor_facet','io_.load(0x314ceau,3u,g[7]+4u+g[0]*4u);',
                 'io_.load(0x314ceau,3u,g[7]+g[0]*4u);'),
                ('omitted_live_link_clear','io_.store(0x314d41u,g[0]+16u,0u);',''),
                ('wrong_return_node_comparison','s_.flags.minus(value,s_.gp[6],4u);','s_.flags.minus(value,value,4u);'),
            )
            report['compiled_defects'] = {}
            for name, old, new in cases:
                if text.count(old) != 1:
                    raise ValueError('Missing genuine facet_ring defect seam: ' + name)
                owned = scratch / name
                header = owned / relative
                header.parent.mkdir(parents=True)
                header.write_text(text.replace(old, new, 1))
                trial = command.copy()
                trial.insert(trial.index('-I' + str(ROOT / 'include')), '-I' + str(owned))
                trial[-1] = str(owned / 'facet_ring')
                built = subprocess.run(trial, capture_output=True, text=True, timeout=600)
                if built.returncode:
                    raise ValueError('Facet ring defect compile failed ' + name + ': ' + built.stderr[-5000:])
                with data.open('rb') as inp:
                    done = subprocess.run([trial[-1]], stdin=inp, capture_output=True, text=True, timeout=600)
                if done.returncode:
                    raise ValueError('Facet ring defect stopped before comparison ' + name + ': ' + done.stderr[-5000:])
                different = json.loads(done.stdout)
                first = next((n for n, (x, y) in enumerate(zip(expected_rows, different, strict=True)) if x != y), None)
                if first is None:
                    raise ValueError('Original i386 facet_ring did not reject ' + name)
                field = next(k for k in expected_rows[first] if expected_rows[first][k] != different[first][k])
                report['compiled_defects'][name] = dict(rejected=True, first_case=rows[first]['name'], first_field=field)
        report.update(passed=True, body_sha256=BODY_PIN, original_sites=len(sites), complete_original_calls=len(rows),
                      real_plane_entries=sum(row['plane_entries']for row in expected_rows),
                      real_clear_entries=sum(row['clear_entries']for row in expected_rows),
                      complete_parent_pre_io_packets=packets, architectural_reads=reads, complete_stores=writes,
                      hostile_provider_comparisons=len(rows), original_text_unmodified=True,
                      source_or_hardware_fault_domains_qualified=False)
    except Exception as error:
        report['error'] = repr(error)
    try:
        after = actual_body()
        report['sources_unchanged'] = all(sha(ROOT / name) == value for name, value in pins.items()) and (
            hashlib.sha256(after.encode()).hexdigest() == BODY_PIN) and (
            tuple(hashlib.sha256(child.encode()).hexdigest()for child in (plane.actual_body(),clear.actual_body()))
            ==(plane.BODY_PIN,clear.BODY_PIN))
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
