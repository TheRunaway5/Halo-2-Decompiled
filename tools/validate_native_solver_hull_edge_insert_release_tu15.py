#!/usr/bin/env python3
"""Compare private 314ED0's dynamic release prefix through the actual allocator entry."""
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
import validate_native_array_growth_tu15 as allocator
import validate_native_solver_hull_edge_insert_frame_tu15 as edge

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'src/physics/physics_live_support_tu15_generated.cpp'
BODY_PIN = 'd231619017a5ca841fc1db48b2b04b4caecb30f28bca868111fe751a6701abe3'
ENTRY = 0x314ed0
RET = 0x149e00
REGION = (0x61000000,0x8000)
EXTRA_REGIONS = ((0x45f000,0x1000),(0x486000,0x1000),(0x48c000,0x1000))


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


def inputs(image):
    captures,provenance=allocator.reference_inputs(image)
    def archived_word(address):
        return next(raw[address-base:address-base+4]for base,raw in captures.items()if base<=address and address+4<=base+len(raw))
    global_page=bytearray(0x1000)
    global_page[0x938:0x93c]=archived_word(0x48c938)
    vtable=image.read(0x45f000,0x1000)
    instance=image.read(0x486000,0x1000)
    if (struct.unpack_from('<I',global_page,0x938)[0],struct.unpack_from('<I',instance,0x80)[0],
        struct.unpack_from('<I',vtable,0x4e0)[0])!=(0x486080,0x45f4cc,RET):
        raise ValueError('Exact captured release router/table drift')
    pages=[dict(address=address,before=data.hex())for (address,_),data in zip(EXTRA_REGIONS,(vtable,instance,global_page),strict=True)]
    rows=[]
    for seed in edge.inputs()[4::16]:
        original_sp=seed['gp'][4]
        local=original_sp-0x1a4
        base=REGION[0]
        context=seed['gp'][1]
        for kind in ('capacity-high-byte','capacity-full-word'):
            row=dict(seed,banks=[dict(bank)for bank in seed['banks']])
            bank=bytearray.fromhex(row['banks'][0]['before'])
            # The genuine Clear child performs the aliased write, so
            # the original private stack capacity is first published as
            # 0x80000064 and then observed with its high bit cleared.
            struct.pack_into('<2I',bank,original_sp+4-base,0,0)
            if kind=='capacity-high-byte':
                struct.pack_into('<2I',bank,context+0x1b0-base,local+19-68,1)
            else:
                struct.pack_into('<2I',bank,context+0x2100-base,local,1)
            row.update(id=len(rows),name=f'{kind}-{seed["id"]}',matrix=0,source=0,
                       stop_before_entries=[RET],release_pointer=local+20,
                       release_bytes=400 if kind=='capacity-high-byte' else 0)
            row['banks'][0]['before']=bank.hex()
            row['banks']+=pages
            rows.append(row)
    return rows,provenance


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report', required=True, type=Path)
    p.add_argument('--xbe', required=True, type=Path)
    p.add_argument('--compiler', default='c++')
    p.add_argument('--sanitizers', action='store_true')
    a = p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        p.error('Run through tools/run_test.py')
    if platform.system() != 'Linux' or platform.machine().lower() not in ('x86_64', 'amd64', 'i386', 'i686') or not a.xbe.is_file():
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    paths = [SOURCE, ROOT / 'include/halo2/native_tu15_solver_hull_edge_insert_kernel.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_edge_insert_frame.hpp',
             ROOT / 'tests/native_tu15_solver_hull_edge_insert_release_tests.cpp',
             ROOT / 'tools/validate_native_tu15_prt_helpers.py',
             ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp',
             ROOT / 'src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp',
             ROOT / 'src/physics/runtime/native_tu15_raw_x87_scope.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_facet_ring_frame.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_facet_slots_frame.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_facet_plane_frame.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_clear_frame.hpp',
             ROOT / 'tools/validate_native_solver_hull_facet_plane_frame_tu15.py',
             ROOT / 'tools/validate_native_solver_hull_clear_frame_tu15.py',
             ROOT / 'tools/validate_native_array_growth_tu15.py',
             ROOT / 'tools/validate_native_solver_hull_edge_insert_frame_tu15.py', Path(__file__)]
    # Pin all involved bodies before and after, independent of unrelated Primary additions.
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
        rows,report['release_table_provenance'] = inputs(image)
        data = scratch / 'inputs.json'
        data.write_text(json.dumps(rows, separators=(',', ':')))
        report['input_sha256'] = sha(data)
        _,ring_body,_,_=edge.child_bodies()
        reads_set,stack_set,stores=authored_io(body)
        child_reads,child_stack,child_stores=authored_io(ring_body)
        explicit=reads_set|child_reads|slots.EXPLICIT_READS|plane.EXPLICIT_READS|clear.EXPLICIT_READS
        stack_reads=stack_set|child_stack|slots.STACK_READS|plane.STACK_READS|clear.STACK_READS
        io=sorted(reads_set|stack_set|stores|child_reads|child_stack|child_stores|set(slots.IO)|set(plane.IO)|set(clear.IO)|{ring.ENTRY})
        traces=[dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=0)for pc in io]
        observed, proof = physical_original(image, rows, scratch, entries=(ENTRY,slots.ENTRY,ring.ENTRY,plane.ENTRY,clear.ENTRY),
            regions=(*EXTRA_REGIONS,REGION), trace_calls=traces, trace_extended=True,
            preserve_x87_prefix=True)
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
            if case.get('stopped_at')!=RET or not case['live_x87_unchanged']:
                raise ValueError('Original edge insertion did not reach the authentic allocator entry')
            local=next(bank for bank in case['banks']if bank['address']==REGION[0])
            args=struct.unpack_from('<4I',bytes.fromhex(local['after']),case['boundary_gp'][4]-REGION[0])
            if args!=(0x314f6a,row['release_pointer'],row['release_bytes'],0x12):
                raise ValueError('Original dynamic release call packet drift: '+repr(args))
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
                reads=complete_reads, writes=edge.split_writes(case['writes']),
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
        reached=sorted(sites&visited)
        if len(sites)!=48 or not DYNAMIC_RELEASE<=set(reached):
            raise ValueError('Missing original dynamic release sites: '+repr(sorted(DYNAMIC_RELEASE-visited)))
        report.update(passed=True,body_sha256=BODY_PIN,original_sites=len(sites),executed_original_sites=len(reached),
                      covered_dynamic_sites=[hex(pc)for pc in sorted(DYNAMIC_RELEASE)],release_entry_prefixes=len(rows),
                      complete_original_calls=0,release_allocator_body_executed=False,
                      genuine_slot_entries=sum(row['slot_entries']for row in expected_rows),
                      genuine_ring_entries=sum(row['ring_entries']for row in expected_rows),
                      real_plane_entries=sum(row['plane_entries']for row in expected_rows),
                      real_clear_entries=sum(row['clear_entries']for row in expected_rows),
                      complete_parent_pre_io_packets=packets,architectural_reads=reads,complete_stores=writes,
                      hostile_provider_comparisons=len(rows),original_text_unmodified=True,
                      source_or_hardware_fault_domains_qualified=False)
    except Exception as error:
        report['error'] = repr(error)
    try:
        after = actual_body()
        report['sources_unchanged'] = all(sha(ROOT / name) == value for name, value in pins.items()) and (
            hashlib.sha256(after.encode()).hexdigest() == BODY_PIN)
        edge.child_bodies()
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
