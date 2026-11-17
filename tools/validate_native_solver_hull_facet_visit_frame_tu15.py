#!/usr/bin/env python3
"""Compare the private actual3148e0 recursive facet visit frame to the original i386 owner."""
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
import validate_native_solver_hull_clip_frame_tu15 as clip
from research_mp_sandbox_update_functions import Xbe
from validate_native_tu15_prt_helpers import PIN, physical_original

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'src/physics/physics_live_support_tu15_generated.cpp'
BODY_PIN = '1784902fe0c372d89787a718641f780a71e377f7e1ccd902babcd2cc87890c58'
ENTRY = 0x3148e0
RET = 0x3149a4
REGIONS = ((0x41d000,0x1000),(0x61000000,0x8000))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def actual_body():
    return next(body for sig, body in function_bodies(SOURCE.read_text()) if sig.symbol == '3148e0')


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
    context,stack,point,origin=(base+offset for offset in (0x1000,0x7000,0x4400,0x4600))
    count,link,reuse=(base+offset for offset in (0x1180,0x1190,0x5400))
    facets=[base+0x2000+0x80*n for n in range(8)]
    edges=[base+0x3100+0x40*n for n in range(24)]
    targets=((1,2,3),(4,5,6),(4,6,7),(5,6,7),(5,6,7),(5,6,7),(5,6,7),(5,6,7))
    live=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f')
    constants=image.read(0x41d000,0x1000)
    plans=(
        ('ready-and-collected',(2,1,2,3,2,1,2,3),-1.0,3.0,0.0),
        ('single-recursion',(2,0,1,2,3,1,2,1),-1.0,3.0,0.0),
        ('nested-recursion',(2,0,1,2,0,1,2,1),-1.0,3.0,0.0),
        ('all-root-neighbors',(2,0,0,0,2,2,2,2),-1.0,3.0,0.0),
        ('clip-below-lower',(2,0,2,1,2,1,2,1),1.0,3.0,0.0),
        ('direct-stop',(2,0,2,1,2,1,2,1),3.0,1.0,0.0),
        ('recursive-stop',(2,0,1,2,0,1,2,1),0.0,3.0,1.0e-7),
        ('positive-upper-update',(2,0,2,1,2,1,2,1),-1.0,4.0,2.0),
    )
    rows=[]
    for name,states,lower,upper,p in plans:
        for repetition in range(6):
            i=len(rows);bank=bytearray(rng.randbytes(0x8000))
            gp=[rng.getrandbits(32)for _ in range(8)]
            gp[1],gp[4]=context,stack+(i&15)
            struct.pack_into('<2f2I',bank,context-base,lower,upper,reuse,repetition%3)
            struct.pack_into('<2I',bank,count-base,0,0)
            struct.pack_into('<4f',bank,point-base,p,0.0,0.0,0.0)
            struct.pack_into('<4f',bank,origin-base,0.0,0.0,0.0,0.0)
            for n,facet in enumerate(facets):
                sign=1.0
                if name=='recursive-stop' and n==4:
                    # After the first genuine clip, the second projection can
                    # meet its narrowed upper bound and take the stop branch.
                    sign=0.0
                struct.pack_into('<4f',bank,facet-base,sign,0.0,0.0,0.0)
                struct.pack_into('<I',bank,facet+20-base,origin)
                struct.pack_into('<B',bank,facet+68-base,states[n])
                for edge_index,neighbor in enumerate(targets[n]):
                    k=n*3+edge_index;edge=edges[k]
                    struct.pack_into('<I',bank,facet+28+edge_index*16-base,edge)
                    struct.pack_into('<I',bank,edge-base,base+0x5600+0x20*k)
                    struct.pack_into('<I',bank,edge+12-base,facets[neighbor])
            returned=0x63000000
            struct.pack_into('<5I',bank,gp[4]-base,returned,count,link,facets[0],point)
            cw=(0x7f,0x27f,0x37f)[i%3]|((i%4)<<10)
            sw=0x4100|(((i//3)%8)<<11)|(0x200 if i&1 else 0)
            mxcsr=0x1f80|((i%4)<<13)|(0x8040 if i&4 else 0)
            rows.append(dict(id=i,name=f'{name}-{repetition}',entry=ENTRY,gp=gp,
                         eflags=0x202|(rng.getrandbits(12)&0xcd5),return_pc=returned,matrix=count,source=link,
                         before_fp=[cw,sw,mxcsr],live_x87=[live[j%4]for j in range((0,1,4,6,7,2)[repetition])],
                         xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],max_steps=1700,
                         banks=[dict(address=0x41d000,before=constants.hex()),dict(address=base,before=bank.hex())]))
    return rows


def child_body():
    result=clip.actual_body()
    if hashlib.sha256(result.encode()).hexdigest()!=clip.BODY_PIN:
        raise ValueError('Complete genuine facet-visit Clip child drift')
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
    paths = [SOURCE, ROOT / 'include/halo2/native_tu15_solver_hull_facet_visit_kernel.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_facet_visit_frame.hpp',
             ROOT / 'tests/native_tu15_solver_hull_facet_visit_frame_tests.cpp',
             ROOT / 'tools/validate_native_tu15_prt_helpers.py',
             ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp',
             ROOT / 'src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp',
             ROOT / 'src/physics/runtime/native_tu15_raw_x87_scope.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_clip_frame.hpp',
             ROOT / 'tools/validate_native_solver_hull_clip_frame_tu15.py',
             ROOT / 'tools/native_solver_hull_clip_replacements.py', Path(__file__)]
    # Pin both complete actual bodies before and after, independent of unrelated Primary additions.
    pins = {str(path.relative_to(ROOT)): sha(path) for path in paths if path != SOURCE}
    report = dict(passed=False, production_bound=False, scratch_owner=str(scratch), source_sha256=pins,
                  retained_large_artifacts=[])
    try:
        if sha(a.xbe) != PIN:
            raise ValueError('Wrong TU1.5 original image')
        body = actual_body()
        if hashlib.sha256(body.encode()).hexdigest() != BODY_PIN:
            raise ValueError('Complete actual3148e0 body drift')
        image = Xbe(a.xbe)
        if image.read(RET, 3).hex() != 'c21000':
            raise ValueError('Actual facet_visit return-word opcode drift')
        rows = inputs(image)
        data = scratch / 'inputs.json'
        data.write_text(json.dumps(rows, separators=(',', ':')))
        report['input_sha256'] = sha(data)
        child_body()
        reads_set,stack_set,stores=authored_io(body)
        explicit=reads_set|clip.EXPLICIT_READS
        stack_reads=stack_set|clip.STACK_READS
        io=sorted(reads_set|stack_set|stores|set(clip.IO_DEPTH)|{ENTRY})
        traces=[dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=clip.IO_DEPTH.get(pc,0))for pc in io]
        observed, proof = physical_original(image, rows, scratch, entries=(ENTRY,clip.ENTRY),
            regions=REGIONS, trace_calls=traces, trace_extended=True)
        report['original'] = proof
        command = [a.compiler, '-std=c++20', '-Wno-ignored-attributes', '-O2', '-fno-fast-math',
                   '-ffp-contract=off', '-frounding-math', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'),
                   '-I' + str(ROOT / 'tests'), str(paths[3]), '-o', str(scratch / 'facet_visit')]
        if a.sanitizers:
            command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        report['command'] = command
        built = subprocess.run(command, capture_output=True, text=True)
        if built.returncode:
            raise ValueError('Private facet_visit compile failed: ' + built.stderr[-8000:])
        result = scratch / 'native.json'
        with data.open('rb') as inp, result.open('wb') as out:
            done = subprocess.run([command[-1]], stdin=inp, stdout=out, stderr=subprocess.PIPE)
        if done.returncode:
            raise ValueError('Private facet_visit stopped: ' + done.stderr.decode()[-8000:])
        actual = json.loads(result.read_text())
        visited, packets, reads, writes = set(), 0, 0, 0
        expected_rows = []
        for case, got in zip(observed['cases'], actual, strict=True):
            row = rows[case['id']]
            if not case['completed'] or not case['live_x87_unchanged']:
                raise ValueError('Actual facet_visit did not preserve its caller x87 stack')
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
                        raise ValueError('Wrong ordered original facet_visit read')
                    complete_reads.append(original_read)
            if next(memory_reads,None) is not None:
                raise ValueError('Unclassified original facet_visit read')
            want = dict(
                abi=dict(entry=RET, gp=case['boundary_gp'], eflags=case['boundary_eflags'],
                         xmm=case['boundary_xmm'], fp=case['after_fp'], live_x87=row['live_x87']),
                packets=[{k: v for k, v in packet.items() if k != 'arguments'} for packet in case['calls']],
                banks=sorted(case['banks'], key=lambda bank: bank['address']),
                reads=complete_reads, writes=case['writes'],
                visit_entries=sum(item['entry']==ENTRY for item in case['calls']),
                clip_entries=sum(item['entry']==clip.ENTRY for item in case['calls']))
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
                raise ValueError('Complete unchanged original facet_visit differs: ' + row['name'] + ' ' + field)
            expected_rows.append(want)
            visited.update(case['executed_sites'])
            packets += len(case['calls'])
            reads += len(want['reads'])
            writes += len(case['writes'])
        sites = {int(value, 16) for value in re.findall(r'services.instruction\(0x([0-9a-f]+)u\)', body)}
        missing=sorted(sites-visited)
        report['missing_sites']=[hex(pc)for pc in missing]
        if len(sites) != 74 or missing:
            raise ValueError('Missing authored original hull facet_visit sites')
        if a.mutations:
            relative = 'physics/runtime/native_tu15_solver_hull_facet_visit_frame.hpp'
            text = (ROOT / 'src' / relative).read_text()
            cases = (
                ('wrong_link_stride','s_.flags.plus(g[2],16u,4u)','s_.flags.plus(g[2],0u,4u)'),
                ('omitted_collected_count','io_.increment(0x314960u,g[0]);',''),
                ('wrong_stop_relation','s_.flags.minus(s_.gp[0]&0xffu,4u,1u)','s_.flags.minus(s_.gp[0]&0xffu,2u,1u)'),
            )
            report['compiled_defects'] = {}
            for name, old, new in cases:
                if text.count(old) != 1:
                    raise ValueError('Missing genuine facet_visit defect seam: ' + name)
                owned = scratch / name
                header = owned / relative
                header.parent.mkdir(parents=True)
                header.write_text(text.replace(old, new, 1))
                trial = command.copy()
                trial.insert(trial.index('-I' + str(ROOT / 'include')), '-I' + str(owned))
                trial[-1] = str(owned / 'facet_visit')
                built = subprocess.run(trial, capture_output=True, text=True, timeout=600)
                if built.returncode:
                    raise ValueError('Facet visit defect compile failed ' + name + ': ' + built.stderr[-5000:])
                with data.open('rb') as inp:
                    done = subprocess.run([trial[-1]], stdin=inp, capture_output=True, text=True, timeout=600)
                if done.returncode:
                    raise ValueError('Facet visit defect stopped before comparison ' + name + ': ' + done.stderr[-5000:])
                different = json.loads(done.stdout)
                first = next((n for n, (x, y) in enumerate(zip(expected_rows, different, strict=True)) if x != y), None)
                if first is None:
                    raise ValueError('Original i386 facet_visit did not reject ' + name)
                field = next(k for k in expected_rows[first] if expected_rows[first][k] != different[first][k])
                report['compiled_defects'][name] = dict(rejected=True, first_case=rows[first]['name'], first_field=field)
        report.update(passed=True, body_sha256=BODY_PIN, original_sites=len(sites), executed_original_sites=len(sites&visited), complete_original_calls=len(rows),
                      genuine_recursive_entries=sum(row['visit_entries']for row in expected_rows)-len(rows),
                      genuine_clip_entries=sum(row['clip_entries']for row in expected_rows),
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
