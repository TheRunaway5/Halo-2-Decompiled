#!/usr/bin/env python3
"""Measure complete original SDK numerical helpers on the host32-bit CPU.

Actual initialized xemu calls are independently replayed and must agree first.
Additional explicitly declared ABI probes supply only scalar/vector argument
backing. All original helper and callee instructions execute unchanged, with
hardware return boundaries and no supplied SDK/kernel/callee results. These
are numerical contracts, not original DSP, host playback or additional game
events. Kernel addresses remain original32-bit identities, never host pointers.
"""
import argparse
import atexit
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import random

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from original_audio_cpu import OriginalAudioCpu

OWNERS = {
    'position': (0x37debf, 5, [(1, 12), (2, 12)], [(3, 12), (4, 4)]),
    'angles': (0x37def9, 8, [(1, 12), (3, 12), (4, 12)], [(6, 4), (7, 4)]),
    'front_rear': (0x37dfe1, 7, [], [(5, 4), (6, 4)]),
    'kernel': (0x37e098, 4, [], [(3, 8)]),
}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', type=Path, action='append', required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    image_path = ROOT / 'assets/disc/default.xbe'
    assert sha(image_path) == '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
    image = Xbe(image_path)
    layout = args.output / 'user-layout.c'
    layout.write_text('#include <stddef.h>\n#include <sys/user.h>\n_Static_assert(offsetof(struct user,u_debugreg)==848,"Unsupported ptrace ABI");\nint main(void){return 0;}\n')
    subprocess.run(['cc', str(layout), '-o', str(args.output / 'user-layout')], check=True)
    subprocess.run([str(args.output / 'user-layout')], check=True)
    captures = []
    for path in args.capture:
        report = json.loads((path / 'report.json').read_text())
        assert report['image_sha256'] == sha(image_path)
        assert not any(report[key] for key in ('instruction_patches', 'guest_memory_writes',
                       'guest_register_writes', 'supplied_callee_returns'))
        captures.append((path, report))
    first = captures[0][1]['registers']
    host = OriginalAudioCpu(image, args.output / 'original32.elf', first['fctrl'], first['mxcsr'])
    atexit.register(host.close)
    rows = []

    def measure(owner, raw_args, backing, fp):
        entry, count, inputs, outputs = OWNERS[owner]
        assert len(raw_args) == count
        host.control_word, host.mxcsr = fp
        words = list(raw_args)
        for index, size in inputs + outputs:
            words[index] = 0x1001000 + index * 0x100
            initial = backing[index]
            assert len(initial) == size
            host.write(words[index], initial)
        stack = 0x100e000
        host.write(stack, struct.pack('<' + 'I' * (count + 1), 0x3000000, *words))
        result = host.run(entry, stack)
        assert result.rip == 0x3000000 and result.rsp == stack + 4 + 4 * count
        for index, size in inputs:
            assert host.read(words[index], size) == backing[index]
        return {str(index): host.read(words[index], size).hex() for index, size in outputs}

    def emit(label, actual, owner, words, backing, fp, expected=None):
        output = measure(owner, words, backing, fp)
        if expected is not None:
            assert output == expected, (label, output, expected)
        rows.append(dict(label=label, actual_initialized_call=actual, owner=owner,
                    scalar_and_pointer_words=[f'{value:08x}' for value in words],
                    backing_hex={str(key): value.hex() for key, value in backing.items()},
                    x87_control_word=fp[0], mxcsr=fp[1], output_hex=output))

    for path, capture in captures:
        owner = capture['owner']
        words = [int(value, 16) for value in capture['argument_words']]
        backing = {region['argument']: bytes.fromhex(region['before_hex']) for region in capture['regions']}
        expected = {str(region['argument']): region['after_hex'] for region in capture['regions'] if region['role'] == 'output'}
        context = capture['registers']
        emit(path.name, True, owner, words, backing, (context['fctrl'], context['mxcsr']), expected)
    fp = (first['fctrl'], first['mxcsr'])
    values = [bits(value) for value in (0., -0., 1., -1., .001, .5, 2., 45., 90., 180.)]
    values += [1, 0x00800000, 0x7f7fffff, 0x7f800000, 0xff800000, 0x7fc12345]
    vec = lambda *words: struct.pack('<3I', *words)
    for mode in (0, 1, 2, 0xffffffff):
        for x in values:
            for z in values:
                for listener in (vec(0, 0, 0), vec(bits(.25), bits(3.), bits(-.5))):
                    emit(f'position_{mode:08x}_{x:08x}_{z:08x}_{listener.hex()}', False, 'position',
                         [mode, 0, 0, 0, 0], {1: listener, 2: vec(x, bits(7.), z),
                                             3: bytes.fromhex('112233445566778899aabbcc'), 4: bytes(4)}, fp)
    template = next(capture for _, capture in captures if capture['owner'] == 'angles')
    basis = {region['argument']: bytes.fromhex(region['before_hex']) for region in template['regions']}
    for mode in (0, 1, 2):
        for x in values:
            for z in values:
                for distance in (0, bits(1.)):
                    backing = dict(basis)
                    backing.update({4: vec(x, 0, z), 6: bytes(4), 7: bytes(4)})
                    emit(f'angles_{mode}_{x:08x}_{z:08x}_{distance:08x}', False, 'angles',
                         [mode, 0, 0, 0, 0, distance, 0, 0], backing, fp)
    for enabled in (0, 1, 2, 0xffffffff):
        for scale in (0, bits(1.), bits(.5), bits(2.), 0x7fc12345):
            for angle in values:
                emit(f'front_rear_{enabled:08x}_{scale:08x}_{angle:08x}', False, 'front_rear',
                     [bits(1.), scale, angle, 0, enabled, 0, 0], {5: bytes(4), 6: bytes(4)}, fp)
    for speaker in (0, 1, 2, 0xffffffff):
        angles = sorted(set(values + [bits(i / 8) for i in range(-1456, 1457)]))
        for angle in angles:
            emit(f'kernel_{speaker:08x}_{angle:08x}', False, 'kernel',
                 [angle, 0, speaker, 0], {3: bytes(8)}, fp)
    # Additional ABI controls exercise non-axis-aligned bases and products
    # outside the earlier unit-distance branch. This private probe RNG is not
    # the game's scheduling/variation RNG and supplies no initialized state.
    probe = random.Random(0x53444b)
    for i in range(512):
        basis_words = [bits(probe.randrange(-16000, 16001) / 997) for _ in range(9)]
        backing = {1: vec(*basis_words[:3]), 3: vec(*basis_words[3:6]),
                   4: vec(*basis_words[6:]), 6: bytes(4), 7: bytes(4)}
        emit(f'rotated_basis_{i}', False, 'angles', [i % 3, 0, 0, 0, 0, bits(1.), 0, 0], backing, fp)
    extended_values = values + [0x7f812345, 0xffc12345, 0xff7fffff]
    for distance in extended_values:
        for scale in extended_values:
            for angle in (bits(30.), bits(90.), bits(150.), 0x7f812345):
                emit(f'volume_product_{distance:08x}_{scale:08x}_{angle:08x}', False, 'front_rear',
                     [distance, scale, angle, 0, 1, 0, 0], {5: bytes(4), 6: bytes(4)}, fp)
    # Explicit caller contexts are numerical controls, not evidence that a
    # running game chooses these non-captured precision/rounding/DAZ/FTZ modes.
    contexts = [(0x003f, 0x1f80), (0x063f, 0x3f80), (0x0a3f, 0x5f80),
                (0x0e3f, 0x7f80), (0x033f, 0x9fc0)]
    for context in contexts:
        for i, value in enumerate(extended_values):
            other = extended_values[(i + 7) % len(extended_values)]
            suffix = f'{context[0]:04x}_{context[1]:04x}_{value:08x}'
            emit('fp_position_' + suffix, False, 'position', [i % 3, 0, 0, 0, 0],
                 {1: vec(bits(.25), 0, bits(-.5)), 2: vec(value, 0, other), 3: bytes(12), 4: bytes(4)}, context)
            emit('fp_angles_' + suffix, False, 'angles', [i % 3, 0, 0, 0, 0, bits(1.), 0, 0],
                 {1: vec(bits(.6), 0, bits(.8)), 3: vec(bits(-.8), 0, bits(.6)),
                  4: vec(value, other, value), 6: bytes(4), 7: bytes(4)}, context)
            emit('fp_volume_' + suffix, False, 'front_rear', [value, other, bits(120.), 0, 1, 0, 0],
                 {5: bytes(4), 6: bytes(4)}, context)
            emit('fp_kernel_' + suffix, False, 'kernel', [value, 0, i % 2, 0], {3: bytes(8)}, context)
    host.close()
    fixture = args.output / 'fixtures.json'
    literals = {f'{address:08x}': image.read(address, size).hex() for address, size in
                [(0x45dbd8, 4), (0x45dbc0, 4), (0x45dbbc, 4), (0x417238, 4),
                 (0x4171f8, 4), (0x4171e8, 4), (0x4171ec, 4), (0x45dfcc, 4),
                 (0x45e320, 4), (0x45dccc, 4), (0x385a48, 12), (0x385804, 12)]}
    fixture.write_text(json.dumps(dict(schema='ORIGINAL_SOUND_SDK_NUMERICAL_V1',
                       original_literal_bytes=literals, cases=rows), separators=(',', ':')) + '\n')
    report = dict(boundary=__doc__, image_sha256=sha(image_path), tool_sha256=sha(__file__),
                  host_runner_sha256=sha(ROOT / 'tools/original_audio_cpu.py'),
                  original_elf_sha256=sha(args.output / 'original32.elf'), fixture_sha256=sha(fixture),
                  actual_captures=[dict(path=str(path), sha256=sha(path / 'report.json')) for path, _ in captures],
                  actual_call_replays_equal=True, actual_calls=len(captures),
                  declared_cases=len(rows) - len(captures),
                  by_owner={owner: sum(row['owner'] == owner for row in rows) for owner in OWNERS},
                  instruction_patches=[], supplied_callee_returns=[], expected_values_from_native=False,
                  native_or_audible_parity_claimed=False)
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'Original SDK CPU: {len(captures)} actual calls replay exactly; '
          f'{len(rows) - len(captures)} declared ABI cases; no native/DSP playback claim')


if __name__ == '__main__':
    main()
