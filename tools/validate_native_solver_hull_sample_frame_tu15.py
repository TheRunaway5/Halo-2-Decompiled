#!/usr/bin/env python3
"""Compare the private actual313300 sampling frame to the original i386 owner."""
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
BODY_PIN = 'abb1e9c9f80b2d38224c12612dc1fbde9091798d3579728bcd998980f7264703'
ENTRY = 0x313300
RET = 0x31338d
IO_DEPTH = {
    0x313300: 0, 0x313304: 0, 0x313308: 1, 0x313312: 1,
    0x313322: 1, 0x31332c: 1, 0x313330: 2, 0x31333b: 2,
    0x313349: 2, 0x313353: 2, 0x313355: 1, 0x313359: 2,
    0x313364: 2, 0x313371: 2, 0x313375: 2, 0x313378: 1,
    0x31337c: 2, 0x313382: 2, 0x313388: 2, RET: 0,
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inputs(image):
    rng = random.Random(ENTRY)
    base, stack = 0x61000000, 0x61002000
    constant = dict(address=0x41d000, before=image.read(0x41d000, 0x1000).hex())
    edges = (0, 0x80000000, 1, 0x80000001, 0x7fffff, 0x800000, 0x7f7fffff,
             0xff7fffff, 0x7f800000, 0xff800000, 0x7fc12345, 0xffc23456,
             0x7f812345, 0xff823456, 0x3f800000, 0xbf800000)
    live = ('0000000000000080ff3f', '35c26821a2da0fc90040',
            '00000000000000000080', '45230100000000c0ff7f')
    rows = []
    for i in range(80):
        bank = bytearray(rng.randbytes(0x4000))
        gp = [rng.getrandbits(32) for _ in range(8)]
        gp[4] = stack + (i & 15)
        output = (base + 0x1000, gp[4] + 4, gp[4] + 8,
                  gp[4] + 12, gp[4] + 16)[i % 5]
        seed = rng.getrandbits(32) if i % 4 else edges[(i // 4) % len(edges)]
        low, high = edges[i % len(edges)], edges[(i * 3 + 5) % len(edges)]
        if i < 32:
            low, high = struct.unpack('<II', struct.pack('<ff', rng.uniform(-3, 3), rng.uniform(-3, 3)))
        returned = 0x63000000
        struct.pack_into('<5I', bank, gp[4] - base, returned, seed, low, high, output)
        cw = (0x7f, 0x27f, 0x37f)[i % 3] | ((i % 4) << 10)
        sw = 0x4100 | (((i // 3) % 8) << 11) | (0x200 if i & 1 else 0)
        mxcsr = 0x1f80 | ((i % 4) << 13) | (0x8040 if i & 4 else 0)
        rows.append(dict(id=i, name=f'sample-{i}', entry=ENTRY, gp=gp,
                         eflags=0x202 | (rng.getrandbits(12) & 0xcd5), return_pc=returned,
                         matrix=seed, source=low, before_fp=[cw, sw, mxcsr],
                         live_x87=[live[j % 4] for j in range((0, 1, 4, 6)[i % 4])],
                         xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)],
                         max_steps=100, banks=[dict(address=base, before=bank.hex()), constant]))
    return rows


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
    paths = [SOURCE, ROOT / 'include/halo2/native_tu15_solver_hull_sample_kernel.hpp',
             ROOT / 'src/physics/runtime/native_tu15_solver_hull_sample_frame.hpp',
             ROOT / 'tests/native_tu15_solver_hull_sample_frame_tests.cpp',
             ROOT / 'tools/validate_native_tu15_prt_helpers.py',
             ROOT / 'tools/native_solver_hull_sample_replacements.py',
             ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp', Path(__file__)]
    pins = {str(path.relative_to(ROOT)): sha(path) for path in paths}
    report = dict(passed=False, production_bound=False, scratch_owner=str(scratch), source_sha256=pins,
                  retained_large_artifacts=[])
    try:
        if sha(a.xbe) != PIN:
            raise ValueError('Wrong TU1.5 original image')
        from native_solver_hull_sample_replacements import canonical_body
        body = canonical_body(next(body for sig, body in function_bodies(SOURCE.read_text()) if sig.symbol == '313300'))
        if hashlib.sha256(body.encode()).hexdigest() != BODY_PIN:
            raise ValueError('Complete actual313300 body drift')
        image = Xbe(a.xbe)
        if image.read(RET, 3).hex() != 'c21000':
            raise ValueError('Actual sample return-word opcode drift')
        rows = inputs(image)
        data = scratch / 'inputs.json'
        data.write_text(json.dumps(rows, separators=(',', ':')))
        report['input_sha256'] = sha(data)
        traces = [dict(entry=pc, arguments=0, gp=True, xmm=True, x87_delta=delta)
                  for pc, delta in IO_DEPTH.items()]
        observed, proof = physical_original(image, rows, scratch, entries=(ENTRY,),
            regions=((0x61000000, 0x4000), (0x41d000, 0x1000)), trace_calls=traces, trace_extended=True)
        report['original'] = proof
        command = [a.compiler, '-std=c++20', '-Wno-ignored-attributes', '-O2', '-fno-fast-math',
                   '-ffp-contract=off', '-frounding-math', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'),
                   '-I' + str(ROOT / 'tests'), str(paths[3]), '-o', str(scratch / 'sample')]
        if a.sanitizers:
            command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        report['command'] = command
        built = subprocess.run(command, capture_output=True, text=True)
        if built.returncode:
            raise ValueError('Private sample compile failed: ' + built.stderr[-8000:])
        result = scratch / 'native.json'
        with data.open('rb') as inp, result.open('wb') as out:
            done = subprocess.run([command[-1]], stdin=inp, stdout=out, stderr=subprocess.PIPE)
        if done.returncode:
            raise ValueError('Private sample stopped: ' + done.stderr.decode()[-8000:])
        actual = json.loads(result.read_text())
        visited, packets, reads, writes = set(), 0, 0, 0
        for case, got in zip(observed['cases'], actual, strict=True):
            row = rows[case['id']]
            if not case['completed'] or not case['live_x87_unchanged']:
                raise ValueError('Actual sample did not preserve its caller x87 stack')
            # The shared observer records explicit operands. This authenticated
            # RET16 also reads the unchanged entry SP before retiring.
            ret_read = dict(instruction=RET, address=row['gp'][4], bytes=4)
            want = dict(
                abi=dict(entry=RET, gp=case['boundary_gp'], eflags=case['boundary_eflags'],
                         xmm=case['boundary_xmm'], fp=case['after_fp'], live_x87=row['live_x87']),
                packets=[{k: v for k, v in packet.items() if k != 'arguments'} for packet in case['calls']],
                banks=sorted(case['banks'], key=lambda bank: bank['address']),
                reads=case['reads'] + [ret_read], writes=case['writes'])
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
                raise ValueError('Complete unchanged original sample differs: ' + row['name'] + ' ' + field)
            visited.update(case['executed_sites'])
            packets += len(case['calls'])
            reads += len(want['reads'])
            writes += len(case['writes'])
        sites = {int(value, 16) for value in re.findall(r'services.instruction\(0x([0-9a-f]+)u\)', body)}
        if len(sites) != 36 or not sites <= visited:
            raise ValueError('Missing authored original hull sample sites')
        report.update(passed=True, body_sha256=BODY_PIN, original_sites=len(sites), complete_original_calls=len(rows),
                      complete_parent_pre_io_packets=packets, architectural_reads=reads, complete_stores=writes,
                      hostile_provider_comparisons=len(rows), original_text_unmodified=True,
                      source_or_hardware_fault_domains_qualified=False)
    except Exception as error:
        report['error'] = repr(error)
    report['sources_unchanged'] = all(sha(ROOT / name) == value for name, value in pins.items())
    if not report['sources_unchanged']:
        report['passed'] = False
        report['source_drift'] = True
    a.report.parent.mkdir(parents=True, exist_ok=True)
    a.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: v for k, v in report.items() if k in ('passed', 'error', 'first_difference')}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
