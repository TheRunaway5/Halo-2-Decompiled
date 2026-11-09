#!/usr/bin/env python3
"""Compare native collision transforms against authenticated translated bodies.

Run through tools/run_test.py; all compiler outputs stay in its owned scratch.
"""
import argparse
import ast
import hashlib
import json
import os
from pathlib import Path
import subprocess
import struct
import sys

from specialize_native_operands import LEAVES, digest, leaf_body, specialize


def qualify_hardware(scratch, entries=(0x2e44d0, 0x2e46e0), global_patches=(), initial_x87_status=0,fs_base=None,extra_assembly=''):
    root = Path(__file__).resolve().parents[1]
    sys.path.insert(0, str(root / 'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    harness = ast.parse((root / 'tools/qualify_loop_combined_publication_hardware_tu15.py').read_text())
    ASM = next(ast.literal_eval(node.value) for node in harness.body if isinstance(node, ast.Assign)
        and any(isinstance(target, ast.Name) and target.id == 'ASM' for target in node.targets))
    # Read the existing reference owner's constants without importing its
    # unrelated emulator/disassembler dependencies into this hardware test.
    owner = ast.parse((root / 'tools/research_mp_sandbox_update_functions.py').read_text())
    constants = {target.id: node.value for node in owner.body if isinstance(node, ast.Assign)
        for target in node.targets if isinstance(target, ast.Name)}
    NEW_DEFAULT = Path(ast.literal_eval(constants['NEW_DEFAULT'].args[0]))
    NEW_PIN = ast.literal_eval(constants['NEW_PIN'])
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    original = bytearray(0x800000)
    for section in image.sections:
        raw = image.section_bytes(section)
        original[section.va-0x10000:section.va-0x10000+len(raw)] = raw
    (scratch / 'original.bin').write_bytes(original)
    captures = 0
    for number, entry in enumerate(entries):
        assembly = ASM.replace('ORIGINAL_IMAGE', str(scratch / 'original.bin'))
        assembly = assembly.replace('entry: .long 0x2216d0', f'entry: .long 0x{entry:x}')
        if fs_base is not None:
            assembly=assembly.replace('_start:\n mov $harness_stack_end,%esp',
                '_start:\n mov $243,%eax\n mov $temporary_tls_desc,%ebx\n int $0x80\n'
                ' test %eax,%eax\n js failure\n mov temporary_tls_desc,%eax\n shl $3,%eax\n'
                ' or $3,%eax\n mov %ax,%fs\n mov $harness_stack_end,%esp')
            assembly+='\n.section .bridge,"awx"\n.balign 16\ntemporary_tls_desc: .long -1,'+str(fs_base)+',0xfffff,0x51\n'
        assembly+=extra_assembly
        assembly = assembly.replace(' mov $OUTPUT_ADDRESS,%ecx\n mov $80,%edx\n call write_exact\n'
            ' mov $CALLER_ARGUMENTS,%ecx\n mov $16,%edx\n call write_exact',
            ' mov $0x1000000,%ecx\n mov $65536,%edx\n call write_exact')
        (scratch / 'reference.s').write_text(assembly)
        (scratch / 'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n.original 0x10000 : { *(.original) }\n'
            '.banks 0x1000000 : { *(.banks) }\n.return 0x103f000 : { *(.return) }\n.bridge 0x70000000 : { *(.bridge) }\n}\n')
        for command in (
            ['as', '--32', str(scratch / 'reference.s'), '-o', str(scratch / 'reference.o')],
            ['ld', '-melf_i386', '-T', str(scratch / 'link.ld'), str(scratch / 'reference.o'), '-o', str(scratch / 'original-hardware')],
            ['objcopy', '--dump-section', '.original=' + str(scratch / 'verified.bin'), str(scratch / 'original-hardware')]):
            subprocess.run(command, check=True, capture_output=True)
        assert (scratch / 'verified.bin').read_bytes() == original
        record = 168 + 65536
        fixtures = (scratch / f'hardware-{number}.bin').read_bytes()
        assert fixtures and len(fixtures) % (record * 2) == 0
        fp_file=scratch/f'hardware-fp-{number}.bin'
        fp_states=list(struct.iter_unpack('<HH',fp_file.read_bytes())) if fp_file.exists() else []
        payload = bytearray(struct.pack('<I', 0))
        expected = []
        for offset in range(0, len(fixtures), record * 2):
            initial = fixtures[offset:offset+record]
            expected.append(fixtures[offset+record:offset+record*2])
            payload += struct.pack('<III', 1+len(global_patches), 0x1000000, 65536) + initial[168:]
            for address,raw in global_patches:
                payload+=struct.pack('<II',address,len(raw))+raw
            payload += struct.pack('<I', 0)
            environment = bytearray(108)
            struct.pack_into('<H', environment, 0, fp_states[len(expected)-1][0] if fp_states else 0x027f)
            struct.pack_into('<H', environment, 4, initial_x87_status)
            struct.pack_into('<H', environment, 8, 0xffff)
            payload += initial[:40] + environment + initial[40:168]
        payload += struct.pack('<I', 0)
        result = subprocess.run([str(scratch / 'original-hardware')], input=payload,
            capture_output=True, timeout=45)
        assert result.returncode == 0, (entry, result.returncode, result.stderr[:512])
        assert len(result.stdout) == (276+65536) * len(expected)
        for index, native in enumerate(expected):
            raw = result.stdout[index*(276+65536):(index+1)*(276+65536)]
            flags = struct.unpack_from('<I', raw, 32)[0] & 0x8d5
            normalized = raw[:32] + struct.pack('<I', flags) + raw[36:40] + raw[148:]
            assert normalized == native, ('unchanged XBE hardware mismatch', hex(entry), index,
                [(i, a, b) for i, (a, b) in enumerate(zip(normalized, native)) if a != b][:12])
            if fp_states:
                status=struct.unpack_from('<H',raw,44)[0]
                assert status&0x23f==fp_states[index][1]&0x23f,('x87 status',hex(entry),index,
                    hex(status),hex(fp_states[index][1]))
            captures += 1
    report=dict(cases=captures, original_xbe_sha256=NEW_PIN,
        backend='unchanged original TU1.5 instructions on Linux i386 CPU',
        original_instruction_patches=[], substituted_calls=[],
        native_outputs_used_by_original=False, xbox_hardware_proof=False)
    if extra_assembly:report['additional_native_service_assembly_sha256']=hashlib.sha256(extra_assembly.encode()).hexdigest()
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py to own compiler outputs')
    root = Path(__file__).resolve().parents[1]
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    source = root / 'src/physics/collision/source_collision_query_tu15_generated.cpp'
    canonical = source.read_text()
    specialized, receipt = specialize(canonical)
    names = ('2e44d0', '2e46e0')
    bodies = {'legacy': [], 'reference': [], 'production': []}
    for name in names:
        original = leaf_body(canonical, name)
        assert digest(original[0].replace(original[1] + ' &s', 'STATE &s')) == LEAVES[name][0]
        assert name in receipt['native_leaves']
        bodies['legacy'].append(original[0])
        # Pin the recovered instruction's destination operand: the compiler
        # otherwise swaps dual-NaN operands after instruction callbacks vanish.
        # Unchanged XBE hardware below independently verifies this edge cohort.
        bodies['reference'].append(original[0].replace('_mm_add_ps(',
            'native_tu15_leaves::ordered_add_ps(').replace('_mm_mul_ps(',
            'native_tu15_leaves::ordered_mul_ps('))
        bodies['production'].append(leaf_body(specialized, name)[0])
    (scratch / 'collision_matrix_leaf_bodies.inc').write_text('\n'.join(
        'namespace ' + namespace + ' {\n' + '\n'.join(content) + '\n}'
        for namespace, content in bodies.items()))
    binary = scratch / 'collision_matrix_leaves'
    command = ['c++', '-std=c++20', '-O2', '-ffp-contract=off', '-fno-fast-math',
        '-frounding-math', '-I' + str(root / 'include'), '-I' + str(root / 'src'),
        '-I' + str(scratch), str(root / 'tests/native_tu15_collision_matrix_leaves_tests.cpp'),
        '-o', str(binary)]
    subprocess.run(command, check=True)
    result = subprocess.run([str(binary), str(scratch)], text=True, capture_output=True)
    if result.returncode:
        sys.stderr.write(result.stderr)
        result.check_returncode()
    report = json.loads(result.stdout)
    report['hardware_reference'] = qualify_hardware(scratch)
    modes = {}
    for mode in ('HALO2_REFERENCE_MATRIX_LEAVES', 'HALO2_VERIFY_MATRIX_LEAVES'):
        environment = dict(os.environ)
        environment.pop('HALO2_REFERENCE_MATRIX_LEAVES', None)
        environment.pop('HALO2_VERIFY_MATRIX_LEAVES', None)
        environment[mode] = '1'
        checked = subprocess.run([str(binary)], env=environment, text=True, capture_output=True)
        if checked.returncode:
            sys.stderr.write(checked.stderr)
            checked.check_returncode()
        modes[mode] = json.loads(checked.stdout)
    report['reference_and_runtime_verify_modes'] = modes
    report.update(command=command, source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
        native_header_sha256=hashlib.sha256((root / 'src/physics/runtime/native_tu15_matrix_leaves.hpp').read_bytes()).hexdigest(),
        native_leaves=receipt['native_leaves'], reference_bodies_authenticated=True,
        translated_finite_cases=1152, instruction_destination_pinned_ieee_cases=384,
        comparisons=['all memory bytes', 'ordered reads', 'ordered stores and before/written bytes',
            'all general and vector registers', 'all arithmetic flags', 'MXCSR status'],
        live_frame_qualification=False,exceptional_partial_abi_qualification=False,
        scratch_directory=str(scratch),retained_large_artifacts=[])
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
