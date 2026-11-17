#!/usr/bin/env python3
"""Compare the actual 313BB0 seeded count classification before genuine child/return branches."""
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
BODY_PIN = '1abb6dc85a3bafd9a84eeb77188384091e10a8389419162c6b7b719f543f232a'
ENTRY = 0x313bb0
FIRST,LAST=0x313bfc,0x313dac
ENTRIES=(0x313d69,0x313d14,0x313c89,0x313bfc)
EXITS=(0x313db2,0x313e8e,0x313df1,0x313de0,0x313dcf,0x3141c2,0x314478)
REGIONS=((0x46a000,0x1000),(0x61000000,0x8000))
DEPTH_ONE={0x313c66,0x313c92,0x313c96,0x313c99,0x313ccb,0x313ced,0x313cf1,0x313d46,0x313d94}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def actual_body():
    return next(body for sig, body in function_bodies(SOURCE.read_text()) if sig.symbol == '313bb0')


def authored_io(body):
    reads,stack,stores=set(),set(),set()
    for line in body.splitlines():
        site=re.search(r'services.instruction\(0x([0-9a-f]+)u\)',line)
        if not site:
            continue
        pc=int(site[1],16)
        if not FIRST<=pc<=LAST:continue
        if re.search(r'\b(?:r|v)\(s.memory',line):reads.add(pc)
        if 's.pop()' in line:stack.add(pc)
        if re.search(r'\b(?:w|vw)\(s.memory|s.push\(',line):stores.add(pc)
    return reads,stack,stores



def inputs(image):
    rng=random.Random(ENTRY)
    base,stack,context,configuration,points=(0x61000000,0x61007000,0x61001000,0x61002000,0x61003000)
    const=dict(address=0x46a000,before=image.read(0x46a000,0x1000).hex())
    live=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f')
    variants=(
        ('solid',((1,0,0),(0,1,0),(0,0,1),(-1,-1,-1))),
        ('plane',((1,0,0),(0,1,0),(0,0,0),(1,1,0))),
        ('line',((1,0,0),(2,0,0),(3,0,0),(4,0,0))),
        ('point',((2,0,0),)*4),
        ('zero',((0,0,0),)*4),
        ('nan-w',((1,0,0),(0,1,0),(0,0,1),(-1,-1,-1))),
    )
    rows=[]
    for count,entry in enumerate(ENTRIES,1):
        for name,vectors in variants:
            for already_attempted in (0xffffffff,0,2,3):
                i=len(rows);bank=bytearray(rng.randbytes(0x8000))
                gp=[rng.getrandbits(32)for _ in range(8)];gp[6],gp[4],gp[7]=context,stack,already_attempted
                struct.pack_into('<I',bank,context+0x48-base,configuration)
                struct.pack_into('<I',bank,context+0x50-base,points)
                struct.pack_into('<I',bank,configuration+0x80-base,count)
                for n,v in enumerate(vectors):struct.pack_into('<4f',bank,points-base+n*16,*v,-0.)
                if name=='nan-w':
                    struct.pack_into('<I',bank,points-base+12,0x7fc12345)
                    struct.pack_into('<I',bank,points-base+28,0xffc23456)
                returned=0x63000000;arg=rng.getrandbits(32);other=rng.getrandbits(32)
                struct.pack_into('<3I',bank,gp[4]-base,returned,arg,other)
                cw=(0x7f,0x27f,0x37f)[i%3]|((i%4)<<10)
                sw=0x4100|(((i//3)%8)<<11)|(0x200 if i&1 else 0)
                mxcsr=0x1f80|((i%4)<<13)|(0x8040 if i&4 else 0)
                rows.append(dict(id=i,name=f'seed-{count}-{name}-{already_attempted:x}',entry=entry,gp=gp,
                    eflags=0x202|(rng.getrandbits(12)&0xcd5),return_pc=returned,matrix=arg,source=other,
                    before_fp=[cw,sw,mxcsr],live_x87=[live[j%4]for j in range((0,1,4,6)[i%4])],
                    xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],max_steps=200,
                    stop_before_entries=list(EXITS),banks=[const,dict(address=base,before=bank.hex())]))
    return rows


def split_writes(writes):
    result=[]
    for event in writes:
        if event['instruction'] not in {0x313c33,0x313ccb}:
            result.append(event);continue
        if len(event['written'])!=32 or len(event['before'])!=32:
            raise ValueError('Expected complete original seed-route vector store')
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
    paths = [SOURCE, ROOT / 'include/halo2/native_tu15_solver_hull_seed_route_kernel.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_seed_route_frame.hpp',
             ROOT / 'tests/native_tu15_solver_hull_seed_route_frame_tests.cpp',
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
            raise ValueError('Complete actual313bb0 body drift')
        image = Xbe(a.xbe)
        rows = inputs(image)
        data = scratch / 'inputs.json'
        data.write_text(json.dumps(rows, separators=(',', ':')))
        report['input_sha256'] = sha(data)
        explicit,stack_reads,stores=authored_io(body)
        io=sorted(explicit|stack_reads|stores)
        traces=[dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=int(pc in DEPTH_ONE))for pc in io]
        observed, proof = physical_original(image, rows, scratch, entries=(ENTRY,),
            regions=REGIONS, trace_calls=traces, trace_extended=True,preserve_x87_prefix=True)
        report['original'] = proof
        command = [a.compiler, '-std=c++20', '-Wno-ignored-attributes', '-O2', '-fno-fast-math',
                   '-ffp-contract=off', '-frounding-math', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'),
                   '-I' + str(ROOT / 'tests'), str(paths[3]), '-o', str(scratch / 'seed_route')]
        if a.sanitizers:
            command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        report['command'] = command
        built = subprocess.run(command, capture_output=True, text=True)
        if built.returncode:
            raise ValueError('Private seed_route compile failed: ' + built.stderr[-8000:])
        result = scratch / 'native.json'
        with data.open('rb') as inp, result.open('wb') as out:
            done = subprocess.run([command[-1]], stdin=inp, stdout=out, stderr=subprocess.PIPE)
        if done.returncode:
            raise ValueError('Private seed_route stopped: ' + done.stderr.decode()[-8000:])
        actual = json.loads(result.read_text())
        visited, packets, reads, writes = set(), 0, 0, 0
        expected_rows = []
        for case, got in zip(observed['cases'], actual, strict=True):
            row = rows[case['id']]
            if case.get('stopped_at') not in EXITS or not case['live_x87_unchanged']:
                raise ValueError('Actual seed_route did not preserve its caller x87 stack')
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
                        raise ValueError('Wrong ordered original seed_route read')
                    complete_reads.append(original_read)
            if next(memory_reads,None) is not None:
                raise ValueError('Unclassified original seed_route read')
            want = dict(
                abi=dict(entry=case['stopped_at'], gp=case['boundary_gp'], eflags=case['boundary_eflags'],
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
                raise ValueError('Complete unchanged original seed_route differs: ' + row['name'] + ' ' + field)
            expected_rows.append(want)
            visited.update(case['executed_sites'])
            packets += len(case['calls'])
            reads += len(want['reads'])
            writes += len(case['writes'])
        sites = {int(value, 16) for value in re.findall(r'services.instruction\(0x([0-9a-f]+)u\)', body)}
        sites={pc for pc in sites if FIRST<=pc<=LAST};missing=sorted(sites-visited)
        report['missing_sites']=[hex(pc)for pc in missing];report['discovered_stage_sites']=len(sites)
        if len(sites)!=118 or missing or {row['abi']['entry']for row in expected_rows}!=set(EXITS):
            raise ValueError('Missing authored original hull seed_route sites')
        if a.mutations:
            relative = 'physics/runtime/native_tu15_solver_hull_seed_route_frame.hpp'
            text = (ROOT / 'src' / relative).read_text()
            cases = (
                ('wrong_four_threshold','io_.compare_float(0x313c66u,0x46a180u)','io_.compare_float(0x313c66u,0x46a0e8u)'),
                ('lost_count_decrement','flags.increment(Base::word(address),4u,true)','flags.increment(Base::word(address),4u,false)'),
                ('wrong_triangle_mask','compare_pop_pair();status_test(0x44u)','compare_pop_pair();status_test(0x04u)'),
            )
            report['compiled_defects'] = {}
            for name, old, new in cases:
                if text.count(old) != 1:
                    raise ValueError('Missing genuine seed_route defect seam: ' + name)
                owned = scratch / name
                header = owned / relative
                header.parent.mkdir(parents=True)
                header.write_text(text.replace(old, new, 1))
                trial = command.copy()
                trial.insert(trial.index('-I' + str(ROOT / 'include')), '-I' + str(owned))
                trial[-1] = str(owned / 'seed_route')
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
                    raise ValueError('Original i386 seed_route did not reject ' + name)
                field = next(k for k in expected_rows[first] if expected_rows[first][k] != different[first][k])
                report['compiled_defects'][name] = dict(rejected=True, first_case=rows[first]['name'], first_field=field)
        report.update(passed=True, body_sha256=BODY_PIN, original_sites=len(sites), executed_original_sites=len(sites&visited),complete_original_calls=0,prefix_paths=len(rows),
                      reached_continuations=sorted(hex(pc)for pc in {row['abi']['entry']for row in expected_rows}),
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
