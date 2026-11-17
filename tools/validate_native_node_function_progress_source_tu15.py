#!/usr/bin/env python3
"""Compare D3950 and its two genuine required-owner handoffs."""
import argparse
import json
import os
from pathlib import Path
import re
import shlex
import subprocess

from generated_routine_ownership import qualified_dispatch_entries
from specialize_native_operands import specialize
from validate_native_node_progress_query_source_tu15 import (
    INPUTS, PIN, ROOT, Xbe, corpus as node_inputs, prepare as prepare_node, sha)

UNIT = 'src/physics/physics_live_support_tu15_generated.cpp'
FRAME = 'src/gameplay/animation/native_tu15_node_function_progress_source_frame.hpp'
DRIVER = 'tests/native_tu15_node_function_progress_source_tests.cpp'
BODY = re.compile(r'^static void run_d3950\(WorldState &s\) \{.*?^\}', re.M | re.S)
BODY_PINS = ('16032edfa64985723047f8ff6ecd0d297bd3e63115c6079161ee8128c27bb607',
             '10c5174a33925cbd8d0944b349b129911dfba8c946199c0a38b17d385ac37842')
DISPATCH_PINS = ('5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0',
                 '37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052')


def corpus(image):
    rows = node_inputs(image)
    for index, row in enumerate(rows):
        banks = {bank['address']:bytearray.fromhex(bank['before']) for bank in row['banks']}
        banks[0x4ec000] = bytearray(4096)

        def put(address, value, width=4):
            for base, data in banks.items():
                offset = address-base
                if 0 <= offset <= len(data)-width:
                    data[offset:offset+width] = (value & ((1<<(width*8))-1)).to_bytes(width, 'little')
                    return
            raise ValueError('D3950 fixture escaped declared banks')

        holder = 0x61000a00
        offset = -0x100 if index % 2 else 0x100
        body = holder-offset
        pool, slot = 0x61000200, 0x61000300
        datum = (0, 1, 17, 65535)[index % 4]
        put(0x4ecbd0, pool)
        put(pool+0x44, slot-datum*12)
        put(slot+8, body)
        put(body+0x12a, offset, 2)
        route, invalid = index % 4, (index//4) % 5
        put(holder+0x7c, (0x700005c, 0x700005d, 0x700005e, 0x700005c)[route])
        if invalid == 2:
            put(holder, 0xffffffff)
        if invalid == 3:
            put(holder+6, 0xffff, 2)
        row['gp'][0] = 0xffffffff if route == 3 else 0xa55a0000 | datum
        row.update(entry=0xd3950, name=f'function-progress-{index}')
        row['banks'] = [dict(address=base, before=value.hex()) for base, value in sorted(banks.items())]
    return rows


def prepare(scratch, image):
    for pc, following in ((0xd3998, 0xd399d), (0xd39b3, 0xd39b8)):
        call = image.read(pc, 5)
        if call[0] != 0xe8 or following+int.from_bytes(call[1:], 'little', signed=True) != 0x1cf0e0:
            raise ValueError('Original D3950 child edge changed')
    raw = (ROOT/UNIT).read_text()
    output = ['static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){s.required(site,target,tail);}']
    sites = []
    for index, text in enumerate((raw, specialize(raw)[0])):
        bodies = BODY.findall(text)
        dispatchers = re.findall(r'^static void dispatch[^\n]+', text, re.M)
        if len(bodies) != 1 or sha(bodies[0]) != BODY_PINS[index]:
            raise ValueError('Complete D3950 body changed')
        if len(dispatchers) != 1 or sha(dispatchers[0]) != DISPATCH_PINS[index]:
            raise ValueError('Complete D3950 dispatcher changed')
        if '1cf0e0' in qualified_dispatch_entries(text, 'tu15') or 's.required(site,target,tail);' not in dispatchers[0]:
            raise ValueError('Actual D3950 required owner is not the authenticated external branch')
        body = bodies[0]
        sites.append(re.findall(r's\.(?:services\.instruction|instruction_literal)\((0x[0-9a-f]+u)\)', body))
        if body.count('target=0x1cf0e0u;') != 2:
            raise ValueError('Retained D3950 required owner edges changed')
        output.append(body.replace('run_d3950(', 'function_progress_'+('literal' if index else 'raw')+'(', 1))
    native = set(re.findall(r'0xd39[0-9a-f]{2}u', (ROOT/FRAME).read_text()))
    if len(set(sites[0])) != 37 or sites[0] != sites[1] or set(sites[0]) != native:
        raise ValueError('D3950 native/source callback sites changed')
    (scratch/'node_function_progress_reference.inc').write_text('\n'.join(output)+'\n')
    return dict(body_pins=BODY_PINS, dispatcher_pins=DISPATCH_PINS, native_sites=37,
                original_call_sites=['d3998','d39b3'], actual_child='1cf0e0', child_result_supplied=False,
                private_owner_routing=True, descriptor=prepare_node(scratch))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--compiler', action='append')
    parser.add_argument('--sanitizers', action='store_true')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    args.output.mkdir(parents=True, exist_ok=False)
    names = set(INPUTS) | {UNIT, FRAME, DRIVER, str(Path(__file__).relative_to(ROOT)),
        'tools/validate_native_node_progress_query_source_tu15.py'}
    before = {name:sha(ROOT/name) for name in sorted(names)}
    headers = {path.resolve():sha(path) for name in ('include','src','tests') for path in (ROOT/name).rglob('*')
               if path.suffix in ('.hpp','.h','.inc')}
    report = dict(passed=False, production_bound=False, owned_scratch=str(scratch), retained_large_artifacts=[], compilers=[])
    try:
        if sha(args.xbe) != PIN:
            raise ValueError('Wrong original TU1.5 image')
        image = Xbe(args.xbe)
        report['authority'] = prepare(scratch, image)
        fixture = scratch/'inputs.json'
        fixture.write_text(json.dumps(corpus(image), separators=(',', ':')))
        report['fixture_sha256'] = sha(fixture)
        for compiler in args.compiler or ['g++']:
            binary = scratch/Path(compiler).name
            dep = binary.with_suffix('.d')
            command = [compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                       *('-I'+str(ROOT/name) for name in ('include','src','tests')),'-I'+str(scratch),
                       str(ROOT/DRIVER),'-o',str(binary),'-MD','-MF',str(dep)]
            if args.sanitizers:
                command.extend(('-fsanitize=address,undefined','-fno-omit-frame-pointer'))
            built = subprocess.run(command, capture_output=True, text=True, timeout=1200)
            if built.returncode:
                raise ValueError('D3950 source compile failed: '+built.stderr[-7000:])
            for item in shlex.split(dep.read_text().replace(chr(92)+'\n',' ').split(':',1)[1]):
                path = Path(item).resolve()
                if path in headers:
                    before[str(path.relative_to(ROOT))] = headers[path]
            run = subprocess.run([str(binary),str(fixture)], capture_output=True, text=True, timeout=1200)
            if run.returncode:
                raise ValueError('D3950 source composition failed: '+(run.stdout+run.stderr)[-8000:])
            report['compilers'].append(dict(compiler=compiler, result=json.loads(run.stdout)))
        report['passed'] = True
    except BaseException as error:
        report['error'] = repr(error)
        raise
    finally:
        report['source_before'] = before
        report['source_after'] = {name:sha(ROOT/name) for name in before}
        if report['source_after'] != before:
            report.update(passed=False, source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
