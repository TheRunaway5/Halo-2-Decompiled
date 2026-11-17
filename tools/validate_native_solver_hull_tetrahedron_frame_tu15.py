#!/usr/bin/env python3
"""Compare the private actual313ab0 inline tetrahedron frame to the original i386 owner."""
import argparse
import hashlib
import json
import os
import platform
import random
import re
import struct
import subprocess
import types
from pathlib import Path

from generated_routine_ownership import function_bodies
import validate_native_solver_hull_facet_plane_frame_tu15 as plane
import validate_native_solver_hull_seed_facets_frame_tu15 as seed
from research_mp_sandbox_update_functions import Xbe
from validate_native_tu15_prt_helpers import PIN, physical_original

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'src/physics/physics_live_support_tu15_generated.cpp'
BODY_PIN = '0358c7d8b74e3fd8d07d191d1a8db448b8e260ff9722732900e5d0e340dc0d83'
ENTRY = 0x313ab0
RET = 0x313ba6
FS = 0x62000000
REGIONS = (*seed.REGIONS,(FS,0x1000))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def actual_body():
    return next(body for sig, body in function_bodies(SOURCE.read_text()) if sig.symbol == '313ab0')


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
    rows=seed.inputs(image)
    for row in rows:
        i=row['id'];sp=row['gp'][4]
        bank=bytearray.fromhex(row['banks'][2]['before'])
        args=[base+0x4200+32*n for n in range(4)]
        if i%12==8:
            # The late second argument aliases the first output vector.
            args[1]=row['gp'][1]+0x2110
        if i%12==10:
            # The late fourth argument aliases the second output vector.
            args[3]=row['gp'][1]+0x2130
        struct.pack_into('<4I',bank,sp+4-base,*args)
        row.update(entry=ENTRY,name=f'tetrahedron-{i}',matrix=args[0],source=args[1],
                   max_steps=2400,fs_base=FS)
        row['banks'][2]['before']=bank.hex()
        row['banks'].append(dict(address=FS,before=rng.randbytes(0x1000).hex()))
    return rows


def child_bodies():
    bodies=(seed.actual_body(),plane.actual_body())
    if tuple(hashlib.sha256(body.encode()).hexdigest()for body in bodies)!=(seed.BODY_PIN,plane.BODY_PIN):
        raise ValueError('Complete genuine tetrahedron child source drift')
    return bodies


def physical_with_fs(image,rows,scratch,traces):
    from oracle_native_campaign_screen_lifecycle_tu15 import _Code
    decoder=_Code(image,'.text')
    run=subprocess.run
    def reference_command(command,*args,**kwargs):
        if command[0]=='as':
            directory=Path(command[2]).parent
            assembly=directory/'reference.s'
            text=assembly.read_text()
            opening='_start:\nfninit\nint3\njmp _start\n'
            if text.count(opening)!=1 or text.count('.section .bridge,"ax"')!=1:
                raise ValueError('Exact private original FS bridge seam drift')
            entry=('_start:\nmov $243,%eax\nmov $tetrahedron_tls,%ebx\nint $0x80\n'
                   'test %eax,%eax\njns 1f\nmov $1,%eax\nmov $126,%ebx\nint $0x80\n'
                   '1: mov tetrahedron_tls,%eax\nshl $3,%eax\nor $3,%eax\nmov %ax,%fs\nfninit\nint3\njmp _start\n')
            text=text.replace(opening,entry,1).replace('.section .bridge,"ax"','.section .bridge,"awx"',1)
            assembly.write_text(text+'\n.balign 16\ntetrahedron_tls: .long -1,'+str(FS)+',0xfffff,0x51\n')
            metadata_path=directory/'metadata.json'
            metadata=json.loads(metadata_path.read_text());fs_sites=[]
            for site in metadata['sites']:
                instruction=decoder.at(site['instruction'])
                operands=[op for op in instruction.operands if op.type==3 and instruction.reg_name(op.mem.segment)=='fs']
                if not operands:continue
                if len(operands)!=1 or len(site['operands'])!=1:
                    raise ValueError('Unexpected actual FS operand shape')
                site['operands'][0]['fs_base']=FS;fs_sites.append(site['instruction'])
            if sorted(fs_sites)!=[0x313ab7,0x313abe,0x313b9c]:
                raise ValueError('Unexpected tetrahedron FS site inventory')
            metadata_path.write_text(json.dumps(metadata))
            observer=directory/'observer.cpp';source=observer.read_text()
            needle='std::uint32_t p=o.at("displacement");'
            if source.count(needle)!=1:raise ValueError('Exact physical FS metadata consumer drift')
            observer.write_text(source.replace(needle,needle+'p+=o.value("fs_base",0u);',1))
        return run(command,*args,**kwargs)
    globals_=physical_original.__globals__.copy()
    globals_['subprocess']=types.SimpleNamespace(run=reference_command)
    wrapper=types.FunctionType(physical_original.__code__,globals_,physical_original.__name__,
                               physical_original.__defaults__,physical_original.__closure__)
    wrapper.__kwdefaults__=physical_original.__kwdefaults__.copy()
    return wrapper(image,rows,scratch,entries=(ENTRY,seed.ENTRY,plane.ENTRY),regions=REGIONS,
                   trace_calls=traces,trace_extended=True)


def split_writes(writes):
    result=[]
    for event in writes:
        if event['instruction'] not in {0x313b2f,0x313b48,0x313b62,0x313b7c}:
            result.extend(plane.split_writes([event]))
            continue
        if len(event['written'])!=32 or len(event['before'])!=32:
            raise ValueError('Expected complete original tetrahedron vector store')
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
    paths = [SOURCE, ROOT / 'include/halo2/native_tu15_solver_hull_tetrahedron_kernel.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_tetrahedron_frame.hpp',
             ROOT / 'tests/native_tu15_solver_hull_tetrahedron_frame_tests.cpp',
             ROOT / 'tools/validate_native_tu15_prt_helpers.py',
             ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp',
             ROOT / 'src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp',
             ROOT / 'src/physics/runtime/native_tu15_raw_x87_scope.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_facet_plane_frame.hpp',
             ROOT / 'tools/validate_native_solver_hull_facet_plane_frame_tu15.py',
             ROOT / 'tools/validate_native_solver_hull_seed_facets_frame_tu15.py',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_seed_facets_frame.hpp',Path(__file__)]
    # Pin all three complete actual bodies before and after, independent of unrelated Primary additions.
    pins = {str(path.relative_to(ROOT)): sha(path) for path in paths if path != SOURCE}
    report = dict(passed=False, production_bound=False, scratch_owner=str(scratch), source_sha256=pins,
                  retained_large_artifacts=[])
    try:
        if sha(a.xbe) != PIN:
            raise ValueError('Wrong TU1.5 original image')
        body = actual_body()
        if hashlib.sha256(body.encode()).hexdigest() != BODY_PIN:
            raise ValueError('Complete actual313ab0 body drift')
        image = Xbe(a.xbe)
        if image.read(RET,3).hex()!='c21000':
            raise ValueError('Actual tetrahedron return-word opcode drift')
        rows = inputs(image)
        data = scratch / 'inputs.json'
        data.write_text(json.dumps(rows, separators=(',', ':')))
        report['input_sha256'] = sha(data)
        seed_body,_=child_bodies()
        reads_set,stack_set,stores=authored_io(body)
        seed_reads,seed_stack,seed_stores=seed.authored_io(seed_body)
        explicit=reads_set|seed_reads|plane.EXPLICIT_READS
        stack_reads=stack_set|seed_stack|plane.STACK_READS
        io=sorted(reads_set|stack_set|stores|seed_reads|seed_stack|seed_stores|set(plane.IO))
        traces=[dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=int(pc==0x314e74))for pc in io]
        observed, proof = physical_with_fs(image,rows,scratch,traces)
        report['original'] = proof
        command = [a.compiler, '-std=c++20', '-Wno-ignored-attributes', '-O2', '-fno-fast-math',
                   '-ffp-contract=off', '-frounding-math', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'),
                   '-I' + str(ROOT / 'tests'), str(paths[3]), '-o', str(scratch / 'tetrahedron')]
        if a.sanitizers:
            command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        report['command'] = command
        built = subprocess.run(command, capture_output=True, text=True)
        if built.returncode:
            raise ValueError('Private tetrahedron compile failed: ' + built.stderr[-8000:])
        result = scratch / 'native.json'
        with data.open('rb') as inp, result.open('wb') as out:
            done = subprocess.run([command[-1]], stdin=inp, stdout=out, stderr=subprocess.PIPE)
        if done.returncode:
            raise ValueError('Private tetrahedron stopped: ' + done.stderr.decode()[-8000:])
        actual = json.loads(result.read_text())
        visited, packets, reads, writes = set(), 0, 0, 0
        expected_rows = []
        for case, got in zip(observed['cases'], actual, strict=True):
            row = rows[case['id']]
            if not case['completed'] or not case['live_x87_unchanged']:
                raise ValueError('Actual tetrahedron did not preserve its caller x87 stack')
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
                        raise ValueError('Wrong ordered original tetrahedron read')
                    complete_reads.append(original_read)
            if next(memory_reads,None) is not None:
                raise ValueError('Unclassified original tetrahedron read')
            want = dict(
                abi=dict(entry=RET, gp=case['boundary_gp'], eflags=case['boundary_eflags'],
                         xmm=case['boundary_xmm'], fp=case['after_fp'], live_x87=row['live_x87']),
                packets=[{k: v for k, v in packet.items() if k != 'arguments'} for packet in case['calls']],
                banks=sorted(case['banks'], key=lambda bank: bank['address']),
                reads=complete_reads, writes=split_writes(case['writes']),
                plane_entries=sum(item['entry']==plane.ENTRY for item in case['calls']),
                seed_entries=sum(item['entry']==seed.ENTRY for item in case['calls']))
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
                raise ValueError('Complete unchanged original tetrahedron differs: ' + row['name'] + ' ' + field)
            expected_rows.append(want)
            visited.update(case['executed_sites'])
            packets += len(case['calls'])
            reads += len(want['reads'])
            writes += len(case['writes'])
        sites = {int(value, 16) for value in re.findall(r'services.instruction\(0x([0-9a-f]+)u\)', body)}
        missing=sorted(sites-visited)
        report['missing_sites']=[hex(pc)for pc in missing]
        if len(sites) != 58 or missing:
            raise ValueError('Missing authored original hull tetrahedron sites')
        if a.mutations:
            relative = 'physics/runtime/native_tu15_solver_hull_tetrahedron_frame.hpp'
            text = (ROOT / 'src' / relative).read_text()
            cases = (
                ('wrong_edge_capacity','g[6]+0x2108u,4u,0x80000037u','g[6]+0x2108u,4u,0x80000038u'),
                ('cached_third_formal','io_.load(0x313b5bu,1u,g[4]+32u);','io_.load(0x313b5bu,1u,g[4]+28u);'),
                ('omitted_FS_restore','io_.store(0x313b9cu,s_.fs_base,4u,g[1]);',''),
            )
            report['compiled_defects'] = {}
            for name, old, new in cases:
                if text.count(old) != 1:
                    raise ValueError('Missing genuine tetrahedron defect seam: ' + name)
                owned = scratch / name
                header = owned / relative
                header.parent.mkdir(parents=True)
                header.write_text(text.replace(old, new, 1))
                trial = command.copy()
                trial.insert(trial.index('-I' + str(ROOT / 'include')), '-I' + str(owned))
                trial[-1] = str(owned / 'tetrahedron')
                built = subprocess.run(trial, capture_output=True, text=True, timeout=600)
                if built.returncode:
                    raise ValueError('Tetrahedron defect compile failed ' + name + ': ' + built.stderr[-5000:])
                with data.open('rb') as inp:
                    done = subprocess.run([trial[-1]], stdin=inp, capture_output=True, text=True, timeout=600)
                if done.returncode:
                    raise ValueError('Tetrahedron defect stopped before comparison ' + name + ': ' + done.stderr[-5000:])
                different = json.loads(done.stdout)
                first = next((n for n, (x, y) in enumerate(zip(expected_rows, different, strict=True)) if x != y), None)
                if first is None:
                    raise ValueError('Original i386 tetrahedron did not reject ' + name)
                field = next(k for k in expected_rows[first] if expected_rows[first][k] != different[first][k])
                report['compiled_defects'][name] = dict(rejected=True, first_case=rows[first]['name'], first_field=field)
        report.update(passed=True, body_sha256=BODY_PIN, original_sites=len(sites), executed_original_sites=len(sites&visited), complete_original_calls=len(rows),
                      genuine_seed_entries=sum(row['seed_entries']for row in expected_rows),
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
