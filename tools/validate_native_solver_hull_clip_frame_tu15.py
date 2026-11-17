#!/usr/bin/env python3
"""Compare the private actual3147d0 clip frame to the original i386 owner."""
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
import validate_native_solver_hull_facet_clip_tu15 as legacy
from research_mp_sandbox_update_functions import Xbe
from validate_native_tu15_prt_helpers import PIN, physical_original

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'src/physics/physics_live_support_tu15_generated.cpp'
BODY_PIN = 'fb1f686d30c823296eba36bf95bea60040458217bbde6e6c4b869dfb7d1f81fd'
ENTRY = 0x3147d0
RET = 0x31486a
IO_DEPTH = {
    0x3147d0:0, 0x3147d9:0, 0x3147dc:0, 0x3147df:0, 0x3147e2:0, 0x3147e8:0,
    0x3147eb:0, 0x31480b:0, 0x31480f:0, 0x314813:1, 0x314815:0, 0x314816:0,
    0x314819:0, 0x314829:1, 0x314832:0, 0x314836:0, 0x31483a:1, 0x314840:1,
    0x314849:1, 0x314854:0, 0x314856:1, 0x314866:0, 0x314869:0, RET:0,
}
STORES = {0x3147d0,0x31480b,0x314815,0x314836,0x314849}
STACK_READS = {0x314866,0x314869,RET}
EXPLICIT_READS = set(IO_DEPTH)-STORES-STACK_READS
REGIONS = ((0x1000000,0x1000),(0x41d000,0x1000),(0x61000000,0x3000))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def actual_body():
    import native_solver_hull_clip_replacements as binding
    source=SOURCE.read_text()
    if binding.prefix_markers_present(source):
        if binding.installed_entries(source)!=['3147d0']:
            raise ValueError('Missing exact installed hull-clip source owner')
        source=binding.remove_solver_hull_clip(source)
    return next(body for sig, body in function_bodies(source) if sig.symbol == '3147d0')


def inputs(image):
    rng = random.Random(ENTRY)
    prior = legacy.cases(image)
    declared = [prior[n+k] for n in range(0,448,16) for k in (0,7)]
    declared += [case for case in prior if 'clip_lower_update' in case['name'] or 'bound_alias' in case['name']]
    declared += [prior[0],prior[4],prior[448],prior[450]]
    constant = image.read(0x41d000,0x1000)
    if len(constant) != 0x1000 or constant[0x7a0:0x7a4].hex() != prior[0]['banks'][1]['before']:
        raise ValueError('Exact original hull clip epsilon page drift')
    live = ('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f')
    rows = []
    for i,case in enumerate(declared):
        name = case['name']
        bank = bytearray.fromhex(case['banks'][0]['before'])
        stack = bytearray(rng.randbytes(0x3000))
        gp = [rng.getrandbits(32)for _ in range(8)]
        gp[1],gp[4] = case['receiver'],0x61002000+(i&15)
        point,lower,upper = case['arguments']
        if i == len(declared)-4:
            struct.pack_into('<I',bank,0x10c,0x7fc12345)
            struct.pack_into('<I',bank,0x30c,0xffc23456)
            name='paired-packed-nan'
        if i == len(declared)-3:
            struct.pack_into('<2I',bank,0x400,0,0)
            upper=((gp[4]-4)&~15)-4
            name='upper-projection-scratch-alias'
        if i == len(declared)-2:
            gp[5]=0
            lower=gp[4]-4
            name='lower-saved-bp-alias'
        if i == len(declared)-1:
            upper=gp[4]+12
            name='upper-argument-word-alias'
        returned = 0x63000000
        struct.pack_into('<4I',stack,gp[4]-0x61000000,returned,point,lower,upper)
        rows.append(dict(id=i,name=name,entry=ENTRY,gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),
                         return_pc=returned,matrix=point,source=lower,before_fp=case['before_fp'],
                         live_x87=[live[j%4]for j in range((0,1,4,7)[i%4])],
                         xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],max_steps=100,
                         banks=[dict(address=0x1000000,before=bank.hex()),dict(address=0x41d000,before=constant.hex()),
                                dict(address=0x61000000,before=stack.hex())]))
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
    paths = [SOURCE, ROOT / 'include/halo2/native_tu15_solver_hull_clip_kernel.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_clip_frame.hpp',
             ROOT / 'tests/native_tu15_solver_hull_clip_frame_tests.cpp',
             ROOT / 'tools/validate_native_tu15_prt_helpers.py',
             ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp',
             ROOT / 'src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp',
             ROOT / 'src/physics/runtime/native_tu15_raw_x87_scope.hpp',
             ROOT / 'tools/native_solver_hull_clip_replacements.py',
             ROOT / 'tools/validate_native_solver_hull_facet_clip_tu15.py', Path(__file__)]
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
            raise ValueError('Complete actual3147d0 body drift')
        image = Xbe(a.xbe)
        if image.read(RET, 3).hex() != 'c20c00':
            raise ValueError('Actual clip return-word opcode drift')
        rows = inputs(image)
        data = scratch / 'inputs.json'
        data.write_text(json.dumps(rows, separators=(',', ':')))
        report['input_sha256'] = sha(data)
        traces = [dict(entry=pc, arguments=0, gp=True, xmm=True, x87_delta=delta)
                  for pc, delta in IO_DEPTH.items()]
        observed, proof = physical_original(image, rows, scratch, entries=(ENTRY,),
            regions=REGIONS, trace_calls=traces, trace_extended=True)
        report['original'] = proof
        command = [a.compiler, '-std=c++20', '-Wno-ignored-attributes', '-O2', '-fno-fast-math',
                   '-ffp-contract=off', '-frounding-math', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'),
                   '-I' + str(ROOT / 'tests'), str(paths[3]), '-o', str(scratch / 'clip')]
        if a.sanitizers:
            command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        report['command'] = command
        built = subprocess.run(command, capture_output=True, text=True)
        if built.returncode:
            raise ValueError('Private clip compile failed: ' + built.stderr[-8000:])
        result = scratch / 'native.json'
        with data.open('rb') as inp, result.open('wb') as out:
            done = subprocess.run([command[-1]], stdin=inp, stdout=out, stderr=subprocess.PIPE)
        if done.returncode:
            raise ValueError('Private clip stopped: ' + done.stderr.decode()[-8000:])
        actual = json.loads(result.read_text())
        visited, packets, reads, writes = set(), 0, 0, 0
        expected_rows = []
        for case, got in zip(observed['cases'], actual, strict=True):
            row = rows[case['id']]
            if not case['completed'] or not case['live_x87_unchanged']:
                raise ValueError('Actual clip did not preserve its caller x87 stack')
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
                        raise ValueError('Wrong ordered original clip read')
                    complete_reads.append(original_read)
            if next(memory_reads,None) is not None:
                raise ValueError('Unclassified original clip read')
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
                raise ValueError('Complete unchanged original clip differs: ' + row['name'] + ' ' + field)
            expected_rows.append(want)
            visited.update(case['executed_sites'])
            packets += len(case['calls'])
            reads += len(want['reads'])
            writes += len(case['writes'])
        sites = {int(value, 16) for value in re.findall(r'services.instruction\(0x([0-9a-f]+)u\)', body)}
        if len(sites) != 56 or not sites <= visited:
            raise ValueError('Missing authored original hull clip sites')
        if a.mutations:
            relative = 'physics/runtime/native_tu15_solver_hull_clip_frame.hpp'
            text = (ROOT / 'src' / relative).read_text()
            cases = (
                ('wrong_final_compare_mask', 'test_status(1u);', 'test_status(0x41u);'),
                ('swapped_projection_mul_operands', 'x[0]=product(x[0],x[1]);', 'x[0]=product(x[1],x[0]);'),
                ('misdirected_upper_store', 'Base::store(0x314836u,g[6],g[0],[]{});',
                 'Base::store(0x314836u,g[1],g[0],[]{});'),
            )
            report['compiled_defects'] = {}
            for name, old, new in cases:
                if text.count(old) != 1:
                    raise ValueError('Missing genuine clip defect seam: ' + name)
                owned = scratch / name
                header = owned / relative
                header.parent.mkdir(parents=True)
                header.write_text(text.replace(old, new, 1))
                trial = command.copy()
                trial.insert(trial.index('-I' + str(ROOT / 'include')), '-I' + str(owned))
                trial[-1] = str(owned / 'clip')
                built = subprocess.run(trial, capture_output=True, text=True, timeout=600)
                if built.returncode:
                    raise ValueError('Clip defect compile failed ' + name + ': ' + built.stderr[-5000:])
                with data.open('rb') as inp:
                    done = subprocess.run([trial[-1]], stdin=inp, capture_output=True, text=True, timeout=600)
                if done.returncode:
                    raise ValueError('Clip defect stopped before comparison ' + name + ': ' + done.stderr[-5000:])
                different = json.loads(done.stdout)
                first = next((n for n, (x, y) in enumerate(zip(expected_rows, different, strict=True)) if x != y), None)
                if first is None:
                    raise ValueError('Original i386 clip did not reject ' + name)
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
