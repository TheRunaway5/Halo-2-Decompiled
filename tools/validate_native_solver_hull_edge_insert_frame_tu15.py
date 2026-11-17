#!/usr/bin/env python3
"""Compare the private actual314ed0 facet-ring construction frame to the original i386 owner."""
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
import validate_native_solver_hull_facet_ring_frame_tu15 as ring
import validate_native_solver_hull_facet_slots_frame_tu15 as slots
from research_mp_sandbox_update_functions import Xbe
from validate_native_tu15_prt_helpers import PIN, physical_original

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'src/physics/physics_live_support_tu15_generated.cpp'
BODY_PIN = 'd231619017a5ca841fc1db48b2b04b4caecb30f28bca868111fe751a6701abe3'
ENTRY = 0x314ed0
RET = 0x314f73
REGION = (0x61000000,0x8000)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def actual_body():
    return next(body for sig, body in function_bodies(SOURCE.read_text()) if sig.symbol == '314ed0')


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


DYNAMIC_RELEASE = {0x314f4f,0x314f55,0x314f57,0x314f5c,0x314f5e,0x314f61,0x314f62,0x314f66,0x314f67}


def inputs():
    base=REGION[0]
    def read(bank,address):return struct.unpack_from('<I',bank,address-base)[0]
    rows=ring.inputs()
    for row in rows:
        i=row['id'];sp=row['gp'][4];context=row['gp'][1]
        bank=bytearray.fromhex(row['banks'][0]['before'])
        descriptor,old_active,holder=read(bank,sp+4),read(bank,sp+12),read(bank,sp+16)
        requested=read(bank,descriptor+4)
        reuse_list,facet=base+0x3600,read(bank,context+0x1b0)
        reused=0 if requested&0x80000000 else i%4
        struct.pack_into('<2I',bank,context+8-base,reuse_list,reused)
        struct.pack_into('<4I',bank,reuse_list-base,*(facet+n*0x50 for n in range(4)))
        struct.pack_into('<I',bank,context+0x1b4-base,4)
        struct.pack_into('<I',bank,context+0x2104-base,i%3)
        point=read(bank,holder)
        struct.pack_into('<4I',bank,sp-base,row['return_pc'],requested,old_active,point)
        row.update(entry=ENTRY,name=f'edge-insertion-{i}',matrix=requested,source=old_active,max_steps=1500)
        row['banks'][0]['before']=bank.hex()
    return rows


def child_bodies():
    result=(slots.actual_body(),ring.actual_body(),plane.actual_body(),clear.actual_body())
    if tuple(hashlib.sha256(body.encode()).hexdigest()for body in result)!=(slots.BODY_PIN,ring.BODY_PIN,plane.BODY_PIN,clear.BODY_PIN):
        raise ValueError('Complete genuine edge insertion child body drift')
    return result


def split_writes(writes):
    result=[]
    for event in writes:
        if event['instruction']!=0x314f07:
            result.extend(plane.split_writes([event]))
            continue
        if len(event['written'])!=32 or len(event['before'])!=32:
            raise ValueError('Expected complete original seed-edge vector store')
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
    paths = [SOURCE, ROOT / 'include/halo2/native_tu15_solver_hull_edge_insert_kernel.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_edge_insert_frame.hpp',
             ROOT / 'tests/native_tu15_solver_hull_edge_insert_frame_tests.cpp',
             ROOT / 'tools/validate_native_tu15_prt_helpers.py',
             ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp',
             ROOT / 'src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp',
             ROOT / 'src/physics/runtime/native_tu15_raw_x87_scope.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_facet_ring_frame.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_facet_slots_frame.hpp',
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
            raise ValueError('Complete actual314ed0 body drift')
        image = Xbe(a.xbe)
        if image.read(RET, 3).hex() != 'c20c00':
            raise ValueError('Actual edge_insert return-word opcode drift')
        rows = inputs()
        data = scratch / 'inputs.json'
        data.write_text(json.dumps(rows, separators=(',', ':')))
        report['input_sha256'] = sha(data)
        _,ring_body,_,_=child_bodies()
        reads_set,stack_set,stores=authored_io(body)
        child_reads,child_stack,child_stores=authored_io(ring_body)
        explicit=reads_set|child_reads|slots.EXPLICIT_READS|plane.EXPLICIT_READS|clear.EXPLICIT_READS
        stack_reads=stack_set|child_stack|slots.STACK_READS|plane.STACK_READS|clear.STACK_READS
        io=sorted(reads_set|stack_set|stores|child_reads|child_stack|child_stores|set(slots.IO)|set(plane.IO)|set(clear.IO)|{ring.ENTRY})
        traces=[dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=0)for pc in io]
        observed, proof = physical_original(image, rows, scratch, entries=(ENTRY,slots.ENTRY,ring.ENTRY,plane.ENTRY,clear.ENTRY),
            regions=(REGION,), trace_calls=traces, trace_extended=True)
        report['original'] = proof
        command = [a.compiler, '-std=c++20', '-Wno-ignored-attributes', '-O2', '-fno-fast-math',
                   '-ffp-contract=off', '-frounding-math', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'),
                   '-I' + str(ROOT / 'tests'), str(paths[3]), '-o', str(scratch / 'edge_insert')]
        if a.sanitizers:
            command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        report['command'] = command
        built = subprocess.run(command, capture_output=True, text=True)
        if built.returncode:
            raise ValueError('Private edge_insert compile failed: ' + built.stderr[-8000:])
        result = scratch / 'native.json'
        with data.open('rb') as inp, result.open('wb') as out:
            done = subprocess.run([command[-1]], stdin=inp, stdout=out, stderr=subprocess.PIPE)
        if done.returncode:
            raise ValueError('Private edge_insert stopped: ' + done.stderr.decode()[-8000:])
        actual = json.loads(result.read_text())
        visited, packets, reads, writes = set(), 0, 0, 0
        expected_rows = []
        for case, got in zip(observed['cases'], actual, strict=True):
            row = rows[case['id']]
            if not case['completed'] or not case['live_x87_unchanged']:
                raise ValueError('Actual edge_insert did not preserve its caller x87 stack')
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
                        raise ValueError('Wrong ordered original edge_insert read')
                    complete_reads.append(original_read)
            if next(memory_reads,None) is not None:
                raise ValueError('Unclassified original edge_insert read')
            want = dict(
                abi=dict(entry=RET, gp=case['boundary_gp'], eflags=case['boundary_eflags'],
                         xmm=case['boundary_xmm'], fp=case['after_fp'], live_x87=row['live_x87']),
                packets=[{k: v for k, v in packet.items() if k != 'arguments'} for packet in case['calls']],
                banks=sorted(case['banks'], key=lambda bank: bank['address']),
                reads=complete_reads, writes=split_writes(case['writes']),
                plane_entries=sum(item['entry']==plane.ENTRY for item in case['calls']),
                clear_entries=sum(item['entry']==clear.ENTRY for item in case['calls']),
                slot_entries=sum(item['entry']==slots.ENTRY for item in case['calls']),
                ring_entries=sum(item['entry']==ring.ENTRY for item in case['calls']))
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
                raise ValueError('Complete unchanged original edge_insert differs: ' + row['name'] + ' ' + field)
            expected_rows.append(want)
            visited.update(case['executed_sites'])
            packets += len(case['calls'])
            reads += len(want['reads'])
            writes += len(case['writes'])
        sites = {int(value, 16) for value in re.findall(r'services.instruction\(0x([0-9a-f]+)u\)', body)}
        if len(sites) != 48 or (sites&visited)!=sites-DYNAMIC_RELEASE:
            raise ValueError('Missing authored original hull edge_insert sites')
        if a.mutations:
            relative = 'physics/runtime/native_tu15_solver_hull_edge_insert_frame.hpp'
            text = (ROOT / 'src' / relative).read_text()
            cases = (
                ('wrong_seed_edge_stride','s_.flags.shift(g[0],5u,4u)','s_.flags.shift(g[0],4u,4u)'),
                ('dropped_seed_edge_vector','io_.store_vector(0x314f07u,g[0],s_.xmm[0]);',''),
            )
            report['compiled_defects'] = {}
            for name, old, new in cases:
                if text.count(old) != 1:
                    raise ValueError('Missing genuine edge_insert defect seam: ' + name)
                owned = scratch / name
                header = owned / relative
                header.parent.mkdir(parents=True)
                header.write_text(text.replace(old, new, 1))
                trial = command.copy()
                trial.insert(trial.index('-I' + str(ROOT / 'include')), '-I' + str(owned))
                trial[-1] = str(owned / 'edge_insert')
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
                    raise ValueError('Original i386 edge_insert did not reject ' + name)
                field = next(k for k in expected_rows[first] if expected_rows[first][k] != different[first][k])
                report['compiled_defects'][name] = dict(rejected=True, first_case=rows[first]['name'], first_field=field)
        report.update(passed=True, body_sha256=BODY_PIN, original_sites=len(sites), executed_original_sites=len(sites&visited), complete_original_calls=len(rows),
                      genuine_slot_entries=sum(row['slot_entries']for row in expected_rows),
                      genuine_ring_entries=sum(row['ring_entries']for row in expected_rows),
                      dynamic_release_path_qualified=False, missing_sites=[hex(pc)for pc in sorted(DYNAMIC_RELEASE)],
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
