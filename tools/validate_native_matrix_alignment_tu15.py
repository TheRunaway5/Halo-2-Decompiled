#!/usr/bin/env python3
"""Compare native MOVAPS fault state with unchanged original i386 instructions."""
import argparse
import ast
import hashlib
import json
import os
from pathlib import Path
import platform
import random
import struct
import subprocess
import sys

from specialize_native_operands import leaf_body, specialize

ROOT = Path(__file__).resolve().parents[1]
BASE, SIZE, STOP = 0x1000000, 65536, 0x63000000


def original_image():
    tree = ast.parse((ROOT / 'tools/research_mp_sandbox_update_functions.py').read_text())
    values = {target.id: node.value for node in tree.body if isinstance(node, ast.Assign)
              for target in node.targets if isinstance(target, ast.Name)}
    path = Path(ast.literal_eval(values['NEW_DEFAULT'].args[0]))
    pin = ast.literal_eval(values['NEW_PIN'])
    if hashlib.sha256(path.read_bytes()).hexdigest() != pin:
        raise ValueError('Original TU1.5 image authentication failed')
    sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    return Xbe(path), pin


def physical(image, rows, scratch):
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    common.ENTRIES = (0x2e44d0, 0x2e46e0)
    common.geometry.STOP = STOP
    metadata = common.metadata(image)
    code = common._Code(image, '.text')
    for site in metadata['sites']:
        if code.at(site['instruction']).mnemonic == 'push':
            site['operands'].append(dict(base=4, index=-1, scale=1,
                displacement=0xfffffffc, bytes=4, read=False, write=True))
    section = image.section('.text')
    raw = image.section_bytes(section)
    (scratch / 'text.bin').write_bytes(raw)
    (scratch / 'metadata.json').write_text(json.dumps(metadata))
    (scratch / 'original.s').write_text('.code32\n.section .original,"ax"\n.incbin "' +
        str(scratch / 'text.bin') + '"\n.section .bank,"aw"\n.space 65536\n'
        '.section .stop,"ax"\nint3\n.section .bridge,"ax"\n.globl _start\n_start:\nfninit\nint3\njmp _start\n')
    (scratch / 'link.ld').write_text(f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} '
        '.bank 0x1000000:{*(.bank)} .stop 0x63000000:{*(.stop)} .bridge 0x70000000:{*(.bridge)}}\n')
    commands = [
        ['as', '--32', str(scratch / 'original.s'), '-o', str(scratch / 'original.o')],
        ['ld', '-melf_i386', '-T', str(scratch / 'link.ld'), str(scratch / 'original.o'), '-o', str(scratch / 'original')],
        ['objcopy', '--dump-section', '.original=' + str(scratch / 'verified.bin'), str(scratch / 'original')],
        ['c++', '-std=c++20', '-O2', str(ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp'),
         '-o', str(scratch / 'observer')],
    ]
    for command in commands:
        subprocess.run(command, check=True, capture_output=True)
    if (scratch / 'verified.bin').read_bytes() != raw:
        raise ValueError('Linked reference changed original instruction bytes')
    command = [str(scratch / 'observer'), str(scratch / 'original'), str(scratch / 'metadata.json'),
               str(scratch / 'inputs.json'), str(scratch / 'physical.json')]
    result = subprocess.run(command, text=True, capture_output=True, timeout=120)
    if result.returncode:
        raise ValueError('Physical alignment observer failed: ' + (result.stdout + result.stderr)[-3000:])
    proof = json.loads((scratch / 'physical.json').read_text())
    if proof['instruction_patches'] or proof['supplied_original_outcomes']:
        raise ValueError('Original reference supplied instruction/callee outcomes')
    return proof, commands + [command]


def split_stores(stores):
    result = []
    for event in stores:
        if len(event['written']) == 32:
            for offset in (0, 8):
                result.append(dict(instruction=event['instruction'], address=event['address']+offset,
                    before=event['before'][offset*2:offset*2+16], written=event['written'][offset*2:offset*2+16]))
        else:
            result.append(event)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    if sys.platform != 'linux' or platform.machine().lower() not in ('x86_64', 'amd64', 'i386', 'i686'):
        print('SKIP unchanged i386 alignment reference requires Linux x86 ptrace')
        return 77
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py to own binaries and raw reference banks')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    image, pin = original_image()
    random_source = random.Random(0x2e4522)
    rows = []
    for entry in (0x2e44d0, 0x2e46e0):
        for kind in ('aligned', 'destination', 'matrix', 'point'):
            for offset in ((0,) if kind == 'aligned' else range(1, 16)):
                data = bytearray(SIZE)
                a, b, out = BASE+0x1000, BASE+0x2000, BASE+0x3000
                for index in range(16):
                    struct.pack_into('<f', data, a-BASE+index*4, (index+1)/16)
                    struct.pack_into('<f', data, b-BASE+index*4, (index+3)/8)
                gp = [random_source.getrandbits(32) for _ in range(8)]
                gp[1] = out + (offset if kind == 'destination' else 0)
                gp[4] = BASE+0xe000+(offset%4)*4
                first = a+(offset if kind == 'matrix' else 0)
                second = b+(offset if kind == 'point' else 0)
                struct.pack_into('<III', data, gp[4]-BASE, STOP, first, second)
                rows.append(dict(id=len(rows), name=f'{entry:x}-{kind}-{offset}', entry=entry,
                    stop_pc=STOP, return_pc=STOP, matrix=first, source=second, gp=gp,
                    eflags=random_source.getrandbits(32)&0x8d5, live_x87=[],
                    before_fp=[0x27f, 0, 0x1f80|((offset%4)<<13)],
                    xmm=[[struct.unpack('<I', struct.pack('<f', float(i+1)))[0] for _ in range(4)] for i in range(8)],
                    alignment_fault=kind!='aligned', max_steps=1000,
                    banks=[dict(address=BASE, before=data.hex())]))
    (scratch / 'inputs.json').write_text(json.dumps(rows, separators=(',', ':')))
    source = ROOT / 'src/physics/collision/source_collision_query_tu15_generated.cpp'
    specialized, receipt = specialize(source.read_text())
    names = ('2e44d0', '2e46e0')
    if not all(name in receipt['native_leaves'] for name in names):
        raise ValueError('Native collision alignment entries were not installed')
    (scratch / 'matrix_alignment_bodies.inc').write_text('namespace production {\n' +
        '\n'.join(leaf_body(specialized, name)[0] for name in names) + '\n}\n')
    binary = scratch / 'native-alignment'
    command = ['c++', '-std=c++20', '-O2', '-ffp-contract=off', '-fno-fast-math', '-frounding-math',
        '-pthread', '-I'+str(ROOT / 'include'), '-I'+str(ROOT / 'src'), '-I'+str(scratch),
        str(ROOT / 'tests/native_tu15_matrix_alignment_tests.cpp'), '-o', str(binary)]
    subprocess.run(command, check=True)
    environment = {key: value for key, value in os.environ.items() if key not in (
        'HALO2_REFERENCE_MATRIX_LEAVES', 'HALO2_VERIFY_MATRIX_LEAVES')}
    subprocess.run([str(binary), str(scratch / 'inputs.json'), str(scratch / 'native.json')], env=environment, check=True)
    native = json.loads((scratch / 'native.json').read_text())
    original, commands = physical(image, rows, scratch)
    checks = []
    for row, observed, actual in zip(rows, original['cases'], native, strict=True):
        expected_done = not row['alignment_fault']
        same = (observed['completed'] == actual['completed'] == expected_done and
                observed['boundary_gp'] == actual['gp'] and observed['boundary_xmm'] == actual['xmm'] and
                (observed['boundary_eflags']&0x8d5) == (actual['eflags']&0x8d5) and
                observed['after_fp'] == actual['fp'] and observed['banks'][0]['after'] == actual['after'] and
                split_stores(observed['writes']) == actual['writes'] and actual['instruction_callbacks'] == 0)
        if row['alignment_fault']:
            same = same and observed['boundary']['phase'] == 'alignment_fault' and observed['boundary']['instruction'] == actual['pc']
        if not same:
            raise ValueError('Native/original alignment boundary mismatch: ' + row['name'] + ' ' + json.dumps({
                'original_gp': observed['boundary_gp'], 'native_gp': actual['gp'],
                'original_flags': observed['boundary_eflags']&0x8d5, 'native_flags': actual['eflags']&0x8d5,
                'original_fp': observed['after_fp'], 'native_fp': actual['fp'],
                'original_boundary': observed['boundary'], 'native_pc': actual['pc'],
                'store_match': split_stores(observed['writes']) == actual['writes']}))
        checks.append(dict(name=row['name'], alignment_fault=row['alignment_fault'], passed=True,
            original_pc=None if expected_done else observed['boundary']['instruction'],
            final_bank_sha256=hashlib.sha256(bytes.fromhex(actual['after'])).hexdigest()))
    report = dict(passed=True, schema='native-matrix-alignment-hardware-v1', cases=checks,
        original_xbe_sha256=pin, original_instruction_patches=[], supplied_original_outcomes=False,
        original_instructions=original['original_instructions'], normal_returns=2, alignment_faults=90,
        comparisons=['fault/completion outcome', 'all GP/XMM/arithmetic flags', 'x87/MXCSR controls and status',
                     'all bank bytes', 'complete ordered store prefix', 'exact failing original PC'],
        native_instruction_callbacks=0, source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
        kernel_sha256=hashlib.sha256((ROOT / 'src/physics/runtime/native_tu15_matrix_leaves.hpp').read_bytes()).hexdigest(),
        commands=[command, *commands], scratch=str(scratch), retained_large_artifacts=[],
        scope='Linux i386 original execution; two collision matrix leaves only', goal_complete=False)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')
    print(json.dumps({'passed': True, 'normal_returns': 2, 'alignment_faults': 90, 'report': str(args.report)}))


if __name__ == '__main__':
    raise SystemExit(main())
