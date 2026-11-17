#!/usr/bin/env python3
"""Compare the private actual3149b0 facet-slot allocation frame to the original i386 owner."""
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
BODY_PIN = '85b1cc15045aec36596f828a6d6c0c48b390e036766a4e9f02ac19b37fea1a1c'
ENTRY = 0x3149b0
RET = 0x314a22
EXPLICIT_READS = {0x3149b0,0x3149b3,0x3149b9,0x3149c8,0x3149cb,0x3149ce,0x3149d1,
                  0x3149d7,0x3149da,0x3149f0,0x3149f6,0x314a05,0x314a10,0x314a15}
STACK_READS = {0x3149e9,0x314a1f,0x314a20,0x314a21,RET}
STORES = {0x3149b7,0x3149b8,0x3149c0,0x3149c3,0x3149d3,0x3149d7,0x3149e2,0x3149ff,0x314a12,0x314a1a}
IO = sorted(EXPLICIT_READS|STACK_READS|STORES)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def actual_body():
    return next(body for sig, body in function_bodies(SOURCE.read_text()) if sig.symbol == '3149b0')


def inputs():
    rng = random.Random(ENTRY)
    base, stack, context, descriptor = 0x61000000,0x61006000,0x61001000,0x61003800
    cases = [(f'reuse-{reuse:x}-request-{amount:x}',reuse,amount)for reuse in (0xffffffff,0,1,2,4)
             for amount in (0,1,2,3,6,0xffffffff)]
    cases += [('recycled-store-aliases-output-count',2,2),('recycled-store-aliases-reuse-count',2,1),
              ('recycled-store-aliases-saved-bp',2,1),('new-output-aliases-facet-count',0,2)]
    live = ('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f')
    rows = []
    for i,(name,reuse,amount) in enumerate(cases):
        bank=bytearray(rng.randbytes(0x8000));gp=[rng.getrandbits(32)for _ in range(8)]
        gp[1],gp[4]=context,stack+(i&7)
        reuse_list,facet_base=base+0x3400,base+0x4000
        output,used=base+0x3900,i%4
        if name=='recycled-store-aliases-output-count':output,used=descriptor+4,0
        if name=='recycled-store-aliases-reuse-count':output,used=context+12,0
        if name=='recycled-store-aliases-saved-bp':output,used=gp[4]-16,0
        if name=='new-output-aliases-facet-count':output,used=context+0x1b4,0
        struct.pack_into('<II',bank,context+8-base,reuse_list,reuse)
        struct.pack_into('<II',bank,context+0x1b0-base,facet_base,3+i%5)
        struct.pack_into('<4I',bank,reuse_list-base,1,2,3,4)
        struct.pack_into('<II',bank,descriptor-base,output,used)
        returned=0x63000000
        struct.pack_into('<3I',bank,gp[4]-base,returned,descriptor,amount)
        cw=(0x7f,0x27f,0x37f)[i%3]|((i%4)<<10)
        sw=0x4100|(((i//3)%8)<<11)|(0x200 if i&1 else 0)
        mxcsr=0x1f80|((i%4)<<13)|(0x8040 if i&4 else 0)
        rows.append(dict(id=i,name=name,entry=ENTRY,gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),
            return_pc=returned,matrix=descriptor,source=amount,before_fp=[cw,sw,mxcsr],
            live_x87=[live[j%4]for j in range((0,1,4,6,7,8)[i%6])],
            xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],max_steps=600,
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
    paths = [SOURCE, ROOT / 'include/halo2/native_tu15_solver_hull_facet_slots_kernel.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_facet_slots_frame.hpp',
             ROOT / 'tests/native_tu15_solver_hull_facet_slots_frame_tests.cpp',
             ROOT / 'tools/validate_native_tu15_prt_helpers.py',
             ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp',
             ROOT / 'src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp',
             ROOT / 'src/physics/runtime/native_tu15_raw_x87_scope.hpp', Path(__file__)]
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
            raise ValueError('Complete actual3149b0 body drift')
        reference=scratch/'solver_hull_facet_slots_actual.inc'
        reference.write_text('static constexpr std::array<std::uint32_t,'+str(len(IO))+'> io_sites{'+
                             ','.join(hex(pc)+'u' for pc in IO)+'};\n'+body+'\n')
        image = Xbe(a.xbe)
        if image.read(RET, 3).hex() != 'c20800':
            raise ValueError('Actual facet_slots return-word opcode drift')
        rows = inputs()
        data = scratch / 'inputs.json'
        data.write_text(json.dumps(rows, separators=(',', ':')))
        report['input_sha256'] = sha(data)
        traces = [dict(entry=pc, arguments=0, gp=True, xmm=True, x87_delta=0) for pc in IO]
        observed, proof = physical_original(image, rows, scratch, entries=(ENTRY,),
            regions=((0x61000000, 0x8000),), trace_calls=traces, trace_extended=True)
        report['original'] = proof
        command = [a.compiler, '-std=c++20', '-Wno-ignored-attributes', '-O2', '-fno-fast-math',
                   '-ffp-contract=off', '-frounding-math', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'),
                   '-I' + str(ROOT / 'tests'), '-I' + str(scratch), str(paths[3]), '-o', str(scratch / 'facet_slots')]
        if a.sanitizers:
            command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        report['command'] = command
        built = subprocess.run(command, capture_output=True, text=True)
        if built.returncode:
            raise ValueError('Private facet_slots compile failed: ' + built.stderr[-8000:])
        result = scratch / 'native.json'
        with data.open('rb') as inp, result.open('wb') as out:
            done = subprocess.run([command[-1]], stdin=inp, stdout=out, stderr=subprocess.PIPE)
        if done.returncode:
            raise ValueError('Private facet_slots stopped: ' + done.stderr.decode()[-8000:])
        actual = json.loads(result.read_text())
        visited, packets, reads, writes = set(), 0, 0, 0
        expected_rows = []
        for case, got in zip(observed['cases'], actual, strict=True):
            row = rows[case['id']]
            if not case['completed'] or not case['live_x87_unchanged']:
                raise ValueError('Actual facet_slots did not preserve its caller x87 stack')
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
                        raise ValueError('Wrong ordered original facet_slots read')
                    complete_reads.append(original_read)
            if next(memory_reads,None) is not None:
                raise ValueError('Unclassified original facet_slots read')
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
                raise ValueError('Complete unchanged original facet_slots differs: ' + row['name'] + ' ' + field)
            expected_rows.append(want)
            visited.update(case['executed_sites'])
            packets += len(case['calls'])
            reads += len(want['reads'])
            writes += len(case['writes'])
        sites = {int(value, 16) for value in re.findall(r'services.instruction\(0x([0-9a-f]+)u\)', body)}
        if len(sites) != 48 or not sites <= visited:
            raise ValueError('Missing authored original hull facet_slots sites')
        with data.open('rb') as inp:
            source=subprocess.run([command[-1],'source'],stdin=inp,capture_output=True,text=True,timeout=600)
        if source.returncode:
            raise ValueError('Complete retained hull facet slots stopped: '+source.stderr[-8000:])
        report['retained_source']=json.loads(source.stdout)
        if a.mutations:
            relative = 'physics/runtime/native_tu15_solver_hull_facet_slots_frame.hpp'
            text = (ROOT / 'src' / relative).read_text()
            cases = (
                ('wrong_facet_stride', 'g[2]+g[2]*4u,4u,4u', 'g[2]+g[2]*3u,4u,4u'),
                ('stale_alias_output_count', 'flags.increment(word(address),4u,false)',
                 'flags.increment(s_.gp[3],4u,false)'),
                ('wrong_reusable_slot_index', 'g[7]+g[2]*4u', 'g[7]+(g[2]-1u)*4u'),
            )
            report['compiled_defects'] = {}
            for name, old, new in cases:
                if text.count(old) != 1:
                    raise ValueError('Missing genuine facet_slots defect seam: ' + name)
                owned = scratch / name
                header = owned / relative
                header.parent.mkdir(parents=True)
                header.write_text(text.replace(old, new, 1))
                trial = command.copy()
                trial.insert(trial.index('-I' + str(ROOT / 'include')), '-I' + str(owned))
                trial[-1] = str(owned / 'facet_slots')
                built = subprocess.run(trial, capture_output=True, text=True, timeout=600)
                if built.returncode:
                    raise ValueError('Facet slots defect compile failed ' + name + ': ' + built.stderr[-5000:])
                with data.open('rb') as inp:
                    done = subprocess.run([trial[-1]], stdin=inp, capture_output=True, text=True, timeout=600)
                if done.returncode:
                    raise ValueError('Facet slots defect stopped before comparison ' + name + ': ' + done.stderr[-5000:])
                different = json.loads(done.stdout)
                first = next((n for n, (x, y) in enumerate(zip(expected_rows, different, strict=True)) if x != y), None)
                if first is None:
                    raise ValueError('Original i386 facet_slots did not reject ' + name)
                field = next(k for k in expected_rows[first] if expected_rows[first][k] != different[first][k])
                report['compiled_defects'][name] = dict(rejected=True, first_case=rows[first]['name'], first_field=field)
        report.update(passed=True, body_sha256=BODY_PIN, original_sites=len(sites), complete_original_calls=len(rows),
                      complete_parent_pre_io_packets=packets, architectural_reads=reads, complete_stores=writes,
                      hostile_provider_comparisons=len(rows), original_text_unmodified=True,
                      actual_i386_fault_domains_qualified=False)
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
