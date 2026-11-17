#!/usr/bin/env python3
"""Compare the private actual3122f0 barycentric projection frame to the original i386 owner."""
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
BODY_PIN = '2f73873ee31d27fdd0bc1d8665c8e618c43930173e5cd6b90ee62382138228a7'
ENTRY = 0x3122f0
RETURNS = (0x312455,0x312504,0x312515,0x312566,0x3125c2,0x312611)
REGIONS = ((0x46a000,0x1000),(0x61000000,0x8000))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def actual_body():
    return next(body for sig, body in function_bodies(SOURCE.read_text()) if sig.symbol == '3122f0')


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
    base,stack=0x61000000,0x61007000
    query,a_ptr,b_ptr,c_ptr=(base+offset for offset in (0x2100,0x2200,0x2300,0x2400))
    const=dict(address=0x46a000,before=image.read(0x46a000,0x1000).hex())
    live=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f')
    shapes=(
        ('triangle',(0.,0.),(4.,0.),(0.,3.)),
        ('ab',(0.,0.),(4.,0.),(1.,0.)),
        ('ac',(0.,0.),(1.,0.),(4.,0.)),
        ('bc-save',(2.,0.),(0.,0.),(3.,0.)),
        ('bc-duplicate',(1.,0.),(0.,0.),(4.,0.)),
        ('point',(1.,0.),(1.,0.),(1.,0.)),
        ('nan-weights',(0.,0.),(1.,0.),(0.,2.)),
        ('unalike-scales',(0.,0.),(0.0001,0.),(0.,8192.)),
    )
    rows=[]
    for name,a,b,c in shapes:
        for repeat in range(8):
            i=len(rows);bank=bytearray(rng.randbytes(0x8000));gp=[rng.getrandbits(32)for _ in range(8)]
            gp[4]=stack+(i&15)
            out=base+0x2600
            aligned=(gp[4]-4)&~15
            if repeat==6:out=aligned-12 # First output aliases determinant; later numerator reload must see it.
            if repeat==7:out=gp[4]-12 # Third output aliases the saved BP, preserving the actual return word.
            for ptr,(x,y) in ((a_ptr,a),(b_ptr,b),(c_ptr,c),(query,(0.25*repeat-1.,0.33*repeat))):
                struct.pack_into('<4f',bank,ptr-base,x,y,0.,-0.)
            if name=='nan-weights':
                struct.pack_into('<I',bank,a_ptr-base+12,0x7fc12345)
                struct.pack_into('<I',bank,query-base+12,0xffc23456)
                if repeat==5:struct.pack_into('<I',bank,query-base+4,0x7fc34567)
            returned=0x63000000
            struct.pack_into('<6I',bank,gp[4]-base,returned,query,b_ptr,a_ptr,c_ptr,out)
            cw=(0x7f,0x27f,0x37f)[i%3]|((i%4)<<10)
            sw=0x4100|(((i//3)%8)<<11)|(0x200 if i&1 else 0)
            mxcsr=0x1f80|((i%4)<<13)|(0x8040 if i&4 else 0)
            rows.append(dict(id=i,name=f'{name}-{repeat}',entry=ENTRY,gp=gp,
                             eflags=0x202|(rng.getrandbits(12)&0xcd5),return_pc=returned,matrix=query,source=b_ptr,
                             before_fp=[cw,sw,mxcsr],live_x87=[live[j%4]for j in range((0,1,4,2)[repeat%4])],
                             xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],max_steps=300,
                             banks=[const,dict(address=base,before=bank.hex())]))
    return rows


def site_depths(body):
    nodes={}
    for line in body.splitlines():
        authored=re.search(r'services.instruction\(0x([0-9a-f]+)u\);\} (.*)',line)
        if authored:nodes[int(authored[1],16)]=authored[2]
    order=list(nodes)
    following=dict(zip(order,order[1:]))
    depths={ENTRY:0};pending=[ENTRY]
    while pending:
        pc=pending.pop();op=nodes[pc]
        out=depths[pc]+op.count('s.fp.push(')-op.count('s.fp.pop()')-int('s.fp.comparison('in op)
        if 'return;' in op:
            if out!=0 or pc not in RETURNS:raise ValueError('Barycentric x87 return depth drift')
            continue
        target=re.search(r'goto L([0-9a-f]+);',op)
        successors=[] if target and not op.startswith('if(') else [following[pc]]
        if target:successors.append(int(target[1],16))
        for next_pc in successors:
            if next_pc not in depths:depths[next_pc]=out;pending.append(next_pc)
            elif depths[next_pc]!=out:raise ValueError('Ambiguous authored x87 depth')
    if len(nodes)!=242 or nodes.keys()!=depths.keys() or min(depths.values())!=0 or max(depths.values())!=4:
        raise ValueError('Complete barycentric x87 depth/site inventory drift')
    return depths


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
    paths = [SOURCE, ROOT / 'include/halo2/native_tu15_solver_hull_barycentric_kernel.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_barycentric_frame.hpp',
             ROOT / 'tests/native_tu15_solver_hull_barycentric_frame_tests.cpp',
             ROOT / 'tools/validate_native_tu15_prt_helpers.py',
             ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp',
             ROOT / 'src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp',
             ROOT / 'src/physics/runtime/native_tu15_raw_x87_scope.hpp',
             ROOT / 'tests/native_tu15_mopp_ray_frame_test_memory.hpp',Path(__file__)]
    # Pin the complete actual body before and after, independent of unrelated Primary additions.
    pins = {str(path.relative_to(ROOT)): sha(path) for path in paths if path != SOURCE}
    report = dict(passed=False, production_bound=False, scratch_owner=str(scratch), source_sha256=pins,
                  retained_large_artifacts=[])
    try:
        if sha(a.xbe) != PIN:
            raise ValueError('Wrong TU1.5 original image')
        body = actual_body()
        if hashlib.sha256(body.encode()).hexdigest() != BODY_PIN:
            raise ValueError('Complete actual3122f0 body drift')
        image = Xbe(a.xbe)
        if any(image.read(pc,1).hex()!='c3' for pc in RETURNS):
            raise ValueError('Actual barycentric return-word opcode drift')
        rows = inputs(image)
        data = scratch / 'inputs.json'
        data.write_text(json.dumps(rows, separators=(',', ':')))
        report['input_sha256'] = sha(data)
        explicit,stack_reads,stores=authored_io(body)
        io=sorted(explicit|stack_reads|stores)
        depths=site_depths(body)
        traces=[dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=depths[pc])for pc in io]
        observed, proof = physical_original(image, rows, scratch, entries=(ENTRY,),
            regions=REGIONS, trace_calls=traces, trace_extended=True)
        report['original'] = proof
        command = [a.compiler, '-std=c++20', '-Wno-ignored-attributes', '-O2', '-fno-fast-math',
                   '-ffp-contract=off', '-frounding-math', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'),
                   '-I' + str(ROOT / 'tests'), str(paths[3]), '-o', str(scratch / 'barycentric')]
        if a.sanitizers:
            command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        report['command'] = command
        built = subprocess.run(command, capture_output=True, text=True)
        if built.returncode:
            raise ValueError('Private barycentric compile failed: ' + built.stderr[-8000:])
        result = scratch / 'native.json'
        with data.open('rb') as inp, result.open('wb') as out:
            done = subprocess.run([command[-1]], stdin=inp, stdout=out, stderr=subprocess.PIPE)
        if done.returncode:
            raise ValueError('Private barycentric stopped: ' + done.stderr.decode()[-8000:])
        actual = json.loads(result.read_text())
        visited, packets, reads, writes = set(), 0, 0, 0
        expected_rows = []
        for case, got in zip(observed['cases'], actual, strict=True):
            row = rows[case['id']]
            if not case['completed'] or not case['live_x87_unchanged']:
                raise ValueError('Actual barycentric did not preserve its caller x87 stack')
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
                        raise ValueError('Wrong ordered original barycentric read')
                    complete_reads.append(original_read)
            if next(memory_reads,None) is not None:
                raise ValueError('Unclassified original barycentric read')
            want = dict(
                abi=dict(entry=next(pc for pc in RETURNS if pc in case['executed_sites']), gp=case['boundary_gp'], eflags=case['boundary_eflags'],
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
                raise ValueError('Complete unchanged original barycentric differs: ' + row['name'] + ' ' + field)
            expected_rows.append(want)
            visited.update(case['executed_sites'])
            packets += len(case['calls'])
            reads += len(want['reads'])
            writes += len(case['writes'])
        sites = {int(value, 16) for value in re.findall(r'services.instruction\(0x([0-9a-f]+)u\)', body)}
        missing=sorted(sites-visited)
        report['missing_sites']=[hex(pc)for pc in missing]
        if len(sites) != 242 or missing:
            raise ValueError('Missing authored original hull barycentric sites')
        if a.mutations:
            relative = 'physics/runtime/native_tu15_solver_hull_barycentric_frame.hpp'
            text = (ROOT / 'src' / relative).read_text()
            cases = (
                ('inverted_determinant','stack(Stack::subtract_pop);io_.spill(0x312395u','stack(Stack::discard);io_.spill(0x312395u'),
                ('omitted_triangle_weight','io_.spill(0x312434u,g[0],true)','io_.spill(0x312434u,g[0]+4u,true)'),
                ('wrong_degenerate_edge','g[5]+24u);io_.store(0x31254bu,g[0]+8u,0u)','g[5]+24u);io_.store(0x31254bu,g[0]+4u,0u)'),
            )
            report['compiled_defects'] = {}
            for name, old, new in cases:
                if text.count(old) != 1:
                    raise ValueError('Missing genuine barycentric defect seam: ' + name)
                owned = scratch / name
                header = owned / relative
                header.parent.mkdir(parents=True)
                header.write_text(text.replace(old, new, 1))
                trial = command.copy()
                trial.insert(trial.index('-I' + str(ROOT / 'include')), '-I' + str(owned))
                trial[-1] = str(owned / 'barycentric')
                built = subprocess.run(trial, capture_output=True, text=True, timeout=600)
                if built.returncode:
                    raise ValueError('Barycentric defect compile failed ' + name + ': ' + built.stderr[-5000:])
                with data.open('rb') as inp:
                    done = subprocess.run([trial[-1]], stdin=inp, capture_output=True, text=True, timeout=600)
                if done.returncode:
                    raise ValueError('Barycentric defect stopped before comparison ' + name + ': ' + done.stderr[-5000:])
                different = json.loads(done.stdout)
                first = next((n for n, (x, y) in enumerate(zip(expected_rows, different, strict=True)) if x != y), None)
                if first is None:
                    raise ValueError('Original i386 barycentric did not reject ' + name)
                field = next(k for k in expected_rows[first] if expected_rows[first][k] != different[first][k])
                report['compiled_defects'][name] = dict(rejected=True, first_case=rows[first]['name'], first_field=field)
        report.update(passed=True, body_sha256=BODY_PIN, original_sites=len(sites), executed_original_sites=len(sites&visited), complete_original_calls=len(rows),
                      original_return_paths=sorted(hex(pc)for pc in RETURNS if pc in visited),
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
