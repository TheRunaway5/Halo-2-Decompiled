#!/usr/bin/env python3
"""Compare the actual 313BB0 begin prefix and genuine 32A660 reservation."""
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
from research_mp_sandbox_update_functions import Xbe
from validate_native_tu15_prt_helpers import PIN, physical_original

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'src/physics/physics_live_support_tu15_generated.cpp'
BODY_PIN = '1abb6dc85a3bafd9a84eeb77188384091e10a8389419162c6b7b719f543f232a'
ENTRY = 0x313bb0
TARGETS = (0x313db2,0x313d69,0x313d14,0x313c89,0x313bfc,0x313e8e)
TABLE = 0x3146f8
PROBE = 0x32a660
PROBE_PIN = 'b77ad187f6f3db2a5ae4f8e214b023d7c62ce25a38ce769a993d2a8c81643760'
FS = 0x62000000
REGIONS = ((0x61000000,0x8000),(FS,0x1000))
PROBE_READS={0x32a689,0x32a696,0x32a69a,0x32a69c}
PROBE_STACK={0x32a6a0}
PROBE_STORES={0x32a679,0x32a69f}


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
        if pc>0x313bf5:break
        if re.search(r'\b(?:r|v)\(s.memory',line):reads.add(pc)
        if 's.pop()' in line:stack.add(pc)
        if re.search(r'\b(?:w|vw)\(s.memory|s.push\(',line):stores.add(pc)
    return reads,stack,stores



def inputs(image):
    rng=random.Random(ENTRY)
    base,stack,context,configuration=0x61000000,0x61007000,0x61001000,0x61002000
    table=image.read(TABLE,20)
    if struct.unpack('<5I',table)!=TARGETS[:5]:raise ValueError('Actual hull count table drift')
    live=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f')
    rows=[]
    for count in (0,1,2,3,4,5,0xffffffff,0x80000000):
        for align in (0,1,3,4,7,8,11,15):
            i=len(rows);bank=bytearray(rng.randbytes(0x8000))
            gp=[rng.getrandbits(32)for _ in range(8)];gp[1],gp[4]=context,stack+align
            struct.pack_into('<I',bank,context+0x48-base,configuration)
            struct.pack_into('<I',bank,configuration+0x80-base,count)
            returned=0x63000000;arg=rng.getrandbits(32);other=rng.getrandbits(32)
            struct.pack_into('<3I',bank,gp[4]-base,returned,arg,other)
            cw=(0x7f,0x27f,0x37f)[i%3]|((i%4)<<10)
            sw=0x4100|(((i//3)%8)<<11)|(0x200 if i&1 else 0)
            mxcsr=0x1f80|((i%4)<<13)|(0x8040 if i&4 else 0)
            rows.append(dict(id=i,name=f'hull-begin-{count:x}-{align}',entry=ENTRY,gp=gp,
                eflags=0x202|(rng.getrandbits(12)&0xcd5),return_pc=returned,matrix=arg,source=other,
                before_fp=[cw,sw,mxcsr],live_x87=[live[j%4]for j in range((0,1,4,8)[i%4])],
                xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],max_steps=250,
                fs_base=FS,actual_table=table.hex(),stop_before_entries=list(TARGETS),
                expected=TARGETS[min(count,5)],
                banks=[dict(address=base,before=bank.hex()),dict(address=FS,before=rng.randbytes(0x1000).hex())]))
    return rows


def physical_with_fs(image,rows,scratch,traces):
    from oracle_native_campaign_screen_lifecycle_tu15 import _Code
    decoder=_Code(image,'.text')
    run=subprocess.run
    def private_command(command,*args,**kwargs):
        if command[0]=='as':
            directory=Path(command[2]).parent
            assembly=directory/'reference.s'
            source=assembly.read_text()
            opening='_start:\nfninit\nint3\njmp _start\n'
            if source.count(opening)!=1 or source.count('.section .bridge,"ax"')!=1:
                raise ValueError('Exact private FS bridge seam drift')
            startup=('_start:\nmov $243,%eax\nmov $hull_tls,%ebx\nint $0x80\n'
                   'test %eax,%eax\njns 1f\nmov $1,%eax\nmov $126,%ebx\nint $0x80\n'
                   '1: mov hull_tls,%eax\nshl $3,%eax\nor $3,%eax\nmov %ax,%fs\nfninit\nint3\njmp _start\n')
            source=source.replace(opening,startup,1).replace('.section .bridge,"ax"','.section .bridge,"awx"',1)
            assembly.write_text(source+'\n.balign 16\nhull_tls: .long -1,'+str(FS)+',0xfffff,0x51\n')
            metadata_path=directory/'metadata.json'
            metadata=json.loads(metadata_path.read_text());fs_sites=[];probe_tests=[]
            for site in metadata['sites']:
                instruction=decoder.at(site['instruction'])
                if site['instruction'] in (0x32a689,0x32a696):
                    if instruction.bytes.hex()!='8501' or len(site['operands'])!=1:
                        raise ValueError('Exact original TEST memory operand drift')
                    # Capstone omits the read access bit on these TESTs.
                    site['operands'][0].update(read=True,write=False)
                    probe_tests.append(site['instruction'])
                operands=[op for op in instruction.operands if op.type==3 and instruction.reg_name(op.mem.segment)=='fs']
                if not operands:continue
                if len(operands)!=1 or len(site['operands'])!=1:raise ValueError('Unexpected actual FS operand shape')
                site['operands'][0]['fs_base']=FS;fs_sites.append(site['instruction'])
            if sorted(fs_sites)!=[0x313bbd,0x313bc4,0x314343,0x314468,0x314484,0x3146e9]:
                raise ValueError('Unexpected actual hull FS site inventory')
            if sorted(probe_tests)!=[0x32a689,0x32a696]:
                raise ValueError('Unexpected actual stack TEST site inventory')
            metadata_path.write_text(json.dumps(metadata))
            observer=directory/'observer.cpp';source=observer.read_text()
            address='std::uint32_t p=o.at("displacement");'
            retained='auto retained=[&](std::uint32_t p,unsigned n){'
            if source.count(address)!=1 or source.count(retained)!=1:
                raise ValueError('Exact physical observer metadata seam drift')
            # Observe reads from authenticated executable table bytes
            # without installing a test bank or writing to original text.
            observer.write_text(source.replace(address,address+'p+=o.value("fs_base",0u);',1).replace(retained,
                retained+'if(n==4u&&p>=0x3146f8u&&p<0x31470cu)return true;',1))
        return run(command,*args,**kwargs)
    globals_=physical_original.__globals__.copy()
    globals_['subprocess']=types.SimpleNamespace(run=private_command)
    wrapper=types.FunctionType(physical_original.__code__,globals_,physical_original.__name__,
                               physical_original.__defaults__,physical_original.__closure__)
    wrapper.__kwdefaults__=physical_original.__kwdefaults__.copy()
    return wrapper(image,rows,scratch,entries=(ENTRY,PROBE),regions=REGIONS,trace_calls=traces,
                   trace_extended=True,preserve_x87_prefix=True)


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
    paths = [SOURCE, ROOT / 'include/halo2/native_tu15_solver_hull_begin_kernel.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_begin_frame.hpp',
             ROOT / 'tests/native_tu15_solver_hull_begin_frame_tests.cpp',
             ROOT / 'tools/validate_native_tu15_prt_helpers.py',
             ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp',
             ROOT / 'src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp',
             ROOT / 'src/physics/runtime/native_tu15_raw_x87_scope.hpp',
             ROOT / 'tests/native_tu15_mopp_ray_frame_test_memory.hpp',
             ROOT / 'src/physics/runtime/native_tu15_stack_probe_abi.hpp',
             ROOT / 'include/halo2/native_tu15_stack_probe_kernel.hpp',Path(__file__)]
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
        if hashlib.sha256(image.read(PROBE,65)).hexdigest()!=PROBE_PIN:
            raise ValueError('Complete actual StackProbe body drift')
        rows = inputs(image)
        data = scratch / 'inputs.json'
        data.write_text(json.dumps(rows, separators=(',', ':')))
        report['input_sha256'] = sha(data)
        explicit,stack_reads,stores=authored_io(body)
        explicit|=PROBE_READS;stack_reads|=PROBE_STACK;stores|=PROBE_STORES
        io=sorted(explicit|stack_reads|stores|{PROBE})
        traces=[dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=0)for pc in io]
        observed, proof = physical_with_fs(image,rows,scratch,traces)
        report['original'] = proof
        command = [a.compiler, '-std=c++20', '-Wno-ignored-attributes', '-O2', '-fno-fast-math',
                   '-ffp-contract=off', '-frounding-math', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'),
                   '-I' + str(ROOT / 'tests'), str(paths[3]), '-o', str(scratch / 'begin')]
        if a.sanitizers:
            command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        report['command'] = command
        built = subprocess.run(command, capture_output=True, text=True)
        if built.returncode:
            raise ValueError('Private begin compile failed: ' + built.stderr[-8000:])
        result = scratch / 'native.json'
        with data.open('rb') as inp, result.open('wb') as out:
            done = subprocess.run([command[-1]], stdin=inp, stdout=out, stderr=subprocess.PIPE)
        if done.returncode:
            raise ValueError('Private begin stopped: ' + done.stderr.decode()[-8000:])
        actual = json.loads(result.read_text())
        visited, packets, reads, writes = set(), 0, 0, 0
        expected_rows = []
        for case, got in zip(observed['cases'], actual, strict=True):
            row = rows[case['id']]
            if case.get('stopped_at')!=row['expected'] or not case['live_x87_unchanged']:
                raise ValueError('Actual begin did not preserve its caller x87 stack')
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
                        raise ValueError('Wrong ordered original begin read: expected '+hex(pc)+' saw '+hex(original_read['instruction']))
                    complete_reads.append(original_read)
            if next(memory_reads,None) is not None:
                raise ValueError('Unclassified original begin read')
            want = dict(
                abi=dict(entry=row['expected'], gp=case['boundary_gp'], eflags=case['boundary_eflags'],
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
                raise ValueError('Complete unchanged original begin differs: ' + row['name'] + ' ' + field)
            expected_rows.append(want)
            visited.update(case['executed_sites'])
            packets += len(case['calls'])
            reads += len(want['reads'])
            writes += len(case['writes'])
        sites = {int(value, 16) for value in re.findall(r'services.instruction\(0x([0-9a-f]+)u\)', body)}
        sites={pc for pc in sites if pc<=0x313bf5};missing=sorted(sites-visited)
        report['missing_sites']=[hex(pc)for pc in missing]
        if len(sites) != 22 or missing:
            raise ValueError('Missing authored original hull begin sites')
        if a.mutations:
            relative = 'physics/runtime/native_tu15_solver_hull_begin_frame.hpp'
            text = (ROOT / 'src' / relative).read_text()
            cases = (
                ('wrong_stack_reservation','s_.gp[0]=0x3004u','s_.gp[0]=0x2004u'),
                ('wrong_table_scale','0x3146f8u+g[0]*4u','0x3146f8u+g[0]*0u'),
            )
            report['compiled_defects'] = {}
            for name, old, new in cases:
                if text.count(old) != 1:
                    raise ValueError('Missing genuine begin defect seam: ' + name)
                owned = scratch / name
                header = owned / relative
                header.parent.mkdir(parents=True)
                header.write_text(text.replace(old, new, 1))
                trial = command.copy()
                trial.insert(trial.index('-I' + str(ROOT / 'include')), '-I' + str(owned))
                trial[-1] = str(owned / 'begin')
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
                    raise ValueError('Original i386 begin did not reject ' + name)
                field = next(k for k in expected_rows[first] if expected_rows[first][k] != different[first][k])
                report['compiled_defects'][name] = dict(rejected=True, first_case=rows[first]['name'], first_field=field)
        report.update(passed=True, body_sha256=BODY_PIN, original_sites=len(sites), executed_original_sites=len(sites&visited),
                      reached_continuations=sorted({hex(row['expected'])for row in rows}),
                      complete_original_calls=0,prefix_paths=len(rows),genuine_stack_probe_entries=len(rows),
                      complete_parent_pre_io_packets=packets, architectural_reads=reads, complete_stores=writes,
                      hostile_parent_provider_comparisons=len(rows), hostile_stack_probe_qualified=False,
                      original_text_unmodified=True,
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
