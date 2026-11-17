#!/usr/bin/env python3
"""Compare private 314870 physical and retained table-clear publication."""
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
import native_solver_hull_clear_replacements as binding
from research_mp_sandbox_update_functions import Xbe
from validate_native_tu15_prt_helpers import PIN, physical_original

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'src/physics/physics_live_support_tu15_generated.cpp'
BODY_PIN = '41b826852497e630c1b0060a1828b0672fd79fbe7446db1aff0a3a630cc5d4cb'
ENTRY = 0x314870
RET = 0x3148da
EXPLICIT_READS = {0x314870,0x314890,0x31489a,0x3148a8,0x3148c0,0x3148ca}
STACK_READS = {0x3148d8,0x3148d9,RET}
STORES = {0x314876,0x31487d,0x314896,0x3148c6}
IO = sorted(EXPLICIT_READS|STACK_READS|STORES)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def actual_body():
    text=SOURCE.read_text()
    if any(marker in text for marker in (binding.INCLUDE,binding.OWNER_MARKER,binding.RESUME_MARKER)):
        if binding.installed_entries(text)!=['314870']:
            raise ValueError('Complete installed clear source ownership drift')
        text=binding.remove_solver_hull_clear(text)
    return next(body for sig, body in function_bodies(text) if sig.symbol == '314870')


def inputs():
    rng = random.Random(ENTRY)
    base, stack, polytope = 0x61000000, 0x61005000, 0x61001000
    cases = [(f'counts-{facets:x}-{edges:x}', facets, edges)
             for facets in (0xffffffff,0x80000000,0,1,2,5)
             for edges in (0xffffffff,0x80000000,0,1,3,6)]
    cases += [(name,3,3) for name in ('facet-count-alias','facet-pointer-alias','edge-count-alias',
               'facet-saved-bx','facet-saved-si','edge-saved-bx','edge-saved-si',
               'tables-alias','edge-after-facet-count-overwrite')]
    live = ('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f')
    rows = []
    for i,(name,facet_count,edge_count) in enumerate(cases):
        bank = bytearray(rng.randbytes(0x7000))
        gp = [rng.getrandbits(32)for _ in range(8)]
        gp[1], gp[4] = polytope, stack+(i&7)
        facets,edges = base+0x3500,base+0x4400
        if name=='facet-count-alias':facets=polytope+0x1b4-0x44
        if name=='facet-pointer-alias':facets=polytope+0x1b0-0x44
        if name=='edge-count-alias':edges=polytope+0x2104-0x10
        if name=='facet-saved-bx':facets=gp[4]-4-0x44
        if name=='facet-saved-si':facets=gp[4]-8-0x44
        if name=='edge-saved-bx':edges=gp[4]-4-0x10
        if name=='edge-saved-si':edges=gp[4]-8-0x10
        if name=='tables-alias':edges=facets+0x34
        if name=='edge-after-facet-count-overwrite':facets=polytope+0x2104-0x44
        struct.pack_into('<II',bank,polytope+0x1b0-base,facets,facet_count)
        struct.pack_into('<II',bank,polytope+0x2100-base,edges,edge_count)
        returned,seed1,seed2=0x63000000,rng.getrandbits(32),rng.getrandbits(32)
        struct.pack_into('<3I',bank,gp[4]-base,returned,seed1,seed2)
        cw = (0x7f,0x27f,0x37f)[i%3]|((i%4)<<10)
        sw = 0x4100|(((i//3)%8)<<11)|(0x200 if i&1 else 0)
        mxcsr = 0x1f80|((i%4)<<13)|(0x8040 if i&4 else 0)
        rows.append(dict(id=i,name=name,entry=ENTRY,gp=gp,
                         eflags=0x202|(rng.getrandbits(12)&0xcd5),return_pc=returned,
                         matrix=seed1,source=seed2,before_fp=[cw,sw,mxcsr],
                         live_x87=[live[j%4]for j in range((0,1,4,6,7,8)[i%6])],
                         xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],
                         max_steps=240,banks=[dict(address=base,before=bank.hex())]))
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
    paths = [SOURCE, ROOT / 'include/halo2/native_tu15_solver_hull_clear_kernel.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_clear_frame.hpp',
             ROOT / 'tests/native_tu15_solver_hull_clear_frame_tests.cpp',
             ROOT / 'tools/validate_native_tu15_prt_helpers.py',
             ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp',
             ROOT / 'src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp',
             ROOT / 'src/physics/runtime/native_tu15_raw_x87_scope.hpp',
             ROOT / 'tools/native_solver_hull_clear_replacements.py', Path(__file__)]
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
            raise ValueError('Complete actual314870 body drift')
        reference=scratch/'solver_hull_clear_actual.inc'
        reference.write_text('static constexpr std::array<std::uint32_t,'+str(len(IO))+'> io_sites{'+
                             ','.join(hex(pc)+'u' for pc in IO)+'};\n'+body+'\n')
        image = Xbe(a.xbe)
        if image.read(RET, 1).hex() != 'c3':
            raise ValueError('Actual clear return-word opcode drift')
        rows = inputs()
        data = scratch / 'inputs.json'
        data.write_text(json.dumps(rows, separators=(',', ':')))
        report['input_sha256'] = sha(data)
        traces = [dict(entry=pc, arguments=0, gp=True, xmm=True, x87_delta=0) for pc in IO]
        observed, proof = physical_original(image, rows, scratch, entries=(ENTRY,),
            regions=((0x61000000, 0x7000),), trace_calls=traces, trace_extended=True)
        report['original'] = proof
        command = [a.compiler, '-std=c++20', '-Wno-ignored-attributes', '-O2', '-fno-fast-math',
                   '-ffp-contract=off', '-frounding-math', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'),
                   '-I' + str(ROOT / 'tests'), '-I' + str(scratch), str(paths[3]), '-o', str(scratch / 'clear')]
        if a.sanitizers:
            command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        report['command'] = command
        built = subprocess.run(command, capture_output=True, text=True)
        if built.returncode:
            raise ValueError('Private clear compile failed: ' + built.stderr[-8000:])
        result = scratch / 'native.json'
        with data.open('rb') as inp, result.open('wb') as out:
            done = subprocess.run([command[-1]], stdin=inp, stdout=out, stderr=subprocess.PIPE)
        if done.returncode:
            raise ValueError('Private clear stopped: ' + done.stderr.decode()[-8000:])
        actual = json.loads(result.read_text())
        visited, packets, reads, writes = set(), 0, 0, 0
        expected_rows = []
        for case, got in zip(observed['cases'], actual, strict=True):
            row = rows[case['id']]
            if not case['completed'] or not case['live_x87_unchanged']:
                raise ValueError('Actual clear did not preserve its caller x87 stack')
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
                        raise ValueError('Wrong ordered original clear read')
                    complete_reads.append(original_read)
            if next(memory_reads,None) is not None:
                raise ValueError('Unclassified original clear read')
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
                raise ValueError('Complete unchanged original clear differs: ' + row['name'] + ' ' + field)
            expected_rows.append(want)
            visited.update(case['executed_sites'])
            packets += len(case['calls'])
            reads += len(want['reads'])
            writes += len(case['writes'])
        sites = {int(value, 16) for value in re.findall(r'services.instruction\(0x([0-9a-f]+)u\)', body)}
        if len(sites) != 32 or not sites <= visited:
            raise ValueError('Missing authored original hull clear sites')
        with data.open('rb') as inp:
            source=subprocess.run([command[-1],'source'],stdin=inp,capture_output=True,text=True,timeout=600)
        if source.returncode:
            raise ValueError('Complete retained hull table-clear stopped: '+source.stderr[-8000:])
        report['retained_source']=json.loads(source.stdout)
        if a.mutations:
            frame = 'physics/runtime/native_tu15_solver_hull_clear_frame.hpp'
            kernel = 'halo2/native_tu15_solver_hull_clear_kernel.hpp'
            cases = (
                ('wrong_facet_stride', 'include', kernel, '0x44u,0x50u,0x314896u', '0x44u,0x40u,0x314896u'),
                ('widened_facet_store', 'include', kernel, '0x314896u,1u', '0x314896u,2u'),
                ('lost_current_count', 'src', frame, 's_.flags.minus(g[0],g[6],4);', 's_.flags.minus(g[0],3u,4);'),
            )
            report['compiled_defects'] = {}
            for name, directory, relative, old, new in cases:
                text = (ROOT / directory / relative).read_text()
                if text.count(old) != 1:
                    raise ValueError('Missing genuine clear defect seam: ' + name)
                owned = scratch / name
                header = owned / relative
                header.parent.mkdir(parents=True)
                header.write_text(text.replace(old, new, 1))
                trial = command.copy()
                trial.insert(trial.index('-I' + str(ROOT / 'include')), '-I' + str(owned))
                trial[-1] = str(owned / 'clear')
                built = subprocess.run(trial, capture_output=True, text=True, timeout=600)
                if built.returncode:
                    raise ValueError('Clear defect compile failed ' + name + ': ' + built.stderr[-5000:])
                with data.open('rb') as inp:
                    done = subprocess.run([trial[-1]], stdin=inp, capture_output=True, text=True, timeout=600)
                if done.returncode:
                    raise ValueError('Clear defect stopped before comparison ' + name + ': ' + done.stderr[-5000:])
                different = json.loads(done.stdout)
                first = next((n for n, (x, y) in enumerate(zip(expected_rows, different, strict=True)) if x != y), None)
                if first is None:
                    raise ValueError('Original i386 clear did not reject ' + name)
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
