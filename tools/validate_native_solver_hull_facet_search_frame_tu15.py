#!/usr/bin/env python3
"""Compare the private actual314c40 facet search frame to the original i386 owner."""
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
import validate_native_solver_hull_facet_visit_frame_tu15 as visit
from research_mp_sandbox_update_functions import Xbe
from validate_native_tu15_prt_helpers import PIN, physical_original

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'src/physics/physics_live_support_tu15_generated.cpp'
BODY_PIN = 'cf064e4ddf5197668cf787167332cdae868d3559178c9b348cdb509ddbb3f3d0'
ENTRY = 0x314c40
RET = 0x314c96
REGIONS = ((0x41d000,0x1000),(0x61000000,0x8000))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def actual_body():
    return next(body for sig, body in function_bodies(SOURCE.read_text()) if sig.symbol == '314c40')


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
    base=0x61000000
    rows=visit.inputs(image)
    for row in rows:
        i=row['id'];sp=row['gp'][4];context=row['gp'][1]
        bank=bytearray.fromhex(row['banks'][1]['before'])
        count,link,facet,point=struct.unpack_from('<4I',bank,sp+4-base)
        struct.pack_into('<2I',bank,sp+4-base,link,count)
        # The seed narrows the initially open interval. A genuine zero
        # or NaN projection refuses the recursive walk.
        struct.pack_into('<I',bank,point-base,0 if i%6==0 else 0x7fc12345 if i%6==1 else 0x3f800000)
        struct.pack_into('<f',bank,facet-base,2.0)
        if row['name'].startswith('recursive-stop'):
            struct.pack_into('<f',bank,facet+0x80-base,1.01e-5)
        row.update(entry=ENTRY,name='search-'+row['name'],matrix=link,source=count,max_steps=2000)
        row['banks'][1]['before']=bank.hex()
    return rows


def child_bodies():
    result=(clip.actual_body(),visit.actual_body())
    if tuple(hashlib.sha256(body.encode()).hexdigest() for body in result)!=(clip.BODY_PIN,visit.BODY_PIN):
        raise ValueError('Complete genuine facet-search child body drift')
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
    paths = [SOURCE, ROOT / 'include/halo2/native_tu15_solver_hull_facet_search_kernel.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_facet_search_frame.hpp',
             ROOT / 'tests/native_tu15_solver_hull_facet_search_frame_tests.cpp',
             ROOT / 'tools/validate_native_tu15_prt_helpers.py',
             ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp',
             ROOT / 'src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp',
             ROOT / 'src/physics/runtime/native_tu15_raw_x87_scope.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_clip_frame.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_facet_visit_frame.hpp',
             ROOT / 'tools/validate_native_solver_hull_facet_visit_frame_tu15.py',
             ROOT / 'tools/validate_native_solver_hull_clip_frame_tu15.py',
             ROOT / 'tools/native_solver_hull_clip_replacements.py', Path(__file__)]
    # Pin all three complete actual bodies before and after, independent of unrelated Primary additions.
    pins = {str(path.relative_to(ROOT)): sha(path) for path in paths if path != SOURCE}
    report = dict(passed=False, production_bound=False, scratch_owner=str(scratch), source_sha256=pins,
                  retained_large_artifacts=[])
    try:
        if sha(a.xbe) != PIN:
            raise ValueError('Wrong TU1.5 original image')
        body = actual_body()
        if hashlib.sha256(body.encode()).hexdigest() != BODY_PIN:
            raise ValueError('Complete actual314c40 body drift')
        image = Xbe(a.xbe)
        if any(image.read(pc,3).hex()!='c21000' for pc in (RET,0x314ca2)):
            raise ValueError('Actual facet_search return-word opcode drift')
        rows = inputs(image)
        data = scratch / 'inputs.json'
        data.write_text(json.dumps(rows, separators=(',', ':')))
        report['input_sha256'] = sha(data)
        _,visit_body=child_bodies()
        reads_set,stack_set,stores=authored_io(body)
        child_reads,child_stack,child_stores=visit.authored_io(visit_body)
        explicit=reads_set|child_reads|clip.EXPLICIT_READS
        stack_reads=stack_set|child_stack|clip.STACK_READS
        io=sorted(reads_set|stack_set|stores|child_reads|child_stack|child_stores|set(clip.IO_DEPTH)|{visit.ENTRY})
        traces=[dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=clip.IO_DEPTH.get(pc,0))for pc in io]
        observed, proof = physical_original(image, rows, scratch, entries=(ENTRY,clip.ENTRY,visit.ENTRY),
            regions=REGIONS, trace_calls=traces, trace_extended=True)
        report['original'] = proof
        command = [a.compiler, '-std=c++20', '-Wno-ignored-attributes', '-O2', '-fno-fast-math',
                   '-ffp-contract=off', '-frounding-math', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'),
                   '-I' + str(ROOT / 'tests'), str(paths[3]), '-o', str(scratch / 'facet_search')]
        if a.sanitizers:
            command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        report['command'] = command
        built = subprocess.run(command, capture_output=True, text=True)
        if built.returncode:
            raise ValueError('Private facet_search compile failed: ' + built.stderr[-8000:])
        result = scratch / 'native.json'
        with data.open('rb') as inp, result.open('wb') as out:
            done = subprocess.run([command[-1]], stdin=inp, stdout=out, stderr=subprocess.PIPE)
        if done.returncode:
            raise ValueError('Private facet_search stopped: ' + done.stderr.decode()[-8000:])
        actual = json.loads(result.read_text())
        visited, packets, reads, writes = set(), 0, 0, 0
        expected_rows = []
        for case, got in zip(observed['cases'], actual, strict=True):
            row = rows[case['id']]
            if not case['completed'] or not case['live_x87_unchanged']:
                raise ValueError('Actual facet_search did not preserve its caller x87 stack')
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
                        raise ValueError('Wrong ordered original facet_search read')
                    complete_reads.append(original_read)
            if next(memory_reads,None) is not None:
                raise ValueError('Unclassified original facet_search read')
            want = dict(
                abi=dict(entry=RET if RET in case['executed_sites'] else 0x314ca2, gp=case['boundary_gp'], eflags=case['boundary_eflags'],
                         xmm=case['boundary_xmm'], fp=case['after_fp'], live_x87=row['live_x87']),
                packets=[{k: v for k, v in packet.items() if k != 'arguments'} for packet in case['calls']],
                banks=sorted(case['banks'], key=lambda bank: bank['address']),
                reads=complete_reads, writes=case['writes'],
                visit_entries=sum(item['entry']==visit.ENTRY for item in case['calls']),
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
                raise ValueError('Complete unchanged original facet_search differs: ' + row['name'] + ' ' + field)
            expected_rows.append(want)
            visited.update(case['executed_sites'])
            packets += len(case['calls'])
            reads += len(want['reads'])
            writes += len(case['writes'])
        sites = {int(value, 16) for value in re.findall(r'services.instruction\(0x([0-9a-f]+)u\)', body)}
        missing=sorted(sites-visited)
        report['missing_sites']=[hex(pc)for pc in missing]
        if len(sites) != 40 or missing:
            raise ValueError('Missing authored original hull facet_search sites')
        if a.mutations:
            relative = 'physics/runtime/native_tu15_solver_hull_facet_search_frame.hpp'
            text = (ROOT / 'src' / relative).read_text()
            cases = (
                ('wrong_initial_lower','io_.store(0x314c71u,g[6],4u,0x3727c5acu);','io_.store(0x314c71u,g[6],4u,0u);'),
                ('dropped_seed_status','io_.store(0x314c7eu,g[7]+68u,1u,g[0]);',''),
                ('wrong_nonintersect_result','g[0]=2u;io_.pop(0x314ca1u,3u);','g[0]=3u;io_.pop(0x314ca1u,3u);'),
            )
            report['compiled_defects'] = {}
            for name, old, new in cases:
                if text.count(old) != 1:
                    raise ValueError('Missing genuine facet_search defect seam: ' + name)
                owned = scratch / name
                header = owned / relative
                header.parent.mkdir(parents=True)
                header.write_text(text.replace(old, new, 1))
                trial = command.copy()
                trial.insert(trial.index('-I' + str(ROOT / 'include')), '-I' + str(owned))
                trial[-1] = str(owned / 'facet_search')
                built = subprocess.run(trial, capture_output=True, text=True, timeout=600)
                if built.returncode:
                    raise ValueError('Facet search defect compile failed ' + name + ': ' + built.stderr[-5000:])
                with data.open('rb') as inp:
                    done = subprocess.run([trial[-1]], stdin=inp, capture_output=True, text=True, timeout=600)
                if done.returncode:
                    raise ValueError('Facet search defect stopped before comparison ' + name + ': ' + done.stderr[-5000:])
                different = json.loads(done.stdout)
                first = next((n for n, (x, y) in enumerate(zip(expected_rows, different, strict=True)) if x != y), None)
                if first is None:
                    raise ValueError('Original i386 facet_search did not reject ' + name)
                field = next(k for k in expected_rows[first] if expected_rows[first][k] != different[first][k])
                report['compiled_defects'][name] = dict(rejected=True, first_case=rows[first]['name'], first_field=field)
        report.update(passed=True, body_sha256=BODY_PIN, original_sites=len(sites), executed_original_sites=len(sites&visited), complete_original_calls=len(rows),
                      genuine_visit_entries=sum(row['visit_entries']for row in expected_rows),
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
        child_bodies()
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
